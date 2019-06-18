/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#ifndef H_translate
#define H_translate

/* PowerPC XCoff Program Sections */
#define DECL_SECT			0
#define TEXT_SECT			1
#define TOC_SECT			2
#define DATA_SECT			3
#define BSS_SECT			4
#define VLIW_SECT			5
#define VLIW_PPC_SECT			6
#define GLUE_CODE_SECT			7
#define VLIW_DESC_SECT			8
#define NUM_SECTIONS			9

/* Classes that a variable name may have */
#define NULL_CLASS		0
#define RW			1
#define BS			2
#define RO			3
#define PR			4
#define DS			5
#define TOC			6
#define TC			7
#define TC0			8
#define LAB			9
#define NUM_VAR_CLASSES		10

/* Types of Relocations */
#define DATA_RELOC		0
#define TOC_RELOC		1		/* (RELOC_TOC ... */
#define BRANCH_RELOC		2
#define LABEL_RELOC		3
#define TOCTARG_RELOC		4		/* (RELOC $TOC$_<varname> ...*/

/* For referring to TOC addresses */
#define TOCNAME_SUFFIX		".toc"

typedef struct {
   int  cnt;
   int  num_alloc;
   char **list;
} OUTLIST;

typedef struct {
   int  class;
   int  suffix;		/* Must distinguish static DEFCON and DEFSYM's. */ 
			/* DEFCON's keep suffix, DEFSYM's don't.	*/
   int  is_static;
   char *basename;
} VARNAME;

typedef struct {
   int  index;
   int  section;
} LOCN;

typedef struct {
   int  curr_addr;
   int  m_image_size;
   char *hiername;
   LOCN *m_image;
} HIER;

char *handle_operation (void);
char *handle_plus_minus (int);

char *get_string_at_addr (char *, unsigned, int *);

#endif
