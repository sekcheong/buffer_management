#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "page.h"
#include "buf.h"


#define CALL(c)    { Status s; \
                     if ((s = c) != OK) { \
		       cerr << "At line " << __LINE__ << ":" << endl << "  "; \
                       error.print(s); \
                       cerr << "TEST DID NOT PASS" <<endl; \
                       exit(1); \
                     } \
                   }

#define FAIL(c)  { Status s; \
                   if ((s = c) == OK) { \
                     cerr << "At line " << __LINE__ << ":" << endl << "  "; \
                     cerr << "This call should fail: " #c << endl; \
                     cerr << "TEST DID NOT PASS" <<endl; \
                     exit(1); \
		     } \
		     }

BufMgr*     bufMgr;

int main()
{

  struct stat statusBuf;


    Error       error;
    DB          db;
    File*	file1;
    File*	file2;
    File* 	file3;
    File*   file4;
    int		i;
    const int   num = 100;
    int         j[num];    

    // create buffer manager

    bufMgr = new BufMgr(num);

    // create dummy files

    lstat("test.1", &statusBuf);
    if (errno == ENOENT)
      errno = 0;
    else 
      (void)db.destroyFile("test.1");

    lstat("test.2", &statusBuf);
    if (errno == ENOENT)
      errno = 0;
    else 
      (void)db.destroyFile("test.2");

    lstat("test.3", &statusBuf);
    if (errno == ENOENT)
      errno = 0;
    else
     (void)db.destroyFile("test.3");

    lstat("test.4", &statusBuf);
    if (errno == ENOENT)
      errno = 0;
    else
      (void)db.destroyFile("test.4");

    CALL(db.createFile("test.1"));
    ASSERT(db.createFile("test.1") == FILEEXISTS);
    CALL(db.createFile("test.2"));
    CALL(db.createFile("test.3"));
    CALL(db.createFile("test.4"));

    CALL(db.openFile("test.1", file1));
    CALL(db.openFile("test.2", file2));
    CALL(db.openFile("test.3", file3));
    CALL(db.openFile("test.4", file4));

    // test buffer manager

    Page* page;
    Page* page2;
    Page* page3;
      char  cmp[PAGESIZE];
    int pageno, pageno2, pageno3;

    cout << "Allocating pages in a file..." << endl;
    for (i = 0; i < num; i++) {
      CALL(bufMgr->allocPage(file1, j[i], page));
      sprintf((char*)page, "test.1 Page %d %7.1f", j[i], (float)j[i]);
      CALL(bufMgr->unPinPage(file1, j[i], true));
    }
    cout <<"Test passed"<<endl<<endl;

    cout << "Reading pages back..." << endl;
    for (i = 0; i < num; i++) {
      CALL(bufMgr->readPage(file1, j[i], page));
      sprintf((char*)&cmp, "test.1 Page %d %7.1f", j[i], (float)j[i]);
      ASSERT(memcmp(page, &cmp, strlen((char*)&cmp)) == 0);
      CALL(bufMgr->unPinPage(file1, j[i], false));
    }
    cout<< "Test passed"<<endl<<endl;

   
    cout << "Writing and reading back multiple files..." << endl;
    cout << "Expected Result: ";
    cout << "The output will consist of the file name, page number, and a value."<<endl;
    cout << "The page number and the value should match."<<endl<<endl;

    for (i = 0; i < num/3; i++) 
    {
      CALL(bufMgr->allocPage(file2, pageno2, page2));
      sprintf((char*)page2, "test.2 Page %d %7.1f", pageno2, (float)pageno2);
      CALL(bufMgr->allocPage(file3, pageno3, page3));
      sprintf((char*)page3, "test.3 Page %d %7.1f", pageno3, (float)pageno3);
      pageno = j[random() % num];
      CALL(bufMgr->readPage(file1, pageno, page));
      sprintf((char*)&cmp, "test.1 Page %d %7.1f", pageno, (float)pageno);
      ASSERT(memcmp(page, &cmp, strlen((char*)&cmp)) == 0);
      cout << (char*)page << endl;
      CALL(bufMgr->readPage(file2, pageno2, page2));
      sprintf((char*)&cmp, "test.2 Page %d %7.1f", pageno2, (float)pageno2);
      ASSERT(memcmp(page2, &cmp, strlen((char*)&cmp)) == 0);
      CALL(bufMgr->readPage(file3, pageno3, page3));
      sprintf((char*)&cmp, "test.3 Page %d %7.1f", pageno3, (float)pageno3);
      ASSERT(memcmp(page3, &cmp, strlen((char*)&cmp)) == 0);
      CALL(bufMgr->unPinPage(file1, pageno, true));
    }
    
    for (i = 0; i < num/3; i++) {
      CALL(bufMgr->unPinPage(file2, i+1, true));
      CALL(bufMgr->unPinPage(file2, i+1, true));
      CALL(bufMgr->unPinPage(file3, i+1, true));
      CALL(bufMgr->unPinPage(file3, i+1, true));
    }

    cout << "Test passed" << endl<<endl;


#ifdef DEBUGBUF
    bufMgr->printSelf();
#endif

    cout << "\nReading \"test.1\"...\n";
    cout << "Expected Result: ";
    cout << "Pages in order.  Values matching page number.\n\n";

    for (i = 1; i < num/3; i++) {
      CALL(bufMgr->readPage(file1, i, page2));
      sprintf((char*)&cmp, "test.1 Page %d %7.1f", i, (float)i);
      ASSERT(memcmp(page2, &cmp, strlen((char*)&cmp)) == 0);
      CALL(bufMgr->unPinPage(file1, i, false));
    }

    cout << "Test passed" <<endl<<endl;

    cout << "\nReading \"test.2\"...\n";
    cout << "Expected Result: ";
    cout << "Pages in order.  Values matching page number.\n\n";

    for (i = 1; i < num/3; i++) {
      CALL(bufMgr->readPage(file2, i, page2));
      sprintf((char*)&cmp, "test.2 Page %d %7.1f", i, (float)i);
      ASSERT(memcmp(page2, &cmp, strlen((char*)&cmp)) == 0);
      cout << (char*)page2 << endl;
      CALL(bufMgr->unPinPage(file2, i, false));
    }
    cout << "Test passed" <<endl<<endl;


    cout << "\nReading \"test.3\"...\n";
    cout << "Expected Result: ";
    cout << "Pages in order.  Values matching page number.\n\n";

    for (i = 1; i < num/3; i++) {
      CALL(bufMgr->readPage(file3, i, page3));
      sprintf((char*)&cmp, "test.3 Page %d %7.1f", i, (float)i);
      ASSERT(memcmp(page3, &cmp, strlen((char*)&cmp)) == 0);
      cout << (char*)page3 << endl;
      CALL(bufMgr->unPinPage(file3, i, false));
    }

    cout << "Test passed" <<endl<<endl;

    cout << "\nTesting error condition...\n\n";
    cout << "Expected Result: Error statments followed by the \"Test passed\" statement."<<endl;

    Status status;
    FAIL(status = bufMgr->readPage(file4, 1, page));
    error.print(status);

    cout << "Test passed" <<endl<<endl;
 

    CALL(bufMgr->allocPage(file4, i, page));
    CALL(bufMgr->unPinPage(file4, i, true));
    FAIL(status = bufMgr->unPinPage(file4, i, false));
    error.print(status);

    cout << "Test passed" <<endl<<endl;

    for (i = 0; i < num; i++) {
      CALL(bufMgr->allocPage(file4, j[i], page));
      sprintf((char*)page, "test.4 Page %d %7.1f", j[i], (float)j[i]);
    }

    int tmp;
    FAIL(status = bufMgr->allocPage(file4, tmp, page));
    error.print(status);

    cout << "Test passed" <<endl<<endl;

    //bufMgr->BufDump();

#ifdef DEBUGBUF
    bufMgr->printSelf();
#endif

    for (i = 0; i < num; i++)
      CALL(bufMgr->unPinPage(file4, i+2, true));
    
    cout << "\nReading \"test.1\"...\n";
    cout << "Expected Result: ";
    cout << "Pages in order.  Values matching page number.\n\n";

    for (i = 1; i < num; i++) {
      CALL(bufMgr->readPage(file1, i, page));
      sprintf((char*)&cmp, "test.1 Page %d %7.1f", i, (float)i);
      ASSERT(memcmp(page, &cmp, strlen((char*)&cmp)) == 0);
      cout << (char*)page << endl;
    }
    
    cout << "Test passed" <<endl<<endl;

    cout << "flushing file with pages still pinned. Should generate an error" << endl;
    FAIL(status = bufMgr->flushFile(file1));
    error.print(status);

    cout << "Test passed"<<endl<<endl;

    for (i = 1; i < num; i++) 
      CALL(bufMgr->unPinPage(file1, i, true));

    CALL(bufMgr->flushFile(file1));


    CALL(db.closeFile(file1));
    CALL(db.closeFile(file2));
    CALL(db.closeFile(file3));
    CALL(db.closeFile(file4));

	CALL(db.destroyFile("test.1"));
	CALL(db.destroyFile("test.2"));
	CALL(db.destroyFile("test.3"));
    CALL(db.destroyFile("test.4"));

    delete bufMgr;

    cout << endl << "Passed all tests." << endl;

    return (1);
}
