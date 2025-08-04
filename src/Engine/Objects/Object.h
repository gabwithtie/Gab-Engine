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
		static std::unordered_map<unsigned int, Object*> valid_objects;
		static unsigned int next_avail_id;
		unsigned int id = 0;

		bool enabled_hierarchy = true;
		bool enabled_self = true;

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
		inline virtual void On_Change_enabled(bool _to) {
			this->enabled_hierarchy = _to;
		}

		editor::InspectorData* inspectorData;
	public:
		Object();
		virtual ~Object();
		inline void Set_is_editor() {
			this->is_editor = true;
			size_t childcount = GetChildCount();
			for (size_t i = 0; i < childcount; i++)
			{
				this->GetChildAt(i)->Set_is_editor();
			}
		}
		inline static bool ValidateObject(Object* obj){
			auto objid = obj->id;
			if (valid_objects.find(objid) == valid_objects.end())
				return false;

			if (valid_objects[objid] != obj)
				return false;

			return true;
		}
		inline void Set_enabled(bool _to) {
			this->enabled_self = _to;

			if (_to == false) {
				this->CallRecursively([](Object* child) {
					if(child->enabled_hierarchy)
						child->On_Change_enabled(false);
					});
			}
			if (_to) {
				this->CallRecursively([](Object* child) {
					bool parent_active = child->parent == nullptr || child->parent->enabled_hierarchy;

					if (child->enabled_self) {
						if (!child->enabled_hierarchy && parent_active)
							child->On_Change_enabled(true);
					}
					}, false);
			}
		}
		inline bool Get_enabled() {
			return this->enabled_hierarchy;
		}
		inline bool Get_enabled_self() {
			return this->enabled_self;
		}
		inline bool Get_is_editor() {
			return this->is_editor;
		}
		inline void Set_id(unsigned int _id) {
			this->id = _id;
		}
		inline unsigned int Get_id() {
			return this->id;
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
		inline size_t GetIndexOfChild(Object* _child) {
			size_t index = 0;

			for (const auto child : this->children)
			{
				if (_child == child)
					return index;

				index++;
			}

			return -1;
		}
		size_t GetChildCount();

		editor::InspectorData* GetInspectorData();

		void Destroy();
		bool get_isDestroyed();
		inline void SetRoot(Root* newroot) {
			this->root = newroot;
		}

		void CallRecursively(std::function<void(Object*)> action, bool bottom_up = true);

		//SERIALIZATION
		virtual SerializedObject Serialize();
		virtual void Deserialize(SerializedObject data, bool root = true);
	};
}