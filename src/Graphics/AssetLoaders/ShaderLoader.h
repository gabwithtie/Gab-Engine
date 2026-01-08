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

			static std::unordered_map<std::string, bgfx::UniformHandle> m_uniforms;
			static std::unordered_map<std::string, bgfx::UniformHandle> m_uniformarrays;

			inline bool GetUniform(bgfx::UniformHandle& out, std::string& name) const {
				auto it = m_uniforms.find(name);
				if (it != m_uniforms.end())
				{
					out = it->second;
					return true;
				}

				return false;
			}


			inline bool GetUniformArray(bgfx::UniformHandle& out, const std::string& name) const {
				auto it = m_uniformarrays.find(name);
				if (it != m_uniformarrays.end())
				{
					out = it->second;
					return true;
				}

				return false;
			}

			template<typename T>
			void RegisterUniform(std::string& name, int size = 1) {
				throw new std::runtime_error("Unsupported uniform type.");
			}

			void Internal_CreateVec4Uniform(std::string& name, int size) {
				bgfx::UniformHandle handle = bgfx::createUniform(name.c_str(), bgfx::UniformType::Vec4, size);
				
				if(size == 1)
					m_uniforms[name] = handle;
				else
					m_uniformarrays[name] = handle;

			}

			template <>
			void RegisterUniform<int>(std::string& name, int size) {
				Internal_CreateVec4Uniform(name, size);
			}
			template <>
			void RegisterUniform<float>(std::string& name, int size) {
				Internal_CreateVec4Uniform(name, size);
			}
			template <>
			void RegisterUniform<Vector2>(std::string& name, int size) {
				Internal_CreateVec4Uniform(name, size);
			}
			template <>
			void RegisterUniform<Vector3>(std::string& name, int size) {
				Internal_CreateVec4Uniform(name, size);
			}
			template <>
			void RegisterUniform<Vector4>(std::string& name, int size) {
				Internal_CreateVec4Uniform(name, size);
			}
			template <>
			void RegisterUniform<Matrix4>(std::string& name, int size) {
				bgfx::UniformHandle handle = bgfx::createUniform(name.c_str(), bgfx::UniformType::Mat4, size);
				if (size == 1)
					m_uniforms[name] = handle;
				else
					m_uniformarrays[name] = handle;
			}

			template <>
			void RegisterUniform<TextureData>(std::string& name, int size) {
				bgfx::UniformHandle handle = bgfx::createUniform(name.c_str(), bgfx::UniformType::Sampler, size);
				if (size == 1)
					m_uniforms[name] = handle;
				else
					m_uniformarrays[name] = handle;
			}

			// BGFX: ApplyOverride for simple types (bool, float, vec, matrix)
			// It uses bgfx::setUniform to update the uniform on the active view/command buffer.
			template<typename T>
			inline bool ApplyOverride(const T& valueref, std::string target) {

				bgfx::UniformHandle handle;

				if (!this->GetUniform(handle, target))
					this->RegisterUniform<T>(target); //temporary fix, we want to do this at shader loading stage

				this->GetUniform(handle, target);

				bgfx::setUniform(handle, &valueref, 1);

				return true;
			}

			template<>
			inline bool ApplyOverride<TextureData>(const TextureData& valueref, std::string target)
			{
				throw new std::runtime_error("Use ApplyTextureOverride instead.");
			}
			// BGFX: ApplyOverride specialization for TextureData
			inline bool ApplyTextureOverride(const TextureData& valueref, std::string target, int stage) {

				bgfx::UniformHandle handle;

				if (!this->GetUniform(handle, target))
					this->RegisterUniform<TextureData>(target); //temporary fix, we want to do this at shader loading stage

				this->GetUniform(handle, target);

				bgfx::setTexture(stage, handle, valueref.textureHandle);

				return true;
			}

			template<typename T>
			inline bool ApplyOverrideArray(const T* valueref, std::string target, int count) {

				bgfx::UniformHandle handle;

				if (!this->GetUniformArray(handle, target))
					this->RegisterUniform<T>(target, count); //temporary fix, we want to do this at shader loading stage

				this->GetUniformArray(handle, target);

				bgfx::setUniform(handle, valueref, count);

				return true;
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