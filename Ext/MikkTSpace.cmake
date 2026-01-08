cmake_minimum_required(VERSION 3.19)

add_library(mikktspace "Ext/MikkTSpace/mikktspace.c" "Ext/MikkTSpace/mikktspace.h")
target_include_directories(mikktspace PUBLIC "${CMAKE_SOURCE_DIR}/Ext/MikkTSpace")