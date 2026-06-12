#include "tftp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include<fcntl.h>


void handle_client(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet);

int main() {

    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    tftp_packet packet;

    // Create UDP socket
    sockfd = socket(AF_INET,SOCK_DGRAM,0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(IP);
    bind(sockfd,(struct sockaddr *)&server_addr,sizeof(server_addr));
  

    printf("TFTP Server listening on port %d...\n", PORT);

    // Main loop to handle incoming requests
    while (1) {
        printf("SERVER WAITING FOR REQUEST...\n");
        int n = recvfrom(sockfd, &packet, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_len);
        
        if (n < 0) {
            perror("Receive failed or timeout occurred");
            continue;
        }

        handle_client(sockfd, client_addr, client_len, &packet);
    }

    close(sockfd);
    return 0;
}

void handle_client(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet) 
{

    // Extract the TFTP operation (read or write) from the received packet
    if(packet->body.request.opcode == WRQ)
    {

        int fd = open(packet->body.request.filename,O_CREAT | O_EXCL | O_WRONLY,0666);
        if(fd == -1)
        {
            close(fd);
            fd = open(packet->body.request.filename,O_CREAT | O_TRUNC | O_WRONLY,0666);
            if(fd == -1)
            {
                printf("Truncate failed\n");//etst
                return;
            }
        }
        
        int ack;
        if(fd != -1)
        {
            ack = SUCCESS;
        }
        else
        {
            ack = FAILURE;
        }

        printf("SEND ACK TO CLIENT FOR PUT OPERATION\n");
        sendto(sockfd,&ack,sizeof(ack),0,(struct sockaddr *)&client_addr,sizeof(client_addr));
        int ret;
        if(packet->body.request.mode == OCTATE)
        {
            while(1)
            {
                recvfrom(sockfd,packet,sizeof(*packet),0,(struct sockaddr *)&client_addr,&client_len);//receive data from client
                if(packet->body.data_packet.data_flag == 0)
                {
                    return;
                }
                ret = write(fd,packet->body.data_packet.data,packet->body.data_packet.data_size);//write data to file
                packet->body.ack_packet.block_number = packet->body.data_packet.block_number;//store packet number
                packet->body.ack_packet.data_size = ret;//store data size
                sendto(sockfd,packet,sizeof(*packet),0,(struct sockaddr *)&client_addr,sizeof(client_addr));//send ack to client about data transfer
            }
        }
        else if(packet->body.request.mode == NETASCII)
        {
            char temp = '\r';
            while(1)
            {
                recvfrom(sockfd,packet,sizeof(*packet),0,(struct sockaddr *)&client_addr,&client_len);//receive data from client
                if(packet->body.data_packet.data_flag == 0)
                {
                    return;
                }

                for(int i=0;i<packet->body.data_packet.data_size;i++)
                {
                    if(packet->body.data_packet.data[i] == '\n')
                    {
                        write(fd,&temp,1);
                        write(fd,&packet->body.data_packet.data[i],1);
                    }
                    else
                    {
                        write(fd,&packet->body.data_packet.data[i],1);
                    }
                }
                packet->body.ack_packet.block_number = packet->body.data_packet.block_number;//store packet number
                packet->body.ack_packet.data_size = packet->body.data_packet.data_size;//store data size
                sendto(sockfd,packet,sizeof(*packet),0,(struct sockaddr *)&client_addr,sizeof(client_addr));//send ack to client about data transfer
            }
        }
        else
        {
            while(1)
            {
                recvfrom(sockfd,packet,sizeof(*packet),0,(struct sockaddr *)&client_addr,&client_len);//receive data from client
                if(packet->body.data_packet.data_flag == 0)
                {
                    return;
                }
                ret = write(fd,packet->body.data_packet.data,packet->body.data_packet.data_size);//write data to file
                packet->body.ack_packet.block_number = packet->body.data_packet.block_number;//store packet number
                packet->body.ack_packet.data_size = ret;//store data size
                sendto(sockfd,packet,sizeof(*packet),0,(struct sockaddr *)&client_addr,sizeof(client_addr));//send ack to client about data transfer
            }
        }

    }
    else if(packet->body.request.opcode == RRQ)
    {
        int fd = open(packet->body.request.filename,O_RDONLY);
        int ack;
        if(fd == -1)
        {
            ack = FAILURE;
        }
        else
        {
            ack = SUCCESS;
        }
        sendto(sockfd,&ack,4,0,(struct sockaddr *)&client_addr,sizeof(client_addr));//send ack for file 

        recvfrom(sockfd,&ack,4,0,(struct sockaddr *)&client_addr,&client_len);//receive ack from client
        printf("ACK FROM CLIENT FILE OPEN FOR GET OPERATION SUCCESSFULL\n");
        if(ack == SUCCESS)
        {
            packet->body.data_packet.block_number = 1;
            packet->body.data_packet.data_flag = 1;

            if(packet->body.request.mode == OCTATE)
            {
                while(1)
                {
                    int ret = read(fd,packet->body.data_packet.data,1);//read data by 1 byte
                    packet->body.data_packet.data_size = ret;//stores data size
                    if(packet->body.data_packet.data_size == 0)
                    {
                        packet->body.data_packet.data_flag = 0;
                        sendto(sockfd,packet,sizeof(*packet),0,(struct sockaddr *)&client_addr,sizeof(client_addr));//send packet to client to let it know data is empty
                        return;
                    }
                    
                    sendto(sockfd,packet,sizeof(*packet),0,(struct sockaddr *)&client_addr,sizeof(client_addr));//send packet to client
                    
                    recvfrom(sockfd,packet,sizeof(*packet),0,(struct sockaddr *)&client_addr,&client_len);//receive ack from client
                    printf("ACK FROM CLIENT data size: %d , Packet No: %d\n",packet->body.ack_packet.data_size,packet->body.ack_packet.block_number);//print ack
                    if(packet->body.ack_packet.block_number == 0)
                    {
                        sendto(sockfd,packet,sizeof(*packet),0,(struct sockaddr *)&client_addr,sizeof(client_addr));//resend data if data send is failed
                    }
                    packet->body.ack_packet.block_number++;
                }
            }
            else
            {
                while(1)
                {
                    int ret = read(fd,packet->body.data_packet.data,512);
                    packet->body.data_packet.data_size = ret;
                    if(packet->body.data_packet.data_size == 0)
                    {
                        packet->body.data_packet.data_flag = 0;
                        sendto(sockfd,packet,sizeof(*packet),0,(struct sockaddr *)&client_addr,sizeof(client_addr));
                        return;
                    }
                    
                    sendto(sockfd,packet,sizeof(*packet),0,(struct sockaddr *)&client_addr,sizeof(client_addr));
                    
                    recvfrom(sockfd,packet,sizeof(*packet),0,(struct sockaddr *)&client_addr,&client_len);
                    printf("ACK FROM CLIENT data size: %d , Packet No: %d\n",packet->body.ack_packet.data_size,packet->body.ack_packet.block_number);
                    if(packet->body.ack_packet.block_number == 0)
                    {
                        sendto(sockfd,packet,sizeof(*packet),0,(struct sockaddr *)&client_addr,sizeof(client_addr));
                    }
                    packet->body.ack_packet.block_number++;
                }
            }
        }
        else
        {
            printf("FILE OPEN FAILED\n");
            return;
        }
    }
    // and call send_file or receive_file accordingly
}




