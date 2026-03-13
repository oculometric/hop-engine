cmake_minimum_required (VERSION 3.12)

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")

project (hop-engine)

if (HOP_BUILD_DEBUG)
if (WIN32)
    add_compile_options("/Od")
    add_compile_options("/ZI")
    add_link_options("/INCREMENTAL")
    add_link_options("/DEBUG")
endif()
endif()

find_package(Vulkan REQUIRED)

file(GLOB SRC_FILES
	"src/*.cpp"
	"src/*/*.cpp"
    "lib/src/*.cpp"
)

add_library(hop-engine STATIC ${SRC_FILES})

target_include_directories(hop-engine PUBLIC
    "lib/inc"
    "lib/inc/glm-1.0.2"
    "lib/inc/imgui"
    "lib/inc/imgui/backends"
    "inc"
)
target_include_directories(hop-engine PUBLIC ${Vulkan_INCLUDE_DIRS})

set_property(TARGET hop-engine PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET hop-engine PROPERTY CXX_STANDARD 20)

IF (WIN32)
	target_link_libraries(hop-engine PRIVATE "${CMAKE_SOURCE_DIR}/lib/bin/GLFW/lib-vc2019/glfw3.lib")
	target_link_libraries(hop-engine PRIVATE ${Vulkan_LIBRARIES})
	target_link_libraries(hop-engine PRIVATE ${Vulkan_INCLUDE_DIRS}/../Lib/shaderc_combined.lib)
ELSE()
	target_link_libraries(hop-engine pthread)
	target_link_libraries(hop-engine glfw)
	target_link_libraries(hop-engine vulkan)
	target_link_libraries(hop-engine dl)
	target_link_libraries(hop-engine X11)
	target_link_libraries(hop-engine Xrandr)
	target_link_libraries(hop-engine Xi)
ENDIF()




project (package-tool)

add_executable(package-tool "src/package-builder/package-builder.cpp")

target_compile_options(package-tool PUBLIC -DSTANDALONE)

target_include_directories(package-tool PUBLIC
    "lib/inc"
    "lib/inc/glm-1.0.2"
    "inc"
)

target_link_libraries(package-tool hop-engine)

set_property(TARGET package-tool PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET package-tool PROPERTY CXX_STANDARD 20)

add_custom_target(engine-resources
	COMMAND package-tool res_engine -c -b -p "engine/" "${CMAKE_BINARY_DIR}/engine.hop"
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	DEPENDS package-tool
)


project(demo)

add_executable(demo "src/main.cpp")

target_include_directories(demo PUBLIC
    "lib/inc"
    "lib/inc/glm-1.0.2"
    "lib/inc/imgui"
    "lib/inc/imgui/backends"
    "inc"
)

target_link_libraries(demo hop-engine)

add_custom_target(demo-resources
	COMMAND package-tool res -c -b "${CMAKE_BINARY_DIR}/resources.hop"
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	DEPENDS package-tool
)

add_dependencies(demo demo-resources)
add_dependencies(demo engine-resources)

set_property(TARGET demo PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET demo PROPERTY CXX_STANDARD 20)