//Server for a simple messaging application
//Author: Justin Jeirles

#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <cstdio>
#include <thread>

using namespace std;

int main (){
    //Declare variables
    struct sockaddr_in saddr, caddr;
    int sockfd, isock;
    unsigned int clen;
    unsigned short port = 8080;

//Establishing Connection
//1) setup socket
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        printf("Error: socket creation failed\n");

//2) bind socket 
    memset(&saddr, '\0', sizeof(saddr)); 
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(port);
    if ((bind(sockfd, (struct sockaddr*) &saddr, sizeof(saddr)) < 0))
        printf("Error: binding failed\n");

//3) listen for client
    if (listen(sockfd, 5) < 0)
        printf("Error: listening failed\n");

//4) accept connection
    clen = sizeof(caddr);
    if ((isock = accept(sockfd, (struct sockaddr *) &caddr, &clen)) < 0)
        printf("Error: accepting failed\n");

//TODO: Add in multithreading to execute messaging
    //create thread to handle message receiving
    thread t1(socket_read);
        
    //create thread to handle message sending
    thread t2(socket_write);
    
    //wait for threads to finish execution
    t1.join();
    t2.join();

//5) close socket
    close(sockfd);
}

void socket_read()
{
    
    //When message is received:
        //1) determine message type
        //2) determine sender
        //3) perform appropriate functions, outputting to appropriate window
}

void socket_write()
{
    //When message is sent:
        //1) determine message type
        //2) determine destination
        //3) write appropriate header
        //4) pass message to TCP
}