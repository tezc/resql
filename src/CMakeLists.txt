add_library(resql-server STATIC
        aux.h
        aux.c
        client.h
        client.c
        cmd.h
        cmd.c
        config.h
        settings.h
        settings.c
        info.h
        info.c
        meta.h
        meta.c
        node.h
        node.c
        store.h
        store.c
        entry.h
        entry.c
        page.h
        page.c
        session.h
        session.c
        server.h
        server.c
        snapshot.h
        snapshot.c
        state.h
        state.c
        conn.h
        conn.c
        file.h
        file.c
        metric.h
        metric.c
        msg.h
        msg.c
        rs.h
        rs.c
        ../deps/sqlite/sqlite3ext.h
        ../deps/sqlite/sqlite3.h
        ../deps/sqlite/sqlite3.c
        ../deps/sqlite/completion.c
        ../deps/sc/sc.h
        ../deps/sc/sc.c
        ../deps/sc/sc_array.h
        ../deps/sc/sc_array.c
        ../deps/sc/sc_buf.h
        ../deps/sc/sc_buf.c
        ../deps/sc/sc_cond.h
        ../deps/sc/sc_cond.c
        ../deps/sc/sc_crc32.h
        ../deps/sc/sc_crc32.c
        ../deps/sc/sc_ini.h
        ../deps/sc/sc_ini.c
        ../deps/sc/sc_map.h
        ../deps/sc/sc_map.c
        ../deps/sc/sc_mmap.h
        ../deps/sc/sc_mmap.c
        ../deps/sc/sc_log.h
        ../deps/sc/sc_log.c
        ../deps/sc/sc_list.h
        ../deps/sc/sc_list.c
        ../deps/sc/sc_option.h
        ../deps/sc/sc_option.c
        ../deps/sc/sc_queue.h
        ../deps/sc/sc_queue.c
        ../deps/sc/sc_signal.h
        ../deps/sc/sc_signal.c
        ../deps/sc/sc_str.h
        ../deps/sc/sc_str.c
        ../deps/sc/sc_sock.h
        ../deps/sc/sc_sock.c
        ../deps/sc/sc_time.h
        ../deps/sc/sc_time.c
        ../deps/sc/sc_timer.h
        ../deps/sc/sc_timer.c
        ../deps/sc/sc_thread.h
        ../deps/sc/sc_thread.c
        ../deps/sc/sc_uri.h
        ../deps/sc/sc_uri.c)

# -DSQLITE_THREADSAFE=0 \
set_source_files_properties(
        ../deps/sqlite/sqlite3.h
        ../deps/sqlite/sqlite3.c
        ../deps/sqlite/sqlite3ext.h
        ../deps/sqlite/completion.c
        APPEND PROPERTIES COMPILE_FLAGS
        "-w \
 -DSQLITE_OMIT_AUTOINIT \
 -DSQLITE_DQS=0 \
 -DSQLITE_ENABLE_JSON1 \
 -DSQLITE_ENABLE_FTS5 \
 -DSQLITE_ENABLE_DBSTAT_VTAB \
 -DSQLITE_LIKE_DOESNT_MATCH_BLOBS \
 -DSQLITE_MAX_EXPR_DEPTH=0 \
 -DSQLITE_OMIT_DECLTYPE \
 -DSQLITE_OMIT_DEPRECATED \
 -DSQLITE_OMIT_PROGRESS_CALLBACK \
 -DSQLITE_OMIT_SHARED_CACHE \
 -DSQLITE_USE_ALLOCA"
)

add_compile_options(-DSC_HAVE_CONFIG_H)
target_link_libraries(resql-server ${ADDITIONAL_LIBRARIES})
target_include_directories(resql-server PUBLIC ${PROJECT_SOURCE_DIR}/deps)
target_include_directories(resql-server PUBLIC ${PROJECT_SOURCE_DIR}/src)



