#include "fnctns.h"
#include <fstream>
#include <sstream>
#include <mutex>

#define AUTH_PORT 9000
#define MSG_SERVER_PORT 8080
#define DB_FILE "users.txt"

using namespace std;

mutex db_mutex;

// Check if user exists in the database
bool user_is_found(const string& username, const string& password) {
    lock_guard<mutex> lock(db_mutex);
    ifstream file(DB_FILE);
    string line, stored_user, stored_pass;
    
    while (getline(file, line)) {
        stringstream ss(line);
        ss >> stored_user >> stored_pass;
        if (stored_user == username && stored_pass == password) {
            return true;
        }
    }
    return false;
}

// Register new user in the database
bool register_user(const string& username, const string& password) {
    lock_guard<mutex> lock(db_mutex);
    if (user_is_found(username, password)) return false; // User already exists

    ofstream file(DB_FILE, ios::app);
    file << username << " " << password << endl;
    return true;
}

// Send authentication success response and notify message server
void auth_user(SOCKET client_socket, SOCKET msg_srvr_sock, const string& username, const string& password) {
    Message auth_msg;
    auth_msg.type = MSG_AUTH;
    strcpy_s(auth_msg.sender, username.c_str());

    send_struct_message(msg_srvr_sock, auth_msg);
    send_struct_message(client_socket, auth_msg);
}

// Handle client authentication requests
void handle_client(SOCKET client_socket, SOCKET msg_srvr_sock) {
    Message msg;
    if (!receive_struct_message(client_socket, msg)) {
        closesocket(client_socket);
        return;
    }

    string username(msg.sender);
    string password(msg.content);
    Message response;

    if (msg.type == LOGIN_REQ) {
        if (user_is_found(username, password)) {
            response.type = MSG_AUTH;
            strcpy_s(response.content, "LOGIN_SUCCESS");
            auth_user(client_socket, msg_srvr_sock, username, password);
        } else {
            response.type = MSG_AUTH;
            strcpy_s(response.content, "LOGIN_FAIL");
        }
    } else if (msg.type == SIGNUP_REQ) {
        if (register_user(username, password)) {
            response.type = MSG_AUTH;
            strcpy_s(response.content, "SIGNUP_SUCCESS");
            auth_user(client_socket, msg_srvr_sock, username, password);
        } else {
            response.type = MSG_AUTH;
            strcpy_s(response.content, "SIGNUP_FAIL");
        }
    }

    send_struct_message(client_socket, response);
    closesocket(client_socket);
}

// Connect authentication server to message server
void connect_to_msg_server(SOCKET& msg_srvr_sock) {
    msg_srvr_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in msg_srvr_addr;
    set_addr(&msg_srvr_addr, MSG_SERVER_PORT, "127.0.0.1");

    if (connect(msg_srvr_sock, (sockaddr*)&msg_srvr_addr, sizeof(msg_srvr_addr)) == SOCKET_ERROR) {
        cerr << "[AUTH_SERVER] Failed to connect to message server." << endl;
        exit(1);
    }
    cout << "[AUTH_SERVER] Connected to Message Server on port " << MSG_SERVER_PORT << endl;
}

// Listen for authentication requests
void manage_auth_requests(SOCKET auth_sock, SOCKET msg_srvr_sock) {
    SOCKET client_socket;
    sockaddr_in client_addr;
    int client_addr_size = sizeof(client_addr);

    while ((client_socket = accept(auth_sock, (sockaddr*)&client_addr, &client_addr_size))) {
        thread(handle_client, client_socket, msg_srvr_sock).detach();
    }
}

// Main Authentication Server Entry Point
int main() {
    initialize_winsock();
    SOCKET auth_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKET msg_srvr_sock;

    sockaddr_in auth_addr;
    set_addr(&auth_addr, AUTH_PORT, "127.0.0.1");

    if (bind_to_this_server(auth_sock, auth_addr) == BIND_ERR) return 1;

    listen(auth_sock, SOMAXCONN);
    cout << "[AUTH_SERVER] Listening on port " << AUTH_PORT << endl;

    connect_to_msg_server(msg_srvr_sock);
    manage_auth_requests(auth_sock, msg_srvr_sock);

    cleanup_winsock();
    return 0;
}