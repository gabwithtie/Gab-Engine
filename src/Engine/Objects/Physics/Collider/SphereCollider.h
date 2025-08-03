#pragma once

#include "Collider.h"

#include "Physics/ColliderData/SphereColliderData.h"

namespace gbe {
	class SphereCollider : public Collider {
	private:
		physics::SphereColliderData* mData;
	public:
		SphereCollider();

		// Inherited via Collider
		physics::ColliderData* GetColliderData() override;

		static Object* Create(SerializedObject data);

		
	};
}