#include "tftp.h"
#include "tftp_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include<fcntl.h>


tftp_packet packet;

int main() {
    char command[256];
    packet.body.request.connection = 0;
    tftp_client_t client;
    memset(&client, 0, sizeof(client));  // Initialize client structure

    // Main loop for command-line interface
    while (1) {
        //print the main menu
        printf("Main Menu:\n\t1.Connect\n\t2.Put\n\t3.Get\n\t4.Mode\n\t5.Disconnect\n");
        printf("Enter Your Chioce: ");
        int choice;
        scanf("%d",&choice);
        getchar();
        switch(choice)
        {
            case 1:
                char ip[30];
                int PORT;
                connect_to_server(&client,ip,PORT);//connect
                break;
            case 2:
                char putfile[30];
                put_file(&client,putfile);//put to file in server
                break;
            case 3:
                char getfile[30];
                get_file(&client,getfile);//get from file in server
                break; 
            case 4:
                mode(&client);//select mode
                break;
            case 5:
                disconnect(&client);//exit from program
                break;
            default :
                printf("INVALID OPTION\n");
        }
    }

    return 0;
}


// This function is to initialize socket with given server IP, no packets sent to server in this function
void connect_to_server(tftp_client_t *client, char *ip, int port) {
    // Create UDP socket
    int sock_fd = socket(AF_INET,SOCK_DGRAM,0);

    // read the server address
    printf("Enter IP address to connect: ");
    scanf("%s",ip);
    //read port number
    printf("Enter PORT number: ");
    scanf("%d",&port);

    // validate the address
    int dot_count = 0;
    if(ip[0] >= '0' && ip[0] <= '9')
    {
        int i;
        for(i=0;ip[i] != '\0';i++)
        {
            if(ip[i] == '.' || (ip[i] >= '0' && ip[i] <= '9'))
            {
                if(ip[i] == '.')
                    dot_count++;
            }
            else
            {
                printf("IP address is not valid...!\n");
                return;
            }
        }
        if(ip[--i] == '.')
        {
            printf("IP address is not valid...!\n");
            return;
        }
    }
    if(dot_count != 3)
    {
        printf("IP address is not valid...!\n");
        return;
    }
    printf("IP ADDRESS VALIDATION SUCCESSFULL\n");

    //port number validation
    if(port >= 1024 && port <= 49151)
    {
        printf("PORT NUMBER VALIDATION SUCCESSFULL\n");
    }
    else
    {
        printf("PORT number is not valid...!\n");
        return;
    }
    //bind
    //struct sockaddr_in servinfo;
    client->server_addr.sin_family = AF_INET;
    client->server_addr.sin_port = htons(port);
    client->server_addr.sin_addr.s_addr = inet_addr(ip);

    packet.body.request.connection = 1;

    client->sockfd = sock_fd;
    printf("CONNECTED TO SERVER\n");

}

//put operation : store data into file in server
void put_file(tftp_client_t *client, char *filename) {

    if(packet.body.request.connection != 1)//check connect operation is done before put operation
    {
        printf("\nCONNECT OPERATION NOT DONE..!\n\n");
        return;
    }

    //read filename from user
    printf("Enter file name : \n");
    scanf("%s",filename);

    //validate the file exists or not
    int fd = open(filename,O_RDONLY);
    if(fd == -1)
    {
        printf("FILE DOES NOT EXIST...!\n");
        return;
    }

    // Send WRQ request and send file
    send_request(client->sockfd,client->server_addr,filename,WRQ);

    //
    int ack;
    socklen_t servlen;
    recvfrom(client->sockfd,&ack,4,0,(struct sockaddr *)&client->server_addr,&client->server_len);
    if(ack == SUCCESS)
    {
        printf("ACK FROM SERVER : FILE OPEN FOR PUT OPERATION SUCCESSFULL\n");
        int ret;
        packet.body.data_packet.block_number = 1;
        packet.body.data_packet.data_flag = 1;//
        if(packet.body.request.mode == OCTATE)
        {
            while(ret != 0)
            {
                ret = read(fd,packet.body.data_packet.data,1);//read 1 bytes data from file
                packet.body.data_packet.data_size = ret;

                if(ret == 0)
                {
                    packet.body.data_packet.data_flag = 0;
                    sendto(client->sockfd,&packet,sizeof(packet),0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
                    return;
                }
                sendto(client->sockfd,&packet,sizeof(packet),0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));//send to server as a packet
                

                recvfrom(client->sockfd,&packet,sizeof(packet),0,(struct sockaddr *)&client->server_addr,&client->server_len);//receive ack from server
                printf("ACK FROM SERVER data size: %d , Packet No: %d\n",packet.body.ack_packet.data_size,packet.body.ack_packet.block_number);//print ack
                if(packet.body.ack_packet.data_size != packet.body.data_packet.data_size)
                {
                    sendto(client->sockfd,&packet,sizeof(packet),0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));//resend data if ack is not equal to sent data
                }
                packet.body.data_packet.block_number++;//increment packet number
            }
        }
        else
        {
            while(ret != 0)
            {
                ret = read(fd,packet.body.data_packet.data,512);//read 512 bytes data from file
                packet.body.data_packet.data_size = ret;

                if(ret == 0)
                {
                    packet.body.data_packet.data_flag = 0;
                    sendto(client->sockfd,&packet,sizeof(packet),0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));//send packet to server to let know data sending over
                    return;
                }

                sendto(client->sockfd,&packet,sizeof(packet),0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));//send to server as a packet
                
                recvfrom(client->sockfd,&packet,sizeof(packet),0,(struct sockaddr *)&client->server_addr,&client->server_len);//receive ack from server
                printf("ACK FROM SERVER data size: %d , Packet No: %d\n",packet.body.ack_packet.data_size,packet.body.ack_packet.block_number);//print ack
                if(packet.body.ack_packet.data_size != packet.body.data_packet.data_size)
                {
                    sendto(client->sockfd,&packet,sizeof(packet),0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));//resend data if ack is not equal to sent data
                }

                packet.body.data_packet.block_number++;//increment packet number
            }
        }
    }
    else
    {
        printf("File Open Failed\n");
        return;
    }
}



//get operation : store data from file in the server to client
void get_file(tftp_client_t *client, char *filename) {

    if(packet.body.request.connection != 1)//check connect operation is done before get operation
    {
        printf("\nCONNECT OPERATION NOT DONE..!\n\n");
        return;
    }

    //read file name
    printf("Enter file name: \n");
    scanf("%s",filename);

    // Send RRQ and recive file 
    receive_request(client->sockfd,client->server_addr,filename,RRQ);

    //sendto(client->sockid,&packet,sizeof(packet),0,&servinfo,sizeof(servinfo));
    int ack;
    socklen_t servlen;

    //receive ack from server if file open successfull or not
    recvfrom(client->sockfd,&ack,4,0,(struct sockaddr *)&client->server_addr,&client->server_len);
    printf("ACK FROM SERVER FILE OPEN FOR GET OPERATION SUCCESSFULL\n");
    if(ack == SUCCESS)
    {
        int fd = open(filename,O_CREAT | O_EXCL | O_WRONLY,0666);
        if(fd == -1)
        {
            close(fd);
            fd = open(filename,O_CREAT | O_TRUNC | O_WRONLY,0666);
            if(fd == -1)
            {
                ack = FAILURE;
            }
            else
            {
                ack = SUCCESS;
            }
        }
        else
        {
            ack = SUCCESS;
        }

        sendto(client->sockfd,&ack,4,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
        if(packet.body.request.mode == OCTATE)
        {
            while(1)
            {
                recvfrom(client->sockfd,&packet,sizeof(packet),0,(struct sockaddr *)&client->server_addr,&client->server_len);//receive data
                if(packet.body.data_packet.data_flag == 0)
                {
                    return;
                }
                if(packet.body.data_packet.data_size != 0)
                {
                    int ret = write(fd,packet.body.data_packet.data,packet.body.data_packet.data_size);

                    if(ret == -1)
                    {
                        printf("Write fails\n");
                        return;
                    }

                    packet.body.ack_packet.data_size = ret;
                    packet.body.ack_packet.block_number = packet.body.data_packet.block_number;
                    sendto(client->sockfd,&packet,sizeof(packet),0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));//send success ack
                }
                else
                {
                    //failure
                    packet.body.ack_packet.data_size = packet.body.data_packet.data_size;
                    packet.body.ack_packet.block_number = 0;
                    sendto(client->sockfd,&packet,sizeof(packet),0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));//send failure ack
                }
            }   
        }
        else if(packet.body.request.mode == NETASCII)
        {
            while(1)
            {
                recvfrom(client->sockfd,&packet,sizeof(packet),0,(struct sockaddr *)&client->server_addr,&client->server_len);//receive data 

                if(packet.body.data_packet.data_flag == 0)
                {
                    return;
                }
                if(packet.body.data_packet.data_size != 0)
                {
                    int ret;
                    char temp = '\r';
                    for(int i=0;i<packet.body.data_packet.data_size;i++)
                    {
                        if(packet.body.data_packet.data[i] == '\n')//checks for '\n'
                        {
                            write(fd,&temp,1);//writes '\r' into file
                            ret = write(fd,&packet.body.data_packet.data[i],1);
                        }
                        else
                        {
                            ret = write(fd,&packet.body.data_packet.data[i],1);
                        }
                    }
                    if(ret == -1)
                    {
                        printf("Write fails\n");
                        return;
                    }
                    packet.body.ack_packet.data_size = packet.body.data_packet.data_size;
                    packet.body.ack_packet.block_number = packet.body.data_packet.block_number;
                    sendto(client->sockfd,&packet,sizeof(packet),0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));//send ack to server
                }
                else
                {
                    packet.body.ack_packet.data_size = packet.body.data_packet.data_size;
                    packet.body.ack_packet.block_number = 0;
                    sendto(client->sockfd,&packet,sizeof(packet),0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
                }
            }   
        }
        else
        {
            while(1)
            {
                recvfrom(client->sockfd,&packet,sizeof(packet),0,(struct sockaddr *)&client->server_addr,&client->server_len);//receive data
                if(packet.body.data_packet.data_flag == 0)
                {
                    return;
                }
                if(packet.body.data_packet.data_size != 0)
                {
                    int ret = write(fd,packet.body.data_packet.data,packet.body.data_packet.data_size);//write to file

                    if(ret == -1)
                    {
                        printf("Write fails\n");
                        return;
                    }
                    packet.body.ack_packet.data_size = ret;
                    packet.body.ack_packet.block_number = packet.body.data_packet.block_number;
                    sendto(client->sockfd,&packet,sizeof(packet),0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));//send ack to server
                }
                else
                {
                    packet.body.ack_packet.data_size = packet.body.data_packet.data_size;
                    packet.body.ack_packet.block_number = 0;
                    sendto(client->sockfd,&packet,sizeof(packet),0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
                }
            }   
        }
    }
    else
    {
        printf("\nFILE DOES NOT EXIST IN SERVER...!\n");;
        return;
    }
}

void mode(tftp_client_t *client)
{
    int mode;
    printf("Enter Mode:\n\t1.Default\n\t2.Octate\n\t3.net ascii\n");
    scanf("%d",&mode);
    switch(mode)
    {
        case 1:
            packet.body.request.mode = DEFAULT;
            printf("TRANSFER MODE : DEFAULT\n");
            break;
        case 2:
             packet.body.request.mode = OCTATE;
             printf("TRANSFER MODE : OCTATE\n");
             break;
        case 3:
            packet.body.request.mode = NETASCII; 
            printf("TRANSFER MODE : NETACSII\n");
            break;
        default :
            printf("ENTERED MODE IS NOT APPLICABLE\nTRANSFER MODE : DEFAULT\n");           
    }
}


void disconnect(tftp_client_t *client) {
    // close fd
   close(client->sockfd);
   exit(0);
}

void send_request(int sockfd,struct sockaddr_in server_addr, char *filename, int opcode)
{
    //send request for write
    strcpy(packet.body.request.filename,filename);
    packet.body.request.opcode = opcode;
    sendto(sockfd,&packet,sizeof(packet),0,(struct sockaddr *)&server_addr,sizeof(server_addr));
}

void receive_request(int sockfd,struct sockaddr_in server_addr, char *filename, int opcode)
{
    //send request for read
    strcpy(packet.body.request.filename,filename);
    packet.body.request.opcode = opcode;
    sendto(sockfd,&packet,sizeof(packet),0,(struct sockaddr *)&server_addr,sizeof(server_addr));
}