#include "MeshAsync.h"
#include <thread>
#include <fstream>
#include <iostream>

// The function that the new thread will execute
void MeshAsync_thread_func(gbe::gdparcm::MeshAsync* instance, int port, std::string client_id, std::filesystem::path filepath) {

    std::cout << "Starting asynchronous mesh download for client_id: " << client_id << std::endl;

    try {
        // 1. Create a TCP connector
        sockpp::tcp_connector connector;

        std::string host = "localhost";
        sockpp::inet_address server_address(host, port);

        // 2. Connect to the server
        if (!connector.connect(server_address)) {

            std::cerr << "Error connecting to server on port " << port << ": "
                << connector.last_error_str() << std::endl;
            return;
        }

        std::cout << "Successfully connected to server. Sending client ID..." << std::endl;

        //GET FILE SIZE
        {
            auto req_str = client_id + "l";
            if (connector.write(req_str) != req_str.length()) {
                std::cerr << "Error sending client_id to server: "
                    << connector.last_error_str() << std::endl;
                return;
            }

            char buffer[128];
            ssize_t bytes_received = connector.read(buffer, sizeof(buffer) - 1);

            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                
				instance->file_size = std::stoi(buffer);
            }
            else if (bytes_received == 0) {
                // Connection closed by the server (EOF)
                std::cerr << "Server closed the connection." << std::endl;
            }
            else {
                // An error occurred (sockpp throws an exception on error, 
                // but for simple read errors, it might return -1)
                std::cerr << "Error receiving data: " << connector.last_error_str() << std::endl;
            }

            instance->file_size = std::stoi(buffer);
        }

        // ... (Rest of the function remains the same)

        // 3. Send the client_id to the server
        if (connector.write(client_id) != client_id.length()) {
            std::cerr << "Error sending client_id to server: "
                << connector.last_error_str() << std::endl;
            return;
        }

        // 4. Receive the server reply (the mesh file) and write to disk
        std::ofstream output_file(filepath);
        if (!output_file.is_open()) {
            std::cerr << "Error: Could not open file for writing: " << filepath.string() << std::endl;
            return;
        }

        char buffer[4024];
		int total_bytes_received = 0;

        // Loop until we find the newline terminator or encounter an error
        while (true) {
            ssize_t bytes_received = connector.read(buffer, sizeof(buffer) - 1);
			total_bytes_received += bytes_received;

            if (bytes_received > 0) {
				buffer[bytes_received] = '\0';
                output_file << buffer;

				instance->progress = (float)total_bytes_received / (float)instance->file_size;

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            else if (bytes_received == 0) {
                // Connection closed by the server (EOF)
                std::cerr << "Server closed the connection." << std::endl;
                break;
            }
            else {
                // An error occurred (sockpp throws an exception on error, 
                // but for simple read errors, it might return -1)
                std::cerr << "Error receiving data: " << connector.last_error_str() << std::endl;
                break;
            }
        }

        output_file.close();

        std::cout << "Successfully received and saved mesh file to: " << filepath.string() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception caught: " << e.what() << std::endl;
    }

	instance->worker_done = true;
}

// ... (MeshAsync::MeshAsync and MeshAsync::CreateRenderer remain the same)
// The rest of your MeshAsync.cpp file...

namespace gbe::gdparcm
{

    std::vector<MeshAsync*> MeshAsync::active_mesh_requests;

    MeshAsync::MeshAsync(int _server_port, std::string _client_id)
    {
		active_mesh_requests.push_back(this);

        this->server_port = _server_port;
        this->client_id = _client_id;

        // Create the full filepath for the output file
        std::filesystem::path output_filepath = Get_filepath();
    }

    void MeshAsync::InvokeUpdate(float deltatime)
    {
        if (this->worker_thread != nullptr && worker_done) {
            this->worker_thread->join();

			std::cout << "Mesh download thread joined for client ID: " << this->client_id << std::endl;
            
            this->CreateRenderer();

			free(this->worker_thread);
			this->worker_thread = nullptr; // Prevent further joins
        }
    }

    void MeshAsync::Reload()
    {
        if (this->this_renderer != nullptr) {
            this->this_renderer->Destroy();
            this->this_renderer = nullptr;
        }

        this->worker_thread = new std::thread(MeshAsync_thread_func, this, this->server_port, this->client_id, Get_filepath());
        this->worker_done = false;
    }

    void gbe::gdparcm::MeshAsync::CreateRenderer()
    {
        // Check if the file exists before attempting to load
        if (!std::filesystem::exists(Get_filepath())) {
            std::cerr << "Error: Mesh file not found at " << Get_filepath().string() << ". Renderer creation aborted." << std::endl;
            return;
        }

        auto newmesh = asset::Mesh::ImportMesh(Get_filepath());

        // Safety check for mesh loading
        if (!newmesh) {
            std::cerr << "Error: Failed to import mesh from file." << std::endl;
            return;
        }

        auto material = asset::Material::GetAssetById("lit");

        this->this_renderer = new RenderObject(RenderPipeline::RegisterDrawCall(newmesh, material));
        this->this_renderer->SetParent(this);

        std::cout << "Renderer created successfully for client ID: " << this->client_id << std::endl;
    }

}