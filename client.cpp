
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

std::mutex log_mutex;
std::map<int, std::string> client_names;
std::mutex client_mutex;
std::ofstream log_file("chat_log.txt", std::ios::app);

void log_message(const std::string &message) {
    std::lock_guard<std::mutex> lock(log_mutex);
    log_file << message << std::endl;
    log_file.flush();
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    std::string name;

    // Receive the username from the client
    recv(client_socket, buffer, BUFFER_SIZE, 0);
    name = buffer;

    {
        std::lock_guard<std::mutex> lock(client_mutex);
        client_names[client_socket] = name;
    }

    std::cout << name << " connected." << std::endl;
    log_message(name + " connected.");

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            std::cout << name << " disconnected." << std::endl;
            log_message(name + " disconnected.");

            {
                std::lock_guard<std::mutex> lock(client_mutex);
                client_names.erase(client_socket);
            }

            close(client_socket);
            break;
        }

        std::string message(buffer);

        // Message format: "recipient_name: message"
        size_t delimiter_pos = message.find(":");
        if (delimiter_pos == std::string::npos) {
            std::string error_msg = "Invalid message format. Use 'recipient_name: message'.";
            send(client_socket, error_msg.c_str(), error_msg.size(), 0);
            continue;
        }

        std::string recipient_name = message.substr(0, delimiter_pos);
        std::string chat_message = message.substr(delimiter_pos + 1);
        chat_message = name + " to " + recipient_name + ": " + chat_message;

        bool recipient_found = false;

        {
            std::lock_guard<std::mutex> lock(client_mutex);
            for (const auto &[socket, cname] : client_names) {
                if (cname == recipient_name) {
                    send(socket, chat_message.c_str(), chat_message.size(), 0);
                    recipient_found = true;
                    break;
                }
            }
        }

        if (!recipient_found) {
            std::string error_msg = "Recipient not found.";
            send(client_socket, error_msg.c_str(), error_msg.size(), 0);
        }

        log_message(chat_message);
    }
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    server_socket = socket(AF_INET, SOCK_STREAM, 0)

    if (server_socket == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure socket options
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    // Bind socket
    if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server is running on port " << PORT << std::endl;

    while (true) {
        if ((client_socket = accept(server_socket, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        std::thread client_thread(handle_client, client_socket);
        client_thread.detach();
    }

    return 0;
}
