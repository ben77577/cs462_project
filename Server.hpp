#include <iostream>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <iomanip>
#include <unistd.h>

#include "Checksum.hpp"

class Server{
	private:
		std::string ip_address;
		std::string port;
		int socketfd;
		int clientfd;
		struct sockaddr_in sockaddr;
		bool print_packets;
	public:
		Server(std::string ip, std::string po, std::string pr_pa);
		int start();
		bool readPackets(int newsockfd, const char* filename);

};