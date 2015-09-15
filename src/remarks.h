/*
    This file is part of DIANA program (DIagram ANAlyser) $Revision: 2.37 $.
    Copyright (C) Mikhail Tentyukov <tentukov@physik.uni-bielefeld.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/
#ifndef REMARKS_H
#define REMARKS_H 1

/*Error codes returned by readTopolRemarksFromScanner:*/
/*Double defined name:*/
#define REM_DOUBLEDEF -1
/*Empty name is not allowed:*/
#define REM_EMPTYNAME -2
/*Not enought memory:*/
#define REM_NOTMEMORY -3
/*Unexpected end of file:*/
#define REM_U_EOF -4
/*Unexpected '=':*/
#define REM_UNEXPECTED_EQ -5
#define REM_CANNOTWRITETOFILE -6

#ifdef __cplusplus
extern "C" {
#endif

/* Remarks are arbitrary texts associated with current topology.
   Since we are not going to use it extensively, this is organized
   as a linear (re-allocatable) array of such pairs:*/
struct remark_struct{
   char *name;/* name of the remark*/
   char *text;/*  contents of the remark*/
};
typedef struct remark_struct REMARK;
typedef char *GET_TOKEN_FUNC(void *);

/*"unsigned int n" in the following functions is the total number of
  ALLOCATED remarks. There may be empty remarks among them!*/

/*Returns the real index of the remark of a logical number theIndex.
  Both indices are counted from 0. If fails, returns -1:*/
int indexRemarks(unsigned int n,REMARK *remarks, unsigned int theIndex);

int howmanyRemarks(unsigned int n,REMARK *remarks);

/*Public prototypes:*/
/*Auxiliary routine, returns proper place for placing topologies remark
  of name "name". It does not allocate the remark array, if there is no place,
  it returns just NULL. In that case the array must be reallocated:*/
REMARK *lookupRemark(unsigned int  n,REMARK *remarks,char *name);

/* Sets remark "name " to "newtext". Previous value of "name" is returned in the buffer
 "arg". If arg == NULL, returns NULL.
 If ind!=NULL, then it is an array of length ind[0]. This function will add found
 index to the end of ind and increments ind[0], ONLY if it creates a new cell.
 If it replaces old value, then it does NOT change ind, anyway.
  And, see the following remark:
 ATTENTION! "newtext" is ALLOCATED and must be stored as is.
  If ind==NULL then "name" will be allocated by setTopolRem. Otherwise (if ind!=NULL)
  "name" is assumed as already allocated:*/
char *setTopolRem(char *name, char *newtext, char *arg,
                                unsigned int *top_remarks,
                                REMARK **remarks,
                                int *ind);

/*Returns 0 in success, or -1 if not found
  If oldval != NULL, old value will be returne to *oldval.
  If *oldval != NULL it is assumed that it is a buffer ( NO checking overflow!),
  if *oldval == NULL  it allocates the buffer itself.*/
int deleteTopolRem(char **oldval, unsigned int top_remarks, REMARK *remarks, char *name);
char *getTopolRem(char *name, unsigned int top_remarks, REMARK *remarks);

/*Creates an array of remarks "to" off a length *fromTop and copies a content of
  fromRemarks to *toRemarks. Returns the number of remarks.*/
int copyTopolRem(
   unsigned int fromTop,/* ptr to the top of allocate remarks array*/
   REMARK *fromRemarks, /*ptr to the  existing remarks array*/
   unsigned int *toTop,/* ptr to the destination top of remarks array*/
   REMARK **toRemarks /*ptr to the  destination remarks array*/
            );

/* reads from scanner pairs
   name1=val1:nam2=val2: ... :name=val;
   val may be empty:
     name=:
     or
     name=;
   name cannot be empty, i.e.
   :=val;
   is an error. But EMPTY pair is just ignored:
   ::
   Returns 0 on success, or error code on error, see above:
 */
int readTopolRemarksFromScanner(
      unsigned int *top,/* ptr to the top of allocate remarks array*/
      REMARK **remarks,/*ptr to the  remarks array*/
      int uniqueName,/*0 if new "name" may rewrite existing one*/
      GET_TOKEN_FUNC *getToken, /*Function char *getToken(void *) must return the token
                                  from the scanner*/
      void *arg/* the argument for the function getToken*/
                                  );

/* Saves to the outfile pairs
   name1=val1:nam2=val2: ... :name=val;
   Returns the number of saved remarks:
 */
int saveTopolRem(
   FILE *outfile,
   unsigned int Top,/* top of allocate remarks array*/
   REMARK *Remarks,/* existing remarks array*/
   char *sep/*Delimiter, "" or "\n   ", printed just after ":"*/
            );

/*Destroies an existing remark array. Resets *Top=0 and *Remarks=NULL, Returns number of
  cells were in the array:*/
int killTopolRem(
   unsigned int *Top,/* ptr to the top of allocated remarks array*/
   REMARK **Remarks /* ptr to the existing remarks array*/
                );

#ifdef __cplusplus
}
#endif

#endif
