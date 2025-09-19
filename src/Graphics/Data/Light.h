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

        inline Matrix4 GetViewMatrix() {
            switch (type) {
            case DIRECTIONAL: {
                // A directional light has no position, so we set a distant eye position
                // and look back towards the origin along its direction.
                Vector3 eye = Vector3(0.0f, 0.0f, 0.0f) - direction * 10.0f;
                Vector3 target = Vector3(0.0f, 0.0f, 0.0f);
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
                // Use an orthographic projection for a directional light.
                // The bounds should cover the relevant part of the scene.
                // These values need to be carefully tuned based on your scene.
                float orthoSize = 100.0f;
                float nearPlane = 0.1f;
                float farPlane = 100.0f;
                return glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
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