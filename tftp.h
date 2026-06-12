/* Common file for server & client*/

#ifndef TFTP_H
#define TFTP_H

#include <stdint.h>
#include <arpa/inet.h>
#include<stdlib.h>

//#define PORT 6969
#define BUFFER_SIZE 516  // TFTP data packet size (512 bytes data + 4 bytes header)
#define TIMEOUT_SEC 5    // Timeout in seconds
#define SUCCESS 1
#define FAILURE 0
#define DEFAULT 1
#define OCTATE 2
#define NETASCII 3

// TFTP OpCodes
typedef enum {
    RRQ = 1,  // Read Request
    WRQ = 2,  // Write Request
    DATA = 3, // Data Packet
    ACK = 4,  // Acknowledgment
    ERROR = 5 // Error Packet
} tftp_opcode;



// TFTP Packet Structure
typedef struct {
    uint16_t opcode; // Operation code (RRQ/WRQ/DATA/ACK/ERROR)
    union {
        struct {
            char filename[256];
            int mode; // mode of transfer
            int opcode; // RRQ and WRQ
            int connection;
        } request;  // 
        struct {
            uint16_t block_number;
            char data[513];
            int data_size;
            int data_flag;
        } data_packet; // DATA
        struct {
            uint16_t block_number;
            int data_size;
        } ack_packet; // ACK
        struct {
            uint16_t error_code;
            char error_msg[512];
        } error_packet; // ERROR
    } body;
} tftp_packet;

void send_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename);
void receive_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename);


#endif // TFTP_H
