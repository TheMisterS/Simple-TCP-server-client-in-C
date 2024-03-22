#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h> 
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <process.h>
#include <stdatomic.h> //Atomic variables library

#pragma comment(lib, "Ws_32.lib")

#define BUFLEN 512
#define PORT 27015
#define ADDRESS "127.0.0.1" //locahost

//global running variable
_Atomic char running = 0; //default = false

DWORD WINAPI sendThreadFunc(LPVOID lpParam);

int main (){
    printf("Hello World\n");

    int res;
    //INITIALIZATION ************************************************************************************************
    WSADATA wsaData;
    res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if(res){
        printf("Startup failed; %d\n", res);
        return 1;
    }
    //************************************************************************************************
    
    //SETUP CLIENT SOCKET ************************************************************************************************

    SOCKET client;
    client = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP);
    if(client == INVALID_SOCKET){
        printf("Error with construction of the socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    //connect to address
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ADDRESS);
    address.sin_port = htons(PORT);
    res = connect(client, (struct sockaddr *)&address, sizeof(address)); //same to bind in serve config
    if (res == SOCKET_ERROR){
        printf("Socket binding failed: %d\n", WSAGetLastError());
        closesocket(client);
        WSACleanup();
        return 1;
    }else if(client == INVALID_SOCKET){
        printf("Socket binding failed, invalid socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;        
    }

    printf("Connected to a server %s:%d\n", ADDRESS, PORT);
    running = !0;
    //************************************************************************************************

    // MAIN LOOP ************************************************************************************************

    //start thead
    DWORD threadID;
    HANDLE sendThread = CreateThread(NULL, 0, sendThreadFunc, &client, 0, &threadID);
    if (sendThread){
        printf("Send thread started with thread ID: %d\n", threadID);
    }else{
        printf("Send thread failed: %d\n", GetLastError());
    }

    //recieve loop

    char recvbuf[BUFLEN];

    do {
        res = recv(client, recvbuf, BUFLEN, 0);
        recvbuf[res] = '\0';
        if(res > 0){
            printf("Received (%d): %s\n", res, recvbuf);           
        }else if(!res){
            printf("Connection closed \n");
            running = 0;
        }else{
            printf("Receive failed %d\n", WSAGetLastError());
            running = 0;
        }
    }while (res > 0 && running);
    running = 0;


    //connection finished, close thread
    if(CloseHandle(sendThread)){
        printf("Send thread closed succesfully.\n");
    }
    //************************************************************************************************
    
    //CLEAN UP ************************************************************************************************
    res = shutdown(client, SD_BOTH);
    if (res == SOCKET_ERROR){
        printf("Shutdown failed: %d\n", WSAGetLastError());
        closesocket(client);
        WSACleanup();
        return 1;
    }
    closesocket(client);
    WSACleanup();

    //************************************************************************************************
    return 0;
}

DWORD WINAPI sendThreadFunc(LPVOID lpParam){
    SOCKET client = *(SOCKET*)lpParam;

    char sendBuffer[BUFLEN];
    int sendBufferLength, res;

    while(running){
        scanf("%s", sendBuffer);
        sendBufferLength = strlen(sendBuffer);
        res = send(client, sendBuffer, sendBufferLength, 0);

        if(res != sendBufferLength){
            printf("Send failed.");
            break;
        }else if(!memcmp(sendBuffer, "/leave", 6)){
            running = 0;
            break;
        }
    }
    return 0;
}
