#include <memory.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include "page.h"
#include "db.h"
#include "buf.h"


#define DBP(p)      (*(DBPage*)&p)

// openfile hash table implementation
OpenFileHashTbl::OpenFileHashTbl()
{
  HTSIZE = 113; // hack 
  // allocate an array of pointers to fleHashBuckets
  ht = new fileHashBucket* [HTSIZE];
  for(int i=0; i < HTSIZE; i++) ht[i] = NULL;
}

OpenFileHashTbl::~OpenFileHashTbl()
{
  for(int i = 0; i < HTSIZE; i++) {
    fileHashBucket* tmpBuf = ht[i];
    while (ht[i]) {
      tmpBuf = ht[i];
      ht[i] = ht[i]->next;
      // blow away the file object in case someone forgot to close it
      if (tmpBuf->file != NULL) delete tmpBuf->file;
      delete tmpBuf;
    }
  }
  delete [] ht;
}

int OpenFileHashTbl::hash(const string fileName)
{
   int i, value, len;
   len =  (int) fileName.length();
   value = 0;
   for (i=0;i<len;i++) value = 31*value + (int) fileName[i];

   value  = abs(value % HTSIZE);
   return value;
}

// inserts fileName into hash table of open files
// returns OK if insertion was successful, HASHTBLERROR if an error occurred
//---------------------------------------------------------------

Status OpenFileHashTbl::insert(const string fileName, File* file ) 
{
  int index = hash(fileName);
  fileHashBucket* tmpBuc = ht[index];
  while (tmpBuc) {
    if (tmpBuc->fname == fileName) return HASHTBLERROR;
    tmpBuc = tmpBuc->next;
  }

  tmpBuc = new fileHashBucket;
  if (!tmpBuc) return HASHTBLERROR;
  tmpBuc->fname = fileName;
  tmpBuc->file = file;
  tmpBuc->next = ht[index];
  ht[index] = tmpBuc;

  return OK;
}


//-------------------------------------------------------------------	     
// returns OK if file is already open.  Else returns HASHNOTFOUND
// if the file is open it also returns a pointer to the associated file object
// via the file
//-------------------------------------------------------------------

Status OpenFileHashTbl::find(const string fileName, File*& file)
{
  int index = hash(fileName);
  fileHashBucket* tmpBuc = ht[index];
  while (tmpBuc) {
    if (tmpBuc->fname == fileName) 
    {
      file = tmpBuc->file;
      return OK;
    }
    tmpBuc = tmpBuc->next;
  }
  return HASHNOTFOUND;
}


//-------------------------------------------------------------------
// remove fileName from list of open files
// returns OK if file was removed.
// Else return HASHTBLERROR
//-------------------------------------------------------------------

Status OpenFileHashTbl::erase(const string fileName)
{
  int index = hash(fileName);
  fileHashBucket* tmpBuc = ht[index];
  fileHashBucket* prevBuc = ht[index];

  while (tmpBuc) {
    if (tmpBuc->fname == fileName)
    {
      if (tmpBuc == ht[index]) ht[index] = tmpBuc->next;
      else prevBuc->next = tmpBuc->next;
      tmpBuc->file = NULL;
      delete tmpBuc;
      return OK;
    } 
    else {
      prevBuc = tmpBuc;
      tmpBuc = tmpBuc->next;
    }
  }

  return HASHTBLERROR;
}

// Construct a File object which can operate on Unix files.

File::File(const string & fname)
{
  fileName = fname;
  openCnt = 0;
  unixFile = -1;
}

// Deallocate a file object
File::~File()
{
  if (openCnt == 0)
    return;

  // This means that file must be closed down if open
  // and buffer pages flushed.
  // To ensure that all this happens, must push down the openCnt to 1.
  openCnt = 1;

  Status status = close();
  if (status != OK)
    {
      Error error;
      error.print(status);
    }
}

Status const File::create(const string & fileName)
{
  int file;
  if ((file = ::open(fileName.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0666)) < 0)
    {
      if (errno == EEXIST)
	return FILEEXISTS;
      else
	return UNIXERR;
    }

  // An empty file contains just a DB header page.

  Page header;
  memset(&header, 0, sizeof header);
  DBP(header).nextFree = -1;
  DBP(header).firstPage = -1;
  DBP(header).numPages = 1;
  if (write(file, (char*)&header, sizeof header) != sizeof header)
    return UNIXERR;

  if (::close(file) < 0)
    return UNIXERR;

  return OK;
}

const Status File::destroy(const string & fileName)
{
  if (remove(fileName.c_str()) < 0)
  {
    cout << "db.destroy. unlink returned error" << "\n";
    return UNIXERR;
  }

  return OK;
}

const Status File::open()
{
  // Open file -- it will be closed in closeFile().

  if (openCnt == 0)
    {
      if ((unixFile = ::open(fileName.c_str(), O_RDWR)) < 0)
	return UNIXERR;

      // Store file info in open files table.

      openCnt = 1;
    }
  else
    openCnt++;

  return OK;
}

const Status File::close()
{
  if (openCnt <= 0)
    return FILENOTOPEN;

  openCnt--;

  // File actually closed only when open count goes to zero.

  if (openCnt == 0) {

    if (bufMgr)
      bufMgr->flushFile(this);

    if (::close(unixFile) < 0)
      return UNIXERR;
  }

  return OK;
}


// Allocate a page either from a free list (list of pages which
// were previously disposed of), or extend file if no free pages
// are available.

Status File::allocatePage(int& pageNo)
{
  Page header;
  Status status;

  if ((status = intread(0, &header)) != OK)
    return status;

  // If free list has pages on it, take one from there
  // and adjust free list accordingly.

  if (DBP(header).nextFree != -1) {     // free list exists?

    // Return first page on free list to the caller,
    // adjust free list accordingly.

    pageNo = DBP(header).nextFree;
    Page firstFree;
    if ((status = intread(pageNo, &firstFree)) != OK)
      return status;
    DBP(header).nextFree = DBP(firstFree).nextFree;

  } else {                              // no free list, have to extend file

    // Extend file -- the current number of pages will be
    // the page number of the page to be returned.

    pageNo = DBP(header).numPages;
    Page newPage;
    memset(&newPage, 0, sizeof newPage);
    if ((status = intwrite(pageNo, &newPage)) != OK)
      return status;

    DBP(header).numPages++;

    if (DBP(header).firstPage == -1)    // first user page in file?
      DBP(header).firstPage = pageNo;
  }

  if ((status = intwrite(0, &header)) != OK)
    return status;
  
#ifdef DEBUGFREE
  listFree();
#endif

  return OK;
}


// Deallocate a page from file. The page will be put on a free
// list and returned back to the caller upon a subsequent
// allocPage() call.

const Status File::disposePage(const int pageNo)
{
  if (pageNo < 1)
    return BADPAGENO;

  Page header;
  Status status;

  if ((status = intread(0, &header)) != OK)
    return status;

  // The first user-allocated page in the file cannot be
  // disposed of. The File layer has no knowledge of what
  // is the next page in the file and hence would not be
  // able to adjust the firstPage field in file header.

  if (DBP(header).firstPage == pageNo || pageNo >= DBP(header).numPages)
    return BADPAGENO;

  // Deallocate page by attaching it to the free list.

  Page away;
  if ((status = intread(pageNo, &away)) != OK)
    return status;
  memset(&away, 0, sizeof away);
  DBP(away).nextFree = DBP(header).nextFree;
  DBP(header).nextFree = pageNo;

  if ((status = intwrite(pageNo, &away)) != OK)
    return status;
  if ((status = intwrite(0, &header)) != OK)
    return status;

#ifdef DEBUGFREE
  listFree();
#endif

  return OK;
}


// Read a page from file and store page contents at the page address
// provided by the caller.

const Status File::intread(int pageNo, Page* pagePtr) const
{
  if (lseek(unixFile, pageNo * sizeof(Page), SEEK_SET) == -1)
    return UNIXERR;

  int nbytes = read(unixFile, (char*)pagePtr, sizeof(Page));

#ifdef DEBUGIO
  cerr << "%%  File " << (int)this << ": read bytes ";
  cerr << pageNo * sizeof(Page) << ":+" << nbytes << endl;
  cerr << "%%  ";
  for(int i = 0; i < 10; i++)
    cerr << *((int*)pagePtr + i) << " ";
  cerr << endl;
#endif

  if (nbytes != sizeof(Page))
    return UNIXERR;

  return OK;
}


// Write a page to file. Page data is at the page address
// provided by the caller.

const Status File::intwrite(const int pageNo, const Page* pagePtr)
{
  if (lseek(unixFile, pageNo * sizeof(Page), SEEK_SET) == -1)
    return UNIXERR;

  int nbytes = write(unixFile, (char*)pagePtr, sizeof(Page));

#ifdef DEBUGIO
  cerr << "%%  File " << (int)this << ": wrote bytes ";
  cerr << pageNo * sizeof(Page) << ":+" << nbytes << endl;
  cerr << "%%  ";
  for(int i = 0; i < 10; i++)
    cerr << *((int*)pagePtr + i) << " ";
  cerr << endl;
#endif

  if (nbytes != sizeof(Page))
    return UNIXERR;

  return OK;
}


// Read a page from file, check parameters for validity.

const Status File::readPage(const int pageNo, Page* pagePtr) const
{
  if (!pagePtr)
    return BADPAGEPTR;
  if (pageNo < 1)
    return BADPAGENO;

  return intread(pageNo, pagePtr);
}


// Write a page to file, check parameters for validity.

const Status File::writePage(const int pageNo, const Page *pagePtr)
{
  if (!pagePtr)
    return BADPAGEPTR;
  if (pageNo < 1)
    return BADPAGENO;

  return intwrite(pageNo, pagePtr);
}


// Return the number of the first page in file. It is stored
// on the file's header page (field firstPage).

const Status File::getFirstPage(int& pageNo) const
{
  Page header;
  Status status;

  if ((status = intread(0, &header)) != OK)
    return status;

  pageNo = DBP(header).firstPage;

  return OK;
}


#ifdef DEBUGFREE

// Print out the page numbers on the free list. For debugging only.

void File::listFree()
{
  cerr << "%%  File " << (int)this << " free pages:";
  int pageNo = 0;
  for(int i = 0; i < 10; i++) {
    Page page;
    if (intread(pageNo, &page) != OK)
      break;
    pageNo = DBP(page).nextFree;
    cerr << " " << pageNo;
    if (pageNo == -1)
      break;
  }
  cerr << endl;
}
#endif


// Construct a DB object which keeps track of creating, opening, and
// closing files.

DB::DB()
{
  // Check that DB header page data fits on a regular data page.

  if (sizeof(DBPage) >= sizeof(Page)) {
    cerr << "sizeof(DBPage) cannot exceed sizeof(Page): "
         << sizeof(DBPage) << " " << sizeof(Page) << endl;
    exit(1);
  }
}


// Destroy DB object. 

DB::~DB()
{
  // this could leave some open files open.
  // need to fix this by iterating through the hash table deleting each open file
}


  
// Create a database file.

const Status DB::createFile(const string &fileName) 
{
  File*  file;
  if (fileName.empty())
    return BADFILE;

  // First check if the file has already been opened
  if (openFiles.find(fileName, file) == OK) return FILEEXISTS;

  // Do the actual work
  return File::create(fileName);
}


// Delete a database file.

const Status DB::destroyFile(const string & fileName) 
{
  File* file;

  if (fileName.empty()) return BADFILE;

  // Make sure file is not open currently.
  if (openFiles.find(fileName, file) == OK) return FILEOPEN;
  
  // Do the actual work
  return File::destroy(fileName);
}


// Open a database file. If file already open, increment open count,
// otherwise find a vacant slot in the open files table and store
// file info there.

const Status DB::openFile(const string & fileName, File*& filePtr)
{
  Status status;
  File* file;

  if (fileName.empty()) return BADFILE;

  // Check if file already open. 
  if (openFiles.find(fileName, file) == OK) 
  {
      // file is already open, call open again on the file object
      // to increment it's open count.
      status = file->open();
      filePtr = file;
  }
  else
  {
      // file is not already open
      // Otherwise create a new file object and open it
      filePtr = new File(fileName);
      status = filePtr->open();

      if (status != OK)
	{
	  delete filePtr;
	  return status;
	}

      // Insert into the mapping table
      status = openFiles.insert(fileName, filePtr);
    }
  return status;
}


// Close a database file. Get file info from open files table,
// call Unix close() only if open count now goes to zero.

const Status DB::closeFile(File* file)
{
  if (!file) return BADFILEPTR;


  // Close the file
  file->close();

  // If there are no remaining references to the file, then we should delete
  // the file object and remove it from the openFilesMap

  if (file->openCnt == 0)
    {
      if (openFiles.erase(file->fileName) != OK) return BADFILEPTR;
      delete file;
    }

  return OK;
}
