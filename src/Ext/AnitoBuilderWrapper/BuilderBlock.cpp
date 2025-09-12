#include "BuilderBlock.h"

#include <vector>

namespace gbe::ext::AnitoBuilder {
	Object* BuilderBlock::CreateBlock(gbe::Vector3 corners[4], float height)
	{
		Object* block = new Object();

		auto base_center = Vector3::Mid(Vector3::Mid(corners[0], corners[1]), Vector3::Mid(corners[2], corners[3]));

		std::vector<Vector3> positions = {
			Vector3::Mid(corners[0], corners[1]) + Vector3(0, height * 0.5f, 0),
			Vector3::Mid(corners[1], corners[2]) + Vector3(0, height * 0.5f, 0),
			Vector3::Mid(corners[2], corners[3]) + Vector3(0, height * 0.5f, 0),
			Vector3::Mid(corners[3], corners[0]) + Vector3(0, height * 0.5f, 0),

			base_center + Vector3(0, height, 0),
			base_center
		};

		for (const auto& pos: positions)
		{
			auto handle = new RigidObject(true);
			auto collider = new BoxCollider();
			collider->SetParent(handle);
			auto renderer = new RenderObject(RenderObject::cube);
			renderer->SetParent(handle);

			handle->SetParent(block);

			handle->World().position.Set(pos);
			handle->Local().scale.Set(Vector3(0.5f));
		}

		return block;
	}
}