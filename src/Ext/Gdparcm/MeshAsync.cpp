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

        // ... (Rest of the function remains the same)

        // 3. Send the client_id to the server
        if (connector.write(client_id) != client_id.length()) {
            std::cerr << "Error sending client_id to server: "
                << connector.last_error_str() << std::endl;
            return;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));

        // 4. Receive the server reply (the mesh file) and write to disk
        std::ofstream output_file(filepath);
        if (!output_file.is_open()) {
            std::cerr << "Error: Could not open file for writing: " << filepath.string() << std::endl;
            return;
        }


        char buffer[4024];
        std::string full_response;

        // Loop until we find the newline terminator or encounter an error
        while (true) {
            // 1. Read data from the socket using the sockpp connector's read()
            // read() returns the number of bytes read, or a negative value on error.
            ssize_t bytes_received = connector.read(buffer, sizeof(buffer));

            if (bytes_received > 0) {
                // Append the data received (using a range constructor for safety)
                full_response.append(buffer, bytes_received);
                output_file << full_response;
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

        // 5. Call CreateRenderer() when the file is successfully saved
        instance->CreateRenderer();

    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception caught: " << e.what() << std::endl;
    }
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

        // Launch a new detached thread to handle the asynchronous networking
        std::thread t(MeshAsync_thread_func, this, this->server_port, this->client_id, output_filepath);
        t.detach();

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

        auto newrenderer = new RenderObject(RenderPipeline::RegisterDrawCall(newmesh, material));
        newrenderer->SetParent(this);

        std::cout << "Renderer created successfully for client ID: " << this->client_id << std::endl;
    }

}