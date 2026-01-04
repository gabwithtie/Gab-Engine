cmake_minimum_required(VERSION 3.19)

# --- Configuration ---
# Update these paths to match your bgfx installation

# --- Function: Compile a single bgfx shader ---
function(add_bgfx_shader_task INPUT TYPE VARYING OUTPUT)
    get_filename_component(OUT_DIR ${OUTPUT} DIRECTORY)
    file(MAKE_DIRECTORY ${OUT_DIR})

    add_custom_command(
        OUTPUT ${OUTPUT}
        COMMAND shaderc
            -f ${INPUT}
            -o ${OUTPUT}
            --type ${TYPE}
            --platform windows
            -p "spirv"
            --varyingdef ${VARYING}
            -i "${BGFX_DIR}/src"
            -i "${BGFX_DIR}/examples/common"
            -i "${OUT_DIR}"
        DEPENDS ${INPUT} ${VARYING}
        COMMENT "Compiling bgfx shader (spirv): ${INPUT}"
        VERBATIM
    )
endfunction()

# --- Recursive JSON Parser ---
set(COMPILED_SHADER_BINARIES "")

# 1. Find all shaders.json files recursively
file(GLOB_RECURSE JSON_MANIFESTS "${CMAKE_SOURCE_DIR}/DefaultAssets/**/shadermanifest.json")
set(JSON_MANIFESTS_RELPATHS "")

foreach(MANIFEST ${JSON_MANIFESTS})
    get_filename_component(SOURCE_MANIFEST_DIR ${MANIFEST} DIRECTORY)

    CMAKE_PATH(RELATIVE_PATH MANIFEST BASE_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE REL_PATH)
    set(BINARY_MANIFEST "${CMAKE_BINARY_DIR}/${REL_PATH}")
    get_filename_component(BINARY_MANIFEST_DIR ${BINARY_MANIFEST} DIRECTORY)

    message("[GABENGINE] Exporting manifest to: " ${BINARY_MANIFEST_DIR})
    file(READ ${MANIFEST} JSON_CONTENT)

    # 2. Get the number of shaders in the list
    string(JSON SHADER_COUNT LENGTH "${JSON_CONTENT}" "shaders")
    math(EXPR SHADER_COUNT "${SHADER_COUNT} - 1")

    # 3. Loop through each shader entry in the JSON
    foreach(I RANGE ${SHADER_COUNT})
        string(JSON S_FILE GET "${JSON_CONTENT}" "shaders" ${I} "file")
        string(JSON S_TYPE GET "${JSON_CONTENT}" "shaders" ${I} "type")
        string(JSON S_VARY GET "${JSON_CONTENT}" "shaders" ${I} "varying")
        set(S_OUT "${S_FILE}.bin")

        # Resolve paths relative to where the JSON file is located
        set(ABS_IN "${SOURCE_MANIFEST_DIR}/${S_FILE}")
        set(ABS_VARY "${SOURCE_MANIFEST_DIR}/${S_VARY}")
        set(ABS_OUT "${BINARY_MANIFEST_DIR}/${S_OUT}") # Output to build folder

        # 4. Create the build task
        add_bgfx_shader_task(${ABS_IN} ${S_TYPE} ${ABS_VARY} ${ABS_OUT})
        list(APPEND COMPILED_SHADER_BINARIES ${ABS_OUT})
    endforeach()
endforeach()

# --- Create a Build Target ---
# This ensures that 'make' or 'Build Solution' triggers the compilation
add_custom_target(COMPILED_SHADERS_TARGET ALL DEPENDS ${COMPILED_SHADER_BINARIES})

# Optional: Ensure your main executable waits for shaders to finish compiling
# add_dependencies(MyEngineExecutable CompileShaders)