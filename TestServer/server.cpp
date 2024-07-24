#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctime>
#include <arpa/inet.h>
#include <thread>

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
    uint16_t checksum;
};

uint16_t calculateChecksum(const uint8_t *data, size_t length)
{
    uint32_t sum = 0;
    for (size_t i = 0; i < length; ++i)
    {
        sum += data[i];
    }
    return static_cast<uint16_t>(~sum);
}

void handleClient(int client_fd)
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
            MessageHeader response;
            response.header_length = sizeof(MessageHeader);
            response.message_type = 2; // ON_RES
            response.timestamp = static_cast<uint32_t>(time(nullptr));
            response.sender_id = header.receiver_id;
            response.receiver_id = header.sender_id;
            response.message_id = header.message_id;
            response.payload_length = 0;
            response.checksum = calculateChecksum(reinterpret_cast<const uint8_t *>(&response), sizeof(response) - sizeof(response.checksum));
            if (send(client_fd, &response, sizeof(response), 0) < 0)
            {
                perror("Send error");
                break;
            }
            break;
        case 3:
        { // CHAT
            cout << "Received chat message" << endl;
            char *payload = new char[header.payload_length];
            ssize_t payloadBytesRead = read(client_fd, payload, header.payload_length);
            if (payloadBytesRead != header.payload_length)
            {
                cerr << "Payload read error" << endl;
                delete[] payload;
                break;
            }
            cout << "Message: " << string(payload, header.payload_length) << endl;
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
            ack.checksum = calculateChecksum(reinterpret_cast<const uint8_t *>(&ack), sizeof(ack) - sizeof(ack.checksum));
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

        // Handle the client in a separate thread
        thread clientThread(handleClient, client_fd);
        clientThread.detach();
    }

    close(server_fd);
    return 0;
}
