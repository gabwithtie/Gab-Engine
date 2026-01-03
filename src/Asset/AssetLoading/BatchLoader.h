#pragma once

#include "../BaseAsset.h"

#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

#include "../AssetTypes/Mesh.h"
#include "../AssetTypes/Material.h"
#include "../AssetTypes/Shader.h"
#include "../AssetTypes/Texture.h"

namespace fs = std::filesystem;

namespace gbe {
	namespace asset {
		class BatchLoader {
		private:
            // Recursively finds all file paths within a given directory and its subdirectories.
            inline static void get_all_filepaths(const fs::path& directory_path, std::vector<fs::path>& filepaths) {
                try {
                    // Iterate through all entries (files and subdirectories) in the given directory.
                    for (const auto& entry : fs::directory_iterator(directory_path)) {
                        // Check if the current entry is a regular file.
                        if (fs::is_regular_file(entry.status())) {
                            // If it's a file, add its path to our vector.
                            filepaths.push_back(entry.path());
                        }
                        // Check if the current entry is a directory.
                        else if (fs::is_directory(entry.status())) {
                            // If it's a directory, recursively call the function on it.
                            get_all_filepaths(entry.path(), filepaths);
                        }
                    }
                }
                catch (const fs::filesystem_error& e) {
                    // Handle potential errors, such as permission denied.
                    std::cerr << "Error accessing directory " << directory_path << ": " << e.what() << std::endl;
                }
            }
            inline static bool is_file_extension(const std::string& filename, const std::string& extension) {
                // If the filename is shorter than the extension, it can't possibly match.
                if (filename.length() < extension.length()) {
                    return false;
                }

                // Compare the end of the filename with the extension.
                return filename.compare(filename.length() - extension.length(), extension.length(), extension) == 0;
            }
		public:
			inline static void LoadAssetsFromDirectory(std::string directory) {
				std::vector<fs::path> filepaths;
                get_all_filepaths(directory, filepaths);

				std::vector<fs::path> filepaths_material;

                for (size_t i = 0; i < filepaths.size(); i++)
                {
                    const auto& filepath = filepaths[i];
                    const auto& filename = filepath.filename().string();

                    internal::BaseAsset_base* newasset = nullptr;

                    if (is_file_extension(filename, ".obj.gbe")) {
                        std::cout << "[BATCHLOADER] Loading Mesh: \"" << filepath << "\"" << std::endl;
                        newasset = new Mesh(filepath);
                    }
                    else if (is_file_extension(filename, ".shader.gbe")) {
                        std::cout << "[BATCHLOADER] Loading Shader: \"" << filepath << "\"" << std::endl;
                        newasset = new Shader(filepath);
                    }
                    else if (is_file_extension(filename, ".mat.gbe")) {
						filepaths_material.push_back(filepath); // Defer material loading
                    }
                    else if (is_file_extension(filename, ".img.gbe")) {
                        std::cout << "[BATCHLOADER] Loading Texture: \"" << filepath << "\"" << std::endl;
                        newasset = new Texture(filepath);
                    }
                    else if(is_file_extension(filename, ".gbe")) {
						std::cout << "[BATCHLOADER] Unknown Asset Type in: \"" << filepath << "\"" << std::endl;
                    }
                }

                for (const auto& fp_mat : filepaths_material)
                {
                    std::cout << "[BATCHLOADER] Loading Material: \"" << fp_mat << "\"" << std::endl;
					internal::BaseAsset_base* newasset = new Material(fp_mat);
                }
			}
		};
	}
}