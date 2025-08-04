#pragma once

#include "Engine/Objects/Object.h"
#include "Collider/Collider.h"
#include "Physics/gbe_physics.h"

namespace gbe {
	class PhysicsObject : public Object {
	protected:
		std::function<PhysicsObject* (physics::PhysicsBody*)>* lookup_func;
		std::list<Collider*> colliders;
		physics::PhysicsBody* body;
		physics::PhysicsWorld* world;

		void On_Change_enabled(bool _to) override;
	public:
		virtual ~PhysicsObject();

		virtual void OnEnterHierarchy(Object* newChild);
		virtual void OnExitHierarchy(Object* newChild);

		void OnLocalTransformationChange(TransformChangeType) override;
		void OnExternalTransformationChange(TransformChangeType, Matrix4 parentmat) override;

		void UpdateCollider(Collider* what);
		inline void ForceWake() {
			body->ForceWake();
		}

		void Set_lookup_func(std::function<PhysicsObject* (physics::PhysicsBody*)>*);
		physics::PhysicsBody* Get_data();

		const std::list<Collider*> Get_colliders();
	};
}