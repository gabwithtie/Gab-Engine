#pragma once

#include <vector>
#include "util.h"
#include "Shader.h"
#include "Texture.h"
#include "DrawCall.h"
#include "Window.h"
#include "Skybox.h"
#include "Light.h"
#include "Framebuffer.h"

#include <atomic>
#include <future>

namespace gde {
	using namespace rendering;

	class RenderPipeline {
	private:
		std::atomic_bool is_running_concurrently = false;
		std::future<void> current_task;

		Shader* depthShader;
		Framebuffer mFrameBuffer;
		Framebuffer mDepthFrameBuffer;

		glm::vec3 from;
		glm::mat4 viewMat;
		glm::mat4 projMat;
		Shader* postprocess;

		std::vector<DrawCall*> drawcalls;

		std::list<rendering::Light*> lights_this_frame;
		int maxlights = 10;

		Skybox* mSkybox;
	public:
		RenderPipeline(glm::vec2);
		void RegisterDrawCall(DrawCall*);

		void SetMaximumLights(int maxlights);
		void SetView(glm::vec3 from, glm::mat4 viewMat, glm::mat4 projMat);
		void SetPostProcessing(Shader* postprocess);
		bool TryPushLight(rendering::Light* data, bool priority = false);

		void RenderFrame();

		glm::mat4 GetProjMat();

		void CleanUp();
	};
}