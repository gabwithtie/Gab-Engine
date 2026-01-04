set(CURRENT_CMAKE_LIB gbe_graphics)

#SUBDIRECTORY LOADING
add_library(${CURRENT_CMAKE_LIB} STATIC
	"RenderPipeline.cpp"
	"Data/DrawCall.cpp"
	
	
	
	"AssetLoaders/TextureLoader.cpp" "AssetLoaders/ShaderLoader.h"
	"AssetLoaders/ShaderLoader.cpp"
	"AssetLoaders/MeshLoader.cpp"
	"AssetLoaders/MaterialLoader.cpp"
	 "Data/CallInstance.h" "Data/Light.h" "Renderer.h")


find_package(Stb REQUIRED)
target_include_directories(${CURRENT_CMAKE_LIB} PRIVATE ${Stb_INCLUDE_DIR})

target_link_libraries(${CURRENT_CMAKE_LIB} PRIVATE gbe_math)
target_link_libraries(${CURRENT_CMAKE_LIB} PRIVATE gbe_editor)

find_package(assimp CONFIG REQUIRED)
target_link_libraries(${CURRENT_CMAKE_LIB} PRIVATE assimp::assimp)

target_link_libraries(${CURRENT_CMAKE_LIB} PUBLIC bgfx bimg bx bgfx-imgui bgfx-utils bgfx-gab)

message("[GABENGINE] LOADED: " ${CMAKE_CURRENT_SOURCE_DIR})