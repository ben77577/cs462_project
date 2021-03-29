#include <iostream>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <iomanip>
#include <unistd.h>
#include "Server.hpp"


//constructor
Server::Server(std::string ip, std::string po, std::string pr_pa){
	ip_address = ip;
	port = po;
	if(pr_pa.compare("y") == 0){
		print_packets = true;
	}
	else{
		print_packets = false;
	}
}
		
int Server::start(){
	int clientSockfd;
	int portno;
	socklen_t clientLength;
	struct sockaddr_in serverAddr;
	struct sockaddr_in clientAddr;
   
	//create socket
	socketfd = socket(AF_INET, SOCK_STREAM, 0);
	if (socketfd < 0) {
		perror("error opening socket");
		exit(1);
	}
   
	bzero((char *) &serverAddr, sizeof(serverAddr));
	portno = std::stoi(port);
   
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(portno);
   
	//bind
	if (bind(socketfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
		perror("error binding");
		exit(1);
	}
      
	//listen for connections
	listen(socketfd,5);
	clientLength = sizeof(clientAddr);

	//accept a connection
	clientSockfd = accept(socketfd, (struct sockaddr *)&clientAddr, &clientLength);
	if (clientSockfd < 0) {
		perror("error on accept");
		exit(1);
	}
	return clientSockfd;
}
		
		
//read packets from the client
bool Server::readPackets(int newsockfd, const char* filename){
	FILE* openedFile;
	char* stdoutBuf = NULL;
	unsigned long stdoutBufLength = 0;
	bool printstdBool;
			
	//if the filename is empty
	if(*filename == 0){
		//open a memstrem to be printed later
		openedFile = open_memstream(&stdoutBuf, &stdoutBufLength);
		printstdBool = true;
	}
	else{
		//open the file for writing
		openedFile = fopen(filename, "w+");
		printstdBool = false;
	}
			
	if(openedFile == NULL){
		std::cout << "Error opening file to write. (Do you have w+ permissions for this file?)\n";
		exit(2);
	}
			
	//read in packet size and convert to int
	char pack_size_buff[8];
	int packSizeLength = read(newsockfd, pack_size_buff, 8);
	std::string p(pack_size_buff); 
	std::stringstream str(p); 
	int pack_size;  
	str >> pack_size;  
		
	//create and zero buffer
	char buffer[pack_size];
	bzero(buffer,pack_size);
			
			
	int amtRead;
	int packet_counter = 0;
	//read packets from client
	while((amtRead = read(newsockfd, buffer, pack_size)) != 0){
		
		//CRC code
		Checksum csum;
		std::string crc = csum.calculateCRC(std::string(buffer).substr(0, pack_size));
		
		//print packet information if appropriate
		if(print_packets){
			bool dots = true;
			std::cout<<std::dec;
			std::cout << "Received packet #" << packet_counter << " - ";
			for(int loop = 0; loop < amtRead; loop++){
				std::cout << buffer[loop];
			}
			
			//print checksum
			std::cout << "  - CRC: " << crc << "\n";
		}
				
		//write packet to file
		size_t cSize = sizeof(char);
		size_t writeSize = amtRead;
		fwrite(buffer,cSize, writeSize,openedFile);
				
		//increment counter and clear buffer
		packet_counter++;
		bzero(buffer,pack_size);
	}
			
	//clean up
	close(newsockfd);
	close(socketfd);
	fclose(openedFile);
			
	std::cout << "\nReceive Success!\n";
	if(printstdBool){
		std::cout << stdoutBuf << "\n";
		free(stdoutBuf);
	}
	return printstdBool;
}

