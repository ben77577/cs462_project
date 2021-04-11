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



ErrorCreate promptErrors(std::string client_server){
	
	std::string in_er;
	std::cout << "Introduce Errors? (y/n): ";
	std::cin >> in_er;

	if(in_er == "n"){
		ErrorCreate errorObj(std::string("none"));
		return errorObj;
	}
	
	if(client_server == "client"){
		ErrorCreate errorObj(std::string("client"));
		
		std::string drop_packets_string;
		std::cout << "Drop packets? (\"no\" or comma-serperated list): ";
		std::cin >> drop_packets_string;
		errorObj.setDropPackets(drop_packets_string);
		
		std::string damage_packets_string;
		std::cout << "Damage packets? (\"no\" or comma-serperated list): ";
		std::cin >> damage_packets_string;
		errorObj.setDamagePackets(damage_packets_string);
		
		return errorObj;
	}
	else if(client_server == "server"){
		ErrorCreate errorObj(std::string("server"));
		
		std::string lost_ack_string;
		std::cout << "Lose Acks? (\"no\" or comma-serperated list): ";
		std::cin >> lost_ack_string;
		errorObj.setLoseAcks(lost_ack_string);
		return errorObj;
	}
	ErrorCreate errorObj(std::string("none"));
	return errorObj;
}


//main
int main(){
	std::string client_server;
    std::string ip_address; 
	std::string port_number;
	std::string filename;
	std::string print_packets;
	int packet_size;
	int windowSize;
	int sequence_max;
	
	//get general information from user
	std::cout << "client or server? (Case sensitive) ";
	std::cin >> client_server;
	std::cout << "print detailed info to console? (y/n) ";
	std::cin >> print_packets;
	
	
	//server
	if(client_server.compare("server") == 0){
		std::cout << "Creating server..\n";
		//get server-specific information from user
		std::cout << "Port # ";
		std::cin >> port_number;
		
		ErrorCreate errorObj = promptErrors("server");
		
		std::cout << "Save file to: ";
		std::cin.ignore();
		getline(std::cin, filename);

		
		//create Server object
		Server server(ip_address, port_number, print_packets, &errorObj);
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
		std::string errors;
		std::string protocolType;
		uint64_t timeout = 0;
		
		std::cout<<"Creating client...\n";
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
		std::cin >> windowSize;
		std::cout << "Sequence number range: (<=Window size) ";
		std::cin >> sequence_max;
		if(sequence_max<=windowSize) {
			std::cout<< "Invalid sequence number\n";
			exit(0);
		}
		std::cout << "Type of protocol: (sr or gbn) ";
		std::cin >> protocolType;
		std::cout << "Timeout (milliseconds): (enter 0 for ping-calculated) ";
		std::cin >> timeout;
		
		ErrorCreate errorObj = promptErrors("client");
	
	
		//create Client object
		Client client(ip_address, port_number, print_packets, protocolType, timeout, &errorObj);
		//start the client
		client.start();
		//send packets to the server
		client.startThreads(filename.c_str(), packet_size, windowSize, sequence_max);
		
		//call md5sum command on input file
		std::cout << "MD5:\n";
		std::string md5command = "md5sum ";
		md5command += filename;
		system(&md5command[0]);
	}else{
		std::cout<<"Neither client/server selected. Exiting";
	}
}	