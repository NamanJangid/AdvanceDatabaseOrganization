Advanced Database Organization(CS525) Assignment 4: B+ Tree 

Group Number: cs525-SP24-group#10

Name: Pranjali Deshmukh  		CWID: A20527773    Email: pdeshmukh3@hawk.iit.edu
Name: Naman Rajendra Jangid 	CWID: A20527772    Email: njangid@hawk.iit.edu

/************************************************************************/
SYNOPSIS:
/************************************************************************/
Implementing B+Tree Functions  

/************************************************************************/
Step by step installation and execution For Linux:
/************************************************************************/

a. Download the repository from git and unzip it.
b. Open 'assign4' folder in VS code(or any IDE). 
c. Make sure you have gcc and make install on your system. (commands : 'sudo apt install make' and 'sudo apt install gcc') 
d. Open Terminal and run command 'make clean'
e. Run make command.
f. On successful run, execute the test file using command './test1'

/************************************************************************/
FUNCTIONS USED: 
/************************************************************************/

1. extern RC initIndexManager (void *mgmtData);
	The initIndexManager ensure the intialization of the index manager and runs a call for storage manager and record manager init functions. 

2. extern RC shutdownIndexManager ();
	The shutdownIndexManager is used to clear all memory allocated to resources used in the code and setting the index manager to NULL and free up its memory. 
	

3. extern RC closeBtree (BTreeHandle *tree);
	The closeBtree flags every page to dirty and shuts downs the buffer manager clearing up all the allocated memory.
	Its also closes B+tree and stores any changes that were made to the disk.
	
	
4. extern RC createBtree (char *idxId, DataType keyType, int n);
	In createBtree function helps assigning an empty initial resources their memory according to size of their handlers. It initializes the buffer manager and also creates a page. 


5. extern RC openBtree (BTreeHandle **tree, char *idxId);
	The openBtree creates a file on the existing B+tree on the location given by the 'idxId'
	This function is used to open a B+tree that has been created and saved on disk before. It needs imputs like the B+tree's order and where the data files are stored on the disk. Then, it reads the data from the files, sets up the B+ree's data structures in memory, and gives back the B+tree object.


6. extern RC deleteBtree (char *idxId);
	This functions destroy's all the resources associated with B+tree and file name 'idxId' is deleted and also all existing directories along side of the B+tree.


7. extern RC getNumEntries (BTreeHandle *tree, int *result);
	The getNumEntries functionality extracts all the number of entries along with records and keys of existing B+Tree. It then saves it into the TreeManager. 


8. extern RC getNumNodes (BTreeHandle *tree, int *result);
	Its a function that returns the total of nodes associated with B+tree and also safes to TreeManager.

9. extern RC getKeyType (BTreeHandle *tree, DataType *result);
	The getKeyType stores the KeyType of the B+tree existing in the disk and returns it back data.

10. extern RC findKey (BTreeHandle *tree, Value *key, RID *result);
	The findkey searches for the Key using the location and value of the key provided. It also calls for the readPage method of storage manager.

11. extern RC insertKey (BTreeHandle *tree, Value *key, RID rid);
	The function here inserts the key given as input into the existing B+tree or create a new tree if its not created. This function first search for the key in the existing tree if it exists it returns a error message RC_IM_KEY_ALREADY_EXISTS, else it search for the space in the leaf node and inserts it or rearranges the tree if leaf node is full.

12. extern RC openTreeScan (BTreeHandle *tree, BT_ScanHandle **handle);	
	The fucntions initialize the scanning of the B+tree. If the B+tree has root node equal to NULL, it returns and error message.

13. extern RC deleteKey (BTreeHandle *tree, Value *key);
	The function deletes the particular key that is mentioned along with its entery it deallocates its value from the B+tree
	
14. extern RC closeTreeScan (BT_ScanHandle *handle);
	

15. extern RC nextEntry (BT_ScanHandle *handle, RID *result);
	The functions scans the B+tree entirely and if reachs end of the Tree and if there are no more entires it returns error RC_IM_NO_MORE_ENTRIES.

16. extern char *printTree (BTreeHandle *tree);
	The function prints the output of formed B+tree in the code and returns it, it transvers through each node and pointer and prints it out.
	