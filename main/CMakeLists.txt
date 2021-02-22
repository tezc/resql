
add_executable(resql
        main.c
        ../src/aux.h
        ../src/aux.c
        ../src/client.h
        ../src/client.c
        ../src/cmd.h
        ../src/cmd.c
        ../src/settings.h
        ../src/settings.c
        ../src/config.h
        ../src/meta.h
        ../src/meta.c
        ../src/node.h
        ../src/node.c
        ../src/server.h
        ../src/server.c
        ../src/session.h
        ../src/session.c
        ../src/snapshot.h
        ../src/snapshot.c
        ../src/store.h
        ../src/store.c
        ../src/entry.h
        ../src/entry.c
        ../src/page.h
        ../src/page.c
        ../src/state.h
        ../src/state.c
        ../src/file.h
        ../src/file.c
        ../src/metric.h
        ../src/metric.c
        ../src/msg.h
        ../src/msg.c
        ../src/rs.h
        ../src/rs.c
        ../clients/c/resql.h
        ../clients/c/resql.c
        ../deps/sqlite/sqlite3.h
        ../deps/sqlite/sqlite3.c
        ../deps/sqlite/sqlite3ext.h
        ../deps/sqlite/completion.c
        ../deps/sc/sc.h
        ../deps/sc/sc.c
        ../deps/sc/sc_queue.h
        ../deps/sc/sc_queue.c
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
        ../deps/sc/sc_list.h
        ../deps/sc/sc_list.c
        ../deps/sc/sc_log.h
        ../deps/sc/sc_log.c
        ../deps/sc/sc_map.h
        ../deps/sc/sc_map.c
        ../deps/sc/sc_mmap.h
        ../deps/sc/sc_mmap.c
        ../deps/sc/sc_option.h
        ../deps/sc/sc_option.c
        ../deps/sc/sc_signal.h
        ../deps/sc/sc_signal.c
        ../deps/sc/sc_sock.h
        ../deps/sc/sc_sock.c
        ../deps/sc/sc_str.h
        ../deps/sc/sc_str.c
        ../deps/sc/sc_thread.h
        ../deps/sc/sc_thread.c
        ../deps/sc/sc_time.h
        ../deps/sc/sc_time.c
        ../deps/sc/sc_timer.h
        ../deps/sc/sc_timer.c
        ../deps/sc/sc_uri.h
        ../deps/sc/sc_uri.c
        ../src/conn.h
        ../src/conn.c
        ../src/info.h
        ../src/info.c)

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

    add_executable(resql-trainer trainer.c ${CMAKE_SOURCE_DIR}/clients/c/resql.c)
    target_include_directories(resql-trainer PRIVATE ${CMAKE_SOURCE_DIR}/clients)
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
        ../deps/sqlite/sqlite3.h
        ../deps/sqlite/sqlite3.c
        ../deps/sqlite/sqlite3ext.h
        ../deps/sqlite/completion.c
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

enable_warning_if_supported(-Wno-stringop-overflow)

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
target_include_directories(resql PUBLIC ../deps)
target_include_directories(resql PUBLIC ../src)
install(TARGETS resql RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin)