#pragma once

// Error Codes
#define SRVR_DISCNT 404
#define BIND_ERR 987
#define SOCKCR_ERR 789
#define LISN_ERR 289
#define ACC_ERR 314

// Authentication Requests
#define LOGIN_REQ 50
#define SIGNUP_REQ 51

// Buffer Sizes
#define MAX_CRED_SIZE 250
#define MAX_BUFFER_SIZE 512
#define REQ_STRLEN   MAX_CRED_SIZE + INET_ADDRSTRLEN

// Message Types
#define MSG_BROADCAST 100
#define MSG_PRIVATE 101
#define MSG_AUTH 102
#define MSG_STATUS 103

// Struct for messages
struct Message {
    int type;
    char sender[50];
    char receiver[50]; 
    char content[MAX_BUFFER_SIZE];
};