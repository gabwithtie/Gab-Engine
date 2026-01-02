set(LIB_NAME "ext_bgfx_imgui")
message("[GBE EXTERNAL] Building Library "${LIB_NAME})

add_library(${LIB_NAME}
	"imgui_impl_bgfx.cpp"
)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(${LIB_NAME} PRIVATE imgui::imgui)

target_link_libraries(${LIB_NAME} PUBLIC bgfx bimg bx)

target_include_directories(${LIB_NAME} PRIVATE ${GABENGINE_INCLUDE_DIRECTORY})
