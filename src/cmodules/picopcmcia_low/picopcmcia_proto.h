#pragma once

typedef struct _mp_picopcmcia_low_c_worker_obj_t {
    mp_obj_base_t base;
    uint32_t (*fn)(void * self,uint32_t mux0,uint32_t mux1,uint32_t idx);
} mp_picopcmcia_low_c_worker_obj_t;
