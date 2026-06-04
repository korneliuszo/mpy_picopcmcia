// Board and hardware specific configuration
#define MICROPY_HW_BOARD_NAME                   "PicoPCMCIA"
#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024)
#define MICROPY_HW_FLASH_STORAGE_BYTES          (PICO_FLASH_SIZE_BYTES - 1536 * 1024)

// Enable networking.
#define MICROPY_PY_NETWORK 1
#define MICROPY_PY_NETWORK_HOSTNAME_DEFAULT     "PicoPCMCIA"

#define CYW43_USE_SPI (1)
#define CYW43_LWIP (1)
#define CYW43_GPIO (1)
#define CYW43_SPI_PIO (1)

// USB VID/PID
#define MICROPY_HW_USB_VID (0x1B4F)
#define MICROPY_HW_USB_PID (0x0047)

// UART0
#define MICROPY_HW_UART0_TX  (34)
#define MICROPY_HW_UART0_RX  (35)
#define MICROPY_HW_UART0_CTS (-1)
#define MICROPY_HW_UART0_RTS (-1)

// UART1
#define MICROPY_HW_UART1_TX  (26)
#define MICROPY_HW_UART1_RX  (-1)
#define MICROPY_HW_UART1_CTS (-1)
#define MICROPY_HW_UART1_RTS (-1)

// I2C0
#define MICROPY_HW_I2C0_SCL  (33)
#define MICROPY_HW_I2C0_SDA  (32)

// SPI0
#define MICROPY_HW_SPI0_SCK  (38)
#define MICROPY_HW_SPI0_MOSI (39)
#define MICROPY_HW_SPI0_MISO (36)

// PSRAM
#define MICROPY_HW_PSRAM_CS_PIN (47)
#define MICROPY_HW_ENABLE_PSRAM (1)

// #include "enable_cyw43.h"

// For debugging mbedtls - also set
// Debug level (0-4) 1=warning, 2=info, 3=debug, 4=verbose
// #define MODUSSL_MBEDTLS_DEBUG_LEVEL 1

#define MICROPY_HW_PIN_EXT_COUNT    CYW43_WL_GPIO_COUNT

int mp_hal_is_pin_reserved(int n);
#define MICROPY_HW_PIN_RESERVED(i) mp_hal_is_pin_reserved(i)
