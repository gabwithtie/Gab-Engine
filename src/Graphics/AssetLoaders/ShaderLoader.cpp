#include "ShaderLoader.h"
// #include "Ext/GabVulkan/Utility/DebugObjectName.h" // Vulkan utility removed
#include <stdexcept>

namespace {
    // Utility to read a binary file (shader)
    std::vector<char> readfile(std::filesystem::path path) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open file: " + path.string());
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    };
}

gbe::gfx::ShaderData gbe::gfx::ShaderLoader::LoadAsset_(asset::Shader* asset, const asset::data::ShaderImportData& importdata, asset::data::ShaderLoadData* data) {
    //============READING SHADER BINARIES AND METADATA============//
    auto vertpath = asset->Get_asset_filepath().parent_path() / importdata.vert;
    auto fragpath = asset->Get_asset_filepath().parent_path() / importdata.frag;
    auto vertmetapath = asset->Get_asset_filepath().parent_path() / importdata.vert_meta;
    auto fragmetapath = asset->Get_asset_filepath().parent_path() / importdata.frag_meta;

    // Read the compiled shader code (assuming these are BGFX-compiled binaries)
    auto vertShaderCode = readfile(vertpath);
    auto fragShaderCode = readfile(fragpath);

    // 1. Create bgfx shader handles
    const bgfx::Memory* vertMem = bgfx::copy(vertShaderCode.data(), (uint32_t)vertShaderCode.size());
    const bgfx::Memory* fragMem = bgfx::copy(fragShaderCode.data(), (uint32_t)fragShaderCode.size());

    bgfx::ShaderHandle vertHandle = bgfx::createShader(vertMem);
    bgfx::ShaderHandle fragHandle = bgfx::createShader(fragMem);

    if (!bgfx::isValid(vertHandle) || !bgfx::isValid(fragHandle)) {
        throw std::runtime_error("Failed to create bgfx shader handles!");
    }

    // 2. Create the bgfx program handle
    // The 'true' flag tells bgfx to destroy the individual shader handles 
    // when the program handle is destroyed.
    bgfx::ProgramHandle programHandle = bgfx::createProgram(vertHandle, fragHandle, true);

    if (!bgfx::isValid(programHandle)) {
        throw std::runtime_error("Failed to create bgfx program handle!");
    }

    ShaderData shaderdata = {};
    shaderdata.asset = asset;
    shaderdata.programHandle = programHandle;

    //============PARSING METADATA AND CREATING BGFX UNIFORM HANDLES============//

    // NOTE: This section is conceptually simplified, as the full Vulkan reflection 
    // parsing logic is not available, but the core change is replacing Vulkan 
    // descriptor set logic with bgfx::createUniform.

    // COMBINE VERTEX AND FRAGMENT META DATA AND POPULATE uniformblocks/uniformfields
    // (Existing complex logic to combine data and map types would go here)

    // --- BGFX Resource Creation (Example Logic) ---
    // After populating uniformblocks and uniformfields from the meta JSON:

    // 3. Create Uniform Handles for Uniform Blocks (UBOs)
    /*
    for (auto& block : shaderdata.uniformblocks) {
        // Uniform blocks are typically named with a 'u_' prefix in bgfx.
        // We use Vec4 as the type, but it represents the entire UBO memory block.
        std::string uniformName = "u_" + block.name;
        block.uniformHandle = bgfx::createUniform(uniformName.c_str(), bgfx::UniformType::Vec4, 1);
        // Error handling for uniformHandle...
    }
    */

    // 4. Create Uniform Handles for Samplers/Textures and non-block single uniforms
    /*
    for (auto& field : shaderdata.uniformfields) {
        if (field.type == asset::Shader::UniformFieldType::TEXTURE) {
            // Samplers require a UniformType::Sampler handle
            field.uniformHandle = bgfx::createUniform(field.name.c_str(), bgfx::UniformType::Sampler);
        } else if (field.block.empty()) {
            // Handle single uniforms outside of blocks (if necessary)
            // Use the appropriate type (e.g., bgfx::UniformType::Vec4 for VEC4, bgfx::UniformType::Sampler for TEXTURE, etc.)
            // field.uniformHandle = bgfx::createUniform(field.name.c_str(), GetBgfxUniformType(field.type));
        }
    }
    */
    // Since the actual parsing logic is unavailable, the function returns the program handle.

    return shaderdata;
}


void gbe::gfx::ShaderLoader::UnLoadAsset_(asset::Shader* asset, const asset::data::ShaderImportData& importdata, asset::data::ShaderLoadData* data)
{
    const auto& shaderdata = this->GetAssetData(asset);

    // BGFX: Destroy the program handle (This also destroys the shaders because of the 'true' flag in createProgram)
    if (bgfx::isValid(shaderdata.programHandle)) {
        bgfx::destroy(shaderdata.programHandle);
    }

    // BGFX: Destroy all created uniform handles
    for (const auto& block : shaderdata.uniformblocks) {
        if (bgfx::isValid(block.uniformHandle)) {
            bgfx::destroy(block.uniformHandle);
        }
    }
    for (const auto& field : shaderdata.uniformfields) {
        // Destroy all handles created for textures and single uniforms (if not part of a block)
        if (bgfx::isValid(field.uniformHandle)) {
            bgfx::destroy(field.uniformHandle);
        }
    }
}

// REMOVED: VkShaderModule gbe::gfx::ShaderLoader::TryCompileShader(const std::vector<char>& code) {
// BGFX uses offline shader compilation.

bool gbe::gfx::ShaderData::FindUniformField(std::string id, ShaderField& out_field, ShaderBlock& out_block)
{
    for (const auto& field : this->uniformfields)
    {
        if (field.name == id) {
            for (const auto& block : this->uniformblocks)
            {
                if (block.name == field.block)
                    out_block = block;
            }
            out_field = field;
            return true;
        }
    }
    return false;
}

bool gbe::gfx::ShaderData::FindUniformBlock(std::string id, ShaderBlock& out_block)
{
    for (const auto& block : this->uniformblocks)
    {
        if (block.name == id) {
            out_block = block;
            return true;
        }
    }
    return false;
}

void gbe::gfx::ShaderLoader::AssignSelfAsLoader()
{
}