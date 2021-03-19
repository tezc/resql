set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(resql-cli
        resql_cli.c
        sc_option.h
        sc_option.c
        linenoise.h
        linenoise.c
        ${CMAKE_SOURCE_DIR}/c-client/resql.h
        ${CMAKE_SOURCE_DIR}/c-client/resql.c
        )

target_include_directories(resql-cli PRIVATE ${CMAKE_SOURCE_DIR}/c-client)
set_source_files_properties(
        linenoise.h
        linenoise.c
        PROPERTIES
        COMPILE_FLAGS "-w"
)

install(TARGETS resql-cli RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin)



