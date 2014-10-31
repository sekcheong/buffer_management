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


const Status BufMgr::allocBuf(int &frame) {
  int pinCnt = 0;
  frame=-1;
  while (true) {
    advanceClock();
    BufDesc bd = bufTable[clockHand];
    if (bd.valid) {
    
      if (bd.refbit) {
        bd.refbit=false;
        continue;
      }
    
      if (bd.pinCnt) {
        pinCnt++;
        if (pinCnt==numBufs) {
          return BUFFEREXCEEDED;
        }
        continue;
      }
    
      if (bd.dirty) {
        Status st = bd.file->writePage(bd.pageNo, &bufPool[bd.frameNo]);
        if (st!=OK) {
          return st;
        }
      }
    }
    bd.Clear();
    frame=clockHand;
    return OK;
  }
  return OK;
}

	
const Status BufMgr::readPage(File* file, const int PageNo, Page*& page) {
  int frame;
  Status st = hashTable->lookup(file, PageNo, frame);
  if (st==HASHNOTFOUND) {
    
    st = this->allocBuf(frame);
    if (st!=OK) return st;
    
    st = hashTable->insert(file, PageNo, frame);
    if (st!=OK) return st;
    BufDesc bd = bufTable[frame];
    bd.Set(file, PageNo);
    page = &bufPool[bd.frameNo];
    file->readPage(PageNo, page);
  }
  else {
    BufDesc bd = bufTable[frame];
    page = &bufPool[bd.frameNo];
    bd.pinCnt = bd.pinCnt+1;
    bd.refbit=true;
  }
	return OK;
}


const Status BufMgr::unPinPage(File* file, const int PageNo, const bool dirty) {
  int frame;
  Status st = hashTable->lookup(file, PageNo, frame);
  if (st!=OK) return st;
  BufDesc bd = bufTable[frame];
  if (bd.pinCnt==0) return PAGENOTPINNED;
  bd.pinCnt--;
  if (dirty) bd.dirty = true;
	return OK;
}


const Status BufMgr::allocPage(File* file, int& pageNo, Page*& page)  {
  int newPage;
  int newFrame;
  Status st;
  
  st = file->allocatePage(newPage);
  if (st!=OK) return st;
  
  st = this->allocBuf(newFrame);
  if (st!=OK) return st;
  
  st = hashTable->insert(file, newPage, newFrame);
  if (st!=OK) return st;
  
  pageNo = newPage;
  bufTable[newFrame].Set(file, newPage);
  page = &bufPool[bufTable[newFrame].frameNo];
  
	return OK;
}


const Status BufMgr::disposePage(File* file, const int pageNo) {
  int frame;
  Status st = hashTable->lookup(file, pageNo, frame);
  if (st!=OK) return st;
  BufDesc bd = bufTable[frame];
  bd.file->disposePage(bd.pageNo);
  bd.Clear();
  st = hashTable->remove(file, pageNo);
  if (st!=OK) return st;
	return OK;
}


const Status BufMgr::flushFile(const File* file) {
  for (int i=0; i<numBufs; i++) {
    BufDesc bd = bufTable[i];
    if (bd.file==file) {
      if (bd.pinCnt!=0) {
        return PAGEPINNED;
      }
      Status st = hashTable->remove(file, bd.pageNo);
      if (bd.dirty) {
        st = bd.file->writePage(bd.pageNo, &bufPool[bd.frameNo]);
        if (st!=OK) return st;
      }
      bd.Clear();
    }
  }
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


