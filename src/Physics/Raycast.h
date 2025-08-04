#pragma once

#include "PhysicsDatatypes.h"
#include "PhysicsBody.h"

#include <bullet/BulletCollision/CollisionDispatch/btCollisionWorld.h>

namespace gbe {
	class PhysicsObject;
	class Collider;

	namespace physics {
		struct Raycast {
		private:
			class RaycastCallback : public btCollisionWorld::ClosestRayResultCallback {
			public:
				const btCollisionShape* hitShape;
				const btCollisionObject* hitObject;
				btCollisionWorld::LocalShapeInfo* localShapeInfo;

				RaycastCallback(PhysicsVector3 from, PhysicsVector3 to);

				btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) override;
			};
		
		public:
			bool result = false;
			PhysicsObject* other = nullptr;
			Collider* collider = nullptr;
			PhysicsVector3 intersection;
			PhysicsVector3 normal;
			float distance = 0;

			Raycast(PhysicsVector3 from, PhysicsVector3 dir);
			
		};
	}
}