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
//thread to find unsent/timedout pkts and resend/send pkts appropriately - TO-DO: THREAD
void Client::writePacket(char * writebuffer, int window_size, size_t pSize, Panel *panel, int buf_size)
{
	time_t now;
	int foundEOF = 0;
	//while(!foundEOF) {
		for (int i = 0; i < window_size; i++)
		{
			std::cout << "Panel" << i << "\n";
			std::cout << (panel + i)->isEmpty() << " " << (panel + i)->isSent() << " " << time(&now) - (panel + i)->getTimeSent() << "\n";
			if ((panel + i)->isEmpty() == 0 && ((panel + i)->isSent() == 0 || (time(&now) - (panel + i)->getTimeSent() > .01)) && !(panel+i)->isLast())
			{
				std::cout<<"Attempting to lock panel #"<<i<<"\n";
				(panel+i)->lockPkt();
				int id = (panel + i)->getSeqNum();
				writebuffer = (panel + i)->getBuffer();
				send(socketfd, writebuffer, pSize + 8 + 5, 0);
				if (!send)
				{
					std::cout << "Packet #" << id << " failed to send.\n";
				}
				else
				{
					i = window_size;
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
					std::cout << "outside print\n";
					time_t sentTime;
					time(&sentTime);
					std::cout << sentTime << "\n";
					(panel + 1)->setTimeSent(sentTime);
					bzero(writebuffer, buf_size);
				}
				(panel+i)->releasePkt();
			}else if (i == 0 && (panel+i)->isLast()) {
				//EOF found in first panel, exit function
				std::cout<<"EOF found\n";
				foundEOF = 1;
				i=window_size;
			}
		}
	//}
}
//handles shifting of window when expected pkt is ack'd - NOT A THREAD
void Client::handleExpected(Panel *panel, int window_size){
	//lock all panels - blocking any new change from threads
	int shift = 0;
	//TO-DO: lock()/unlock() throwing seg faults
	/**for (shift = 0; shift < window_size; shift++)
	{
		(panel + shift)->lockPkt();
	}**/
	//loop until the first in window is no longer ack'd
	int expectedReceived = 1;
	while (expectedReceived)
	{
		expectedReceived = 0;
		shift = 0;
		//loop until found the end of window or next panel is empty
		while (!(panel + shift + 1)->isEmpty() && shift < window_size - 1)
		{
			//shift panel back one
			//TO-DO: make into its own function in PANEL::PANEL
			(panel+shift)->setSeqNum((panel+shift+1)->getSeqNum());
			(panel+shift)->fillBuffer((panel+shift+1)->getBuffer());
			(panel+shift)->setPktSize((panel+shift+1)->getPktSize());
			(panel+shift)->setTimeSent((panel+shift+1)->getTimeSent());
			if ((panel+shift+1)->isSent())
			{
				(panel+shift)->markAsSent();
			}
			if ((panel+shift+1)->isReceived())
			{
				(panel+shift)->markAsReceived();
			}
			shift++;
		}
		(panel + shift)->setAsEmpty();
		//reset loop variable if necessary
		if ((panel)->isReceived())
		{
			expectedReceived = 1;
		}
	}
	//unlock all panels - allowing threads to edit
	/**for (shift = 0; shift < window_size; shift++)
	//{
	//	(panel + shift)->releasePkt();
	}**/
}
//thread to read socket and mark panels as ack'd -> TO-DO: THREAD
void Client::readAck(char * readBuffer, int window_size, Panel *panel){
	read(socketfd, readBuffer, 8);
	//while(read(socketfd, buffer, 8)){ -- structure of eventual thread
	if (read < 0){
		std::cout << "Failure to catch ACK \n";
	}
	else{
		std::string ack = std::string(readBuffer);
		std::cout << "Ack #" << ack << " received \n";
		int ackComp = std::stoi(ack);
		std::cout << "ackComp" << ackComp;
		for (int i = 0; i < window_size; i++){
			std::cout << (panel + i)->getSeqNum() << "=?" << ackComp;
			if ((panel + i)->getSeqNum() == ackComp){
				std::cout << "Found the right panel " << ackComp << "\n";
				(panel + i)->markAsReceived();
				if (i == 0){
					std::cout<<"handling expected\n";
					handleExpected(panel, window_size);
				}
			}
		}
	}
	bzero(readBuffer, sizeof(readBuffer));
	//}
}
//send packets to server
void Client::sendPacket(const char *filename, int pack_size, int window_size, int sequence_max)
{
	int sn = sequence_max;
	//initialize array(window)
	Panel *panel = new Panel(window_size);
	for (int i = 0; i < window_size; i++)
	{
		(panel + i)->setSeqNum(i);
		std::cout << "Panel seqNum " << (panel + i)->getSeqNum() << "\n";
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

	//open file for reading
	FILE *openedFile = fopen(filename, "r+");
	if (openedFile == NULL)
	{
		std::cout << "Error opening file to read. (Do you have r+ permissions for this file?)\n";
		exit(2);
	}

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
	//write packet size
	std::cout<<(char *)pack_size_char_arr<<"\n";
	write(socketfd, (char *)pack_size_char_arr, 8);

	int currentPanel = 0;
	int packet_counter = 0;
	//start ACK thread
	//read information from file, pSize chars at a time
	while ((result = fread(buffer, cSize, pSize, openedFile)) > 0)
	{
		std::cout << "more file read \n";
		//CRC code
		Checksum csum;
		std::string crc = csum.calculateCRC(std::string(buffer));
		//construct id
		std::string id = std::to_string(packet_counter);
		std::string leftPad = "00000";
		id = leftPad.substr(0, 5 - id.length()) + id;
		//write the packet to the server (ADD 8+5 TO RESULT IF ADDING CRC+ID) (and use this to add crc to end of buffer std::strcat(buffer, crc.c_str()))
		std::cout << "buffer" << std::strcat(std::strcat(buffer, id.c_str()), crc.c_str()) << "\n";
		int emptyNotFound = 1;
		std::cout << emptyNotFound << "\n";
		//continue to loop until a panel is empty
		while (emptyNotFound)
		{
			for (int i = 0; i < window_size; i++)
			{
				std::cout << "Panel for loop" << i << "\n";
				std::cout << (panel + i)->isEmpty() << "\n";
				if ((panel + i)->isEmpty())
				{
					(panel+i)->lockPkt();
					std::cout << "Panel is empty: " << i << "\n";
					(panel + i)->fillBuffer(buffer);
					std::cout<<"buffer filled "<<(panel+i)->getBuffer()<<"\n";
					(panel + i)->setSeqNum(packet_counter);
					(panel + i)->setAsOccupied();
					(panel + i)->setPktSize(result);
					//exit while
					emptyNotFound = 0;
					//exit for
					i = window_size;
					(panel+i)->releasePkt();
				}
			}
		}
		std::cout << "Buffer before write" << buffer << "\n";
		//call write to loop through window and check for pkts to send - TO-DO:  std::thread wPkt([&](){ Client::writeAck(buffer, window_size, panel);});
		writePacket(buffer, window_size, pSize, panel, buf_size);
		//std::cout << "Return from write\n";
		std::cout << "Buffer before read" << buffer << "\n";
		//call to read socket for acks and change appropriate panel in window - TO-DO: std::thread rPkt([&](){ Client::readAck(buffer, window_size, panel);});
		readAck(buffer, window_size, panel);
		std::cout << "return from read\n";
		std::cout << "buffer after read" << buffer << "\n";
		if (packet_counter == sequence_max){
			packet_counter = 0;
		}
		else{
			packet_counter++;
		}
		if (currentPanel == window_size){
			currentPanel = 0;
		}
		else{
			currentPanel++;
		}
		bzero(buffer, buf_size);
		std::cout<<"buffer okay before fread?"<<buffer<<"\n";
	}
	/**continue to loop until a panel is empty
	int emptyNotFound = 1;
		while (emptyNotFound){
			for (int i = 0; i < window_size; i++){
				std::cout << "Panel for loop" << i << "\n";
				std::cout << (panel + i)->isEmpty() << "\n";
				if ((panel + i)->isEmpty()){
					std::cout << "Panel is empty: " << i << "\n";
					(panel+i)->setAsOccupied();
					(panel+i)->setAsLast();
					//exit while
					emptyNotFound = 0;
					//exit for
					i = window_size;
				}
			}
		}**/
	//TO-DO: wait for threads to finish
	//clean up
	//free(buffer);
	close(socketfd);
	fclose(openedFile);

	std::cout << std::dec;
	std::cout << "\nSend Success!\n";
}
