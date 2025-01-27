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
    //creating socket
    cout << "--------------STEP 1--------------" << endl ;
    int client_socket = INVALID_SOCKET;
    client_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET)
    {
        cout << "Error Creating Client Socket" << WSAGetLastError() << endl ;
        WSACleanup();
        exit(0);
    }
    else {
        cout << "Client Socket Created Successfully" << endl;
    }
    //connecting to server
    cout << "--------------STEP 2--------------" << endl ;
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET ;
    server_addr.sin_port = htons(55555);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client_socket,(sockaddr*)&server_addr,sizeof(server_addr)) == SOCKET_ERROR)
    {

        cout << "Failed to Connect ErrCode: " << WSAGetLastError() << endl;
        closesocket(client_socket);
        WSACleanup();
        exit(0);
    }
    else {
        cout << "Successfully Connected to Server Whose IP : 127.0.0.1" << endl;
    }
    
    char temp;
    cin >> temp;
    closesocket(client_socket);
    WSACleanup();
}