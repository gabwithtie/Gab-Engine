set(CURRENT_CMAKE_LIB gbe_graphics)

#SUBDIRECTORY LOADING
add_library(${CURRENT_CMAKE_LIB} STATIC
	"RenderPipeline.cpp"
	"Data/DrawCall.cpp"
	
	
	
	"AssetLoaders/TextureLoader.cpp" "AssetLoaders/ShaderLoader.h"
	"AssetLoaders/ShaderLoader.cpp"
	"AssetLoaders/MeshLoader.cpp"
	"AssetLoaders/MaterialLoader.cpp"
	 "Data/CallInstance.h" "Data/Light.h")


find_package(Stb REQUIRED)
target_include_directories(${CURRENT_CMAKE_LIB} PRIVATE ${Stb_INCLUDE_DIR})

target_link_libraries(${CURRENT_CMAKE_LIB} PRIVATE gbe_math)
target_link_libraries(${CURRENT_CMAKE_LIB} PRIVATE gbe_editor)
target_link_libraries(${CURRENT_CMAKE_LIB} PRIVATE gbe_ext_gabvulkan)

IF (WIN32)
	set(OpenCV_DIR "${VCPKG_INSTALLED_DIR}/x64-windows/share/opencv2")
	message("[GABENGINE WIN32] FOUND: ${OpenCV_DIR}")
ELSE()
# set stuff for other systems
ENDIF()

find_package(assimp CONFIG REQUIRED)
target_link_libraries(${CURRENT_CMAKE_LIB} PRIVATE assimp::assimp)

find_package(OpenCV REQUIRED)
target_link_libraries(${CURRENT_CMAKE_LIB} PRIVATE opencv_core opencv_highgui)

target_link_libraries(${CURRENT_CMAKE_LIB} PUBLIC bgfx bimg bx)

message("[GABENGINE] LOADED: " ${CMAKE_CURRENT_SOURCE_DIR})