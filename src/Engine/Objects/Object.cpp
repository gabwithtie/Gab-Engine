#include "Object.h"
#include "Root.h"
#include "Engine/Serialization/TypeSerializer.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include "Editor/gbe_editor.h"

unsigned int gbe::Object::next_avail_id = 0;
std::unordered_map<unsigned int, gbe::Object*> gbe::Object::valid_objects;

void gbe::Object::PushState(ObjectStateName state)
{
	if (this->isDestroyQueued)
		return;

	auto it = this->state_checkers.find(state);

	if (it != this->state_checkers.end())
	{
		it->second.clear();
	}
	else {
		this->state_checkers.insert_or_assign(state, std::vector<uint16_t>());
	}
}

bool gbe::Object::CheckState(ObjectStateName state, void* checker)
{
	if (this->isDestroyQueued)
		return false;

	auto it = this->state_checkers.find(state);
	
	if (it != this->state_checkers.end())
	{
		auto& checkers = it->second;

		auto check_it = std::find(checkers.begin(), checkers.end(), (uint16_t)checker);

		if (check_it == checkers.end()) {
			checkers.push_back((uint16_t)checker);
			return true;
		}
	}

	return false;
}

void gbe::Object::OnLocalTransformationChange(TransformChangeType changetype)
{
	this->PushState(ObjectStateName::TRANSFORMED_LOCAL);

	auto worldmat = this->parent_matrix * this->local.GetMatrix();

	for (auto child : this->children)
	{
		if (child->get_isDestroyed())
			continue;

		child->OnExternalTransformationChange(changetype, worldmat);
	}

	//silent because only reflecting
	this->world.SetMatrix(worldmat, true);

	if (isnan(this->local.GetMatrix()[0][0])) {
		std::cerr << "NAN transform, resetting transform." << std::endl;
		this->local.Reset();
	}
}

void gbe::Object::OnExternalTransformationChange(TransformChangeType changetype, Matrix4 newparentmatrix)
{
	this->PushState(ObjectStateName::TRANSFORMED_WORLD_NOT_LOCAL);

	this->parent_matrix = newparentmatrix;
	auto worldmat = this->parent_matrix * this->local.GetMatrix();

	for (auto child : this->children)
	{
		child->OnExternalTransformationChange(changetype, worldmat);
	}

	//silent because only reflecting
	this->world.SetMatrix(worldmat, true);

	if (isnan(this->local.GetMatrix()[0][0])) {
		std::cerr << "NAN transform, resetting transform." << std::endl;
		this->local.Reset();
	}
}

void gbe::Object::General_init()
{
	//TRANSFORM
	this->parent_matrix = Matrix4(1.0f);
	this->parent = nullptr;

	OnLocalTransformationChange(TransformChangeType::ALL);
	OnExternalTransformationChange(TransformChangeType::ALL, this->parent_matrix);

	this->world.position.AddCallback([this](Vector3 oldval, Vector3 newval) {
		if (this->get_isDestroyed()) return;

		this->Local().position.Set(Vector3(parent_matrix.Inverted() * Vector4(newval, 1.0f)));
		});
	this->world.scale.AddCallback([this](Vector3 oldval, Vector3 newval) {
		if (this->get_isDestroyed()) return;

		Vector3 finalLocalScale;

		if (parent != nullptr) {
			auto parent_scale = this->parent->World().scale.Get();
			finalLocalScale = Vector3(
				newval.x / parent_scale.x,
				newval.y / parent_scale.y,
				newval.z / parent_scale.z
			);
		}
		else {
			finalLocalScale = newval;
		}

		this->Local().scale.Set(finalLocalScale);
		});
	this->world.rotation.AddCallback([this](Quaternion oldval, Quaternion newval) {
		if (this->get_isDestroyed()) return;

		auto parent_rot = Quaternion(parent_matrix);
		auto new_local_rot = parent_rot.Inverted() * newval;
		this->Local().rotation.Set(new_local_rot);
		});

	this->InitializeInspectorData();

	this->id = next_avail_id;
	next_avail_id++;
	valid_objects.insert_or_assign(this->id, this);
}

gbe::Object::Object():
	local(Transform([this](TransformChangeType type) {this->OnLocalTransformationChange(type); })),
	world()
{
	General_init();
}

void gbe::Object::InitializeInspectorData() {
	this->inspectorData = new editor::InspectorData();
}

gbe::Object::~Object(){
	
}

gbe::Transform& gbe::Object::World()
{
	return this->world;
}

gbe::Transform& gbe::Object::Local()
{
	return this->local;
}

void gbe::Object::ReEnterHierarchy()
{
	auto og_parent = this->parent;

	if (og_parent != nullptr) {
		og_parent->OnExitHierarchy(this);
		og_parent->OnEnterHierarchy(this);
	}
}

void gbe::Object::OnEnterHierarchy(Object* newChild)
{
	Object* current = this->parent;

	while (current != nullptr)
	{
		current->OnEnterHierarchy(newChild);
		current = current->parent;
	}

	this->PushState(TREE_CHANGED);
}

void gbe::Object::OnExitHierarchy(Object* newChild)
{
	this->PushState(TREE_CHANGED);
}

gbe::Object* gbe::Object::GetParent()
{
	return this->parent;
}

void gbe::Object::SetParent(Object* newParent)
{
	if (newParent == this->parent)
		return;

	if (parent != nullptr) {
		auto propagate_upwards = [this](Object* message) {
			Object* current = this->parent;

			while (current != nullptr)
			{
				current->OnExitHierarchy(message);
				current = current->parent;
			}
			};

		this->CallRecursively([propagate_upwards](Object* child) {
			propagate_upwards(child);
			});

		std::erase(parent->children, this);

		this->parent_matrix = Matrix4(1.0f);

		if (parent->enabled_hierarchy != this->enabled_hierarchy && this->enabled_self)
			this->On_Change_enabled(parent->enabled_hierarchy);
	}

	if (newParent != nullptr) {
		this->CallRecursively([newParent, this](Object* child) {
			newParent->OnEnterHierarchy(child);
			});
		newParent->children.push_back(this);

		this->parent_matrix = newParent->World().GetMatrix();
	}

	this->parent = newParent;

	if (!this->isDestroyQueued) { //skip updators because they might access invalid memory
		OnLocalTransformationChange(TransformChangeType::ALL);
		OnExternalTransformationChange(TransformChangeType::ALL, this->parent_matrix);
	}
}

gbe::Object* gbe::Object::GetChildAt(size_t i)
{
	auto start = this->children.begin();

	for (int count = 0; count < i; count++)
		++start;

	return *start;
}

size_t gbe::Object::GetChildCount()
{
	return this->children.size();
}

gbe::editor::InspectorData* gbe::Object::GetInspectorData()
{
	return this->inspectorData;
}

void gbe::Object::Destroy()
{
	this->isDestroyQueued = true;

	valid_objects.insert_or_assign(this->id, nullptr);

	for (const auto& child : this->children)
	{
		child->Destroy();
	}
}

bool gbe::Object::get_isDestroyed()
{
	return this->isDestroyQueued;
}

void gbe::Object::CallRecursively(std::function<void(Object*)> action, bool bottom_up)
{
	if (!bottom_up)
		action(this);

	size_t childcount = GetChildCount();
	for (size_t i = 0; i < childcount; i++)
	{
		this->GetChildAt(i)->CallRecursively(action);
	}

	if(bottom_up)
		action(this);
}

gbe::SerializedObject gbe::Object::Serialize() {
	std::vector<SerializedObject> serialized_children;
	for (const auto child : this->children)
	{
		if (child->GetEditorFlag(Object::SERIALIZABLE))
			serialized_children.push_back(child->Serialize());
	}

	auto euler_rot = this->local.rotation.Get().ToEuler();

	return {
		.type = typeid(*this).name(),
		.enabled = this->enabled_self,
		.local_position = { this->local.position.Get().x, this->local.position.Get().y, this->local.position.Get().z },
		.local_scale = { this->local.scale.Get().x, this->local.scale.Get().y, this->local.scale.Get().z },
		.local_euler_rotation = { euler_rot.x, euler_rot.y, euler_rot.z },
		.children = serialized_children
	};
}

gbe::Object::Object(gbe::SerializedObject* data, bool load_children):
	local(Transform([this](TransformChangeType type) {this->OnLocalTransformationChange(type); })),
	world([](TransformChangeType type) {}) 
{
	General_init();

	this->local.position.Set(Vector3(data->local_position[0], data->local_position[1], data->local_position[2]));
	this->local.scale.Set(Vector3(data->local_scale[0], data->local_scale[1], data->local_scale[2]));
	this->local.rotation.Set(Quaternion::Euler(Vector3(data->local_euler_rotation[0], data->local_euler_rotation[1], data->local_euler_rotation[2])));
	
	if(load_children)
		this->LoadChildren(data);
}

void gbe::Object::LoadChildren(SerializedObject* data)
{
	for (size_t i = 0; i < data->children.size(); i++)
	{
		const auto& child = &data->children[i];
		auto new_child = gbe::TypeSerializer::Instantiate(child->type, child);
		new_child->PushEditorFlag(Object::SERIALIZABLE);

		if (new_child != nullptr) {
			new_child->SetParent(this);
		}
		else {
			data->children.erase(data->children.begin() + i);
			i--;
		}
	}
	std::function<void(gbe::SerializedObject* data, Object* obj)>* _commit_enabled;
	std::function<void(gbe::SerializedObject* data, Object* obj)> commit_enabled = [&](gbe::SerializedObject* data, Object* obj) {
		for (size_t i = 0; i < data->children.size(); i++)
		{
			(*_commit_enabled)(&data->children[i], obj->GetChildAt(i));
		}

		obj->Set_enabled(data->enabled);
		};

	_commit_enabled = &commit_enabled;

	(*_commit_enabled)(data, this);
}