#pragma once

namespace gbe::gfx {
    struct Light {
        enum LightType {
            DIRECTIONAL = 0,
            SPOT = 1,
            POINT = 2
        };

        alignas(16) Vector3 position;       // For point/spot lights
        alignas(16) Vector3 direction;      // For directional/spot lights
        alignas(16) Vector3 color;
        LightType type;

        Matrix4 cam_view;
        Matrix4 cam_proj;

        std::vector<Vector4> frustrum_corners;
        Vector3 frustrum_center;

        inline void UpdateContext(Matrix4 _cam_view, Matrix4 _cam_proj) {
            cam_view = _cam_view;
            cam_proj = _cam_proj;

            frustrum_corners = Matrix4::get_frustrum_corners(cam_proj, cam_view);
            frustrum_center = Matrix4::get_frustrum_center(frustrum_corners);
        }

        inline Matrix4 GetViewMatrix() {
            switch (type) {
            case DIRECTIONAL: {
                // A directional light has no position, so we set a distant eye position
                // and look back towards the origin along its direction.
                auto backtrack_dist = 20.0f;

                Vector3 eye = frustrum_center - (direction * backtrack_dist);
                Vector3 target = frustrum_center;
                Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
                return lookAt(eye, target, up);
            }
            case SPOT: {
                // A spot light has a position and a direction.
                Vector3 target = position + direction;
                Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
                return lookAt(position, target, up);
            }
            case POINT:
                throw new std::runtime_error("Incompatible.");
            }
            return Matrix4();
        }

        inline Matrix4 GetProjectionMatrix() {
            switch (type) {
            case DIRECTIONAL: {
                auto overshoot_dist = 100.0f;

                float minX = std::numeric_limits<float>::max();
                float maxX = std::numeric_limits<float>::lowest();
                float minY = std::numeric_limits<float>::max();
                float maxY = std::numeric_limits<float>::lowest();
                float maxZ = std::numeric_limits<float>::lowest();
                for (const auto& v : frustrum_corners)
                {
                    const auto trf = GetViewMatrix() * v;
                    minX = std::min(minX, trf.x);
                    maxX = std::max(maxX, trf.x);
                    minY = std::min(minY, trf.y);
                    maxY = std::max(maxY, trf.y);
                    maxZ = std::max(maxZ, trf.z);
                }

                return glm::ortho(minX, maxX, minY, maxY, 0.0f, maxZ + overshoot_dist);
            }
            case SPOT: {
                // Use a perspective projection for a spot light.
                float fov = glm::radians(45.0f); // Or based on your light's properties
                float aspect = 1.0f; // Shadow map is typically square
                float nearPlane = 0.1f;
                float farPlane = 100.0f; // Based on light's range
                return glm::perspective(fov, aspect, nearPlane, farPlane);
            }
            case POINT: {
                throw new std::runtime_error("Incompatible.");
            }
            }
            return Matrix4();
        }
    };
}