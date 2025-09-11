set(LIB_NAME "gbe_ext_anitobuilder")
message("[GBE EXTERNAL] Building Library "${LIB_NAME})


add_library(${LIB_NAME} "AnitoBuilder.cpp" )

message("[GBE EXTERNAL] Including directory: " ${GABENGINE_INCLUDE_DIRECTORY})


target_link_libraries(${LIB_NAME} gbe_math)
target_include_directories(${LIB_NAME} PRIVATE ${GABENGINE_INCLUDE_DIRECTORY})
