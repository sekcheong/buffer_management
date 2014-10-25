#ifndef PAGE_H
#define PAGE_H

#include "error.h"
#include "string.h"

struct RID{
    int  pageNo;
    int	 slotNo;
};

const RID NULLRID = {-1,-1};

struct Record
{
  void* data;
  int length;
};

// slot structure
struct slot_t {
        short	offset;  
        short	length;  // equals -1 if slot is not in use
};

const unsigned PAGESIZE = 1024;
const unsigned DPFIXED= sizeof(slot_t)+4*sizeof(short)+2*sizeof(int);
const unsigned PAGEDATASIZE = PAGESIZE-DPFIXED+sizeof(slot_t);
// size of the data area of a page

// Class definition for a minirel data page.   
// The design assumes that records are kept compacted when
// deletions are performed. Notice, however, that the slot
// array cannot be compacted.  Notice, this class does not keep
// the records align, relying instead on upper levels to take
// care of non-aligned attributes

class Page {
private:
    char 	data[PAGESIZE - DPFIXED]; 
    slot_t 	slot[1]; // first element of slot array - grows backwards!
    short	slotCnt; // number of slots in use;
    short	freePtr; // offset of first free byte in data[]
    short	freeSpace; // number of bytes free in data[]
    short	dummy;	// for alignment purposes
    int		nextPage; // forwards pointer
    int		curPage;  // page number of current pointer

public:
    void init(const int pageNo); // initialize a new page
    void dumpPage() const;       // dump contents of a page

    const Status getNextPage(int& pageNo) const; // returns value of nextPage
    const Status setNextPage(const int pageNo); // sets value of nextPage to pageNo
    const short getFreeSpace() const; // returns amount of free space

    // inserts a new record (rec) into the page, returns RID of record 
    const Status insertRecord(const Record & rec, RID& rid);

    // delete the record with the specified rid
    const Status deleteRecord(const RID & rid);

    // returns RID of first record on page
    // returns  NORECORDS if page contains no records.  Otherwise, returns OK
    const Status firstRecord(RID& firstRid) const;

    // returns RID of next record on the page 
    // returns ENDOFPAGE if no more records exist on the page
    const Status nextRecord (const RID & curRid, RID& nextRid) const;

    // returns reference to record with RID rid
    const Status getRecord(const RID & rid, Record & rec);
};

#endif
