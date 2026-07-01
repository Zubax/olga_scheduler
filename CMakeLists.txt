# This software is distributed under the terms of the MIT License.
# Copyright (c) 2024 Zubax Robotics  <info@zubax.com>

cmake_minimum_required(VERSION 3.12)
project(olga_scheduler_tests C CXX)
enable_testing()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CTEST_OUTPUT_ON_FAILURE ON)

# Use -DNO_STATIC_ANALYSIS=1 to suppress static analysis. If not suppressed, the tools used here shall be available.
if (NOT NO_STATIC_ANALYSIS)
    find_program(clang_tidy NAMES clang-tidy)
    if (NOT clang_tidy)
        message(FATAL_ERROR "Could not locate clang-tidy")
    endif ()
    message(STATUS "Using clang-tidy: ${clang_tidy}")
    set(CMAKE_CXX_CLANG_TIDY ${clang_tidy})
endif ()

find_program(clang_format NAMES clang-format)
if (NOT clang_format)
    message(STATUS "Could not locate clang-format")
else ()
    file(GLOB format_files
            ${CMAKE_CURRENT_SOURCE_DIR}/include/*/*.h*
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.[ch]*
    )
    message(STATUS "Using clang-format: ${clang_format}; files: ${format_files}")
    add_custom_target(format COMMAND ${clang_format} -i -fallback-style=none -style=file --verbose ${format_files})
endif ()

include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
)
FetchContent_MakeAvailable(googletest)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic -fstrict-aliasing")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wdouble-promotion -Wswitch-enum -Wfloat-equal -Wundef")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wconversion -Wsign-promo")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wsign-conversion -Wcast-align -Wmissing-declarations")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wtype-limits -Wnon-virtual-dtor -Woverloaded-virtual")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes")

option(OLGA_ENABLE_COVERAGE "Enable coverage instrumentation and reporting" OFF)
if (OLGA_ENABLE_COVERAGE)
    # Coverage builds are optimized for instrumentation rather than static analysis.
    set(CMAKE_CXX_CLANG_TIDY "")

    if ((CMAKE_C_COMPILER_ID MATCHES "GNU|Clang") OR (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g --coverage -DNDEBUG")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g --coverage -DNDEBUG")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    else ()
        message(WARNING "Coverage is enabled but the compiler may not support --coverage flags.")
    endif ()

    find_program(gcovr_path NAMES gcovr)
    if (NOT gcovr_path)
        message(FATAL_ERROR "gcovr not found; install it or set OLGA_ENABLE_COVERAGE=OFF.")
    endif ()
    add_custom_target(coverage
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            COMMAND ${gcovr_path}
                    -r ${CMAKE_CURRENT_SOURCE_DIR}
                    --filter ${CMAKE_CURRENT_SOURCE_DIR}/include/olga_scheduler/olga_scheduler\\.h
                    --exclude ${CMAKE_CURRENT_SOURCE_DIR}/include/olga_scheduler/olga_scheduler\\.hpp
                    --txt-metric branch
                    --fail-under-line 100
                    --fail-under-branch 100
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endif ()

add_executable(test_olga_scheduler_cpp20 ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_olga_scheduler.cpp)
set_target_properties(test_olga_scheduler_cpp20 PROPERTIES CXX_STANDARD 20)
target_include_directories(test_olga_scheduler_cpp20 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/olga_scheduler)
target_include_directories(test_olga_scheduler_cpp20 SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/cavl)
target_link_libraries(test_olga_scheduler_cpp20
        PRIVATE
        GTest::gmock_main
)

include(GoogleTest)
gtest_discover_tests(test_olga_scheduler_cpp20)

add_executable(test_olga_scheduler_capi ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_olga_scheduler_c.cpp)
set_target_properties(test_olga_scheduler_capi PROPERTIES CXX_STANDARD 20)
target_include_directories(test_olga_scheduler_capi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/olga_scheduler)
target_include_directories(test_olga_scheduler_capi SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/cavl)
target_link_libraries(test_olga_scheduler_capi
        PRIVATE
        GTest::gmock_main
)
gtest_discover_tests(test_olga_scheduler_capi)

add_executable(olga_scheduler_c_demo ${CMAKE_CURRENT_SOURCE_DIR}/tests/olga_scheduler_c_demo.c)
set_target_properties(olga_scheduler_c_demo PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES C_EXTENSIONS OFF)
target_include_directories(olga_scheduler_c_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/olga_scheduler)
target_include_directories(olga_scheduler_c_demo SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/cavl)
target_compile_options(olga_scheduler_c_demo PRIVATE -Wall -Wextra -Werror -pedantic)

add_executable(basic_example_c ${CMAKE_CURRENT_SOURCE_DIR}/examples/basic_example.c)
set_target_properties(basic_example_c PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES C_EXTENSIONS OFF)
target_include_directories(basic_example_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/olga_scheduler)
target_include_directories(basic_example_c SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/cavl)
target_compile_definitions(basic_example_c PRIVATE _POSIX_C_SOURCE=200809L)
target_compile_options(basic_example_c PRIVATE -Wall -Wextra -Werror -pedantic)
add_custom_target(run_basic_example_c
         COMMAND $<TARGET_FILE:basic_example_c>
         DEPENDS basic_example_c
         USES_TERMINAL
 )

add_executable(basic_example_cpp ${CMAKE_CURRENT_SOURCE_DIR}/examples/basic_example.cpp)
set_target_properties(basic_example_cpp PROPERTIES CXX_STANDARD 20)
target_include_directories(basic_example_cpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/olga_scheduler)
target_include_directories(basic_example_cpp SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/cavl)
target_compile_options(basic_example_cpp PRIVATE -Wall -Wextra -Werror -pedantic)
add_custom_target(run_basic_example_cpp
          COMMAND $<TARGET_FILE:basic_example_cpp>
          DEPENDS basic_example_cpp
          USES_TERMINAL
  )

add_custom_target(examples)
add_dependencies(examples basic_example_c basic_example_cpp task_trace)
add_test(NAME BuildExamples
         COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target examples
)
