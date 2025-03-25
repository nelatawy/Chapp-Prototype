#include "fnctns.h"

int main() {
    initialize_winsock();
    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in server_addr;
    set_addr(&server_addr, 8080, "127.0.0.1");

    if (connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        cerr << "Connection failed." << endl;
        return 1;
    }
    cout << "Connected to server." << endl;

    thread([](SOCKET client_socket) {
        Message msg;
        while (receive_struct_message(client_socket, msg)) {
            cout << "[MSG] " << msg.sender << ": " << msg.content << endl;
        }
        closesocket(client_socket);
    }, client_socket).detach();

    Message msg;
    strcpy_s(msg.sender, "User1");
    msg.type = MSG_BROADCAST;
    while (true) {
        cin.getline(msg.content, MAX_BUFFER_SIZE);
        send_struct_message(client_socket, msg);
    }

    cleanup_winsock();
    return 0;
}
