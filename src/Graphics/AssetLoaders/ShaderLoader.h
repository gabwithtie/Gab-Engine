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

			std::unordered_map<std::string, bgfx::UniformHandle> m_uniforms;

			inline bool GetUniform(bgfx::UniformHandle& out, const std::string& name) {
				auto it = m_uniforms.find(name);
				if (it != m_uniforms.end())
				{
					out = it->second;
					return true;
				}

				return false;
			}

			std::unordered_map<std::string, bgfx::UniformHandle> m_uniformarrays;

			inline bool GetUniformArray(bgfx::UniformHandle& out, const std::string& name) {
				auto it = m_uniformarrays.find(name);
				if (it != m_uniformarrays.end())
				{
					out = it->second;
					return true;
				}

				return false;
			}

			template<typename T>
			void RegisterUniform(const std::string& name, int size = 1) {
				throw new std::runtime_error("Unsupported uniform type.");
			}

			void Internal_CreateVec4Uniform(const std::string& name, int size) {
				bgfx::UniformHandle handle = bgfx::createUniform(name.c_str(), bgfx::UniformType::Vec4, size);
				
				if(size == 1)
					m_uniforms[name] = handle;
				else
					m_uniformarrays[name] = handle;

			}

			template <>
			void RegisterUniform<int>(const std::string& name, int size) {
				Internal_CreateVec4Uniform(name, size);
			}
			template <>
			void RegisterUniform<float>(const std::string& name, int size) {
				Internal_CreateVec4Uniform(name, size);
			}
			template <>
			void RegisterUniform<Vector2>(const std::string& name, int size) {
				Internal_CreateVec4Uniform(name, size);
			}
			template <>
			void RegisterUniform<Vector3>(const std::string& name, int size) {
				Internal_CreateVec4Uniform(name, size);
			}
			template <>
			void RegisterUniform<Vector4>(const std::string& name, int size) {
				Internal_CreateVec4Uniform(name, size);
			}
			template <>
			void RegisterUniform<Matrix4>(const std::string& name, int size) {
				bgfx::UniformHandle handle = bgfx::createUniform(name.c_str(), bgfx::UniformType::Mat4, size);
				if (size == 1)
					m_uniforms[name] = handle;
				else
					m_uniformarrays[name] = handle;
			}

			template <>
			void RegisterUniform<TextureData>(const std::string& name, int size) {
				bgfx::UniformHandle handle = bgfx::createUniform(name.c_str(), bgfx::UniformType::Sampler, size);
				if (size == 1)
					m_uniforms[name] = handle;
				else
					m_uniformarrays[name] = handle;
			}
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