set(LIB_NAME "gbe_ext_gdparcm")
message("[GBE EXTERNAL] Building Library "${LIB_NAME})

add_library(${LIB_NAME}
	"MeshAsync.cpp"	
)

target_link_libraries(${LIB_NAME} gbe_math)
target_link_libraries(${LIB_NAME} gbe_engine)
target_link_libraries(${LIB_NAME} gbe_editor)
target_link_libraries(${LIB_NAME} gbe_graphics)

find_package(sockpp CONFIG REQUIRED)
target_link_libraries(${LIB_NAME} Sockpp::sockpp-static)

target_include_directories(${LIB_NAME} PRIVATE ${GABENGINE_INCLUDE_DIRECTORY})
