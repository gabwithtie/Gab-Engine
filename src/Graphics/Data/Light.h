#pragma once

namespace gbe::gfx {
    struct Light {
        enum LightType {
            DIRECTIONAL = 0,
            CONE = 1,
            POINT = 2
        };

        Vector3 position;       // For point/spot lights
        Vector3 direction;      // For directional/spot lights
        Vector3 color;
        LightType type;

        Matrix4 cam_view;
        Matrix4 cam_proj;

        //Directional
        float override_dist = 50;
        float dir_backtrack_dist = 180;
        float dir_overshoot_dist = 450;
        float bias_min = 0.0001;
        float bias_mult = 0.001;
        std::vector<Vector4> frustrum_corners;
        Vector3 frustrum_center;

        //Cone
        float angle_inner = 50;
        float angle_outer = 80;
        float range = 50;

        //CACHE
        bool created_context_view = false;
        bool created_context_proj = false;

        Matrix4 proj_cache;
        Matrix4 view_cache;

        inline void UpdateContext(Matrix4 _cam_view, Matrix4 _cam_proj) {
            cam_view = _cam_view;
            cam_proj = _cam_proj;

            frustrum_corners = Matrix4::get_frustrum_corners(cam_proj, cam_view);
            frustrum_center = Matrix4::get_frustrum_center(frustrum_corners);

            created_context_view = false;
            created_context_proj = false;
        }

        inline Matrix4 GetViewMatrix() {
            if (created_context_view)
                return view_cache;

            switch (type) {
            case DIRECTIONAL: {
                Vector3 eye = frustrum_center - (direction * dir_backtrack_dist);
                Vector3 target = frustrum_center;
                Vector3 up = Vector3(0.0f, 1.0f, 0.0f);

                view_cache = lookAt(eye, target, up);
                break;
            }
            case CONE: {
                // A spot light has a position and a direction.
                Vector3 target = position + (direction * range);
                Vector3 up = Vector3(0.0f, 1.0f, 0.0f);

                view_cache = lookAt(position, target, up);
                break;
            }
            case POINT:
                throw new std::runtime_error("Incompatible.");
            }
            
            created_context_view = true;
            return view_cache;
        }

        inline Matrix4 GetProjectionMatrix() {
            if (created_context_proj)
                return proj_cache;

            switch (type) {
            case DIRECTIONAL: {
                
                const auto viewmat = GetViewMatrix();
                float minX = std::numeric_limits<float>::max();
                float maxX = std::numeric_limits<float>::lowest();
                float minY = std::numeric_limits<float>::max();
                float maxY = std::numeric_limits<float>::lowest();
                float maxZ = std::numeric_limits<float>::lowest();
                for (const auto& v : frustrum_corners)
                {
                    const auto trf = viewmat * v;
                    minX = std::min(minX, trf.x);
                    maxX = std::max(maxX, trf.x);
                    minY = std::min(minY, trf.y);
                    maxY = std::max(maxY, trf.y);
                    maxZ = std::max(maxZ, trf.z);
                }

                proj_cache = glm::ortho(minX, maxX, minY, maxY, 0.0f, maxZ + dir_overshoot_dist);
                break;
            }
            case CONE: {
                // Use a perspective projection for a spot light.
                float fov = glm::radians(angle_outer); // Or based on your light's properties
                float aspect = 1.0f; // Shadow map is typically square
                float nearPlane = 0.5f;
                float farPlane = range; // Based on light's range
                proj_cache = glm::perspective(fov, aspect, nearPlane, farPlane);
                break;
            }
            case POINT: {
                throw new std::runtime_error("Incompatible.");
            }
            }

            created_context_proj = true;
            return proj_cache;
        }
    };
}