#include "../include/stdafx.h"

using namespace std;


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
        exit(0);
    }
    
    int port = 55555;
    //creating socket
    cout << "--------------STEP 1--------------" << endl ;
    int server_socket = INVALID_SOCKET;
    server_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET)
    {
        cout << "Error Creating Server Socket" << WSAGetLastError() << endl ;
        WSACleanup();
        exit(0);
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
        exit(0);
    }
    //listening for connections
    cout << "--------------STEP 2--------------" << endl ;
    if(listen(server_socket,1) == SOCKET_ERROR){
        cout << "Error While Listening" << WSAGetLastError() << endl;
        closesocket(server_socket);
        WSACleanup();
        exit(0);
    }
    else{
        cout << "Listening Initiated Successfully" << endl;
    }
    SOCKET accept_socket ;
    if ((accept_socket = accept(server_socket,NULL,NULL))==INVALID_SOCKET){
        cout << "Error While Creating Accepting Socket" << WSAGetLastError() << endl;
        closesocket(server_socket);
        WSACleanup();
        exit(0);
    }
    else {
        cout << "Accepted Socket Created --> Connection_Estabilished" << endl;
    }

    char buffer_send[250],buffer_recv[250];
    
    while(1){
        // Sleep(10);
    
        if (recv(accept_socket,buffer_recv,250,0) < 0)
        {
            cout << "Error Receiving the message from server" << WSAGetLastError() << endl;
        }
        else {
            cout << "Client Sends " << buffer_recv << endl;
        }

        cout << "Enter The Message You Want To Send" << endl;
        cin.getline(buffer_send,250);
        if (send(accept_socket,buffer_send,250,0) < 0){
            cout << "Error sending the message to Client" << WSAGetLastError() << endl;
        }
        else {
            cout << "Message Sent" << endl;
        }
        
    }
    closesocket(server_socket);
    system("pause");
    WSACleanup();
}