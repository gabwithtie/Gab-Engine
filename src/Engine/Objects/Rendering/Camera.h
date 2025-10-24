#pragma once

#include "../../Objects/Object.h"

#include "Window/gbe_window.h"
#include "Math/gbe_math.h"
#include "Graphics/gbe_graphics.h"

namespace gbe {
    using namespace gfx;

    /// <summary>
    /// Generic camera class with assignable project matrix and utility functions for rendering.
    /// </summary>
    struct Camera : public Object {
        float nearClip;
        float farClip;

        bool moved = true;
        inline bool GetMoved() {
            return moved;
        }
        inline void OnRender() {
            moved = false;
        }

        Camera();

        Vector3 WorldUp = Vector3(0, 1, 0);

        Matrix4 GetViewMat();
        virtual Matrix4 GetProjectionMat() = 0;
        virtual Matrix4 GetProjectionMat(float override_range) = 0;
        Vector3 ScreenToRay(Vector2 normalizedscreenpos);
        Vector2 WorldToScreen(Vector3 worldpos);

        inline virtual void OnExternalTransformationChange(TransformChangeType changetype, Matrix4 newparentmatrix) override {
            Camera::moved = true;
            Object::OnExternalTransformationChange(changetype, newparentmatrix);
		}
    };

    struct OrthographicCamera : public Camera {
        float orthoRange = 50;

        // Inherited via GDCamera
        virtual Matrix4 GetProjectionMat() override;
        virtual Matrix4 GetProjectionMat(float override_range) override;
    };

    struct PerspectiveCamera : public Camera {
        float angles = 90;


        // Inherited via GDCamera
        virtual Matrix4 GetProjectionMat() override;
        virtual Matrix4 GetProjectionMat(float override_range) override;
    };
}