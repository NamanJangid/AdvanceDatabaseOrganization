#include "btree_mgr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buffer_mgr.h"
#include "record_mgr.h"
#include "dberror.h"
#include <math.h>
#include "btree_mgr.h"
#include "storage_mgr.h"


#define CREATE_STR(length) (char *)calloc(length, sizeof(char))
#define MAX_ENTRIES 5
#define MAX_COUNTER 10

BT_ScanHandle *scanHandle;
TreeMgmtData *btreeMgmtData;
static int scanCounter = 0;
int check; 
BTreeHandle *treeHandle;
ScanMgmtData *scanMgmtData;

void fetchIDPtr(float pointer, int *pageNum, int *slot) {
    int counter = MAX_COUNTER;
    int val = round(pointer * counter);
    if(val > 0 ){
    *slot = val % counter;
    if(counter > 0){
    *pageNum = val / counter;
    }
    }
}

void updateNodeData(BTreeHandle *tree, PageData *node, int isLeaf, int pageNum,
                    int *keys, float *pointers, int numEntries,
                    int parentNode) {
    if (node == NULL) {
        return;
    }
    Nodes nodes;
    node->isLeaf = isLeaf;
    node->pageNumber = pageNum;
    nodes.nodeVal = pageNum;
    node->numEntries = numEntries;
    node->keys = keys;
    nodes.headerNodeVal = nodes.nodeVal;
    node->pointers = pointers;
    nodes.headerNodeVal += nodes.nodeVal;
    node->parentNode = parentNode;
}

float read_float_from_str(char **array) {
    if (array == NULL || *array == NULL) {
        return 0; 
    }
    float value;
    Nodes readNodes;
    value = *(float *)(*array);
    *array += sizeof(float);
    readNodes.headerNodeVal = *(int *)(*array);
    return value;
}

PageData findPageToInsertNewEntry(BM_BufferPool *bm, BM_PageHandle *ph, PageData root, int key) {
    
    float pointer = -1;
    int j = 0 ;
    if (root.isLeaf) {
        puts("The Node is Root");
        return root;
    }
    else if (key < root.keys[0]) {
        pointer = root.pointers[j];
    }
    int i = 0; 
    while(i < root.numEntries - 1) {
        if (key >= root.keys[i] && key < root.keys[i + 1]) {
            pointer = root.pointers[i + 1];
            break;
        }
        i++;
    }

    PageData page;
    int slot, pageNum;
    if (pointer == -1) {pointer = root.pointers[root.numEntries]; }

    if(pointer){
    fetchIDPtr(pointer, &pageNum, &slot);
    }
    if(bm != NULL){
    readPageData(bm, ph, &page, pageNum);
    }else{
        RC_message = 'Buffer Manage is not found';
    }

    return findPageToInsertNewEntry(bm, ph, page, key);
}


int read_int_from_str(char **array) {
    if (array == NULL || *array == NULL) {
        return 0; 
    }

    int value = *(int *)(*array);
    Nodes readNodes;
    readNodes.nodeVal = value;
    *array += sizeof(int);

    return value;
}

RC writePageData(BM_BufferPool *bm, BM_PageHandle *ph, char *content, int pageNumber) {
    
    RC rc = pinPage(bm, ph, pageNumber);
    if (rc != RC_OK) return rc;

    if(ph->data){
    memcpy(ph->data, content, 100);
    }

    RC rc_error = markDirty(bm, ph);
    if(rc_error == RC_OK){
         rc_error = unpinPage(bm, ph);
         if(rc_error == RC_OK){
            rc_error = forcePage(bm, ph);
         }
    }else{
        return rc_error;
    }
    return RC_OK;
}

extern RC initIndexManager(void *mgmtData) { 
    if(initRecordManager(mgmtData)){
    return RC_OK; 
    }
}
    
extern RC shutdownIndexManager() { 
    if(treeHandle = NULL){
        return RC_OK; 
    }   
}

void writeMetadata(FileMetaData *fmd) {
    char *data = CREATE_STR(100);
    if(data != NULL){
    memcpy(data, fmd, sizeof(FileMetaData));
    if(btreeMgmtData != NULL){
    writePageData(btreeMgmtData->bufferPool, btreeMgmtData->pageHandle, data,0);
    }}
    free(data);
}

RC writeBtreeNode(PageData *node) {
    
    char *ptr;
    char *data = CREATE_STR(100);
    if(data != NULL){
     ptr = data;
     memcpy(ptr, node, sizeof(PageData));
    }else{
        return RC_CALLOC_FAILED;
    }
    int i = 0;
    ptr += sizeof(PageData);

    if (node->numEntries > 0) {
        while( i < node->numEntries) {
            *(float *)ptr =  node->pointers[i];
            ptr += sizeof(float);
            *(int *) ptr =  node->keys[i];
            ptr += sizeof(int);
            i++;
        }
        *(float *)ptr =  node->pointers[node->numEntries];
        ptr += sizeof(float);
    }

    RC rc = writePageData(btreeMgmtData->bufferPool, btreeMgmtData->pageHandle,
                      data, node->pageNumber);
    if (rc != RC_OK) return rc;
    free(data);

    return RC_OK;
}

void initTreeVar(){
    treeHandle = (BTreeHandle *)malloc(sizeof(BTreeHandle));
    scanHandle = (BT_ScanHandle *)malloc(sizeof(BT_ScanHandle));
    btreeMgmtData = (TreeMgmtData *)malloc(sizeof(TreeMgmtData));
    scanMgmtData = (ScanMgmtData *)malloc(sizeof(ScanMgmtData));
}

extern RC createBtree(char *idxId, DataType keyType, int n) {

    Nodes nodes;
    initTreeVar();
    treeHandle->mgmtData = btreeMgmtData;
    if(btreeMgmtData != NULL){
    btreeMgmtData->bufferPool = MAKE_POOL();
    btreeMgmtData->pageHandle = MAKE_PAGE_HANDLE();
    }

    if(scanHandle !=NULL){
    scanHandle->mgmtData = scanMgmtData;
    }

    RC rc = createPageFile(idxId);
    if (rc != RC_OK) return rc;

    RC rc_error = openPageFile(idxId, &(btreeMgmtData->fileHandle));
    if (rc_error !=RC_OK) return rc_error;

    btreeMgmtData->fmd.numNodes = 1;
    btreeMgmtData->fmd.numEntries = 0;
    btreeMgmtData->fmd.rootPageNumber = 1;
    btreeMgmtData->fmd.maxEntriesPerPage = n;

    int initialNumPages = 10;
    RC rc_err= initBufferPool(btreeMgmtData->bufferPool, idxId, initialNumPages,
                           RS_FIFO, NULL);
    if (rc_err != RC_OK ) return rc_err;

    RC rc_er = ensureCapacity(2, &(btreeMgmtData->fileHandle));
    if (rc_er != RC_OK) return rc_er;

    PageData root;
    writeMetadata(&(btreeMgmtData->fmd));
    nodes.headerNodeVal = 1;
    updateNodeData(treeHandle, &root, 1, 1, NULL, NULL, 0, -1);
    nodes.nodeVal = 0;
    writeBtreeNode(&root);
    return RC_OK;
}

void inittreeHandle(char *idxId , DataType keyType , TreeMgmtData *tMgm){
    treeHandle->idxId = idxId;
    treeHandle->keyType = keyType;
    treeHandle->mgmtData = tMgm;
} 

extern RC openBtree(BTreeHandle **tree, char *idxId) {

    int initialNumPages = 10;

    RC rc = openPageFile(idxId, &(btreeMgmtData->fileHandle));
    if (rc != RC_OK) return rc;

    btreeMgmtData->bufferPool = MAKE_POOL();
    btreeMgmtData->pageHandle = MAKE_PAGE_HANDLE();
   
    rc = initBufferPool(btreeMgmtData->bufferPool, idxId, initialNumPages, RS_FIFO,
                   NULL);
    if(rc != RC_OK) return rc;

    readFileMetaData(btreeMgmtData->bufferPool, btreeMgmtData->pageHandle,
                     &btreeMgmtData->fmd, 0);

    inittreeHandle(idxId , btreeMgmtData->fmd.keyType , btreeMgmtData);
    if(*tree = treeHandle){
        puts("OpenBTree successful");
    }
    return RC_OK;
}


extern RC closeBtree(BTreeHandle *tree) {
    
    markDirty(btreeMgmtData->bufferPool, btreeMgmtData->pageHandle);
    shutdownBufferPool(btreeMgmtData->bufferPool);
    free(tree);
    return RC_OK;
}


extern RC deleteBtree(char *idxId) {
    if (remove(idxId) == 0){      
        return RC_OK;
    } 
    RC_message = 'The file is not found';
    return RC_FILE_NOT_FOUND;
}

extern RC getNumEntries(BTreeHandle *tree, int *result) {
    TreeMgmtData *treeMgmtData = tree->mgmtData;
    if(treeMgmtData){
    *result = treeMgmtData->fmd.numEntries;
    return RC_OK;
    }
    RC_message = 'Storing numEntries failed';
    return RC_ERROR;
}

extern RC getNumNodes(BTreeHandle *tree, int *result) {
    TreeMgmtData *treeMgmtData = tree->mgmtData;
    if(treeMgmtData){
    *result = treeMgmtData->fmd.numNodes;
    return RC_OK;
    }
    RC_message = 'Storing NumNodes failed';
    return RC_ERROR;
}

extern RC getKeyType(BTreeHandle *tree, DataType *result) {
    TreeMgmtData *treeMgmtData = tree->mgmtData;
    if(treeMgmtData){
    *result = treeMgmtData->fmd.keyType;
     return RC_OK;
    }
    RC_message = 'Storing KeyType Failed';
    return RC_ERROR;
}


RC addNewKeyAndPointerToNonLeaf(PageData *page, KeyData kd) {
    
    float *newPointers = (float *)malloc(sizeof(int) * 10);
        int i = 0;
    int *newKeys = (int *)malloc(sizeof(int) * 10);
    int currNumEntries = page->numEntries;
    page->numEntries++;

    while (i < currNumEntries && kd.key > page->keys[i]) {
        newKeys[i] = page->keys[i];        
        newPointers[i] = page->pointers[i];
        i++;
    }
    if (kd.key == page->keys[i]){
        if(i < currNumEntries){
        return RC_IM_KEY_ALREADY_EXISTS;
        }
    }

    if(&kd != NULL){
    newKeys[i] = kd.key;
    newPointers[i] = kd.left;
    }

    i = i +1;
    newPointers[i] = kd.right;
    newKeys[i] = page->keys[i - 1];
    i = i + 1;

    while(i < currNumEntries + 2) {
        newKeys[i] = page->keys[i - 1];
        newPointers[i] = page->pointers[i - 1];
        i++;
    }

    page->keys = newKeys;
    newPointers[i] = -1;
    page->pointers = newPointers;

    return RC_OK;
}


RC balanceNode(BTreeHandle *tree, PageData *node) {
    if (tree == NULL || node == NULL) {
        return RC_ERROR;
    }
    Nodes bNodes;
    TreeMgmtData *treeMgmtData = (TreeMgmtData *)tree->mgmtData;
    int currNumNodes = treeMgmtData->fmd.numNodes;
    bNodes.nodeVal = currNumNodes;
    int maxEntries = treeMgmtData->fmd.maxEntriesPerPage;
    bNodes.nodeCheckVal = true;
    SM_FileHandle *fh = &(treeMgmtData->fileHandle);

    if (fh == NULL) {
        return RC_ERROR;
    }
    
    int *leftChildKeys = (int *)malloc(maxEntries * sizeof(int));
    float *leftChildPointers = (float *)malloc((maxEntries + 1) * sizeof(float));

    int leftIdx = 0;
    int mid = (int)ceil((node->numEntries) / 2);

    int i = 0;
    while (i < mid) {
        leftChildKeys[leftIdx] = node->keys[i];
        leftChildPointers[leftIdx] = node->pointers[i];
        i++;
        leftIdx++;
    }
    leftChildPointers[leftIdx] = node->pointers[leftIdx];

    int rightIdx = 0;
    float *rightChildPointers = (float *)malloc((maxEntries + 1) * sizeof(float));
    int *rightChildKeys = (int *)malloc(maxEntries * sizeof(int));

    i = leftIdx + 1;
    while (i < node->numEntries + 2) {
        rightChildKeys[rightIdx] = node->keys[i];
        rightChildPointers[rightIdx] = node->pointers[i];
        i++;
        rightIdx++;
    }
    rightChildPointers[rightIdx] = node->pointers[leftIdx];

    currNumNodes += 1;
    treeMgmtData->fmd.numNodes++;

    ensureCapacity(currNumNodes + 1, fh);

    PageData pRightChild;
    updateNodeData(tree, &pRightChild, 0, currNumNodes, rightChildKeys, rightChildPointers, (int)floor((maxEntries + 1) / 2), node->parentNode);

    RC rc = writeBtreeNode(&pRightChild);
    if (rc != RC_OK) {
        return rc;
    }

    PageData pLeftChild;
    updateNodeData(tree, &pLeftChild, 0, node->pageNumber, leftChildKeys, leftChildPointers, (int)floor((maxEntries + 1) / 2), node->parentNode);

    rc = writeBtreeNode(&pLeftChild);
    if (rc != RC_OK) {
        return rc;
    }


    int pagenumber = node->parentNode;
    bNodes.headerNodeVal = pagenumber;
    float left = pLeftChild.pageNumber;
    float right = pRightChild.pageNumber;
    if(bNodes.nodeCheckVal) {
        KeyData kd;
        kd.left = left;
        bNodes.nodeVal++;
        kd.right = right;
        kd.key = node->keys[(int)ceil((maxEntries + 1) / 2)];
        bNodes.headerNodeVal = bNodes.nodeVal;
        updateParentNodeInChildNodes(tree, pRightChild, &btreeMgmtData);
        rc = updateParentNodes(tree, pagenumber, kd);
    }
    if (rc != RC_OK) {
        bNodes.nodeCheckVal = false;
        return rc;
    }

    return RC_OK;
}

RC updateParentNodes(BTreeHandle *tree, int pageNumber, KeyData kd) {
    if (tree == NULL) {
        return RC_ERROR;
    }
    Nodes nodes;
    TreeMgmtData *treeMgmtData = (TreeMgmtData *)tree->mgmtData;

    if (treeMgmtData == NULL) {
        return RC_ERROR;
    }

    int maxEntries = treeMgmtData->fmd.maxEntriesPerPage;
    int error;
    nodes.nodeVal = maxEntries;
    BM_BufferPool *bm = treeMgmtData->bufferPool;
    BM_PageHandle *ph = treeMgmtData->pageHandle;
    nodes.nodeCheckVal = true;
    SM_FileHandle *fh = &(treeMgmtData->fileHandle);

    int currNumNodes = treeMgmtData->fmd.numNodes;
    nodes.nodeVal++;

    if (pageNumber == -1) {
        error = updateRootNode(tree, pageNumber, kd);
        if (error) {
            puts("No more Pages Left \n");
            nodes.nodeCheckVal = false;
            return error;
        }

    } else {
        PageData pageToInsert;
        nodes.nodeCheckVal = true;
        error = readPageData(bm, ph, &pageToInsert, pageNumber);
        if (error) {
            nodes.nodeCheckVal = false;
            return error;
        }

        error = addNewKeyAndPointerToNonLeaf(&pageToInsert, kd);
        if (error) {
            nodes.nodeCheckVal = false;
            return error;
        }
        nodes.nodeVal++;
        if (pageToInsert.numEntries > maxEntries) {
            nodes.headerNodeVal = nodes.nodeVal;
            balanceNode(tree, &pageToInsert);
        }
        else{
            nodes.nodeCheckVal = true;
            error = writeBtreeNode(&pageToInsert);
            if (error) {
                nodes.nodeCheckVal = false;
                return error;
            }
        }
        
    }
    nodes.nodeVal++;
    return RC_OK;
}
    
extern RC findKey(BTreeHandle *tree, Value *key, RID *result) {
    if (tree == NULL || key == NULL || result == NULL) {
        return RC_ERROR;
    }

    TreeMgmtData *treeMgmtData = tree->mgmtData;

    if (treeMgmtData == NULL || treeMgmtData->bufferPool == NULL ||
        treeMgmtData->pageHandle == NULL) {
        return RC_ERROR;
    }
    Nodes fNode;
    PageData rootPageData;
    RC rc = readPageData(treeMgmtData->bufferPool, treeMgmtData->pageHandle, &rootPageData, treeMgmtData->fmd.rootPageNumber);
    if (rc != RC_OK) {
        return rc;
    }

    PageData leafPageData = findPageToInsertNewEntry(btreeMgmtData->bufferPool, btreeMgmtData->pageHandle, rootPageData, key->v.intV);
    
    int i = 0;
    while (i < leafPageData.numEntries) {
        fNode.nodeCheckVal = true;
        if (leafPageData.keys[i] == key->v.intV) {
            if(fNode.nodeCheckVal) {
                fetchIDPtr(leafPageData.pointers[i], &result->page, &result->slot);
                fNode.nodeCheckVal = false;
                return RC_OK;
            }
        }
        i++;
    }

    return RC_IM_KEY_NOT_FOUND;
}

RC deleteIndexFromLeafNode(PageData *page, int key) {
    if (page == NULL) {
        return RC_ERROR;
    }

    if (page->numEntries <= 0 || page->numEntries > MAX_ENTRIES) {
        return RC_ERROR;
    }

    Nodes indexLeafNode;
    float newPointers[MAX_ENTRIES];
    int newKeys[MAX_ENTRIES];

    int rightIdx = 0, found = 0;

    int i = 0;
    while (i < page->numEntries) {
        if (key == page->keys[i]) {
            indexLeafNode.nodeCheckVal = true;
            found = 1;
        } else {
            indexLeafNode.headerNodeVal = found;
            newKeys[rightIdx] = page->keys[i];
            newPointers[rightIdx] = page->pointers[i];
            indexLeafNode.headerNodeVal++;
            rightIdx++;
        }
        i++;
    }
    
    if (!found) {
        return RC_IM_KEY_NOT_FOUND;
    }
    indexLeafNode.nodeVal = found;
    newPointers[rightIdx] = -1;

    page->pointers[rightIdx] = newPointers[rightIdx];
    indexLeafNode.headerNodeVal = page->numEntries;
    page->numEntries -= 1;

    i = 0;
    while (i < rightIdx) {
        page->keys[i] = newKeys[i];
        page->pointers[i] = newPointers[i];
        i++;
    }

    return RC_OK;
}

extern RC insertKey(BTreeHandle *tree, Value *key, RID rid) {
    if (tree == NULL || key == NULL) {
        return RC_ERROR;
    }

    Nodes btreeReadDatan;
    TreeMgmtData *treeMgmtData = (TreeMgmtData *)tree->mgmtData;
    if (treeMgmtData == NULL) {
        return RC_ERROR;
    }

    int rootPageNumber = treeMgmtData->fmd.rootPageNumber;
    btreeReadDatan.nodeVal = treeMgmtData->fmd.numNodes; 
    int maxEntries = treeMgmtData->fmd.maxEntriesPerPage;
    int currNumNodes = treeMgmtData->fmd.numNodes;
    btreeReadDatan.headerNodeVal = btreeReadDatan.nodeVal;

    BM_BufferPool *bm = treeMgmtData->bufferPool;
    BM_PageHandle *ph = treeMgmtData->pageHandle;
    btreeReadDatan.nodeCheckVal = true;
    if (bm == NULL || ph == NULL) {
        return RC_ERROR;
    }

    PageData rootPageData, pageToInsert;
    RC rc = readPageData(bm, ph, &rootPageData, rootPageNumber);
    if (rc != RC_OK) {
        return rc;
    }

    pageToInsert = findPageToInsertNewEntry(bm, ph, rootPageData, key->v.intV);

    rc = insertKeyInLeafNode(&pageToInsert, key->v.intV, rid);
    if (rc != RC_OK) {
        return RC_IM_KEY_ALREADY_EXISTS;
    }

    treeMgmtData->fmd.numEntries++;

    if (pageToInsert.numEntries <= maxEntries) {
        rc = writeBtreeNode(&pageToInsert);
        if (rc != RC_OK) {
            return rc;
        }
        return RC_OK;
    }

    int mid = ceil((pageToInsert.numEntries) / 2);

    int *rightChildKeys = (int *)malloc(maxEntries * sizeof(int));
    float *rightChildPointers = (float *)malloc((maxEntries + 1) * sizeof(float));
    int *leftChildKeys = (int *)malloc(maxEntries * sizeof(int));
    float *leftChildPointers = (float *)malloc((maxEntries + 1) * sizeof(float));

    if (rightChildKeys == NULL || rightChildPointers == NULL ||
        leftChildKeys == NULL || leftChildPointers == NULL) {
     
        free(rightChildKeys);
        free(rightChildPointers);
        free(leftChildKeys);
        free(leftChildPointers);
        return RC_ERROR;
    }

    int leftIdx = 0;
    int i = mid + 1;
    while (i < pageToInsert.numEntries) {
        rightChildKeys[leftIdx] = pageToInsert.keys[i];
        rightChildPointers[leftIdx] = pageToInsert.pointers[i];
        leftIdx++;
        i++;
    }
    rightChildPointers[leftIdx] = -1;

    leftIdx = 0;
    i = 0;
    while (i <= mid) {
        leftChildKeys[leftIdx] = pageToInsert.keys[i];
        leftChildPointers[leftIdx] = pageToInsert.pointers[i];
        leftIdx++;
        i++;
    }
    leftChildPointers[leftIdx] = -1;
    btreeReadDatan.nodeVal += 1;
    currNumNodes++;
    ensureCapacity(currNumNodes + 1, &treeMgmtData->fileHandle);
    if(btreeReadDatan.nodeCheckVal) {
        treeMgmtData->fmd.numNodes++;
    }
  
    PageData rightNode;
    btreeReadDatan.headerNodeVal = btreeReadDatan.nodeVal;
    updateNodeData(tree, &rightNode, 1, currNumNodes, rightChildKeys, rightChildPointers, floor((maxEntries + 1) / 2), pageToInsert.parentNode == -1 ? 3 : pageToInsert.parentNode);
    if (rc != RC_OK) {
        free(rightChildKeys);
        free(rightChildPointers);
        free(leftChildKeys);
        free(leftChildPointers);
        return rc;
    }

    rc = writeBtreeNode(&rightNode);
    if (rc != RC_OK) {
        free(rightChildKeys);
        free(rightChildPointers);
        free(leftChildKeys);
        free(leftChildPointers);
        return rc;
    }

    PageData leftNode;
    btreeReadDatan.headerNodeVal++;
    updateNodeData(tree, &leftNode, 1, pageToInsert.pageNumber, leftChildKeys, leftChildPointers, ceil((maxEntries + 1) / 2) + 1, pageToInsert.parentNode == -1 ? 3 : pageToInsert.parentNode);
    if (rc != RC_OK) {
        free(rightChildKeys);
        free(rightChildPointers);
        free(leftChildKeys);
        free(leftChildPointers);
        return rc;
    }

    rc = writeBtreeNode(&leftNode);
    if (rc != RC_OK) {
        free(rightChildKeys);
        free(rightChildPointers);
        free(leftChildKeys);
        free(leftChildPointers);
        return rc;
    }

    KeyData kd;
    kd.key = rightNode.keys[0];
    kd.left = leftNode.pageNumber;
    kd.right = rightNode.pageNumber;


    rc = updateParentNodes(tree, pageToInsert.parentNode, kd);
    if (rc != RC_OK) {
        free(rightChildKeys);
        free(rightChildPointers);
        free(leftChildKeys);
        free(leftChildPointers);
        return rc;
    }

    free(rightChildKeys);
    free(rightChildPointers);
    free(leftChildKeys);
    free(leftChildPointers);

    return RC_OK;
}

RC readPageData(BM_BufferPool *bm, BM_PageHandle *ph, PageData *pd, int pageNumber) {
    Nodes nd;
    RC rc;
    //pinPage(bm, ph, pageNumber);
    if (bm == NULL || ph == NULL) {
        return RC_ERROR;
    }

    if (pageNumber < 0) {
        return RC_ERROR;
    }
    
    rc = pinPage(bm, ph, pageNumber);
    if (rc != RC_OK) {
        return rc;
    }

    char *data = ph->data;
    char *ptr = data;
    Nodes btreeReadData;

    memcpy(pd, ptr, sizeof(PageData));
    ptr += sizeof(PageData);
    btreeReadData.nodeCheckVal = true;

    float *children = malloc((pd->numEntries + 1) * sizeof(float));
    int *keys = malloc(pd->numEntries * sizeof(int));
    btreeReadData.nodeVal = NULL;
    if (children == NULL || keys == NULL) {
        if (children != NULL) {
			free(children);
		}
        if (keys != NULL) {
			free(keys);
		}
		btreeReadData.nodeCheckVal = false;
        unpinPage(bm, ph);
        return RC_ERROR;
    }

    if (pd->numEntries > 0) {
        int i = 0;
        while (i < pd->numEntries) {
            children[i] = read_float_from_str(&ptr);
			btreeReadData.nodeVal = pageNumber;
            keys[i] = read_int_from_str(&ptr);
            i++;
        }
        children[pd->numEntries] = read_float_from_str(&ptr);
    }

    pd->keys = keys;
	nd.headerNodeVal = btreeReadData.nodeVal;
    pd->pointers = children;

    rc = unpinPage(bm, ph);
    switch (rc) {
        case RC_OK:
            break;
        default:
            if (keys != NULL) {
				free(keys);
			}
            if (children != NULL) {
				free(children);
			}
            return rc;
    }

    return RC_OK;
}


extern RC deleteKey(BTreeHandle *tree, Value *key) {
    if (tree == NULL || key == NULL) {
        return RC_ERROR;
    }

    Nodes btreeNode;
    TreeMgmtData *treeMgmtData = (TreeMgmtData *)tree->mgmtData;
    if (treeMgmtData == NULL) {
        return RC_ERROR;
    }

    int rootPageNumber = treeMgmtData->fmd.rootPageNumber;
    if (rootPageNumber < 0) {
        return RC_ERROR;
    }

    BM_BufferPool *bm = btreeMgmtData->bufferPool;
    btreeNode.nodeCheckVal = true;
    BM_PageHandle *ph = btreeMgmtData->pageHandle;
    if (bm == NULL || ph == NULL) {
        return RC_ERROR;
    }

    PageData rootPageData;
    RC error = readPageData(bm, ph, &rootPageData, rootPageNumber);
    if (error != RC_OK) {
        return error;
    }


    PageData pd = findPageToInsertNewEntry(bm, ph, rootPageData, key->v.intV);
    if(btreeNode.nodeCheckVal) {
        error = deleteIndexFromLeafNode(&pd, key->v.intV);
        if (error != RC_OK) {
            return error;
        }
    }
    
    btreeNode.headerNodeVal = rootPageData.parentNode;
    error = writeBtreeNode(&pd);
    if (error != RC_OK) {
        return error;
    }

    return RC_OK;
}

void findLeafPages(BTreeHandle *tree, PageData node, int *leafPages) {
    if (tree == NULL) {
        puts("The Tree is Empty \n");
        return;
    }
    Nodes scanMgmtDatan;
    BM_PageHandle *ph = btreeMgmtData->pageHandle;
    scanMgmtDatan.headerNodeVal = leafPages[scanCounter];
    BM_BufferPool *bm = btreeMgmtData->bufferPool;

    if (node.isLeaf) {
        scanMgmtDatan.nodeVal = scanMgmtDatan.headerNodeVal;
        leafPages[scanCounter] = node.pageNumber;
        scanMgmtDatan.headerNodeVal = 0;
        scanCounter += 1;
        return;
    }

    int i = 0;
    while (i <= node.numEntries) {
        PageData child;
        RC rc = readPageData(bm, ph, &child, (int)node.pointers[i]);
        if (rc != RC_OK) {
            return;
        }

        findLeafPages(tree, child, leafPages);
        i++;
    }
}

extern RC openTreeScan(BTreeHandle *tree, BT_ScanHandle **handle) {
    scanCounter = 0;
    if (handle == NULL || tree == NULL) {
        return RC_ERROR;
    }

    TreeMgmtData *treeMgmtData = (TreeMgmtData *)tree->mgmtData;
    Nodes scanMgmtDatan;
    int rootPageNumber = treeMgmtData->fmd.rootPageNumber;

    scanHandle = (BT_ScanHandle *)malloc(sizeof(BT_ScanHandle));
    if (scanHandle == NULL) {
        return RC_ERROR;
    }
    
    scanMgmtData = (ScanMgmtData *)malloc(sizeof(ScanMgmtData));
    if (scanMgmtData == NULL) {
        free(scanHandle);
        return RC_ERROR;
    }

    BM_BufferPool *bm = treeMgmtData->bufferPool;
    BM_PageHandle *ph = treeMgmtData->pageHandle;

    PageData rootPageData;
    RC rc = readPageData(bm, ph, &rootPageData, rootPageNumber);
    if (rc != RC_OK) {
        free(scanHandle);
        free(scanMgmtData);
        return rc;
    }

    int *leafPageNumbers = (int *)malloc(100 * sizeof(int));
    if (leafPageNumbers == NULL) {
        free(scanHandle);
        free(scanMgmtData);
        return RC_ERROR;
    }

    findLeafPages(tree, rootPageData, leafPageNumbers);

    PageData leafPage;
    rc = readPageData(bm, ph, &leafPage, leafPageNumbers[0]);
    if (rc != RC_OK) {
        free(scanHandle);
        free(scanMgmtData);
        free(leafPageNumbers);
        return rc;
    }

    scanMgmtData->leafPages = leafPageNumbers;
    scanMgmtDatan.nodeVal = leafPageNumbers[0];
    scanMgmtData->currentPage = leafPageNumbers[0];
    scanMgmtData->currentPageData = leafPage;
    scanMgmtDatan.nodeCheckVal = true;
    scanMgmtData->numOfLeafPages = scanCounter;
    scanMgmtData->currentPosition = 0;
    scanMgmtDatan.headerNodeVal = scanMgmtData->currentPosition;
    scanMgmtData->nextPagePosInLeafPages = 1;

    scanHandle->mgmtData = scanMgmtData;
    scanMgmtDatan.nodeVal += 1;
    scanHandle->tree = tree;
    scanMgmtDatan.headerNodeVal = scanMgmtDatan.nodeVal;
    *handle = scanHandle;
    return RC_OK;
}

RC readNextPage(BT_ScanHandle *handle) {
    if (handle == NULL) {
        puts("Next Page not available.\n");
        return RC_ERROR;
    }
    ScanMgmtData *scanMgmtData = handle->mgmtData;
    int val = scanMgmtData->nextPagePosInLeafPages;
    if ( val == -1)
        return RC_IM_NO_MORE_ENTRIES;

    Nodes nd;
    TreeMgmtData *treeMgmtData = ((TreeMgmtData *)handle->tree->mgmtData);
    BM_BufferPool *bm = treeMgmtData->bufferPool;
    nd.nodeVal += 1;
    BM_PageHandle *ph = treeMgmtData->pageHandle;

    int nextPos = scanMgmtData->nextPagePosInLeafPages;
    nd.nodeCheckVal = true;
    scanMgmtData->currentPage = scanMgmtData->leafPages[nextPos];

    scanMgmtData->nextPagePosInLeafPages += 1;

    if(nd.nodeCheckVal) {
        PageData leafPage;
        readPageData(bm, ph, &leafPage, scanMgmtData->currentPage);
        nd.nodeVal = scanMgmtData->nextPagePosInLeafPages;
        scanMgmtData->currentPageData = leafPage;
        scanMgmtData->currentPosition = 0;

    }

    if(nd.nodeCheckVal) {
        if (scanMgmtData->nextPagePosInLeafPages >= scanMgmtData->numOfLeafPages) {
            scanMgmtData->nextPagePosInLeafPages = -1;
            nd.headerNodeVal = scanMgmtData->nextPagePosInLeafPages;
        }
    }



    return RC_OK;
}

extern RC nextEntry(BT_ScanHandle *handle, RID *result) {
    if (handle == NULL || handle->mgmtData == NULL) {
        return RC_ERROR;
    }
	ScanMgmtData *scanMgmtData = handle->mgmtData;

    int currPosInPage = scanMgmtData->currentPosition;
    int maxEntriesInPage = scanMgmtData->currentPageData.numEntries;

    if (currPosInPage == maxEntriesInPage) {
        RC rc = readNextPage(handle);
        if (rc != RC_OK) {
            return rc;
        }
        currPosInPage = 0;
    }

    fetchIDPtr(
        scanMgmtData->currentPageData.pointers[currPosInPage],
        &result->page, &result->slot);
    check = check + 1;
    scanMgmtData->currentPosition += 1;

    return RC_OK;
}

extern RC closeTreeScan(BT_ScanHandle *handle) {
    if (handle == NULL) {
        return RC_ERROR;
    }

    if (handle->mgmtData != NULL) {
        free(handle->mgmtData);
    }
    check++;
    free(handle);

    return RC_OK;
}

void printLeafNode(PageData root) {
    puts("LeafNode{ ");
    for (int i = 0; i < root.numEntries; i++) {
        printf("%d ", root.keys[i]);
    }
    puts("}");
}


void printInternalNode(BM_BufferPool *bm, BM_PageHandle *ph, PageData root) {
    puts("Node{ ");
    for (int i = 0; i < root.numEntries; i++) {
        PageData pageData;
        Nodes pageNode;
        int pageNumber = (int)root.pointers[i];
        pageData.pageNumber = pageNumber;
        pageNode.nodeVal = pageNode.nodeVal + pageNumber; 
        readPageData(bm, ph, &pageData, pageNumber);
        printNodeContent(bm, ph, pageData);
        pageNode.nodeCheckVal = false;
        puts(" ");
    }

    Nodes lastNode;
    PageData lastPageData;
    int lastPageNumber = (int)root.pointers[root.numEntries];
    lastNode.headerNodeVal = 0;
    lastPageData.pageNumber = lastPageNumber;
    readPageData(bm, ph, &lastPageData, lastPageNumber);
    lastNode.nodeCheckVal = false;
    printNodeContent(bm, ph, lastPageData);
    
    puts("}");
}


void printNodeContent(BM_BufferPool *bm, BM_PageHandle *ph, PageData root) {
    if (root.isLeaf) {
        printLeafNode(root);
    } else {
        printInternalNode(bm, ph, root);
    }
}

extern char *printTree (BTreeHandle *tree){
    if (tree == NULL || tree->mgmtData == NULL) {
        puts("Invalid tree handle or management data.\n");
        return NULL;
    }

    TreeMgmtData *treeMgmtData = tree->mgmtData;
    Nodes node;
    int rootPageNumber = treeMgmtData->fmd.rootPageNumber;

    if (treeMgmtData->bufferPool == NULL || treeMgmtData->pageHandle == NULL) {
        puts("Buffer pool or page handle not initialized.\n");
        return NULL;
    }

    BM_BufferPool *bm = btreeMgmtData->bufferPool;
    node.nodeVal = node.nodeVal+1;
    BM_PageHandle *ph = btreeMgmtData->pageHandle;

    PageData rootPageData;
    readPageData(bm, ph, &rootPageData, rootPageNumber);
    node.nodeCheckVal = true;
    printNodeContent(bm, ph, rootPageData);
	
	return NULL;
}
