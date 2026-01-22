set(CURRENT_CMAKE_LIB gbe_editor)

#SUBDIRECTORY LOADING
add_library(${CURRENT_CMAKE_LIB} STATIC
	"Editor.cpp"

	"Gui/Windows/InspectorWindow.cpp"
	"Gui/Windows/CreditsWindow.cpp"
	"Gui/Windows/GuiWindow.cpp"
	"Gui/GuiElement.cpp"
	"Gui/MenuBar.cpp"
	"Gui/Windows/HierarchyWindow.cpp"
	"Gui/Windows/StateWindow.cpp"
	"Gui/Windows/SpawnWindow.cpp"
	"Gui/Windows/ConsoleWindow.cpp"
	"Gui/GizmoLayer.cpp"
	"Gui/ContextMenus.cpp"

	"Gui/Windows/ImageDebugger.cpp"
	"Gui/Windows/ViewportWindow.cpp"
	"Utility/CreateFunctions.cpp"
	"Gui/Windows/LightExplorer.cpp"

	"Utility/ModelExport.cpp"
 "Utility/ProjectLoader.h")

target_link_libraries(${CURRENT_CMAKE_LIB} PRIVATE gbe_math)
target_link_libraries(${CURRENT_CMAKE_LIB} PRIVATE gbe_graphics)
target_link_libraries(${CURRENT_CMAKE_LIB} PRIVATE gbe_window)
target_link_libraries(${CURRENT_CMAKE_LIB} PRIVATE gbe_engine)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(${CURRENT_CMAKE_LIB} PRIVATE imgui::imgui)

find_package(imguizmo CONFIG REQUIRED)
target_link_libraries(${CURRENT_CMAKE_LIB} PRIVATE imguizmo::imguizmo)

find_package(nfd CONFIG REQUIRED)
target_link_libraries(${CURRENT_CMAKE_LIB} PRIVATE nfd::nfd)

target_link_libraries(${CURRENT_CMAKE_LIB} PUBLIC bgfx bimg bx bgfx-imgui)

message("[GABENGINE] LOADED: " ${CMAKE_CURRENT_SOURCE_DIR})
