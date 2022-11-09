cmake_minimum_required(VERSION 3.1)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(MY_PROJECT_NAME HF_CSV) 
project(${MY_PROJECT_NAME} C)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/debug)
else()
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/release)
endif()

add_executable(${MY_PROJECT_NAME}_test ./src/main.c)

# Make compiler scream out every possible warning
# Make compiler scream out every possible warning
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(${MY_PROJECT_NAME}_test PRIVATE -Wstrict-prototypes -Wconversion -Wall -Wextra -Wpedantic -pedantic -Werror)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(${MY_PROJECT_NAME}_test PRIVATE -Wstrict-prototypes -Wconversion -Wall -Wextra -Wpedantic -pedantic -Werror)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	target_compile_options(${MY_PROJECT_NAME}_test PRIVATE /W4 /WX)
endif()
#copy all resources
file(GLOB_RECURSE resources ${CMAKE_SOURCE_DIR}/res/*.*)
file(COPY ${resources} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/res)
