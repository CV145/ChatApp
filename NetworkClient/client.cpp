#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctime>
#include <thread>
#include <atomic>
#include <signal.h>

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

// Atomic flag to indicate running state
atomic<bool> running(true);

// Function to handle SIGINT signal
void signalHandler(int signum)
{
    running = false;
}

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

// Function to handle server responses
void handleServerResponse(int sock, atomic<bool> &running)
{
    char buffer[1024];
    while (running)
    {
        int bytesRead = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesRead > 0)
        {
            MessageHeader *header = reinterpret_cast<MessageHeader *>(buffer);
            string message(buffer + sizeof(MessageHeader), header->payload_length);

            switch (header->message_type)
            {
            case 2: // ON_RES
                cout << "\nServer: Received online status response" << endl;
                break;
            case 3: // CHAT
                cout << "\nServer: " << message << endl;
                break;
            case 4: // ACK
                cout << "\nServer: Received ACK for message ID: " << header->message_id << endl;
                break;
            case 5: // NACK
                cerr << "\nServer: Received NACK for message ID: " << header->message_id << endl;
                break;
            case 6: // ERR
                cout << "\nServer: Received error message: " << message << endl;
                break;
            default:
                cerr << "\nServer: Unknown message type received" << endl;
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

    // Register the signal handler for Ctrl+C (SIGINT)
    signal(SIGINT, signalHandler);

    // Step 1: Create a socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cerr << "Socket creation error" << endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);                       // Port number for the server
    serv_addr.sin_addr.s_addr = inet_addr("192.168.1.165"); // Server IP address, REPLACE this using server IP (use Windows wireless IP forwarded to WSL IP). Client and server must be connected to the same wifi network

    // Step 2: Connect to the server, -1 means error
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        cerr << "Connection failed: " << strerror(errno) << endl; // Print the error message
        close(sock);                                              // Close the socket to clean up resources
        return -1;
    }

    cout << "Connection successful!" << endl;

    // Step 3: Send an online status request
    sendOnlineStatusRequest(sock);

    // Step 4: Create a thread to handle server responses
    thread responseThread(handleServerResponse, sock, ref(running));

    // Step 5: Main loop to send chat messages
    string message;

    while (running)
    {
        cout << "Enter message: ";
        getline(cin, message);

        if (message == "quit")
        {
            running = false;
            break;
        }

        cout << "Client: " << message << endl;
        sendChatMessage(sock, message);
    }

    // Step 6: Close the socket and clean up
    running = false;
    responseThread.join();
    close(sock);

    return 0;
}
