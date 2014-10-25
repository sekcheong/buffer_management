#ifndef BUF_H
#define BUF_H

#include "db.h"
// define if debug output wanted
//#define DEBUGBUF

// declarations for buffer pool hash table
struct hashBucket
{
	File*	file;    // pointer a file object (more on this below)
	int	pageNo;  // page number within a file
	int	frameNo; // frame number of page in the buffer pool
	hashBucket* 	next;	 // next node in the hash table
};


// hash table to keep track of pages in the buffer pool
class BufHashTbl
{
private:
    int HTSIZE;
    hashBucket**  ht; // actual hash table
    int	 hash(const File* file, const int pageNo); // returns value between 0 and HTSIZE-1

public:
    BufHashTbl(const int htSize);  // constructor
    ~BufHashTbl(); // destructor
	
    // insert entry into hash table mapping (file,pageNo) to frameNo;
    // returns 0 if OK, HASHTBLERROR if an error occurred
  Status insert(const File* file, const int pageNo, const int frameNo);

    // Check if (file,pageNo) is currently in the buffer pool (ie. in
    // the hash table).  If so, return corresponding frameNo. else return 
    // HASHNOTFOUND
  Status lookup(const File* file, const int pageNo, int & frameNo);

    // delete entry (file,pageNo) from hash table. REturn OK if page was
    // found.  Else return HASHTBLERROR
  Status remove(const File* file, const int pageNo);  
};


class BufMgr;  //forward declaration of BufMgr class 

// class for maintaining information about buffer pool frames
class BufDesc {
    friend class BufMgr;
private:
  File* file;   // pointer to file object
  int   pageNo; // page within file
  int	frameNo;  // frame # of frame
  int   pinCnt; // number of times this page has been pinned
  bool 	dirty;	  // true if dirty;  false otherwise
  bool 	valid;   // true if page is valid
  bool  refbit;	 // has this buffer frame been reference recently

  void Clear() {  // initialize buffer frame for a new user
    	pinCnt = 0;
	file = NULL;
	pageNo = -1;
    	dirty = false;
	valid = false;
  };

  void Set(File* filePtr, int pageNum) { 
      file = filePtr;
      pageNo = pageNum;
      pinCnt = 1;
      dirty = false;
      valid = true;
      refbit = true;
  }

  BufDesc() {
      Clear();
  }
};


struct BufStats
{
  int accesses;    // Total number of accesses to buffer pool
  int diskreads;   // Number of pages read from disk (including allocs)
  int diskwrites;  // Number of pages written back to disk

  void clear()
    {
      accesses = diskreads = diskwrites = 0;
    }
      
  BufStats()
    {
      clear();
    }
};


class BufMgr 
{
private:
  unsigned int 	 clockHand;
  int   	 numBufs;    	// Number of pages in buffer pool
  BufHashTbl*    hashTable;  	// hash table mapping (File, page) to frame
  BufDesc*	 bufTable;  	// vector of status info, 1 per page
  BufStats	 bufStats;	// buffer pool statistics

  const Status allocBuf(int & frame);   // allocate a free frame.  
  const void releaseBuf(int frame); // return unused frame to end of list
  void advanceClock()
  {
	clockHand = (clockHand + 1) % numBufs;
  }


public:
  Page*	         bufPool;   // actual buffer pool

  BufMgr(const int bufs);
  ~BufMgr();

  const Status readPage(File* file, const int PageNo, Page*& page);
  const Status unPinPage(File* file, const int PageNo, const bool dirty);
  const Status allocPage(File* file, int& PageNo, Page*& page); 
                        // allocates a new, empty page 
  const Status flushFile(const File* file); // writing out all dirty pages of the file
  const Status disposePage(File* file, const int PageNo); // dispose of page in file
  void  printSelf();

  const BufStats & getBufStats() const // get buffer pool usage
  {
	return bufStats;
  }
  const void clearBufStats() 
  {
	bufStats.clear();
  }
};

#endif

