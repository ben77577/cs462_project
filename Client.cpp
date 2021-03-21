#include <iostream>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <iomanip>
#include <unistd.h>
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
	struct addrinfo hints, *result;
	int error;

	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;

	//convert ip address and port
	if ((error = getaddrinfo(ip_address.c_str(), port.c_str(), &hints, &result)) != 0) {
		printf("Error %d : %s\n", error, gai_strerror(error));
		return;
	}

	//create the socket and connect
	socketfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	connect(socketfd, result->ai_addr, result->ai_addrlen);
	freeaddrinfo(result);	
}
		
		
		//send packets to server
void Client::sendPacket(const char* filename, int pack_size){
	size_t cSize = sizeof(char);
	size_t pSize = pack_size;
	int result;
	char * buffer;
	int buf_size = sizeof(char)*pack_size;
			
	//open file for reading
	FILE* openedFile = fopen(filename, "r+");
	if(openedFile == NULL){
		std::cout << "Error opening file to read. (Do you have r+ permissions for this file?)\n";
		exit(2);
	}

	//set up buffer
	buffer =(char*)malloc(buf_size);
	if(buffer == NULL){
		fputs("Error setting up buffer",stderr); 
		exit(2);
	}
			
			
	//set up pack_size to send
	std::string pack_size_string = std::to_string(pack_size);
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
				
		//print the packet if appropriate
		if(print_packets){
			bool dots = true;
			std::cout<<std::dec;
			std::cout << "Sent packet #" << packet_counter << " - ";
			for(int loop = 0; loop < result; loop++){
				std::cout << buffer[loop];
			}
			std::cout << "\n";
		}
		//write the packet to the server
		write(socketfd, buffer, result);
		packet_counter++;
		bzero(buffer,buf_size);
	}
	//clean up
	free(buffer);
	close(socketfd);
	fclose(openedFile);
			
	std::cout<<std::dec;
	std::cout << "\nSend Success!\n";
}
