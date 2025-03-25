
#include <iostream>
#include <thread>
#include <vector>
#include <winsock2.h>
#include <map>
#include <atomic>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

#define SERVER_PORT 8080
#define MAX_BUFFER_SIZE 250

SOCKET server_socket;
map<SOCKET, string> client_list;
atomic<bool> server_running(true);  // Flag to control server shutdown

void handle_client(SOCKET client_socket) {
    char buffer[MAX_BUFFER_SIZE];
    string client_name = "Unnamed Client";

    // Receive client name
    int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
        cerr << "Error receiving client name." << endl;
        closesocket(client_socket);
        return;
    }
    buffer[bytes_received] = '\0';
    client_name = string(buffer);

    // Add client to client list
    client_list[client_socket] = client_name;
    cout << client_name << " connected." << endl;

    // Broadcast client connection to all other clients
    string connect_msg = client_name + " has joined the chat.";
    for (auto& client : client_list) {
        if (client.first != client_socket) {
            send(client.first, connect_msg.c_str(), connect_msg.length(), 0);
        }
    }

    // Handle messages from the client
    while (server_running.load()) {
        bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            cerr << client_name << " disconnected." << endl;
            break;
        }
        buffer[bytes_received] = '\0';

        string message = client_name + ": " + string(buffer);

        // Broadcast message to all clients
        for (auto& client : client_list) {
            if (client.first != client_socket) {
                send(client.first, message.c_str(), message.length(), 0);
            }
        }
    }

    // Remove client from the list and broadcast disconnection
    client_list.erase(client_socket);
    string disconnect_msg = client_name + " has left the chat.";
    for (auto& client : client_list) {
        send(client.first, disconnect_msg.c_str(), disconnect_msg.length(), 0);
    }

    closesocket(client_socket);
}

void server_main() {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed." << endl;
        exit(1);
    }

    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        cerr << "Socket creation failed." << endl;
        WSACleanup();
        exit(1);
    }

    // Bind socket to the server address
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        cerr << "Binding failed." << endl;
        closesocket(server_socket);
        WSACleanup();
        exit(1);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) == SOCKET_ERROR) {
        cerr << "Listening failed." << endl;
        closesocket(server_socket);
        WSACleanup();
        exit(1);
    }

    cout << "Server is running, waiting for connections..." << endl;

    // Accept clients and spawn threads to handle them
    while (server_running.load()) {
        SOCKET client_socket = accept(server_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            cerr << "Client connection failed." << endl;
            continue;
        }

        // Create a thread for each client
        thread(handle_client, client_socket).detach();
    }

    closesocket(server_socket);
    WSACleanup();
}

void shutdown_server() {
    cout << "Shutting down server..." << endl;
    server_running.store(false);
}

int main() {
    thread server_thread(server_main);

    // Wait for user input to shutdown the server
    string input;
    while (true) {
        cout << "Enter '/shutdown' to stop the server: ";
        getline(cin, input);
        if (input == "/shutdown") {
            shutdown_server();
            break;
        }
    }

    server_thread.join();  // Ensure the server thread completes before exiting
    return 0;
}