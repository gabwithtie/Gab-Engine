#pragma once

#include "Math/gbe_math.h"

#include <bullet/btBulletDynamicsCommon.h>
#include "PhysicsDatatypes.h"
#include <list>
#include "ColliderData/ColliderData.h"

#include "PhysicsBody.h"


namespace gbe {
	class PhysicsObject;

	namespace physics {
		class TriggerRigidBody : public PhysicsBody {
		public:
			TriggerRigidBody(PhysicsObject* object);

			void Activate() override;
			void Deactivate() override;

			int Get_numInside();
			PhysicsBody* Get_inside(int index);

			void ForceUpdateTransform() override;
			void Pre_Tick_function(float deltatime) override;
		};
	}
}