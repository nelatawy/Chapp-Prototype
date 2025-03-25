#include "../include/stdafx.h"
#include "../include/fnctns.h"

#define AUTH_SERVER_PORT 55555
#define MSG_SERVER_PORT 9090
#define SERVER_IP "127.0.0.1"  // Server IP address on local machine
#define AUTH_ACCEPT 1


#define LOGIN_REQ 50
#define SIGNUP_REQ 51

#define MAX_CRED_SIZE 250
#define MAX_BUFFER_SIZE 2*(MAX_CRED_SIZE) + sizeof(int)

SOCKET client_msg_sock;
sockaddr_in msg_server_addr;

SOCKET auth_sock;
sockaddr_in auth_server_addr;

typedef
struct auth_req{
    uint32_t op_code;
    char username[MAX_CRED_SIZE];
    char password[MAX_CRED_SIZE];

}msg;


void* chk_wrt_msgs(SOCKET socket) {
    char buffer[MAX_BUFFER_SIZE];
    while (1)
    {   

        if(recv(socket,buffer,MAX_BUFFER_SIZE,0) > 0){
            cout << "Server ==>" << buffer << endl;
        }
    }
    
}

void connect_to_auth_server() {

    auth_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (auth_sock == INVALID_SOCKET) {
        cerr << "Socket creation failed." << endl;
        cleanup_winsock();
        exit(1);
    }


    auth_server_addr.sin_family = AF_INET;
    auth_server_addr.sin_port = htons(AUTH_SERVER_PORT);
    auth_server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    ensure_connection(auth_sock,auth_server_addr, "<auth>");
    //will only terminate program if tried connection and failed 5 times
}

void send_ping(SOCKET socket){
    char temp = 'a';
    if (send(socket,(char*)&temp,1,0) == SOCKET_ERROR)
    {
        cout << "sending issue" << WSAGetLastError() << endl;
    }
    
}
void send_auth_creds() {

    // ensure_connection(auth_sock,auth_server_addr);
    auth_req request;
    
    cout << "Enter your Username: ";
    cin.getline(request.username,MAX_CRED_SIZE);

    cout << "Enter your password: ";
    cin.getline(request.password,MAX_CRED_SIZE);

    request.op_code = LOGIN_REQ;

    if (send(auth_sock, (const char*)&request, sizeof(request), 0) == SOCKET_ERROR) {
        cerr << "Failed to send credentials to server." << endl;
        closesocket(auth_sock);
        cleanup_winsock();
        exit(1);
    }
}


bool recv_auth_approval(){

    char confirmation_flag[MAX_BUFFER_SIZE];
    if (recv(auth_sock,(char*)confirmation_flag,sizeof(confirmation_flag),0)  == SOCKET_ERROR){
        cerr << "Server Disconnected" << endl;
        ensure_connection(auth_sock,auth_server_addr, "<auth> regained");
        cout << "Server Reconnected ...Please Try Again";
        return false;
    }
    else if(strcmp("accepted",confirmation_flag) == 0){
        cout << "user found\n";
        return true;
    }
    else{
        cout << "Your Credentials Are Incorrect.. Please Try Again" << endl;
        return false;
    }
}


void connect_to_msg_server() {
    client_msg_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_msg_sock == INVALID_SOCKET) {
        cerr << "Socket creation failed." << endl;
        cleanup_winsock();
        exit(1);
    }

    msg_server_addr.sin_family = AF_INET;
    msg_server_addr.sin_port = htons(MSG_SERVER_PORT);
    msg_server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    ensure_connection(client_msg_sock,msg_server_addr, "<msg>");
    //if not exited it means i'm now connected so i'll proceed
}

void launch_auth_protocol(){ // 1 if successful ,0 if not

    connect_to_auth_server(); //will only proceed if connection is established
    cout << "connected to auth server\n";
    int connecion_flag;
    while(1){
        send_auth_creds();//will auto-terminate program if connection was lost and did not come back after 5 tries
        cout << "Sent Creds\n";
        if(recv_auth_approval()){ // authenticated successfully
            cout << "Auth approval rcvd" << endl;
            // connect_to_msg_server();
            closesocket(auth_sock);
            break;
        }
    }
    //now i'm ready to connect to other services
}

void send_message(char* message) {

    // ensure_connection(client_msg_sock,msg_server_addr);
    if (send(client_msg_sock, (char*)&message, MAX_BUFFER_SIZE, 0) == SOCKET_ERROR) {
        cerr << "Error sending message." << WSAGetLastError() << endl;
        closesocket(client_msg_sock);
        cleanup_winsock();
        exit(1);
    }
    cout << "sent : " << message << endl;
}

//for the reading thread
void receive_message() {
    char buffer[MAX_BUFFER_SIZE];
    int bytes_received;
    while (1) {
        bytes_received = recv(client_msg_sock, buffer, sizeof(buffer), 0);
        if (bytes_received == SOCKET_ERROR) {
            ensure_connection(client_msg_sock,msg_server_addr, "<msg> regained"); //tries to reconnect to server 5 times
        }

        buffer[bytes_received] = '\0';
        cout << "Server: " << buffer << endl;
    }
}

void handle_user_input() {
    
    while (true) {
        cout << "Enter message: ";
        char message[MAX_BUFFER_SIZE];
        cin.getline(message,MAX_BUFFER_SIZE);

        if (message == "/exit") {
            cout << "Exiting chat..." << endl;
            break;
        }

        send_message(message);
    }
}

int main() {
    initialize_winsock();
    cout << "step1\n";
    launch_auth_protocol();
    cout << "step2\n";
    connect_to_msg_server();
    cout << "step3\n";
    // thread(receive_message).detach(); // Run message receiving in a separate thread
    send_ping(client_msg_sock);
    thread read_thread(chk_wrt_msgs,client_msg_sock);
    char buffer_send[MAX_BUFFER_SIZE],buffer_recv[MAX_BUFFER_SIZE];

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