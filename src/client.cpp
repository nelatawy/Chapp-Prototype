#include "../include/stdafx.h"
#include "../include/fnctns.h"
#include "../include/constants.h"

#define AUTH_SERVER_PORT 55555
#define MSG_SERVER_PORT 9090
#define SERVER_IP "127.0.0.1"  // Server IP address on local machine
#define AUTH_ACCEPT 1




typedef
struct auth_req{
    uint32_t op_code;
    char username[MAX_CRED_SIZE];
    char password[MAX_CRED_SIZE];

}msg;


int send_auth_creds(SOCKET socket, sockaddr_in server_addr) {

    // ensure_connection(auth_sock,auth_server_addr);
    auth_req request;
    
    cout << "Enter your Username: ";
    cin.getline(request.username,MAX_CRED_SIZE);

    cout << "Enter your password: ";
    cin.getline(request.password,MAX_CRED_SIZE);

    request.op_code = LOGIN_REQ;

    if (send(socket, (const char*)&request, sizeof(request), 0) == SOCKET_ERROR) {
        if(ensure_connection(socket,server_addr,"") == SRVR_DISCNT) return SRVR_DISCNT;
        return false;
    }
    return true;
}


int recv_auth_approval(SOCKET socket, sockaddr_in server_addr){

    char confirmation_flag[MAX_BUFFER_SIZE];
    if (recv(socket,(char*)confirmation_flag,sizeof(confirmation_flag),0)  == SOCKET_ERROR){
        cerr << "Server Disconnected" << endl;
        if(ensure_connection(socket, server_addr, "<auth server>") == SRVR_DISCNT){
            return SRVR_DISCNT;
        }
        cout << "Server Reconnected ...Please Try Again";
        return 0;
    }
    else if(strcmp("accepted",confirmation_flag) == 0){
        cout << "user found\n";
        return 1;
    }
    else{
        cout << "Your Credentials Are Incorrect.. Please Try Again" << endl;
        return 0;
    }
}


int launch_auth_protocol(SOCKET& auth_socket, sockaddr_in server_addr){ //true if authenticated

    if(ensure_connection(auth_socket, server_addr, "<auth server>") == SRVR_DISCNT) 
        return SRVR_DISCNT; //will only proceed if connection is established
    
    int connecion_flag;
    while(1){
        if (send_auth_creds(auth_socket,server_addr))
        {
            cout << "Creds Sent\n";
            if(recv_auth_approval(auth_socket,server_addr)){ // authenticated successfully
                cout << "Auth approval rcvd" << endl;
                break;
            }
        }
        cerr << "error sending credentials ...please try again";
    }
    return 1;
}

int main() {
    //STEP 1
    initialize_winsock();
    cout << "Initialized Winsock Services\n";


    //STEP 2
    SOCKET client_msg_sock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    SOCKET auth_sock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

    sockaddr_in auth_server_addr;
    sockaddr_in msg_server_addr;
    set_addr(&msg_server_addr,MSG_SERVER_PORT,SERVER_IP);
    set_addr(&auth_server_addr,AUTH_SERVER_PORT,SERVER_IP);
    cout << "sockets and addresses set\n";

    //STEP 3
    if(launch_auth_protocol(auth_sock,auth_server_addr) == SRVR_DISCNT){
        cerr << "server disconnected \n and program will now terminate\n";
        terminate_and_close_this_socket(auth_sock);
    }
    closesocket(auth_sock);


    cout << "Authentication Approved\n";


    //STEP 4
    if(ensure_connection(client_msg_sock,msg_server_addr,"Message Server") == SRVR_DISCNT){
    //NOTE : ensure connection in fnctns.h tries to connect/reconnect to server and gives SRVR_DISCNT on faiure 5 times
        terminate_and_close_this_socket(client_msg_sock);
    }
    cout << "Connected To Message Server\n";


    //STEP 5
    thread read_thread(chk_wrt_msgs,client_msg_sock); //chk_wrt_msg it keeps checking for new messages in fnctns.h

    char buffer_send[MAX_BUFFER_SIZE];

    while(1){

        cin.getline(buffer_send,MAX_BUFFER_SIZE);
        cout << "About to send ==> " << buffer_send << endl;

        if (send(client_msg_sock, (char*)buffer_send, MAX_BUFFER_SIZE,0) <= 0){
            cout << "Error sending the message to server" << WSAGetLastError() << endl;
        }
        else {
            cout << "Message Sent" << endl;
            cout << "Client ==> " << buffer_send << endl;
        }
        
    }

    read_thread.join();

    closesocket(client_msg_sock);
    cleanup_winsock();
    return 0;
}