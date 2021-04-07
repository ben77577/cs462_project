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
        char * panelBuffer;
        int pktSize;
        int fileDone;
        int panelBufferSize;
		int fail;
        //locks Panel for edits
        std::mutex mtxPktLock;
	public:
        Panel();
		int getSeqNum();
        void setSeqNum(int givenSeqNum);
        int isSent();
        int isReceived();
        void markAsSent();
        void markAsReceived();
        void fillBuffer(char * buffer, int bufferSize);
        char* getBuffer();
        int tryLockPkt();
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
        void summary();
        void allocateBuffer(int buffer_size);
		int getFail();
		void setFail(int failStatus);
};