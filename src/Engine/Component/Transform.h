#pragma once

#include "Math/gbe_math.h"
#include "TransformChangeType.h"
#include <functional>

namespace gbe {
	class Transform {
	private:
		Matrix4 updated_matrix_without_scale;
		Matrix4 updated_matrix_with_scale;

		std::function<void(TransformChangeType)> onChange;

		void UpdateAxisVectors();
		void OnComponentChange(TransformChangeType type, bool silent = false);

		//AXIS VECTORS
		Vector3 Right;
		Vector3 Up;
		Vector3 Forward;

		inline Transform& operator =(const Transform& other) {
			return *this;
		}
	public:

		const Vector3& GetRight();
		const Vector3& GetUp();
		const Vector3& GetForward();

		TrackedVariable<Vector3> position = TrackedVariable<Vector3>([this](Vector3 old, Vector3 newvar) {
			if(!newvar.isfinite())
				throw new std::runtime_error("NAN position set.");
			this->OnComponentChange(TransformChangeType::TRANSLATION);
			});
		TrackedVariable<Vector3> scale = TrackedVariable<Vector3>([this](Vector3 old, Vector3 newvar) {
			if (!newvar.isfinite())
				throw new std::runtime_error("NAN scale set.");
			this->OnComponentChange(TransformChangeType::SCALE);
			});
		TrackedVariable<Quaternion> rotation = TrackedVariable<Quaternion>([this](Quaternion old, Quaternion newvar) {
			if (!isfinite(newvar.x) || !isfinite(newvar.y) || !isfinite(newvar.z) || !isfinite(newvar.w))
				throw new std::runtime_error("NAN rotation set.");
			this->OnComponentChange(TransformChangeType::ROTATION);
			});

		void Reset();

		Transform();
		Transform(std::function<void(TransformChangeType)> onChange);
		Transform(Matrix4 mat);

		void SetMatrix(Matrix4 mat, bool silent = false);
		Matrix4 GetMatrix(bool include_scale = true) const;
	};
}