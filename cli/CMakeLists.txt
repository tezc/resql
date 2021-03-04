add_executable(resql-cli
        resql_cli.c
        ../c/resql.c
        ../deps/linenoise/linenoise.h
        ../deps/linenoise/linenoise.c
        ../deps/sc/sc_uri.h
        ../deps/sc/sc_uri.c
        ../deps/sc/sc_option.h
        ../deps/sc/sc_option.c
        )
target_include_directories(resql-cli PRIVATE ..)

set_source_files_properties(
        ../deps/linenoise/linenoise.h
        ../deps/linenoise/linenoise.c
        PROPERTIES
        COMPILE_FLAGS "-w"
)

include_directories(../clients)
include_directories(../deps)

install(TARGETS resql-cli RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin)



