#include "py/runtime.h"
#include "stdint.h"


extern const volatile uint8_t _binary_optionrom_bin_size;
extern const volatile uint8_t _binary_optionrom_bin_end;
extern const volatile uint8_t _binary_optionrom_bin_start;


// 2. The module function exposing the data
static mp_obj_t included_roms_get_8088(void) {
    // Arguments for mp_obj_new_memoryview:
    // Type code (e.g., 'B' for uint8_t / unsigned char)
    // Number of elements (size)
    // Pointer to the static C array
    
    return mp_obj_new_memoryview('B', 
        (&_binary_optionrom_bin_end-&_binary_optionrom_bin_start),
        (void *)&_binary_optionrom_bin_start);
}

extern const volatile uint8_t _binary_attr_bin_size;
extern const volatile uint8_t _binary_attr_bin_end;
extern const volatile uint8_t _binary_attr_bin_start;


// 2. The module function exposing the data
static mp_obj_t included_roms_get_attr(void) {
    // Arguments for mp_obj_new_memoryview:
    // Type code (e.g., 'B' for uint8_t / unsigned char)
    // Number of elements (size)
    // Pointer to the static C array
    
    return mp_obj_new_memoryview('B', 
        (&_binary_attr_bin_end-&_binary_attr_bin_start),
        (void *)&_binary_attr_bin_start);
}

// Bind the function to MicroPython
static MP_DEFINE_CONST_FUN_OBJ_0(included_roms_get_8088_obj, included_roms_get_8088);
static MP_DEFINE_CONST_FUN_OBJ_0(included_roms_get_attr_obj, included_roms_get_attr);

// Define all properties of the module.
// Table entries are key/value pairs of the attribute name (a string)
// and the MicroPython object reference.
// All identifiers and strings are written as MP_QSTR_xxx and will be
// optimized to word-sized integers by the build system (interned strings).
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