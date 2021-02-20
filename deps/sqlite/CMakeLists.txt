target_sources(resql PUBLIC
        sqlite3.h
        sqlite3ext.h
        sqlite3.c
        completion.c)

target_compile_definitions(resql PRIVATE
        -DSQLITE_CORE
        -DSQLITE_OMIT_AUTOINIT
        -DSQLITE_THREADSAFE=0
        -DSQLITE_DEFAULT_MEMSTATUS=0
        -DSQLITE_DQS=0
        -DSQLITE_ENABLE_JSON1
        -DSQLITE_ENABLE_FST5
        -DSQLITE_ENABLE_DBSTAT_VTAB
        -DSQLITE_LIKE_DOESNT_MATCH_BLOBS
        -DSQLITE_MAX_EXPR_DEPTH=0
        -DSQLITE_OMIT_DECLTYPE
        -DSQLITE_OMIT_DEPRECATED
        -DSQLITE_OMIT_PROGRESS_CALLBACK
        -DSQLITE_OMIT_SHARED_CACHE
        -DSQLITE_USE_ALLOCA)

target_include_directories(resql PUBLIC .)

set_source_files_properties(
        sqlite3.c
        PROPERTIES
        COMPILE_FLAGS "-w"
)
