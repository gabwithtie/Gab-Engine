set(CURRENT_CMAKE_LIB bgfx-gab)

#SUBDIRECTORY LOADING
add_library(${CURRENT_CMAKE_LIB} STATIC
	"bgfx-gab/forwardrenderer.cpp" "bgfx-gab/ScreenUtil.cpp")

target_link_libraries(${CURRENT_CMAKE_LIB} PRIVATE gbe_math)
target_link_libraries(${CURRENT_CMAKE_LIB} PRIVATE gbe_editor)
target_link_libraries(${CURRENT_CMAKE_LIB} PRIVATE gbe_graphics)
target_link_libraries(${CURRENT_CMAKE_LIB} PRIVATE gbe_asset)

find_package(assimp CONFIG REQUIRED)
target_link_libraries(${CURRENT_CMAKE_LIB} PRIVATE assimp::assimp)

target_link_libraries(${CURRENT_CMAKE_LIB} PUBLIC bgfx bimg bx bgfx-imgui bgfx-utils)

target_include_directories(${CURRENT_CMAKE_LIB} INTERFACE ${CMAKE_CURRENT_LIST_DIR})

message("[GABENGINE] LOADED: " ${CMAKE_CURRENT_SOURCE_DIR})