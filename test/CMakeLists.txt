set(CMAKE_C_STANDARD 11)

macro(resql_test name)
    add_executable(${name} ${name})
    target_link_libraries(${name} ${PROJECT_LIBS} resql-test)
    add_test(NAME ${name} COMMAND $<TARGET_FILE:${name}>)
endmacro()

add_compile_options(-g -fno-omit-frame-pointer -DRS_ENABLE_ASSERT)

add_library(resql-server STATIC
        ../src/aux.h
        ../src/aux.c
        ../src/client.h
        ../src/client.c
        ../src/cmd.h
        ../src/cmd.c
        ../src/conf.h
        ../src/conf.c
        ../src/info.h
        ../src/info.c
        ../src/meta.h
        ../src/meta.c
        ../src/node.h
        ../src/node.c
        ../src/store.h
        ../src/store.c
        ../src/entry.h
        ../src/entry.c
        ../src/page.h
        ../src/page.c
        ../src/session.h
        ../src/session.c
        ../src/server.h
        ../src/server.c
        ../src/snapshot.h
        ../src/snapshot.c
        ../src/state.h
        ../src/state.c
        ../src/conn.h
        ../src/conn.c
        ../src/file.h
        ../src/file.c
        ../src/metric.h
        ../src/metric.c
        ../src/msg.h
        ../src/msg.c
        ../src/rs.h
        ../src/rs.c
        ../cresql/resql.h
        ../cresql/resql.c
        ../lib/sqlite/sqlite3ext.h
        ../lib/sqlite/sqlite3.h
        ../lib/sqlite/sqlite3.c
        ../lib/sqlite/completion.c
        ../lib/sc/sc.h
        ../lib/sc/sc.c
        ../lib/sc/sc_array.h
        ../lib/sc/sc_array.c
        ../lib/sc/sc_buf.h
        ../lib/sc/sc_buf.c
        ../lib/sc/sc_cond.h
        ../lib/sc/sc_cond.c
        ../lib/sc/sc_crc32.h
        ../lib/sc/sc_crc32.c
        ../lib/sc/sc_ini.h
        ../lib/sc/sc_ini.c
        ../lib/sc/sc_map.h
        ../lib/sc/sc_map.c
        ../lib/sc/sc_mmap.h
        ../lib/sc/sc_mmap.c
        ../lib/sc/sc_log.h
        ../lib/sc/sc_log.c
        ../lib/sc/sc_list.h
        ../lib/sc/sc_list.c
        ../lib/sc/sc_option.h
        ../lib/sc/sc_option.c
        ../lib/sc/sc_queue.h
        ../lib/sc/sc_queue.c
        ../lib/sc/sc_signal.h
        ../lib/sc/sc_signal.c
        ../lib/sc/sc_str.h
        ../lib/sc/sc_str.c
        ../lib/sc/sc_sock.h
        ../lib/sc/sc_sock.c
        ../lib/sc/sc_time.h
        ../lib/sc/sc_time.c
        ../lib/sc/sc_timer.h
        ../lib/sc/sc_timer.c
        ../lib/sc/sc_thread.h
        ../lib/sc/sc_thread.c
        ../lib/sc/sc_uri.h
        ../lib/sc/sc_uri.c)

# -DSQLITE_THREADSAFE=0 \
set_source_files_properties(
        ../lib/sqlite/sqlite3.h
        ../lib/sqlite/sqlite3.c
        ../lib/sqlite/sqlite3ext.h
        ../lib/sqlite/completion.c
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

target_include_directories(resql-server PUBLIC ../lib)

add_library(resql-test test_util.c test_util.h)
target_link_libraries(resql-test resql-server ${ADDITIONAL_LIBRARIES})
target_include_directories(resql-test PUBLIC ../cresql)
target_include_directories(resql-test PUBLIC ../src)

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
        --output-file coverage.info --rc lcov_branch_coverage=1 --rc lcov_excl_br_line='assert' --rc lcov_excl_line='abort'
        COMMAND lcov --remove coverage.info '/usr/*' '*test*' '*/cresql/*' '*/sqlite/*' '*/sc/*'
        --output-file coverage.info --rc lcov_branch_coverage=1 --rc lcov_excl_br_line='assert' --rc lcov_excl_line='abort'
        COMMAND lcov --list coverage.info --rc lcov_branch_coverage=1 --rc lcov_excl_br_line='assert' --rc lcov_excl_line='abort'
)

add_dependencies(coverage check)