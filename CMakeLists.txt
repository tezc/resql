# -------------------- Header start -----------------------------#
project(resql C)
cmake_minimum_required(VERSION 3.9)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
# -------------------- Header end -------------------------------#


# -------------------- Includes start ---------------------------#
include(CheckCCompilerFlag)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckStructHasMember)
include(CheckLibraryExists)
include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(CheckIncludeFiles)
include(CheckIPOSupported)
include(CTest)
include(TestBigEndian)

# -------------------- Includes end -----------------------------#


# -------------------- Macros start ---------------------------- #
macro(define_flag _append_to_var _def)
    message("Adding definition ${_def}")
    set(${_append_to_var} "${${_append_to_var}} -D${_def}")
endmacro()

macro(try_sanitizer _type)
    set(OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})

    set(SANITIZER_FLAG "-fsanitize=${_type}")
    set(CMAKE_REQUIRED_FLAGS "-Werror ${SANITIZER_FLAG}")

    check_c_compiler_flag(${SANITIZER_FLAG} HAVE_SANITIZER)

    set(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQUIRED_FLAGS})
    unset(OLD_CMAKE_REQUIRED_FLAGS)

    if (HAVE_SANITIZER)
        message(STATUS "Building with ${_type} sanitizer")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${SANITIZER_FLAG}")
    endif ()

    unset(HAVE_SANITIZER)
    unset(SANITIZER_FLAG)
endmacro()
# -------------------- Macros end ------------------------------ #


# -------------------- Build type start ------------------------ #
if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, defaulting to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif ()

message(STATUS "Main build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "OS System is ${CMAKE_SYSTEM_NAME}")
message(STATUS "OS Version is ${CMAKE_SYSTEM_VERSION}")
message(STATUS "Compiler is ${CMAKE_C_COMPILER_ID}")
message(STATUS "Compiler version is ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "Arch is ${CMAKE_SYSTEM_PROCESSOR}")

if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    message(STATUS "Target arch is 64 bit architecture")
    define_flag(C_FLAGS_COMMON DEF_ARCH='"64"')
else ()
    message("Target arch is 32 bit architecture")
    define_flag(C_FLAGS_COMMON DEF_ARCH='"32"')
endif ()

# -------------------- Build info end -------------------------- #


#--------------------- Dependencies Start ---------------------- #

find_package(Threads REQUIRED)
set(ADDITIONAL_LIBRARIES ${CMAKE_THREAD_LIBS_INIT} -ldl -lm)

# -------------------- Dependencies End------------------------- #


# -------------------- Git info start -------------------------- #
set(GIT_BRANCH Unknown)
set(GIT_COMMIT_HASH Unknown)

if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
            COMMAND git rev-parse --abbrev-ref HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_BRANCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
            COMMAND git log -1 --format=%h
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif ()

message(STATUS "Git current branch: ${GIT_BRANCH}")
message(STATUS "Git commit hash: ${GIT_COMMIT_HASH}")

define_flag(C_FLAGS_COMMON RS_COMPILER_GIT_COMMIT='"${GIT_COMMIT_HASH}"')
define_flag(C_FLAGS_COMMON RS_COMPILER_GIT_BRANCH='"${GIT_BRANCH}"')

# -------------------- Git info end ---------------------------- #


# -------------------- Memory allocator start ------------------ #
option(USE_MALLOC "Use malloc implementation" "OFF")
if (USE_MALLOC)
    unset(MALLOC_NAME CACHE)
    unset(MALLOC_LIB CACHE)

    if (${USE_MALLOC} STREQUAL "mimalloc")
        set(MALLOC_NAME mimalloc)
    elseif (${USE_MALLOC} STREQUAL "tcmalloc")
        set(MALLOC_NAME tcmalloc_minimal tcmalloc)
    elseif (${USE_MALLOC} STREQUAL "jemalloc")
        set(MALLOC_NAME jemalloc)
    else ()
        set(MALLOC_NAME "mimalloc tcmalloc_minimal tcmalloc jemalloc")
    endif ()

    find_library(MALLOC_LIB NAMES ${MALLOC_NAME})
    if (MALLOC_LIB)
        message(STATUS "Using alternative malloc (${USE_MALLOC}): ${MALLOC_LIB}")
        list(APPEND ADDITIONAL_LIBRARIES ${MALLOC_LIB})
    endif ()

endif ()
# -------------------- Memory allocator end -------------------- #


# -------------------- Sanitizer Start ------------------------- #
option(SANITIZER "Use sanitizer (undefined, address, thread, none)" "none")

if (${SANITIZER} MATCHES "(undefined)")
    try_sanitizer("undefined")
elseif (${SANITIZER} MATCHES "(address)")
    try_sanitizer("address")
elseif (${SANITIZER} MATCHES "(thread)")
    try_sanitizer("thread")
else ()
    message(STATUS "Building without a sanitizer")
endif ()
# -------------------- Sanitizer End --------------------------- #


# -------------------- Operating System Start ------------------ #

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    define_flag(C_FLAGS_COMMON HAVE_LINUX)
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    define_flag(C_FLAGS_COMMON HAVE_OSX)
elseif (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    define_flag(C_FLAGS_COMMON HAVE_FREEBSD)
else ()
    message(AUTHOR_WARNING "WARNING: Unknown platform: ${CMAKE_SYSTEM_NAME}")
    message(AUTHOR_WARNING "${ISSUE_REPORT_MSG}")
endif ()
# -------------------- Operating System End -------------------- #


# -------------------- Valgrind Start -------------------------- #
SET(VALGRIND_COMMAND_OPTIONS "-q")
SET(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full --error-exitcode=255 --log-fd=2")
# -------------------- Valgrind End ---------------------------- #


# -------------------- Code Coverage Start --------------------- #
if (${CMAKE_BUILD_TYPE} MATCHES "Coverage")
    if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
        add_compile_options(--coverage)
        link_libraries(gcov)
    else ()
        message(FATAL_ERROR "Only GCC is supported for coverage")
    endif ()
endif ()

# -------------------- Code Coverage End ------------------------ #

# -------------------- Clang Tidy Start ------------------------- #
if (RESQL_CLANG_TIDY)
    message(STATUS "Enabled CLANG_TIDY")
    set(CMAKE_C_CLANG_TIDY "clang-tidy")
endif ()
# -------------------- Clang Tidy End --------------------------- #

# -------------------- Compile Flags Start ---------------------- #

if (NOT HAVE_CLOCK_GETTIME AND ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    list(APPEND ADDITIONAL_LIBRARIES rt)
endif ()

check_c_source_compiles("
    #include <execinfo.h>
    #include <unistd.h>

    int main(int argc, char **argv) {
        void *array[10];
        size_t size = backtrace(array, 10);
        backtrace_symbols_fd(array, size, STDERR_FILENO);
        return 0;
}" HAVE_BACKTRACE)

if (${HAVE_BACKTRACE})
    define_flag(C_FLAGS_COMMON HAVE_BACKTRACE)
endif ()

set(C_FLAGS_COMMON "${C_FLAGS_COMMON} -Wall -Werror -pedantic -Wextra")
set(C_FLAGS_DEB "${C_FLAGS_DEB} -fno-omit-frame-pointer")

message(STATUS "Additional Libraries : ${ADDITIONAL_LIBRARIES}")

if (CMAKE_SYSTEM_PROCESSOR MATCHES "amd64.*|x86_64.*|AMD64.*")
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(STATUS "CPU is x86_64, defined HAVE_CRC32C")
        add_compile_options(-msse4.2)
        define_flag(C_FLAGS_COMMON HAVE_CRC32C)
    endif ()
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64.*|AARCH64.*|arm64.*|ARM64.*)")
    message(STATUS "CPU = aarch64, defined HAVE_CRC32C")
    add_compile_options(-march=armv8.1-a)
    define_flag(C_FLAGS_COMMON HAVE_CRC32C)
endif ()

# detect software version endianness
test_big_endian(HAVE_BIG_ENDIAN)
if (${HAVE_BIG_ENDIAN})
    define_flag(C_FLAGS_COMMON HAVE_BIG_ENDIAN)
else ()
    message(STATUS "System is LITTLE ENDIAN")
endif ()

set(C_FLAGS_COMMON
        "${C_FLAGS_COMMON} -D'RESQL_FILE_NAME=\"$(subst $(realpath ${CMAKE_SOURCE_DIR})/,,$(abspath $<))\"'")

set(C_FLAGS_COMMON "${C_FLAGS_COMMON} -D__FILE_OFFSET_BITS=64 -D_GNU_SOURCE")

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    if ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-export-dynamic")
    else ()
        set(C_FLAGS_COMMON "${C_FLAGS_COMMON} -rdynamic")
    endif ()
endif ()

set(CMAKE_C_FLAGS_COVERAGE "${CMAKE_C_FLAGS_COVERAGE} ${C_FLAGS_COMMON} ${C_FLAGS_DEB}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${C_FLAGS_COMMON} ${C_FLAGS_DEB}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${C_FLAGS_COMMON} ${C_FLAGS_REL}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} ${C_FLAGS_COMMON} ${C_FLAGS_REL}")
set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} ${C_FLAGS_COMMON} ${C_FLAGS_REL}")


# -------------------- Compile Flags End ----------------------- #


# -------------------- Subfolder Start ------------------------- #

add_subdirectory(src)
add_subdirectory(util/cli)
add_subdirectory(util/benchmark)

if (RESQL_BUILD_TESTS)
    add_subdirectory(test)
    add_subdirectory(test/example)
endif ()
# -------------------- Subfolder End --------------------------- #


