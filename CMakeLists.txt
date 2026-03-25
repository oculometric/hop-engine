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
	add_compile_options("/Od")
	add_compile_options("/ZI")
	add_link_options("/INCREMENTAL")
endif()
	add_link_options("/DEBUG")
endif()
find_package(Vulkan REQUIRED)
set(ENGINE_HOP "${CMAKE_BINARY_DIR}/engine.hop")

# ================ EXECUTABLE TARGET 'package-tool' =======================================
add_executable(package-tool
    "src/package-builder/package-builder.cpp"
    "src/file/package.cpp"
    "src/rendering/binmesh_loading.cpp"
    "src/rendering/obj_loading.cpp"
    "src/exec.cpp"
    "src/debug/debug.cpp"
)

target_include_directories(package-tool PRIVATE
    "lib/inc"
    "lib/inc/glm-1.0.2"
    "inc"
)
set_property(TARGET package-tool PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET package-tool PROPERTY CXX_STANDARD 20)

if(WIN32)
else()
	target_compile_options(package-tool PRIVATE
        -fno-delete-null-pointer-checks
        -Wno-tautological-undefined-compare
    )
endif()

# ================ RESOURCE TARGET 'engine.hop' ===========================================
add_custom_command(OUTPUT ${ENGINE_HOP}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND package-tool res_engine -c -b -p "engine/" ${ENGINE_HOP}
    DEPENDS package-tool
)

# ================ STATIC LIBRARY TARGET 'hop-engine' =====================================
file(GLOB SRC_FILES
	"src/*.cpp"
	"src/*/*.cpp"
	"lib/src/*.cpp"
)
list(FILTER SRC_FILES EXCLUDE REGEX
    "src/package-builder/package-builder.cpp"
)
list(FILTER SRC_FILES EXCLUDE REGEX
    ".*main.cpp"
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
    ${Vulkan_LIBRARIES}
)

if(WIN32)
    list(APPEND LIBS
        "${CMAKE_SOURCE_DIR}/lib/bin/GLFW/lib-vc2019/glfw3.lib"
        "${Vulkan_INCLUDE_DIRS}/../Lib/shaderc_combined.lib"
    )
else()
    list(APPEND INC_DIRS
        "${CMAKE_SOURCE_DIR}/lib/inc/shaderc"
    )
    list(APPEND LIBS
        "${CMAKE_SOURCE_DIR}/lib/bin/libshaderc/libshaderc_combined.a"
        pthread
        glfw
        dl
        X11
        Xrandr
        Xi
    )
endif()

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/engine_resource_binary.cpp
    COMMAND python ${CMAKE_SOURCE_DIR}/bin_to_cpp.py ${ENGINE_HOP} "${CMAKE_BINARY_DIR}/engine_resource_binary.cpp" engine_hop_raw
    DEPENDS ${ENGINE_HOP} ${CMAKE_SOURCE_DIR}/bin_to_cpp.py
)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(hop-engine SHARED ${SRC_FILES} ${CMAKE_BINARY_DIR}/engine_resource_binary.cpp)

target_include_directories(hop-engine PRIVATE ${INC_DIRS})
target_link_libraries(hop-engine PRIVATE ${LIBS})
set_property(TARGET hop-engine PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET hop-engine PROPERTY CXX_STANDARD 20)

if(WIN32)
else()
    target_compile_options(hop-engine PRIVATE
        -fno-delete-null-pointer-checks
        -Wno-tautological-undefined-compare
    )
endif()

# ================ EXECUTABLE TARGET 'hop-editor' =========================================
add_executable(hop-editor "src/main.cpp")

target_link_libraries(hop-editor hop-engine)

set_property(TARGET hop-editor PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET hop-editor PROPERTY CXX_STANDARD 20)
if(WIN32)
else()
	target_compile_options(hop-editor PRIVATE
        -fno-delete-null-pointer-checks
        -Wno-tautological-undefined-compare)
endif()

target_include_directories(hop-editor PRIVATE
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
    set(EXPORTS hop-editor.exe hop-engine.lib hop-engine.dll hop-engine.exp package-tool.exe)
	set(TAR_OPTIONS -caf)
	set(TAR_OUTPUT "${CMAKE_SOURCE_DIR}/builds/hop-engine ${PLATFORM} ${TODAY}.zip")
else()
	set(PLATFORM Linux)
	set(TAR_OPTIONS -cf)
	set(TAR_OUTPUT "${CMAKE_SOURCE_DIR}/builds/hop-engine ${PLATFORM} ${TODAY}.tar")
endif()

add_custom_target(OUTPUT ${EXPORTED_INCLUDE}
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${CMAKE_SOURCE_DIR}/inc/" ${EXPORTED_INCLUDE}
    COMMENT "Copying include directory"
)

add_custom_target(export
    DEPENDS ${EXPORTED_INCLUDE}
    DEPENDS hop-engine
    DEPENDS package-tool
    DEPENDS hop-editor
	COMMAND ${ZIP_COMMAND} ${TAR_OPTIONS} ${TAR_OUTPUT} ${EXPORTS} "include"
)