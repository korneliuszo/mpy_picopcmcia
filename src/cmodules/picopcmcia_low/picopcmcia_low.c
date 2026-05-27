// Include MicroPython API.
#include "py/runtime.h"
#include "hardware/pio.h"
#include "py/ringbuf.h"
#include "picopcmcia_proto.h"

#include "shared/runtime/mpirq.h"

#define IRQHANDLERS_SIZE 5

// it should be params of init, def is easier for now
#define PIO_SEL_INSTANCE 0
#define SM_MACHINE 0
#define PIO_IRQ PIO0_IRQ_0

static uint8_t * l1_sel[8192]={};

static uint8_t l2_tables[7][32768];

MP_REGISTER_ROOT_POINTER(void * mpy_global_irq_callback[IRQHANDLERS_SIZE]);


//not in header
typedef struct _micropython_ringio_obj_t {
    mp_obj_base_t base;
    ringbuf_t ringbuffer;
} micropython_ringio_obj_t;
//

MP_REGISTER_ROOT_POINTER(mp_obj_t mpy_global_trace);


static void __not_in_flash_func(picopcmcia_trace_put(uint32_t mux0,uint32_t mux1,uint32_t idx))
{
    micropython_ringio_obj_t * trace = MP_STATE_PORT(mpy_global_trace);
    if(trace)
    {
        static uint16_t frame_idx = 0; // provides always smallint
        uint32_t buf[] =  {frame_idx++,mux0,mux1,idx};
        ringbuf_t  rng = trace->ringbuffer;
        if(ringbuf_free(&rng)>=sizeof(buf))
        {
            ringbuf_memcpy_put_internal(&rng, (uint8_t *)buf, sizeof(buf));
            trace->ringbuffer.iput = rng.iput;
        }
    }
}

static void __not_in_flash_func(picopcmcia_isr_call(uint32_t mux0,uint32_t mux1,uint32_t idx))
{
    mp_picopcmcia_low_c_worker_obj_t *irq = MP_STATE_PORT(mpy_global_irq_callback)[idx];
    if(irq && irq->fn)
    {
        uint32_t ret = irq->fn(irq,mux0,mux1,idx);
        pio_sm_put(PIO_INSTANCE(PIO_SEL_INSTANCE),SM_MACHINE,ret);
    }
    else
    {
        pio_sm_put(PIO_INSTANCE(PIO_SEL_INSTANCE),SM_MACHINE,0);
    }
}


// add into ram section ... prefferably exclusive to running core
// callback outside self immutable object field tuple needs core1 started mpy way for atomics to work
// 
static void __not_in_flash_func(picopcmcia_irq)()
{
    uint32_t TMP2,TMP3;
    uint32_t mux0;
    uint32_t mux1;
    uint32_t idx;
    uint8_t ** out;
    PIO isa_pio;


    asm volatile goto(
        "ldr %[PIOB], =%[PIOV]\n"
        "ldr  %[MUX0],[%[PIOB],%[PIOR]] \n\t" // read ISA_TRANSACTION from PIO
        //nops could be needed here ... remove isr entry latency
        //"nop\n"
        "lsr %[TMP],%[MUX0],#10\n"
        //"nop\n"
        "lsl %[TMP2],%[MUX0],#(32-26+8)\n"
        //"nop\n"
        "lsr %[TMP2],%[TMP2],#(32-15)\n"
        //"nop\n"
        "ldr %[TMP3], =%[FLM]\n"
        //"nop\n"
        "ldr %[OUT], =%[L1T] \n\t"
        "ldr  %[MUX1],[%[PIOB],%[PIOR]] \n\t" // read ISA_TRANSACTION from PIO

        "ands %[TMP3],%[TMP3],%[MUX1]\n"
        "orrs %[TMP3],%[TMP3],%[TMP]\n"
        "lsr %[TMP3],%[TMP3],#8\n"
        "lsl %[TMP3],%[TMP3],#2\n"

        "ldr %[TMP],[%[OUT],%[TMP3]]\n"
        "tst %[TMP],%[TMP]\n"
        "beq 1f\n"
        "ldrb %[TMP],[%[TMP],%[TMP2]]\n"
        "ldr %[TMP3], =%[L3] \n\t"
        "ands %[TMP3],%[TMP3],%[TMP]\n"
		"str    %[TMP3],[%[PIOB],%[PIOT]] \n\t" // pio->txf[0] = pin status;
        "beq 2f\n"

        "lsr %[TMP],%[TMP],#2\n"
        "b %l[us]\n\t"
        "1:\n"
		"str    %[TMP],[%[PIOB],%[PIOT]] \n\t" // pio->txf[0] = pin status;
        "2:\n"
        "b %l[not_us]\n\t" // end of loop
        "\n"
        ".ltorg\n\t"
    :
        [MUX0]"=l" (mux0),
        [MUX1]"=l" (mux1),
        [OUT]"=l" (out),
        [TMP]"=l" (idx),
        [TMP2]"=l" (TMP2),
        [TMP3]"=l" (TMP3),
        [PIOB]"=l"(isa_pio)
    :
        [L1T]"i" (l1_sel),
        [L3]"i"(3),
        [FLM]"i" (0x1F0000),
        [PIOV]"i" (pio0),
        [PIOT]"i" (PIO_TXF0_OFFSET),
        [PIOR]"i" (PIO_RXF0_OFFSET)
        : "cc"
        : us,not_us
    );
    us:
    picopcmcia_trace_put(mux0,mux1,idx);
    picopcmcia_isr_call(mux0,mux1,idx);
    not_us:
    return;
}


static mp_obj_t picopcmcia_get_l2(mp_obj_t idx_obj) {

    if (!mp_obj_is_small_int(idx_obj)) {
        mp_raise_TypeError_int_conversion(idx_obj);
    }
    uint32_t idx = MP_OBJ_SMALL_INT_VALUE(idx_obj);

    return mp_obj_new_bytearray_by_ref(sizeof(l2_tables[0])>>0, l2_tables[idx]);
}

static MP_DEFINE_CONST_FUN_OBJ_1(picopcmcia_get_l2_obj, picopcmcia_get_l2);

static mp_obj_t picopcmcia_set_l1_entry(mp_obj_t idx_obj, mp_obj_t l2idx_obj) {

    if (!mp_obj_is_small_int(idx_obj)) {
        mp_raise_TypeError_int_conversion(idx_obj);
    }
    if (!mp_obj_is_small_int(l2idx_obj)) {
        mp_raise_TypeError_int_conversion(l2idx_obj);
    }

    uint32_t idx = MP_OBJ_SMALL_INT_VALUE(idx_obj);
    uint32_t l2idx = MP_OBJ_SMALL_INT_VALUE(l2idx_obj);

    l1_sel[idx] = l2_tables[l2idx];

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_2(picopcmcia_set_l1_entry_obj, picopcmcia_set_l1_entry);

static mp_obj_t picopcmcia_set_trace(mp_obj_t ringio_obj) {

    MP_STATE_PORT(mpy_global_trace) = MP_OBJ_TO_PTR(ringio_obj);
    return mp_const_none;   
}

static MP_DEFINE_CONST_FUN_OBJ_1(picopcmcia_set_trace_obj, picopcmcia_set_trace);

static mp_obj_t picopcmcia_set_irq(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_idx, ARG_handler };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_idx, MP_ARG_INT, {.u_int = 0x0} },
        { MP_QSTR_handler, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
    };

    // Parse the arguments.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint32_t idx = mp_obj_get_int(pos_args[0]);
    //TODO: check oob and raise exception

    // Get the IRQ object.

    if (n_args > 1 || kw_args->used != 0) {
        // Configure IRQ.
        if (args[ARG_handler].u_obj == mp_const_none)
            MP_STATE_PORT(mpy_global_irq_callback)[idx] = NULL;
        else
            MP_STATE_PORT(mpy_global_irq_callback)[idx] = MP_OBJ_TO_PTR(args[ARG_handler].u_obj);
    }
    mp_picopcmcia_low_c_worker_obj_t *irq = MP_STATE_PORT(mpy_global_irq_callback)[idx];
    return MP_OBJ_FROM_PTR(irq);
}

static MP_DEFINE_CONST_FUN_OBJ_KW(picopcmcia_irq_obj, 1, picopcmcia_set_irq);

static mp_obj_t picopcmcia_init()
{
    irq_set_exclusive_handler(PIO_IRQ,picopcmcia_irq);
    irq_set_enabled(PIO_IRQ, true); // Enable the IRQ
    const uint irq_index = PIO_IRQ - pio_get_irq_num(PIO_INSTANCE(PIO_SEL_INSTANCE), 0); // Get index of the IRQ
    pio_set_irqn_source_enabled(PIO_INSTANCE(PIO_SEL_INSTANCE), irq_index, pio_get_rx_fifo_not_empty_interrupt_source(SM_MACHINE), true); // Set pio to tell us when the FIFO is NOT empty
    pio0->input_sync_bypass |= ((1u << 26)-1)<<0;

    for(int gp=0;gp<30;gp++)
    {
        if(gp == 26) continue;
        gpio_set_slew_rate(gp,GPIO_SLEW_RATE_FAST);
        gpio_set_drive_strength(gp,GPIO_DRIVE_STRENGTH_12MA);
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(picopcmcia_init_obj,picopcmcia_init);

static mp_obj_t picopcmcia_deinit()
{
    const uint irq_index = PIO_IRQ - pio_get_irq_num(PIO_INSTANCE(PIO_SEL_INSTANCE), 0); // Get index of the IRQ
    pio_set_irqn_source_enabled(PIO_INSTANCE(PIO_SEL_INSTANCE), irq_index, pio_get_rx_fifo_not_empty_interrupt_source(SM_MACHINE), false);
    irq_set_enabled(PIO_IRQ, false);
    irq_remove_handler(PIO_IRQ, picopcmcia_irq);

    // This will free resources and unload our program
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(picopcmcia_deinit_obj,picopcmcia_deinit);


// Define all properties of the module.
// Table entries are key/value pairs of the attribute name (a string)
// and the MicroPython object reference.
// All identifiers and strings are written as MP_QSTR_xxx and will be
// optimized to word-sized integers by the build system (interned strings).
static const mp_rom_map_elem_t picopcmcia_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_picopcmcia_low) },
    
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&picopcmcia_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&picopcmcia_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_trace), MP_ROM_PTR(&picopcmcia_set_trace_obj) },
    
    { MP_ROM_QSTR(MP_QSTR_irq), MP_ROM_PTR(&picopcmcia_irq_obj) },

    { MP_ROM_QSTR(MP_QSTR_set_l1_entry), MP_ROM_PTR(&picopcmcia_set_l1_entry_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_l2), MP_ROM_PTR(&picopcmcia_get_l2_obj) },
};
static MP_DEFINE_CONST_DICT(picopcmcia_module_globals, picopcmcia_module_globals_table);

// Define module object.
const mp_obj_module_t picopcmcia_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&picopcmcia_module_globals,
};

// Register the module to make it available in Python.
MP_REGISTER_MODULE(MP_QSTR_picopcmcia_low, picopcmcia_module);