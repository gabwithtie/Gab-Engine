set(LIB_NAME "gbe_anitobuilder")
message("[GBE EXTERNAL] Building Library "${LIB_NAME})

add_library(${LIB_NAME} "BuilderBlock.cpp" "BuilderBlockSet.cpp" "gui/AnitoBuilderWindow.cpp" "AnitoBuilderExtension.h" "AnitoBuilderExtension.cpp")

message("[GBE EXTERNAL] Including directory: "${GABENGINE_INCLUDE_DIRECTORY})

target_link_libraries(${LIB_NAME} gbe_math)
target_link_libraries(${LIB_NAME} gbe_engine)
target_link_libraries(${LIB_NAME} gbe_editor)
target_link_libraries(${LIB_NAME} gbe_graphics)
target_link_libraries(${LIB_NAME} gbe_physics)
target_link_libraries(${LIB_NAME} gbe_extension)
target_include_directories(${LIB_NAME} PUBLIC ${GABENGINE_INCLUDE_DIRECTORY})
target_include_directories(${LIB_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
