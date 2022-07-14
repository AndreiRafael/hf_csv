cmake_minimum_required(VERSION 3.1)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(MY_PROJECT_NAME HF_CSV) 
project(${MY_PROJECT_NAME} C)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_executable(${MY_PROJECT_NAME}_test ./src/main.c)
else()
	if(WIN32)#hide console window on release
		add_executable(${MY_PROJECT_NAME}_test WIN32 ./src/main.c)
	else()
		add_executable(${MY_PROJECT_NAME}_test ./src/main.c)
	endif()
endif()

#export csv.c as dll
add_definitions(-DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=TRUE -DBUILD_SHARED_LIBS=TRUE)
add_library(${MY_PROJECT_NAME} SHARED ./src/hf_csv.c)

# Make compiler scream out every possible warning
if(MSVC)
	target_compile_options(${MY_PROJECT_NAME} PRIVATE /W4 /WX)
	target_compile_options(${MY_PROJECT_NAME}_test PRIVATE /W4 /WX)
else()
	target_compile_options(${MY_PROJECT_NAME} PRIVATE -Wstrict-prototypes -Wconversion -Wall -Wextra -Wpedantic -pedantic -Werror)
	target_compile_options(${MY_PROJECT_NAME}_test PRIVATE -Wstrict-prototypes -Wconversion -Wall -Wextra -Wpedantic -pedantic -Werror)
endif()

target_link_libraries(${MY_PROJECT_NAME}_test ${MY_PROJECT_NAME})

#copy all resources
file(GLOB_RECURSE resources ${CMAKE_SOURCE_DIR}/res/*.*)
file(COPY ${resources} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/res)
