#pragma once

#include "../Object.h"
#include "RigidObject.h"

namespace gbe {
	class ForceVolume : public Object {
	public:
		enum Shape {
			GLOBAL,
			BOX,
			SPHERE
		};
		enum Mode {
			DIRECTIONAL,
			RADIAL,
			ORBITAL
		};
		enum ForceMode {
			FORCE,
			VELOCITY
		};

		Shape shape = ForceVolume::GLOBAL;
		Vector3 half_bounds;
		float radius;
		
		Mode mode = ForceVolume::DIRECTIONAL;
		//for directional
		Vector3 vector = Vector3(0.f, -12, 0.f);
		//for radial and orbital
		float scalar;

		ForceMode forceMode = ForceVolume::VELOCITY;

		void TryApply(RigidObject* object);
	};
}