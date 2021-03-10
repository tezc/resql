add_executable(resql-cli
        resql_cli.c
        sc_uri.h
        sc_uri.c
        sc_option.h
        sc_option.c
        linenoise.h
        linenoise.c
        ${CMAKE_SOURCE_DIR}/c/resql.h
        ${CMAKE_SOURCE_DIR}/c/resql.c
        )
set_source_files_properties(
        linenoise.h
        linenoise.c
        PROPERTIES
        COMPILE_FLAGS "-w"
)

install(TARGETS resql-cli RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin)



