
add_compile_options(-DSC_HAVE_CONFIG_H)

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
        conn.h
        conn.c
        info.h
        info.c
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
        ${CMAKE_SOURCE_DIR}/lib/sqlite/sqlite3.h
        ${CMAKE_SOURCE_DIR}/lib/sqlite/sqlite3.c
        ${CMAKE_SOURCE_DIR}/lib/sqlite/sqlite3ext.h
        ${CMAKE_SOURCE_DIR}/lib/sqlite/completion.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_queue.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_queue.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_array.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_array.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_buf.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_buf.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_cond.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_cond.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_crc32.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_crc32.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_ini.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_ini.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_list.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_list.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_log.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_log.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_map.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_map.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_mmap.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_mmap.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_option.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_option.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_signal.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_signal.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_sock.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_sock.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_str.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_str.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_thread.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_thread.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_time.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_time.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_timer.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_timer.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_uri.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_uri.c)

target_include_directories(resql PUBLIC ${CMAKE_SOURCE_DIR}/lib)
target_include_directories(resql PUBLIC ${CMAKE_SOURCE_DIR}/src)

set(PGO_DIR "${CMAKE_SOURCE_DIR}/bin/pgo")
set(GCC_PGO_GEN "-fprofile-generate=${PGO_DIR}")
set(GCC_PGO_USE "-fprofile-use=${PGO_DIR}")
set(CLANG_PGO_GENPATH "${PGO_DIR}/gen.profdata")
set(CLANG_PGO_USEPATH "${PGO_DIR}/use.profdata")
set(CLANG_PGO_GEN "-fprofile-instr-generate=${CLANG_PGO_GENPATH}")
set(CLANG_PGO_USE "-fprofile-instr-use=${CLANG_PGO_USEPATH}")
set(RESQL_LINK_LIBRARIES "")

if (${PGO} MATCHES "generate")
    MESSAGE("profile will be generated.")

    if ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
        target_compile_options(resql PRIVATE ${CLANG_PGO_GEN})
        list(APPEND RESQL_LINK_LIBRARIES ${CLANG_PGO_GEN})
    else ()
        target_compile_options(resql PRIVATE ${GCC_PGO_GEN})
        list(APPEND RESQL_LINK_LIBRARIES ${GCC_PGO_GEN})
        link_libraries(gcov)
    endif ()

    add_executable(resql-trainer trainer.c ${CMAKE_SOURCE_DIR}/cresql/resql.c)
    target_include_directories(resql-trainer PRIVATE ${CMAKE_SOURCE_DIR}/cresql)
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
        ${CMAKE_SOURCE_DIR}/lib/sqlite/sqlite3.h
        ${CMAKE_SOURCE_DIR}/lib/sqlite/sqlite3.c
        ${CMAKE_SOURCE_DIR}/lib/sqlite/sqlite3ext.h
        ${CMAKE_SOURCE_DIR}/lib/sqlite/completion.c
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

target_link_libraries(resql ${RESQL_LINK_LIBRARIES} ${ADDITIONAL_LIBRARIES})
install(TARGETS resql RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin)