#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctime>

using namespace std;

// Define the message header structure
struct MessageHeader
{
    uint8_t header_length;
    uint8_t message_type;
    uint32_t timestamp;
    uint16_t sender_id;
    uint16_t receiver_id;
    uint32_t message_id;
    uint16_t payload_length;
    uint16_t checksum;
};

// Function to calculate checksum
uint16_t calculateChecksum(const uint8_t *data, size_t length)
{
    uint32_t sum = 0;
    for (size_t i = 0; i < length; ++i)
    {
        sum += data[i];
    }
    return static_cast<uint16_t>(~sum);
}

// Function to send an online status request
void sendOnlineStatusRequest(int sock)
{
    MessageHeader header;
    header.header_length = sizeof(MessageHeader);
    header.message_type = 1; // ON_REQ
    header.timestamp = static_cast<uint32_t>(time(nullptr));
    header.sender_id = 1;      // Example sender ID
    header.receiver_id = 2;    // Example receiver ID
    header.message_id = 12345; // Example message ID
    header.payload_length = 0;
    header.checksum = calculateChecksum(reinterpret_cast<const uint8_t *>(&header), sizeof(header) - sizeof(header.checksum));

    send(sock, &header, sizeof(header), 0);
}

// Function to send a chat message
void sendChatMessage(int sock, const string &message)
{
    MessageHeader header;
    header.header_length = sizeof(MessageHeader);
    header.message_type = 3; // CHAT
    header.timestamp = static_cast<uint32_t>(time(nullptr));
    header.sender_id = 1;      // Example sender ID
    header.receiver_id = 2;    // Example receiver ID
    header.message_id = 12346; // Example message ID
    header.payload_length = message.size();
    header.checksum = 0; // Initial checksum

    // Calculate checksum including the payload
    size_t total_size = sizeof(header) + message.size();
    uint8_t *buffer = new uint8_t[total_size];
    memcpy(buffer, &header, sizeof(header));
    memcpy(buffer + sizeof(header), message.c_str(), message.size());
    header.checksum = calculateChecksum(buffer, total_size);
    memcpy(buffer, &header, sizeof(header));

    send(sock, buffer, total_size, 0);
    delete[] buffer;
}

// Function to handle different server responses
void handleServerResponse(int sock)
{
    MessageHeader response;
    while (read(sock, &response, sizeof(response)) > 0)
    {
        switch (response.message_type)
        {
        case 2: // ON_RES
            cout << "Received online status response" << endl;
            break;
        case 4: // ACK
            cout << "Received ACK for message ID: " << response.message_id << endl;
            break;
        case 5: // NACK
            cerr << "Received NACK for message ID: " << response.message_id << endl;
            break;
        case 6: // ERR
            cout << "Received error message" << endl;
            // Handle error message (payload contains error description)
            break;
        default:
            cerr << "Unknown message type received" << endl;
            break;
        }
    }
}

int main()
{
    int sock = 0;
    struct sockaddr_in serv_addr;

    // Step 1: Create a socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cerr << "Socket creation error" << endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);                   // Port number for the server - server will be listening on port 8080
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Server IP address, REPLACE this using server IP found using ifconfig. Client and server must be connected to the same wifi network

    // Step 2: Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        cerr << "Connection failed" << endl;
        return -1;
    }

    // Step 3: Send an online status request
    sendOnlineStatusRequest(sock);

    // Step 4: Send a chat message
    sendChatMessage(sock, "Hello, this is a test message!");

    // Step 5: Handle server responses
    handleServerResponse(sock);

    // Step 6: Close the socket
    close(sock);

    return 0;
}
