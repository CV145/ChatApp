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
};

// Function to send a message (MessageHeader)
void sendMessage(int sock, uint8_t message_type, const string &message)
{
    static int message_counter = 0; // Static counter for unique message IDs

    MessageHeader header;
    header.header_length = sizeof(MessageHeader);
    header.message_type = message_type; // CHAT

    // Set the current time as timestamp
    header.timestamp = static_cast<uint32_t>(time(nullptr));
    header.sender_id = 2;                  // Client ID
    header.receiver_id = 1;                // Server ID
    header.message_id = ++message_counter; // Generate unique message ID
    header.payload_length = message.size();

    // Combining MessageHeader and payload into single buffer:
    // Calculate the total size of the message
    // size_t represents the size of an object in bytes
    size_t total_size = sizeof(header) + message.size();

    // Allocate memory for the combined buffer
    char *buffer = new char[total_size];

    // Copy the header to the buffer
    memcpy(buffer, &header, sizeof(header));

    // Copy the message payload to the buffer after the header
    memcpy(buffer + sizeof(header), message.c_str(), message.size());

    // Send the combined buffer over the socket
    // Send all bytes of buffer through socket
    send(sock, buffer, total_size, 0);

    // Free the allocated memory
    delete[] buffer;
}

// Function to handle server responses/retrieve msgs
// Different ways to respond depending on msgs type
void handleServerResponse(int sock)
{
    char buffer[1024];
    while (true)
    {
        // writes into the buffer from socket file descriptor
        int bytesRead = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesRead > 0)
        {
            // cast buffer into MessageHeader
            MessageHeader *header = reinterpret_cast<MessageHeader *>(buffer);

            // Convert timestamp to human-readable format
            time_t raw_time = header->timestamp;
            struct tm *time_info = localtime(&raw_time);
            char time_str[20];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info); // FORMAT time into string

            // Display the timestamp and message content
            cout << "\n[" << time_str << "]: ";

            string message(buffer + sizeof(MessageHeader), header->payload_length);
            switch (header->message_type)
            {
            case 1: // ON_REQ
                cout << "Received online status request from server" << endl;
                sendMessage(sock, 2, "Online status response"); // Respond with ON_RES
                break;
            case 2: // ON_RES
                cout << "Received online status response from server" << endl;
                break;
            case 3: // CHAT
                cout << "Server: " << message << endl;

                // Send ACK for the received chat message
                sendMessage(sock, 4, to_string(header->message_id));

                break;
            case 4: // ACK
                cout << "Received ACK for message ID: " << header->message_id << endl;
                break;
            case 5: // NACK
                cerr << "Received NACK for message ID: " << header->message_id << endl;
                break;
            case 6: // ERR
                cout << "Received error message from server: " << message << endl;
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
    sendMessage(sock, 1, "");

    // Step 4: Create a thread to handle server responses
    // Threads allow the client to send and receive messages at the same time without blocking either operation aka listen for server msgs and send msgs at the same time
    thread responseThread(handleServerResponse, sock); // This thread runs the 'handleServerResponse' function which gets 'sock' as input

    // Step 5: Main loop to send chat messages
    string message;

    while (true)
    {
        cout << "Enter message: ";
        getline(cin, message);

        if (message == "quit")
        {
            return 0;
        }

        if (message == "ON_REQ")
        {
            sendMessage(sock, 1, "");
            cout << "Sent online status request" << endl;
        }
        else
        {
            // Capture the current time for display purposes
            time_t raw_time = time(nullptr);
            struct tm *time_info = localtime(&raw_time);
            char time_str[20];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);

            // Display the timestamp and message
            cout << "Client [" << time_str << "]: " << message << endl;
            sendMessage(sock, 3, message);
        }
    }

    // Step 6: Close the socket and clean up
    close(sock);

    // join() - wait for thread to finish execution before the main program exits
    responseThread.join();

    return 0;
}
