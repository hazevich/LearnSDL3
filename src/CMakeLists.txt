add_executable(${PROJECT_NAME} LearnSDL3.cpp LearnSDL3.h)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)


set_target_properties(${PROJECT_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
)
