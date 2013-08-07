#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <compress.h>

/*********
 * STRUCTS
 * -------
*********/

typedef struct LinkedList {
	void *elem; //TreeNode Pointer
	void *next; //LinkedList pointer
} LinkedList;

typedef struct TreeNode {
	int character;
	int frequency;
	void *child1; //TreeNode Pointer
	void *child2; //TreeNode Pointer
} TreeNode;

typedef struct Bits {
	unsigned int bits;
	char len;
} Bits;

typedef struct StreamBuffer {
	unsigned int bits; //Set of bits
	char len; //Number of bits in buffer
} StreamBuffer;

/**************************
 * SHARED UTILITY FUNCTIONS
 * ------------------------
 * Functions used by both
 * compress and decompress.
**************************/

int log_2(int i) {
	int len = 0;
	while (i >>= 1) len++;
	return len;
}

int *initFrequencies() {
	int *frequencies = malloc(257 * sizeof(int));
	
	//Initialize the array of characters to count
	for (int i = 0; i < 256; i++)
		frequencies[i] = 0;
	frequencies[256] = 1; //One additional EOF character
	
	return frequencies;
}

StreamBuffer *initStreamBuffer(){
	StreamBuffer *sb = malloc(sizeof(StreamBuffer));
	sb->bits = 0;
	sb->len = 0;
	return sb;
}

Bits *initBitsMap() {
	Bits *charMap = malloc(257 * sizeof(Bits));
	for (int i = 0; i < FREQ_TABLE_SZ; i++) {
		charMap[i].bits = -1;
		charMap[i].len = 0;
	}
	return charMap;
}

/***************************************
 * COMPRESSION PROGRAM UTILITY FUNCTIONS
 * -------------------------------------
 * These utility functions are used by
 * the compression algorithm
***************************************/

TreeNode *createTreeNode(int ch, int freq, void *child1, void *child2) {
	TreeNode *new_node = malloc(sizeof(TreeNode));
	new_node->character = ch;
	new_node->frequency = freq;
	new_node->child1 = child1;
	new_node->child2 = child2;
	return new_node;
}

void freeTreeNodes(TreeNode *root) {
	if (root->child1 != NULL) freeTreeNodes(root->child1);
	if (root->child2 != NULL) freeTreeNodes(root->child2);
	free(root);
}

//Inserts in order of increasing value
LinkedList *insertIntoLinkedList(LinkedList *first, LinkedList *elem, int value) {
	
	if (first == NULL) return elem;
	
	LinkedList *prev = NULL;
	LinkedList *search;
	for (search = first; search != NULL; search = search->next) {
		TreeNode *curr = (TreeNode *)search->elem;
		if (value < curr->frequency) {
			elem->next = search;
			if (prev != NULL) {
				prev->next = elem;
				return first;
			}
			return elem;
		}
		prev = search;
	}
	prev->next = elem;
	return first;
}

void writeBit(StreamBuffer *sb, char bit) {
	
	sb->bits <<= 1;
	if (bit) sb->bits += 1;
	sb->len += 1;
	if (sb->len == 8) {
		putchar((char)sb->bits);
		sb->bits = 0;
		sb->len = 0;
	}
}

void flushStream(StreamBuffer *sb) {
	for (int i = 0; i < 7; i++) writeBit(sb,0);
}

void printBits(StreamBuffer *sb, Bits *b) {
	for (int i = b->len-1; i >= 0; i--) {
		writeBit(sb,b->bits&(1<<i));
	}
}

void printChar(StreamBuffer *sb, int ch) {
	for (int i = 128; i > 0; i>>=1) writeBit(sb, ch&i);
}

void printHeaderFreqEncoding(StreamBuffer *sb, int frequency) {
	int len = log_2(frequency);
	for (int i = 16; i > 0; i>>=1) writeBit(sb,len&i);
	for (int i = 1<<len; i > 0; i>>=1) writeBit(sb,frequency&i);
}

/*************************************
 * COMPRESSION PROGRAM LOGIC FUNCTIONS
 * -----------------------------------
 * Each is called once in order during
 * execution of the compress algorithm
*************************************/

int countNumUsedChars(int *frequencies) {
	int num = 0;
	for (int i = 0; i < 256; i++) {
		if (frequencies[i] != 0) num++;
	}
	return num;
}

int *countCharFrequencies(FILE *fp) {
	
	int *frequencies = initFrequencies();
	
	//Iterate through the file once characterwise
	char ch;
	while ((ch = fgetc(fp)) != EOF)
		frequencies[ch]++;
	
	//Close file and return
	fclose(fp);
	return frequencies;
}

LinkedList *buildLinkedList(int *frequencies) {
	
	//Init the list
	LinkedList *linkedList = NULL;
	
	//Loop over all the characters used in the file and add them to the list
	for (int i = 0; i < 257; i++) {
		if (frequencies[i] != 0) {
			
			//Initialize a new linked list element
			LinkedList *new_elem = malloc(sizeof(LinkedList));
			new_elem->elem = (void*) createTreeNode(i, frequencies[i], NULL, NULL);
			new_elem->next = NULL;
			
			//Inserts the elem into the list IN ORDER OF INCREASING FREQUENCY
			linkedList = insertIntoLinkedList(linkedList, new_elem, frequencies[i]);
		}
	}
	return linkedList;
}

TreeNode *buildBinaryTree(LinkedList *linkedList) {
	
	//Loop until exactly one element remains in the linked list
	while (linkedList->next != NULL) {
		
		LinkedList *first = linkedList;
		LinkedList *second = (LinkedList *)linkedList->next;
		
		TreeNode *child1 = (TreeNode *) first->elem;
		TreeNode *child2 = (TreeNode *) second->elem;
		
		int new_frequency = child1->frequency + child2->frequency;
		TreeNode *combinedNode = createTreeNode(NO_CHARACTER, new_frequency, child1, child2);
		
		free(linkedList);
		linkedList = second->next;
	
		//Repurpose the second linked list elem for our combined node and reinsert it
		second->elem = combinedNode;
		second->next = NULL;
		linkedList = insertIntoLinkedList(linkedList, second, new_frequency);
	}
	
	//Extract the root TreeNode from the shrunk linked list
	TreeNode *root = (TreeNode *)linkedList->elem;
	free(linkedList);
	return root;
}

void buildBitsMapping(Bits *charMap, TreeNode *treeNode, int bits, int depth) {
	
	//Base case, we reach a character
	if (treeNode->character != NO_CHARACTER) {
		charMap[treeNode->character].bits = bits;
		charMap[treeNode->character].len = depth;
		return;
	}
	
	//Bitshift and recursive call
	bits <<= 1;
	if (treeNode->child1 != NULL)
		buildBitsMapping(charMap, treeNode->child1, bits, depth+1);
	if (treeNode->child2 != NULL)
		buildBitsMapping(charMap, treeNode->child2, bits+1, depth+1);
}

void printCompressionHeader(StreamBuffer *sb, int *frequencies) {
	
	//The number of characters we will print to the header file
	printChar(sb, countNumUsedChars(frequencies));
	
	//The encoding data for the rest of the used characters
	for (int i = 0; i < 256; i++) {
		if (frequencies[i] != 0) {
			printChar(sb, i);
			printHeaderFreqEncoding(sb, frequencies[i]);
		}
	}
}

void printCompressedFile(StreamBuffer *sb, const char *infile, Bits *charMap) {
	
	//Open the file a second time
	FILE *fp = fopen(infile,"r");
	
	//Print the encoding for each character in the file
	char ch;
	while ((ch = fgetc(fp)) != EOF) printBits(sb,&charMap[ch]);
	printBits(sb,&charMap[EOF_CHARACTER]);
	flushStream(sb); //Clear out the last few bits
	
	//Close the file
	fclose(fp);
}

/**
 * compressFile: Given a string infile and 
*/
int compressFile(const char *infile, FILE *outfile) {
	
	//Open file
	FILE *fp = fopen(infile,"r");
	if (fp == NULL) return 1;
	
	//Generate frequencies
	int *frequencies = countCharFrequencies(fp);
	
	//Build the linked list of tree nodes from the frequency table
	LinkedList *linkedList = buildLinkedList(frequencies);
	
	//Shrink the linked list into a tree
	TreeNode *root = buildBinaryTree(linkedList); //And frees the linked list
	
	//charMap[CHARACTER] = BITS (stored in an int)
	Bits *charMap = initBitsMap();
	
	//A recursive algorithm to traverse the tree
	buildBitsMapping(charMap, root, 0, 0);
	freeTreeNodes(root);
	
	//Used to print individual bits
	StreamBuffer *sb = initStreamBuffer();
	printCompressionHeader(sb, frequencies);
	printCompressedFile(sb, infile, charMap);
	
	//I clean up my memory :)
	free(charMap);
	return 0;
}

/*****************************************
 * DECOMPRESSION PROGRAM UTILITY FUNCTIONS
 * ---------------------------------------
 * These utility functions are used by the
 * decompression algorithm
*****************************************/

int readBit(FILE *infile, StreamBuffer *sb) {
	if (sb->len == 0) {
		if ((sb->bits = fgetc(infile)) == EOF) return -1;
		sb->len = 8;
	}
	sb->len--;
	if ((sb->bits & (1 << sb->len)) != 0) return 1;
	else return 0;
}

int readNBits(FILE *infile, StreamBuffer *sb, int n) {
	int bit;
	int bits = 0;
	for (int i = 0; i < n; i++) {
		bit = readBit(infile, sb);
		if (bit == -1) return -1;
		bits <<= 1;
		if (bit) bits++;
	}
	return bits;
}

// typedef struct TreeNode {
// 	int character;
// 	int frequency;
// 	void *child1; //TreeNode Pointer
// 	void *child2; //TreeNode Pointer
// } TreeNode;

/***************************************
 * DECOMPRESSION PROGRAM LOGIC FUNCTIONS
 * -------------------------------------
 * Each function is called once in order
 * during the decompress algorithm
***************************************/

int *readFileHeader(FILE *infile, StreamBuffer *sb) {
	
	int *frequencies = initFrequencies();
	int num_header_chars = fgetc(infile);
	
	for (int i = 0; i < num_header_chars; i++) {
		int character = readNBits(infile, sb, 8);
		int size = readNBits(infile, sb, 5);
		frequencies[character] = readNBits(infile, sb, size+1);
	}
	
	return frequencies;
}

void printDecompressedFile(StreamBuffer *sb, FILE *infile, TreeNode *root) {
	
	int ch;
	TreeNode *curr = root;
	while ((ch = readBit(infile, sb)) != -1) {
		
		//Progress throught the tree
		if (ch) curr = curr->child2;
		else curr = curr->child1;
		
		//Print if we reach a character
		if (curr->character != NO_CHARACTER) {
			if (curr->character == EOF_CHARACTER) return;
			putchar(curr->character);
			curr = root;
		}
	}
	printf("Error: corrupted file!");
}

int decompressFile(const char *infile, FILE *outfile) {
	
	//Open file
	FILE *fp = fopen(infile,"r");
	if (fp == NULL) return 1;
	
	//Read in the header to the frequencies array
	StreamBuffer *sb = initStreamBuffer();
	
	int *frequencies = readFileHeader(fp, sb);
	
	//Build the linked list of tree nodes from the frequency table
	LinkedList *linkedList = buildLinkedList(frequencies);
	
	//Shrink the linked list into a tree
	TreeNode *root = buildBinaryTree(linkedList); //And frees the linked list

	printDecompressedFile(sb, fp, root);
	
	free(frequencies);
	freeTreeNodes(root);
	
	return 0;
}
