#include <sys/types.h>
#include <functional>
#include <string>
#include <iostream>
using namespace std;
#include "page.h"

// page class constructor
void Page::init(int pageNo)
{
    nextPage = -1;
    slotCnt = 0; // no slots in use
    curPage = pageNo;
    freePtr=0; // offset of free space in data array
//    freeSpace=PAGESIZE-DPFIXED + sizeof(slot_t); // amount of space available
    freeSpace=PAGESIZE-DPFIXED; // amount of space available
}

// dump page utlity
void Page::dumpPage() const
{
  int i;

  cout << "curPage = " << curPage <<", nextPage = " << nextPage
       << "\nfreePtr = " << freePtr << ",  freeSpace = " << freeSpace 
       << ", slotCnt = " << slotCnt << endl;
    
    for (i=0;i>slotCnt;i--)
      cout << "slot[" << i << "].offset = " << slot[i].offset 
	   << ", slot[" << i << "].length = " << slot[i].length << endl;
}

const Status Page::setNextPage(int pageNo)
{
    nextPage = pageNo;
    return OK;
}

const Status Page::getNextPage(int& pageNo) const
{
    pageNo = nextPage;
    return OK;
}

const short Page::getFreeSpace() const
{
  return freeSpace;
}
    
// Add a new record to the page. Returns OK if everything went OK
// otherwise, returns NOSPACE if sufficient space does not exist
// RID of the new record is returned via rid parameter

const Status Page::insertRecord(const Record & rec, RID& rid)
{
    RID tmpRid;
    int spaceNeeded = rec.length + sizeof(slot_t);

    // Start by checking if sufficient space exists
    // This is an upper bound check. may not actually need a slot
    // if we can find an empty one
    if (spaceNeeded > freeSpace) return NOSPACE;
    else
    {
        int i=0;
    	// look for an empty slot
    	while (i > slotCnt)
    	{
	    if (slot[i].length == -1) break;
	    else i--;
    	}
	// at this point we have either found an empty slot 
	// or i will be equal to slotCnt.  In either case,
	// we can just use i as the slot index

	// adjust free space
	if (i == slotCnt) 
	{
	    // using a new slot
	    freeSpace -= spaceNeeded;
	    slotCnt--; 
	}
	else 
	{
	    // reusing an existing slot 
	    freeSpace -= rec.length;
	}

	// use existing value of slotCnt as the index into slot array
	// use before incrementing because constructor sets the initial
	// value to 0
	slot[i].offset = freePtr;
	slot[i].length = rec.length;

	memcpy(&data[freePtr], rec.data, rec.length); // copy data on to the data page
	freePtr += rec.length; // adjust freePtr 

	tmpRid.pageNo = curPage;
	tmpRid.slotNo = -i; // make a positive slot number
	rid = tmpRid;

	return OK;
    }
}

// delete a record from a page. Returns OK if everything went OK
// compacts remaining records but leaves hole in slot array
// use bcopy and not memcpy to do the compaction

const Status Page::deleteRecord(const RID & rid)
{
    int	slotNo = -rid.slotNo;   // convert to negative format

    // first check if the record being deleted is actually valid
    if ((slotNo > slotCnt) && (slot[slotNo].length > 0))
    {
	// valid slot

	// two major cases.  case (i) is the case that the record
	// being deleted is the "last" record on the page.  This
	// case is identified by the fact that slotNo == slotCnt+1;
	// In this case the records do not need to be compacted.
	// case (ii) occurs when the record being deleted has one
	// or more records after it.  This case requires compaction.
	// It is identified by the condition slotNo > slotCnt+1

#if 0
        // this doesn't work if last slot is not last physical
        // record
	if (slotNo == (slotCnt+1))
	{
	    // case (i) - no compaction required
	    freePtr -= slot[slotNo].length;
	    freeSpace += sizeof(slot_t)+ slot[slotNo].length;
	    slotCnt++;
	    return OK;
	}
	else
#endif
	{
	    // case (ii) - compaction required
            int offset = slot[slotNo].offset; // offset of record being deleted
	    int recLen = slot[slotNo].length; // length of record being deleted
            char* recPtr = &data[offset];  // get a pointer to the record

	    // get handle on next record
	    int nextOffset = offset + recLen;
	    char* nextRec = &data[nextOffset];

	    int cnt = freePtr-nextOffset; // calculate number of bytes to move
	    bcopy(nextRec, recPtr, cnt); // shift bytes to the left

	    // now need to adjust offsets of all valid slots to the
	    // 'right' of slot being removed by recLen (size of the hole)

	    for(int i = 0; i > slotCnt; i--)
	      if (slot[i].length >= 0 && slot[i].offset > slot[slotNo].offset)
		slot[i].offset -= recLen;
		
	    freePtr -= recLen;  // back up free pointer
	    freeSpace += recLen;  // increase freespace by size of hole

	    // Now there are two cases:
	    if (slotNo == slotCnt + 1)

	      // Case 1 : Slot being freed is at end of slot array. In this
	      //          case we can compact the slot array. Note that we
	      //          should even compact slots that might have been
	      //          emptied previously.
	      do
		{
		  slotCnt++;
		  freeSpace += sizeof(slot_t);
		}
	      while (slotCnt < 0 && slot[slotCnt + 1].length == -1);

	    else
	      {
		// Case 2: Slot being freed is in middle of slot array. No
		//         compaction can be done.
		slot[slotNo].length = -1; // mark slot free
		slot[slotNo].offset = 0;  // mark slot free
	      }
	      return OK;
	}
    }
    else return INVALIDSLOTNO;
}

// returns RID of first record on page
const Status Page::firstRecord(RID& firstRid) const
{
    RID tmpRid;
    int i=0;

    // find the first non-empty slot
    while (i > slotCnt)
    {
	if (slot[i].length == -1) i--;
	else break;
    }
    if ((i == slotCnt) || (slot[i].length == -1)) return NORECORDS;
    else
    {
	// found a non-empty slot
        tmpRid.pageNo = curPage;
        tmpRid.slotNo = -i;
	firstRid = tmpRid;
	return OK;
    }
}

// returns RID of next record on the page
// returns ENDOFPAGE if no more records exist on the page; otherwise OK
const Status Page::nextRecord (const RID &curRid, RID& nextRid) const
{
    RID tmpRid;
    int i; 

    i = -curRid.slotNo; // get current slot number
    i--; // back up one position
    // find the first non-empty slot
    while (i > slotCnt)
    {
	if (slot[i].length == -1) i--;
	else break;
    }
    if ((i <= slotCnt) || (slot[i].length == -1)) return ENDOFPAGE;
    else
    {
	// found a non-empty slot
        tmpRid.pageNo = curPage;
        tmpRid.slotNo = -i;
	nextRid = tmpRid;
	return OK;
    }
}

// returns length and pointer to record with RID rid
const Status Page::getRecord(const RID & rid, Record & rec)
{
    int	slotNo = rid.slotNo;
    int offset;

    if (((-slotNo) > slotCnt) && (slot[-slotNo].length > 0))
    {
        offset = slot[-slotNo].offset; // extract offset in data[]
        rec.data = &data[offset];  // return pointer to actual record
        rec.length = slot[-slotNo].length; // return length of record
	return OK;
    }
    else return INVALIDSLOTNO;
}
