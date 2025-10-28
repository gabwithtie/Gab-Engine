#pragma once

#include <bullet/btBulletDynamicsCommon.h>

#include <unordered_map>
#include <functional>

#include "PhysicsBody.h"
#include "ColliderData/ColliderData.h"

namespace gbe {
	namespace physics {
		class PhysicsWorld
		{
		private:
			std::unordered_map<const btCollisionObject*, PhysicsBody*> body_wrapper_dictionary;
			std::unordered_map<const btCollisionShape*, ColliderData*> collider_wrapper_dictionary;
			std::function<void(float physicsdeltatime)> OnFixedUpdate_callback;

			btDefaultCollisionConfiguration* collisionConfiguration;
			btCollisionDispatcher* dispatcher;
			btBroadphaseInterface* overlappingPairCache;
			btSequentialImpulseConstraintSolver* solver;
			btDiscreteDynamicsWorld* dynamicsWorld;

			void internal_physics_callback(btDynamicsWorld* world, btScalar timeStep);
		public:
			bool Init();
			void Tick(double delta);
			inline void RegisterBody(PhysicsBody* body) {
				if (body->IsActive())
					UnRegisterBody(body);

				body_wrapper_dictionary.insert_or_assign(body->Get_wrapped_data(), body);
				body->Register(this);
				body->Activate();
			}
			inline void UnRegisterBody(PhysicsBody* body) {
				body_wrapper_dictionary.erase(body->Get_wrapped_data());
				body->Deactivate();
			}
			inline void RegisterCollider(ColliderData* body) {
				collider_wrapper_dictionary.insert_or_assign(body->GetShape(), body);
			}
			inline void UnRegisterCollider(ColliderData* body) {
				collider_wrapper_dictionary.erase(body->GetShape());
			}
			inline void Set_OnFixedUpdate_callback(std::function<void(float physicsdeltatime)> newfunc) {
				this->OnFixedUpdate_callback = newfunc;
			}

			btDiscreteDynamicsWorld* Get_world();
			inline btCollisionDispatcher* Get_dispatcher() {
				return this->dispatcher;
			}

			PhysicsBody* GetRelatedBody(const btCollisionObject*);
			ColliderData* GetRelatedCollider(const btCollisionShape*);
		};
	}
}