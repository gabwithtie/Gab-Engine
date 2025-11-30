#pragma once

#include "Engine/gbe_engine.h"
#include "Math/gbe_math.h"

#include <sockpp/tcp_connector.h>
#include <sockpp/exception.h>

#include <sockpp/version.h> // Include this header

namespace gbe::gdparcm
{
	class MeshAsync : public Object {
	private:
		static std::vector<MeshAsync*> active_mesh_requests;

		int server_port;
		std::string client_id;

		const std::filesystem::path mesh_file_directory = "cache/models/";

		inline std::filesystem::path Get_filepath() {
			return mesh_file_directory / std::string(client_id + ".obj");
		}
	public:
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
	};
}