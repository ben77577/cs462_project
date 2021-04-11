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
#include <thread>
#include <chrono>

#include "Client.hpp"

//constructor
Client::Client(std::string ip, std::string po, std::string pr_pa, std::string protocolType, uint64_t timeoutVal, ErrorCreate* er){
	ip_address = ip; 
	port = po;
	//set print_detailed appropriatly
	if (pr_pa.compare("y") == 0)
	{
		print_detailed = true;
	}
	else
	{
		print_detailed = false;
	}
	errorObj = er;
	foundEndFile = false;
	
	//use go back n or selective repeat
	if(protocolType.compare("gbn") == 0){
		gbn = true;
	}
	else{
		gbn = false;
	}
	timeout = timeoutVal;
	endPacketCount = 0;
	retransmittedCount = 0;
}


//start the client
void Client::start()
{
	struct sockaddr_in servaddr;

	//create the socket
	socketfd = socket(AF_INET, SOCK_STREAM, 0);
	if (socketfd == -1)
	{
		perror("Error creating socket: ");
		exit(1);
	}

	bzero(&servaddr, sizeof(servaddr));

	//assign the ip address and port
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(ip_address.c_str());
	servaddr.sin_port = htons(std::stoi(port));

	//connect the socket
	if (connect(socketfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
	{
		printf("connection with the server failed...\n");
		exit(0);
	}
}


int Client::writeMyPkt(Panel *panel) {
	char * writebuffer =(char*)malloc(buf_size);
	bzero(writebuffer, buf_size);

	if (writebuffer == NULL){
		std::lock_guard<std::mutex> client_print(clientPrint);
		fputs("Error setting up buffer", stderr);
		exit(2);
	}
	
	int foundEOF = 0;
	std::lock_guard<std::mutex> window_lock(windowLock);

	bool timedOut = false;
	bool dropPacket = false;
	bool dontSend = false;
	for (int writeLoop = 0; writeLoop < window_size; writeLoop++){
		time_t now;	
		//if the panel is the last
		if ((panel)->isLast()) {
			//EOF found in first panel then exit function
			foundEOF = 1;
			foundEndFile = true;
			return foundEOF;
		}		
			
			
		//is sent && didn't receive an ack && timed out
		int timeDifference = milliNow() - (panel+writeLoop)->getTimeSent();
		if(((panel+writeLoop)->isSent() == 1 && (panel+writeLoop)->isReceived() == 0) &&  timeDifference > timeout){
			timedOut = true;
			std::lock_guard<std::mutex> client_print(clientPrint);
			std::cout << "\nPacket #" << (panel+writeLoop)->getPackNum() << " *****TIMED OUT*****";
			if(gbn){
				//clear out
				for(int loop = 0; loop < window_size && !(panel+loop)->isLast(); loop++){
					(panel+loop)->markAsNotSent();
					(panel+loop)->markAsNotReceived();
					(panel+loop)->markRetransmit();
				}
			}
		}
		else{
			timedOut = false;
		}
			
			
		//not empty && (not sent || timed-out) && not last
		if ((panel + writeLoop)->isEmpty() == 0 && ((panel + writeLoop)->isSent() == 0 || timedOut)){
			int writeID = (panel + writeLoop)->getPackNum();
			writebuffer = (panel + writeLoop)->getBuffer();

			char buff[pSize + 13];
			strcpy(buff,writebuffer);
				
			dontSend = false;
			dropPacket = false;
				
			if(buff[0] == '\0') {
				//std::cout<<"buff[0]=='0'\n";
				dontSend = true;
			}
				
			//Introduce errors if applicable
			std::string introduceError = (*errorObj).getPacketError(writeID);
			if(introduceError != "na"){
				if(introduceError == "da"){
					//damage packet
					buff[0] = buff[0]+1;
					std::lock_guard<std::mutex> client_print(clientPrint);
					std::cout << "\nPacket " << (panel+writeLoop)->getPackNum() << " damaged";
				}
				else if(introduceError == "dr"){
					dropPacket = true;
					std::lock_guard<std::mutex> client_print(clientPrint);
					std::cout << "\nPacket " << (panel+writeLoop)->getPackNum() << " dropped";
				}
			}
				
			//send packet
			if(!dropPacket && !dontSend){
				send(socketfd, buff, pSize + 13, 0);
			}
					
			if (!send){
				std::lock_guard<std::mutex> client_print(clientPrint);
				std::cout << "writePacket: Packet #" << writeID << " failed to send.\n";
			}else if(!dontSend){
				(panel + writeLoop)->markAsSent();
				
				if(timedOut || (panel + writeLoop)->getRetransmit()){
					std::lock_guard<std::mutex> client_print(clientPrint);
					std::cout << "\nPacket #" << (panel + writeLoop)->getSeqNum() << " Re-transmitted";
					retransmittedCount++;
					if((panel + writeLoop)->getRetransmit()){
						(panel + writeLoop)->markUnRetransmit();
					}
				}
				else{
					std::lock_guard<std::mutex> client_print(clientPrint);
					std::cout << "\nPacket #" << (panel + writeLoop)->getSeqNum() << " sent";
				}	
				
				time_t sentTime;
				time(&sentTime);
						
				(panel + writeLoop)->setTimeSent(milliNow());
				writeLoop = window_size;
			}
		}
	}
	return foundEOF;
}


//thread to find unsent/timedout pkts and resend/send pkts appropriately
void Client::writePacket(Panel *panel)
{
	int foundEOF = 0;
	while(!foundEOF) {
		foundEOF = writeMyPkt(panel);
	}
}


//handles shifting of window when expected pkt is ack'd - NOT A THREAD
void Client::handleExpected(Panel *panel, int window_size){
	//lock all panels - blocking any new change from threads
	int shift = 0;
	
	//loop until the first in window is no longer ack'd
	int expectedReceived = 1;
	int shiftToEmpty = 1;
	while (expectedReceived){
		expectedReceived = 0;
		shift = 0;
		
		//loop until found the end of window or next panel is empty
		while (shift < window_size - 1 && !(panel + shift + 1)->isEmpty()){
			(panel+shift)->setSeqNum((panel+shift+1)->getSeqNum());
			(panel+shift)->fillBuffer((panel+shift+1)->getBuffer(), buf_size);
			(panel+shift)->setPktSize((panel+shift+1)->getPktSize());
			(panel+shift)->setTimeSent((panel+shift+1)->getTimeSent());
			(panel+shift)->setPackNum((panel+shift+1)->getPackNum());
			if ((panel+shift+1)->isLast()) {
				(panel+shift)->setAsLast();
			}
			
			if ((panel+shift+1)->isSent()){
				(panel+shift)->markAsSent();
			}
			else{
				(panel+shift)->markAsNotSent();
			}
			
			if ((panel+shift+1)->isReceived()){
				(panel+shift)->markAsReceived();
			}
			else{
				(panel+shift)->markAsNotReceived();
			}
			shift++;
		}
		
		while(shift<window_size) {
			(panel + shift)->setAsEmpty();
			shift++;
		}
		
		//reset loop variable if necessary
		if ((panel)->isReceived() == 1){
			expectedReceived = 1;
			shiftToEmpty++;
		}else if (!(panel)->isReceived() || (panel)->isLast()){
			expectedReceived = 0;
		}
	}
	
	//print window
	if(print_detailed){
		std::lock_guard<std::mutex> client_print(clientPrint);
		std::cout << "\nCurrent window = [";
		for(int loop = 0; loop < window_size; loop++){
			if(loop == window_size -1){
				std::cout << (panel + loop)->getPackNum()%seq_max << "]";
			}
			else{
				std::cout << (panel + loop)->getPackNum()%seq_max << ", ";
			}
		}
	}
}


//thread to read socket and mark panels as ack'd
void Client::readAck(Panel *panel){
	char * readBuffer =(char*)malloc(buf_size);
	//char buffer[buf_size];
	bzero(readBuffer, buf_size);

	if (readBuffer == NULL){
		std::lock_guard<std::mutex> client_print(clientPrint);
		fputs("Error setting up buffer", stderr);
		exit(2);
	}
	
	while(!(panel)->isLast()) {	
		read(socketfd, readBuffer, 8);
		if (read < 0){
			std::lock_guard<std::mutex> client_print(clientPrint);
			std::cout << "readAck: Failure to catch ACK \n";
		}
		else{
			std::string ack = std::string(readBuffer);
			if(print_detailed){
				std::lock_guard<std::mutex> client_print(clientPrint);
				std::cout << "\nAck #" << ack << " received";
			}
			int ackComp = std::stoi(ack);
			std::lock_guard<std::mutex> window_lock(windowLock);
			
			for (int i = 0; i < window_size; i++){
				if ((panel + i)->getPackNum() == ackComp){
					//ack panel
					(panel+i)->markAsReceived();
					if (i == 0){
						//shift window
						handleExpected(panel, window_size);
					}
				}
			}
		}
	bzero(readBuffer, sizeof(readBuffer));
	}
}


//find and populate empty panel
int Client::findAndFillBuffer(Panel *panel, char *buffer, int packet_counter, int packet_number, int result) {
	int foundEmpty = 0;
	std::lock_guard<std::mutex> window_lock(windowLock);
	
	for (int i = 0; i < window_size; i++){
		if ((panel + i)->isEmpty()){	
			(panel + i)->fillBuffer(buffer, buf_size);
			(panel + i)->setSeqNum(packet_counter);
			(panel + i)->setAsOccupied();
			(panel + i)->setPktSize(result);
			(panel + i)->setPackNum(packet_number);
			//exit while
			foundEmpty = 1;
			//exit for
			i = window_size;
		}
	}

	return foundEmpty;
}


//find and populate empty panel with EOF information
int Client::findAndFillEOF(Panel *panel) {
	int foundEmpty = 0;
	std::lock_guard<std::mutex> window_lock(windowLock);
	
	for (int i = 0; i < window_size; i++){
		if ((panel + i)->isEmpty()){
			(panel+i)->setAsOccupied();
			(panel+i)->setAsLast();
			//exit while
			foundEmpty = 1;
			//exit for
			i = window_size;
		}
	}
	return foundEmpty;
}


//send packets to server
void Client::sendPacket(const char *filename, Panel *panel, int pack_size){
	//create threads
	//call write to loop through window and check for pkts to send
	std::thread wPkt([&](){ Client::writePacket(panel);});
	//call to read socket for acks and change appropriate panel in window
	std::thread rPkt([&](){ Client::readAck(panel);});
	
	//create buffer
	char * fillPacket = (char *)malloc(buf_size);
	bzero(fillPacket, buf_size);
	int sn = seq_max;
	int result;
	
	//open file for reading
	FILE *openedFile = fopen(filename, "r");
	if (openedFile == NULL){
		std::lock_guard<std::mutex> client_print(clientPrint);
		std::cout << "sendpacket: Error opening file to read. (Do you have r permissions for this file?)\n";
		exit(2);
	}
	
	int packet_seqNum = 0;
	int currentPacket = 0;
	int foundEOF = 0;
	while ((result = fread(fillPacket, cSize, pSize, openedFile)) > 0){	
		//CRC code
		Checksum csum;
		std::string crc = csum.calculateCRC(std::string(fillPacket));
		std::string leftPad = "00000000";
		crc = leftPad.substr(0, 8 - crc.length()) + crc;
		
		//construct id
		std::string id = std::to_string(currentPacket);
		leftPad = "00000";
		id = leftPad.substr(0, 5 - id.length()) + id;
		
		//combine buffer, id, and crc
		std::strcat(std::strcat(fillPacket, id.c_str()), crc.c_str());
		
		//find an empty panel
		//if found, then populate pkt with appropriate information
		//if none, then pause and wait till there is one
		int emptyNotFound = 0;
		while (!emptyNotFound){
			emptyNotFound = findAndFillBuffer(panel, fillPacket, packet_seqNum, currentPacket, result);
		}
		
		//reset sequence counter
		if (packet_seqNum == seq_max){
			packet_seqNum = 0;
		}else{
			packet_seqNum++;
		}
		
		currentPacket++;
		bzero(fillPacket, buf_size);
	}
	
	//end of file
	int emptyNotFound = 0;
	while (!emptyNotFound){
		emptyNotFound = findAndFillEOF(panel);
	}
	
	endPacketCount = currentPacket;
	
	//join threads and close file/socket
	wPkt.join();
	rPkt.join();
	close(socketfd);
	fclose(openedFile);
	std::cout << std::dec;
	std::cout << "\n\nSession successfully terminated!\n";
}


//calculates the current epoch in milliseconds
uint64_t Client::milliNow() {
  using namespace std::chrono;
  return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
}


//Initializes global variables & begins sendPacket thread
void Client::startThreads(const char *filename, int pack_size, int windowSize, int sequence_max){
	//initialize array(window)
	Panel panel[windowSize] = {};
	
	//set global variables
	//total size of CRC and sequence number
	int packetInfoSize = 13;
	seq_max = sequence_max;
	window_size = windowSize;
	int serverWindowSize = window_size;
	if (gbn) {serverWindowSize = 1;}
	cSize = sizeof(char);
	pSize = pack_size;
	buf_size = sizeof(char) * (pack_size + packetInfoSize);
	
	//set up buffer
	char * buffer =(char*)malloc(buf_size);
	bzero(buffer, buf_size);

	if (buffer == NULL){
		fputs("Error setting up buffer", stderr);
		exit(2);
	}
	
	//allocate panel buffers
	for (int initLoop = 0; initLoop<windowSize; initLoop++) {
		(panel+initLoop)->allocateBuffer(buf_size);
	}
	//set up pack_size to send (ADD 8 TO PACK_SIZE IF ADDING CRC)
	std::string pack_size_string = std::to_string(pack_size + packetInfoSize);
	std::string windowSizeString = std::to_string(serverWindowSize);
	std::string seqNumString = std::to_string(seq_max);
	//pad on left with 0 until 8 characters long
	while (pack_size_string.length() != 8 || windowSizeString.length() != 8 || seqNumString.length() != 8) {
		if (pack_size_string.length() != 8){pack_size_string = "0" + pack_size_string;}
		if (windowSizeString.length() != 8) {windowSizeString = "0" + windowSizeString;}
		if (seqNumString.length() != 8) {seqNumString = "0" + seqNumString;}
	}
	char * pack_size_char_arr = new char[pack_size_string.length()+1];
	strcpy(pack_size_char_arr, pack_size_string.c_str());
	char * window_size_char_arr = new char[windowSizeString.length()+1];
	strcpy(window_size_char_arr, windowSizeString.c_str());
	char * seq_num_char_arr = new char[seqNumString.length()+1];
	strcpy(seq_num_char_arr, seqNumString.c_str());
	char * all_info = std::strcat(std::strcat(pack_size_char_arr,window_size_char_arr),seq_num_char_arr);
	//start timer for RTT
	uint64_t startSend = milliNow();
	uint64_t endSend;
	
	//write packet size to server
	write(socketfd, (char *)all_info, 26);
	
	bzero(all_info, 26);
	read(socketfd, all_info, 8);
	if (read>0) {
		endSend = milliNow();
	}
	uint64_t RTT = endSend-startSend;
	if(RTT == 0){
			RTT = 1;
	}
	roundtt = RTT;
	
	if(timeout == 0){
		timeout = (roundtt*1000);
	}
	
	uint64_t startPacketSending = milliNow();
	
	std::thread sPkt([&](){ Client::sendPacket(filename, panel, pack_size);});
	sPkt.join();
	
	uint64_t endPacketSending = milliNow();
	double totalElapsedTime = ((double)endPacketSending-(double)startPacketSending)/1000000;
	
	//free(buffer);
	std::cout<<"\nNumber of original packets sent: "<< endPacketCount <<"\n";
	std::cout<<"Number of retransmitted packets: "<< retransmittedCount <<"\n";
	std::cout<<"Total elapsed time: "<< totalElapsedTime <<" seconds\n";
	std::cout<<"Total throughput (Mbps): "<< ((double)((pSize+13)*endPacketCount)/1000.0)/totalElapsedTime <<"\n";
	std::cout<<"Effective throughput (Mbps): "<< ((double)((pSize)*endPacketCount)/1000.0)/totalElapsedTime <<"\n\n";
}
