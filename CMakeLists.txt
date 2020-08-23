cmake_minimum_required(VERSION 3.2)
project(threadpooltest)

if(MSVC)
    # /std:c++latest -> for c++ standarts
	# /W4 - warning level
	# /WX - treat warnings as errors
    add_compile_options("/std:c++latest" "/W4" "/WX")
	# "-D_WIN32_WINNT=0x0501 -> supress warning in boost
	add_compile_options("-D_WIN32_WINNT=0x0501")
else()
    # -std=c++1z -> for c++ standarts
    # -Wall for WARNING
	# -Werror -> Make all warnings into errors.
    # -pedantic -> for warning between c and c++
    # -pthread for portability
    # -g -O0 -fprofile-arcs -ftest-coverage for code coverage
    add_compile_options("-std=c++11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pedantic")
endif()

# Dependencies
if(NOT MSVC)
    find_package(Threads REQUIRED)
endif()

# Create executable
set(EXECUTABLE_NAME TestThreadPool)
set(TEST_FILES Test.cpp)
add_executable(${EXECUTABLE_NAME} ${TEST_FILES})

## Linking up all libraries
if(NOT MSVC)
	target_link_libraries(${EXECUTABLE_NAME} pthread)
endif()

add_test(NAME ${EXECUTABLE_NAME} COMMAND ${EXECUTABLE_NAME})
