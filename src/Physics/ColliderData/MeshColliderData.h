#pragma once

#include "ColliderData.h"

#include <vector>
#include "Math/gbe_math.h"

namespace gbe {
	namespace physics {
		class MeshColliderData : public ColliderData{
		private:
			btBvhTriangleMeshShape* trimeshShape = nullptr;
			btTriangleMesh* trimesh;
		public:
			MeshColliderData(Collider* related_engine_wrapper);
			MeshColliderData(std::vector<std::vector<Vector3>>, Collider* related_engine_wrapper);
			void UpdateMesh(std::vector<std::vector<Vector3>> faces);
			virtual btCollisionShape* GetShape() override;
		};
	}
}