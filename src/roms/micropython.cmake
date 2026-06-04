add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/8088 ${CMAKE_CURRENT_BINARY_DIR}/8088)

# Create an INTERFACE library for our C module.
add_library(included_roms INTERFACE )

# Add our source files to the lib
target_sources(included_roms INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/included_roms.c
)

set_source_files_properties(
    ${CMAKE_CURRENT_LIST_DIR}/included_roms.c
    PROPERTIES COMPILE_FLAGS "-O3"
)

set(BFDARG elf32-littlearm)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/attr.o
	WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
	COMMAND ${CMAKE_OBJCOPY} -I binary -O ${BFDARG} --rename-section .data=.rodata attr.bin ${CMAKE_CURRENT_BINARY_DIR}/attr.o
	#places in data
	DEPENDS ${CMAKE_CURRENT_LIST_DIR}/attr.bin
)

add_custom_target(attr_obj DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/attr.o)


add_library(attr INTERFACE)
add_dependencies(attr attr_obj)
set_target_properties(attr PROPERTIES
  INTERFACE_LINK_LIBRARIES "${CMAKE_CURRENT_BINARY_DIR}/attr.o")



target_link_libraries(included_roms INTERFACE optionrom attr)


# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE included_roms)
