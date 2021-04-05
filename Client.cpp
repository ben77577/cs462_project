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
int Client::writeMyPkt(char * writebuffer, int window_size, size_t pSize, Panel *panel, int buf_size) {
	std::lock_guard<std::mutex> window_lock(windowLock);
	int foundEOF = 0;
	for (int i = 0; i < window_size; i++)
		{
			time_t now;
			if ((panel + i)->isEmpty() == 0 && ((panel + i)->isSent() == 0 || (time(&now) - (panel + i)->getTimeSent() > .01)) && !(panel+i)->isLast())
			{
				int id = (panel + i)->getSeqNum();
				writebuffer = (panel + i)->getBuffer();
				send(socketfd, writebuffer, pSize + 8 + 5, 0);
				if (!send)
				{
					std::cout << "Packet #" << id << " failed to send.\n";
				}
				else
				{
					(panel + i)->markAsSent();
					//print the packet if appropriate
					if (print_packets)
					{
						bool dots = true;
						std::cout << std::dec;
						std::cout << "Sent packet #" << id << " - ";
						for (int loop = 0; loop < buf_size + 5; loop++)
						{
							std::cout << writebuffer[loop];
						}
						std::cout << "\n";
					}
					time_t sentTime;
					time(&sentTime);
					(panel + 1)->setTimeSent(sentTime);
					bzero(writebuffer, buf_size);
					i = window_size;
				}
			}else if (i == 0 && (panel+i)->isLast()) {
				//EOF found in first panel, exit function
				foundEOF = 1;
				i=window_size;
			}
		}
	return foundEOF;
}
//thread to find unsent/timedout pkts and resend/send pkts appropriately - TO-DO: THREAD
void Client::writePacket(char * writebuffer, int window_size, size_t pSize, Panel *panel, int buf_size)
{
	int foundEOF = 0;
	//while(!foundEOF) { - WILL OCCUR WHEN THREADED
		foundEOF = writeMyPkt(writebuffer, window_size, pSize, panel, buf_size);
	//}
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
			//shift panel back one
			//TO-DO(maybe): make into its own function in PANEL::PANEL
			(panel+shift)->setSeqNum((panel+shift+1)->getSeqNum());
			(panel+shift)->fillBuffer((panel+shift+1)->getBuffer());
			(panel+shift)->setPktSize((panel+shift+1)->getPktSize());
			(panel+shift)->setTimeSent((panel+shift+1)->getTimeSent());
			if ((panel+shift+1)->isSent()){
				(panel+shift)->markAsSent();
			}
			if ((panel+shift+1)->isReceived()){
				(panel+shift)->markAsReceived();
			}
			shift++;
		}
		(panel + shift)->setAsEmpty();
		//reset loop variable if necessary
		if ((panel)->isReceived()){
			expectedReceived = 1;
		}
	}
}
//thread to read socket and mark panels as ack'd -> TO-DO: THREAD
void Client::readAck(char * readBuffer, int window_size, Panel *panel){
		read(socketfd, readBuffer, 8);
		if (read < 0){
			std::cout << "Failure to catch ACK \n";
		}
		else{
			std::lock_guard<std::mutex> window_lock(windowLock);
			std::string ack = std::string(readBuffer);
			std::cout << "Ack #" << ack << " received \n";
			int ackComp = std::stoi(ack);
			for (int i = 0; i < window_size; i++){
				if ((panel + i)->getSeqNum() == ackComp){
					(panel+i)->markAsReceived();
					if (i == 0){
						handleExpected(panel, window_size);
					}
				}
			}
		}
	bzero(readBuffer, sizeof(readBuffer));
}
//send packets to server
void Client::sendPacket(const char *filename, int pSize, int cSize, int window_size, int sequence_max, char * buffer, Panel *panel, int buf_size){
	int sn = sequence_max;
	int result;
	//open file for reading
	FILE *openedFile = fopen(filename, "r+");
	if (openedFile == NULL)
	{
		std::cout << "Error opening file to read. (Do you have r+ permissions for this file?)\n";
		exit(2);
	}

	int currentPanel = 0;
	int packet_counter = 0;
	//start ACK thread
	//read information from file, pSize chars at a time
	while ((result = fread(buffer, cSize, pSize, openedFile)) > 0)
	{
		//CRC code
		Checksum csum;
		std::string crc = csum.calculateCRC(std::string(buffer));
		//construct id
		std::string id = std::to_string(packet_counter);
		std::string leftPad = "00000";
		id = leftPad.substr(0, 5 - id.length()) + id;
		//write the packet to the server (ADD 8+5 TO RESULT IF ADDING CRC+ID) (and use this to add crc to end of buffer std::strcat(buffer, crc.c_str()))
		std::strcat(std::strcat(buffer, id.c_str()), crc.c_str());
		int emptyNotFound = 1;
		//continue to loop until a panel is empty
		while (emptyNotFound)
		{
			for (int i = 0; i < window_size; i++)
			{
				std::lock_guard<std::mutex> window_lock(windowLock);
				if ((panel + i)->isEmpty())
				{
					(panel + i)->fillBuffer(buffer);
					(panel + i)->setSeqNum(packet_counter);
					(panel + i)->setAsOccupied();
					(panel + i)->setPktSize(result);
					//exit while
					emptyNotFound = 0;
					//exit for
					i = window_size;
				}
			}
		}
		//call write to loop through window and check for pkts to send - TO-DO:  std::thread wPkt([&](){ Client::writeAck(buffer, window_size, panel);});
		writePacket(buffer, window_size, pSize, panel, buf_size);
		//std::cout << "Return from write\n";
		//call to read socket for acks and change appropriate panel in window - TO-DO: std::thread rPkt([&](){ Client::readAck(buffer, window_size, panel);});
		readAck(buffer, window_size, panel);
		if (packet_counter == sequence_max){
			packet_counter = 0;
		}else{
			packet_counter++;
		}
		if (currentPanel == window_size){
			currentPanel = 0;
		}else{
			currentPanel++;
		}
		bzero(buffer, buf_size);
	}
	//continue to loop until a panel is empty
	int emptyNotFound = 1;
		while (emptyNotFound){
			for (int i = 0; i < window_size; i++){
				std::lock_guard<std::mutex> window_lock(windowLock);
				if ((panel + i)->isEmpty()){
					(panel+i)->setAsOccupied();
					(panel+i)->setAsLast();
					//exit while
					emptyNotFound = 0;
					//exit for
					i = window_size;
				}
			}
		}
	//TO-DO:remove - for testing purposes
	writePacket(buffer, window_size, pSize, panel, buf_size);
	close(socketfd);
	fclose(openedFile);
	std::cout << std::dec;
	std::cout << "\nSend Success!\n";
}
void Client::startThreads(const char *filename, int pack_size, int window_size, int sequence_max){
	//initialize array(window)
	Panel panel[window_size];
	for (int i = 0; i < window_size; i++)
	{
		(panel + i)->setSeqNum(i);
	}
	//total size of CRC, sequence numbers, etc...
	int packetInfoSize = 8;
	int idSize = 5;
	//size_t
	int cSize = sizeof(char);
	//size_t
	int pSize = pack_size;
	int result;
	//char * buffer;
	int buf_size = sizeof(char) * (pack_size + packetInfoSize+idSize);
	//set up buffer
	char * buffer =(char*)malloc(buf_size);
	//char buffer[buf_size];
	bzero(buffer, buf_size);

	if (buffer == NULL)
	{
		fputs("Error setting up buffer", stderr);
		exit(2);
	}
	//set up pack_size to send (ADD 8 TO PACK_SIZE IF ADDING CRC)
	std::string pack_size_string = std::to_string(pack_size + packetInfoSize+idSize);
	//pad on left with 0 until 8 characters long
	while (pack_size_string.length() != 8)
	{
		pack_size_string = "0" + pack_size_string;
	}
	char const *pack_size_char_arr = pack_size_string.c_str();
	//write packet size to server
	write(socketfd, (char *)pack_size_char_arr, 8);
	//TO-DO:set as thread
	sendPacket(filename, pSize, cSize, window_size, sequence_max, buffer, panel, buf_size);
	//writePacket(thread);
	//readAck(thread);
	//clean up
	//free(buffer);
	std::cout<<"End of program\n";
}
