#include "../include/stdafx.h"

using namespace std;

void* chk_wrt_msgs(SOCKET socket) {
    char buffer[250];
    while (1)
    {   

        if(recv(socket,buffer,250,0) > 0){
            cout << "Client ==> " << buffer << endl;
        }
        Sleep(1e-2);
    }
    
}

int main(){
    //setting up the dll
    WSADATA wsaData ;
    WORD wVersionRequested = MAKEWORD(2,2);
    int wsaerr = WSAStartup(wVersionRequested,&wsaData);
    if (wsaerr == 0)
    {
        cout << "Ws2_32 DLL Found" << endl ;
    }
    else {
        cout << "DLL NOT FOUND" << endl;
        system("pause");
        // exit(0);
    }
    
    int port = 8080;
    //creating socket
    cout << "--------------STEP 1--------------" << endl ;
    int server_socket = INVALID_SOCKET;
    server_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET)
    {
        cout << "Error Creating Server Socket" << WSAGetLastError() << endl ;
        WSACleanup();
        system("pause");
        // exit(0);
    }
    else {
        cout << "Socket Created Successfully" << endl;
    }
    //binding
    cout << "--------------STEP 2--------------" << endl ;
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET ;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(server_socket,(sockaddr*)&server_addr,sizeof(server_addr)) == 0)
    {
        cout << "Successfully bound to 127.0.0.1" << endl;

    }
    else {
        cout << "failed to bound ErrCode: " << WSAGetLastError() << endl;
        closesocket(server_socket);
        WSACleanup();
        // exit(0);
        system("pause");
    }
    //listening for connections
    cout << "--------------STEP 2--------------" << endl ;
    if(listen(server_socket,1) == SOCKET_ERROR){
        cout << "Error While Listening" << WSAGetLastError() << endl;
        closesocket(server_socket);
        WSACleanup();
        // exit(0);
        system("pause");
    }
    else{
        cout << "Listening Initiated Successfully" << endl;
    }
    SOCKET accept_socket ;
    if ((accept_socket = accept(server_socket,NULL,NULL))==INVALID_SOCKET){
        cout << "Error While Creating Accepting Socket" << WSAGetLastError() << endl;
        closesocket(server_socket);
        WSACleanup();
        // exit(0);
        system("pause");
    }
    else {
        cout << "Accepted Socket Created --> Connection_Estabilished" << endl;
    }

    char buffer_send[250],buffer_recv[250];

    thread read_thread(chk_wrt_msgs,accept_socket);

    while(1){

        cin.getline(buffer_send,250);
        if (send(accept_socket,buffer_send,250,0) < 0){
            cout << "Error sending the message to Client" << WSAGetLastError() << endl;
        }
        else {
            // cout << "Message Sent" << endl;
            cout << "Server ==> " << buffer_send << endl;
        }
        
    }

    read_thread.join();
    closesocket(server_socket);
    system("pause");
    WSACleanup();
}

