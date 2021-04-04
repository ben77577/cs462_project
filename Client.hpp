#include <iostream>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <iomanip>
#include <unistd.h>
#include "Checksum.hpp"
#include "Panel.hpp"


class Client{
	private:
		std::string ip_address;
		std::string port;
		int socketfd;
		struct sockaddr_in sockaddr;
		bool print_packets;
		int window_size;

	public:
		Client(std::string ip, std::string po, std::string pr_pa);
		void start();
		void sendPacket(const char* filename, int pack_size, int ws, int sm);
		void writePacket(char * buffer, int window_size, size_t pSize, Panel *panel, int buf_size);
		void readAck(char * buffer, int window_size, Panel *panel);
		const int getWindowSize(int windowSize);
		void handleExpected(Panel *panel, int window_size);
};