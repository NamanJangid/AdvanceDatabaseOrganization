#ifndef BTREE_MGR_H
#define BTREE_MGR_H

#include "dberror.h"
#include "tables.h"
#include "storage_mgr.h"
#include "buffer_mgr.h"

// structure for accessing btrees
typedef struct BTreeHandle {
  DataType keyType;
  char *idxId;
  void *mgmtData;
} BTreeHandle;

typedef struct BT_ScanHandle {
  BTreeHandle *tree;
  void *mgmtData;
} BT_ScanHandle;

typedef struct FileMetaData {
    int rootPageNumber;
    int maxEntriesPerPage;
    int numNodes;
    int numEntries;
    int keyType;
} FileMetaData;

typedef struct TreeMgmtData {
    FileMetaData fmd;
    BM_BufferPool *bufferPool;
    BM_PageHandle *pageHandle;
    SM_FileHandle fileHandle;
} TreeMgmtData;

typedef struct PageData {
    int pageNumber;
    int parentNode;
    int numEntries;
    int isLeaf;
    int *keys;
    float *pointers;
} PageData;

typedef struct ScanMgmtData {
    int *leafPages;
    int currentPage;
    int currentPosition;
    int nextPagePosInLeafPages;
    int numOfLeafPages;
    PageData currentPageData;
} ScanMgmtData;

typedef struct KeyData {
    int key;
    float left;
    float right;
} KeyData;

typedef struct Nodes {
    int nodeVal;
    bool nodeCheckVal;
    int headerNodeVal;
}Nodes;


// init and shutdown index manager
extern RC initIndexManager (void *mgmtData);
extern RC shutdownIndexManager ();

// create, destroy, open, and close an btree index
extern RC createBtree (char *idxId, DataType keyType, int n);
extern RC openBtree (BTreeHandle **tree, char *idxId);
extern RC closeBtree (BTreeHandle *tree);
extern RC deleteBtree (char *idxId);

// access information about a b-tree
extern RC getNumNodes (BTreeHandle *tree, int *result);
extern RC getNumEntries (BTreeHandle *tree, int *result);
extern RC getKeyType (BTreeHandle *tree, DataType *result);

// index access
extern RC findKey (BTreeHandle *tree, Value *key, RID *result);
extern RC insertKey (BTreeHandle *tree, Value *key, RID rid);
extern RC deleteKey (BTreeHandle *tree, Value *key);
extern RC openTreeScan (BTreeHandle *tree, BT_ScanHandle **handle);
extern RC nextEntry (BT_ScanHandle *handle, RID *result);
extern RC closeTreeScan (BT_ScanHandle *handle);

// debug and test functions
extern char *printTree (BTreeHandle *tree);

#endif // BTREE_MGR_H
