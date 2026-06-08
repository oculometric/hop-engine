project(space-hop-demo)

set(HOP_INCLUDE "${CMAKE_SOURCE_DIR}/inc")
if(WIN32)
    set(HOP_LIB_FILE "hop-engine.dll")
	set(HOP_PACKAGE_TOOL "${CMAKE_BINARY_DIR}/package-tool.exe")
	set(HOP_LINK "${CMAKE_BINARY_DIR}/hop-engine.lib")
else()
	set(HOP_PACKAGE_TOOL "${CMAKE_BINARY_DIR}/package-tool")
    set(HOP_LIB_FILE "libhop-engine.so")
	set(HOP_LINK "${CMAKE_BINARY_DIR}/${HOP_LIB_FILE}")
endif()
set(HOP_LIB "${CMAKE_BINARY_DIR}/${HOP_LIB_FILE}")

file(GLOB HOP_SRC_FILES
	"src/*.cpp"
	"src/*/*.cpp"
)

add_executable(space-hop-demo ${HOP_SRC_FILES})

target_include_directories(space-hop-demo
	PRIVATE ${HOP_INCLUDE}
    "${CMAKE_SOURCE_DIR}/lib/inc/glm-1.0.2"
)
target_link_libraries(space-hop-demo
	PRIVATE ${HOP_LINK}
)

# ensure C++20 is in use
set_property(TARGET space-hop-demo PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET space-hop-demo PROPERTY CXX_STANDARD 20)

if(WIN32)
else()
	target_compile_options(space-hop-demo
		PRIVATE
			-fno-delete-null-pointer-checks
			-Wno-tautological-undefined-compare
	)
endif()

add_custom_target(space-hop-demo_resources
    DEPENDS package-tool
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	COMMAND ${HOP_PACKAGE_TOOL} res -c -b
		"${CMAKE_CURRENT_BINARY_DIR}/resources.hop"
	COMMENT "Compressing resources"
)

add_custom_target(space-hop-demo_libs
    DEPENDS hop-engine
	COMMAND ${CMAKE_COMMAND} -E copy ${HOP_LIB}
		"${CMAKE_CURRENT_BINARY_DIR}/${HOP_LIB_FILE}"
	COMMENT "Copying dependency"
)

add_dependencies(space-hop-demo space-hop-demo_resources)
add_dependencies(space-hop-demo space-hop-demo_libs)

set(DEMO_FILES ${HOP_LIB} "${CMAKE_CURRENT_BINARY_DIR}/resources.hop" $<TARGET_FILE:space-hop-demo> PARENT_SCOPE)
