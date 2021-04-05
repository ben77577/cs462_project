#include <iostream>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <iomanip>
#include <unistd.h>

#include "Client.hpp"
#include "Server.hpp"


//main
int main(){
	std::string client_server;
    std::string ip_address; 
	std::string port_number;
	std::string filename;
	std::string print_packets;
	int packet_size;
	int window_size;
	int sequence_max;
	
	//get general information from user
	std::cout << "client or server? (Case sensitive) ";
	std::cin >> client_server;
	std::cout << "print packets to console? (y/n) ";
	std::cin >> print_packets;
	
	
	//server
	if(client_server.compare("server") == 0){
		std::cout << "Creating server..";
		//get server-specific information from user
		std::cout << "Port # ";
		std::cin >> port_number;
		std::cout << "Save file to: ";
		std::cin.ignore();
		getline(std::cin, filename);

		
		//create Server object
		Server server(ip_address, port_number, print_packets);
		//start the server (return connected client sockFd)
		int clientsockfd = server.start();
		//read packets from client
		bool md5bool = server.readPackets(clientsockfd, filename.c_str());
		
		//if appropriate, call md5sum command on output file
		if(!md5bool){
			std::cout << "MD5:\n";
			std::string md5command = "md5sum ";
			md5command += filename;
			system(&md5command[0]);
		}
	}
	//client
	else if(client_server.compare("client") == 0){
		std::cout<<"Creating client...";
		//get client-specific information from user
		std::cout << "Connect to IP address: ";
		std::cin >> ip_address;
		std::cout << "Port # ";
		std::cin >> port_number;
		std::cout << "File to be sent: ";
		std::cin >> filename;
		std::cout << "Pkt size: ";
		std::cin >> packet_size;
		std::cout << "Window size: ";
		std::cin >> window_size;
		std::cout << "Sequence number range: ";
		std:: cin >> sequence_max;
		
		//create Client object
		Client client(ip_address, port_number, print_packets);
		//start the client
		client.start();
		//send packets to the server
		client.startThreads(filename.c_str(), packet_size, window_size, sequence_max);
		
		//call md5sum command on input file
		std::cout << "MD5:\n";
		std::string md5command = "md5sum ";
		md5command += filename;
		system(&md5command[0]);
	}else{
		std::cout<<"Neither client/server selected. Exiting";
	}
}	