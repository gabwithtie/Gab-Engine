#pragma once

#include "Math/gbe_math.h"

#include <bullet/btBulletDynamicsCommon.h>
#include "PhysicsDatatypes.h"
#include <list>
#include "ColliderData/ColliderData.h"

namespace gbe {
	class PhysicsObject;

	namespace physics {
		class PhysicsWorld;

		class PhysicsBody {
		protected:
			PhysicsWorld* world = nullptr;

			btTransform transform;
			btCollisionObject* base_data = nullptr;
			btDefaultMotionState* motionstate = nullptr;

			PhysicsObject* related_engine_wrapper = nullptr;

			btCompoundShape* mMainShape = nullptr;
			PhysicsBody(PhysicsObject* _related_engine_wrapper);

			bool active = true;
		public:
			void InjectCurrentTransformMatrix(Matrix4);
			void PullTransformationData(Vector3&, Quaternion&);
			void PassTransformationMatrix(Matrix4&);

			btCollisionObject* Get_wrapped_data();
			PhysicsObject* Get_wrapper();

			inline PhysicsWorld* Get_world() {
				return this->world;
			}

			inline void Register(PhysicsWorld* register_to) {
				this->world = register_to;
			}

			inline virtual void Activate() {
				active = true;
			}
			inline virtual void Deactivate() {
				active = false;
			}

			void AddCollider(ColliderData*);
			void UpdateColliderTransform(ColliderData*);
			void RemoveCollider(ColliderData*);

			void ForceWake();
			void UpdateAABB();
			virtual void ForceUpdateTransform() = 0;
			virtual void Pre_Tick_function(float deltatime) = 0;
		};
	}
}