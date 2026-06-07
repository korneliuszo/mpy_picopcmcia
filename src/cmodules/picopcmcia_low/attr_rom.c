/*
 * SPDX-FileCopyrightText: Korneliusz Osmenda <korneliuszo@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "py/runtime.h"
#include "pico.h"
#include "picopcmcia_proto.h"
#include "stdint.h"

typedef struct _attr_rom_ROM_obj_t {
    mp_picopcmcia_low_c_worker_obj_t base;
    mp_obj_t buff;
    mp_buffer_info_t bufinfo;
} attr_rom_ROM_obj_t;

static mp_obj_t attr_rom_ROM_buff(mp_obj_t self_in) {
    attr_rom_ROM_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return self->buff;
}
static MP_DEFINE_CONST_FUN_OBJ_1(attr_rom_ROM_buff_obj, attr_rom_ROM_buff);

static uint32_t __no_inline_not_in_flash_func(attr_rom_fn)(void * self,uint32_t mux0,uint32_t mux1,uint32_t idx)
{
    attr_rom_ROM_obj_t * self_ptr = self;
    if(mux1 & (1<<23)) //CE1 high
        return 0;
    if(mux0 & 0x1)
        return 0;
    uint32_t addr = mux0>>1;
    if(self_ptr->bufinfo.len < addr)
        return 0;
    uint8_t * buf;
    if(!(buf = self_ptr->bufinfo.buf))
        return 0;
    return (0xff0000ul) | (buf[addr]);
}

static mp_obj_t attr_rom_ROM_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    attr_rom_ROM_obj_t *self = mp_obj_malloc(attr_rom_ROM_obj_t, type);

    enum { ARG_buff };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_buff, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
    };

    // Parse the arguments.
    mp_arg_val_t args_out[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed_args), allowed_args, args_out);

    self->bufinfo.buf = NULL;
    self->bufinfo.len = 0;
    self->bufinfo.typecode = (int)'b';
    self->base.fn = attr_rom_fn;

    if (args_out[ARG_buff].u_obj == mp_const_none)
    {
        self->buff = mp_const_none;
    }
    else
    {
        self->buff = args_out[ARG_buff].u_obj;
        mp_get_buffer_raise(self->buff,&self->bufinfo,MP_BUFFER_READ);
    }
    // The make_new function always returns self.
    return MP_OBJ_FROM_PTR(self);
}

static const mp_rom_map_elem_t attr_rom_ROM_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_buff), MP_ROM_PTR(&attr_rom_ROM_buff_obj) },
};
static MP_DEFINE_CONST_DICT(attr_rom_ROM_locals_dict, attr_rom_ROM_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    attr_rom_type_ROM,
    MP_QSTR_ROM,
    MP_TYPE_FLAG_NONE,
    make_new, attr_rom_ROM_make_new,
    locals_dict, &attr_rom_ROM_locals_dict
    );

static const mp_rom_map_elem_t attr_rom_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_attr_rom_low) },
    { MP_ROM_QSTR(MP_QSTR_rom), MP_ROM_PTR(&attr_rom_type_ROM) },
};

static MP_DEFINE_CONST_DICT(attr_rom_module_globals, attr_rom_module_globals_table);

// Define module object.
const mp_obj_module_t attr_rom_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&attr_rom_module_globals,
};

// Register the module to make it available in Python.
MP_REGISTER_MODULE(MP_QSTR_attr_rom, attr_rom_module);