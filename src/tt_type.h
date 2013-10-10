/*
    This file is part of DIANA program (DIagram ANAlyser) $Revision: 2.36 $.
    Copyright (C) Mikhail Tentyukov <tentukov@physik.uni-bielefeld.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

/*tt_type.h*/
#ifndef TT_TYPE_H
#define TT_TYPE_H

/*Idetifiers for "op" parameters for tt_gate.c::l_pTOPOL2singletop and
  tt_topol2singletop, use them ORed e.g. TT_NEL|TT_NIL|TT_NV|TT_EXT*/
/*Filed nel will be copied:*/
#define TT_NEL 1
 /*Filed nil will be copied:*/
#define TT_NIL 2
/*Filed nv will be copied:*/
#define TT_NV 4
/*extTopol will be copied:*/
#define TT_EXT 8
/*redTopol will be copied:*/
#define TT_RED 16
/*usrTopol will be copied:*/
#define TT_USR 32
/*Filed nmomenta will be copied:*/
#define TT_NMOMENTA 64
/*momenta will be copied:*/
#define TT_MOMENTA 128
/* extMomenta will be copied:*/
#define TT_EXT_MOMENTA 256
/*external coordinates will be copied:*/
#define TT_EXT_COORD 512
/*internal coordinates will be copied:*/
#define TT_INT_COORD 1024
/*v_red2usr,l_red2usr,v_usr2red,l_usr2red will be copied:*/
#define TT_S_RED_USR 2048
/*v_usr2nusr,v_nusr2usr,l_usr2nusr,l_nusr2usr will be INITIALIZED by identities:*/
#define TT_S_USR_NUSR 4096
/*l_dirUsr2Red will be copied:*/
#define TT_D_USR_RED 8192
/*l_dirUsr2Nusr will be INITIALIZED by identity:*/
#define TT_D_USR_NUSR 16384
/*Both remarks and top_remarks will be copied:*/
#define TT_REM 32768
/*Name will be copied:*/
#define TT_NAME 65536

#define TT_TOKENTOPOLOGY "topology"
#define TT_TOKENTMAX "numberoftopologies"
#define TT_TOKENTMAXTRANS "numberoftokens"
#define TT_TOKENCOORDINATES "coordinates"
#define TT_TOKENEV "ev"
#define TT_TOKENEVL "evl"
#define TT_TOKENIV "iv"
#define TT_TOKENIVL "ivl"
#define TT_TOKENEL "el"
#define TT_TOKENELL "ell"
#define TT_TOKENIL "il"
#define TT_TOKENILL "ill"
#define TT_TOKENTRANS "token"
#define TT_TOKENREMARKS "remarks"
#define TT_TOKENMoS "momentaorshape"
#define TT_ONLYMOMENTA "momenta"
#define TT_ONLYSHAPE "shape"
/*#define TT_BOTHMOMENTAANDSHAPE "both"*/

/*Default hash table size for reduced topologies in the table.
  It will be assigned to
  unsigned int g_defaultHashTableSize
  (see below). It is better to set it to some prime number.*/
#define DEFAULT_HASH_SIZE 809

/*Default hash table size for translation tokens.
  It will be assigned to
  unsigned int g_defaultTokenDictSize
  (see below). It is better to set it to some prime number.*/
#define TT_DICTSIZE 17

/*Identifiers for tt_getCoords. Important -- smth+1= ORIG_smth!
  Meanwhile, smth may be used as a bitmask!*/
#define EXT_VERT 1
#define EXT_VLBL 4
#define EXT_LINE 8
#define EXT_LLBL 16
#define INT_VERT 32
#define INT_VLBL 64
#define INT_LINE 128
#define INT_LLBL 256
#define ORIG_EXT_VERT 2
#define ORIG_EXT_VLBL 5
#define ORIG_EXT_LINE 9
#define ORIG_EXT_LLBL 17
#define ORIG_INT_VERT 33
#define ORIG_INT_VLBL 65
#define ORIG_INT_LINE 129
#define ORIG_INT_LLBL 257

/*Error codes:*/
/*  ATTENTION!
    File macro.c, function macttErrorCodes
    uses these codes! Correct this function when codes are added/removed!*/
/*Unspecified non-fatal error:*/
#define TT_NONFATAL -1
/*Unspecified fatal error:*/
#define TT_FATAL -2
/*Not enought memory:*/
#define TT_NOTMEM -3
/*Invalid topology*/
#define TT_INVALID -4
/*Not found by lookup:*/
#define TT_NOTFOUND -5
/* Removed:*/
/*Attempt to change maximal number of topologies during loading a table:*/
/* #define TT_DBL_MAXTOP -6*/
/*Unexpected end of file:*/
#define TT_U_EOF -7
/*Too many topologies:*/
#define TT_TOOMANYTOPS -8
/*Double defined topology name*/
#define TT_DOUBLEDEFNAM -9
/*Token conversion error (e.g. non-number instead of an expected  number) */
#define TT_FORMAT -10
/*Empty tables not allowed:*/
#define TT_EMPTYTABLE -11
/*Can't initialize a scanner:*/
#define TT_CANTINITSCAN -12
/*Call getToken without initializing of a scanner:*/
#define TT_SCANNER_NOTINIT -13
/*Two topologies in the table are coincide after reduction:*/
#define TT_DOUBLETOPS -14
/*Too many momenta sets:*/
#define TT_TOOMANYMOMS -15
/*Too long string:*/
#define TT_TOOLONGSTRING -16
/*Double defined translation token:*/
#define TT_DOUBLEDEFTOKEN -17
/*Undefined topology name:*/
#define TT_TOPUNDEF -18
/*Invalid substitution:*/
#define TT_INVALIDSUBS -19
/*Error codes returned by readTopolRemarksFromScanner:*/
/*Double defined name:*/
#define TT_REM_DOUBLEDEF -20
/*Empty name is not allowed:*/
#define TT_REM_EMPTYNAME -21
/*Unexpected '=':*/
#define TT_REM_UNEXPECTED_EQ -22
/*Can't write to disk:*/
#define TT_CANNOT_WRITE -23

/*Type used as integer for lines/vertex accounting:*/
#define TT_INT char
/*Default maximal number of topologies:*/
#define TT_MAX_TOP_N 1009

#ifndef MTRACE_DEBUG
#define NEWSTR new_str
#else
#define NEWSTR new_str_function
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TT_LOAD_C
/*Default hash table size for reduced topologies in the table.
  It is set to DEFAULT_HASH_SIZE (see above) and may be changed in the program.
  It is better to set it to some prime number.*/
extern unsigned int g_defaultHashTableSize;

/*Default hash table size for translation tokens.
  It is set to TT_DICTSIZE (see above) and may be changed in the program.
    It is better to set it to some prime number.*/
extern unsigned int g_defaultTokenDictSize;
             /*tt_newVector is some external function to store a vector,if needed.
              It is initialised by NULL, but the user may provide his non-trivial
              function void tt_newVector(long n, char *vec). */
extern void (*tt_newVector)(long n, char *vec);
extern char * tt_tokenTopology;
extern char * tt_tokenTMax;
extern char * tt_tokenTrMax;
extern char * tt_tokenCoordinates;
extern char * tt_tokenEv;
extern char * tt_tokenEvl;
extern char * tt_tokenIv;
extern char * tt_tokenIvl;
extern char * tt_tokenEl;
extern char * tt_tokenEll;
extern char * tt_tokenIl;
extern char * tt_tokenIll;
extern char * tt_tokenTrans;
extern char * tt_tokenRemarks;
#endif

typedef struct {
   TT_INT from,to;
}tt_line_type;

typedef struct { /*Data describing the single topology*/
   char *name;/*The unique name of the topology*/
   long n;/*index number, strting from 0! NOT a user number!*/
   long duty;/*service field, not used by tt itself.*/
   TT_INT nel,/*number of external lines*/
          nil,/*number of internal lines*/
          nv;/*number of vertices*/
   tt_line_type *extPart,/*external part of incoming topology -- never changed!*/
                *usrTopol,/*incoming topology*/
                *redTopol;/*reduced/canonical topology*/
   /*
      i_line:    [0]   [1]   [2]   [3]
                (0,0) (1,3) (1,4) (2,5)
      redTopol:        [0]   [1]   [2]
                      (1,3) (1,4) (2,5)

      e_line:    [0]    [1]    [2]    [3]
                (0,0) (-1,1) (-2,2) (-3,1)
      extPart:          [0]    [1]    [2]
                      (-1,1) (-2,2) (-3,1)
   */
   TT_INT nmomenta;/* total number of momenta sets */
   char *** momenta,   /*usr momenta */
        *** extMomenta;/*External usr momenta */

           /*ATTENTION! External coordinates are stored in reverse (against topol) order!*/
   double *ext_vert,/*coordinates of external vertices*/
          *ext_vlbl,/*coordinates of external vertices labels*/
          *ext_line,/*coordinates of external lines*/
          *ext_llbl,/*coordinates of external lines labels*/

          *int_vert,/*coordinates of internal  vertices*/
          *int_vlbl,/*coordinates of internal vertices labels*/
          *int_line,/*coordinates of internal lines*/
          *int_llbl;/*coordinates of internal lines labels*/
   /* Substitutions here:*/
   TT_INT *v_red2usr,/*red2usr[red]=usr*/
         *l_red2usr,/*red2usr[red]=usr*/
         *v_usr2red,/*usr2red[usr]=red*/
         *l_usr2red,/*usr2red[usr]=red*/
         *v_usr2nusr,/*usr2nusr[usr]=nusr*/
         *v_nusr2usr,/*nusr2usr[nusr]=usr*/
         *l_usr2nusr,/*usr2nusr[usr]=nusr*/
         *l_nusr2usr;/*nusr2usr[nusr]=usr*/
   /*Directions here:*/
   char *l_dirUsr2Red,/*l_dirUsr2Red[usr]:  usr<->red*/
         *l_dirUsr2Nusr;/*l_dirUsr2Nusr[usr]: usr<->nusr*/

   /*Note, REMARK is defined in types.h:*/
   REMARK *remarks;/*Array of remarks*/
   word top_remarks;/*Number of allocated cells.*/
   /*REMARK is defined in types.h as follows:
   Remarks are arbitrary texts associated with current topology.
   Since we are not going to use it extensively, this is organized
   as a linear (re-allocatable) array of such pairs:
   struct remark_struct{
      char *name;// name of the remark
      char *text;//  contents of the remark
   };
   Free cell, if the "name" field is NULL.*/
}tt_singletopol_type;

typedef struct{
   long totalN;/*size of a table*/
   long extN;/*number of full topologies*/
   long intN;/*number of purely internal topologies*/
   int momentaOrShape;/*0 (default)--both, >0 -- only momenta, <0 -- only shape*/
   HASH_TABLE hred;/*hash table of rduced/canonical topologies*/
   HASH_TABLE htrans;/* translation dictionary (keys=old tokens, vals=new tokens*/
   tt_singletopol_type **topols;/* topologies...*/
}tt_table_type;

/**********************************************************************************/
/*Function prototypes:*/

/*Creates new table and inserts it into the array tt_table, If tokenTableSize>0,
  initializes the token table. If topolTableSize>0, initializes the reduced topology
  table:*/
int tt_createNewTable(unsigned int tokenTableSize, unsigned int topolTableSize);

/*Adds top to the specified table. If top->redTopol!=null, neither reduces topology nor
  makes substitutions, i.e. stores them "as is".
  if deep != 0 then create new copy before storing. If deep==0 and top->redTopol == NULL,
  it returns TT_NONFATAL*/
long tt_addToTable(int table, tt_singletopol_type *top, int deep);

/* adds topology "topol" from a table "fromTable" to the end of the table "toTable".
   If addTransTbl!=0, tries to add translations from "fromTable" to "toTable".
   Returns the index of added topology in  the table  "toTable", or error code <0:*/
long tt_appendToTable(int fromTable, long topol,int toTable, int addTransTbl);

/*Allocates and fills up array '*buf' by a successive indicex of all
  existing tables, and returns the number of tables placed into 'buf':*/
int tt_getAllTables(int **buf);

/*Loads table form the named file:*/
int tt_loadTable(char *fname);

/*Free table number n; returns a number of elements stored in the table, or error <0:*/
/*n starts from 1, not from 0!:*/
long tt_deleteTable(int n);

/* Returns number of topologies in the table, or TT_NOTFOUND:*/
long tt_howmany(int table);

/*Returns original (incoming) topology:*/
char *tt_origtopology(char *buf,int table, long top);

/*Returns reduced topology:*/
char *tt_redtopology(char *buf,int table, long top);

/*Returns number of internal lines:*/
int tt_ninternal(int table, long top);

/*Returns number of external lines:*/
int tt_nexternal(int table, long top);

/*Returns number of vertices:*/
int tt_vertex(int table, long top);

/*Low level function, looks up the topology:*/
long tt_lookup(int table, tt_singletopol_type *topology);

/*Reduces the topology top and looks it up in the table:*/
long tt_lookupFromStr(int table, char *top);

/*Returns number of momenta set:*/
int tt_nmomenta(int table, long top);

char *tt_momenta(char *buf,int kind, int table, long top,int n,int *err);
char *tt_getMomenta(int isOrig,int table, long top,int n,int *err);
TT_INT *tt_l_usr2red(int table, long top);
TT_INT *tt_v_usr2red(int table, long top);
TT_INT *tt_l_dirUsr2Red(int table, long top);

/*returns 1 if it stores new token (i.e., if such a token is absent in the table),
  TT_NOTFOUND if there is no such a table, 0 if it replaces existing token:*/
int tt_translate(char *token, char *translation,int table);

/*Looks up the token in the translation table and returns it.
  WARNING: it returns a pointe to the table, it belongs to the table,
  do not touch it!*/
char *tt_lookupTranslation(char *token, int table);

/*Sets new separator for line/vertex substitutions:*/
void tt_setSep(char sep);

/*Returns current separator for line/vertex substitutions:*/
char tt_getSep(void);

/*Reorders internal lines enumerating. Pattern:
  positions -- old, values -- new, separated by tt_sep, e.g.
  "2:1:-3:5:-4" means that line 1 > 2, 2 ->1, 3 just upsets,
  4->5, 5 upsets and becomes of number 4.
  Returns 0 if substitution was implemented, or error code, if fails.
  In the latter case substitutions will be preserved:*/
int tt_linesReorder(char *str,  int table, long top );

/*Reorders  vertex enumerating. Pattern:
  positions -- old, values -- new, separated by tt_sep, e.g.
  "2:1:3:5:4" means that vertices 1 > 2, 2 ->1, 3->3,
  4->5, 5 ->4.
  Returns 0 if substitution was implemented, or error code, if fails.
  In the latter case the vertex substitution will be preserved:*/
int tt_vertexReorder(char *str,  int table, long top );

/*Returns topology reordered by tt_linesReorder/tt_vertexReorder:*/
char *tt_transformedTopology(char *buf,int table, long top);

/*tt_getCoords --
  Puts into buf coordiates -- double pairs x,y,x,y,.... The value of n
  must be one of the following identifiers:
     EXT_VERT
     EXT_VLBL
     EXT_LINE
     EXT_LLBL
     INT_VERT
     INT_VLBL
     INT_LINE
     INT_LLBL
     ORIG_EXT_VERT
     ORIG_EXT_VLBL
     ORIG_EXT_LINE
     ORIG_EXT_LLBL
     ORIG_INT_VERT
     ORIG_INT_VLBL
     ORIG_INT_LINE
     ORIG_INT_LLBL
  buf is of a length maxLen.
  ATTENTION! Checks possible overflow only before processing each pair!
   Returns length of created string, or error code <0:
  */
int tt_getCoords( char *buf,int maxLen,int table, long top, int n);

/*Returns total number of remarks in topology, or TT_NOTFOUND if fails :*/
int tt_howmanyRemarks(int table, long top);
/*Gets remark. Returns 0, or "" if fails:*/
char *tt_getRemark(char *buf,int table, long top, char *name);
/*Sets remark. Returns 0, or TT_NOTFOUND if fails.
    ATTENTION! Both "name" and "val" must be allocated :*/
int tt_setRemark(int table, long top,char *name, char *val);

/*Removes remark. Returns 0, if ok, or or TT_NOTFOUND if fails :*/
int tt_killRemark(int table, long top,char *name);

/*Expects txt is a text with tokens of kind
  blah-blah-blah ... {token} ... blah-blah-blah ... {token} ... blah-blah-blah ...
  Replaces all {token} according to the table from the table "table" and puts the result
  into buf. If the token is absent, it will be translated into empty string.
  If fails (built line is too long, >maxLen) returns  TT_TOOLONGSTRING.
  In the latter case (too long line) it correctly terminates "buf":*/
int tt_translateText(char *txt, char *buf,HASH_TABLE ht, int maxLen);
/*Wrapper to tt_translateText, accepts int table instead of HASH_TABLE ht.
  If fails ( table not found, or built line is too long, >maxLen) returns TT_NOTFOUND
  or TT_TOOLONGSTRING, respectively:*/
int tt_translateTokens(char *txt, char *buf,int table, int maxLen);

/*Returns the value of the remark number "num". "num" is counted from 0:*/
char *tt_valueOfRemark(char *buf,int table, long top, unsigned int num);

/*Returns the name of the remark number "num". "num" is counted from 0:*/
char *tt_nameOfRemark(char *buf,int table, long top, unsigned int num);
/* If to == NULL, allocates and returns new topology identical to given one.
   If to != NULL, modifies existing "to".
   Touches fields according to mask "op", see tt_type.h.
   Newer touch field n!:*/
tt_singletopol_type *tt_copySingleTopology(
                        tt_singletopol_type *from,
                        tt_singletopol_type *to,
                        int op
                        );

/*outfile must be opened, and will be opened after this function.
  isOrig==0 -- usr, non-translated; isOrig!=0 -- nusr, translated:*/
long tt_saveTableToFile(FILE *outfile, int table, int isOrig);
int  tt_saveCoordsToFile(
      FILE *outfile,
      char *buf,
      int maxLen,
      int table,
      long top,
      int isOrig,
      char *sep);
/*outfile must be opened, and will be opened after this function.
  isOrig==0 -- usr, non-translated; isOrig!=0 -- nusr, translated:*/
int tt_saveTopology( FILE *outfile, int table, tt_singletopol_type *top, int isOrig);

/*
 The function fills up fields in "t" from "topol" according to a binary mack "op".
 Full set is:
TT_NEL|TT_NIL|TT_NV|TT_EXT|TT_RED|TT_USR|TT_NMOMENTA|TT_MOMENTA|TT_EXT_MOMENTA|
TT_EXT_COORD|TT_INT_COORD|TT_S_RED_USR|TT_S_USR_NUSR|TT_D_USR_RED|TT_D_USR_NUSR|TT_REM
 see tt_type.h. "t" must be allocated, all internal memory will be allocated by this function.
 Returns 0 on success, or error code <0.
 */
int tt_topol2singletop(aTOPOL *topol,tt_singletopol_type *t, unsigned int op);

/* Allocates and initializes new single topology data:*/
tt_singletopol_type *tt_initSingleTopology(void);
void tt_clearSingleTopology(tt_singletopol_type *t);
/*Auxiliary routine checks the validity and resets (table,top) from user specified
  values to the system ones:*/
int tt_chktop(int *table, long *top);
/*mask is OR'ed ids: INT_VERT INT_VLBL INT_LINE INT_LLBL EXT_VERT EXT_VLBL
  EXT_LINE EXT_LLBL. The function returns 0 if all mentioned types of coordinates
  are present, error code <0, or 1, if there are no some of required coordinates.
 */
int tt_coordinatesOk(int table, long top,int mask);

#ifdef __cplusplus
}
#endif

#endif
