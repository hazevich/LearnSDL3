add_executable(${PROJECT_NAME} LearnSDL3.cpp LearnSDL3.h)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)


set_target_properties(${PROJECT_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
)
