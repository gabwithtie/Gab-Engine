set(LIB_NAME "bgfx-imgui")
message("[GBE EXTERNAL] Building Library "${LIB_NAME})

add_library(${LIB_NAME}
	"bgfx-imgui/imgui_impl_bgfx.cpp"
)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(${LIB_NAME} PRIVATE imgui::imgui)

target_link_libraries(${LIB_NAME} PUBLIC bgfx bimg bx)

target_include_directories(${LIB_NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR})
