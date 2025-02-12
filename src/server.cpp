#include "../include/stdafx.h"
#include "../include/fnctns.h"
#include "../include/constants.h"

using namespace std;

#define AUTH_SERVER_IP "127.0.0.1"
#define MSG_SERVER_IP "127.0.0.1"

#define AUTH_SERVER_PORT 55556

#define MSG_SERVER_AUTH_PORT 8080
#define MSG_SERVER_CLIENT_PORT 9090


map<string, SOCKET> client_list;
atomic<bool> server_running(true);  // Flag to control server shutdown


typedef
struct create_sock_req{
    char client_username[MAX_CRED_SIZE];
    char ip_address[INET_ADDRSTRLEN];
}create_sock_request;


void rcv_frwrd_msg(SOCKET client_socket , char* username)  {
    char temp_username[MAX_CRED_SIZE];
    strcpy(temp_username,username); //to work on a local copy because the parameter passed (username) will change in the next iteration while this is still working
                                    //while the socket does not need this precaution
    message msg;
    while (recv(client_socket, (char*)&msg, sizeof(message), 0) > 0) {
        cout << "[MSG] " << ": " << msg.content << endl;
        cout << "\n msg receiver : " << msg.receiver << endl;

        strcpy(msg.sender , username);
        if(send(client_list[msg.receiver],(char*)&msg,sizeof(msg),0)<0){
            cout << "forwarding err\n";
        }
    }
    closesocket(client_socket);
}

int manage_auth_requests(SOCKET &req_sock, SOCKET &msg_sock , sockaddr_in  server_addr, map<string,SOCKET>&sockmap){
    //NOTE : req sock must be existing and msg_sock must be bound
    if(ensure_connection(req_sock,server_addr,"auth server manage requests") == SRVR_DISCNT) return SRVR_DISCNT;

    if (listen(msg_sock, 5) == SOCKET_ERROR) {
        cerr << "Listening failed on msg port for authenticated connections." << WSAGetLastError() <<  endl;
        return LISN_ERR;
    }
    cout << "MSG_SOCK set to listening mode\n";
    while (1)
    {   
        char req_buffer[REQ_STRLEN];
        cout << "trying to recv\n";
        if (recv(req_sock,(char*)&req_buffer,sizeof(req_buffer),0) == SOCKET_ERROR)
        {
            if(ensure_connection(req_sock,server_addr,"auth server regained") == SRVR_DISCNT) 
                return SRVR_DISCNT;
            // will continue if connection was re-established
            continue;
        }
        //recieved successfully
        //will try to accept from the msg socket
        create_sock_req* request = (create_sock_req*) req_buffer;
        cout << "trying to accept\n";
        SOCKET accept_sock = accept(msg_sock,NULL,NULL) ;
        if (accept_sock == INVALID_SOCKET) {
            cerr << "accepting connection to client failed. .. waiting for other clients" << endl;
            continue;
            //to keep checking for other requests
        }
        //recieved successfully
        cout << "Client Accepted Handling Now In A New Thread\n";
        thread(rcv_frwrd_msg , accept_sock , request->client_username).detach();

        cout << request->client_username << "wants to connect .... Attempting" << endl;
        // client_list.insert_or_assign((string)request->client_username, accept_sock);
        client_list[request->client_username] = accept_sock;
    }
}


void shutdown_server() {
    cout << "Shutting down server..." << endl;
    server_running.store(false);
}

void handle_shutdown(){
    string input;
    while (true) {
        cout << "Enter '/shutdown' to stop the server: ";
        getline(cin, input);
        if (input == "/shutdown") {
            shutdown_server();
            Sleep(5000);
            exit(1);
            break;
        }
    }
}

int main() {
    //STEP 1
    initialize_winsock();
    cout << "Initialized Winsock Services\n";

    //STEP 2
    SOCKET msg_socket;
    SOCKET auth_req_socket;

    sockaddr_in auth_server_addr;
    sockaddr_in msg_server_auth_addr;
    sockaddr_in msg_server_client_addr;

    auth_req_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (auth_req_socket == INVALID_SOCKET) {
        cerr << "Socket creation with auth_server failed." << endl;
        Sleep(5000);
        WSACleanup();
        exit(1);
    }  
    cout << "auth socket created\n";


    set_addr(&msg_server_auth_addr,MSG_SERVER_AUTH_PORT,MSG_SERVER_IP);
    set_addr(&msg_server_client_addr,MSG_SERVER_CLIENT_PORT,MSG_SERVER_IP);
    set_addr(&auth_server_addr,AUTH_SERVER_PORT,AUTH_SERVER_IP);

    cout << "sockets and addresses set\n";


    //STEP 3
    if(assign_and_bind_sock(msg_socket, msg_server_client_addr) == 1){ // only needs a SOCKET without calling socket()
        cout << "assigned socket for client sockets...Successful\n";
    }else{
        cout << "assigned socket for client sockets...Failed\n";
        Sleep(5000);
        terminate_and_close_this_socket(msg_socket);
    }

    cout << "MSG_SOCK bound\n";

    thread(handle_shutdown).detach();
    //STEP 4 
    cout << "managing auth requests\n";
    int err_code_mng_auth = manage_auth_requests(auth_req_socket, msg_socket, auth_server_addr,client_list);
    if(err_code_mng_auth == SRVR_DISCNT || err_code_mng_auth == LISN_ERR ){
        cerr << "Managing Auth Request Inexpectedly Terminated\n";
        Sleep(3000);
        closesocket(auth_req_socket);
        closesocket(msg_socket);
        WSACleanup();
        exit(1);

    }


    WSACleanup();
    return 0;
}