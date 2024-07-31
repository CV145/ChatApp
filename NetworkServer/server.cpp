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
#include <atomic>

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

atomic<bool> exitThread{false};

uint16_t checkSum_Gen(const uint8_t *data, size_t len)
{
    uint32_t sum = 0;
    for (int i = 0; i < len; i++)
        sum += data[i];

    return (uint16_t)sum;
}

bool checkSum_Check(uint16_t checkSum, size_t len)
{
    return true;
}

void socket_Send(int sockfd, const u_int8_t msgType, const string& message)
{
    //When message is sent:
        //1) write appropriate header
        MessageHeader myHeader;
        myHeader.headerLen = sizeof(MessageHeader); //Calculate after filling header?
        myHeader.messageType = msgType;
        myHeader.timeStamp = (uint32_t)time(nullptr);
        myHeader.sender_id = 2; //sample id
        myHeader.receiver_id = 1; //sample id
        myHeader.message_id = 12345; //sample id
        myHeader.payloadLen = message.size();
        myHeader.checksum = 0; //calculate
        
        size_t packetSize = myHeader.headerLen + myHeader.payloadLen;
        uint8_t *buff = new uint8_t[packetSize];
        memcpy(buff, &myHeader, sizeof(myHeader));
        memcpy(buff + sizeof(myHeader), message.c_str(), message.size());
        myHeader.checksum = checkSum_Gen(buff, packetSize);
        memcpy(buff, &myHeader, sizeof(myHeader));

        //4) pass message to TCP
        send(sockfd, buff, sizeof(myHeader), 0);
}

void socket_Receive(int sockfd)
{
    MessageHeader *myHeader;
    int socketClosed, error=0;
    socklen_t len = sizeof(error);
    char buff[1024];
    while (1) {
        cout << "Waiting for Msg\n";
        while (recv(sockfd, buff, sizeof(buff), MSG_DONTWAIT | MSG_PEEK) == -1)
        {
            if (exitThread.load() == 1)
                return;
        }
        ssize_t inBytes = recv(sockfd, buff, sizeof(buff), 0);
        if (inBytes > 0)
        {
        //When message is received:
            //1) convert header
            myHeader = reinterpret_cast<MessageHeader *>(buff);
            cout << "Msg received\n";
            time_t timeRaw = myHeader->timeStamp;
            struct tm* timeInfo = localtime(&timeRaw);
            char timeString[20];
            strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M%S", timeInfo);

            cout << "Msg timestamp: " << timeString << "\n";
            cout << "Msg type: " << (int)myHeader->messageType << ", Payload Length: " << myHeader->payloadLen << " bytes\n";

            if (myHeader->payloadLen > sizeof(buff) - sizeof(myHeader))
            {
                cerr << "Message Length exceedes Buffer, dumping Message and sending Error\n";
                socket_Send(sockfd, 6, to_string(myHeader->message_id));
                continue;
            }
            else if (myHeader->checksum != 0)
            {
                cerr << "Invalid checksum, sending Error Message\n";
                socket_Send(sockfd, 6, to_string(myHeader->message_id));
                continue;
            }
            
            string message(buff + sizeof(MessageHeader), myHeader->payloadLen);

            //1) determine message type and perform appropriate functions
            switch ((int)myHeader->messageType)
            {
                case 1:
                    cout << "Status Request received from: " << (int)myHeader->sender_id << ", sending response\n";
                    socket_Send(sockfd, 2, to_string(myHeader->message_id));
                    break;
                case 2:
                    cout << "Status Response received, " << (int)myHeader->sender_id << " is online\n";
                    break;
                case 3:
                    cout << "Chat Message received: " << message << "\n";
                    socket_Send(sockfd, 4, to_string(myHeader->message_id));
                    break;
                case 4:
                    cout << "Received ACK for Message " << message << "\n";
                    break;
                case 5:
                    cout << "Received NACK for Message " << message << "\n";
                    break;
                case 6:
                    cout << "Received Error for Message " << message << "\n";
                    break;
                default:
                    cout << "Unknown message type: " << (int)myHeader->messageType << " received, sending Error Message\n";
                    socket_Send(sockfd, 6, to_string(myHeader->message_id));
                    break;
            }
        }
        else if (inBytes == -1)
        {
            cerr << strerror(errno) << "\n";
            close(sockfd);
            return;
        }
    }
}



int main (){
    //Declare variables
    struct sockaddr_in saddr, caddr;
    int sockfd, isock;
    unsigned int clen;
    unsigned short port = 8080;
    bool isError;
    int loops = 0;

//Establishing Connection
    do {
        isError = false;

        if (loops > 1)
        {
            cerr << "Failed to establish connection after 3 attempts, exiting program\n";
            return EXIT_FAILURE;
        }

        //1) setup socket
        if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            cerr << strerror(errno) << "\n";
            close(sockfd);
            isError = EXIT_FAILURE;
        }
        printf("Socket created\n");

        //2) bind socket 
        memset(&saddr, '\0', sizeof(saddr)); 
        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = htonl(INADDR_ANY);
        saddr.sin_port = htons(port);

        if ((bind(sockfd, (struct sockaddr*) &saddr, sizeof(saddr)) < 0))
        {
            cerr << strerror(errno) << "\n";
            close(sockfd);
            isError = EXIT_FAILURE;
        }
        printf("Socket bound\n");

        //3) listen for client
        if (listen(sockfd, 5) < 0)
        {
            cerr << strerror(errno) << "\n";
            close(sockfd);
            isError = EXIT_FAILURE;
        }
        printf("Socket listening\n");

        //4) accept connection
        clen = sizeof(caddr);
        if ((isock = accept(sockfd, (struct sockaddr *) &caddr, &clen)) < 0)
        {
            cerr << strerror(errno) << "\n";
            close(sockfd);
            isError = EXIT_FAILURE;
        }
    } while (isError);
    printf("Client connection accepted\n");

    //create thread to handle message receiving
    thread t1(socket_Receive, isock);

    string message;

    while (1)
    {
        cout << "Enter Message, or 'e' to exit\n";
        getline(cin, message);
        if (message == "e")
            break;

        socket_Send(isock, 3, message);
    }

//5) close socket
    close(sockfd);
    cout << "Socket closed\n";
    exitThread.store(true);

    t1.join();
    cout << "Thread(s) joined\n";
}