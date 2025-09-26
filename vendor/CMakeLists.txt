add_subdirectory(SDL)

set_target_properties(SDL3-shared PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug")
