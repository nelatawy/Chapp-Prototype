
#include "../include/stdafx.h"
#include "../include/fnctns.h"

using namespace std;

#define AUTH_SERVER_IP "127.0.0.1"
#define MSG_SERVER_IP "127.0.0.1"

#define AUTH_SERVER_CLIENT_PORT 55555
#define AUTH_SERVER_MSG_PORT 55556

#define MSG_SERVER_PORT 8080

#define MAX_CRED_SIZE 250
#define MAX_BUFFER_SIZE 2*(MAX_CRED_SIZE) + sizeof(int)
#define REQ_STRLEN MAX_CRED_SIZE + INET_ADDRSTRLEN

#define LOGIN_REQ 50
#define SIGNUP_REQ 51

//Socket for each service
SOCKET auth_sock;
SOCKET msg_sock_forwarding;//connected socket after listening and accepting from msg_server 

sockaddr_in msg_server_addr;//address of target port in msg_server for socket creation requests

sockaddr_in auth_server_client_addr;//port on this server for client msgs
sockaddr_in auth_server_msg_addr;//port on this server for sending to msg_server

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


void auth_user(SOCKET client_socket, string username,string password){
    send(client_socket,"accepted", sizeof("accepted"),0);
    sockaddr_in client_addr;//for ipv4
    socklen_t addr_length = sizeof(client_addr);
    //getting the peer name --> client_ip
    char client_ip[INET_ADDRSTRLEN];
    if(getpeername(client_socket,(sockaddr*)&client_addr,&addr_length )  == 0) {
        sockaddr_in* ip = &client_addr;//no typecasting needed since it's already a reference to a (sockadd_in)
        inet_ntop(AF_INET, &ip->sin_addr,client_ip,INET_ADDRSTRLEN);

        create_sock_req creation_req;
        strcpy(creation_req.client_username, "bob marley");
        strcpy(creation_req.ip_address, client_ip);

        send(msg_sock_forwarding,(const char*)&creation_req, REQ_STRLEN,0);
    }

}

void handle_client(SOCKET client_socket){
        // auth_req user_req;
        char buffer[MAX_BUFFER_SIZE];
        cout << "handling client\n";
        if (recv(client_socket,(char*)buffer, sizeof(buffer), 0) == SOCKET_ERROR) //connection was lost user will have to try to regain connection so it will create another thread
        {   cout << WSAGetLastError();
            cout << "error connecting msg\n";
            Sleep(15000);
            closesocket(client_socket);
            WSACleanup();
            return;
        }
        //recv is successful
        cout << "recieve accepted";
        auth_req* user_req = (auth_req*)buffer;
        char buffer_send[MAX_BUFFER_SIZE] = "prohibited";\
        char buffer_send2[MAX_BUFFER_SIZE] = "accepted";

        string username = user_req->username;
        string password = user_req->password;

        switch (user_req->op_code)
        {
            case LOGIN_REQ:

                if(user_is_found(username,password)){
                    auth_user(client_socket,user_req->username,user_req->password);
                    send(client_socket,(char*)buffer_send2, sizeof(buffer_send2),0);
                } else {
                    send(client_socket,(char*)buffer_send, sizeof(buffer_send),0);
                }
                cout << "i am here deserialized\n";
                break;
            
            case SIGNUP_REQ:
            //TODO: signup conditions
            break;
        }
        Sleep(10000);
        cout << "Problem Occurred\n";
}

void set_addr(sockaddr_in * server_addr, u_short port, const char* address){
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(port);
    server_addr->sin_addr.s_addr = inet_addr(address);
}

void bind_to_this_server(SOCKET server_socket, sockaddr_in server_addr){

    if (bind(server_socket,(sockaddr*)&server_addr,sizeof(server_addr)) == 0)
    {
        cout << "Successfully bound to 127.0.0.1" << endl;

    }
    else {
        cout << "failed to bind ErrCode: " << WSAGetLastError() << endl;
        closesocket(server_socket);
        WSACleanup();

        system("pause");
        exit(0);
    }
}

void connect_to_msg_server(){
    // set up server socket
    SOCKET msg_server_connect_listener;//temp listening socket
    msg_server_connect_listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (msg_server_connect_listener == INVALID_SOCKET) {
        cerr << "Socket creation failed." << endl;
        WSACleanup();
        Sleep(3000);
        exit(1);
    }

    // Bind socket to the server address (msg server comms)

    cout << "binding for msg service\n";
    if (bind(msg_server_connect_listener,(sockaddr*)&auth_server_msg_addr,sizeof(auth_server_msg_addr)) == 0)
    {
        cout << "Successfully bound to 127.0.0.1" << endl;

    }
    else {
        cout << "failed to bind ErrCode: " << WSAGetLastError() << endl;
        closesocket(msg_server_connect_listener);
        WSACleanup();

        system("pause");
        exit(0);
    }
    cout << "bound for msg service\n";

    // Listen for incoming connections
    if (listen(msg_server_connect_listener, 1) == SOCKET_ERROR) {
        cerr << "Listening failed." << endl;
        closesocket(msg_server_connect_listener);
        WSACleanup();
        Sleep(3000);
        exit(1);
    }
    cout << "waiting for msg server connection" << endl;
    msg_sock_forwarding = accept(msg_server_connect_listener,NULL,NULL);
    if ( msg_sock_forwarding == SOCKET_ERROR) { //only accept from msg server
        cerr << "Accepting failed1." << endl;
        closesocket(msg_sock_forwarding);
        closesocket(msg_server_connect_listener);
        WSACleanup();
        Sleep(5000);
        exit(1);
    }
    closesocket(msg_server_connect_listener);//dont listen for other connections the global socket is now the connected socket
    
    //it will only procceed if connected
}

void manage_auth_requests() {

    // Create server socket
    auth_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (auth_sock == INVALID_SOCKET) {
        cerr << "Socket creation failed." << endl;
        WSACleanup();
        exit(1);
    }

    // Bind socket to the server address
    cout << "before binding for authentication\n";
    bind_to_this_server(auth_sock,auth_server_client_addr);
    cout << "bound for authentication\n";

    // Listen for incoming connections
    if (listen(auth_sock, 5) == SOCKET_ERROR) {
        cerr << "Listening failed2." << endl;
        Sleep(10000);
        closesocket(auth_sock);
        WSACleanup();
        exit(1);
    }

    cout << "Server is running, waiting for connections..." << endl;

    // Accept clients and spawn threads to handle them
    while (server_running.load()) {
        SOCKET client_socket = accept(auth_sock, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            cerr << "Client connection failed." << endl;
            Sleep(80000);
            closesocket(auth_sock);
            WSACleanup();
            continue;
        }
        cout << "connected to client" << endl;
        thread new_thread(handle_client, client_socket);
        new_thread.join();
        
    }
    Sleep(80000);
    closesocket(auth_sock);
    WSACleanup();

}

void shutdown_server() {
    cout << "Shutting down server..." << endl;
    server_running.store(false);
}

int main() {
    initialize_winsock();
    cout << "initialized\n";
    set_addr(&auth_server_client_addr,AUTH_SERVER_CLIENT_PORT,AUTH_SERVER_IP);
    set_addr(&msg_server_addr,MSG_SERVER_PORT,MSG_SERVER_IP);
    set_addr(&auth_server_msg_addr,AUTH_SERVER_MSG_PORT,AUTH_SERVER_IP);
    //all addresses set
    cout << "addresses_set\n";

    connect_to_msg_server();
    cout <<"connected to msg server";

    client_list.insert_or_assign("nour","batman");
    cout << user_is_found("nour","batman") << "\n";

    thread manage_req_thread(manage_auth_requests);

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

    manage_req_thread.join(); 
    WSACleanup(); // Ensure the server thread completes before exiting
    system("pause");
    return 0;
}