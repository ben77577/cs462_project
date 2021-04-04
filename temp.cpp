#include <iostream>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <iomanip>
#include <unistd.h>
#include <arpa/inet.h>
#include <regex>
#include "Client.hpp"
#include <thread>
#include <ctime>
#include <iostream>

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
//method to be called when expected panels[0] is found
void Client::handleExpected(int window_size){
	//represents if first panel in window is ack'd
int expectedFound = 1;
while (expectedFound) {
	expectedFound = 0;
	//lock all pkts for shifting
	for (int i = 0; i<window_size; i++) {
		panels[i].lockPkt();
	}
	//shift panels' values over to left by 1
	for (int i = 0; i<window_size-1; i++) {
		panels[i].fillBuffer(panels[i+1].getBuffer());
		if(panels[i+1].isSent) {panels[i].markAsSent();}
		if(panels[i+1].isReceived) {panels[i].markAsReceived();}
		panels[i].setSeqNum(panels[i+1].getSeqNum());
	}
	//empty last panel
	panels[i].setAsEmpty();
	//if expected panel is ack'd continue while
	if (panels[0].isReceived()) {expectedFound=1;}
}
	for (int i = 0; i<window_size; i++) {
		panels[i].releasePkt();
	}
}
//thread to find unsent/timedout pkts and resend/send pkts appropriately
void Client::writePacket(char * buffer, int window_size, size_t pSize){
	while (1) {
		for (int i = 0; i<10; i++) {
			if (panels[i].isSent() == 0) {
				send(socketfd, panels[window_size].getBuffer(), pSize+8+5,0)
				if (!send) {
					std::cout<<"Packet #"<<panels[window_size].getSeqNum() << "failed to send";
				}
			}else {
				std::time_t sentTime = std::time(nullptr);
    			std::asctime(std::localtime(&sentTime));
				panels[i].setTimeSent(sentTime);
			}
			)
		}
	}
}
//thread to read socket and mark panels as ack'd
void Client::readAck(char * buffer, int window_size){
while(read(socketfd, buffer, 8)){ 
	if (read<0) {
		std::cout<<"Failure to catch ACK \n";
		}else{
			std::string ack = std::string(buffer);
			std::cout<<"Ack #"<<ack<<" received \n";
			int ackComp = std::stoi(ack);
			std::cout<<"ackComp"<<ackComp;
			//find appropriate panel - seqNum
			for (int i = 0; i<window_size; i++) {
				if (panels[i].getSeqNum()==ackComp) {
					std::cout<<"Found the right panel \n";
					handleExpected(window_size);
			}
		}
	}
	}
}
void Client::initializePanel(){
	for (int i = 0; i<10; i++) {
		panels[i] = Panel(i);
	}
}
		//send packets to server
void Client::sendPacket(const char* filename, int pack_size, int window_size, int sequence_max){
	//initialize panel
	initializePanel();
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
			
	
	//create window & initiliaze panels within - CHANGE: Initialize as initial pkts read in, old ones will be replaced with new when mepty mutex is unlocked. 
	int panels[window_size];
	for (int i = 0; i<window_size; i++) {
		panels[i] = i;
		std::cout<<"panel val: " <<panels[i];
	}
	int currentPanel = 0;
	int packet_counter = 0;
	//start ACK thread
		//std::thread rPkt([&](){ Client::readAck(buffer);});
		//std::thread wPkt([&](){ Client::writeAck(buffer);});
	//read information from file, pSize chars at a time
	while((result = fread(buffer, cSize, pSize, openedFile)) > 0){
		//CRC code
		Checksum csum;
		std::string crc = csum.calculateCRC(std::string(buffer));
		//construct id
		std::string id = std::to_string(packet_counter);
		std::string leftPad = "00000";
		id = leftPad.substr(0,5-id.length())+id;
		//ULTIMAELY - panels[current locked panel].fillBuffer(std::strcat(std::strcat(buffer,src_c_str()),id.c_str())));
		int noEmpty = 1;
		while (noEmpty) {
			for (int i = 0; i<10; i++) {
				if (panels[i].getSeqNum()<0) {
					panels[i].lockPkt();
					panels[i].fillBuffer(std::strcat(std::strcat(buffer,src.c_str()),id.c_str()));
					panels[i].setSeqNum(id);
					panels[i].
					panels[i].releasePkt();
					noEmpty = 0;
				}
			}
		}
		//write the packet to the server (ADD 8+5 TO RESULT IF ADDING CRC+ID) (and use this to add crc to end of buffer std::strcat(buffer, crc.c_str()))
		/**send(socketfd, std::strcat(std::strcat(buffer, crc.c_str()),id.c_str()), pSize+8+5, 0);
		if (!send) {
			std::cout<<"Packet #"<<id<<" failed to send.";
		}else{
			std::cout<<"mark Panel as sent" << currentPanel << "\n";
			//panels[currentPanel].markAsSent();
			//print the packet if appropriate
			if(print_packets){
				bool dots = true;
				std::cout<<std::dec;
				std::cout << "Sent packet #" << id << " - ";
				for(int loop = 0; loop < result; loop++){
					std::cout << buffer[loop];
				}
				std::cout << "\n";
				//print checksum
				//std::cout << "  - CRC: " << crc << "\n";
			}
		}
		bzero(buffer,buf_size);
		//panels[currentPanel].mtxPktLock.unlock();
		read(socketfd,buffer,8);**/
		//}
	bzero(buffer,8);
	if (packet_counter == sequence_max) {
		packet_counter=0;
	}else {
		packet_counter++;
	}
	if (currentPanel == 5) {
		currentPanel = 0;
	}else{
		currentPanel++;
	}
	}
	//clean up
	//free(buffer);
	close(socketfd);
	fclose(openedFile);
			
	std::cout<<std::dec;
	std::cout << "\nSend Success!\n";
}
