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
#include <chrono>

//constructor
Client::Client(std::string ip, std::string po, std::string pr_pa)
{
	ip_address = ip;
	port = po;
	//set print_packets appropriatly
	if (pr_pa.compare("y") == 0)
	{
		print_packets = true;
	}
	else
	{
		print_packets = false;
	}
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
	//char buffer[buf_size];
	bzero(writebuffer, buf_size);

	if (writebuffer == NULL)
	{
		fputs("Error setting up buffer", stderr);
		exit(2);
	}
	int foundEOF = 0;
	std::lock_guard<std::mutex> window_lock(windowLock);
	std::cout<<"writePacket: has panel locked\n";
	for (int writeLoop = 0; writeLoop < window_size; writeLoop++){
			time_t now;	
			if ((panel)->isLast()) {
				//EOF found in first panel, exit function
				foundEOF = 1;
				return foundEOF;
			}		
			if ((panel + writeLoop)->isEmpty() == 0 && ((panel + writeLoop)->isSent() == 0 || 
												(time(&now) - (panel + writeLoop)->getTimeSent() > 500)) && !(panel+writeLoop)->isLast()){
				std::cout<<"writePacket: Panel to be sent found - id: " << (panel+writeLoop)->getSeqNum()<<"\n";
				int writeID = (panel + writeLoop)->getSeqNum();
				writebuffer = (panel + writeLoop)->getBuffer();
				std::cout<<"writePacket: buffer found to be sent: " << writebuffer<<" at i val: " << writeLoop << "\n";
				write(socketfd, writebuffer, pSize + 8 + 5);
				if (!send)
				{
					std::cout << "writePacket: Packet #" << writeID << " failed to send.\n";
				}
				else
				{
					(panel + writeLoop)->markAsSent();
					//print the packet if appropriate
					if (print_packets)
					{
						bool dots = true;
						std::cout << std::dec;
						std::cout << "writePacket: Sent packet #" << writeID << " - ";
						for (int loop = 0; loop < buf_size + 5; loop++)
						{
							std::cout << writebuffer[loop];
						}
						std::cout << "\n";
					}
					time_t sentTime;
					time(&sentTime);
					(panel + writeLoop)->setTimeSent(sentTime);
					bzero(writebuffer, buf_size);
					writeLoop = window_size;
				}
			}
		}
		std::cout<<"writePacket: writeMyPkt should be losing guard\n";
	return foundEOF;
}
//thread to find unsent/timedout pkts and resend/send pkts appropriately - TO-DO: THREAD
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
	while (expectedReceived){
		expectedReceived = 0;
		shift = 0;
		//loop until found the end of window or next panel is empty
		while (shift < window_size - 2 && !(panel + shift + 1)->isEmpty()){
			std::cout<<"readAck: Shifting panels at indeces " << shift << " and " << shift+1<< "\n";
			//shift panel back one
			//TO-DO(maybe): make into its own function in PANEL::PANEL
			(panel+shift)->setSeqNum((panel+shift+1)->getSeqNum());
			(panel+shift)->fillBuffer((panel+shift+1)->getBuffer(), buf_size);
			(panel+shift)->setPktSize((panel+shift+1)->getPktSize());
			(panel+shift)->setTimeSent((panel+shift+1)->getTimeSent());
			if ((panel+shift+1)->isLast()) {
				(panel+shift)->setAsLast();
			}
			if ((panel+shift+1)->isSent()){
				(panel+shift)->markAsSent();
			}
			if ((panel+shift+1)->isReceived()){
				(panel+shift)->markAsReceived();
			}
			shift++;
		}
		while(shift<window_size) {
			(panel + shift)->setAsEmpty();
			shift++;
		}
		//reset loop variable if necessary
		if ((panel)->isReceived()){
			expectedReceived = 1;
		}
	}
}
//thread to read socket and mark panels as ack'd -> TO-DO: THREAD
void Client::readAck(Panel *panel){
	char * readBuffer =(char*)malloc(buf_size);
	//char buffer[buf_size];
	bzero(readBuffer, buf_size);

	if (readBuffer == NULL)
	{
		fputs("Error setting up buffer", stderr);
		exit(2);
	}
	while (!(panel)->isLast()) {	
		read(socketfd, readBuffer, 8);
		if (read < 0){
			std::cout << "readAck: Failure to catch ACK \n";
		}
		else{
			std::string ack = std::string(readBuffer);
			std::cout << "readAck: Ack #" << ack << " received \n";
			int ackComp = std::stoi(ack);
			std::lock_guard<std::mutex> window_lock(windowLock);
			std::cout<<"readAck: has panel locked\n";
			for (int i = 0; i < window_size; i++){
				if ((panel + i)->getSeqNum() == ackComp){
					std::cout<<"readAck: Ack'ing panel " << (panel+i)->getSeqNum()<<"at value i: " << i << "\n";
					(panel+i)->markAsReceived();
					if (i == 0){
						handleExpected(panel, window_size);
					}
				}
			}
		}
	bzero(readBuffer, sizeof(readBuffer));
	}
}
//find and populate empty panel
int Client::findAndFillBuffer(Panel *panel, char *buffer, int packet_counter, int result) {
	int foundEmpty = 0;
	std::lock_guard<std::mutex> window_lock(windowLock);
	std::cout<<"sendpacket: has panel locked\n";
			for (int i = 0; i < window_size; i++){
				if ((panel + i)->isEmpty()){
					std::cout<<"sendPacket: buffer to be filled: " << buffer << "\n";
					(panel + i)->fillBuffer(buffer, buf_size);
					std::cout<<"sendPacket: setting sequence number of " << packet_counter<<"\n";
					(panel + i)->setSeqNum(packet_counter);
					(panel + i)->setAsOccupied();
					(panel + i)->setPktSize(result);
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
	std::cout<<"findAndFillEOF\n";
	int foundEmpty = 0;
	std::lock_guard<std::mutex> window_lock(windowLock);
	std::cout<<"starting loop\n";
	for (int i = 0; i < window_size; i++){
				if ((panel + i)->isEmpty()){
					std::cout<<"sendPacket: Panel found - filling with EOF\n";
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
	//call write to loop through window and check for pkts to send
	std::thread wPkt([&](){ Client::writePacket(panel);});
	//call to read socket for acks and change appropriate panel in window - TO-DO: std::thread rPkt([&](){ Client::readAck(buffer, window_size, panel);});
	std::thread rPkt([&](){ Client::readAck(panel);});
	
	std::cout<<"sendpacket: past threads\n";
	char * fillPacket = (char *)malloc(buf_size);
	bzero(fillPacket, buf_size);
	int sn = seq_max;
	int result;
	//open file for reading
	FILE *tempFile = fopen(filename, "r+");
	if (tempFile == NULL)
	{
		std::cout << "sendpacket: Error opening file to read. (Do you have r+ permissions for this file?)\n";
		exit(2);
	}

	int currentPanel = 0;
	int packet_counter = 0;
	//start ACK thread
	//read information from file, pSize chars at a time
	fseek(tempFile, 0, SEEK_END);
	int fileSize = ftell(tempFile);
	fclose(tempFile);
	FILE *openedFile = fopen(filename, "r+");
	if (openedFile == NULL)
	{
		std::cout << "sendpacket: Error opening file to read. (Do you have r+ permissions for this file?)\n";
		exit(2);
	}
	numbPcktsExpected = fileSize/pack_size;
	int currentPacket = 0;
	int foundEOF = 0;
	bool lastPacket = 0;
	while ((result = fread(fillPacket, cSize, pSize, openedFile)) > 0)
	{	
		//CRC code
		Checksum csum;
		std::cout<<"Buffer to be crc'd: " << fillPacket << "\n";
		/**if(numbPcktsExpected == (currentPacket)){
			fillPacket = const_cast<char*>((std::string(fillPacket).substr(0,result-1)).c_str());
			if (strlen(fillPacket)==0) {
				std::cout<<"Last packet is empty, break\n";
				break;
			}
			lastPacket = 1;
		}**/
		std::string crc = csum.calculateCRC(std::string(fillPacket));
		std::string leftPad = "00000000";
		crc = leftPad.substr(0, 8 - crc.length()) + crc;
		//construct id
		std::string id = std::to_string(packet_counter);
		leftPad = "00000";
		id = leftPad.substr(0, 5 - id.length()) + id;
		//write the packet to the server (ADD 8+5 TO RESULT IF ADDING CRC+ID) (and use this to add crc to end of fillPacket std::strcat(fillPacket, crc.c_str()))
		std::strcat(std::strcat(fillPacket, id.c_str()), crc.c_str());
		std::cout<<"fillPacket after stringify: " << fillPacket << "\n";
		int emptyNotFound = 0;
		//find an empty panel
		//if found, then populate pkt with appropriate information
		//if none, then pause and wait till there is one
		while (!emptyNotFound){
			emptyNotFound = findAndFillBuffer(panel, fillPacket, packet_counter, result);
		}
		std::cout<<"sendpacket: Made it out of emptyNotFound\n";
		std::cout<<"sendpacket: packet_counter " << packet_counter<<"\n";
		std::cout<<"sendpacket: seq_max" << seq_max <<"\n";
		if (packet_counter == seq_max){
			packet_counter = 0;
		}else{
			packet_counter++;
		}
		currentPacket++;
		bzero(fillPacket, buf_size);
		if (lastPacket) {
			break;
		}
	}
	int emptyNotFound = 0;
	std::cout<<"Attempting to add EOF packet\n";
	while (!emptyNotFound){
		emptyNotFound = findAndFillEOF(panel);
	}
	wPkt.join();
	rPkt.join();
	close(socketfd);
	fclose(openedFile);
	std::cout << std::dec;
	std::cout << "\nSend Success!\n";
}
//calculates the current epoch in milliseconds
uint64_t Client::milliNow() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}
//Initializes global variables & begins sendPacket thread
void Client::startThreads(const char *filename, int pack_size, int windowSize, int sequence_max){
	//initialize array(window)
	Panel panel[windowSize];
	//total size of CRC, sequence numbers, etc...
	int packetInfoSize = 8;
	int idSize = 5;
	//set global variables
	seq_max = sequence_max;
	window_size = windowSize;
	//size_t
	cSize = sizeof(char);
	//size_t
	pSize = pack_size;
	//char * buffer;
	buf_size = sizeof(char) * (pack_size + packetInfoSize+idSize);
	//set up buffer
	char * buffer =(char*)malloc(buf_size);
	//char buffer[buf_size];
	bzero(buffer, buf_size);

	if (buffer == NULL)
	{
		fputs("Error setting up buffer", stderr);
		exit(2);
	}
	for (int initLoop = 0; initLoop<windowSize; initLoop++) {
		std::cout<<"buffer size given to panel: " <<  buf_size << "\n";
		(panel+initLoop)->allocateBuffer(buf_size);
	}
	//set up pack_size to send (ADD 8 TO PACK_SIZE IF ADDING CRC)
	std::string pack_size_string = std::to_string(pack_size + packetInfoSize+idSize);
	//pad on left with 0 until 8 characters long
	while (pack_size_string.length() != 8)
	{
		pack_size_string = "0" + pack_size_string;
	}
	char const *pack_size_char_arr = pack_size_string.c_str();
	//start timer for RTT
	uint64_t startSend = milliNow();
	uint64_t endSend;
	//write packet size to server
	write(socketfd, (char *)pack_size_char_arr, 8);
	bzero(buffer, buf_size);
	read(socketfd, buffer, 8);
	if (read>0) {
		endSend = milliNow();
	}
	uint64_t RTT = startSend-endSend;
	std::cout<< startSend << "-"<<endSend << "=" << RTT<<"\n";
	//TO-DO:set as thread
	//sendPacket(filename, pSize, cSize, window_size, sequence_max, buffer, panel, buf_size);
	std::thread sPkt([&](){ Client::sendPacket(filename, panel, pack_size);});
	sPkt.join();
	//writePacket(thread);
	//readAck(thread);
	//clean up
	//free(buffer);
	std::cout<<"End of program\n";
}
