#include <iostream>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <iomanip>
#include <unistd.h>
#include <vector>

#include "ErrorCreate.hpp"

//constructor
ErrorCreate::ErrorCreate(std::string cli_ser){
	client_server = cli_ser;
}



//return error for given packet_num, return "na" if no error for that packet
std::string ErrorCreate::getPacketError(int packet_num){
	if(errorList.size() == 0){
		//no errors
		return "na";
	}
	
	int index_num = 0;
	for (std::vector<std::string>::iterator iterate = errorList.begin(); iterate != errorList.end(); ++iterate){
		std::string curError = *iterate;
		if(std::stoi(curError.substr(0, curError.find(","))) == packet_num){
			//remove error from list
			errorList.erase(errorList.begin()+index_num);
			
			//return the error
			return curError.substr(curError.find(",")+1, 2);
		}
		index_num++;
	}
	
	//packet packet_num is not in errorList
	return "na";
}
 
 
 
//parse string to get packet numbers from user-inputted string
void ErrorCreate::parseString(std::string string_to_parse, std::string errorType){
	std::string errorIdentifier;
	if(errorType == "drop"){
		errorIdentifier = ",dr";
	}
	else if(errorType == "damage"){
		errorIdentifier = ",da";
	}
	else if(errorType == "loseAck"){
		errorIdentifier = ",la";
	}
	
	std::string delimiter = ",";
	string_to_parse = string_to_parse + ",";

	size_t pos = 0;
	std::string token;
	while ((pos = string_to_parse.find(delimiter)) != std::string::npos) {
		token = string_to_parse.substr(0, pos);
		errorList.push_back(token + errorIdentifier);
		
		//std::cout << (token + errorIdentifier) << "\n";
		
		string_to_parse.erase(0, pos + delimiter.length());
	}
}
 
 
 
//set drop packets
void ErrorCreate::setDropPackets(std::string drop_packets_string){
	if(drop_packets_string != "no"){
		parseString(drop_packets_string, "drop");
	}
}



//set damage packets
void ErrorCreate::setDamagePackets(std::string damage_packets_string){
	if(damage_packets_string != "no"){
		parseString(damage_packets_string, "damage");
	}
}



//set lose acks
void ErrorCreate::setLoseAcks(std::string lost_acks_string){
	if(lost_acks_string != "no"){
		parseString(lost_acks_string, "loseAck");
	}
}