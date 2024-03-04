#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // consolidated version if windows.h ()
#endif


#include <windows.h> 
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment(lib, "Ws_32.lib") //library file that has winsock libraries in it

#define BUFLEN 512 //length of the buffer
#define PORT 27015
#define ADDRESS "127.0.0.1" //local host

int main(){
    printf("hello world\n");

    int res, sendRes;

    // INITIALIZATION

    WSADATA wsaData; //configuration data
    res = WSAStartup(MAKEWORD(2, 2), &wsaData); //returns integer
    if(res){
        printf("Startup failed: %d\n", res);
        return 1;
    }
    //***************************************************************************************************************
    
    //SETUP SERVER***************************************************************************************************|
    //creating the socket
    SOCKET listener;
    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //INET -> IPv4 , SOCK_STREAM ->, IPPROTO_TCP -> TCP PROTOCL
    if(listener == INVALID_SOCKET){
        printf("Error with construction of the socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    //binding the socket
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ADDRESS); //convert selected IP to address
    address.sin_port = htons(PORT);
    res = bind(listener, (struct sockaddr*)&address, sizeof(address));
    if (res == SOCKET_ERROR){
        printf("Socket binding failed: %d\n", WSAGetLastError());
        closesocket(listener);
        WSACleanup();
        return 1;
    }

    //set listener mode
    res = listen(listener, SOMAXCONN); // SOMAXCONN = maximum amount of pending connetions(before we refuse connections)
    if(res == SOCKET_ERROR){
        printf("Listen failed: %d\n", WSAGetLastError());
        closesocket(listener);
        WSACleanup();
        return 1;
    }

    printf("Accepting on %s:%d\n", ADDRESS, PORT);

    //HANDLE A CLIENT ************************************************************************************************

    SOCKET client;
    struct sockaddr_in clientAddr;
    int clientAddrlen;
    client = accept(listener, NULL, NULL);
    if(client == INVALID_SOCKET){
        printf("Could not accept: %d\n", WSAGetLastError());
        closesocket(listener);
        WSACleanup();
        return 1;
    }

    // get client info
    getpeername(client, (struct sockaddr *)&clientAddr, &clientAddrlen);
    printf("Client connected at %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port)); //inet_ntoaddress/ntohs converts numerical address/port to string value

    //send welcome message
    char *welcome = "Welcome to the server: ";
    char *welcome = "Welcome to the server: ";
    sendRes = send(client, welcome, strlen(welcome), 0); //returns number of bytes sent
    if(sendRes != strlen(welcome)){
        printf("Error sending; %d\n", WSAGetLastError());
        shutdown(client, SD_BOTH);
        closesocket(client);
    }

    //receive messages
    char recvbuf[BUFLEN];
    int recvbuflen = BUFLEN;
    do {
        res = recv(client, recvbuf, recvbuflen, 0); // 0 -> no flags
        if(res > 0){ //if any bytes received
            recvbuf[res] = '\0'; // add string terminator
            printf("Message received: %s\n", recvbuf);
            if(!memcmp(recvbuf, "/quit", 5 * sizeof(char))){  //received quit command(memcmp, bcus putty includes random string so stringcmp might not work)
                printf("Closing connection. \n");
                break;
            }
            //echo message back
            sendRes = send(client, recvbuf, res, 0);
            if(sendRes != res){
                printf("Error sending; %d\n", WSAGetLastError());
                shutdown(client, SD_BOTH);
                closesocket(client);
                break;
            }            
        }else if(!res){
            // client disconnected
            printf("Closing connection \n");
            break;
        }else{
            printf("Receive failed: %d\n", WSAGetLastError());
            break;
        }
    } while(res > 0);

    //shutdown client
    res = shutdown(client, SD_BOTH);
    if(res == SOCKET_ERROR){
        printf("Client shutdown failed: %d\n", WSAGetLastError());
    }
    closesocket(client);
    
    //CLEANUP*********************************************************************************************************
    closesocket(listener);
    res = WSACleanup();
    if(res){
        printf("Cleanup failed: %d\n", res);
        return 1;
    }
    //***************************************************************************************************************
    printf("Bye bye....\n");
    return 0;
}