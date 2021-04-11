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
		print_detailed = true;
	}
	else{
		print_detailed = false;
	}
	errorObj = er;
	retransmittedCount = 0;
	lastSeqNum = 0;
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
	int currentPacket = 0;
	char pack_size_buff[26];
	int packSizeLength = read(newsockfd, pack_size_buff, 26);
	int windowSize;
	int pack_size;
	int seq_num;
	std::string pack_size_string(pack_size_buff, 30);
	pack_size = std::stoi(pack_size_string.substr(0,8));
	windowSize = std::stoi(pack_size_string.substr(8,8));
	seq_num = std::stoi(pack_size_string.substr(16, 8));
	std::string p(pack_size_buff); 
	bzero(pack_size_buff, 26);
	//std::stringstream str(p); 
	//int pack_size;  
	//str >> pack_size;  
	//create and zero buffer
	char buffer[pack_size];
	bzero(buffer,pack_size);
	int receivedPktSize = -1;
	std::string rps_string = std::to_string(receivedPktSize);
	send(newsockfd, std::strcat(pack_size_buff, rps_string.c_str()), 8, 0);
	bzero(pack_size_buff,8);
	if (!send) {
		std::cout<<"Failure to send RTT ACK \n";
	}else{
		if(print_detailed){
			std::cout<<"Successfully sent RTT ACK\n";
		}
	}
	Panel sPanels[windowSize]= {};
	for (currentPacket = 0; currentPacket<windowSize; currentPacket++) {
		(sPanels+currentPacket)->setPackNum(currentPacket);
		(sPanels+currentPacket)->markAsUnsent();
		(sPanels+currentPacket)->markAsNotReceived();
		(sPanels+currentPacket)->allocateBuffer(pack_size-13);
		(sPanels+currentPacket)->setAsOccupied();
	}
	//total size of CRC, sequence numbers, etc...		
	int packetInfoSize = 8;
	int idSize = 5;
	
	int amtRead;
	//read packets from client
	while((amtRead = read(newsockfd, buffer, pack_size)) > 0){
		retransmittedCount++;
		if(strlen(buffer) < pack_size){
			amtRead = strlen(buffer);
		}
		
		//take client crc & id off buffer		
		std::string clientCrc = std::string(buffer).substr(amtRead-(packetInfoSize),packetInfoSize);
		std::string id = std::string(buffer).substr(amtRead-(packetInfoSize+idSize),idSize);
		
		//id to print at the end
		curId = std::stoi(id);
		//CRC code - run crc on server side
		Checksum csum;
		std::string currentBuffer = std::string(buffer).substr(0, amtRead-13);
		std::string crc = csum.calculateCRC(currentBuffer);
		std::string leftPad = "00000000";
		crc = leftPad.substr(0, 8 - crc.length()) + crc;
		bool anyBufferFilled = false;
		for (int i = 0; i<windowSize; i++) {
			if ((sPanels+i)->getPackNum() == std::stoi(id)){
				(sPanels+i)->markAsReceived();
				(sPanels+i)->setPktSize(amtRead-13);
				(sPanels+i)->fillBuffer(buffer, amtRead-13);
				std::cout<<"Filled buffer: " << (sPanels+i)->getBuffer()<< "\n";
				i = windowSize;
				anyBufferFilled = true;
			}
		}
		//print packet information if appropriate

		bool dots = true;
		
		std::cout << "Packet #" << std::stoi(id)%seq_num << " received\n";
		//for(int loop = 0; loop < amtRead - (packetInfoSize+idSize); loop++){
		//	std::cout << buffer[loop];
		//}
		
		
		
		//test if the two crcs are equal
		if(print_detailed){
			if(clientCrc == crc){
				//equal
				std::cout << "   - Checksum ok\n";
			}
			else{
				std::cout << "   - Checksum failed\n";
			}
		}
		//increment counter and clear buffer
		//std::cout<<(std::stoi(id)<=(sPanels+windowSize-1)->getPackNum())<<("\n");
		if (clientCrc == crc && std::stoi(id) <= (sPanels+windowSize-1)->getPackNum()) {
			//check for lose ack error before sending ack
			//std::cout << "ACK ID::: "<<std::stoi(id)<<"\n";
			std::string introduceError = (*errorObj).getPacketError(std::stoi(id));
			if(introduceError == "la"){
				std::cout << "\nLose Ack " << std::stoi(id)%seq_num << "\n";

				//(*errorObj).erasePacketError(std::stoi(id));
			}else{
				bzero(buffer, pack_size);
				send(newsockfd, std::strcat(buffer, id.c_str()), 8, 0);
				if (!send) {
					std::cout<<"Failure to send ACK for packet #" << id<<"\n";
				}else{
					if(print_detailed){
						std::cout<<"Ack " << std::stoi(id)%seq_num << " sent\n";
					}
				}
			}
			
			for (int ackShift = 0; ackShift<windowSize; ackShift++) {
				if (std::stoi(id)==(sPanels+ackShift)->getPackNum()) {
					(sPanels+ackShift)->markAsSent();
				}
			} 
			
		}
		while ((sPanels)->isSent() && (sPanels)->isReceived()) {
			//write packet to file
			size_t cSize = sizeof(char);
			bzero(buffer, pack_size);
			fwrite((sPanels)->getBuffer(), cSize, amtRead - 13,openedFile);
			lastSeqNum = std::stoi(id)%seq_num;
			(sPanels)->setAsEmpty();
			(sPanels)->setAsOccupied();
			currentPacket = shift(sPanels, windowSize, currentPacket, seq_num, pack_size-13);
		}
		bzero(buffer, pack_size);
	}
			
	//clean up
	close(newsockfd);
	close(socketfd);
	fclose(openedFile);
			
	
	if(printstdBool){
		std::cout << stdoutBuf << "\n";
		free(stdoutBuf);
	}
	
	std::cout << "\nLast packet seq# received: " << lastSeqNum << "\n";
	std::cout << "Number of original packets received: " << (curId+1) << "\n";
	std::cout << "Number of retransmitted packets received: " << retransmittedCount-(curId+1) << "\n\n";
	
	return printstdBool;
}
int Server::shift(Panel *panels, int window_size, int currentPacket, int seqNum, int pack_size) {
	int shiftPanel = 0;
	int nextPktSize = 0;
	while (shiftPanel<window_size-1) {
		(panels+shiftPanel)->setPackNum((panels+shiftPanel+1)->getPackNum());
		if ((panels+shiftPanel+1)->getPktSize()==0) {
			nextPktSize = pack_size;
		}else{nextPktSize = (panels+shiftPanel+1)->getPktSize();}
		(panels+shiftPanel)->fillBuffer((panels+shiftPanel+1)->getBuffer(), nextPktSize);
		if ((panels+shiftPanel+1)->isReceived()) {
			(panels+shiftPanel)->markAsReceived();
		}else{(panels+shiftPanel)->markAsNotReceived();}
		if ((panels+shiftPanel+1)->isSent()) {
			(panels+shiftPanel)->markAsSent();
		}else{(panels+shiftPanel)->markAsUnsent();}
		shiftPanel++;
	}
	while (shiftPanel<window_size) {
		(panels+shiftPanel)->setAsEmpty();
		(panels+shiftPanel)->setPackNum(currentPacket);
		(panels+shiftPanel)->setAsOccupied();
		currentPacket++;
		shiftPanel++;
	}
	
	//print window
	if(print_detailed){
		std::cout << "\nCurrent window = [";
		for(int loop = 0; loop < window_size; loop++){
			if(loop == window_size -1){
				std::cout << (panels + loop)->getPackNum()%seqNum << "]";
			}
			else{
				std::cout << (panels + loop)->getPackNum()%seqNum << ", ";
			}
		}
		std::cout<<"\n";
	}
	return currentPacket;
}
