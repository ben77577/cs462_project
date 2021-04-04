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
Panel::Panel(int sn){
    seqNum = sn;
    sentPkt = 0;
    receivedAck = 0;
    empty = 1;
    editFlag = false;
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
    if(sentPkt = 0) {
        sentPkt=1;
    }
};
// marks pkt as ACK'd
void Panel::markAsReceived() {
    tryLockPkt();
    if(receivedAck = 0) {
        receivedAck=1;
    }
    releasePkt();
};
char * Panel::getBuffer(){
    return buffer;
}
void Panel::fillBuffer(char * givenBuffer) {
    buffer = givenBuffer;
};
void Panel::setAsEmpty(){
    bzero(buffer, sizeof(buffer));
    seqNum = -1;
    sentPkt = 0;
    receivedAck = 0;
    empty = 1;
    timeSent = 0;
    pktSize = 0;
}
int Panel::isEmpty(){
    return empty;
}
void Panel::setAsOccupied(){
    empty=0;
}
int Panel::tryLockPkt(){
    std::cout<<"Not available: " <<editFlag<<"\n";
    while(editFlag==1){
        
    }
    editFlag = 1;
    return editFlag;
}
int Panel::getEditFlag() {
    return editFlag;
}
void Panel::releasePkt(){
    editFlag = 0;
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