#ifndef _ErrorCreate_H
#define _ErrorCreate_H

#include <vector>

class ErrorCreate{
	private:
		std::string client_server;
		std::vector<std::string> errorList;
		void parseString(std::string string_to_parse, std::string errorType);

	public:
		ErrorCreate(std::string cli_ser);
		std::string getPacketError(int packet_num);
		void setDropPackets(std::string drop_packets_string);
		void setDamagePackets(std::string damage_packets_string);
		void setLoseAcks(std::string lost_acks_string);
};

#endif