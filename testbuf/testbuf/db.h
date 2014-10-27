#ifndef DB_H
#define DB_H

#include <sys/types.h>
#include <functional>
#include "error.h"
#include <string.h>
using namespace std;

// define if debug output wanted

//#define DEBUGIO
//#define DEBUGFREE

// forward class definition for db
class DB;

// class definition for open files
class File {
  friend class DB;
  friend class OpenFileHashTbl;

 public:

  Status allocatePage(int& pageNo);     // allocate a new page
  const Status disposePage(const int pageNo);       // release space for a page
  const Status readPage(const int pageNo,
		  Page* pagePtr) const;       // read page from file
  const Status writePage(const int pageNo,
		   const Page* pagePtr);      // write page to file
  const Status getFirstPage(int& pageNo) const;     // returns pageNo of first page

  bool operator == (const File & other) const
    {
      return fileName == other.fileName;
    }

 private: 

  File(const string &fname);                   // initialize
  ~File();                  // deallocate file object

  static const Status create(const string &fileName);
  static const Status destroy(const string &fileName);

  const Status open();
  const Status close();

  const Status intread(const int pageNo,
		 Page* pagePtr) const;        // internal file read
  const Status intwrite(const int pageNo,
		  const Page* pagePtr);       // internal file write

#ifdef DEBUGFREE
  void listFree();                      // list free pages
#endif

  string fileName;                    // The name of the file
  int openCnt;                        // # times file has been opened
  int unixFile;                       // unix file stream for file
};

class BufMgr;
extern BufMgr* bufMgr;

// declarations for hash table of open files
struct fileHashBucket
{
	string	fname;    // name of the file
        File*   file;    // pointer to file object
	fileHashBucket* next;	 // next node in the hash table
	
};

// hash table to keep track of open files
class OpenFileHashTbl
{
private:
    int HTSIZE;
    fileHashBucket**  ht; // actual hash table
    int	 hash(string fileName);  // returns value between 0 and HTSIZE-1

public:
    OpenFileHashTbl();
    ~OpenFileHashTbl(); // destructor
	
    // returns OK if no error occured, HASHTBLERROR if an error occurred
    Status insert(const string fileName, File* file);

    // see if fileName is already in hash table.  If so a pointer to the file
    // object is returned.
    // returns OK if found. else returns HASHNOTFOUND
    Status find(const string fileName, File*& file);

    // returns OK if fileName was found.  Else return HASHTBLERROR
    Status erase(const string fileName);
};



class DB {
 public:
  DB();                                 // initialize open file table
  ~DB();                                // clean up any remaining open files

  const Status createFile(const string & fileName) ;  // create a new file
  const Status destroyFile(const string & fileName) ; // destroy a file, 
                                                           // release all space
  const Status openFile(const string & fileName, File* & file);  // open a file
  const Status closeFile(File* file);         // close a file

 private:
  OpenFileHashTbl   openFiles;    // list of open files
};


// structure of DB (header) page

typedef struct {
  int nextFree;                         // page # of next page on free list
  int firstPage;                        // page # of first page in file
  int numPages;                         // total # of pages in file
} DBPage;

#endif
