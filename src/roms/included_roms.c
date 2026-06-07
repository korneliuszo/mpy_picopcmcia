/*
 * SPDX-FileCopyrightText: Korneliusz Osmenda <korneliuszo@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "py/runtime.h"
#include "stdint.h"


extern const volatile uint8_t _binary_optionrom_bin_size;
extern const volatile uint8_t _binary_optionrom_bin_end;
extern const volatile uint8_t _binary_optionrom_bin_start;


static mp_obj_t included_roms_get_8088(void) {

    return mp_obj_new_memoryview('B', 
        (&_binary_optionrom_bin_end-&_binary_optionrom_bin_start),
        (void *)&_binary_optionrom_bin_start);
}

extern const volatile uint8_t _binary_attr_bin_size;
extern const volatile uint8_t _binary_attr_bin_end;
extern const volatile uint8_t _binary_attr_bin_start;


static mp_obj_t included_roms_get_attr(void) {

    return mp_obj_new_memoryview('B', 
        (&_binary_attr_bin_end-&_binary_attr_bin_start),
        (void *)&_binary_attr_bin_start);
}

static MP_DEFINE_CONST_FUN_OBJ_0(included_roms_get_8088_obj, included_roms_get_8088);
static MP_DEFINE_CONST_FUN_OBJ_0(included_roms_get_attr_obj, included_roms_get_attr);

static const mp_rom_map_elem_t included_roms_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_included_roms) },    
    { MP_ROM_QSTR(MP_QSTR_rom_8088), MP_ROM_PTR(&included_roms_get_8088_obj) },
    { MP_ROM_QSTR(MP_QSTR_attr), MP_ROM_PTR(&included_roms_get_attr_obj) },
};

static MP_DEFINE_CONST_DICT(included_roms_module_globals, included_roms_module_globals_table);

// Define module object.
const mp_obj_module_t included_roms_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&included_roms_module_globals,
};

// Register the module to make it available in Python.
MP_REGISTER_MODULE(MP_QSTR_included_roms, included_roms_module);