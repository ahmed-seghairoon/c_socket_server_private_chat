#include<Windows.h>
#include<stdio.h>
#include<stdlib.h>
#include <time.h>


#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN

#pragma comment(lib, "ws2_32.lib")


FILE *filePtr;


void writeLog(char *message){

    time_t now = time(NULL);

    char *ctime_no_newline;
    ctime_no_newline = strtok(ctime(&now), "\n");
    fprintf(filePtr, "[%s] %s\n",ctime_no_newline, message);
}


typedef struct {
    int AcceptedSocketFD;
    struct sockaddr_in address;
    int error;
    int acceptedSuccessfully;
    char *username;
}AcceptedSocket;


struct Node
{
    AcceptedSocket *data;
    struct Node *next;
};

typedef struct Node Node;

Node *clientsHead = NULL;

Node* addFirst(Node *head, AcceptedSocket *data){
    Node *newNode = (Node*) malloc(sizeof(Node));
    newNode->data = data;

    if (!head){
        newNode->next = NULL;
        return newNode;
    }

    newNode->next = head;
    
    return newNode;
}

Node* addLast(Node *head, AcceptedSocket *data){
    Node *newNode = (Node*) malloc(sizeof(Node));
    newNode->data = data;
    newNode->next = NULL;

    if (!head)
        return newNode;

    Node *temp = (Node*) malloc(sizeof(Node));
    temp = head;

    while (temp->next)
        temp = temp->next;

    temp->next = newNode;

    return head;
}


void printList(Node *head){
    while (head)
    {
        printf("%d -> ", head->data->AcceptedSocketFD);
        head = head->next;
    }

    printf("\n");
}


Node *deleteNode(Node *head, int socketFD){

    if (!head)
        return head;    

    if (head->data->AcceptedSocketFD == socketFD)
    {
        Node *temp = head->next;
        free(head);
        head = temp;
        return head;
    }
    
    Node* traverse = head;

    while (traverse->next)
    {
        if (traverse->next->data->AcceptedSocketFD == socketFD)
        {
            Node *temp = traverse->next->next;
            free(traverse->next);
            traverse->next = temp;
            return head;
        }

        traverse = traverse->next;
    }
}


int listSize(Node *head){
    int count = 0;

    while (head)
    {
        count++;
        head = head->next;
    }

    return count;
}




// reusable function used to initialize the Windows Sockets library
void wslInit(){
    writeLog("initializing wsl");
    // WSAStartup function is used to initialize the Windows Sockets library
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        writeLog("WSAStartup failed");
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
        writeLog("Error creating socket.");
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


AcceptedSocket * acceptIncomingConnection(int serverSocketFD){
    struct sockaddr_in clientAddress;
    int clientAddressSize = sizeof(clientAddress);
    int clientSocketFD = accept(serverSocketFD, (SOCKADDR *) &clientAddress, &clientAddressSize);

    AcceptedSocket * acceptedSocket = (AcceptedSocket*)malloc(sizeof(AcceptedSocket));
    acceptedSocket->address = clientAddress;
    acceptedSocket->AcceptedSocketFD = clientSocketFD;

    // if accept function returns zero or less that means there was an error
    acceptedSocket->acceptedSuccessfully = clientSocketFD > 0;

    // if its not accepted successfully we will store the error
    if (!acceptedSocket->acceptedSuccessfully)
        acceptedSocket->error = clientSocketFD;
    

    return acceptedSocket;
}


char *trimwhitespace(char *str)
{
  char *end;

  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)
    return str;

  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  end[1] = '\0';

  return str;
}


void sendRecivedMessageToOtherClients(AcceptedSocket *sender,char *buffer){

   
    char *recepientName = strtok(buffer, ">");
    char *context = strtok(NULL, ">");

    recepientName = trimwhitespace(recepientName);
    context = trimwhitespace(context);

    Node* temp = clientsHead;
    
    char message[2048];

    sprintf(message, "%s: %s",sender->username ,context);

    int userfound = 0;

    char request[2048];

    sprintf(request,"Request: from %s to %s: %s\n",sender->username, recepientName ,context);
    printf(request);
    writeLog(request);

    while (temp != NULL)
    {
        if (strcmp(temp->data->username, recepientName) == 0){
            int sendResults = send(temp->data->AcceptedSocketFD, message, strlen(message), 0);
            if (sendResults != -1){
                send(sender->AcceptedSocketFD, "200 - message sent successfuly", strlen("200 - message sent successfuly"), 0);
                printf("Response: 200 - message sent successfuly\n");
                writeLog("Response: 200 - message sent successfuly");
            }else if (sendResults != strlen(message)){
                send(sender->AcceptedSocketFD, "499 - client closed the connection ungracefully", strlen("499 - client closed the connection ungracefully"), 0);
                printf("Response: 499 - client closed the connection ungracefully\n");
                writeLog("Response: 499 - client closed the connection ungracefully");
            }
            userfound = 1;
        }
        temp = temp->next;
    }

    if (userfound == 0)
    {
        send(sender->AcceptedSocketFD, "404 - user not found", strlen("404 - user not found"), 0);
        printf("Response: 404 - user not found\n");
        writeLog("Response: 404 - user not found");
    }
    

}

void setUsername(char *username, int clientSocketFD){
    Node* temp = clientsHead;
    username = username + 9;
    
    while (temp != NULL)
    {
        if (temp->data->AcceptedSocketFD == clientSocketFD){
            temp->data->username = (char*)malloc(sizeof(char) * 1024);
            strcpy(temp->data->username, username);
        }
        
        temp = temp->next;
    }
}


DWORD WINAPI sendClientList(LPVOID lpParameter){

    AcceptedSocket *sender = (AcceptedSocket *)lpParameter;

    Node* temp = clientsHead;
    char joinedMessage[2048];
    sprintf(joinedMessage, "%s connected to the server", sender->username);
    printf("%s\n",joinedMessage);

    writeLog(joinedMessage);


    while (temp != NULL)
    {
        if (temp->data->AcceptedSocketFD != sender->AcceptedSocketFD)
        {
            send(temp->data->AcceptedSocketFD, joinedMessage, strlen(joinedMessage), 0);
        }
        temp = temp->next;
    }
    
    temp = clientsHead;
    char message[10000] = "connected users list:\n_______________________";

    while (temp != NULL)
    {
        sprintf(message, "%s\n%s", message,temp->data->username);
        temp = temp->next;
    }

    sprintf(message, "%s\n%s", message,"_______________________\n");

    temp = clientsHead;

    while (temp != NULL)
    {
        send(temp->data->AcceptedSocketFD, message, strlen(message), 0);
        temp = temp->next;
    }

    printf("%s",message);
    writeLog(message);
}

void sendClientListOnANewThread(AcceptedSocket *clientSocket){
    HANDLE thread = CreateThread(NULL, 0, sendClientList, (LPVOID)clientSocket, 0, NULL);
}


void dissconnectUser(AcceptedSocket *client){

    Node* temp = clientsHead;
    char dissconnectMessage[2048];
    sprintf(dissconnectMessage, "%s dissconnected from the server", client->username);

    clientsHead = deleteNode(clientsHead, client->AcceptedSocketFD);
    closesocket(client->AcceptedSocketFD);

    printf("%s\n",dissconnectMessage);
    writeLog(dissconnectMessage);

    while (temp != NULL)
    {
        if (temp->data->AcceptedSocketFD != client->AcceptedSocketFD)
        {
            send(temp->data->AcceptedSocketFD, dissconnectMessage, strlen(dissconnectMessage), 0);
        }
        temp = temp->next;
    }
    
    temp = clientsHead;
    char message[10000] = "connected users list:\n_______________________";

    while (temp != NULL)
    {
        sprintf(message, "%s\n%s", message,temp->data->username);
        temp = temp->next;
    }

    sprintf(message, "%s\n%s", message,"_______________________\n");

    temp = clientsHead;

    while (temp != NULL)
    {
        send(temp->data->AcceptedSocketFD, message, strlen(message), 0);
        temp = temp->next;
    }

    printf("%s",message);
    writeLog(message);

}

DWORD WINAPI receiveAndPrintIncommingMessages(LPVOID lpParameter){
    AcceptedSocket *sender = (AcceptedSocket *)lpParameter;
    char buffer[1024];
    // keep reciving messages from clients
    while (1)
    {
        
        SSIZE_T amountRecived = recv(sender->AcceptedSocketFD, buffer, 1024, 0);

        if (amountRecived > 0)
        {
            buffer[amountRecived] = 0;

            if (strstr(buffer, "/setname:") != NULL) {
                printf("%s\n", buffer);
                writeLog(buffer);
                setUsername(buffer, sender->AcceptedSocketFD);
                sendClientListOnANewThread(sender);
            }
            else if (strstr(buffer, "/exit:") != NULL) {
                printf("%s\n", buffer);
                writeLog(buffer);
                dissconnectUser(sender);
            }else{
                sendRecivedMessageToOtherClients(sender,buffer);
            }
        }
        
        // if recived amount is zero there is an error or the client closed the connection
        //break from the loop then shutdown the server
        if (amountRecived == 0)
            break;
        
    }
    
    dissconnectUser(sender);
}

void receiveAndPrintIncommingMessagesOnASeperateThread(AcceptedSocket *clientSocket ){

    HANDLE thread = CreateThread(NULL, 0, receiveAndPrintIncommingMessages, 
                                    (LPVOID)clientSocket, 0, NULL);
}


void startAcceptingIncomingConnections(int serverSocketFD){
    while (1)
    {
        AcceptedSocket * clientSocket = acceptIncomingConnection(serverSocketFD);
        clientsHead = addLast(clientsHead, clientSocket);
        receiveAndPrintIncommingMessagesOnASeperateThread(clientSocket);
    }
}

int main(){

    filePtr = fopen("logs.txt", "a");
    wslInit();

    int serverSocketFD = createTCPIpv4Socket();
    struct sockaddr_in* serverAddress = CreateIPv4Address("127.0.0.1", 2000);


    /*
        bind function will bind the proccess to the port from the operating system
        will return result if its zero that means it wass successful else not successful
    */ 
    int result = bind(serverSocketFD, (SOCKADDR *) serverAddress, sizeof(*serverAddress));

    if (result == 0){
        printf("socket was bound successfully\n");
        writeLog("socket was bound successfully");
    }else{
        printf("socket was not bound successfully\n");
        writeLog("socket was not bound successfully");
        exit(1);
    }

    /*
        start listening to incoming sockets requests
        10 is the number of backlog (socket requests queues)
        it will return a result too
    */
    int listenResult = listen(serverSocketFD, 10);

    if (listenResult == 0){
        printf("listening\n");
        writeLog("listening");

    }
    else{
        printf("listening was not bound successfully\n");
        writeLog("listening was not bound successfully");
        exit(1);
    }

    startAcceptingIncomingConnections(serverSocketFD);
    
    // shut down the server with option 2 shut down receive and send
    shutdown(serverSocketFD, 2);

    fclose(filePtr);    
    return 0;
}



/*
    use command  
    "gcc SocketServer.c -o socketserver -lws2_32" 
    to compile 
*/

