#include "RenderPipeline.h"

// Update includes
#include <stdexcept>
#include "Editor/gbe_editor.h" 

// Helper to initialize bgfx for SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL_syswm.h>

#include "bgfx-gab/forwardrenderer.h"

using namespace gbe;

gbe::RenderPipeline* gbe::RenderPipeline::Instance;

// BGFX: Simplified constructor
gbe::RenderPipeline::RenderPipeline(gbe::Window& window, Vector2Int dimensions) :
	window(window)
{
	std::cout << "[RENDERER] Initializing..." << std::endl;

	if (this->Instance != nullptr)
		throw std::runtime_error("RenderPipeline instance already exists!");

	this->Instance = this;

	this->window = window;
	this->screen_resolution = dimensions;
	this->viewport_resolution = dimensions; // Initialize to full screen

	//===================BGFX INIT==================
	bgfx::Init init;
	// Use platform data from SDL
	SDL_Window* implemented_window = static_cast<SDL_Window*>(window.Get_implemented_window());
	
#if !BX_PLATFORM_EMSCRIPTEN
	SDL_SysWMinfo wmi;
	SDL_VERSION(&wmi.version);
	if (!SDL_GetWindowWMInfo(implemented_window, &wmi)) {
		throw new std::runtime_error("SDL_SysWMinfo could not be retrieved. SDL_Error: %s\n" + std::string(SDL_GetError()));
	}
	bgfx::renderFrame(); // single threaded mode
#endif // !BX_PLATFORM_EMSCRIPTEN

	bgfx::PlatformData pd{};
#if BX_PLATFORM_WINDOWS
	pd.nwh = wmi.info.win.window;
#elif BX_PLATFORM_OSX
	pd.nwh = wmi.info.cocoa.window;
#elif BX_PLATFORM_LINUX
	pd.ndt = wmi.info.x11.display;
	pd.nwh = (void*)(uintptr_t)wmi.info.x11.window;
#elif BX_PLATFORM_EMSCRIPTEN
	pd.nwh = (void*)"#canvas";
#endif // BX_PLATFORM_WINDOWS ? BX_PLATFORM_OSX ? BX_PLATFORM_LINUX ?
	// BX_PLATFORM_EMSCRIPTEN

	bgfx::Init bgfx_init;
	bgfx_init.type = bgfx::RendererType::Vulkan; // auto choose renderer
	bgfx_init.resolution.width = dimensions.x;
	bgfx_init.resolution.height = dimensions.y;
	bgfx_init.resolution.reset = BGFX_RESET_VSYNC;
	bgfx_init.platformData = pd;
	bgfx::init(bgfx_init);

	// Set main view to be the whole window/viewport
	bgfx::setViewClear(
		0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x6495EDFF, 1.0f, 0);
	bgfx::setViewRect(0, 0, 0, dimensions.x, dimensions.y);
	bgfx::setDebug(BGFX_DEBUG_TEXT);

	// Asset Loaders (no change, but their implementation now uses bgfx handles)
	this->shaderloader.AssignSelfAsLoader();
	this->meshloader.AssignSelfAsLoader();
	this->materialloader.AssignSelfAsLoader();
	this->textureloader.AssignSelfAsLoader();

	auto layout_stride = s_VERTEXLAYOUT.getStride();
	auto vertexstruct_size = sizeof(asset::data::Vertex);
	assert(layout_stride == vertexstruct_size && "BGFX Layout stride must match Vertex struct size!");
	
	//SETUP RENDERER
	this->cur_renderer = new bgfx_gab::ForwardRenderer(this->currentrenderinfo);

	ReloadFrame();
}

gbe::RenderPipeline::~RenderPipeline()
{
	delete(this->cur_renderer);

	bgfx::shutdown();
}

// BGFX: Function to create main and shadow frame buffers
void gbe::RenderPipeline::ReloadFrame()
{
	auto mainpass_tex = this->cur_renderer->ReloadFrame(this->viewport_resolution);
	TextureLoader::RegisterExternal("mainpass", mainpass_tex);
}

void gbe::RenderPipeline::DrawLine(Vector3 a, Vector3 b)
{
	Instance->currentrenderinfo.lines_this_frame.push_back({
			.pos = a
		});
	Instance->currentrenderinfo.lines_this_frame.push_back({
			.pos = b
		});
}

// BGFX: RenderFrame adapted to use bgfx submission model
void gbe::RenderPipeline::RenderFrame(const SceneRenderInfo& frameinfo)
{
	if (window.isMinimized())
		return;

	// BGFX: Handle resolution change / FBO recreation
	if (!handled_resolution_change) {
		bgfx::reset((uint16_t)screen_resolution.x, (uint16_t)screen_resolution.y, BGFX_RESET_VSYNC);
		ReloadFrame(); // Recreate FBOs with new dimensions
		handled_resolution_change = true;
	}

	this->cur_renderer->RenderFrame(frameinfo, this->currentrenderinfo);

	//EDITOR/GUI PASS [VIEW_EDITOR_PASS]
	// The editor/GUI is rendered last to the screen.
	if (editor != nullptr) {
		this->editor->RenderPass();
	}

	// BGFX: Present the frame
	this->currentrenderinfo.frame_id = bgfx::frame();
}

gbe::gfx::DrawCall* gbe::RenderPipeline::RegisterDrawCall(asset::Mesh* mesh, asset::Material* material)
{
	for (const auto& pair : Instance->currentrenderinfo.callgroups)
	{
		const auto& drawcall = pair.first;

		if (drawcall->get_mesh() == mesh && drawcall->get_material() == material) {
			return drawcall;
		}
	}

	auto newdrawcall = new DrawCall(mesh, material, &Instance->shaderloader.GetAssetRuntimeData(material->Get_load_data().shader->Get_assetId()));

	return newdrawcall;
}

DrawCall* gbe::RenderPipeline::RegisterDefaultDrawCall(asset::Mesh* mesh, asset::Material* material)
{
	Instance->default_drawcall = RegisterDrawCall(mesh, material);

	return Instance->default_drawcall;
}

DrawCall* gbe::RenderPipeline::GetDefaultDrawCall()
{
	return Instance->default_drawcall;
}

gbe::Matrix4* gbe::RenderPipeline::RegisterInstance(uint32_t instance_id, DrawCall* drawcall, Matrix4 matrix)
{
	//COMMITTING
	Instance->currentrenderinfo.infomap.insert_or_assign(
		instance_id,
		GraphicsRenderInfo::InstanceInfo{
			.transform = matrix,
			.drawcall = drawcall,
			.rendergroups = {
				{
					drawcall->get_material()->Get_load_data().defaultrendergroup, true
				}
			}
		});

	auto drawcall_it = Instance->currentrenderinfo.callgroups.find(drawcall);
	if (drawcall_it == Instance->currentrenderinfo.callgroups.end()) {
		Instance->currentrenderinfo.callgroups.insert_or_assign(
			drawcall,
			std::vector<uint32_t>{ instance_id }
		);
	}
	else {
		drawcall_it->second.push_back(instance_id);
	}

	return &Instance->currentrenderinfo.infomap[instance_id].transform;
}

void gbe::RenderPipeline::RegisterAdditionalGroup(uint32_t instance_id, int rendergroup)
{
	auto it = Instance->currentrenderinfo.infomap.find(instance_id);

	if (it == Instance->currentrenderinfo.infomap.end())
		return;

	it->second.rendergroups.insert_or_assign(rendergroup, true);
}

void gbe::RenderPipeline::UnRegisterInstanceGroup(uint32_t instance_id, int rendergroup)
{
	auto info_it = Instance->currentrenderinfo.infomap.find(instance_id);

	if (info_it == Instance->currentrenderinfo.infomap.end())
		return;

	auto& renderinfo = info_it->second;

	if (rendergroup == renderinfo.drawcall->get_material()->Get_load_data().defaultrendergroup)
		return;

	renderinfo.rendergroups.erase(rendergroup);
}

void gbe::RenderPipeline::UnRegisterInstanceAll(uint32_t instance_id)
{
	auto info_it = Instance->currentrenderinfo.infomap.find(instance_id);

	if (info_it == Instance->currentrenderinfo.infomap.end())
		return;

	auto& renderinfo = info_it->second;


	auto& callgroup = Instance->currentrenderinfo.callgroups[renderinfo.drawcall];

	callgroup.erase(
		std::remove(callgroup.begin(), callgroup.end(), instance_id),
		callgroup.end()
	);

	Instance->currentrenderinfo.infomap.erase(instance_id);

}

void gbe::RenderPipeline::SetEnableInstance(uint32_t instance_id, bool value)
{
	auto info_it = Instance->currentrenderinfo.infomap.find(instance_id);

	if (info_it == Instance->currentrenderinfo.infomap.end())
		return;

	auto& renderinfo = info_it->second;
	renderinfo.enabled = value;
}
