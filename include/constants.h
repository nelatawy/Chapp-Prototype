#define SRVR_DISCNT 404
#define BIND_ERR 987
#define SOCKCR_ERR 789
#define LISN_ERR 289
#define ACC_ERR 314


#define LOGIN_REQ 50
#define SIGNUP_REQ 51

#define MAX_CRED_SIZE 250
#define MAX_BUFFER_SIZE 2*(MAX_CRED_SIZE) + sizeof(int)
#define REQ_STRLEN   MAX_CRED_SIZE + INET_ADDRSTRLEN
