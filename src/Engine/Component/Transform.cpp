#include "Transform.h"

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

gbe::Transform::Transform()
{
	this->Right = Vector3(1, 0, 0);
	this->Up = Vector3(0, 1, 0);
	this->Forward = Vector3(0, 0, 1);

	this->scale.Get() = Vector3(1.0f);
	this->position.Get() = Vector3::zero;
	this->rotation.Get() = Quaternion();

	OnComponentChange(TransformChangeType::ALL, true);
}

void gbe::Transform::Reset() {
	this->scale.Get() = Vector3(1.0f);
	this->position.Get() = Vector3::zero;
	this->rotation.Get() = Quaternion();

	OnComponentChange(TransformChangeType::ALL, true);
}

gbe::Matrix4 gbe::Transform::GetMatrix(bool include_scale) const
{
	if(include_scale)
		return this->updated_matrix_with_scale;
	else
		return this->updated_matrix_without_scale;
}

void gbe::Transform::OnComponentChange(TransformChangeType value, bool silent)
{
	auto newmat = Matrix4();
	newmat = glm::translate(newmat, this->position.Get());
	newmat *= glm::toMat4(this->rotation.Get());

	this->updated_matrix_without_scale = newmat;


	glm::mat4 shearMatrix(1.0f);
	// shear of the X-axis along the Y-axis
	shearMatrix[1][0] = this->Skew.z;
	// shear of the X-axis along the Z-axis
	shearMatrix[2][0] = this->Skew.y;
	// shear of the Y-axis along the Z-axis
	shearMatrix[2][1] = this->Skew.x;

	newmat *= shearMatrix;
	newmat = glm::scale(newmat, this->scale.Get());

	this->updated_matrix_with_scale = newmat;

	if(!this->updated_matrix_without_scale.isfinite())
		throw std::runtime_error("NAN transform matrix generated.");
	if (!this->updated_matrix_with_scale.isfinite())
		throw std::runtime_error("NAN transform matrix generated.");

	if (value == TransformChangeType::ROTATION)
		this->UpdateAxisVectors();

	if (!silent && this->onChange)
		this->onChange(value);
}

const gbe::Vector3& gbe::Transform::GetRight()
{
	return this->Right;
}

const gbe::Vector3& gbe::Transform::GetUp()
{
	return this->Up;
}
const gbe::Vector3& gbe::Transform::GetForward()
{
	return this->Forward;
}

void gbe::Transform::UpdateAxisVectors()
{
	auto newbasismat = glm::toMat4(this->rotation.Get());
	this->Right= (Vector3)newbasismat[0];
	this->Up=(Vector3)newbasismat[1];
	this->Forward=(Vector3)newbasismat[2];
}

gbe::Transform::Transform(std::function<void(TransformChangeType)> func) : gbe::Transform::Transform() {
	this->onChange = func;
}

void gbe::Transform::SetMatrix(Matrix4 mat, bool silent) {

	Vector3 _scale;
	Vector3 _position;
	Quaternion _rotation;
	Vector3 _skew;

	Vector4 perspective;

	glm::decompose(mat, _scale, _rotation, _position, _skew, perspective);

	if (silent) {
		this->scale.Get() = _scale;
		this->position.Get() = _position;
		this->rotation.Get() = _rotation;
		this->Skew = _skew;

		UpdateAxisVectors();
		OnComponentChange(TransformChangeType::ALL, silent);
	}
	else {
		this->scale.Set(_scale);
		this->position.Set(_position);
		this->rotation.Set(_rotation);
		this->Skew = _skew;

		OnComponentChange(TransformChangeType::ALL);
	}
}

gbe::Transform::Transform(Matrix4 mat) {

	Vector3 _scale;
	Vector3 _position;
	Quaternion _rotation;

	Vector3 skew;
	Vector4 perspective;

	glm::decompose(mat, _scale, _rotation, _position, skew, perspective);

	this->scale.Get() = _scale;
	this->position.Get() = _position;
	this->rotation.Get() = _rotation;

	UpdateAxisVectors();
	OnComponentChange(TransformChangeType::ALL, true);
}