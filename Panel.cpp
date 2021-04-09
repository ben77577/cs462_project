#include <iostream>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <iomanip>
#include <unistd.h>
#include <chrono>
#include "Panel.hpp"
#include <mutex>
//constructor
Panel::Panel(){
    seqNum = -1;
    sentPkt = 0;
    receivedAck = 0;
    empty = 1;
    fileDone = 0;
    timeSent = 0; //epoch now-timeSent>=timeout -> resend pkt -> timeSent++;
    numberOfSent = 0;
    pktSize = 0;  
	fail = 0;
};
//returns seqNum
int Panel::getSeqNum() {
    return seqNum;
    };
void Panel::setSeqNum(int givenSeqNum) {
    seqNum = givenSeqNum;
    };
//returns if sent
int Panel::isSent() {
    return sentPkt;
    };
//returns if ACK'd
int Panel::isReceived() {
    return receivedAck;
    };
// mark pkt as sent to server
void Panel::markAsSent() {
    sentPkt=1;
};
// marks pkt as ACK'd
void Panel::markAsReceived() {
    if(receivedAck == 0) {
        receivedAck=1;
    }
    //releasePkt();
};
char * Panel::getBuffer(){
    return panelBuffer;
}
void Panel::fillBuffer(char * givenBuffer, int bufferSize) {
    memcpy(panelBuffer, givenBuffer, bufferSize);
};
void Panel::setAsEmpty(){
    bzero(panelBuffer, sizeof(panelBufferSize));
    seqNum = -1;
    sentPkt = 0;
    receivedAck = 0;
    empty = 1;
    timeSent = 0;
    pktSize = 0;
	fail = 0;
}
int Panel::isEmpty(){
    return empty;
}
void Panel::setAsOccupied(){
    empty=0;
}
int Panel::tryLockPkt(){
    return 0;
}
void Panel::releasePkt(){
}
time_t Panel::getTimeSent() {
    return timeSent;
}
void Panel::setTimeSent(time_t currentTime) {
    timeSent = currentTime;
}
void Panel::setPktSize(int givenSize) {
    pktSize = pktSize;
}
int Panel::getPktSize() {
    return pktSize;
}
void Panel::setAsLast() {
    fileDone = 1;
}
int Panel::isLast() {
    return fileDone;
}
void Panel::setFail(int failStatus){
	fail = failStatus;
}
int Panel::getFail(){
	return fail;
}
void Panel::summary() {
    std::cout<<"Summary for panel " <<seqNum<<"\n";
    std::cout<<"seqNum"<<seqNum
        <<"sentPkt = "<<sentPkt<<"\n"
        <<"receivedAck = "<<receivedAck
        <<"timeSent = "<<timeSent<<"\n"
        <<"numberOfSent = "<<numberOfSent
        <<"empty = "<<empty
        <<"pktSiz = "<<pktSize<<"\n"
        <<"fileDone = "<<fileDone<<"\n\n";
        if(empty!=1) {
            std::cout<<panelBuffer<<"\n";
        }
}
void Panel::allocateBuffer(int buffer_size) {
    panelBufferSize = buffer_size;
    panelBuffer =(char*)malloc(buffer_size);
	//char buffer[buf_size];
	bzero(panelBuffer, panelBufferSize);

	if (panelBuffer == NULL)
	{
		fputs("Error setting up buffer", stderr);
		exit(2);
	}
}