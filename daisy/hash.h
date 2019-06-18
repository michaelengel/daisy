/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

/* HASH_TABLE_SIZE is assumed to be a power of 2 for efficiency */
#define HASH_TABLE_SIZE			16384
#define HASH_TABLE_LOG			14
#define HASH_TABLE_ENTRIES		(16*HASH_TABLE_SIZE)

typedef struct _hash_entry {
   unsigned	      addr;
   unsigned	      xlated;
   struct _hash_entry *next;
} HASH_ENTRY;

