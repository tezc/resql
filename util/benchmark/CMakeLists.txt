set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(Threads REQUIRED)
set(ADDITIONAL_LIBRARIES ${CMAKE_THREAD_LIBS_INIT} -ldl -lm)

if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "4")
    list(APPEND ADDITIONAL_LIBRARIES -latomic)
endif ()

if (NOT HAVE_CLOCK_GETTIME AND ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    list(APPEND ADDITIONAL_LIBRARIES rt)
endif ()

add_executable(resql-benchmark
        resql_benchmark.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_mutex.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_mutex.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_time.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_time.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_thread.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_thread.c
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_option.h
        ${CMAKE_SOURCE_DIR}/lib/sc/sc_option.c
        ${CMAKE_SOURCE_DIR}/lib/hdr/hdr_atomic.h
        ${CMAKE_SOURCE_DIR}/lib/hdr/hdr_histogram.h
        ${CMAKE_SOURCE_DIR}/lib/hdr/hdr_histogram.c
        ${CMAKE_SOURCE_DIR}/cresql/resql.h
        ${CMAKE_SOURCE_DIR}/cresql/resql.c
        )

target_link_libraries(resql-benchmark ${ADDITIONAL_LIBRARIES})
target_include_directories(resql-benchmark PRIVATE ${CMAKE_SOURCE_DIR}/cresql)
target_include_directories(resql-benchmark PRIVATE ${CMAKE_SOURCE_DIR}/lib/sc)
target_include_directories(resql-benchmark PRIVATE ${CMAKE_SOURCE_DIR}/lib/hdr)

set_source_files_properties(
        ${CMAKE_SOURCE_DIR}/lib/hdr/hdr_atomic.h
        ${CMAKE_SOURCE_DIR}/lib/hdr/hdr_histogram.h
        ${CMAKE_SOURCE_DIR}/lib/hdr/hdr_histogram.c
        PROPERTIES
        COMPILE_FLAGS "-w"
)

install(TARGETS resql-benchmark RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin)



