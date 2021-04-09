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
Server::Server(std::string ip, std::string po, std::string pr_pa, ErrorCreate* er){
	ip_address = ip;
	port = po;
	if(pr_pa.compare("y") == 0){
		print_packets = true;
	}
	else{
		print_packets = false;
	}
	errorObj = er;
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
	std::cout << "Listening...\n";
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
	int currentPktSeq = 0;
	char pack_size_buff[30];
	int packSizeLength = read(newsockfd, pack_size_buff, 30);
	std::cout<<pack_size_buff<<"\n";
	int windowSize;
	int pack_size;
	int seq_num;
	std::string pack_size_string(pack_size_buff, 30);
	pack_size = std::stoi(pack_size_string.substr(0,8));
	windowSize = std::stoi(pack_size_string.substr(8,8));
	seq_num = std::stoi(pack_size_string.substr(16, 10));
	std::cout<<pack_size << " " << windowSize << " " << seq_num << "\n";
	std::string p(pack_size_buff); 
	//std::stringstream str(p); 
	//int pack_size;  
	//str >> pack_size;  
	std::cout<<"pack_size"<<pack_size<<"\n";
	//create and zero buffer
	char buffer[pack_size];
	bzero(buffer,pack_size);
	int receivedPktSize = -1;
	std::string rps_string = std::to_string(receivedPktSize);
	send(newsockfd, std::strcat(buffer, rps_string.c_str()), 8, 0);
	bzero(buffer,8);
	if (!send) {
		std::cout<<"Failure to send RTT ACK \n";
	}else{
		std::cout<<"Successfully sent RTT ACK\n";
	}
	Panel sPanels[windowSize]= {};
	for (currentPktSeq = 0; currentPktSeq<windowSize; currentPktSeq++) {
		(sPanels+currentPktSeq)->setSeqNum(currentPktSeq);
		(sPanels+currentPktSeq)->markAsUnsent();
		(sPanels+currentPktSeq)->markNotReceived();
		(sPanels+currentPktSeq)->allocateBuffer(pack_size-13);
	}
	//total size of CRC, sequence numbers, etc...		
	int packetInfoSize = 8;
	int idSize = 5;
	
	int amtRead;
	int packet_counter = 0;
	//read packets from client
	while((amtRead = read(newsockfd, buffer, pack_size)) > 0){
		int currentPkt = -1;
		amtRead = strlen(buffer);
		//take client crc & id off buffer		
		std::cout<<amtRead-packetInfoSize<<"\n";
		std::string clientCrc = std::string(buffer).substr(amtRead-(packetInfoSize),packetInfoSize);
		std::cout<<"crc: " << clientCrc<<"\n";
		std::string id = std::string(buffer).substr(amtRead-(packetInfoSize+idSize),idSize);
		std::cout<<"id: " << id<<"\n";
		//CRC code - run crc on server side
		Checksum csum;
		std::string currentBuffer = std::string(buffer).substr(0, amtRead-(packetInfoSize+idSize));
		char * tempBuffer = new char[pack_size-13];
		memcpy(tempBuffer, buffer, amtRead);
		std::cout<<"String to be crc'd: " << currentBuffer << "\n" << tempBuffer << "\n";;
		std::string crc = csum.calculateCRC(currentBuffer);
		std::string leftPad = "00000000";
		crc = leftPad.substr(0, 8 - crc.length()) + crc;
		std::cout<<crc<<"\n";
		for (int i = 0; i<windowSize; i++) {
			if ((sPanels+i)->getSeqNum() == std::stoi(id)){
				(sPanels+i)->markAsReceived();
				(sPanels+i)->setPktSize(amtRead);
				(sPanels+i)->fillBuffer(tempBuffer, pack_size-13);
				bzero(tempBuffer, pack_size-13);
				currentPkt = i;
				(sPanels+i)->summary();
				i = windowSize;
			}
		}
		//print packet information if appropriate
		if(print_packets){
			bool dots = true;
			std::cout<<std::dec;
			std::cout << "Received packet #" << packet_counter << " - ";
			for(int loop = 0; loop < amtRead - packetInfoSize; loop++){
				std::cout << buffer[loop];
			}
			std::cout<<"\n";
			//print checksum
			//std::cout << "  - CRC: " << crc << "    Client-calculated CRC: "<< clientCrc << "\n";
		}
		
		//test if the two crcs are equal
		if(clientCrc == crc){
			//equal
			std::cout << "   - Checksum ok\n";
		}
		else{
			std::cout << "   - Checksum failed\n";
		}				
		//increment counter and clear buffer
		packet_counter++;
		bzero(buffer,pack_size);
		std::cout<<"id recieved: " << id << "\n";
		if (clientCrc == crc) {
			//check for lose ack error before sending ack
			//std::cout << "ACK ID::: "<<std::stoi(id)<<"\n";
			std::string introduceError = (*errorObj).getPacketError(std::stoi(id));
			if(introduceError == "la"){
				std::cout << "\nLOSE ACK " << id << "\n";
				//(*errorObj).erasePacketError(std::stoi(id));
			}else{
				send(newsockfd, std::strcat(buffer, id.c_str()), 8, 0);
				if (!send) {
					std::cout<<"Failure to send ACK for packet #" << id<<"\n";
				}else{
					std::cout<<"ACK " << id << " sent\n";
					if (currentPkt!= -1) {
						(sPanels+currentPkt)->markAsSent();
					}
				}
			}
			
			bzero(buffer,pack_size);
		}
		while ((sPanels)->isSent() && (sPanels)->isReceived()) {
			//write packet to file
			size_t cSize = sizeof(char);
			memcpy(tempBuffer, (sPanels)->getBuffer(), amtRead-13);
			fwrite(tempBuffer, cSize, amtRead - 13,openedFile);
			currentPktSeq = shift(sPanels, windowSize, currentPktSeq);
		}
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
int Server::shift(Panel *panels, int window_size, int currentPktSeq) {
	int shiftPanel = 0;
	while (shiftPanel<window_size-1) {
		(panels+shiftPanel)->setSeqNum((panels+shiftPanel+1)->getSeqNum());
		(panels+shiftPanel)->fillBuffer((panels+shiftPanel+1)->getBuffer(), (panels+shiftPanel+1)->getPktSize());
		if ((panels+shiftPanel+1)->isReceived()) {
			(panels+shiftPanel)->markAsReceived();
		}else{(panels+shiftPanel)->markNotReceived();}
		if ((panels+shiftPanel+1)->isSent()) {
			(panels+shiftPanel)->markAsSent();
		}else{(panels+shiftPanel)->markAsUnsent();}
		shiftPanel++;
	}
	while (shiftPanel<window_size) {
		(panels+shiftPanel)->setAsEmpty();
		(panels+shiftPanel)->setSeqNum(currentPktSeq);
		currentPktSeq++;
		shiftPanel++;
	}
	return currentPktSeq;
}
