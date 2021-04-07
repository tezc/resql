set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(resql-cli
        resql_cli.c
        ../../cresql/resql.h
        ../../cresql/resql.c
        ../../lib/linenoise/linenoise.h
        ../../lib/linenoise/linenoise.c
        ../../lib/sc/sc_option.h
        ../../lib/sc/sc_option.c
        )

target_include_directories(resql-cli PRIVATE ../../cresql)
target_include_directories(resql-cli PRIVATE ../../lib/linenoise)
target_include_directories(resql-cli PRIVATE ../../lib/sc)
set_source_files_properties(
        ../../lib/linenoise/linenoise.h
        ../../lib/linenoise/linenoise.c
        PROPERTIES
        COMPILE_FLAGS "-w"
)

install(TARGETS resql-cli RUNTIME DESTINATION ../../bin)



