#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <math.h>


#include "Vector.h"

namespace gde {
	Vector3::Vector3()
	{
		this->x = 0;
		this->y = 0;
		this->z = 0;
	}

	gde::Vector3::Vector3(float x, float y, float z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

	float gde::Vector3::SqrMagnitude()
	{
		auto result = 0;

		for (auto component : components)
		{
			result += component * component;
		}

		return result;
	}

	float gde::Vector3::Magnitude()
	{
		return sqrtf(this->SqrMagnitude());
	}

	gde::Vector3 gde::Vector3::Normalized()
	{
		Vector3 result = Vector3(*this);
		return result * (1 / this->Magnitude());
	}

	Vector3 gde::Vector3::ComponentMult(const Vector3& right)
	{
		Vector3 result = Vector3(*this);

		for (int i = 0; i < VECTORSIZE; i++)
		{
			result.components[i] *= right.components[i];
		}

		return result;
	}
	float Vector3::Dot(const Vector3& right)
	{
		float result = 0;

		for (int i = 0; i < VECTORSIZE; i++)
		{
			result += this->components[i] * right.components[i];
		}

		return result;
	}
	Vector3 Vector3::Cross(const Vector3& right)
	{
		Vector3 result = Vector3(0, 0, 0);

		result.x = (this->y * right.z) - (this->z * right.y);
		result.y = (this->z * right.x) - (this->x * right.z);
		result.z = (this->x * right.y) - (this->y * right.x);

		return result;
	}
	void Vector3::operator=(const Vector3& right)
	{
		for (int i = 0; i < VECTORSIZE; i++)
		{
			this->components[i] = right.components[i];
		}
	}
	void Vector3::operator+=(const Vector3& right)
	{
		for (int i = 0; i < VECTORSIZE; i++)
		{
			this->components[i] += right.components[i];
		}
	}
	void Vector3::operator-=(const Vector3& right)
	{
		for (int i = 0; i < VECTORSIZE; i++)
		{
			this->components[i] -= right.components[i];
		}
	}

	Vector3 Vector3::operator-()
	{
		Vector3 result = Vector3(*this);
		result *= -1;
		return result;
	}

	Vector3 Vector3::operator*(const float& right)
	{
		Vector3 result = Vector3(*this);
		result *= right;
		return result;
	}

	Vector3 Vector3::operator+(const Vector3& right)
	{
		Vector3 result = Vector3(*this);
		result += right;
		return result;
	}
	Vector3 Vector3::operator-(const Vector3& right)
	{
		Vector3 result = Vector3(*this);
		result -= right;
		return result;
	}

	std::string Vector3::ToString()
	{
		std::string tostring = "";

		auto get_rounded = [](double value) {
			auto decimals = 2;

			std::ostringstream ss;
			ss << std::fixed << std::setprecision(decimals) << value;
			std::string s = ss.str();
			if (decimals > 0 && s[s.find_last_not_of('0')] == '.') {
				s.erase(s.size() - decimals + 1);
			}
			return s;
		};

		for (int i = 0; i < VECTORSIZE; i++)
		{
			tostring += get_rounded(this->components[i]);

			if (i + 1 < VECTORSIZE) {
				tostring += " , ";
			}
		}

		return tostring;
	}

}