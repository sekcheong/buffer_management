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
  
  for (int i=0; i<numBufs; i++) {
    BufDesc *bd = &bufTable[i];
    if (bd->dirty) {
      Status st = bd->file->writePage(bd->pageNo, &bufPool[bd->frameNo]);
      if (st!=OK) {
        cerr << "Error: " << st << endl;
        exit(1);
      }
    }
  }

  delete hashTable;
  delete []bufPool;
  delete []bufTable;
}


const Status BufMgr::allocBuf(int &slot) {
  int pinCnt = 0;      //tracks the number of pages being pinned
  int ticks = 0;       //tracks the number of ticks
  BufDesc *bd = 0;
  slot=-1;
  Status st;
  bool blockFound = false;
  
  while (true) {
    advanceClock();
    ticks++;
    
    //if we already went through the buffer twice and couldn't find a free block
    if (ticks>2*numBufs) {
      return BUFFEREXCEEDED;
    }
    
    bd = &bufTable[clockHand];
    if (!bd->valid) {
      blockFound = true;
      break;
    }
    
    if (bd->refbit) {
      bd->refbit = false;
      continue;
    }
    
    //if the block is not pinned, we write the block to disk if it's dirty
    if (bd->pinCnt==0) {
      
      if (bd->dirty) {
        //cout << "Writing: " << bd->file->getFileName() << " P:" << bd->pageNo << " [" << (char*) bufPool[bd->frameNo].getData() << "]" << endl;
        st = bd->file->writePage(bd->pageNo, &bufPool[bd->frameNo]);
        if (st!=OK) return st;
      }
      
      st = hashTable->remove(bd->file, bd->pageNo);
      if (st!=OK) return st;
      
      blockFound = true;
      break;
    }
    else {
      pinCnt++;
      //all blocks are pinned?
      if (pinCnt==numBufs) {
        return BUFFEREXCEEDED;
      }
      continue;
    }
  }
  
  if (blockFound) {
    bd->Clear();
    slot = clockHand;
    return OK;
  }
  
  return BADBUFFER;
}



const Status BufMgr::readPage(File* file, const int PageNo, Page*& page) {
  int slot = 0;
  page = 0;
  Status st = hashTable->lookup(file, PageNo, slot);
  
  if (st==HASHNOTFOUND) {
    st = this->allocBuf(slot);
    if (st!=OK) return st;
    
    BufDesc *bd = &bufTable[slot];
    page = &bufPool[bd->frameNo];
    st = file->readPage(PageNo, page);
    if (st!=OK) return st;
    bd->Set(file, PageNo);

    st = hashTable->insert(file, PageNo, slot);
    if (st!=OK) return st;

  }
  else {
    BufDesc *bd = &bufTable[slot];
    page = &bufPool[bd->frameNo];
    bd->pinCnt = bd->pinCnt+1;
    bd->refbit = true;
  }
  return OK;
}


const Status BufMgr::unPinPage(File* file, const int PageNo, const bool dirty) {
  int slot = -1;
  Status st = hashTable->lookup(file, PageNo, slot);
  if (st!=OK) return st;
  BufDesc *bd = &bufTable[slot];
  if (bd->pinCnt==0) return PAGENOTPINNED;
  bd->pinCnt = bd->pinCnt-1;
  if (dirty) bd->dirty = true;
  return OK;
}


const Status BufMgr::allocPage(File* file, int& pageNo, Page*& page)  {
  int newPage = -1;
  int newSlot = -1;
  page = 0;
  Status st;
  
  st = file->allocatePage(newPage);
  if (st!=OK) return st;
  
  st = this->allocBuf(newSlot);
  if (st!=OK) return st;
  
  st = hashTable->insert(file, newPage, newSlot);
  if (st!=OK) return st;
  
  pageNo = newPage;
  BufDesc *bd = &bufTable[newSlot];
  bd->Set(file, newPage);
  page = &bufPool[bd->frameNo];
  
	return OK;
}


const Status BufMgr::disposePage(File* file, const int pageNo) {
  int slot;
  Status st = hashTable->lookup(file, pageNo, slot);
  if (st!=OK) return st;
  BufDesc *bd = &bufTable[slot];
  bd->file->disposePage(bd->pageNo);
  bd->Clear();
  st = hashTable->remove(file, pageNo);
  if (st!=OK) return st;
	return OK;
}


const Status BufMgr::flushFile(const File* file) {
  Status st;
  for (int i=0; i<numBufs; i++) {
    BufDesc *bd = &bufTable[i];
    if (!bd->valid) continue;
    if (bd->file!=file) continue;
    
    if (bd->pinCnt!=0) {
      return PAGEPINNED;
    }
    if (bd->dirty) {
      st = bd->file->writePage(bd->pageNo, &bufPool[bd->frameNo]);
      if (st!=OK) return st;
    }
    st = hashTable->remove(file, bd->pageNo);
    if (st!=OK) return st;
    bd->Clear();
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


