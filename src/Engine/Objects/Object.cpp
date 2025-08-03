#include "Object.h"
#include "Root.h"
#include "Engine/Serialization/TypeSerializer.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include "Editor/gbe_editor.h"

void gbe::Object::MatToTrans(Transform* target, Matrix4 mat)
{
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(mat, scale, rotation, translation, skew, perspective);

	target->position.Set(translation);
	target->rotation.Set(rotation);
	target->scale.Set(scale);
}

void gbe::Object::OnLocalTransformationChange(TransformChangeType changetype)
{
	auto worldmat = this->parent_matrix * this->local.GetMatrix();

	for (auto child : this->children)
	{
		child->OnExternalTransformationChange(changetype, worldmat);
	}

	MatToTrans(&this->world, worldmat);

	if (isnan(this->local.GetMatrix()[0][0])) {
		std::cout << "NAN transform" << std::endl;
		this->MatToTrans(&this->local, Matrix4(1.0f));
	}
}

void gbe::Object::OnExternalTransformationChange(TransformChangeType changetype, Matrix4 newparentmatrix)
{
	this->parent_matrix = newparentmatrix;
	auto worldmat = this->parent_matrix * this->local.GetMatrix();

	for (auto child : this->children)
	{
		child->OnExternalTransformationChange(changetype, worldmat);
	}

	MatToTrans(&this->world, worldmat);
}

gbe::Object::Object()
{
	this->parent_matrix = Matrix4(1.0f);
	this->parent = nullptr;

	OnLocalTransformationChange(TransformChangeType::ALL);
	OnExternalTransformationChange(TransformChangeType::ALL, this->parent_matrix);

	//INSPECTOR DATA
	inspectorData = new editor::InspectorData();
}

gbe::Object::~Object(){
	for (const auto& child : this->children)
	{
		delete child;
	}
}

gbe::Transform& gbe::Object::World()
{
	return this->world;
}

gbe::Transform& gbe::Object::Local()
{
	return this->local;
}

gbe::Matrix4 gbe::Object::GetWorldMatrix(bool include_local_scale)
{
	return this->parent_matrix * this->Local().GetMatrix(include_local_scale);
}

void gbe::Object::SetLocalMatrix(Matrix4 mat)
{
	MatToTrans(&this->local, mat);

	OnLocalTransformationChange(TransformChangeType::ALL);
}

void gbe::Object::SetWorldPosition(Vector3 vector)
{
	auto world_pos = this->World().position.Get();

	this->TranslateWorld(-world_pos);
	this->TranslateWorld(vector);
}

void gbe::Object::TranslateWorld(Vector3 vector)
{
	auto curmat = this->local.GetMatrix();

	auto curloc = glm::vec3(curmat[3]);
	curloc += (glm::vec3)vector;
	auto newrow4 = glm::vec4(curloc, curmat[3][3]);

	curmat[3] = newrow4;

	MatToTrans(&this->local, curmat);

	OnLocalTransformationChange(TransformChangeType::TRANSLATION);
}

void gbe::Object::OnEnterHierarchy(Object* newChild)
{
	Object* current = this->parent;

	while (current != nullptr)
	{
		current->OnEnterHierarchy(newChild);
		current = current->parent;
	}
}

void gbe::Object::OnExitHierarchy(Object* newChild)
{
	
}

gbe::Object* gbe::Object::GetParent()
{
	return this->parent;
}

void gbe::Object::SetParent(Object* newParent)
{
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

		parent->children.remove_if([this](Object* child) {return child == this; });

		this->parent_matrix = Matrix4(1.0f);
	}

	if (newParent != nullptr) {
		this->CallRecursively([newParent, this](Object* child) {
			newParent->OnEnterHierarchy(child);
			});
		newParent->children.push_back(this);

		this->parent_matrix = newParent->GetWorldMatrix();
	}

	this->parent = newParent;

	OnLocalTransformationChange(TransformChangeType::ALL);
	OnExternalTransformationChange(TransformChangeType::ALL, this->parent_matrix);
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
}

bool gbe::Object::get_isDestroyed()
{
	return this->isDestroyQueued;
}

void gbe::Object::CallRecursively(std::function<void(Object*)> action)
{
	size_t childcount = GetChildCount();
	for (size_t i = 0; i < childcount; i++)
	{
		this->GetChildAt(i)->CallRecursively(action);
	}

	action(this);
}

gbe::SerializedObject gbe::Object::Serialize() {
	std::vector<SerializedObject> serialized_children;
	for (const auto child : this->children)
	{
		serialized_children.push_back(child->Serialize());
	}

	auto euler_rot = this->local.rotation.Get().ToEuler();

	return {
		.type = typeid(*this).name(),
		.local_position = { this->local.position.Get().x, this->local.position.Get().y, this->local.position.Get().z },
		.local_scale = { this->local.scale.Get().x, this->local.scale.Get().y, this->local.scale.Get().z },
		.local_euler_rotation = { euler_rot.x, euler_rot.y, euler_rot.z },
		.children = serialized_children
	};
}

void gbe::Object::Deserialize(gbe::SerializedObject data) {
	this->local.position.Set(Vector3(data.local_position[0], data.local_position[1], data.local_position[2]));
	this->local.scale.Set(Vector3(data.local_scale[0], data.local_scale[1], data.local_scale[2]));
	this->local.rotation.Set(Quaternion::Euler(Vector3(data.local_euler_rotation[0], data.local_euler_rotation[1], data.local_euler_rotation[2])));
	for (const auto& child : data.children)
	{
		auto new_child = gbe::TypeSerializer::Instantiate(child.type, child);

		if (new_child != nullptr) {
			new_child->SetParent(this);
			new_child->Deserialize(child);
		}
	}
}