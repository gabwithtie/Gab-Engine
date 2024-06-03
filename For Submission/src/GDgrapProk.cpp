#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#include <GD_Graphics/util.h>
#include <GD_Graphics/Shader.h>
#include <GD_Graphics/Texture.h>
#include <GD_Graphics/Window.h>
#include <GD_Graphics/Camera.h>
#include <GD_Graphics/Skybox.h>
#include <GD_Graphics/Light.h>
#include <GD_Graphics/Framebuffer.h>
#include <GD_Graphics/RenderPipeline.h>
#include <GD_Graphics/Mesh.h>

#include <GD_Engine/PhysicsPipeline.h>
#include <GD_Engine/Vector.h>
#include <GD_Engine/Time.h>

using namespace gde;

static Window* mWindow;

int main(void)
{
    auto mTime = new Time();

    /* Initialize GLFW*/
    if (!glfwInit())
        return -1;

    mWindow = new Window(700, 700);
    
    /* Initialize GLAD*/
    gladLoadGL();

#pragma region Rendering Requisites Setup
    //Shaders setup
    auto depthShader = new Shader("Shaders/object.vert", "Shaders/depth.frag");
    auto litShader = new Shader("Shaders/object.vert", "Shaders/lit.frag");
    auto unlitShader = new Shader("Shaders/object.vert", "Shaders/unlit.frag");
    auto Cam3rdPPShader = new Shader("Shaders/camshader.vert", "Shaders/camshader.frag");
    auto Cam1stPPShader = new Shader("Shaders/camshader.vert", "Shaders/camshader.frag");
    auto CamOrthoPPShader = new Shader("Shaders/camshader.vert", "Shaders/camshader.frag");

    //Camera setup
    auto mCameraOrtho = new OrthographicCamera(mWindow, CamOrthoPPShader);
    glUseProgram(CamOrthoPPShader->shaderID);
    glUniform1f(glGetUniformLocation(CamOrthoPPShader->shaderID, "saturation"), 1.0f);
    glUniform4fv(glGetUniformLocation(CamOrthoPPShader->shaderID, "tint"), 1, glm::value_ptr(glm::vec4(1, 1, 1, 1)));
    mCameraOrtho->orthoRange = 700;
    mCameraOrtho->farClip = 2000.0f;
    mCameraOrtho->cameraPos = glm::vec3(0, 0, -1000);
    mCameraOrtho->CamF = glm::vec3(0, 0, 1);
    mCameraOrtho->WorldUp = glm::vec3(0, 1, 0);

    //RenderPipeline setup
    auto mRenderPipeline = new RenderPipeline(glm::vec2(mWindow->win_x, mWindow->win_y), mCameraOrtho);
    //PhysicsPipeline setup
    auto mPhysicsPipeline = new PhysicsPipeline();
#pragma endregion

#pragma region Object setup
    //sphere setup
    auto makesphere = [mRenderPipeline, mPhysicsPipeline, unlitShader](std::string name, Vector3 pos, float speed, float accel, glm::vec3 color = glm::vec3(1, 0, 0)) {
        auto sphere_mesh = new Mesh("3D/sphere.obj");
        auto sphere_material = new Material(unlitShader);
        sphere_material->setOverride<glm::vec3>("color", color);
        auto sphere_drawcall = new DrawCall(sphere_mesh, sphere_material);
        mRenderPipeline->RegisterDrawCall(sphere_drawcall);

        auto sphere_object = new Object(name);
        sphere_object->scale = Vector3(1, 1, 1) * 5;
        sphere_object->position = pos;
        sphere_object->mDrawCall = sphere_drawcall;
        mPhysicsPipeline->Register(sphere_object);

        //PROGRAMMING CHALLENGE 1 CODE
        auto dir = (-sphere_object->position).Normalized();
        sphere_object->velocity = dir * speed;
        sphere_object->acceleration = dir * accel;

        return sphere_object;
    };

    std::vector<Object*> spheres;

    spheres.push_back(makesphere("Red", Vector3(-700, 700, 201), 80, 14.5, glm::vec3(1, 0, 0)));
    spheres.push_back(makesphere("Green", Vector3(700, 700, 173), 90, 8, glm::vec3(0, 1, 0)));
    spheres.push_back(makesphere("Blue", Vector3(700, -700, -300), 130, 1, glm::vec3(0, 0, 1)));
    spheres.push_back(makesphere("Yellow", Vector3(-700, -700, -150), 110, 3, glm::vec3(1, 1, 0)));

    std::vector<Object*> balls_finished;
    float total_time = 0;

#pragma endregion

    /// <summary>
    /// MAIN GAME LOOP
    /// </summary>

    while (!glfwWindowShouldClose(mWindow->window))
    {
        //Update pipeline
        mRenderPipeline->RenderFrame();

        //Update window
        glfwSwapBuffers(mWindow->window);
        /* Poll for and process events */
        glfwPollEvents();

        double fixedTickDur = 0;

        if (mTime->TickFixed(&fixedTickDur)) {
            mPhysicsPipeline->Update(fixedTickDur);
        }

        total_time += fixedTickDur;

        for (auto obj : spheres)
        {
            bool alrFinished = false;

            if (obj->position.Dot(obj->velocity) <= 0)
                continue;
            for (auto ball_finished : balls_finished)
            {
                if (ball_finished != obj)
                    continue;

                alrFinished = true;
            }
            if (alrFinished)
                continue;

            auto final_vel = obj->velocity;
            obj->velocity = Vector3(0, 0, 0);
            obj->acceleration = Vector3(0, 0, 0);

            balls_finished.push_back(obj);
            std::string position_label = "";

            switch (balls_finished.size())
            {
            case 1:
                position_label = "1st";
                break;
            case 2:
                position_label = "2nd";
                break;
            case 3:
                position_label = "3rd";
                break;
            case 4:
                position_label = "4th";
                break;
            }

            auto vel_mag = final_vel.Magnitude();
            auto vel_ave = final_vel * (1 / total_time);

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

            std::cout << obj->get_name() << ": " << position_label << std::endl;
            std::cout << "Mag. of velocity: " << get_rounded(vel_mag) << std::endl;
            std::cout << "Average velocity: " << vel_ave.ToString() << std::endl;
            std::cout << get_rounded(total_time) << "secs" << std::endl << std::endl;
        }
    }
    
    mRenderPipeline->CleanUp();

    glfwTerminate();
}