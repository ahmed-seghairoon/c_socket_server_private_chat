#include<Windows.h>
#include<stdio.h>
#include<stdlib.h>

#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN

#pragma comment(lib, "ws2_32.lib")

void wslInit();
int createTCPIpv4Socket();
struct sockaddr_in* CreateIPv4Address(char* ip, int port);

int main(){

    wslInit();

    int serverSocketFD = createTCPIpv4Socket();
    struct sockaddr_in* serverAddress = CreateIPv4Address("127.0.0.1", 2000);


    /*
        bind function will bind the proccess to the port from the operating system
        will return result if its zero that means it wass successful else not successful
    */ 
    int result = bind(serverSocketFD, (SOCKADDR *) serverAddress, sizeof(*serverAddress));

    if (result == 0)
        printf("socket was bound successfully\n");
    else{
        printf("socket was not bound successfully\n");
        exit(1);
    }

    
    /*
        start listening to incoming sockets requests
        10 is the number of backlog (socket requests queues)
        it will return a result too
    */
    int listenResult = listen(serverSocketFD, 10);

    if (result == 0)
        printf("listening was successfully\n");
    else{
        printf("listening was not bound successfully\n");
        exit(1);
    }

    struct sockaddr_in clientAddress;
    int clientAddressSize = sizeof(clientAddress);
    int clientSocketFD = accept(serverSocketFD, (SOCKADDR *) &clientAddress, &clientAddressSize);


    char buffer[1024];
    // keep reciving messages from clients
    while (1)
    {
        
        SSIZE_T amountRecived = recv(clientSocketFD, buffer, 1024, 0);
        if (amountRecived > 0)
        {
            buffer[amountRecived] = 0;
            printf("Response was %s", buffer);
        }
        
        // if recived amount is zero there is an error or the client closed the connection
        //break from the loop then shutdown the server
        if (amountRecived == 0)
            break;
        
        
       
    }
    

    closesocket(clientSocketFD);
    // shut down the server with option 2 shut down receive and send
    shutdown(serverSocketFD, 2);
   
    
    return 0;
}

// reusable function used to initialize the Windows Sockets library
void wslInit(){
    // WSAStartup function is used to initialize the Windows Sockets library
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        exit(1);
    }
}

// reusable function used to create the ipv4 socket
int createTCPIpv4Socket(){
    /*
        AF_INET means IPv4
        SOCK_STREAM means TCP socket
        We passed 0 as the protocol to determine the layer beneath the transport layer (IP Layer)
        IF the return number is not a negative number that means the socket have been created successfully
        and a socket file descriptor will be returned, we can use it the same way we use files in c
    */
    int socketFD = socket(AF_INET, SOCK_STREAM, 0);

    if (socketFD == -1) {
        // WSAGetLastError() is used to retrieve the error code when a socket-related function fails.
        printf("Error creating socket. Error code: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    return socketFD;
}


// reusable function used to create the ipv4 address
struct sockaddr_in* CreateIPv4Address(char* ip, int port){
    struct sockaddr_in* address = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
    address->sin_port = htons(port);  //server port, htons function wraps the port number and it get the bytes inside it
    address->sin_family = AF_INET;  //IPv4
    address->sin_addr.s_addr = inet_addr(ip); // the ip address the inet_addr function converets the string to an unsigend int
    return address;
}

/*
    use command  
    "gcc SocketServer.c -o socketserver -lws2_32" 
    to compile 
*/