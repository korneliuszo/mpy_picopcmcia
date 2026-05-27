# Create an INTERFACE library for our C module.
add_library(picopcmcia_low INTERFACE)

# Add our source files to the lib
target_sources(picopcmcia_low INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/picopcmcia_low.c
    ${CMAKE_CURRENT_LIST_DIR}/attr_rom.c
)

set_source_files_properties(
    ${CMAKE_CURRENT_LIST_DIR}/picopcmcia_low.c
    ${CMAKE_CURRENT_LIST_DIR}/attr_rom.c
    PROPERTIES COMPILE_FLAGS "-O3"
)

# Add the current directory as an include directory.
target_include_directories(picopcmcia_low INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE picopcmcia_low)
