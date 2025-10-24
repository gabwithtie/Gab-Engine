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

		//RENDERING CACHE
		gfx::DrawCall* mDrawCall = nullptr;
		Matrix4* to_update = nullptr;

		RenderObject* shadow_renderer = nullptr;

		PrimitiveType ptype = PrimitiveType::NONE;

		bool user_created = false;

	protected:
		void On_Change_enabled(bool _to) override;
	public:
		static inline void RegisterPrimitiveDrawcall(PrimitiveType ptype, gfx::DrawCall* drawtype) {
			primitive_drawcalls.insert_or_assign(ptype, drawtype);
		}

		void SetUserCreated();

		void SetShadowCaster();

		RenderObject(SerializedObject* data);
		RenderObject(gfx::DrawCall* mDrawCall);
		RenderObject(PrimitiveType ptype);
		virtual ~RenderObject();

		SerializedObject Serialize() override;

		// Inherited via EarlyUpdate
		virtual void InvokeEarlyUpdate() override;

		inline gfx::DrawCall* Get_DrawCall() {
			return this->mDrawCall;
		}
	};
}