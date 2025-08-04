#pragma once
#include "ColliderData.h"
#include "../PhysicsDatatypes.h"

namespace gbe {
	namespace physics {
		class CapsuleColliderData : public ColliderData {
		private:
			btCapsuleShape mShape;
		public:
			CapsuleColliderData(Collider* related_engine_wrapper);
			virtual btCollisionShape* GetShape() override;
		};
	}
}