cmake_minimum_required(VERSION 3.12)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")

# ================ PROJECT 'hop-engine' ===================================================
project(hop-engine)

if(WIN32)
	if(HOP_BUILD_DEBUG)
		# add_compile_options("/Od")
		string(REPLACE "/O2" "/Od" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
		add_link_options("/INCREMENTAL")
	endif()
	add_compile_options("/ZI")
	add_link_options("/DEBUG")
endif()
find_package(Vulkan REQUIRED COMPONENTS glslang SPIRV-Tools)
set(ENGINE_HOP "${CMAKE_BINARY_DIR}/engine.hop")

# ================ EXECUTABLE TARGET 'package-tool' =======================================
add_executable(package-tool
	"src/package-builder/package-builder.cpp"
	"src/core/package.cpp"
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
)
set(LIBS
	Vulkan::Vulkan
	Vulkan::glslang
	Vulkan::SPIRV-Tools
)

if(WIN32)
	list(APPEND LIBS
		"${CMAKE_SOURCE_DIR}/lib/bin/GLFW/lib-vc2019/glfw3.lib"
		"${Vulkan_INCLUDE_DIRS}/../Lib/glslang-default-resource-limits.lib"
		"${Vulkan_INCLUDE_DIRS}/../Lib/SPIRV-Tools-opt.lib"
	)
else()
	list(APPEND LIBS
		"${CMAKE_SOURCE_DIR}/lib/bin/libshaderc/glslang-default-resource-limits.a"
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
		#${CMAKE_SOURCE_DIR}/bin_to_cpp.py
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
	# target_compile_options("/W4")
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
message("Generating exported zip...")
string(TIMESTAMP TODAY "%d_%m_%Y %H.%M.%S")
set(ZIP_COMMAND "tar")
set(EXPORTED_INCLUDE "${CMAKE_BINARY_DIR}/include")
file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/builds")
if(WIN32)
	set(PLATFORM Windows)
	set(EXPORTS
		hop-editor.exe
		hop-engine.lib
		hop-engine.dll
		hop-engine.exp
		package-tool.exe
	)
	set(TAR_OPTIONS -caf)
	set(TAR_OUTPUT
		"${CMAKE_SOURCE_DIR}/builds/hop-engine ${PLATFORM} ${TODAY}.zip"
	)
else()
	set(PLATFORM Linux)
	set(TAR_OPTIONS -cf)
	set(TAR_OUTPUT
		"${CMAKE_SOURCE_DIR}/builds/hop-engine ${PLATFORM} ${TODAY}.tar"
	)
endif()

add_custom_target(
	OUTPUT
	${EXPORTED_INCLUDE}
	COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
		"${CMAKE_SOURCE_DIR}/inc/" ${EXPORTED_INCLUDE}
	COMMENT "Copying include directory"
)

add_custom_target(export
	DEPENDS ${EXPORTED_INCLUDE}
	DEPENDS hop-engine
	DEPENDS package-tool
	DEPENDS hop-editor
	COMMAND ${ZIP_COMMAND} ${TAR_OPTIONS} ${TAR_OUTPUT} ${EXPORTS} "include"
)