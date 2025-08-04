#pragma once

#include <bullet/btBulletDynamicsCommon.h>

#include <unordered_map>
#include <functional>

#include "PhysicsWorld.h"
#include "ColliderData/ColliderData.h"

namespace gbe {
	namespace physics {
		class PhysicsPipeline
		{
		private:
			static PhysicsWorld* current_world;
		public:
			static inline void PushContext(PhysicsWorld* _world) {
				current_world = _world;
			}

			static PhysicsWorld* GetContext() {
				return current_world;
			}
		};
	}
}