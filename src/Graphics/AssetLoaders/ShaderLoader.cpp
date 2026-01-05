#include "ShaderLoader.h"
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

    //obscure error catcher
    if (importdata.vert.contains("fs")) {
        std::cout << "Vertex shader is tagged as fragment shader, check if you got it swapped." << std::endl;
    }
    if (importdata.frag.contains("vs")) {
        std::cout << "Fragment shader is tagged as vertex shader, check if you got it swapped." << std::endl;
    }

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

    return shaderdata;
}


void gbe::gfx::ShaderLoader::UnLoadAsset_(asset::Shader* asset, const asset::data::ShaderImportData& importdata, asset::data::ShaderLoadData* data)
{
    const auto& shaderdata = this->GetAssetRuntimeData(asset->Get_assetId());

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

void gbe::gfx::ShaderLoader::AssignSelfAsLoader()
{
    AssetLoader::AssignSelfAsLoader();
}