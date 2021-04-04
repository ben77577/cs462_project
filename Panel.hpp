#include <string>
#include <mutex>
#include <ctime>
using namespace std;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds milliseconds;

class Panel{
    private:
		int seqNum;
        int sentPkt;
        int receivedAck;
        time_t timeSent; //epoch now-timeSent>=timeout -> resend pkt -> timeSent++;
        int numberOfSent;
        int empty;
        char * buffer;
        int pktSize;
        int fileDone;
        //locks Panel for edits
        std::mutex mtxPktLock;
	public:
        Panel(int sn);
		int getSeqNum();
        void setSeqNum(int givenSeqNum);
        int isSent();
        int isReceived();
        void markAsSent();
        void markAsReceived();
        void fillBuffer(char * buffer);
        char* getBuffer();
        void lockPkt();
        void releasePkt();
        int isEmpty();
        void setAsEmpty();
        void setAsOccupied();
        time_t getTimeSent();
        void setTimeSent(time_t currentTime);
        void setPktSize(int givenSize);
        int getPktSize();
        int isLast();
        void setAsLast();
};