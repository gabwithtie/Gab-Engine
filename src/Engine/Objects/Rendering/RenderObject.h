#pragma once

#include "../Object.h"
#include "Engine/ObjectFunctions/EarlyUpdate.h"
#include "Graphics/Data/Drawcall.h"

#include <unordered_map>

namespace gbe {
	class RenderObject : public Object, public EarlyUpdate {
	public:
		enum PrimitiveType {
			NONE = 0,
			cube,
			sphere,
			capsule,
			plane
		};
		const static std::unordered_map<PrimitiveType, std::string> PrimitiveTypeStrs;
		inline static std::string PrimitiveTypeStr(PrimitiveType _ptype) {
			return PrimitiveTypeStrs.at(_ptype);
		}
	private:
		//PRIMITIVES CACHE
		static std::unordered_map<PrimitiveType, gfx::DrawCall*> primitive_drawcalls;

		//Rendering stuff
		gfx::DrawCall* mDrawCall;
		Matrix4* to_update;
		const bool editor = false;
		PrimitiveType ptype = PrimitiveType::NONE;
	public:
		static inline void RegisterPrimitiveDrawcall(PrimitiveType ptype, gfx::DrawCall* drawtype) {
			primitive_drawcalls.insert_or_assign(ptype, drawtype);
		}

		RenderObject(gfx::DrawCall* mDrawCall, bool editor = false);
		RenderObject(PrimitiveType ptype, bool editor = false);
		virtual ~RenderObject();

		SerializedObject Serialize() override;
		static Object* Create(SerializedObject data);

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