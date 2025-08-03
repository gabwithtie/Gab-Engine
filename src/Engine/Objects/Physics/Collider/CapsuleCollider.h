#pragma once

#include "Collider.h"

#include "Physics/ColliderData/CapsuleColliderData.h"

namespace gbe {
	class CapsuleCollider : public Collider {
	private:
		physics::CapsuleColliderData* mData;
	public:
		CapsuleCollider();

		// Inherited via Collider
		physics::ColliderData* GetColliderData() override;

		static Object* Create(SerializedObject data);
	};
}