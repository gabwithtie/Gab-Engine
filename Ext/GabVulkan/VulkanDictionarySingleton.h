#pragma once

#include <unordered_map>
#include <string>

namespace gbe::vulkan {
	template<typename T>
	class VulkanDictionarySingleton {
		static std::unordered_map<std::string, T*> _Actives;
	public:
		static T* GetActive(std::string id) {
			return _Actives[id];
		}
		static void SetActive(std::string id, T* newactive) {
			_Actives[id] = newactive;
		}
	};

	template<typename T>
	std::unordered_map<std::string, T*> VulkanDictionarySingleton<T>::_Actives = {};
}