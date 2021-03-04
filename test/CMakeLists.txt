set(CMAKE_C_STANDARD 11)
include_directories(../src)

add_library(resql-test test_util.c ../c/resql.c)
target_link_libraries(resql-test PUBLIC resql-server)

target_include_directories(resql-test PUBLIC ..)
target_include_directories(resql-test PUBLIC ../src)
target_include_directories(resql-test PUBLIC ../clients)

enable_testing()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/test)
set(CTEST_BINARY_DIRECTORY ${PROJECT_BINARY_DIR}/test)

file(GLOB files "*_test.c")

foreach (file ${files})
    string(REGEX REPLACE "(^.*/|\\.[^.]*$)" "" file_without_ext ${file})
    add_executable(${file_without_ext} ${file} test_util.c test_util.h)
    target_link_libraries(${file_without_ext} ${PROJECT_LIBS} resql-test)
    add_test(NAME ${file_without_ext} COMMAND $<TARGET_FILE:${file_without_ext}>)
endforeach ()

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