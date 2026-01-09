#pragma once

namespace gbe::vulkan{
	template<typename T>
	class VulkanObjectSingleton {
		static T* _Active;
	public:
		static T* GetActive() {
			return _Active;
		}
		static void SetActive(T* newactive) {
			_Active = newactive;
		}
	};

	template<typename T>
	T* VulkanObjectSingleton<T>::_Active = nullptr;
}