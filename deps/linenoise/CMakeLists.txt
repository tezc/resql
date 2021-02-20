target_sources(resql-cli PUBLIC
        linenoise.h
        linenoise.c)

set_source_files_properties(
        linenoise.c
        PROPERTIES
        COMPILE_FLAGS "-w"
)