#pragma once

#include "../Object.h"
#include "Engine/ObjectFunctions/EarlyUpdate.h"
#include "Graphics/Data/Drawcall.h"

#include <unordered_map>

namespace gbe {
	class RenderObject : public Object, public EarlyUpdate {
	public:
		enum PrimitiveType {
			cube,
			sphere,
			capsule,
			plane
		};
	private:
		//PRIMITIVES CACHE
		static std::unordered_map<PrimitiveType, gfx::DrawCall*> primitive_drawcalls;

		//Rendering stuff
		gfx::DrawCall* mDrawCall;
		Matrix4* to_update;
		const bool editor = false;
	public:
		static void RegisterPrimitiveDrawcall(PrimitiveType, gfx::DrawCall*);

		RenderObject(gfx::DrawCall* mDrawCall, bool editor = false);
		virtual ~RenderObject();

		// Inherited via EarlyUpdate
		virtual void InvokeEarlyUpdate() override;

		inline gfx::DrawCall* Get_DrawCall() {
			return this->mDrawCall;
		}

		bool is_editor() {
			return this->editor;
		}
	};
}