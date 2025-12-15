#pragma once

#include "Asset/gbe_asset.h"
#include "Math/gbe_math.h"

#include "TextureLoader.h"
#include <bgfx/bgfx.h> // ADDED bgfx header

#include <optional>
#include <tuple>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <map> 

namespace gbe {
	namespace gfx {
		// (ShaderStageMeta, TextureMeta, UboType, UboMeta structs remain the same)
		struct ShaderStageMeta {
			struct TextureMeta {
				std::string name;
				std::string type;
				unsigned int set;
				unsigned int binding;
				std::vector<int> array;
			};
			struct UboType {
				struct UboTypeMember {
					std::string name;
					std::string type;
					unsigned int offset;
				};

				std::string name;
				std::vector<UboTypeMember> members;
			};
			struct UboMeta {
				std::string name;
				std::string type;
				unsigned int block_size;
				unsigned int set;
				unsigned int binding;
				std::vector<int> array;
			};
			std::unordered_map<std::string, UboType> types;
			std::vector<TextureMeta> textures;
			std::vector<UboMeta> ubos;
			std::vector<UboMeta> push_constants;
		};

		struct ShaderData {
			struct ShaderBlock {
				std::string name = "";
				size_t block_size = 0;
				size_t block_size_aligned = 0;
				unsigned int set = 0;
				unsigned int binding = 0;
				unsigned int array_size = 1;
				// NEW: Bgfx Uniform Handle for the uniform block (e.g., UBOs)
				bgfx::UniformHandle uniformHandle = BGFX_INVALID_HANDLE;
			};

			struct ShaderField {
				std::string name = "";
				std::string block = "";
				unsigned int set = 0;
				unsigned int binding = 0;
				asset::Shader::UniformFieldType type;
				unsigned int array_size = 1;
				size_t offset = 0;
				size_t size = 0;
				// NEW: Bgfx Uniform Handle for Samplers or single uniforms (if not in a block)
				bgfx::UniformHandle uniformHandle = BGFX_INVALID_HANDLE;
			};

			// REMOVED VULKAN MEMBERS (DescriptorSetLayouts, PipelineLayout, Pipeline)

			// NEW BGFX PROGRAM HANDLE
			bgfx::ProgramHandle programHandle = BGFX_INVALID_HANDLE;

			std::vector<ShaderBlock> uniformblocks;
			std::vector<ShaderField> uniformfields;
			asset::Shader* asset;

			bool FindUniformField(std::string id, ShaderField& out_field, ShaderBlock& out_block);
			bool FindUniformBlock(std::string id, ShaderBlock& out_block);
		};

		class ShaderLoader : public asset::AssetLoader<asset::Shader, asset::data::ShaderImportData, asset::data::ShaderLoadData, ShaderData> {
		protected:
			ShaderData LoadAsset_(asset::Shader* asset, const asset::data::ShaderImportData& importdata, asset::data::ShaderLoadData* data) override;
			void UnLoadAsset_(asset::Shader* asset, const asset::data::ShaderImportData& importdata, asset::data::ShaderLoadData* data) override;

			// REMOVED TryCompileShader as bgfx shaders are typically pre-compiled
		public:
			void AssignSelfAsLoader() override;
		};
	}
}