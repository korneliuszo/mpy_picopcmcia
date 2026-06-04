#include "py/runtime.h"
#include "pico.h"
#include "hardware/gpio.h"
#include "py/ringbuf.h"
#include "picopcmcia_proto.h"
#include "stdint.h"

//not in header
typedef struct _micropython_ringio_obj_t {
    mp_obj_base_t base;
    ringbuf_t ringbuffer;
} micropython_ringio_obj_t;
//

typedef struct _mp_ioiface_c_worker_obj_t {
    mp_obj_base_t base;
    mp_obj_t prev;
    mp_obj_t send;
    mp_obj_t recv;
    mp_obj_t self;
    mp_obj_t creat_proto_obj;
    bool connected;
    size_t idx;
} mp_ioiface_c_worker_obj_t;


MP_REGISTER_ROOT_POINTER(mp_obj_t mpy_global_ioiface_slots[4]);

MP_REGISTER_ROOT_POINTER(mp_obj_t mpy_global_ioiface_protocols_e[4]);

extern const mp_obj_type_t ioiface_type_Worker;


// This is the Timer.time() method. After creating a Timer object, this
// can be called to get the time elapsed since creating the Timer.
static mp_obj_t ioiface_new_proto(mp_obj_t worker_in) {
    mp_ioiface_c_worker_obj_t * worker = MP_OBJ_TO_PTR(worker_in);
    if(worker->creat_proto_obj == mp_const_none)
        return mp_const_none; // shouldn't happen

    mp_obj_t proto = worker->creat_proto_obj;
    worker->creat_proto_obj = mp_const_none;

    mp_ioiface_c_worker_obj_t *worker_new = mp_obj_malloc(mp_ioiface_c_worker_obj_t, &ioiface_type_Worker);
    if(!worker_new)
        return mp_const_none;

    worker_new->prev = worker;
    worker_new->recv = mp_const_none;
    worker_new->send = mp_const_none;
    worker_new->self = mp_const_none;
    worker_new->creat_proto_obj = mp_const_none;
    worker_new->connected = true;
    worker_new->idx = worker->idx;

    worker_new->self = mp_call_function_1_protected(proto,MP_OBJ_FROM_PTR(worker_new));

    if(worker_new->self)
    {
        MP_STATE_PORT(mpy_global_ioiface_slots[worker->idx]) = MP_OBJ_FROM_PTR(worker_new);
    }
    
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(ioiface_new_proto_obj, ioiface_new_proto);

static uint32_t __no_inline_not_in_flash_func(ioiface_rd_fn)(void * self,uint32_t mux0,uint32_t mux1,uint32_t idx)
{
    if(mux1 & (1<<23)) //CE1 high
        return 0;
    uint32_t addr =(mux0>>1) & 0x3;
    mp_ioiface_c_worker_obj_t * ioiface = MP_OBJ_TO_PTR(MP_STATE_PORT(mpy_global_ioiface_slots[addr]));
    if(!ioiface)
        return (0xff0000ul) | 0xff;
    if(mux0 & 0x1)
    {
        return (0xff0000ul) | (ioiface->connected ? 1 : 0);
    }
    mp_obj_t ring_obj = ioiface->send;
    if (ring_obj == mp_const_none)
        return (0xff0000ul) | 0xff;
    micropython_ringio_obj_t * ring = MP_OBJ_TO_PTR(ring_obj);
    return (0xff0000ul) | (ringbuf_get(&ring->ringbuffer)&0xff);
}

static uint32_t __no_inline_not_in_flash_func(ioiface_wr_fn)(void * self,uint32_t mux0,uint32_t mux1,uint32_t idx)
{
    if(mux1 & (1<<23)) //CE1 high
        return 0;
    uint32_t addr =(mux0>>1) & 0x3;
    mp_ioiface_c_worker_obj_t * ioiface = MP_OBJ_TO_PTR(MP_STATE_PORT(mpy_global_ioiface_slots[addr]));
    if(!ioiface)
        return 0x00;
    if(mux0 & 0x1)
    {
        uint8_t data = gpio_get_all()&0xff;//should be function
        if(data == 0)
        {
            if (ioiface->prev != mp_const_none)
            {
                mp_ioiface_c_worker_obj_t * ioiface_prev = MP_OBJ_TO_PTR(ioiface->prev);
                ioiface_prev->connected = true;
                MP_STATE_PORT(mpy_global_ioiface_slots[addr]) = ioiface->prev;
            }
        }
        else
        {
            ioiface->connected = false;
            if(data>5)
                return 0;
            mp_obj_t proto = MP_STATE_PORT(mpy_global_ioiface_protocols_e[data-1]);
            if(!proto)
                return 0;
            ioiface->creat_proto_obj = proto;
            mp_sched_schedule((mp_obj_t)&ioiface_new_proto_obj,MP_OBJ_FROM_PTR(ioiface));
        }
        return 0;
    }
    mp_obj_t ring_obj = ioiface->recv;
    if (ring_obj == mp_const_none)
        return 0x00;
    micropython_ringio_obj_t * ring = MP_OBJ_TO_PTR(ring_obj);
    uint8_t data = gpio_get_all()&0xff;//should be function
    ringbuf_put(&ring->ringbuffer,data);
    return 0;

}

static mp_obj_t ioiface_worker_set_send(mp_obj_t self_in, mp_obj_t send) {
    // The first argument is self. It is cast to the *example_Timer_obj_t
    // type so we can read its attributes.
    mp_ioiface_c_worker_obj_t *self = MP_OBJ_TO_PTR(self_in);
    self->send = send;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(ioiface_worker_set_send_obj, ioiface_worker_set_send);

static mp_obj_t ioiface_worker_send(mp_obj_t self_in) {
    // The first argument is self. It is cast to the *example_Timer_obj_t
    // type so we can read its attributes.
    mp_ioiface_c_worker_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return self->send;
}
static MP_DEFINE_CONST_FUN_OBJ_1(ioiface_worker_send_obj, ioiface_worker_send);

static mp_obj_t ioiface_worker_set_recv(mp_obj_t self_in, mp_obj_t recv) {
    // The first argument is self. It is cast to the *example_Timer_obj_t
    // type so we can read its attributes.
    mp_ioiface_c_worker_obj_t *self = MP_OBJ_TO_PTR(self_in);
    self->recv = recv;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(ioiface_worker_set_recv_obj, ioiface_worker_set_recv);

static mp_obj_t ioiface_worker_recv(mp_obj_t self_in) {
    // The first argument is self. It is cast to the *example_Timer_obj_t
    // type so we can read its attributes.
    mp_ioiface_c_worker_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return self->recv;
}
static MP_DEFINE_CONST_FUN_OBJ_1(ioiface_worker_recv_obj, ioiface_worker_recv);

// This collects all methods and other static class attributes of the Timer.
// The table structure is similar to the module table, as detailed below.
static const mp_rom_map_elem_t ioiface_worker_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_set_send), MP_ROM_PTR(&ioiface_worker_set_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&ioiface_worker_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_recv), MP_ROM_PTR(&ioiface_worker_set_recv_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv), MP_ROM_PTR(&ioiface_worker_recv_obj) },
};
static MP_DEFINE_CONST_DICT(ioiface_worker_locals_dict, ioiface_worker_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    ioiface_type_Worker,
    MP_QSTR_Worker,
    MP_TYPE_FLAG_NONE,
    locals_dict, &ioiface_worker_locals_dict
    );


static mp_obj_t ioiface_rd_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // Allocates the new object and sets the type.
    mp_picopcmcia_low_c_worker_obj_t *self = mp_obj_malloc(mp_picopcmcia_low_c_worker_obj_t, type);
    self->fn = ioiface_rd_fn;
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t ioiface_wr_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // Allocates the new object and sets the type.
    mp_picopcmcia_low_c_worker_obj_t *self = mp_obj_malloc(mp_picopcmcia_low_c_worker_obj_t, type);
    self->fn = ioiface_wr_fn;
    return MP_OBJ_FROM_PTR(self);
}


MP_DEFINE_CONST_OBJ_TYPE(
    ioiface_type_RD,
    MP_QSTR_READ,
    MP_TYPE_FLAG_NONE,
    make_new, ioiface_rd_make_new
);

MP_DEFINE_CONST_OBJ_TYPE(
    ioiface_type_WR,
    MP_QSTR_WRITE,
    MP_TYPE_FLAG_NONE,
    make_new, ioiface_wr_make_new
);

static mp_obj_t ioiface_register_proto_entry(mp_obj_t idx_obj, mp_obj_t fn_obj)
{
    if (!mp_obj_is_small_int(idx_obj)) {
        mp_raise_TypeError_int_conversion(idx_obj);
    }

    uint32_t idx = MP_OBJ_SMALL_INT_VALUE(idx_obj);

    MP_STATE_PORT(mpy_global_ioiface_protocols_e[idx]) = fn_obj;
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_2(ioiface_register_proto_entry_obj, ioiface_register_proto_entry);

static mp_obj_t ioiface_init() {
    
    for(size_t addr=0;addr<4;addr++)
    {
        mp_ioiface_c_worker_obj_t *worker_new = mp_obj_malloc(mp_ioiface_c_worker_obj_t, &ioiface_type_Worker);
        worker_new->prev = mp_const_none;
        worker_new->recv = mp_const_none;
        worker_new->send = mp_const_none;
        worker_new->self = mp_const_none;
        worker_new->creat_proto_obj = mp_const_none;
        worker_new->connected = true;
        worker_new->idx = addr;
        MP_STATE_PORT(mpy_global_ioiface_slots[addr]) = MP_OBJ_FROM_PTR(worker_new);
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(ioiface_init_obj, ioiface_init);


// Define all properties of the module.
// Table entries are key/value pairs of the attribute name (a string)
// and the MicroPython object reference.
// All identifiers and strings are written as MP_QSTR_xxx and will be
// optimized to word-sized integers by the build system (interned strings).
static const mp_rom_map_elem_t ioiface_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ioiface_low) },    
    { MP_ROM_QSTR(MP_QSTR_worker), MP_ROM_PTR(&ioiface_type_Worker) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&ioiface_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_register_entry), MP_ROM_PTR(&ioiface_register_proto_entry_obj) },
    { MP_ROM_QSTR(MP_QSTR_RD), MP_ROM_PTR(&ioiface_type_RD) },
    { MP_ROM_QSTR(MP_QSTR_WR), MP_ROM_PTR(&ioiface_type_WR) },
};

static MP_DEFINE_CONST_DICT(ioiface_module_globals, ioiface_module_globals_table);

// Define module object.
const mp_obj_module_t ioiface_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&ioiface_module_globals,
};

// Register the module to make it available in Python.
MP_REGISTER_MODULE(MP_QSTR_ioiface, ioiface_module);