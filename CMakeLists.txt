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

#DEBUGGING PROJECT FILES
message("[GABENGINE] Project install prefix at: " "${CMAKE_INSTALL_PREFIX}")
message("[GABENGINE] Including VCPKG at: " "${CMAKE_TOOLCHAIN_FILE}")

#RELOAD BASE ASSETS TO BUILD

add_custom_target(RELOAD_ASSETS
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/DefaultAssets" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/DefaultAssets"
)

#==================SHADER COMPILATION==================
#reconfiguration dependency
file(GLOB CONFIGURE_DEPENDS VERT_PATHS "${CMAKE_CURRENT_LIST_DIR}/DefaultAssets/shaders/*.vert")
file(GLOB CONFIGURE_DEPENDS FRAG_PATHS "${CMAKE_CURRENT_LIST_DIR}/DefaultAssets/shaders/*.frag")

set(ENGINE_TOOL_DIR "${CMAKE_CURRENT_LIST_DIR}/vcpkg_installed/${ENGINE_ARCHITECTURE}-${ENGINE_PLATFORM}/tools")
message("[GABENGINE] Tool directory at:" ${ENGINE_TOOL_DIR})

set(COMPILED_SHADER_FILES "")
function(COMPILE_SHADER SRCNAME)
  set(SRCPATH "$<TARGET_FILE_DIR:${PROJECT_NAME}>/DefaultAssets/shaders/${SRCNAME}")
  set(TARGET_NAME "${SRCNAME}_COMPILATION")
  add_custom_target(${TARGET_NAME}
      COMMAND "${ENGINE_TOOL_DIR}/shaderc/glslc.exe" "${SRCPATH}" -g -o "${SRCPATH}.spv"
  )
  list(APPEND COMPILED_SHADER_FILES ${TARGET_NAME})
  set(COMPILED_SHADER_FILES "${COMPILED_SHADER_FILES}" PARENT_SCOPE)
endfunction()

set(SHADER_META_FILES "")
function(DECOMPILE_SHADER SRCNAME)
  set(SRCPATH "$<TARGET_FILE_DIR:${PROJECT_NAME}>/DefaultAssets/shaders/${SRCNAME}")
  set(TARGET_NAME "${SRCNAME}_DECOMPILATION")
  add_custom_target(${TARGET_NAME}
    COMMAND "${ENGINE_TOOL_DIR}/spirv-cross/spirv-cross.exe" --output "${SRCPATH}.json" "${SRCPATH}.spv" --reflect
  )
  list(APPEND SHADER_META_FILES ${TARGET_NAME})
  set(SHADER_META_FILES "${SHADER_META_FILES}" PARENT_SCOPE)
endfunction()

file(GLOB VERT_PATHS "${CMAKE_CURRENT_LIST_DIR}/DefaultAssets/shaders/*.vert")
file(GLOB FRAG_PATHS "${CMAKE_CURRENT_LIST_DIR}/DefaultAssets/shaders/*.frag")

set(SHADER_FILEPATHS ${VERT_PATHS})
list(APPEND SHADER_FILEPATHS ${FRAG_PATHS})

foreach(SHADER_FILEPATH ${SHADER_FILEPATHS})
    get_filename_component(SHADER_FILENAME "${SHADER_FILEPATH}" NAME)
    COMPILE_SHADER(${SHADER_FILENAME})
    DECOMPILE_SHADER(${SHADER_FILENAME})
endforeach()

add_custom_target(COMPILE_SHADERS ALL DEPENDS ${COMPILED_SHADER_FILES})
add_custom_target(GENERATE_SHADERMETAS ALL DEPENDS ${SHADER_META_FILES})

#COMMAND SEQUENCING
add_dependencies(COMPILE_SHADERS RELOAD_ASSETS)
add_dependencies(GENERATE_SHADERMETAS COMPILE_SHADERS)
add_dependencies(${PROJECT_NAME} GENERATE_SHADERMETAS)



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