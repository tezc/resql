set(CMAKE_C_STANDARD 11)
include_directories(../../src)
include_directories(../)

add_library(resql-example ../test_util.c)
target_link_libraries(resql-example PUBLIC resql-server)

target_include_directories(resql-example PUBLIC ..)

enable_testing()


add_executable(single_test single_test.c)
target_link_libraries(single_test ${PROJECT_LIBS} resql-test)
add_test(NAME single_test COMMAND $<TARGET_FILE:single_test>)

add_executable(cluster2_test cluster2_test.c)
target_link_libraries(cluster2_test ${PROJECT_LIBS} resql-test)
add_test(NAME cluster2_test COMMAND $<TARGET_FILE:cluster2_test>)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/example)
set(CTEST_BINARY_DIRECTORY ${PROJECT_BINARY_DIR}/example)
