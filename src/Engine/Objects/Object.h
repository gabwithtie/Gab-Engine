#pragma once

#include "Engine/Component/Transform.h"
#include "Engine/Component/TransformChangeType.h"
#include "Engine/Serialization/SerializedObject.h"

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

#include <list>
#include <functional>

#include "Math/gbe_math.h"

namespace gbe {
	class Root;

	namespace editor {
		struct InspectorData;
	}

	class Object {
	private:
		bool isDestroyQueued = false;
		bool is_editor = false;

		std::list<Object*> children;

		Transform local = Transform([this](TransformChangeType type) {this->OnLocalTransformationChange(type); });
		Transform world = Transform([](TransformChangeType type) {});

		void MatToTrans(Transform* target, Matrix4 mat);
		
		Matrix4 parent_matrix;
	protected:
		Root* root;
		Object* parent;
		virtual void OnLocalTransformationChange(TransformChangeType changetype);
		virtual void OnExternalTransformationChange(TransformChangeType changetype, Matrix4 newparentmatrix);

		editor::InspectorData* inspectorData;
	public:
		Object();
		virtual ~Object();
		inline void Set_is_editor() {
			this->is_editor = true;
		}
		inline bool Get_is_editor() {
			return this->is_editor;
		}

		Transform& World();
		Transform& Local();
		Matrix4 GetWorldMatrix(bool include_local_scale = true);
		void SetLocalMatrix(Matrix4 mat);
		void SetWorldPosition(Vector3 vector);
		void TranslateWorld(Vector3 vector);

		virtual void OnEnterHierarchy(Object* newChild);
		virtual void OnExitHierarchy(Object* newChild);
		Object* GetParent();
		virtual void SetParent(Object* newParent);
		Object* GetChildAt(size_t i);
		size_t GetChildCount();

		editor::InspectorData* GetInspectorData();

		void Destroy();
		bool get_isDestroyed();
		inline void SetRoot(Root* newroot) {
			this->root = newroot;
		}

		void CallRecursively(std::function<void(Object*)> action);

		//SERIALIZATION
		virtual SerializedObject Serialize();
		virtual void Deserialize(SerializedObject data);
	};
}