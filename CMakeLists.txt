# CMakeList.txt : CMake project for LearnSDL3, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(1_getting_started
	1.1_creating_a_window
	2.1_hello_window
)

set(SDL3_DEBUG_BUILD_PATH "${CMAKE_BINARY_DIR}/bin/Debug/SDL3")

add_subdirectory(vendor/SDL)
set_target_properties(SDL3-shared PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_DEBUG ${SDL3_DEBUG_BUILD_PATH})

foreach (SECTION ${1_getting_started})
	file(GLOB SOURCE_FILES
		"src/1_getting_started/${SECTION}/*.cpp"
	)

	project(${SECTION})

	add_executable(${SECTION} ${SOURCE_FILES})
	target_link_libraries(${SECTION} PRIVATE SDL3::SDL3)

	set(DEBUG_PATH ${CMAKE_BINARY_DIR}/1_getting_started/${SECTION})

	set_target_properties(${SECTION} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DEBUG_PATH}
	)

	add_custom_command(TARGET ${SECTION} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${SDL3_DEBUG_BUILD_PATH} ${DEBUG_PATH}
	)

	set_property(TARGET ${SECTION} PROPERTY CXX_STANDARD 23)
endforeach()


