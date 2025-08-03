#pragma once

#include <vector>
#include <string>
#include <functional>

namespace gbe {
	class Console {
		static std::vector<std::string> logs;
		static std::vector < std::function<void(std::string)>> subscribed_delegates;
	public:
		static inline void Subscribe(std::function<void(std::string)> _delegate) {
			subscribed_delegates.push_back(_delegate);
		}

		static inline void Log(std::string log) {
			logs.push_back(log);

			for (const auto subscriber : subscribed_delegates)
			{
				subscriber(log);
			}
		}
	};
}