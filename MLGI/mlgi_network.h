#ifndef mlgi_network
#define mlgi_network

// Defining windows wocket...
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
#include "mlgi_include.h"

// Defining port // Not assigned according to wikipedia
#define DEFAULT_PORT "28012"
#define MAX_IP_LENGTH 15
#define DEFAULT_BUFLEN 131072

#define CONNECTION_CLEAN 0
#define WSASTARTUP_FAILED 1
#define GETADDRINFO_FAILED 2
#define SOCKET_INVALID 3
#define ERROR_SOCKET 4
#define CONNECTION_FAILED 5

#define SEND_FAILED 0
#define NOTHING_RECV 0

int initiateConnection(char* IP_ADDRESS);
int pushSend(char* SendBuf);
int getRecv(char* &RecvBuf);

void network_loop(_network *network, _gui *gui);

#endif