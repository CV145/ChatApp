#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctime>
#include <arpa/inet.h>
#include <thread>

// Using individual declarations to avoid std:: prefixes
using namespace std;

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
    header.sender_id = 1;                  // Server ID
    header.receiver_id = 2;                // Client ID
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

// Function to handle client responses
void handleClientResponse(int client_fd)
{
    MessageHeader header;
    while (true)
    {
        ssize_t bytesRead = read(client_fd, &header, sizeof(header));
        if (bytesRead <= 0)
        {
            if (bytesRead == 0)
            {
                cout << "Client disconnected" << endl;
            }
            else
            {
                perror("Read error");
            }
            break;
        }

        switch (header.message_type)
        {
        case 1: // ON_REQ
            cout << "Received online status request" << endl;
            // Send online status response
            sendMessage(client_fd, 2, "Server is online");
            break;
        case 3: // CHAT
        {
            char *payload = new char[header.payload_length];
            ssize_t payloadBytesRead = read(client_fd, payload, header.payload_length);
            if (payloadBytesRead != header.payload_length)
            {
                cerr << "Payload read error: Expected " << header.payload_length << " bytes, but got " << payloadBytesRead << " bytes." << endl;
                delete[] payload;
                break;
            }
            cout << "Client Message: " << string(payload, header.payload_length) << endl;
            delete[] payload;

            // Send ACK
            MessageHeader ack;
            ack.header_length = sizeof(MessageHeader);
            ack.message_type = 4; // ACK
            ack.timestamp = static_cast<uint32_t>(time(nullptr));
            ack.sender_id = header.receiver_id;
            ack.receiver_id = header.sender_id;
            ack.message_id = header.message_id;
            ack.payload_length = 0;
            if (send(client_fd, &ack, sizeof(ack), 0) < 0)
            {
                perror("Send error");
                break;
            }
            break;
        }
        default:
            cerr << "Unknown message type received" << endl;
            break;
        }
    }
}

// Function to handle sending messages from the server to the client
void sendMessageToClient(int client_fd)
{
    string message;
    while (true)
    {
        cout << "Enter message: ";
        getline(cin, message);
        if (message == "quit")
        {
            break;
        }
        sendMessage(client_fd, 3, message); // Use message type 3 for chat messages
    }
}

int main()
{
    int server_fd, client_fd;
    struct sockaddr_in address;
    struct sockaddr_in client_address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    socklen_t client_addrlen = sizeof(client_address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket creation error");
        return -1;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("Setsockopt error");
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        return -1;
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("Listen failed");
        return -1;
    }

    cout << "Server listening on port 8080" << endl;

    while (true)
    {
        if ((client_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_addrlen)) < 0)
        {
            perror("Accept failed");
            continue;
        }

        // Get and display client information
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
        cout << "Connection accepted from " << client_ip << ":" << ntohs(client_address.sin_port) << endl;

        // Handle client responses in a separate thread
        thread clientResponseThread(handleClientResponse, client_fd);
        clientResponseThread.detach();

        // Handle sending messages to the client in the main thread
        sendMessageToClient(client_fd);

        // Close client socket after communication is done
        close(client_fd);
    }

    close(server_fd);

    return 0;
}
