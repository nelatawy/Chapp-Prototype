#include "fnctns.h"

int main() {
    initialize_winsock();
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in server_addr;
    set_addr(&server_addr, 8080, "127.0.0.1");

    if (bind_to_this_server(server_socket, server_addr) == BIND_ERR) {
        return 1;
    }

    listen(server_socket, SOMAXCONN);
    cout << "Server listening on port 8080" << endl;

    SOCKET client_socket;
    sockaddr_in client_addr;
    int client_addr_size = sizeof(client_addr);

    while ((client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_addr_size))) {
        thread([](SOCKET client_socket) {
            Message msg;
            while (receive_struct_message(client_socket, msg)) {
                cout << "[MSG] " << msg.sender << ": " << msg.content << endl;
            }
            closesocket(client_socket);
        }, client_socket).detach();
    }

    cleanup_winsock();
    return 0;
}