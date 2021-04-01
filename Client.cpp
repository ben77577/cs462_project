#include <iostream>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <iomanip>
#include <unistd.h>
#include <arpa/inet.h>
#include "Client.hpp"


//constructor
Client::Client(std::string ip, std::string po, std::string pr_pa){
	ip_address = ip;
	port = po;
	//set print_packets appropriatly
	if(pr_pa.compare("y") == 0){
		print_packets = true;
	}
	else{
		print_packets = false;
	}
}
		
		
//start the client
void Client::start(){
    struct sockaddr_in servaddr;
  
    //create the socket
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd == -1) {
        perror("Error creating socket: ");
        exit(1);
    }
   
    bzero(&servaddr, sizeof(servaddr));
  
    //assign the ip address and port
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip_address.c_str());
    servaddr.sin_port = htons(std::stoi(port));
  
    //connect the socket
    if (connect(socketfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
}
		
		
		//send packets to server
void Client::sendPacket(const char* filename, int pack_size){
	//total size of CRC, sequence numbers, etc...
	int packetInfoSize = 8;
	
	size_t cSize = sizeof(char);
	size_t pSize = pack_size;
	int result;
	//char * buffer;
	int buf_size = sizeof(char)*(pack_size+packetInfoSize);
			
	//open file for reading
	FILE* openedFile = fopen(filename, "r+");
	if(openedFile == NULL){
		std::cout << "Error opening file to read. (Do you have r+ permissions for this file?)\n";
		exit(2);
	}

	//set up buffer
	//buffer =(char*)malloc(buf_size);
	char buffer[buf_size];
	bzero(buffer,buf_size);
	
	if(buffer == NULL){
		fputs("Error setting up buffer",stderr); 
		exit(2);
	}
			
			
	//set up pack_size to send (ADD 8 TO PACK_SIZE IF ADDING CRC)
	std::string pack_size_string = std::to_string(pack_size + packetInfoSize);
	//pad on left with 0 until 8 characters long
	while(pack_size_string.length() != 8){
		pack_size_string = "0" + pack_size_string;
	}
	char const *pack_size_char_arr = pack_size_string.c_str();
	//write packet size
	write(socketfd, (char*)pack_size_char_arr, 8);
			
	
			
	int packet_counter = 0;
	//read information from file, pSize chars at a time
	while((result = fread(buffer, cSize, pSize, openedFile)) > 0){
		
		//CRC code
		Checksum csum;
		std::string crc = csum.calculateCRC(std::string(buffer));
		
		//print the packet if appropriate
		if(print_packets){
			bool dots = true;
			std::cout<<std::dec;
			std::cout << "Sent packet #" << packet_counter << " - ";
			for(int loop = 0; loop < result; loop++){
				std::cout << buffer[loop];
			}
			std::cout << "\n";
			//print checksum
			//std::cout << "  - CRC: " << crc << "\n";
		}
		
		//write the packet to the server (ADD 8 TO RESULT IF ADDING CRC) (and use this to add crc to end of buffer std::strcat(buffer, crc.c_str()))
		write(socketfd, std::strcat(buffer, crc.c_str()), (result + packetInfoSize));
		packet_counter++;
		bzero(buffer,buf_size);
	}
	//clean up
	//free(buffer);
	close(socketfd);
	fclose(openedFile);
			
	std::cout<<std::dec;
	std::cout << "\nSend Success!\n";
}
