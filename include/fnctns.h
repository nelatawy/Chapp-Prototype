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


int bind_to_this_server(SOCKET server_socket, sockaddr_in server_addr){

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        cerr << "Binding failed." << endl;
        cout << "ErrCode : " << WSAGetLastError();
        return BIND_ERR;
    }
    return 1;
}


int assign_and_bind_sock(SOCKET& sock, sockaddr_in address){
    // Create server socket
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        cerr << "Socket creation failed." << WSAGetLastError() <<  endl;
        return SOCKCR_ERR;
        
    }

    // Bind socket to the server address
    cout << "before binding for authentication\n";
    if(bind_to_this_server(sock, address) == BIND_ERR){
        return BIND_ERR;
    }

    cout << "bound for msgs\n";
    return 1;

    // Listen for incoming connections
}

void check_internet_connection(){
    int err = WSAGetLastError();
    if (err == WSAENETDOWN || err == WSAEHOSTUNREACH || err == WSAENETUNREACH)
    {
        cerr << "No Internet... Please Check Your Internet Connection" << endl;
    }
    
}

void set_addr(sockaddr_in* server_addr, u_short port, const char* address){
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(port);
    server_addr->sin_addr.s_addr = inet_addr(address);
}

void* chk_wrt_msgs(SOCKET socket) {
    char buffer[MAX_BUFFER_SIZE];
    while (1)
    {   

        if(recv(socket,buffer,MAX_BUFFER_SIZE,0) > 0){
            cout << "Incoming ==>" << buffer << endl;
        }
    }
    
}

void send_ping(SOCKET socket){
    char temp = 'a';
    if (send(socket,(char*)&temp,1,0) == SOCKET_ERROR)
    {
        cout << "sending issue" << WSAGetLastError() << endl;
    }
    
}

bool try_gain_connection(SOCKET& socket, sockaddr_in server_addr, string approved_msg){

    for(int i = 0; i < 5; i++){
        if (connect(socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            check_internet_connection();
            cerr << "Connection to server failed.... Retrying in 5 seconds" << endl;
            Sleep(5000);
        }
        else{
            cout << approved_msg << endl;
            return true;
        }
    }
    return false;
}


int ensure_connection(SOCKET& socket, sockaddr_in server_addr, string server_name){
    char buffer;
    if(send(socket,&buffer,0,MSG_PEEK) == SOCKET_ERROR){
        if(!try_gain_connection(socket,server_addr, server_name)){
            cout << "Connection to server failed";
            return SRVR_DISCNT;
        }
    }
    return  1;
}

void terminate_and_close_this_socket(SOCKET& socket){
    cout << "An Issue Occured and program will terminate in 5 seconds ..we are sorry \n";
    closesocket(socket);
    WSACleanup();
    exit(1);
}