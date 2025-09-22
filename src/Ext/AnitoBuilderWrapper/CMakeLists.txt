set(LIB_NAME "gbe_ext_anitobuilderwrapper")
message("[GBE EXTERNAL] Building Library "${LIB_NAME})


add_library(${LIB_NAME} "AnitoBuilderWrapper.cpp"  "BuilderBlock.cpp" "BuilderBlockSet.h" "BuilderBlockSet.cpp")

message("[GBE EXTERNAL] Including directory: "${GABENGINE_INCLUDE_DIRECTORY})


target_link_libraries(${LIB_NAME} gbe_ext_anitobuilder)
target_link_libraries(${LIB_NAME} gbe_math)
target_link_libraries(${LIB_NAME} gbe_engine)
target_link_libraries(${LIB_NAME} gbe_editor)
target_link_libraries(${LIB_NAME} gbe_graphics)
target_include_directories(${LIB_NAME} PRIVATE ${GABENGINE_INCLUDE_DIRECTORY})
