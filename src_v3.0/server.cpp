#include "../include/stdafx.h"
#include "../include/fnctns.h"

using namespace std;

#define AUTH_SERVER_IP "127.0.0.1"
#define MSG_SERVER_IP "127.0.0.1"

#define AUTH_SERVER_PORT 55556

#define MSG_SERVER_AUTH_PORT 8080
#define MSG_SERVER_CLIENT_PORT 9090

#define MAX_CRED_SIZE 250
#define MAX_BUFFER_SIZE 2*(MAX_CRED_SIZE) + sizeof(int)
#define REQ_STRLEN   MAX_CRED_SIZE + INET_ADDRSTRLEN

#define LOGIN_REQ 50
#define SIGNUP_REQ 51

SOCKET msg_socket;

SOCKET auth_req_socket;

sockaddr_in auth_server_addr;
sockaddr_in msg_server_auth_addr;
sockaddr_in msg_server_client_addr;

map<string, SOCKET> client_list;
atomic<bool> server_running(true);  // Flag to control server shutdown


typedef
struct create_sock_req{
    char client_username[MAX_CRED_SIZE];
    char ip_address[INET_ADDRSTRLEN];
}create_sock_request;

void set_addr(sockaddr_in* server_addr, u_short port, const char* address){
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(port);
    server_addr->sin_addr.s_addr = inet_addr(address);
}





void bind_to_this_server(SOCKET server_socket, sockaddr_in server_addr){

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        cerr << "Binding failed." << endl;
        closesocket(server_socket);
        WSACleanup();
        exit(1);
    }
}


void handle_user_messages(SOCKET* socket){
    cout << "\nhandling user messages\n";
    int client_id = rand()%10;
    char buffer[MAX_BUFFER_SIZE];
    while (1)
    {   
        cout << "trying to recieve msgs\n\n";

        if (recv(*socket,(char*)buffer,MAX_BUFFER_SIZE,0) == SOCKET_ERROR)
        {   
            cout << "error recieving message";
            closesocket(*socket);
            return;
        }
        cout << WSAGetLastError() << "\n";
        cout << "Client" << client_id << ": " << buffer << endl;
        
    }
    
}

void* chk_wrt_msgs(SOCKET socket) {
    char buffer[MAX_BUFFER_SIZE];
    while (1)
    {   

        if(recv(socket,buffer,MAX_BUFFER_SIZE,0) > 0){
            cout << "Client ==> " << buffer << endl;
        }
        Sleep(1e-2);
    }
    
}

void send_ping(SOCKET socket){
    char temp;
    if (send(socket,(char*)&temp,0,MSG_PEEK) == SOCKET_ERROR)
    {
        cout << "sending issue Err : " << WSAGetLastError() << endl;
    }
    
}

void manage_auth_requests(){
    auth_req_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (auth_req_socket == INVALID_SOCKET) {
        cerr << "Socket creation with auth_server failed." << endl;
        WSACleanup();
        exit(1);
    }  
    
    //first thing
    ensure_connection(auth_req_socket,auth_server_addr,"auth server manage requests");

    if (listen(msg_socket, 1) == SOCKET_ERROR) {
        cerr << "Listening failed2." << endl;
        closesocket(msg_socket);
        WSACleanup();
        exit(1);
    }
    while (1)
    {   

        char req_buffer[REQ_STRLEN];
        cout << "trying to recv\n";
        if (recv(auth_req_socket,(char*)&req_buffer,sizeof(req_buffer),0) == SOCKET_ERROR)
        {
            ensure_connection(auth_req_socket,auth_server_addr,"auth server regained");
            //will continue if connection was re-established
            // continue;
        }
        //recieved successfully
        //will try to accept from the msg socket
        create_sock_req* request = (create_sock_req*) req_buffer;
        cout << "trying to accept\n";
        SOCKET accept_sock = accept(msg_socket,NULL,NULL) ;
        if (accept_sock == INVALID_SOCKET) {
            cerr << "accepting connection failed." << endl;
            continue;
            //to keep checking for other requests
        }
        closesocket(auth_req_socket);
        //recieved successfully
        //create socket for user
        cout << "protocol init\n";
        // u_long mode = 0;
        // ioctlsocket(accept_sock,FIONBIO,&mode);
        // thread new_t(&handle_user_messages,&accept_sock);
        // new_t.join();
        send_ping(accept_sock);
        thread read_thread(chk_wrt_msgs,accept_sock);
        char buffer_send[MAX_BUFFER_SIZE],buffer_recv[MAX_BUFFER_SIZE];

        while(1){

            cin.getline(buffer_send,MAX_BUFFER_SIZE);
            if (send(accept_sock,(char*)buffer_send,MAX_BUFFER_SIZE,0) <= 0){
                cout << "Error sending the message to server" << WSAGetLastError() << endl;
            }
            else {
                cout << "Message Sent" << endl;
                cout << "Server ==> " << buffer_send << endl;
            }
            
        }

        read_thread.join();

        cout << request->client_username << "wants to connect .... Attempting" << endl;
        client_list.insert_or_assign((string)request->client_username,accept_sock);
        
    }
    
    
}


void set_msg_sock(){
    // Create server socket
    msg_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (msg_socket == INVALID_SOCKET) {
        cerr << "Socket creation failed." << endl;
        WSACleanup();
        exit(1);
    }

    // Bind socket to the server address
    cout << "before binding for authentication\n";
    bind_to_this_server(msg_socket,msg_server_client_addr);
    cout << "bound for msgs\n";

    // Listen for incoming connections
}

void shutdown_server() {
    cout << "Shutting down server..." << endl;
    server_running.store(false);
}

int main() {
    initialize_winsock();
    set_addr(&msg_server_auth_addr,MSG_SERVER_AUTH_PORT,MSG_SERVER_IP);
    set_addr(&msg_server_client_addr,MSG_SERVER_CLIENT_PORT,MSG_SERVER_IP);
    set_addr(&auth_server_addr,AUTH_SERVER_PORT,AUTH_SERVER_IP);
    set_msg_sock();
    manage_auth_requests();



    // Wait for user input to shutdown the server
    // string input;
    // while (true) {
    //     cout << "Enter '/shutdown' to stop the server: ";
    //     getline(cin, input);
    //     if (input == "/shutdown") {
    //         shutdown_server();
    //         break;
    //     }
    // }
    // Ensure the server thread completes before exiting
    WSACleanup();
    return 0;
}












// void handle_client(SOCKET client_socket) {
//     char buffer[MAX_BUFFER_SIZE];
//     string client_name = "Unnamed Client";

//     // Receive client name
//     int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
//     if (bytes_received <= 0) {
//         cerr << "Error receiving client name." << endl;
//         closesocket(client_socket);
//         return;
//     }
//     buffer[bytes_received] = '\0';
//     client_name = string(buffer);

//     // Add client to client list
//     client_list[client_socket] = client_name;
//     cout << client_name << " connected." << endl;

//     // Broadcast client connection to all other clients
//     string connect_msg = client_name + " has joined the chat.";
//     for (auto& client : client_list) {
//         if (client.first != client_socket) {
//             send(client.first, connect_msg.c_str(), connect_msg.length(), 0);
//         }
//     }

//     // Handle messages from the client
//     while (server_running.load()) {
//         bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
//         if (bytes_received <= 0) {
//             cerr << client_name << " disconnected." << endl;
//             break;
//         }
//         buffer[bytes_received] = '\0';

//         string message = client_name + ": " + string(buffer);

//         // Broadcast message to all clients
//         for (auto& client : client_list) {
//             if (client.first != client_socket) {
//                 send(client.first, message.c_str(), message.length(), 0);
//             }
//         }
//     }

//     // Remove client from the list and broadcast disconnection
//     client_list.erase(client_socket);
//     string disconnect_msg = client_name + " has left the chat.";
//     for (auto& client : client_list) {
//         send(client.first, disconnect_msg.c_str(), disconnect_msg.length(), 0);
//     }

//     closesocket(client_socket);
// }

// void server_main() {
//     // Initialize Winsock
//     initialize_winsock();
//     set_auth_addr();
//     set_msg_addr();
//     manage_auth_requests();
//     // Create server socket
//     // server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//     // if (server_socket == INVALID_SOCKET) {
//     //     cerr << "Socket creation failed." << endl;
//     //     WSACleanup();
//     //     exit(1);
//     // }

//     // // Bind socket to the server address
//     // sockaddr_in server_addr;
//     // server_addr.sin_family = AF_INET;
//     // server_addr.sin_port = htons(SERVER_PORT);
//     // server_addr.sin_addr.s_addr = INADDR_ANY;

//     // if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
//     //     cerr << "Binding failed." << endl;
//     //     closesocket(server_socket);
//     //     WSACleanup();
//     //     exit(1);
//     // }

//     // // Listen for incoming connections
//     // if (listen(server_socket, 5) == SOCKET_ERROR) {
//     //     cerr << "Listening failed." << endl;
//     //     closesocket(server_socket);
//     //     WSACleanup();
//     //     exit(1);
//     // }

//     // cout << "Server is running, waiting for connections..." << endl;

//     // // Accept clients and spawn threads to handle them
//     // while (server_running.load()) {
//     //     SOCKET client_socket = accept(server_socket, NULL, NULL);
//     //     if (client_socket == INVALID_SOCKET) {
//     //         cerr << "Client connection failed." << endl;
//     //         continue;
//     //     }

//     //     // Create a thread for each client
//     //     char buffer[MAX_BUFFER_SIZE] = "accepted";
//     //     send(client_socket,(const char *)buffer,sizeof(buffer),0);
//     //     char buffer2[MAX_BUFFER_SIZE] = "i am";
//     //     // send(client_socket,(const char *)buffer2,sizeof(buffer2),0);
//     //     // char buffer3[MAX_BUFFER_SIZE] = "nour";
//     //     // send(client_socket,(const char *)buffer3,sizeof(buffer3),0);
//     //     thread(handle_client, client_socket).detach();
//     // }

//     closesocket(auth_req_socket);
//     WSACleanup();
// }