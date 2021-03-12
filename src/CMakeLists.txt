
add_executable(resql
        main.c
        aux.h
        aux.c
        client.h
        client.c
        cmd.h
        cmd.c
        conf.h
        conf.c
        config.h
        meta.h
        meta.c
        node.h
        node.c
        server.h
        server.c
        session.h
        session.c
        snapshot.h
        snapshot.c
        store.h
        store.c
        entry.h
        entry.c
        page.h
        page.c
        state.h
        state.c
        file.h
        file.c
        metric.h
        metric.c
        msg.h
        msg.c
        rs.h
        rs.c
        sqlite/sqlite3.h
        sqlite/sqlite3.c
        sqlite/sqlite3ext.h
        sqlite/completion.c
        sc/sc.h
        sc/sc.c
        sc/sc_queue.h
        sc/sc_queue.c
        sc/sc_array.h
        sc/sc_array.c
        sc/sc_buf.h
        sc/sc_buf.c
        sc/sc_cond.h
        sc/sc_cond.c
        sc/sc_crc32.h
        sc/sc_crc32.c
        sc/sc_ini.h
        sc/sc_ini.c
        sc/sc_list.h
        sc/sc_list.c
        sc/sc_log.h
        sc/sc_log.c
        sc/sc_map.h
        sc/sc_map.c
        sc/sc_mmap.h
        sc/sc_mmap.c
        sc/sc_option.h
        sc/sc_option.c
        sc/sc_signal.h
        sc/sc_signal.c
        sc/sc_sock.h
        sc/sc_sock.c
        sc/sc_str.h
        sc/sc_str.c
        sc/sc_thread.h
        sc/sc_thread.c
        sc/sc_time.h
        sc/sc_time.c
        sc/sc_timer.h
        sc/sc_timer.c
        sc/sc_uri.h
        sc/sc_uri.c
        conn.h
        conn.c
        info.h
        info.c)

set(PGO_DIR "${CMAKE_SOURCE_DIR}/bin/pgo")
set(GCC_PGO_GEN "-fprofile-generate=${PGO_DIR}")
set(GCC_PGO_USE "-fprofile-use=${PGO_DIR}")
set(CLANG_PGO_GENPATH "${PGO_DIR}/gen.profdata")
set(CLANG_PGO_USEPATH "${PGO_DIR}/use.profdata")
set(CLANG_PGO_GEN "-fprofile-instr-generate=${CLANG_PGO_GENPATH}")
set(CLANG_PGO_USE "-fprofile-instr-use=${CLANG_PGO_USEPATH}")

if (${PGO} MATCHES "generate")
    MESSAGE("profile will be generated.")

    if ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
        target_compile_options(resql PRIVATE ${CLANG_PGO_GEN})
        target_link_options(resql PRIVATE ${CLANG_PGO_GEN})
    else ()
        target_compile_options(resql PRIVATE ${GCC_PGO_GEN})
        target_link_options(resql PRIVATE ${GCC_PGO_GEN})
        link_libraries(gcov)
    endif ()

    add_executable(resql-trainer trainer.c ${CMAKE_SOURCE_DIR}/c-client/resql.c)
    target_include_directories(resql-trainer PRIVATE ${CMAKE_SOURCE_DIR}/c-client)
    install(TARGETS resql-trainer RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin)
elseif (${PGO} MATCHES "use")
    MESSAGE("Profile will be used.")

    if ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
        if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            execute_process(
                    COMMAND sh -c "xcrun llvm-profdata merge -output=${CLANG_PGO_USEPATH} ${CLANG_PGO_GENPATH}"
                    RESULT_VARIABLE CMD_ERROR
            )
        else ()
            execute_process(
                    COMMAND sh -c "llvm-profdata merge -output=${CLANG_PGO_USEPATH} ${CLANG_PGO_GENPATH}"
                    RESULT_VARIABLE CMD_ERROR
            )
        endif ()

        if (NOT CMD_ERROR EQUAL 0)
            MESSAGE("llvm-profdata failed.")
        else ()
            target_compile_options(resql PRIVATE ${CLANG_PGO_USE})
        endif ()
    else ()
        target_compile_options(resql PRIVATE ${GCC_PGO_USE})
        compile_option(resql -fprofile-partial-training HAVE_PARTIAL)
    endif ()
endif ()

set_source_files_properties(
        sqlite/sqlite3.h
        sqlite/sqlite3.c
        sqlite/sqlite3ext.h
        sqlite/completion.c
        APPEND PROPERTIES
        COMPILE_FLAGS
        "-w \
 -DSQLITE_OMIT_AUTOINIT \
 -DSQLITE_THREADSAFE=0 \
 -DSQLITE_DEFAULT_MEMSTATUS=0 \
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

if (${CMAKE_BUILD_TYPE} MATCHES "Rel")
    check_ipo_supported(RESULT LTO_SUPPORT OUTPUT ERROR_MSG)

    if (LTO_SUPPORT)
        message(STATUS "LTO enabled for resql")
        set_property(TARGET resql PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    else ()
        message(STATUS "LTO not supported: <${ERROR_MSG}>")
    endif ()
endif ()

#--------------------------------------------------------------- #

add_compile_options(-DSC_HAVE_CONFIG_H)
target_link_libraries(resql ${ADDITIONAL_LIBRARIES})
target_include_directories(resql PUBLIC ../../deps)
target_include_directories(resql PUBLIC .)
install(TARGETS resql RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin)