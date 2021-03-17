set(CMAKE_C_STANDARD 11)
include_directories(../../src)
include_directories(..)

add_library(resql-example ../test_util.c)
target_link_libraries(resql-example PUBLIC resql-server)

target_include_directories(resql-example PUBLIC ..)


macro(resql_example name)
    add_executable(${name} ${name}.c)
    target_link_libraries(${name} ${PROJECT_LIBS} resql-test)
endmacro()


resql_example(single_test)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/example)
set(CTEST_BINARY_DIRECTORY ${PROJECT_BINARY_DIR}/example)
