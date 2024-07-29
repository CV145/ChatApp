//Server for a simple messaging application
//Author: Justin Jeirles

#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <cstdio>
#include <thread>
#include <ctime>

using namespace std;

struct MessageHeader {
    uint8_t headerLen;
    uint8_t messageType;
    uint32_t timeStamp;
    uint16_t sender_id;
    uint16_t receiver_id;
    uint32_t message_id;
    uint16_t payloadLen;
    uint16_t checksum;
};

void socket_read(int sockfd)
{
    MessageHeader *myHeader;
    cout << "Waiting on Msg\n";
    char buff[1024];
    while (1) {
        ssize_t inBytes = recv(sockfd, buff, sizeof(buff), 0);
        if (inBytes > 0)
        {
            myHeader = (MessageHeader *) buff;
            cout << "Msg received\n";
            time_t timeRaw = myHeader->timeStamp;
            struct tm* timeInfo = localtime(&timeRaw);
            char timeString[20];
            strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M%S", timeInfo);

            cout << "Msg timestamp: " << timeString << "\n";

            //
            string message(buff + sizeof(MessageHeader), myHeader->payloadLen);

            switch ((int)myHeader->messageType)
            {
                case 2:
                    cout << "Case 2\n";
                    break;
                case 3:
                    cout << "Chat message received: " << message << "\n";
                    break;
                case 4:
                    cout << "Case 4\n";
                    break;
                case 5:
                    cout << "Case 5\n";
                    break;
                case 6:
                    cout << "Case 6\n";
                    break;
                default:
                    cout << "Unknown message type: " << (int)myHeader->messageType << " received\n";
                    break;
            }
        }
        else if (inBytes == -1)
        {
            cerr << strerror(errno) << "\n";
            close(sockfd);
            EXIT_FAILURE;
        }
    }
    //When message is received:
        //1) determine message type
        //2) determine sender
        //3) perform appropriate functions, outputting to appropriate window
}

uint16_t checkSum_Gen(const uint8_t *data, size_t len)
{
    uint32_t sum = 0;
    for (int i = 0; i < len; i++)
        sum += data[i];
    
    return (uint16_t)sum;
}

void socket_SendMessage(int sockfd, const string& message)
{
    while (1)
    {
    //When message is sent:
        //1) determine message type
        //2) write appropriate header
        MessageHeader myHeader;
        myHeader.headerLen = sizeof(MessageHeader); //Calculate after filling header?
        myHeader.messageType = 3; //sample message value
        myHeader.timeStamp = (uint32_t)time(nullptr);
        myHeader.sender_id = 2; //sample id
        myHeader.receiver_id = 1; //sample id
        myHeader.message_id = 12345; //sample id
        myHeader.payloadLen = 0; //sample length
        myHeader.checksum = 0;
        
        size_t packetSize = sizeof(myHeader) + message.size();
        uint8_t *buff = new uint8_t[packetSize];
        memcpy(buff, &myHeader, sizeof(myHeader));
        memcpy(buff + sizeof(myHeader), message.c_str(), message.size());
        myHeader.checksum = checkSum_Gen(buff, packetSize);
        memcpy(buff, &myHeader, sizeof(myHeader));

        //4) pass message to TCP
        send(sockfd, buff, sizeof(myHeader), 0);
    }
}

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
    printf("Socket created\n");

//2) bind socket 
    memset(&saddr, '\0', sizeof(saddr)); 
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(port);
    if ((bind(sockfd, (struct sockaddr*) &saddr, sizeof(saddr)) < 0))
        cout << "binding failed Error: " << strerror(errno) << "\n";
    printf("Socket bound\n");

//3) listen for client
    if (listen(sockfd, 5) < 0)
        printf("Error: listening failed\n");
    printf("socket listening\n");

//4) accept connection
    clen = sizeof(caddr);
    if ((isock = accept(sockfd, (struct sockaddr *) &caddr, &clen)) < 0)
        printf("Error: accepting failed\n");
    printf("Client connection accepted\n");

    string message = "hello otherworld\n";

//TODO: Add in multithreading to execute messaging
    //create thread to handle message receiving
    socket_read(isock);
        
    //create thread to handle message sending
    //thread t1(socket_SendMessage, sockfd, message);
    
    //wait for threads to finish execution
    //t1.join();

//5) close socket
    close(sockfd);
    cout << "Socket closed\n";
}