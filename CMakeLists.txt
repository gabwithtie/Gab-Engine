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

project ("gab-engine" LANGUAGES CXX)
#SUBDIRECTORY LOADING
add_subdirectory(src)

# Add source to this project's executable.
add_executable(${PROJECT_NAME} "src/gab-engine.h" "src/gab-engine.cpp")
target_link_libraries(${PROJECT_NAME} PRIVATE gbe_engine_src)

#Dependency Loading
#WINDOW CORE LIBRARY
find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)
find_package(SDL2_mixer CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>)

#DEBUGGING PROJECT FILES/TOOLS
message("[GABENGINE] Project install prefix at: " "${CMAKE_INSTALL_PREFIX}")
message("[GABENGINE] Including VCPKG at: " "${CMAKE_TOOLCHAIN_FILE}")

set(ENGINE_TOOL_DIR "${CMAKE_CURRENT_LIST_DIR}/vcpkg_installed/${ENGINE_ARCHITECTURE}-${ENGINE_PLATFORM}/tools")
message("[GABENGINE] Tool directory at:" ${ENGINE_TOOL_DIR})

#RELOAD BASE ASSETS TO BUILD
add_custom_target(
    COPY_ASSETS_TARGET
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/DefaultAssets" "${CMAKE_BINARY_DIR}/DefaultAssets"
)

#==================SHADER COMPILATION==================
# Define a variable for the output directory
message("[GABENGINE] Shader output directory at: " ${CMAKE_BINARY_DIR})

#reconfiguration dependency
file(GLOB_RECURSE CONFIGURE_DEPENDS VERT_PATHS "${CMAKE_CURRENT_LIST_DIR}/DefaultAssets/shaders/*.vert")
file(GLOB_RECURSE CONFIGURE_DEPENDS FRAG_PATHS "${CMAKE_CURRENT_LIST_DIR}/DefaultAssets/shaders/*.frag")

#Collect Files
file(GLOB_RECURSE VERT_PATHS "${CMAKE_CURRENT_LIST_DIR}/DefaultAssets/shaders/*.vert")
file(GLOB_RECURSE FRAG_PATHS "${CMAKE_CURRENT_LIST_DIR}/DefaultAssets/shaders/*.frag")

set(SHADER_FILEPATHS ${VERT_PATHS})
list(APPEND SHADER_FILEPATHS ${FRAG_PATHS})

set(SHADER_FILEPATHS_REL "")
foreach(SHADER_FILEPATH ${SHADER_FILEPATHS})
    CMAKE_PATH(RELATIVE_PATH SHADER_FILEPATH BASE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}" OUTPUT_VARIABLE REL_PATH)
    list(APPEND SHADER_FILEPATHS_REL ${REL_PATH})
    message("[GABENGINE] Found shader: " ${REL_PATH})
endforeach()

set(SHADER_OUTPUTS "")
foreach(SRCRELPATH ${SHADER_FILEPATHS_REL})
  set(OGPATH "${CMAKE_CURRENT_LIST_DIR}/${SRCRELPATH}")
  set(DSTPATH "${CMAKE_BINARY_DIR}/${SRCRELPATH}")
  add_custom_command(
    OUTPUT "${DSTPATH}.spv"
    COMMAND "${ENGINE_TOOL_DIR}/shaderc/glslc.exe" "${OGPATH}" -g -o "${DSTPATH}.spv"
    MAIN_DEPENDENCY "${OGPATH}"
  )
  set(SRCPATH "${CMAKE_BINARY_DIR}/${SRCRELPATH}")
  add_custom_command(
    OUTPUT "${SRCPATH}.json"
    COMMAND "${ENGINE_TOOL_DIR}/spirv-cross/spirv-cross.exe" --output "${SRCPATH}.json" "${SRCPATH}.spv" --reflect
    MAIN_DEPENDENCY "${SRCPATH}.spv"
  )

  message("[GABENGINE] Prepared compilation of: ${TARGETNAME}.spv || ${TARGETNAME}.json")

  list(APPEND SHADER_OUTPUTS "${DSTPATH}.spv")
  list(APPEND SHADER_OUTPUTS "${SRCPATH}.json")
endforeach()

add_custom_target(COMPILESHADERSTARGET ALL
    DEPENDS ${SHADER_OUTPUTS}
    COMMENT "Compiling all shaders"
)

add_dependencies(COMPILESHADERSTARGET COPY_ASSETS_TARGET)
add_dependencies(${PROJECT_NAME} COMPILESHADERSTARGET)

#COPY DLL TO BUILD
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:${PROJECT_NAME}> $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
  COMMAND_EXPAND_LISTS
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