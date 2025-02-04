
#include "../include/stdafx.h"
#include "../include/fnctns.h"
#include "../include/constants.h"

using namespace std;

#define AUTH_SERVER_IP "127.0.0.1"
#define MSG_SERVER_IP "127.0.0.1"

#define AUTH_SERVER_CLIENT_PORT 55555
#define AUTH_SERVER_MSG_PORT 55556

#define MSG_SERVER_PORT 8080



map<string, string> client_list;//username password combination
atomic<bool> server_running(true);  // Flag to control server shutdown

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

void auth_user(SOCKET client_socket,SOCKET msg_srvr_sock, string username,string password){
    
    sockaddr_in client_addr;//for ipv4
    socklen_t addr_length = sizeof(client_addr);
    //getting the peer name --> client_ip
    char client_ip[INET_ADDRSTRLEN];
    if(getpeername(client_socket,(sockaddr*)&client_addr,&addr_length )  == 0) {
        sockaddr_in* ip = &client_addr;//no typecasting needed since it's already a reference to a (sockadd_in)
        inet_ntop(AF_INET, &ip->sin_addr,client_ip,INET_ADDRSTRLEN);

        create_sock_req creation_req;
        strcpy(creation_req.client_username, " bob marley ");
        strcpy(creation_req.ip_address, client_ip);

        send(msg_srvr_sock,(const char*)&creation_req, REQ_STRLEN,0);
    }

}

void handle_client(SOCKET client_socket, SOCKET msg_srvr_sock){
        // auth_req user_req;
        char buffer[MAX_BUFFER_SIZE];
        cout << "handling client\n";
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
                } else {
                    send(client_socket,(char*)prevent_msg, sizeof(prevent_msg),0);
                }
                cout << "i am here deserialized\n";
                break;
            
            case SIGNUP_REQ:
                //TODO: signup conditions
                break;

            default:
                cout << "A Problem Occured\n";
                break;
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
        cout << "Successfully bound to 127.0.0.1" << endl;
        break;
    
    case SOCKCR_ERR:
        return SOCKCR_ERR;
        break;
    case BIND_ERR :
        cerr << "error binding connection listener socket\n";
        return BIND_ERR;
    }
    cout << "bound for msg service\n";

    // Listen for incoming connections
    if (listen(msg_server_connect_listener, 1) == SOCKET_ERROR) {
        cerr << "Listening For Connection Failed." << endl;
        return LISN_ERR;
    }

    cout << "waiting for msg server connection" << endl;
    msg_srvr_sock = accept(msg_server_connect_listener,NULL,NULL);
    if (msg_srvr_sock == SOCKET_ERROR) { //only accept from msg server
        cerr << "Accepting Connection From Msg Server Failed.\n";
        return ACC_ERR;
        
    }
    closesocket(msg_server_connect_listener);//dont listen for other connections the global socket is now the connected socket
    return 1;
    //it will only procceed if connected
}

void manage_auth_requests(SOCKET auth_sock , SOCKET msg_srvr_sock) {

    // needs an already bound and listening socket
    cout << "Server is running, waiting for connections..." << endl;

    // Accept clients and spawn threads to handle them
    while (server_running.load()) {
        SOCKET client_socket = accept(auth_sock, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            cerr << "Client connection failed." << endl;
            continue;
        }
        cout << "connected to client" << endl;
        thread new_thread(handle_client, client_socket,msg_srvr_sock);
        new_thread.join();
        
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
    cout << "Auth Sock waiting for authentication requests\n";

    //STEP 4
    thread manage_req_thread(manage_auth_requests,auth_sock, msg_sock_forwarding);
    cout << "currently managing auth requests\n";

    //STEP 5
    manage_req_thread.join(); 
    Sleep(8000);
    closesocket(auth_sock);
    WSACleanup(); // Ensure the server thread completes before exiting
    system("pause");
    return 0;
}