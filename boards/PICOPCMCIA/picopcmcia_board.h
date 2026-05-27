/*
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------
// Board definition for the SparkFun IoT RedBoard - RP2350
//
// This header may be included by other board headers as "boards/sparkfun_iotredboard_rp2350.h"

// pico_cmake_set PICO_PLATFORM=rp2350
// pico_cmake_set PICO_CYW43_SUPPORTED = 1

#ifndef _BOARDS_PICOPCMCIA_BOARD_H
#define _BOARDS_PICOPCMCIA_BOARD_H

// For board detection
#define PICOPCMCIA_BOARD

// --- RP2350 VARIANT ---
#define PICO_RP2350A 0 // 1 for RP2350A, 0 for RP2350B

// --- BOARD SPECIFIC ---
#define PICOPCMCIA_BOARD_PSRAM_CS_PIN 47


// --- UART ---
#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 0
#endif
#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 34
#endif
#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 35
#endif

// --- I2C --- Qwiic connector is on these pins
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C 0
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN 32
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN 33
#endif

// --- SPI ---
#ifndef PICO_DEFAULT_SPI
#define PICO_DEFAULT_SPI 0
#endif
#ifndef PICO_DEFAULT_SPI_SCK_PIN
#define PICO_DEFAULT_SPI_SCK_PIN 38
#endif
#ifndef PICO_DEFAULT_SPI_TX_PIN
#define PICO_DEFAULT_SPI_TX_PIN 39
#endif
#ifndef PICO_DEFAULT_SPI_RX_PIN
#define PICO_DEFAULT_SPI_RX_PIN 36
#endif
#ifndef PICO_DEFAULT_SPI_CSN_PIN
#define PICO_DEFAULT_SPI_CSN_PIN 37
#endif

// --- FLASH ---
// QSPI - magic enabled with PICO_EMBED_XIP_SETUP
#define PICO_BOOT_STAGE2_CHOOSE_W25Q080 1
#define MICROPY_GC_SPLIT_HEAP 0

#define MICROPY_HW_FLASH_MAX_FREQ 133000000
#define PICO_FLASH_SPI_CLKDIV 3

// pico_cmake_set_default PICO_FLASH_SIZE_BYTES = (4 * 1024 * 1024)
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (4 * 1024 * 1024)
#endif


// pico_cmake_set_default PICO_RP2350_A2_SUPPORTED = 0
#ifndef PICO_RP2350_A2_SUPPORTED
#define PICO_RP2350_A2_SUPPORTED 0
#endif

// Bootloader activity LED in double reset mode.
#ifndef PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED
#define PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED PICO_DEFAULT_LED_PIN
#endif

// Bootloader activity LED in USB reset mode.
#ifndef PICO_STDIO_USB_RESET_BOOTSEL_ACTIVITY_LED
#define PICO_STDIO_USB_RESET_BOOTSEL_ACTIVITY_LED PICO_DEFAULT_LED_PIN
#endif

// --- CYW43 ---

#ifndef CYW43_WL_GPIO_COUNT
#define CYW43_WL_GPIO_COUNT 3
#endif

#ifndef CYW43_WL_GPIO_LED_PIN
#define CYW43_WL_GPIO_LED_PIN 0
#endif

// cyw43 SPI pins can't be changed at runtime
#ifndef CYW43_PIN_WL_DYNAMIC
#define CYW43_PIN_WL_DYNAMIC 0
#endif

// gpio pin to power up the cyw43 chip
#ifndef CYW43_DEFAULT_PIN_WL_REG_ON
#define CYW43_DEFAULT_PIN_WL_REG_ON 40u
#endif

// gpio pin for spi data out to the cyw43 chip
#ifndef CYW43_DEFAULT_PIN_WL_DATA_OUT
#define CYW43_DEFAULT_PIN_WL_DATA_OUT 42u
#endif

// gpio pin for spi data in from the cyw43 chip
#ifndef CYW43_DEFAULT_PIN_WL_DATA_IN
#define CYW43_DEFAULT_PIN_WL_DATA_IN 42u
#endif

// gpio (irq) pin for the irq line from the cyw43 chip
#ifndef CYW43_DEFAULT_PIN_WL_HOST_WAKE
#define CYW43_DEFAULT_PIN_WL_HOST_WAKE 42u
#endif

// gpio pin for the spi clock line to the cyw43 chip
#ifndef CYW43_DEFAULT_PIN_WL_CLOCK
#define CYW43_DEFAULT_PIN_WL_CLOCK 41u
#endif

// gpio pin for the spi chip select to the cyw43 chip
#ifndef CYW43_DEFAULT_PIN_WL_CS
#define CYW43_DEFAULT_PIN_WL_CS 43u
#endif

#endif // _BOARDS_PICOPCMCIA_BOARD_H
