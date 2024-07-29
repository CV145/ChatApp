#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctime>
#include <thread>

// Using individual declarations to avoid std:: prefixes
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

    // Set the current time as timestamp
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

// Function to handle server responses/retrieve msgs
// Different ways to respond depending on msgs type
void handleServerResponse(int sock)
{
    char buffer[1024];
    while (true)
    {
        int bytesRead = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesRead > 0)
        {
            MessageHeader *header = reinterpret_cast<MessageHeader *>(buffer);

            // Convert timestamp to human-readable format
            time_t raw_time = header->timestamp;
            struct tm *time_info = localtime(&raw_time);
            char time_str[20];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);

            // Display the timestamp and message content
            cout << "\n[" << time_str << "]: ";

            string message(buffer + sizeof(MessageHeader), header->payload_length);
            switch (header->message_type)
            {
            case 2: // ON_RES
                cout << "Server: Received online status response" << endl;
                break;
            case 3: // CHAT
                cout << "Server: " << message << endl;
                break;
            case 4: // ACK
                cout << "Server: Received ACK for message ID: " << header->message_id << endl;
                break;
            case 5: // NACK
                cerr << "Server: Received NACK for message ID: " << header->message_id << endl;
                break;
            case 6: // ERR
                cout << "Server: Received error message: " << message << endl;
                break;
            default:
                cerr << "Server: Unknown message type received" << endl;
                break;
            }

            cout << "Enter message: ";
            cout.flush();
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
    serv_addr.sin_port = htons(8080);                   // Port number for the server
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Server IP address, REPLACE this using server IP (use Windows wireless IP forwarded to WSL IP). Client and server must be connected to the same wifi network

    /*
    In socket programming, the client connects to the server program running on a specific device by targeting the server's IP address and port number.

    The port number specifies the specific service or application on the server device that the client wants to connect to. For example, 8080 is the port where the server program is listening for incoming connections.
    */

    // Step 2: Connect to the server, -1 means error
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        cerr << "Connection failed: " << strerror(errno) << endl; // Print the error message
        close(sock);                                              // Close the socket to clean up resources
        return -1;
    }

    cout << "Connection successful!" << endl;

    // Get and display local socket information
    sockaddr_in localAddress;
    socklen_t localAddressLen = sizeof(localAddress);
    if (getsockname(sock, (struct sockaddr *)&localAddress, &localAddressLen) == -1)
    {
        cerr << "Error getting local socket information: " << strerror(errno) << endl;
    }
    else
    {
        char localIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &localAddress.sin_addr, localIP, sizeof(localIP));
        cout << "Local IP: " << localIP << ", Local Port: " << ntohs(localAddress.sin_port) << endl;
    }

    // Get and display remote socket information
    sockaddr_in remoteAddress;
    socklen_t remoteAddressLen = sizeof(remoteAddress);
    if (getpeername(sock, (struct sockaddr *)&remoteAddress, &remoteAddressLen) == -1)
    {
        cerr << "Error getting remote socket information: " << strerror(errno) << endl;
    }
    else
    {
        char remoteIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &remoteAddress.sin_addr, remoteIP, sizeof(remoteIP));
        cout << "Connected to IP: " << remoteIP << ", Port: " << ntohs(remoteAddress.sin_port) << endl;
    }

    // Step 3: Send an online status request
    sendOnlineStatusRequest(sock);

    // Step 4: Create a thread to handle server responses
    // Threads allow the client to send and receive messages at the same time without blocking either operation aka listen for server msgs and send msgs at the same time
    thread responseThread(handleServerResponse, sock); // This thread runs the 'handleServerResponse' function which gets 'sock' as input

    // Step 5: Main loop to send chat messages
    string message;

    while (true)
    {
        cout << "Enter message: ";
        getline(cin, message); // TODO: make this non-blocking

        if (message == "quit")
        {
            break;
        }

        // Capture the current time
        time_t raw_time = time(nullptr);
        struct tm *time_info = localtime(&raw_time);
        char time_str[20];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);

        // Display the timestamp and message
        cout << "Client [" << time_str << "]: " << message << endl;
        sendChatMessage(sock, message);
    }

    // Step 6: Close the socket and clean up
    close(sock);

    // join() - wait for thread to finish execution
    responseThread.join();

    return 0;
}
