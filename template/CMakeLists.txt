cmake_minimum_required(VERSION 3.12)

# force all output to the same place
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")

# locate hop-engine library. you should move this into a local lib folder, probably
set(HOP_ENGINE "${CMAKE_SOURCE_DIR}/..")
# set(HOP_ENGINE "${CMAKE_SOURCE_DIR}/lib/hop-engine")
if(WIN32)
	set(HOP_PACKAGE_TOOL "package-tool.exe")
	set(HOP_LIB "hop-engine.dll")
	set(HOP_LINK "hop-engine.lib")
else()
	set(HOP_PACKAGE_TOOL "package-tool")
	set(HOP_LIB "libhop-engine.so")
	set(HOP_LINK "libhop-engine.so")
endif()

# you can customise `EXPORT_DIR` and `EXPORT_EXE` to your liking
set(EXPORT_DIR "${CMAKE_SOURCE_DIR}/export")
set(EXPORT_EXE "mygame")

# template project
project(template)

# custom behaviour to allow debugging, even if the libraries are not built for debug mode
if(WIN32)
	set(CMAKE_C_COMPILER "cl.exe")
	set(CMAKE_CXX_COMPILER "cl.exe")

	add_compile_options("/ZI")
	add_link_options("/DEBUG")

	if(${GENERATE_DEBUG})
		string(REPLACE
			"/O2"
			"/Od"
			CMAKE_CXX_FLAGS_RELEASE
			"${CMAKE_CXX_FLAGS_RELEASE}"
		)
		add_link_options("/INCREMENTAL")
	endif()
else()
	set(CMAKE_C_COMPILER "/usr/bin/clang")
	set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

	add_compile_options("-g")
	add_compile_options("-O1")
	add_link_options("-g")
endif()

file(GLOB SRC_FILES
	"src/*.cpp"
	"src/*/*.cpp"
)

# define executable target
add_executable(build ${SRC_FILES})

# add hop-engine and glm include directories
target_include_directories(build
	PRIVATE "${HOP_ENGINE}/include"
)
# link against hop-engine shared library
target_link_libraries(build
	PRIVATE "${HOP_ENGINE}/lib/${HOP_LINK}"
)

# ensure C++20 is in use
set_property(TARGET build PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET build PROPERTY CXX_STANDARD 20)

if(WIN32)
else()
	target_compile_options(build
		PRIVATE
			-fno-delete-null-pointer-checks
			-Wno-tautological-undefined-compare
	)
endif()

# compile game resources into a package file
add_custom_target(resources
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND ${HOP_ENGINE}/tools/${HOP_PACKAGE_TOOL} res -c -b
		"${CMAKE_BINARY_DIR}/resources.hop"
	COMMENT "Compressing resources"
)

# copy dependencies into output directory
add_custom_target(libs
	COMMAND ${CMAKE_COMMAND} -E copy "${HOP_ENGINE}/lib/${HOP_LIB}"
		"${CMAKE_BINARY_DIR}/${HOP_LIB}"
	COMMENT "Copying dependency"
)

add_dependencies(build resources)
add_dependencies(build libs)

# export target which copies the built files to an export directory
if(WIN32)
	set(EXPORT_EXE "${EXPORT_EXE}.exe")
else()
endif()
add_custom_target(export
	DEPENDS build
	COMMAND ${CMAKE_COMMAND} -E make_directory ${EXPORT_DIR}
	COMMAND ${CMAKE_COMMAND} -E copy "${HOP_ENGINE}/lib/${HOP_LIB}"
		"${EXPORT_DIR}/${HOP_LIB}"
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/resources.hop"
		"${EXPORT_DIR}/resources.hop"
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:build>
		"${EXPORT_DIR}/${EXPORT_EXE}"
	COMMENT "Exporting project"
)