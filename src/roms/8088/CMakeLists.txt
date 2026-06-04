cmake_minimum_required(VERSION 3.16)
project(picopcmcia-optionrom)

set(WATCOM_PATH "$ENV{HOME}/watcom" CACHE STRING "Watcom compiler path")

add_custom_target(
	build_optionrom
	COMMAND 
	${CMAKE_COMMAND} -E env 
		"PATH=${WATCOM_PATH}/binl64/:$ENV{PATH}"
		"INCLUDE=${WATCOM_PATH}/h"
		"WATCOM=${WATCOM_PATH}"
	${CMAKE_MAKE_PROGRAM} -f "${CMAKE_CURRENT_SOURCE_DIR}/Makefile"
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/optionrom.bin
)

set(BFDARG elf32-littlearm)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/optionrom.o
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMAND ${CMAKE_OBJCOPY} -I binary -O ${BFDARG} --rename-section .data=.rodata optionrom.bin optionrom.o
	#places in data
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/optionrom.bin
)

add_custom_target(optionrom_obj DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/optionrom.o)


add_library(optionrom INTERFACE)
add_dependencies(optionrom optionrom_obj)
set_target_properties(optionrom PROPERTIES
  INTERFACE_LINK_LIBRARIES "${CMAKE_CURRENT_BINARY_DIR}/optionrom.o")
