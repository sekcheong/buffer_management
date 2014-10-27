#include <memory.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include "page.h"
#include "buf.h"

#define ASSERT(c)  { if (!(c)) { \
		       cerr << "At line " << __LINE__ << ":" << endl << "  "; \
                       cerr << "This condition should hold: " #c << endl; \
                       exit(1); \
		     } \
                   }

//----------------------------------------
// Constructor of the class BufMgr
//----------------------------------------

BufMgr::BufMgr(const int bufs)
{
    numBufs = bufs;

    bufTable = new BufDesc[bufs];
    memset(bufTable, 0, bufs * sizeof(BufDesc));
    for (int i = 0; i < bufs; i++) 
    {
        bufTable[i].frameNo = i;
        bufTable[i].valid = false;
    }

    bufPool = new Page[bufs];
    memset(bufPool, 0, bufs * sizeof(Page));

    int htsize = ((((int) (bufs * 1.2))*2)/2)+1;
    hashTable = new BufHashTbl (htsize);  // allocate the buffer hash table

    clockHand = bufs - 1;
}


BufMgr::~BufMgr() {
	// TODO: Implement this method by looking at the description in the writeup.
}


const Status BufMgr::allocBuf(int & frame) {
	// TODO: Implement this method by looking at the description in the writeup.
	return OK;
}

	
const Status BufMgr::readPage(File* file, const int PageNo, Page*& page) {
	// TODO: Implement this method by looking at the description in the writeup.
	return OK;
}


const Status BufMgr::unPinPage(File* file, const int PageNo, 
			       const bool dirty) {
	// TODO: Implement this method by looking at the description in the writeup.
	return OK;
}


const Status BufMgr::allocPage(File* file, int& pageNo, Page*& page)  {
	// TODO: Implement this method by looking at the description in the writeup.
	return OK;
}


const Status BufMgr::disposePage(File* file, const int pageNo) {
	// TODO: Implement this method by looking at the description in the writeup.
	return OK;
}


const Status BufMgr::flushFile(const File* file) {
	// TODO: Implement this method by looking at the description in the writeup.
	return OK;
}


void BufMgr::printSelf(void) 
{
    BufDesc* tmpbuf;
  
    cout << endl << "Print buffer...\n";
    for (int i=0; i<numBufs; i++) {
        tmpbuf = &(bufTable[i]);
        cout << i << "\t" << (char*)(&bufPool[i]) 
             << "\tpinCnt: " << tmpbuf->pinCnt;
    
        if (tmpbuf->valid == true)
            cout << "\tvalid\n";
        cout << endl;
    };
}


