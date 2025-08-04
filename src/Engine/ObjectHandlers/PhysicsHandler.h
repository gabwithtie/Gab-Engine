#pragma once

#include <glm/gtx/quaternion.hpp>

#include <list>

#include "Physics/gbe_physics.h"
#include "ObjectHandler.h"
#include "Engine/Objects/Physics/RigidObject.h"
#include "Engine/Objects/Physics/ForceVolume.h"
#include <unordered_map>
#include <functional>

namespace gbe {
	class PhysicsHandler : public ObjectHandler<PhysicsObject>{
	private:
		std::function<PhysicsObject* (physics::PhysicsBody*)> lookup_func;
		std::unordered_map<physics::PhysicsBody*, PhysicsObject*> map;
		ObjectHandler<ForceVolume> forcevolume_handler;

		physics::PhysicsWorld* localpipeline;
	public:
		PhysicsHandler();

		inline physics::PhysicsWorld* GetLocalPipeline() {
			return this->localpipeline;
		}

		void Update(double dt);

		virtual void OnAdd(PhysicsObject*) override;
		virtual void OnRemove(PhysicsObject*) override;
	};
}