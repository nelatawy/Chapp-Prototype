
#include "../include/stdafx.h"
#include "../include/fnctns.h"
#include "../include/constants.h"
#include <fstream>
#include <sstream>
#include <mutex>


using namespace std;

#define AUTH_SERVER_IP "127.0.0.1"
#define MSG_SERVER_IP "127.0.0.1"

#define AUTH_SERVER_CLIENT_PORT 55555
#define AUTH_SERVER_MSG_PORT 55556

#define MSG_SERVER_PORT 8080
#define DB_FILE "users.txt"



mutex db_mutex;
map<string, string> client_list;//username password combination
atomic<bool> server_running(true);  // Flag to control server shutdown

// Check if user exists in the database
// bool user_is_found(const string& username, const string& password) {
//     lock_guard<mutex> lock(db_mutex);
//     ifstream file(DB_FILE);
//     string line, stored_user, stored_pass;
    
//     while (getline(file, line)) {
//         stringstream ss(line);
//         ss >> stored_user >> stored_pass;
//         if (stored_user == username && stored_pass == password) {
//             return true;
//         }
//     }
//     return false;
// }



typedef
struct auth_req{
    uint32_t op_code;
    char username[MAX_CRED_SIZE];
    char password[MAX_CRED_SIZE];

}msg;


typedef
struct create_sock_req{
    char client_username[MAX_CRED_SIZE];
    char ip_address[INET_ADDRSTRLEN];
}create_sock_request;




// Register new user in the database
// bool register_user(const string& username, const string& password) {
//     lock_guard<mutex> lock(db_mutex);
//     if (user_is_found(username, password)) return false; // User already exists

//     ofstream file(DB_FILE, ios::app);
//     file << username << " " << password << endl;
//     return true;
// }

bool register_user(const string& username , const string& password){

    if (client_list.find(username) == client_list.end())
    {
        client_list[username] = password;
        return 1;
    }
    return 0;
    
}

bool user_is_found(string username, string password){
    auto found_at = client_list.find(username);
    if (found_at == client_list.end())
    {
        return false;
    }
    //username is found
    if (found_at->second != password)
    {
        return false;
    }
    // password is also found
    return true;

}

void auth_user(SOCKET client_socket,SOCKET msg_srvr_sock, char* username,char* password){
    
    sockaddr_in client_addr;//for ipv4
    socklen_t addr_length = sizeof(client_addr);
    //getting the peer name --> client_ip
    char client_ip[INET_ADDRSTRLEN];
    if(getpeername(client_socket,(sockaddr*)&client_addr,&addr_length )  == 0) {
        sockaddr_in* ip = &client_addr;//no typecasting needed since it's already a reference to a (sockadd_in)
        inet_ntop(AF_INET, &ip->sin_addr,client_ip,INET_ADDRSTRLEN);

        create_sock_req creation_req;
        strcpy(creation_req.client_username, username);
        strcpy(creation_req.ip_address, client_ip);

        send(msg_srvr_sock,(const char*)&creation_req, REQ_STRLEN,0);
    }

}

void handle_client_reg(SOCKET client_socket, SOCKET msg_srvr_sock){
        // auth_req user_req;
        cout << "handling client\n";
        while(1){
            char buffer[MAX_BUFFER_SIZE];
            if (recv(client_socket,(char*)buffer, sizeof(buffer), 0) == SOCKET_ERROR) //connection was lost user will have to try to regain connection so it will create another thread
            {   cout << WSAGetLastError();
                cout << "error connecting msg\n";
                Sleep(1500);
                closesocket(client_socket);
                return;
            }
            //recv is successful
            cout << "recieve accepted";
            auth_req* user_req = (auth_req*)buffer;
            char prevent_msg[MAX_BUFFER_SIZE] = "prohibited";
            char accept_msg[MAX_BUFFER_SIZE] = "accepted";
                switch (user_req->op_code)
                {
                    case LOGIN_REQ:

                        if(user_is_found(user_req->username, user_req->password)){
                            auth_user(client_socket, msg_srvr_sock, user_req->username, user_req->password);
                            send(client_socket,(char*)accept_msg, sizeof(accept_msg),0);
                            return;
                        } else {
                            send(client_socket,(char*)prevent_msg, sizeof(prevent_msg),0);
                        }
                        cout << "i am here deserialized\n";
                        break;
                    
                    case SIGNUP_REQ:
                        if (register_user(user_req->username, user_req->password)) {
                            send(client_socket,(char*)accept_msg, sizeof(accept_msg),0);
                            return;
                            // response.type = MSG_AUTH;
                            // strcpy_s(response.content, "SIGNUP_SUCCESS");
                            // auth_user(client_socket, msg_srvr_sock, username, password);
                        } else {
                            char prevent_msg[MAX_BUFFER_SIZE] = "prohibited";
                            // response.type = MSG_AUTH;
                            // strcpy_s(response.content, "SIGNUP_FAIL");
                        }
                        break;

                    default:
                        cout << "A Problem Occured\n";
                        return;
                }
        }
        Sleep(1000);
        return;

}

int connect_to_msg_server(SOCKET& msg_srvr_sock, sockaddr_in auth_srvr_addr){//intended connected socket after connecting with msg server
    SOCKET msg_server_connect_listener;//temp listening socket

    int bind_flag = assign_and_bind_sock(msg_server_connect_listener,auth_srvr_addr);
    switch (bind_flag)
    {
    case 1:
        cout << "Successfully Bound To 127.0.0.1" << endl;
        break;
    
    case SOCKCR_ERR:
        return SOCKCR_ERR;
        break;
    case BIND_ERR :
        cerr << "Error Binding Socket [listener socket]\n";
        return BIND_ERR;
    }
    cout << "Bound & Ready For Msg Server Connection\n";

    // Listen for incoming connections
    if (listen(msg_server_connect_listener, 1) == SOCKET_ERROR) {
        cerr << "Listening For Connection Failed." << endl;
        return LISN_ERR;
    }

    cout << "Waiting For Msg Server Connection" << endl;
    msg_srvr_sock = accept(msg_server_connect_listener,NULL,NULL);
    if (msg_srvr_sock == SOCKET_ERROR) { //only accept from msg server
        cerr << "[AUTH_SERVER] Failed To Connect To Message Server." << endl;
        return ACC_ERR;
        
    }
    cout << "[AUTH_SERVER] Connected to Message Server from port " << AUTH_SERVER_MSG_PORT << "to port "<< MSG_SERVER_PORT << endl;
    closesocket(msg_server_connect_listener);//dont listen for other connections the global socket is now the connected socket
    return 1;
    //it will only procceed if connected
}

void manage_auth_requests(SOCKET auth_sock , SOCKET msg_srvr_sock) {

    // needs an already bound and listening socket
    cout << "Server Is Running, Waiting For Connections..." << endl;

    // Accept clients and spawn threads to handle them
    while (server_running.load()) {
        SOCKET client_socket;
        if ((client_socket = accept(auth_sock, NULL, NULL)) == INVALID_SOCKET) {
            cerr << "Client connection failed." << endl;
            continue;
        }
        cout << "connected to client" << endl;
        thread (handle_client_reg, client_socket,msg_srvr_sock).detach();
        
    }

}

void shutdown_server() {
    cout << "Shutting down server..." << endl;
    server_running.store(false);
}

int main() {
    //STEP1
    initialize_winsock();
    cout << "initialized\n";


    //STEP 2
    //Socket for each service

    SOCKET auth_sock;
    SOCKET msg_sock_forwarding;//connected socket after listening and accepting from msg_server 

    sockaddr_in msg_server_addr;//address of target port in msg_server for socket creation requests

    sockaddr_in auth_server_client_addr;//port on this server for client msgs
    sockaddr_in auth_server_msg_addr;//port on this server for sending to msg_server

    set_addr(&auth_server_client_addr,AUTH_SERVER_CLIENT_PORT,AUTH_SERVER_IP);
    set_addr(&msg_server_addr,MSG_SERVER_PORT,MSG_SERVER_IP);
    set_addr(&auth_server_msg_addr,AUTH_SERVER_MSG_PORT,AUTH_SERVER_IP);
    //all addresses set
    cout << "addresses_set\n";


    //STEP 3
    if(connect_to_msg_server(msg_sock_forwarding,auth_server_msg_addr) != 1){
        terminate_and_close_this_socket(msg_sock_forwarding);
    }
    cout <<"connected to msg server\n";



    client_list.insert_or_assign("nour","batman");
    client_list.insert_or_assign("kareem","crimson");


    //STEP 4
    if(assign_and_bind_sock(auth_sock,auth_server_client_addr) != 1){
        cout << "problem setting up auth socket\n";
        terminate_and_close_this_socket(auth_sock);
    }
    cout << "auth sock bound successfully\n";

    // Listen for incoming connections
    if (listen(auth_sock, 5) == SOCKET_ERROR) {
        cerr << "Listening failed2." << endl;
        Sleep(10000);
        closesocket(auth_sock);
        WSACleanup();
        exit(1);
    }
    cout << "[AUTH_SERVER] Listening on port " << AUTH_SERVER_CLIENT_PORT << endl;

    //STEP 4
    manage_auth_requests(auth_sock,msg_sock_forwarding);
    cout << "currently managing auth requests\n";

    //STEP 5
    Sleep(8000);
    closesocket(auth_sock);
    WSACleanup(); // Ensure the server thread completes before exiting
    system("pause");
    return 0;
}