#pragma once

#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace gbe {
	struct Matrix4 : public glm::mat4x4 {
		Matrix4();
		Matrix4(const glm::mat4x4& from);

		inline Matrix4 Inverted() {
			return glm::inverse(*this);
		}

		inline Matrix4(float model_mat[16]) : Matrix4(glm::make_mat4(model_mat)) {

		}

		inline bool isfinite() const {
			for (int i = 0; i < 4; i++)
				for (int j = 0; j < 4; j++)
					if (!std::isfinite((*this)[i][j]))
						return false;
			return true;
		}

		const float* Get_Ptr();
	};
}