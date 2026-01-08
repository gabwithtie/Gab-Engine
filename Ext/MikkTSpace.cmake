cmake_minimum_required(VERSION 3.19)

add_library(mikktspace "Ext/mikktspace/mikktspace.c" "Ext/mikktspace/mikktspace.h")
target_include_directories(mikktspace PUBLIC "${CMAKE_SOURCE_DIR}/Ext/MikkTSpace")