#include <iostream>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <iomanip>
#include <unistd.h>

class Client{
	private:
		std::string ip_address;
		std::string port;
		int socketfd;
		struct sockaddr_in sockaddr;
		bool print_packets;
	public:
		Client(std::string ip, std::string po, std::string pr_pa);
		void start();
		void sendPacket(const char* filename, int pack_size);
};