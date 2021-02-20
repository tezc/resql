add_executable(resql-cli
        resql_cli.c
        ../c/resql.h
        ../c/resql.c
        ../../deps/linenoise/linenoise.h
        ../../deps/linenoise/linenoise.c
        ../../deps/sc/sc_time.h
        ../../deps/sc/sc_time.c
        ../../deps/sc/sc_uri.h
        ../../deps/sc/sc_uri.c
        ../../deps/sc/sc_option.h
        ../../deps/sc/sc_option.c
        )

set_source_files_properties(
        ../../deps/linenoise/linenoise.h
        ../../deps/linenoise/linenoise.c
        PROPERTIES
        COMPILE_FLAGS "-w"
)

include_directories(${PROJECT_SOURCE_DIR}/clients)
include_directories(${PROJECT_SOURCE_DIR}/deps)

install(TARGETS resql-cli RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin)



