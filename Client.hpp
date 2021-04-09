#include <iostream>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <iomanip>
#include <unistd.h>

#include "Checksum.hpp"
#include "ErrorCreate.hpp"
#include "Panel.hpp"


class Client{
	private:
		std::string ip_address;
		std::string port;
		int socketfd;
		struct sockaddr_in sockaddr;
		bool print_packets;
		int window_size;
		int pSize;
		int cSize;
		int buf_size;
		int seq_max;
		std::mutex windowLock;
		int numbPcktsExpected;
		ErrorCreate *errorObj;
		bool foundEndFile;

	public:
		Client(std::string ip, std::string po, std::string pr_pa, ErrorCreate* er);
		void start();
		void sendPacket(const char *filename,Panel *panel, int pack_size);
		void writePacket(Panel *panel);
		void readAck(Panel *panel);
		const int getWindowSize(int windowSize);
		void handleExpected(Panel *panel, int window_size);
		void startThreads(const char *filename, int pack_size, int windowSize, int sequence_max);
		int writeMyPkt(Panel *panel);
		int findAndFillBuffer(Panel *panel, char *buffer, int packet_counter, int packet_number, int result);
		int findAndFillEOF(Panel *panel);
		uint64_t milliNow();
};