#include "stdafx.h"
#include "constants.h"

using namespace std;

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

int bind_to_this_server(SOCKET server_socket, sockaddr_in server_addr) {
    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        cerr << "Binding failed. ErrCode: " << WSAGetLastError() << endl;
        return BIND_ERR;
    }
    return 1;
}

bool send_struct_message(SOCKET socket, Message& msg) {
    return send(socket, (char*)&msg, sizeof(Message), 0) > 0 ;
}

bool receive_struct_message(SOCKET socket, Message& msg) {
    return recv(socket, (char*)&msg, sizeof(Message), 0) > 0;
}