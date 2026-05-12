// Include MicroPython API.
#include "py/runtime.h"
#include "hardware/pio.h"

#include "shared/runtime/mpirq.h"

#define COMMON_SIZE 5
#define ATTR_SIZE 5
#define IO_SIZE 5

// it should be params of init, def is easier for now
#define PIO_SEL_INSTANCE 0
#define SM_MACHINE 0
#define PIO_IRQ PIO0_IRQ_0

struct _mask_t;

typedef struct _picopcmcia_irq_obj_t {
    mp_irq_obj_t base;
    uint32_t mux0;
    uint32_t mux1;
    uint32_t offset;
    uint32_t data;
    struct _mask_t * low;
} picopcmcia_irq_obj_t;

static mp_obj_t picopcmcia_irq_set_data(mp_obj_t self_in, mp_obj_t data_obj) {
    picopcmcia_irq_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint32_t data = mp_obj_get_uint(data_obj);
    if(self->base.ishard)
        self->data = data;
    else
        pio_sm_put(PIO_INSTANCE(PIO_SEL_INSTANCE),SM_MACHINE,data); 
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(picopcmcia_irq_set_data_obj, picopcmcia_irq_set_data);

//packed? as asm interface?
typedef struct _mask_t {
    uint32_t start; // less than skip
    uint32_t mask; // and - !=0 means skip 
    picopcmcia_irq_obj_t * callback_ptr;
} mask_t;


static mask_t common_selectors[COMMON_SIZE];
static mask_t attr_selectors[ATTR_SIZE];
static mask_t io_selectors[IO_SIZE];

static void isr_sel(
    const uint32_t mux0,
    const uint32_t mux1,
    const uint32_t offset,
    mask_t * out
)
{
    //callback call
    if(out->callback_ptr)
    {
        out->callback_ptr->mux0 = mux0;
        out->callback_ptr->mux1 = mux1;
        out->callback_ptr->offset = offset;
        out->callback_ptr->data = 0;
        mp_irq_handler(&out->callback_ptr->base);
    }
    else
        pio_sm_put(PIO_INSTANCE(PIO_SEL_INSTANCE),SM_MACHINE,0);
    return;
}

// add into ram section ... prefferably exclusive to running core
// callback outside self immutable object field tuple needs core1 started mpy way for atomics to work
// 
static void __not_in_flash_func(picopcmcia_irq())
{
    uint32_t TMP2,TMP3;
    uint32_t mux0;
    uint32_t mux1;
    uint32_t offset;
    mask_t * out;
    PIO isa_pio = pio0;

    asm volatile goto(
        "ldr  %[MUX0],[%[PIOB],%[PIOR]] \n\t" // read ISA_TRANSACTION from PIO
        //nops could be needed here ... remove isr entry latency
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n"
        "ldr  %[MUX1],[%[PIOB],%[PIOR]] \n\t" // read ISA_TRANSACTION from PIO

        "ldr %[TMP2], =%[IORM] \n\t"
        "and %[TMP],%[TMP2],%[MUX1]\n"
        "teq %[TMP],%[TMP2]\n"
        "beq mem\n"
        "ldr %[OUT], =%[IORT] \n\t"
        "ldr %[TMP2], =%[IORC] \n\t"
        "mem: \n"
        "ldr %[TMP], =%[REGM]\n"
        "tst %[TMP], %[MUX1]\n\t"
        "beq attr\n"
        "ldr %[OUT], =%[COMT] \n\t"
        "ldr %[TMP2], =%[COMC] \n\t"
        "b check_table\n"
        "attr:\n"
        "ldr %[OUT], =%[ATRT] \n\t"
        "ldr %[TMP2], =%[ATRC] \n\t"
        "check_table:\n"
        "subs %[TMP2], %[TMP2], #1\n\t"
        "beq loop_exit\n"
        "ldr %[TMP3],[%[OUT],%[MA]]\n\t"
        "subs %[TMP],%[MUX0],%[TMP3]\n" //now offset out
        "ldr %[TMP3],[%[OUT],%[MX]]\n\t"
        "adds %[OUT],%[OUT], %[MS]\n"
        "tst %[TMP3],%[TMP]\n"
        "bne check_table\n"
        "ldr %[TMP2], =%[IOWR] \n\t"
        "tst %[MUX1],%[TMP2]\n"
        "ldr %[TMP2], =#1\n\t"
        "bne not_iowr\n"
        "ldr %[TMP2], =#2\n\t"        
        "not_iowr:\n"
		"strb    %[TMP2],[%[PIOB],%[PIOT]] \n\t" // pio->txf[0] = pin status;

        "subs %[OUT],%[OUT], %[MS]\n" // be nice for upper layers
        "bic %[TMP],%[TMP],%[TMP3]\n" // too nice
        "b %l[us]\n\t"
        "loop_exit:\n"
		"strb    %[TMP2],[%[PIOB],%[PIOT]] \n\t" // pio->txf[0] = pin status;
        "b %l[not_us]\n\t" // end of loop

        "\n"
        ".ltorg\n\t"
    :
        [MUX0]"=l" (mux0),
        [MUX1]"=l" (mux1),
        [OUT]"=l" (out),
        [TMP]"=l" (offset),
        [TMP2]"=l" (TMP2),
        [TMP3]"=l" (TMP3),
        [PIOB]"+l"(isa_pio)
    :
        [IORM]"i" ((1<<16)|(1<<17)),
        [IOWR]"i" ((1<<17)),
        [PIOT]"i" (PIO_TXF0_OFFSET),
        [PIOR]"i" (PIO_RXF0_OFFSET),
        [COMT]"i" (common_selectors),
        [ATRT]"i" (attr_selectors),
        [IORT]"i" (io_selectors),
        [COMC]"i" (COMMON_SIZE),
        [ATRC]"i" (ATTR_SIZE),
        [IORC]"i" (IO_SIZE),
        [REGM]"i" ((1<<18)),
        [MS]"i" (sizeof(mask_t)),
        [MA]"i" (&((mask_t*)NULL)->start),
        [MX]"i" (&((mask_t*)NULL)->mask)
        : "cc"
        : us,not_us
    );

    not_us:
    return;
    {
        us:
        isr_sel(mux0,mux1,offset,out);
        if(out->callback_ptr && out->callback_ptr->base.ishard)
            pio_sm_put(PIO_INSTANCE(PIO_SEL_INSTANCE),SM_MACHINE,out->callback_ptr->data);
        else if(!out->callback_ptr)
            pio_sm_put(PIO_INSTANCE(PIO_SEL_INSTANCE),SM_MACHINE,0);
        return;
    }
}


static mp_uint_t picopcmcia_irq_trigger(mp_obj_t self_in, mp_uint_t new_trigger) {
    picopcmcia_irq_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if(new_trigger == 0) //exception disable
    {
        __compiler_memory_barrier();
        self->low->mask = 0xfffffffful; // mask
        __compiler_memory_barrier(); // makes only exact case pass here - well at least old irq notouchy
        self->low->start = 0xe0000000ul; // start
        __compiler_memory_barrier(); // no irq from this point can trigger

    }
    return 0;
}

static mp_uint_t picopcmcia_irq_info(mp_obj_t self_in, mp_uint_t info_type) {
    //picopcmcia_irq_obj_t *self = MP_OBJ_TO_PTR(self_in);

    return 0;
}

static const mp_irq_methods_t picopcmcia_irq_methods = {
    .trigger = picopcmcia_irq_trigger,
    .info = picopcmcia_irq_info,
};

static mp_obj_t picopcmcia_irq_mux0(mp_obj_t self_in) {
    picopcmcia_irq_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return MP_OBJ_NEW_SMALL_INT(self->mux0);
}
static MP_DEFINE_CONST_FUN_OBJ_1(picopcmcia_irq_mux0_obj, picopcmcia_irq_mux0);

static mp_obj_t picopcmcia_irq_mux1(mp_obj_t self_in) {
    picopcmcia_irq_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return MP_OBJ_NEW_SMALL_INT(self->mux1);
}
static MP_DEFINE_CONST_FUN_OBJ_1(picopcmcia_irq_mux1_obj, picopcmcia_irq_mux1);

static mp_obj_t picopcmcia_irq_offset(mp_obj_t self_in) {
    picopcmcia_irq_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return MP_OBJ_NEW_SMALL_INT(self->offset);
}
static MP_DEFINE_CONST_FUN_OBJ_1(picopcmcia_irq_offset_obj, picopcmcia_irq_offset);


static const mp_rom_map_elem_t picopcmcia_irq_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_mux0), MP_ROM_PTR(&picopcmcia_irq_mux0_obj) },
    { MP_ROM_QSTR(MP_QSTR_mux1), MP_ROM_PTR(&picopcmcia_irq_mux1_obj) },
    { MP_ROM_QSTR(MP_QSTR_offset), MP_ROM_PTR(&picopcmcia_irq_offset_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_data), MP_ROM_PTR(&picopcmcia_irq_set_data_obj)},
};


static MP_DEFINE_CONST_DICT(picopcmcia_irq_locals_dict, picopcmcia_irq_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    picopcmcia_irq_type,
    MP_QSTR_PCMCIA_IRQ,
    MP_TYPE_FLAG_NONE,
    locals_dict, &picopcmcia_irq_locals_dict
);

static mp_obj_t picopcmcia_common_irq(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_start, ARG_mask, ARG_handler, ARG_hard };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_start, MP_ARG_INT, {.u_int = 0xe0000000ul} },
        { MP_QSTR_mask, MP_ARG_INT, {.u_int = 0xfffffffful} },
        { MP_QSTR_handler, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_hard, MP_ARG_BOOL, {.u_bool = false} },
    };

    // Parse the arguments.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint32_t idx = mp_obj_get_int(pos_args[0]);
    //TODO: check oob and raise exception

    // Get the IRQ object.
    picopcmcia_irq_obj_t *irq = MP_STATE_PORT(mpy_global_common_callback[idx]);

    // Allocate the IRQ object if it doesn't already exist.
    if (irq == NULL) {
        irq = m_new_obj(picopcmcia_irq_obj_t);
        irq->base.base.type = &picopcmcia_irq_type;
        irq->base.methods = (mp_irq_methods_t *)&picopcmcia_irq_methods;
        irq->base.parent = MP_OBJ_FROM_PTR(irq);
        irq->base.handler = mp_const_none;
        irq->base.ishard = false;
        irq->low = &common_selectors[idx];
        MP_STATE_PORT(mpy_global_common_callback[idx]) = irq;
    }

    if (n_args > 1 || kw_args->used != 0) {
        // Configure IRQ.

        // Disable all IRQs while data is updated.
        //irq_set_enabled(self->irq, false);
        // write lock free code 
        common_selectors[idx].mask = 0xfffffffful; // mask
        __compiler_memory_barrier(); // makes only exact case pass here - well at least old irq notouchy
        common_selectors[idx].start = 0xe0000000ul; // start
        __compiler_memory_barrier(); // no irq from this point can trigger

        irq->base.handler = args[ARG_handler].u_obj;
        irq->base.ishard = args[ARG_hard].u_bool;

        // Enable IRQ if a handler is given.
        if (args[ARG_handler].u_obj != mp_const_none) {
        
            common_selectors[idx].callback_ptr = irq;
            __compiler_memory_barrier(); // reseated object

            common_selectors[idx].mask = 0xe0000000ul | args[ARG_mask].u_int; // mask with no pass on high bits
            __compiler_memory_barrier(); 
            common_selectors[idx].start = args[ARG_start].u_int; // start ... offset broken but still correct
            __compiler_memory_barrier();
            common_selectors[idx].mask = args[ARG_mask].u_int; // make offset technically correct
            __compiler_memory_barrier();
        }
        else
        {
            common_selectors[idx].callback_ptr = NULL;
            __compiler_memory_barrier(); // reseated object
        }
    }
    return MP_OBJ_FROM_PTR(irq);
}

static MP_DEFINE_CONST_FUN_OBJ_KW(picopcmcia_common_irq_obj, 1, picopcmcia_common_irq);
MP_REGISTER_ROOT_POINTER(void *mpy_global_common_callback[COMMON_SIZE]);

static mp_obj_t picopcmcia_attr_irq(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_start, ARG_mask, ARG_handler, ARG_hard };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_start, MP_ARG_INT, {.u_int = 0xe0000000ul} },
        { MP_QSTR_mask, MP_ARG_INT, {.u_int = 0xfffffffful} },
        { MP_QSTR_handler, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_hard, MP_ARG_BOOL, {.u_bool = false} },
    };

    // Parse the arguments.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint32_t idx = mp_obj_get_int(pos_args[0]);
    //TODO: check oob and raise exception

    // Get the IRQ object.
    picopcmcia_irq_obj_t *irq = MP_STATE_PORT(mpy_global_attr_callback[idx]);

    // Allocate the IRQ object if it doesn't already exist.
    if (irq == NULL) {
        irq = m_new_obj(picopcmcia_irq_obj_t);
        irq->base.base.type = &picopcmcia_irq_type;
        irq->base.methods = (mp_irq_methods_t *)&picopcmcia_irq_methods;
        irq->base.parent = MP_OBJ_FROM_PTR(irq);
        irq->base.handler = mp_const_none;
        irq->base.ishard = false;
        irq->low = &attr_selectors[idx];
        MP_STATE_PORT(mpy_global_attr_callback[idx]) = irq;
    }

    if (n_args > 1 || kw_args->used != 0) {
        // Configure IRQ.

        // Disable all IRQs while data is updated.
        //irq_set_enabled(self->irq, false);
        // write lock free code 
        attr_selectors[idx].mask = 0xfffffffful; // mask
        __compiler_memory_barrier(); // makes only exact case pass here - well at least old irq notouchy
        attr_selectors[idx].start = 0xe0000000ul; // start
        __compiler_memory_barrier(); // no irq from this point can trigger

        irq->base.handler = args[ARG_handler].u_obj;
        irq->base.ishard = args[ARG_hard].u_bool;

        // Enable IRQ if a handler is given.
        if (args[ARG_handler].u_obj != mp_const_none) {
        
            attr_selectors[idx].callback_ptr = irq;
            __compiler_memory_barrier(); // reseated object

            attr_selectors[idx].mask = 0xe0000000ul | args[ARG_mask].u_int; // mask with no pass on high bits
            __compiler_memory_barrier(); 
            attr_selectors[idx].start = args[ARG_start].u_int; // start ... offset broken but still correct
            __compiler_memory_barrier();
            attr_selectors[idx].mask = args[ARG_mask].u_int; // make offset technically correct
            __compiler_memory_barrier();
        }
        else
        {
            attr_selectors[idx].callback_ptr = NULL;
            __compiler_memory_barrier(); // reseated object
        }
    }
    return MP_OBJ_FROM_PTR(irq);
}

static MP_DEFINE_CONST_FUN_OBJ_KW(picopcmcia_attr_irq_obj, 1, picopcmcia_attr_irq);
MP_REGISTER_ROOT_POINTER(void *mpy_global_attr_callback[ATTR_SIZE]);

static mp_obj_t picopcmcia_io_irq(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_start, ARG_mask, ARG_handler, ARG_hard };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_start, MP_ARG_INT, {.u_int = 0xe0000000ul} },
        { MP_QSTR_mask, MP_ARG_INT, {.u_int = 0xfffffffful} },
        { MP_QSTR_handler, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_hard, MP_ARG_BOOL, {.u_bool = false} },
    };

    // Parse the arguments.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint32_t idx = mp_obj_get_int(pos_args[0]);
    //TODO: check oob and raise exception

    // Get the IRQ object.
    picopcmcia_irq_obj_t *irq = MP_STATE_PORT(mpy_global_io_callback[idx]);

    // Allocate the IRQ object if it doesn't already exist.
    if (irq == NULL) {
        irq = m_new_obj(picopcmcia_irq_obj_t);
        irq->base.base.type = &picopcmcia_irq_type;
        irq->base.methods = (mp_irq_methods_t *)&picopcmcia_irq_methods;
        irq->base.parent = MP_OBJ_FROM_PTR(irq);
        irq->base.handler = mp_const_none;
        irq->base.ishard = false;
        irq->low = &io_selectors[idx];
        MP_STATE_PORT(mpy_global_io_callback[idx]) = irq;
    }

    if (n_args > 1 || kw_args->used != 0) {
        // Configure IRQ.

        // Disable all IRQs while data is updated.
        //irq_set_enabled(self->irq, false);
        // write lock free code 
        io_selectors[idx].mask = 0xfffffffful; // mask
        __compiler_memory_barrier(); // makes only exact case pass here - well at least old irq notouchy
        io_selectors[idx].start = 0xe0000000ul; // start
        __compiler_memory_barrier(); // no irq from this point can trigger

        irq->base.handler = args[ARG_handler].u_obj;
        irq->base.ishard = args[ARG_hard].u_bool;

        // Enable IRQ if a handler is given.
        if (args[ARG_handler].u_obj != mp_const_none) {
        
            io_selectors[idx].callback_ptr = irq;
            __compiler_memory_barrier(); // reseated object

            io_selectors[idx].mask = 0xe0000000ul | args[ARG_mask].u_int; // mask with no pass on high bits
            __compiler_memory_barrier(); 
            io_selectors[idx].start = args[ARG_start].u_int; // start ... offset broken but still correct
            __compiler_memory_barrier();
            io_selectors[idx].mask = args[ARG_mask].u_int; // make offset technically correct
            __compiler_memory_barrier();
        }
        else
        {
            io_selectors[idx].callback_ptr = NULL;
            __compiler_memory_barrier(); // reseated object
        }
    }
    return MP_OBJ_FROM_PTR(irq);
}

static MP_DEFINE_CONST_FUN_OBJ_KW(picopcmcia_io_irq_obj, 1, picopcmcia_io_irq);
MP_REGISTER_ROOT_POINTER(void *mpy_global_io_callback[IO_SIZE]);

static mp_obj_t picopcmcia_init()
{

    for(size_t i=0;i<COMMON_SIZE;i++)
        common_selectors[i] = (mask_t){0xe0000000ul,0xfffffffful,NULL};
    for(size_t i=0;i<ATTR_SIZE;i++)
        attr_selectors[i] = (mask_t){0xe0000000ul,0xfffffffful,NULL};
    for(size_t i=0;i<IO_SIZE;i++)
        io_selectors[i] = (mask_t){0xe0000000ul,0xfffffffful,NULL};

    irq_set_exclusive_handler(PIO_IRQ,picopcmcia_irq);
    irq_set_enabled(PIO_IRQ, true); // Enable the IRQ
    const uint irq_index = PIO_IRQ - pio_get_irq_num(PIO_INSTANCE(PIO_SEL_INSTANCE), 0); // Get index of the IRQ
    pio_set_irqn_source_enabled(PIO_INSTANCE(PIO_SEL_INSTANCE), irq_index, pio_get_rx_fifo_not_empty_interrupt_source(SM_MACHINE), true); // Set pio to tell us when the FIFO is NOT empty
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
    { MP_ROM_QSTR(MP_QSTR_common_irq), MP_ROM_PTR(&picopcmcia_common_irq_obj) },
    { MP_ROM_QSTR(MP_QSTR_attr_irq), MP_ROM_PTR(&picopcmcia_attr_irq_obj) },
    { MP_ROM_QSTR(MP_QSTR_io_irq), MP_ROM_PTR(&picopcmcia_io_irq_obj) },
};
static MP_DEFINE_CONST_DICT(picopcmcia_module_globals, picopcmcia_module_globals_table);

// Define module object.
const mp_obj_module_t picopcmcia_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&picopcmcia_module_globals,
};

// Register the module to make it available in Python.
MP_REGISTER_MODULE(MP_QSTR_picopcmcia_low, picopcmcia_module);