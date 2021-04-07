set(CMAKE_C_STANDARD 11)

macro(resql_test name)
    add_executable(${name} ${name})
    target_link_libraries(${name} ${PROJECT_LIBS} resql-test)
    add_test(NAME ${name} COMMAND $<TARGET_FILE:${name}>)
endmacro()

add_compile_options(-DSC_HAVE_CONFIG_H -g -fno-omit-frame-pointer -DRS_ENABLE_ASSERT)

add_library(resql-test STATIC
        test_util.h
        test_util.c
        ${CMAKE_SOURCE_DIR}/src/aux.h
        ${CMAKE_SOURCE_DIR}/src/aux.c
        ${CMAKE_SOURCE_DIR}/src/client.h
        ${CMAKE_SOURCE_DIR}/src/client.c
        ${CMAKE_SOURCE_DIR}/src/cmd.h
        ${CMAKE_SOURCE_DIR}/src/cmd.c
        ${CMAKE_SOURCE_DIR}/src/conf.h
        ${CMAKE_SOURCE_DIR}/src/conf.c
        ${CMAKE_SOURCE_DIR}/src/info.h
        ${CMAKE_SOURCE_DIR}/src/info.c
        ${CMAKE_SOURCE_DIR}/src/meta.h
        ${CMAKE_SOURCE_DIR}/src/meta.c
        ${CMAKE_SOURCE_DIR}/src/node.h
        ${CMAKE_SOURCE_DIR}/src/node.c
        ${CMAKE_SOURCE_DIR}/src/store.h
        ${CMAKE_SOURCE_DIR}/src/store.c
        ${CMAKE_SOURCE_DIR}/src/entry.h
        ${CMAKE_SOURCE_DIR}/src/entry.c
        ${CMAKE_SOURCE_DIR}/src/page.h
        ${CMAKE_SOURCE_DIR}/src/page.c
        ${CMAKE_SOURCE_DIR}/src/session.h
        ${CMAKE_SOURCE_DIR}/src/session.c
        ${CMAKE_SOURCE_DIR}/src/server.h
        ${CMAKE_SOURCE_DIR}/src/server.c
        ${CMAKE_SOURCE_DIR}/src/snapshot.h
        ${CMAKE_SOURCE_DIR}/src/snapshot.c
        ${CMAKE_SOURCE_DIR}/src/state.h
        ${CMAKE_SOURCE_DIR}/src/state.c
        ${CMAKE_SOURCE_DIR}/src/conn.h
        ${CMAKE_SOURCE_DIR}/src/conn.c
        ${CMAKE_SOURCE_DIR}/src/file.h
        ${CMAKE_SOURCE_DIR}/src/file.c
        ${CMAKE_SOURCE_DIR}/src/metric.h
        ${CMAKE_SOURCE_DIR}/src/metric.c
        ${CMAKE_SOURCE_DIR}/src/msg.h
        ${CMAKE_SOURCE_DIR}/src/msg.c
        ${CMAKE_SOURCE_DIR}/src/rs.h
        ${CMAKE_SOURCE_DIR}/src/rs.c
        ${CMAKE_SOURCE_DIR}/cresql/resql.h
        ${CMAKE_SOURCE_DIR}/cresql/resql.c
        ${CMAKE_SOURCE_DIR}/lib/sqlite/sqlite3ext.h
        ${CMAKE_SOURCE_DIR}/lib/sqlite/sqlite3.h
        ${CMAKE_SOURCE_DIR}/lib/sqlite/sqlite3.c
        ${CMAKE_SOURCE_DIR}/lib/sqlite/completion.c
        ${CMAKE_SOURCE_DIR}/lib/sc/config.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc.c
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
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_map.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_map.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_mmap.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_mmap.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_log.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_log.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_list.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_list.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_option.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_option.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_queue.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_queue.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_signal.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_signal.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_str.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_str.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_sock.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_sock.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_time.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_time.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_timer.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_timer.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_thread.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_thread.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_uri.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_uri.c)

# -DSQLITE_THREADSAFE=0 \
set_source_files_properties(
        ${CMAKE_SOURCE_DIR}/lib/sqlite/sqlite3.h
        ${CMAKE_SOURCE_DIR}/lib/sqlite/sqlite3.c
        ${CMAKE_SOURCE_DIR}/lib/sqlite/sqlite3ext.h
        ${CMAKE_SOURCE_DIR}/lib/sqlite/completion.c
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

target_link_libraries(resql-test ${ADDITIONAL_LIBRARIES})
target_include_directories(resql-test PUBLIC ${CMAKE_SOURCE_DIR}/lib)
target_include_directories(resql-test PUBLIC ${CMAKE_SOURCE_DIR}/cresql)
target_include_directories(resql-test PUBLIC ${CMAKE_SOURCE_DIR}/src)

enable_testing()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/test)
set(CTEST_BINARY_DIRECTORY ${PROJECT_BINARY_DIR}/test)

resql_test(client_test.c)
resql_test(c_client_test.c)
resql_test(cluster_test.c)
resql_test(config_test.c)
resql_test(entry_test.c)
resql_test(leader_test.c)
resql_test(meta_test.c)
resql_test(msg_test.c)
resql_test(page_test.c)
resql_test(restart_test.c)
resql_test(session_test.c)
resql_test(single_test.c)
resql_test(snapshot_test.c)
resql_test(state_test.c)
resql_test(store_test.c)

add_custom_target(check ${CMAKE_COMMAND} -E env CTEST_OUTPUT_ON_FAILURE=1
        ${CMAKE_CTEST_COMMAND} -C $<CONFIG> --verbose
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

SET(MEMORYCHECK_COMMAND_OPTIONS
        "-q --log-fd=2 --trace-children=yes --track-origins=yes       \
           --leak-check=full --show-leak-kinds=all \
           --error-exitcode=255")

add_custom_target(valgrind ${CMAKE_COMMAND}
        -E env CTEST_OUTPUT_ON_FAILURE=1
        ${CMAKE_CTEST_COMMAND} -C $<CONFIG>
        --overwrite MemoryCheckCommandOptions=${MEMORYCHECK_COMMAND_OPTIONS}
        --verbose -T memcheck WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_custom_target(coverage)
add_custom_command(
        TARGET coverage
        COMMAND lcov --capture --directory .
        --output-file coverage.info --rc lcov_branch_coverage=1
        COMMAND lcov --remove coverage.info '/usr/*' '*test*' '*/cresql/*' '*/sqlite/*' '*/sc/*'
        --output-file coverage.info --rc lcov_branch_coverage=1
        COMMAND lcov --list coverage.info --rc lcov_branch_coverage=1
)

add_dependencies(coverage check)