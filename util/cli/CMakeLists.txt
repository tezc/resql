set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(resql-cli
        resql_cli.c
        ${CMAKE_SOURCE_DIR}/cresql/resql.h
        ${CMAKE_SOURCE_DIR}/cresql/resql.c
        ${CMAKE_SOURCE_DIR}/lib/linenoise/linenoise.h
        ${CMAKE_SOURCE_DIR}/lib/linenoise/linenoise.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_option.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_option.c
        )

target_include_directories(resql-cli PRIVATE ${CMAKE_SOURCE_DIR}/cresql)
target_include_directories(resql-cli PRIVATE ${CMAKE_SOURCE_DIR}/lib/linenoise)
target_include_directories(resql-cli PRIVATE ${CMAKE_SOURCE_DIR}/lib/sc)
set_source_files_properties(
        ${CMAKE_SOURCE_DIR}/lib/linenoise/linenoise.h
        ${CMAKE_SOURCE_DIR}/lib/linenoise/linenoise.c
        PROPERTIES
        COMPILE_FLAGS "-w"
)

install(TARGETS resql-cli RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin)



