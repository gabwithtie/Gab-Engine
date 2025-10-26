#pragma once

namespace gbe::gfx {
    struct Light {
        enum LightType {
            DIRECTIONAL = 0,
            CONE = 1,
            POINT = 2
        };

        //==============RUNTIME DATA
        Vector3 position;
        Vector3 direction;
        Matrix4 cam_view;
        Matrix4 cam_proj;

        //==============ATTRIBUTES
        Vector3 color;
        LightType type;

        //Directional
        float override_dist = 50;
        float dir_backtrack_dist = 60;
        std::vector<Vector4> frustrum_corners;
        Vector3 frustrum_center;

        //Cone
        float angle_inner = 50;
        float angle_outer = 80;
        float near_clip = 1;
        float range = 15;
        bool square_project = false;

        //BIAS
        float bias_min = 0.005;
        float bias_mult = 0.05;
        
        //===============CACHE
        bool created_context_view = false;
        bool created_context_proj = false;

        Matrix4 proj_cache;
        Matrix4 view_cache;

        inline void UpdateContext(Matrix4 _cam_view, Matrix4 _cam_proj) {
            cam_view = _cam_view;
            cam_proj = _cam_proj;

            created_context_view = false;
            created_context_proj = false;

            if(type != DIRECTIONAL)
				return;

            frustrum_corners = Matrix4::get_frustrum_corners(cam_proj, cam_view);
            frustrum_center = Matrix4::get_frustrum_center(frustrum_corners);

            float radius = 0.0f;
            for (uint32_t j = 0; j < 8; j++) {
                Vector3 delta = (Vector3)frustrum_corners[j] - frustrum_center;
                float distance = delta.Magnitude();
                radius = glm::max(radius, distance);
            }
            radius = std::ceil(radius * 16.0f) / 16.0f;

            glm::vec3 maxExtents = glm::vec3(radius);
            glm::vec3 minExtents = -maxExtents;

            created_context_view = true;
            created_context_proj = true;

            range = radius * 2;
            view_cache = glm::lookAt(frustrum_center - (direction * dir_backtrack_dist), frustrum_center, glm::vec3(0.0f, 1.0f, 0.0f));
            proj_cache = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, range + dir_backtrack_dist);
            near_clip = 0;
        }

        inline Matrix4 GetViewMatrix() {
            if (created_context_view)
                return view_cache;

            switch (type) {
            case CONE: {
                // A spot light has a position and a direction.
                Vector3 target = position + (direction * range);
                Vector3 up = Vector3(0.0f, 1.0f, 0.0f);

                view_cache = lookAt(position, target, up);
                break;
            }
            case POINT:
                view_cache = Matrix4(1);
            }
            
            created_context_view = true;
            return view_cache;
        }

        inline Matrix4 GetProjectionMatrix() {
            if (created_context_proj)
                return proj_cache;

            switch (type) {
            case CONE: {
                // Use a perspective projection for a spot light.
                float fov = glm::radians(angle_outer); // Or based on your light's properties
                float aspect = 1.0f; // Shadow map is typically square
                float nearPlane = near_clip;
                float farPlane = range; // Based on light's range
                proj_cache = glm::perspective(fov, aspect, nearPlane, farPlane);
                break;
            }
            case POINT: {
                proj_cache = Matrix4(1);
                break;
            }
            }

            created_context_proj = true;
            return proj_cache;
        }
    };
}