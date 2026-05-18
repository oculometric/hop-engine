cmake_minimum_required(VERSION 3.27)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")

# ================ PROJECT 'hop-engine' ===================================================
project(hop-engine)

if(WIN32)
	set(CMAKE_C_COMPILER "cl.exe")
	set(CMAKE_CXX_COMPILER "cl.exe")

	add_compile_options("/ZI")
	add_link_options("/DEBUG")

	if(${GENERATE_DEBUG})
		string(REPLACE "/O2" "/Od" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
		add_link_options("/INCREMENTAL")
	endif()
else()
	set(CMAKE_C_COMPILER "/usr/bin/clang")
	set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

	add_compile_options("-g")
	add_compile_options("-O1")
	add_link_options("-g")
endif()

find_package(Vulkan REQUIRED COMPONENTS SPIRV-Tools)
set(ENGINE_HOP "${CMAKE_BINARY_DIR}/engine.hop")
execute_process(
	COMMAND git log --pretty=format:%h -n 1
	OUTPUT_VARIABLE GIT_TAG
	ERROR_QUIET
)
add_compile_definitions(HOP_ENGINE_COMMIT=${GIT_TAG})

# ================ EXECUTABLE TARGET 'package-tool' =======================================
add_executable(package-tool
	"src/package-builder/package-builder.cpp"
	"src/filesystem/filesystem.cpp"
	"src/filesystem/package_file.cpp"
	"src/filesystem/package_management.cpp"
	"src/rendering/binmesh_loading.cpp"
	"src/rendering/obj_loading.cpp"
	"src/misc/exec.cpp"
	"src/core/debug.cpp"
)

target_include_directories(package-tool
	PRIVATE
		"lib/inc"
		"lib/inc/glm-1.0.2"
		"inc"
)
set_property(TARGET package-tool PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET package-tool PROPERTY CXX_STANDARD 20)

if(WIN32)
else()
	target_compile_options(package-tool
		PRIVATE
			-fno-delete-null-pointer-checks
			-Wno-tautological-undefined-compare
	)
endif()

# ================ EXECUTABLE TARGET 'bin-to-cpp' =======================================
add_executable(bin-to-cpp
	"src/package-builder/bin_to_cpp.cpp"
)

set_property(TARGET bin-to-cpp PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET bin-to-cpp PROPERTY CXX_STANDARD 20)

if(WIN32)
	string(REPLACE "/Od" "/O2" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
else()
	target_compile_options(bin-to-cpp PRIVATE -O3)
endif()

# ================ RESOURCE TARGET 'engine.hop' ===========================================
file(GLOB RES_ENGINE
	"res_engine/*"
	"res_engine/*/*"
	"res_engine/*/*/*"
)

add_custom_command(
	OUTPUT ${ENGINE_HOP}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND package-tool res_engine -c -b -p "engine/" ${ENGINE_HOP}
	DEPENDS
		package-tool
		${RES_ENGINE}
	COMMENT "Generating engine.hop"
)

# ================ STATIC LIBRARY TARGET 'hop-engine' =====================================
file(GLOB SRC_FILES
	"src/*.cpp"
	"src/*/*.cpp"
	"lib/src/*.cpp"
)
list(FILTER SRC_FILES
	EXCLUDE
	REGEX "src/package-builder/package-builder.cpp"
)
list(FILTER SRC_FILES
	EXCLUDE
	REGEX "src/package-builder/bin_to_cpp.cpp"
)
list(FILTER SRC_FILES
	EXCLUDE
	REGEX "src/editor/main.cpp"
)
set(INC_DIRS
	"lib/inc"
	"lib/inc/glm-1.0.2"
	"lib/inc/imgui"
	"lib/inc/imgui/backends"
	"inc"
	${Vulkan_INCLUDE_DIRS}
	"lib/inc/glslang"
)
set(LIBS
	Vulkan::Vulkan
)

if(WIN32)
	list(APPEND LIBS
		"${CMAKE_SOURCE_DIR}/lib/bin/GLFW/lib-vc2019/glfw3.lib"
		"${Vulkan_INCLUDE_DIRS}/../Lib/glslang-default-resource-limits.lib"
		"${Vulkan_INCLUDE_DIRS}/../Lib/SPIRV-Tools-opt.lib"
		"${Vulkan_INCLUDE_DIRS}/../Lib/glslang.lib"
		"${CMAKE_SOURCE_DIR}/lib/bin/discord-presence/discord-rpc.lib"
		"${CMAKE_SOURCE_DIR}/lib/bin/discord-presence/fmt.lib"
		"Ws2_32.lib"
		Vulkan::SPIRV-Tools
	)
else()
	list(APPEND LIBS
		"${CMAKE_SOURCE_DIR}/lib/bin/glslang/libglslang-default-resource-limits.a"
		"${CMAKE_SOURCE_DIR}/lib/bin/glslang/libglslang.a"
		"${CMAKE_SOURCE_DIR}/lib/bin/glslang/libSPIRV-Tools-opt.a"
		"${CMAKE_SOURCE_DIR}/lib/bin/glslang/libSPIRV-Tools.a"
		"${CMAKE_SOURCE_DIR}/lib/bin/discord-presence/libdiscord-rpc.a"
		"${CMAKE_SOURCE_DIR}/lib/bin/discord-presence/libfmt.a"
		pthread
		glfw
		dl
		X11
		Xrandr
		Xi
	)
endif()

add_custom_command(
	OUTPUT ${CMAKE_BINARY_DIR}/engine_resource_binary.cpp
	COMMAND $<TARGET_FILE:bin-to-cpp> ${ENGINE_HOP}
		"${CMAKE_BINARY_DIR}/engine_resource_binary.cpp" engine_hop_raw
	DEPENDS
		${ENGINE_HOP}
		bin-to-cpp
	COMMENT "Converting engine.hop to CPP format"
)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(hop-engine SHARED
	${SRC_FILES}
	${CMAKE_BINARY_DIR}/engine_resource_binary.cpp
)

target_include_directories(hop-engine PRIVATE ${INC_DIRS})
target_link_libraries(hop-engine PRIVATE ${LIBS})
set_property(TARGET hop-engine PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET hop-engine PROPERTY CXX_STANDARD 20)

if(MSVC)
	target_compile_options(hop-engine PRIVATE /wd4100 /wd4101)
	string(REPLACE "/W3" "/W4" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
	string(REPLACE "/W3" "/W4" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
else()
	target_compile_options(hop-engine PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32)
else()
	target_compile_options(hop-engine
		PRIVATE
			-fno-delete-null-pointer-checks
			-Wno-tautological-undefined-compare
			-Wno-unused-parameter
			-Wno-unused-variable
			-Wno-missing-field-initializers
	)
endif()

# ================ EXECUTABLE TARGET 'hop-editor' =========================================
add_executable(hop-editor "src/editor/main.cpp")

target_link_libraries(hop-editor hop-engine)

set_property(TARGET hop-editor PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET hop-editor PROPERTY CXX_STANDARD 20)
if(WIN32)
else()
	target_compile_options(hop-editor
		PRIVATE
			-fno-delete-null-pointer-checks
			-Wno-tautological-undefined-compare
			-Wno-unused-parameter
			-Wno-unused-variable
			-Wno-missing-field-initializers
	)
endif()

target_include_directories(hop-editor
	PRIVATE
		"lib/inc/glm-1.0.2"
		"lib/inc/imgui"
		"lib/inc/imgui/backends"
		"inc"
)

# ================ EXPORT TARGET 'export' =================================================
string(TIMESTAMP TODAY "%d_%m_%Y %H.%M.%S")
set(ZIP_COMMAND "tar")
if(WIN32)
	set(PLATFORM Windows)
	set(TOOLS
		hop-editor.exe
		package-tool.exe
		bin-to-cpp.exe
	)
	set(LIBS
		hop-engine.lib
		hop-engine.dll
	)
	set(TAR_OUTPUT
		"${CMAKE_SOURCE_DIR}/builds/hop-engine ${GIT_TAG} ${PLATFORM}.zip"
	)
else()
	set(PLATFORM Linux)
	set(TOOLS
		hop-editor
		package-tool
		bin-to-cpp
	)
	set(LIBS
		libhop-engine.so
	)
	set(TAR_OUTPUT
		"${CMAKE_SOURCE_DIR}/builds/hop-engine ${GIT_TAG} ${PLATFORM}.zip"
	)
endif()

add_custom_target(export
	DEPENDS hop-engine
	DEPENDS package-tool
	DEPENDS hop-editor
	DEPENDS bin-to-cpp
	COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/builds"
	COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/staging"
	COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/staging/tools"
	COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/staging/lib"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/inc/"
		"${CMAKE_BINARY_DIR}/staging/include"
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_SOURCE_DIR}/lib/inc/glm-1.0.2/glm"
		"${CMAKE_BINARY_DIR}/staging/include/glm"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/template"
		"${CMAKE_BINARY_DIR}/staging/template"
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/COPYING.md"
		"${CMAKE_BINARY_DIR}/staging/COPYING.md"
	COMMAND ${CMAKE_COMMAND} -E copy ${TOOLS} "${CMAKE_BINARY_DIR}/staging/tools"
	COMMAND ${CMAKE_COMMAND} -E copy ${LIBS} "${CMAKE_BINARY_DIR}/staging/lib"
	COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_BINARY_DIR}/staging"
		${CMAKE_COMMAND} -E tar cfv ${TAR_OUTPUT} --format=zip .
	COMMENT "Creating exported archive"
)