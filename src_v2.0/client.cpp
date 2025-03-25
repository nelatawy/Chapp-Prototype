#include <iostream>
#include <winsock2.h>
#include <string>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

#define SERVER_PORT 8080
#define SERVER_IP "127.0.0.1"  // Server IP address
#define MAX_BUFFER_SIZE 250

SOCKET client_socket;

void initialize_winsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed." << endl;
        exit(1);
    }
}

void cleanup_winsock() {
    WSACleanup();
}

void connect_to_server() {
    client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET) {
        cerr << "Socket creation failed." << endl;
        cleanup_winsock();
        exit(1);
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        cerr << "Connection to server failed." << endl;
        closesocket(client_socket);
        cleanup_winsock();
        exit(1);
    }
    cout << "Connected to server." << endl;
}

void send_name_to_server() {
    string client_name;
    cout << "Enter your name: ";
    getline(cin, client_name);

    if (send(client_socket, client_name.c_str(), client_name.length(), 0) == SOCKET_ERROR) {
        cerr << "Failed to send name to server." << endl;
        closesocket(client_socket);
        cleanup_winsock();
        exit(1);
    }
}

void send_message(const string& message) {
    if (send(client_socket, message.c_str(), message.length(), 0) == SOCKET_ERROR) {
        cerr << "Error sending message." << endl;
        closesocket(client_socket);
        cleanup_winsock();
        exit(1);
    }
}

void receive_message() {
    char buffer[MAX_BUFFER_SIZE];
    int bytes_received;
    while (true) {
        bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            cerr << "Server disconnected or message receive error." << endl;
            break;
        }
        buffer[bytes_received] = '\0';
        cout << "Server: " << buffer << endl;
    }
}

void handle_user_input() {
    string message;
    while (true) {
        cout << "Enter message: ";
        getline(cin, message);

        if (message == "/exit") {
            cout << "Exiting chat..." << endl;
            break;
        }

        send_message(message);
    }
}

int main() {
    initialize_winsock();
    connect_to_server();
    send_name_to_server();

    thread(receive_message).detach(); // Run message receiving in a separate thread

    handle_user_input();

    closesocket(client_socket);
    cleanup_winsock();
    return 0;
}