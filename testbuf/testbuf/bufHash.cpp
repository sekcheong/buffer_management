#include <memory.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include "page.h"
#include "buf.h"

// buffer pool hash table implementation

int BufHashTbl::hash(const File* file, const int pageNo)
{
  int tmp, value;
  tmp = (long)file;  // cast of pointer to the file object to an integer
  value = (tmp + pageNo) % HTSIZE;
  return value;
}


BufHashTbl::BufHashTbl(int htSize)
{
  HTSIZE = htSize;
  // allocate an array of pointers to hashBuckets
  ht = new hashBucket* [htSize];
  for(int i=0; i < HTSIZE; i++)
    ht[i] = NULL;
}


BufHashTbl::~BufHashTbl()
{
  for(int i = 0; i < HTSIZE; i++) {
    hashBucket* tmpBuf = ht[i];
    while (ht[i]) {
      tmpBuf = ht[i];
      ht[i] = ht[i]->next;
      delete tmpBuf;
    }
  }
  delete [] ht;
}


//---------------------------------------------------------------
// insert entry into hash table mapping (file,pageNo) to frameNo;
// returns OK if OK, HASHTBLERROR if an error occurred
//---------------------------------------------------------------

Status BufHashTbl::insert(const File* file, const int pageNo, const int frameNo) {

  int index = hash(file, pageNo);

  hashBucket* tmpBuc = ht[index];
  while (tmpBuc) {
    if (tmpBuc->file == file && tmpBuc->pageNo == pageNo)
      return HASHTBLERROR;
    tmpBuc = tmpBuc->next;
  }

  tmpBuc = new hashBucket;
  if (!tmpBuc)
    return HASHTBLERROR;
  tmpBuc->file = (File*) file;
  tmpBuc->pageNo = pageNo;
  tmpBuc->frameNo = frameNo;
  tmpBuc->next = ht[index];
  ht[index] = tmpBuc;

  return OK;
}


//-------------------------------------------------------------------	     
// Check if (file,pageNo) is currently in the buffer pool (ie. in
// the hash table).  If so, return corresponding frameNo. else return 
// HASHNOTFOUND
//-------------------------------------------------------------------

Status BufHashTbl::lookup(const File* file, const int pageNo, int& frameNo) 
  {
  int index = hash(file, pageNo);
  hashBucket* tmpBuc = ht[index];
  while (tmpBuc) {
    if (tmpBuc->file == file && tmpBuc->pageNo == pageNo)
    {
      frameNo = tmpBuc->frameNo; // return frameNo by reference
      return OK;
    }
    tmpBuc = tmpBuc->next;
  }
  return HASHNOTFOUND;
}


//-------------------------------------------------------------------
// delete entry (file,pageNo) from hash table. REturn OK if page was
// found.  Else return HASHTBLERROR
//-------------------------------------------------------------------

Status BufHashTbl::remove(const File* file, const int pageNo) {

  int index = hash(file, pageNo);
  hashBucket* tmpBuc = ht[index];
  hashBucket* prevBuc = ht[index];

  while (tmpBuc) {
    if (tmpBuc->file == file && tmpBuc->pageNo == pageNo) {
      if (tmpBuc == ht[index]) 
	ht[index] = tmpBuc->next;
      else
	prevBuc->next = tmpBuc->next;
      delete tmpBuc;
      return OK;
    } else {
      prevBuc = tmpBuc;
      tmpBuc = tmpBuc->next;
    }
  }

  return HASHTBLERROR;
}
