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
        sc_mutex.h
        sc_mutex.c
        sc_time.h
        sc_time.c
        sc_thread.h
        sc_thread.c
        sc_option.h
        sc_option.c
        hdr_atomic.h
        hdr_histogram.h
        hdr_histogram.c
        ${CMAKE_SOURCE_DIR}/c-client/resql.h
        ${CMAKE_SOURCE_DIR}/c-client/resql.c
        )

target_link_libraries(resql-benchmark ${ADDITIONAL_LIBRARIES})
target_include_directories(resql-benchmark PRIVATE ${CMAKE_SOURCE_DIR}/c-client)

set_source_files_properties(
        hdr_atomic.h
        hdr_histogram.h
        hdr_histogram.c
        PROPERTIES
        COMPILE_FLAGS "-w"
)

install(TARGETS resql-benchmark RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin)



