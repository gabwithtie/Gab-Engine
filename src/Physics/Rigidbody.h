#pragma once

#include "Math/gbe_math.h"

#include <bullet/btBulletDynamicsCommon.h>

#include <list>

#include "PhysicsBody.h"
#include "PhysicsDatatypes.h"
#include "ColliderData/ColliderData.h"

namespace gbe {
	class PhysicsObject;

	namespace physics {
		class PhysicsWorld;

		class Rigidbody : public PhysicsBody{
		private:
			PhysicsVector3 continouus_forces_this_frame;
			float mass;
		public:
			Rigidbody(PhysicsObject* object, bool is_static = false);

			void Activate() override;
			void Deactivate() override;

			void SetStatic(bool);
			void AddForce(PhysicsVector3 force);
			void AddContinuousForce(PhysicsVector3 force);
			float Get_mass();
			PhysicsVector3 Get_angularVelocity();
			void Set_angularVelocity(PhysicsVector3);
			PhysicsVector3 Get_velocity();
			void Set_velocity(PhysicsVector3);
			void AddForceAtPoint(PhysicsVector3 force, PhysicsVector3 relativeWorldPoint);			

			void Pre_Tick_function(float deltatime) override;

			void ForceUpdateTransform() override;
		};
	}
}