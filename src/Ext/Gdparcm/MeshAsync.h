#pragma once

#include "Engine/gbe_engine.h"
#include "Math/gbe_math.h"

#include <sockpp/tcp_connector.h>
#include <sockpp/exception.h>

#include <sockpp/version.h> // Include this header

#include <thread>

namespace gbe::gdparcm
{
	class MeshAsync : public Object, public Update {
	private:
		static std::vector<MeshAsync*> active_mesh_requests;

		int server_port;
		std::string client_id;

		const std::filesystem::path mesh_file_directory = "cache/models/";

		inline std::filesystem::path Get_filepath() {
			return mesh_file_directory / std::string(client_id + ".obj");
		}

		std::thread* worker_thread = nullptr;
		gbe::RenderObject* this_renderer = nullptr;

		bool shown = false;
		
	public:
		float progress = 0.0f;
		int file_size = 1;
		bool worker_done = false;
		
		inline bool Get_shown() const {
			return shown;
		}

		inline float Get_progress() const {
			return progress;
		}

		inline static void Init_client() {
			sockpp::initialize();
			std::cout << "Socket library initialized successfully." << std::endl;
		}

		static const std::vector<MeshAsync*> Get_active_mesh_requests() {
			return active_mesh_requests;
		}

		MeshAsync(int _server_port, std::string _client_id);
		inline ~MeshAsync() {
			auto it = std::find(active_mesh_requests.begin(), active_mesh_requests.end(), this);
			
			if(it != active_mesh_requests.end())
				active_mesh_requests.erase(it);
		}
		void CreateRenderer();

		void InvokeUpdate(float deltatime) override;

		void Unload();
		void Load();
		void Show();
		void Hide();
	};
}