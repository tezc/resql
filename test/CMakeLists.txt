set(CMAKE_C_STANDARD 11)

macro(resql_test name)
    add_executable(${name} ${name} test_util.c test_util.h)
    target_link_libraries(${name} ${PROJECT_LIBS} resql-test)
    add_test(NAME ${name} COMMAND $<TARGET_FILE:${name}>)
endmacro()

add_library(resql-server STATIC
        ../src/aux.h
        ../src/aux.c
        ../src/client.h
        ../src/client.c
        ../src/cmd.h
        ../src/cmd.c
        ../src/config.h
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
        ../src/sqlite/sqlite3ext.h
        ../src/sqlite/sqlite3.h
        ../src/sqlite/sqlite3.c
        ../src/sqlite/completion.c
        ../src/sc/sc.h
        ../src/sc/sc.c
        ../src/sc/sc_array.h
        ../src/sc/sc_array.c
        ../src/sc/sc_buf.h
        ../src/sc/sc_buf.c
        ../src/sc/sc_cond.h
        ../src/sc/sc_cond.c
        ../src/sc/sc_crc32.h
        ../src/sc/sc_crc32.c
        ../src/sc/sc_ini.h
        ../src/sc/sc_ini.c
        ../src/sc/sc_map.h
        ../src/sc/sc_map.c
        ../src/sc/sc_mmap.h
        ../src/sc/sc_mmap.c
        ../src/sc/sc_log.h
        ../src/sc/sc_log.c
        ../src/sc/sc_list.h
        ../src/sc/sc_list.c
        ../src/sc/sc_option.h
        ../src/sc/sc_option.c
        ../src/sc/sc_queue.h
        ../src/sc/sc_queue.c
        ../src/sc/sc_signal.h
        ../src/sc/sc_signal.c
        ../src/sc/sc_str.h
        ../src/sc/sc_str.c
        ../src/sc/sc_sock.h
        ../src/sc/sc_sock.c
        ../src/sc/sc_time.h
        ../src/sc/sc_time.c
        ../src/sc/sc_timer.h
        ../src/sc/sc_timer.c
        ../src/sc/sc_thread.h
        ../src/sc/sc_thread.c
        ../src/sc/sc_uri.h
        ../src/sc/sc_uri.c)

# -DSQLITE_THREADSAFE=0 \
set_source_files_properties(
        ../src/sqlite/sqlite3.h
        ../src/sqlite/sqlite3.c
        ../src/sqlite/sqlite3ext.h
        ../src/sqlite/completion.c
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
#target_include_directories(resql-server PUBLIC ${PROJECT_SOURCE_DIR}/src)

add_library(resql-test test_util.c ../c-client/resql.c)
target_link_libraries(resql-test PUBLIC resql-server)
target_include_directories(resql-test PUBLIC ../c-client)
target_include_directories(resql-test PUBLIC ../src)

enable_testing()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/test)
set(CTEST_BINARY_DIRECTORY ${PROJECT_BINARY_DIR}/test)

resql_test(client_test.c)
resql_test(cluster_test.c)
resql_test(config_test.c)
resql_test(leader_test.c)
resql_test(page_test.c)
resql_test(restart_test.c)
resql_test(session_test.c)
resql_test(snapshot_test.c)
resql_test(state_test.c)
resql_test(store_test.c)

add_custom_target(check ${CMAKE_COMMAND} -E env CTEST_OUTPUT_ON_FAILURE=1
        ${CMAKE_CTEST_COMMAND} -C $<CONFIG> --verbose
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_custom_target(coverage)
add_custom_command(
        TARGET coverage
        COMMAND lcov --capture --directory ../src
        --output-file coverage.info --rc lcov_branch_coverage=1
        COMMAND lcov --remove coverage.info '/usr/*' '*example*' '*test*' '*/deps/*'
        --output-file coverage.info --rc lcov_branch_coverage=1
        COMMAND lcov --list coverage.info --rc lcov_branch_coverage=1
)

add_dependencies(coverage check)