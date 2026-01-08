# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.22)
set(CMAKE_CXX_STANDARD 23)
# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

#INCLUDES
include(FetchContent)

#PROJECT DECLARATION
project ("gab-engine" LANGUAGES C CXX)
add_subdirectory(src)
add_executable(${PROJECT_NAME} "src/gab-engine.h" "src/gab-engine.cpp")
target_link_libraries(${PROJECT_NAME} PRIVATE gbe_engine_src)

#PROJECT DIRECTORIES FOR FILES/TOOLS
message("[GABENGINE] Project install prefix at: " "${CMAKE_INSTALL_PREFIX}")
message("[GABENGINE] Including VCPKG at: " "${CMAKE_TOOLCHAIN_FILE}")
set(ENGINE_TOOL_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/${ENGINE_ARCHITECTURE}-${ENGINE_PLATFORM}/tools")
message("[GABENGINE] Tool directory at:" ${ENGINE_TOOL_DIR})

#======================Dependency Loading======================#

#WINDOW CORE LIBRARY
find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)
find_package(SDL2_mixer CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>)

#RENDERER CORE LIBRARY
FetchContent_Declare(
  "bgfx.cmake"
  GIT_REPOSITORY "https://github.com/bkaradzic/bgfx.cmake.git"
  GIT_TAG        "v1.135.9062-502"
  SOURCE_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/Ext/bgfx.cmake"
)
FetchContent_MakeAvailable(bgfx.cmake)

add_subdirectory("Ext/bgfx-imgui")
add_subdirectory("Ext/bgfx-utils")
add_subdirectory("Ext/bgfx-gab")

#MikkTSpace
FetchContent_Declare(
  "mikktspace"
  GIT_REPOSITORY "https://github.com/mmikk/MikkTSpace.git"
  GIT_TAG        "master"
  SOURCE_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/Ext/mikktspace"
)
FetchContent_MakeAvailable(mikktspace)

include("Ext/MikkTSpace.cmake")

#RELOAD BASE ASSETS TO BUILD
add_custom_target(
    COPY_ASSETS_TARGET
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/DefaultAssets" "${CMAKE_BINARY_DIR}/DefaultAssets"
    COMMENT "[GABENGINE] Copying DefaultAssets to build directory..."
)

#==================SHADER COMPILATION==================
include("Ext/ShaderCompiler.cmake")
add_dependencies(COMPILED_SHADERS_TARGET COPY_ASSETS_TARGET)
add_dependencies(${PROJECT_NAME} COMPILED_SHADERS_TARGET)

#COPY DLL TO BUILD
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:${PROJECT_NAME}> $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
  COMMAND_EXPAND_LISTS
  COMMENT "[GABENGINE] Copying DLLs to build directory..."
)

get_target_property(LINKED_LIBS ${PROJECT_NAME} LINK_LIBRARIES)
# Construct the compiler string for the include directories.
    foreach(lib ${LINKED_LIBS})
        string(APPEND LINKED_LIBS_STRING " | ${lib} ")
    endforeach()
message("LINKED LIBRARIES: " ${LINKED_LIBS_STRING})
get_target_property(INCLUDED_DIRS ${PROJECT_NAME} INCLUDE_DIRECTORIES)
    # Construct the compiler string for the include directories.
    foreach(dir ${INCLUDED_DIRS})
        string(APPEND INCLUDED_DIRS_STRING " | ${dir} ")
    endforeach()
message("INCLUDED DIRECTORIES: " ${INCLUDED_DIRS_STRING})