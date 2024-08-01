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

struct MessageHeader
{
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

void socket_Send(int sockfd, const u_int8_t msgType, const string &message)
{
    MessageHeader myHeader;
    myHeader.headerLen = sizeof(MessageHeader);
    myHeader.messageType = msgType;
    myHeader.timeStamp = (uint32_t)time(nullptr);
    myHeader.sender_id = 2;      // sample id
    myHeader.receiver_id = 1;    // sample id
    myHeader.message_id = 12345; // sample id
    myHeader.payloadLen = message.size();
    myHeader.checksum = 0; // calculate

    size_t packetSize = myHeader.headerLen + myHeader.payloadLen;
    uint8_t *buff = new uint8_t[packetSize];
    memcpy(buff, &myHeader, sizeof(myHeader));
    memcpy(buff + sizeof(myHeader), message.c_str(), message.size());
    myHeader.checksum = checkSum_Gen(buff, packetSize);
    memcpy(buff, &myHeader, sizeof(myHeader));

    send(sockfd, buff, packetSize, 0); // Ensure sending the entire packet
    delete[] buff;
}

void socket_Receive(int sockfd)
{
    MessageHeader *myHeader;
    char buff[1024];
    while (true)
    {
        cout << "Waiting for Msg\n";
        while (recv(sockfd, buff, sizeof(buff), MSG_DONTWAIT | MSG_PEEK) == -1)
        {
            if (exitThread.load())
                return;
        }
        ssize_t inBytes = recv(sockfd, buff, sizeof(buff), 0);
        if (inBytes > 0)
        {
            myHeader = reinterpret_cast<MessageHeader *>(buff);
            cout << "Msg received\n";
            time_t timeRaw = myHeader->timeStamp;
            struct tm *timeInfo = localtime(&timeRaw);
            char timeString[20];
            strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", timeInfo);

            cout << "Msg timestamp: " << timeString << "\n";
            cout << "Msg type: " << (int)myHeader->messageType << ", Payload Length: " << myHeader->payloadLen << " bytes\n";

            if (myHeader->payloadLen > sizeof(buff) - sizeof(MessageHeader))
            {
                cerr << "Message Length exceeds Buffer, dumping Message and sending Error\n";
                socket_Send(sockfd, 6, to_string(myHeader->message_id));
                continue;
            }
            else if (!checkSum_Check(myHeader->checksum, myHeader->payloadLen))
            {
                cerr << "Invalid checksum, sending Error Message\n";
                socket_Send(sockfd, 6, to_string(myHeader->message_id));
                continue;
            }

            string message(buff + sizeof(MessageHeader), myHeader->payloadLen);
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

int main()
{
    struct sockaddr_in saddr, caddr;
    int sockfd = -1, isock = -1;
    unsigned int clen;
    unsigned short port = 8080;
    bool isError;
    int loops = 0;

    do
    {
        isError = false;

        if (loops > 1)
        {
            cerr << "Failed to establish connection after 3 attempts, exiting program\n";
            return EXIT_FAILURE;
        }

        if (sockfd != -1)
            close(sockfd);
        if (isock != -1)
            close(isock);

        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            cerr << strerror(errno) << "\n";
            isError = true;
            loops++;
            continue;
        }
        printf("Socket created\n");

        memset(&saddr, '\0', sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = htonl(INADDR_ANY);
        saddr.sin_port = htons(port);

        if ((bind(sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0))
        {
            cerr << strerror(errno) << "\n";
            isError = true;
            loops++;
            continue;
        }
        printf("Socket bound\n");

        if (listen(sockfd, 5) < 0)
        {
            cerr << strerror(errno) << "\n";
            isError = true;
            loops++;
            continue;
        }
        printf("Socket listening\n");

        clen = sizeof(caddr);
        if ((isock = accept(sockfd, (struct sockaddr *)&caddr, &clen)) < 0)
        {
            cerr << strerror(errno) << "\n";
            isError = true;
            loops++;
            continue;
        }
        printf("Client connection accepted\n");

    } while (isError);

    thread t1(socket_Receive, isock);

    string message;
    while (true)
    {
        cout << "Enter Message, or 'e' to exit\n";
        getline(cin, message);
        if (message == "e")
            break;

        socket_Send(isock, 3, message);
    }

    close(sockfd);
    cout << "Socket closed\n";
    exitThread.store(true);

    t1.join();
    cout << "Thread(s) joined\n";
}
