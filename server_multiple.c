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
#define MAX_CLIENTS 32
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

    //setup for multiple connections
    char multiple = !0;
    res = setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &multiple, sizeof(multiple)); // So_REUSEADDR -> allows local address reuse
    if(res < 0){
        printf("Multiple client setup failed: %d\n", WSAGetLastError());
        closesocket(listener);
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


    // MAIN LOOP ************************************************************************************************
    //variables
    fd_set socketSet;               //set of active clients, fd -> file descriptor, will return true if something is in the s et only if socket desciptor is activated    
    SOCKET clients [MAX_CLIENTS];   //array of clients
    int curNoClients = 0;          //size of the array          

    SOCKET sd, max_sd; //placeholders
    int clientAddrlen;
    struct sockaddr_in clientAddr;
    char running = !0;

    char recvbuf[BUFLEN];


    char *welcome = "Welcome to the server";
    char *goodbye = "Bye bye...";
    int welcomeLength = strlen(welcome);
    int goodbyeLength = strlen(goodbye);

    //clear client array
    memset(clients, 0, MAX_CLIENTS * sizeof(SOCKET));

    while(running){
        FD_ZERO(&socketSet); //clear the set

        FD_SET(listener, &socketSet); // add listener socket

        for(int i = 0; i < MAX_CLIENTS; ++i){
            sd = clients[i];
            if (sd > 0){
                FD_SET(sd, &socketSet); //add active client to set
            }

            if(sd > max_sd){
                max_sd = sd;
            }
        }

        int activity = select(max_sd + 1, &socketSet, NULL, NULL, NULL);
        if (activity < 0){
            continue;
        }

        //determine if listene has activity
        if(FD_ISSET(listener, &socketSet)){ //returns non zero value if socket is active(checking for socket activity)
            //accept connection
            sd = accept(listener, NULL, NULL);

            if(sd == INVALID_SOCKET){
                printf("Error accepting; %d\n", WSAGetLastError());
            }

            //get client info
            getpeername(sd, (struct sockaddr*)&clientAddr, &clientAddrlen);
            printf("Client connected [%s:%d]\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
            //welcome message
            sendRes = send(sd, welcome, welcomeLength, 0);
            if(sendRes != welcomeLength){
                printf("Error sending: %d\n", WSAGetLastError());
                shutdown(sd, SD_BOTH);
                closesocket(sd);
            }

            //add to array
            if(curNoClients >= MAX_CLIENTS){
                //list full
                printf("Full\n");
                shutdown(sd, SD_BOTH);
                closesocket(sd);
            }else{
                //scan through list
                int i;
                for(i = 0; i < MAX_CLIENTS; i++){
                    if(!clients[i]){
                        clients[i] = sd;
                        printf("Added to the list at index [%d]\n ", i);
                        curNoClients++;
                        break;
                    }
                }

                if (sendRes != welcomeLength){
                    printf("Error sending: %d\n", WSAGetLastError());
                    shutdown(sd, SD_BOTH);
                    closesocket(sd);
                    clients[i] = 0;
                    curNoClients--;
                }
            }
        }
            //iterate though clients
            for(int i = 0; i < MAX_CLIENTS; ++i){
                if(!clients[i]){
                    continue;
                }
                sd = clients[i];
                if (FD_ISSET(sd, &socketSet)){
                    //get message
                    res = recv(sd, recvbuf, BUFLEN, 0);
                    if(res > 0){
                        //print message
                        recvbuf[res] = '\0'; // string terminator message
                        printf("Received (%d): %s\n", res, recvbuf);

                        //test if quit command
                        if(!memcmp(recvbuf, "/quit", 5 * sizeof(char))){
                            running = 0;
                            break;
                        }

                        //echo message
                        sendRes = send(sd, recvbuf, res, 0);
                        if(sendRes == SOCKET_ERROR){
                            printf("echo failed: %d\n", WSAGetLastError());
                            shutdown(sd, SD_BOTH);
                            closesocket(sd);
                            clients[i] = 0;
                            curNoClients--;
                        }
                    }else{
                        // close message
                        getpeername(sd, (struct sockaddr *)&clientAddr, &clientAddrlen);
                        printf("Client disconnected [%s:%d]\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
                        shutdown(sd, SD_BOTH);
                        closesocket(sd);
                        clients[i] = 0;
                        curNoClients--;
                    }
                }
            }
    }




    //CLEANUP*********************************************************************************************************

    //disconnect all clients
    for(int i = 0; i < MAX_CLIENTS; ++i){
        if(clients[i] > 0){
            sendRes = send(clients[i], goodbye, goodbyeLength, 0);
            shutdown(clients[i], SD_BOTH);
            closesocket(clients[i]);
            clients[i] = 0;
        }
    }

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