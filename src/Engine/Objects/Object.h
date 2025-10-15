#pragma once

#include "Engine/Component/Transform.h"
#include "Engine/Component/TransformChangeType.h"
#include "Engine/Serialization/SerializedObject.h"

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

#include <list>
#include <functional>
#include <algorithm>

#include "Math/gbe_math.h"


namespace gbe {
	class Root;

	namespace editor {
		struct InspectorData;
	}

	class Object {
	public:
		enum ObjectStateName
		{
			TRANSFORMED_LOCAL,
			TRANSFORMED_USER,
			TRANSFORMED_WORLD_NOT_LOCAL
		};
		enum EditorFlags
		{
			STATIC_POS_X = 1 << 0,
			STATIC_POS_Y = 1 << 1,
			STATIC_POS_Z = 1 << 2,
			STATIC_ROT_X = 1 << 3,
			STATIC_ROT_Y = 1 << 4,
			STATIC_ROT_Z = 1 << 5,
			STATIC_SCALE_X = 1 << 6,
			STATIC_SCALE_Y = 1 << 7,
			STATIC_SCALE_Z = 1 << 8,

			IS_STATE_MANAGED = 1 << 9,
			SELECT_PARENT_INSTEAD = 1 << 10,
			EXCLUDE_FROM_OBJECT_TREE = 1 << 11,
			SERIALIZABLE = 1 << 12
		};

	private:
		static std::unordered_map<unsigned int, Object*> valid_objects;
		static unsigned int next_avail_id;
		unsigned int id = 0;

		std::string name = "Object";

		bool enabled_hierarchy = true;
		bool enabled_self = true;

		bool isDestroyQueued = false;

		std::list<Object*> children;

		//A smart way to keep track of states and whether some external objects have already read the state or not.
		std::unordered_map<ObjectStateName, std::vector<void*>> state_checkers;
		EditorFlags editor_flags = (EditorFlags)0;

		Transform local;
		Transform world;
		
		Matrix4 parent_matrix = Matrix4(1.0f);

		void General_init();
	protected:
		Root* root = nullptr;
		Object* parent = nullptr;
		virtual void OnLocalTransformationChange(TransformChangeType changetype);
		virtual void OnExternalTransformationChange(TransformChangeType changetype, Matrix4 newparentmatrix);
		inline virtual void On_Change_enabled(bool _to) {
			this->enabled_hierarchy = _to;
		}

		editor::InspectorData* inspectorData = nullptr;

		virtual void InitializeInspectorData();
	public:

		Object();
		virtual ~Object();

		inline std::string GetName() {
			return this->name;
		}
		inline void SetName(std::string newname) {
			this->name = newname;
		}

		inline void PushState(ObjectStateName state) {
			auto it = this->state_checkers.find(state);

			if (it != this->state_checkers.end())
			{
				it->second.clear();
			}
			else {
				this->state_checkers.insert_or_assign(state, std::vector<void*>());
			}
		}
		inline bool CheckState(ObjectStateName state, void* checker) {
			auto it = this->state_checkers.find(state);

			if (it != this->state_checkers.end())
			{
				auto check_it = std::find(it->second.begin(), it->second.end(), checker);

				if (check_it == it->second.end()) {
					it->second.push_back(checker);
					return true;
				}
			}

			return false;
		}
		inline bool GetEditorFlag(EditorFlags flag) {
			return (this->editor_flags & flag) == flag;
		}
		inline void PushEditorFlag(EditorFlags flag) {
			this->editor_flags = (EditorFlags)(this->editor_flags | flag);
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
					bool parent_active = child->parent != nullptr ? child->parent->enabled_hierarchy : true;

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
		inline void Set_id(unsigned int _id) {
			this->id = _id;
		}
		inline unsigned int Get_id() {
			return this->id;
		}

		Transform& World();
		Transform& Local();

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
		//DESERIALIZATION
		Object(SerializedObject* data, bool load_children = true);
		void LoadChildren(SerializedObject* data);
	};
}