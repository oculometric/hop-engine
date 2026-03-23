cmake_minimum_required(VERSION 3.12)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")

project(hop-engine)

if(WIN32)
if(HOP_BUILD_DEBUG)
	add_compile_options("/Od")
	add_compile_options("/ZI")
	add_link_options("/INCREMENTAL")
endif()
	add_link_options("/DEBUG")
endif()

find_package(Vulkan REQUIRED COMPONENTS glslang SPIRV-Tools)

file(GLOB SRC_FILES
	"src/*.cpp"
	"src/*/*.cpp"
	"lib/src/*.cpp"
)
list(FILTER SRC_FILES EXCLUDE REGEX ".*/package-builder.cpp")

add_library(hop-engine STATIC ${SRC_FILES})

target_include_directories(hop-engine
	PUBLIC
		"lib/inc"
		"lib/inc/glm-1.0.2"
		"lib/inc/imgui"
		"lib/inc/imgui/backends"
		"inc"
)
target_include_directories(hop-engine PUBLIC ${Vulkan_INCLUDE_DIRS})

set_property(TARGET hop-engine PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET hop-engine PROPERTY CXX_STANDARD 20)

if(WIN32)
	target_link_libraries(hop-engine
		PRIVATE "${CMAKE_SOURCE_DIR}/lib/bin/GLFW/lib-vc2019/glfw3.lib"
	)
	target_link_libraries(hop-engine PRIVATE
        ${Vulkan_LIBRARIES}
        ${Vulkan_INCLUDE_DIR}/../lib/glslang-default-resource-limits.lib
        ${Vulkan_INCLUDE_DIR}/../lib/SPIRV-Tools-opt.lib
        Vulkan::glslang
        Vulkan::SPIRV-Tools)
else()
	target_include_directories(hop-engine
		PRIVATE ${CMAKE_SOURCE_DIR}/lib/inc/shaderc
	)
	target_link_libraries(hop-engine
		${CMAKE_SOURCE_DIR}/lib/bin/libshaderc/libshaderc_combined.a
	)
	target_link_libraries(hop-engine pthread)
	target_link_libraries(hop-engine glfw)
	target_link_libraries(hop-engine vulkan)
	target_link_libraries(hop-engine dl)
	target_link_libraries(hop-engine X11)
	target_link_libraries(hop-engine Xrandr)
	target_link_libraries(hop-engine Xi)
	target_compile_options(hop-engine PRIVATE -fno-delete-null-pointer-checks)
	target_compile_options(hop-engine PRIVATE -Wno-tautological-undefined-compare)
endif()

project(package-tool)

add_executable(package-tool "src/package-builder/package-builder.cpp")

target_compile_options(package-tool PUBLIC -DSTANDALONE)

target_include_directories(package-tool
	PUBLIC
		"lib/inc"
		"lib/inc/glm-1.0.2"
		"inc"
)

target_link_libraries(package-tool hop-engine)

if(WIN32)
else()
	target_compile_options(package-tool PRIVATE -fno-delete-null-pointer-checks)
	target_compile_options(package-tool
		PRIVATE -Wno-tautological-undefined-compare
	)
endif()

set_property(TARGET package-tool PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET package-tool PROPERTY CXX_STANDARD 20)

add_custom_target(engine-resources
	COMMAND package-tool res_engine -c -b -p "engine/"
		"${CMAKE_BINARY_DIR}/engine.hop"
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	DEPENDS package-tool
)

project(demo)

add_executable(demo "src/main.cpp")

target_include_directories(demo
	PUBLIC
		"lib/inc"
		"lib/inc/glm-1.0.2"
		"lib/inc/imgui"
		"lib/inc/imgui/backends"
		"inc"
)

target_link_libraries(demo hop-engine)

if(WIN32)
else()
	target_compile_options(demo PRIVATE -fno-delete-null-pointer-checks)
	target_compile_options(demo PRIVATE -Wno-tautological-undefined-compare)
endif()

add_custom_target(demo-resources
	COMMAND package-tool res -c -b "${CMAKE_BINARY_DIR}/resources.hop"
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	DEPENDS package-tool
)

add_dependencies(demo demo-resources)
add_dependencies(demo engine-resources)

set_property(TARGET demo PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET demo PROPERTY CXX_STANDARD 20)

project(exporting)

message("Generating exported zip...")
string(TIMESTAMP TODAY "%d_%m_%Y %H.%M.%S")
set(ENGINE_PACKAGE_OUTPUT ${CMAKE_BINARY_DIR}/engine.hop)
set(DEMO_PACKAGE_OUTPUT ${CMAKE_BINARY_DIR}/resources.hop)
set(ZIP_COMMAND "tar")
if(WIN32)
	set(PLATFORM Windows)
	set(DEMO_EXE_PATH ${CMAKE_BINARY_DIR}/demo.exe)
    set(ENGINE_LIB_PATH ${CMAKE_BINARY_DIR}/hop-engine.lib)
    set(PACKAGE_EXE_PATH ${CMAKE_BINARY_DIR}/package-tool.exe)
	set(TAR_OPTIONS -caf)
	set(TAR_OUTPUT "${CMAKE_SOURCE_DIR}/hop-engine ${PLATFORM} ${TODAY}.zip")
    set(COPY_COMMAND robocopy /E ${CMAKE_SOURCE_DIR}/inc ${CMAKE_BINARY_DIR}/include)
else()
	set(PLATFORM Linux)
	set(DEMO_EXE_PATH ${CMAKE_BINARY_DIR}/demo)
    set(ENGINE_LIB_PATH ${CMAKE_BINARY_DIR}/libhop-engine.a)
    set(PACKAGE_EXE_PATH ${CMAKE_BINARY_DIR}/package-tool)
	set(TAR_OPTIONS -cf)
	set(TAR_OUTPUT "${CMAKE_SOURCE_DIR}/hop-engine ${PLATFORM} ${TODAY}.tar")
    set(COPY_COMMAND "cp -r ${CMAKE_SOURCE_DIR}/inc/* ${CMAKE_BINARY_DIR}/include")
endif()
cmake_path(GET ENGINE_PACKAGE_OUTPUT FILENAME ENGINE_HOP)
cmake_path(GET DEMO_PACKAGE_OUTPUT FILENAME RESOURCES_HOP)
cmake_path(GET DEMO_EXE_PATH FILENAME DEMO_EXE)
cmake_path(GET ENGINE_LIB_PATH FILENAME ENGINE_LIB)
cmake_path(GET PACKAGE_EXE_PATH FILENAME PACKAGE_EXE)
set(ARCHIVE_TARGETS ${ENGINE_HOP} ${RESOURCES_HOP} ${DEMO_EXE} ${ENGINE_LIB} "include" ${PACKAGE_EXE})

add_custom_target(exporting
    COMMAND ${COPY_COMMAND}
	COMMAND ${ZIP_COMMAND} ${TAR_OPTIONS} ${TAR_OUTPUT} ${ARCHIVE_TARGETS}
)

add_dependencies(exporting demo)
add_dependencies(exporting hop-engine)
add_dependencies(exporting engine-resources)
