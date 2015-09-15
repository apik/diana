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

/*remarks.c*/
#include <stdio.h>
#include <stdlib.h>
#include "remarks.h"

/*increment for realloc:*/
#define DELTA_TOPOLOGY_REMARKS 3

/*****************************************************************************
   External values: the following prototypes/variables/defines are external:
 */
#ifndef NOTMEMORY
#define NOTMEMORY "Not enough memory"
#endif

#ifndef MTRACE_DEBUG
void *get_mem(size_t nitems, size_t size);/* the same as calloc but
         halts process if fail*/
char *new_str(char *s);
int *new_int(int *s);
#else
int s_len(char *pattern);/*Required by sme macros from mtrace_macro.h*/
/* The file "mtrace_macro.h" must be included in case of compilation with mtarce.
   This file contains several macro definitions used to trace through
   get_mem, new_str, and new_int:
 */
#include "mtrace_macro.h"

#endif

void halt(char *fmt, ...);
void free_mem(void *block); /* If *block=NULL, do nothing;
           otherwice, call to free and set *block=NULL*/
char *s_let(char *from,char *to);

int s_scmp(char *s1, char *s2);/* Fast version of s_cmp.
 Returns 0 if strings are coincide*/
/*Concatenates s1 and s2 with memory rellocation for s1:*/
char *s_inc(char *s1, char *s2);
/*****************************************************************************
  End of external values
 */

/*Returns the real index of the remark of a logical number theIndex.
  Both indices are counted from 0. If fails, returns -1:*/
int indexRemarks(unsigned int n,REMARK *remarks, unsigned int theIndex)
{
int i=0;
   /*Note, NO any checking about g_nocurrenttopology! We trast caller...*/
   for(;i<n;i++){
      if( remarks[i].name != NULL ){/*Occupied cell*/
         if(theIndex == 0)return(i);/*That's is what we need*/
         theIndex--;/*Account it!*/
      }/*if( remarks[i].name != NULL )*/
   }/*for(;n>0;n--,remarks++)*/
   return(-1);/*Not found*/
}/*indexRemarks*/

int howmanyRemarks(unsigned int n,REMARK *remarks)
{
int i=0;
   /*Note, NO any checking about g_nocurrenttopology! We trast caller...*/
   for(;n>0;n--,remarks++)if( remarks->name != NULL )i++;/*Occupied cell*/
   return(i);
}/*howmanyRemarks*/

/*Auxiliary routine, returns proper place for placing topologies remark
  of name "name". It does not allocate the remark array, if there is no place,
  it returns just NULL. In that case the array must be reallocated:*/
REMARK *lookupRemark(unsigned int n,REMARK *remarks,char *name)
{
REMARK *mem=NULL;
   /*Note, NO any checking about g_nocurrenttopology! We trast caller...*/
   for(;n>0;n--,remarks++){
      if( remarks->name == NULL ){/*Free cell*/
         if(mem==NULL)mem=remarks;/*First proper, if no "name" -- store it*/
      }else{/*Is it "name"?*/
         if ( s_scmp(remarks->name,name) == 0)return(remarks);/*Found*/
      }
   }/*for(i=0; i<n; i++)*/
   /*If there was a free cell, it must be stored in the "mem" pointer.
     If we reach the end of the array, then there are no free cells, and "mem"
     is just untouched, i.e., NULL:*/
   return(mem);
}/*lookupRemark*/

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
                                int *ind)
{
REMARK *tmp;

  if(  (tmp=lookupRemark(*top_remarks,*remarks,name)) == NULL   ){
     /*No any free cells, array must be extended*/

     /*Store the top  -- here we will put the cell:*/
     int i=*top_remarks;
     /* Extend the array:*/
     if(
        (
         (*remarks)=realloc(
           *remarks,
           ( (*top_remarks)+=DELTA_TOPOLOGY_REMARKS)*sizeof(REMARK)
         )
        )==NULL
     )halt(NOTMEMORY,NULL);
     /*Set "tmp" to the proper cell:*/
     tmp=(*remarks)+i;

     /*Realloc does not initialise allocated memory, so do this here:*/
     for(i=0;i<DELTA_TOPOLOGY_REMARKS; i++)
        tmp[i].name=tmp[i].text=NULL;
  }/*if(  (tmp=lookupTopolRemark(name)) == NULL   )*/
  if(tmp->name == NULL){/*The cell is free*/
     if(ind!=NULL){
        ind[++(*ind)]=tmp-(*remarks);/*store found index:*/
        tmp->name=name;/*Swallow copy*/
     }else
        tmp->name=new_str(name);/*deep copy*/
     tmp->text=newtext;
     if(arg!=NULL)*arg='\0';
  }else{/*Theh was such a name! So, the cell is occupied. */
     if(arg!=NULL)s_let(tmp->text, arg);/*Store returned value*/
     if(ind!=NULL)/*"name" is alocated! Free it:*/
        free(name);
     free_mem(&(tmp->text));/*free old value*/
     tmp->text=newtext;/*Put the new value*/
  }/*if(tmp->name == NULL)...else*/
  return(arg);
}/*setTopolRem*/

char *getTopolRem(char *name, unsigned int top_remarks, REMARK *remarks)
{

  if(
      ( (remarks=
           lookupRemark(
              top_remarks,
              remarks,
              name
           )
        ) == NULL )
      ||
      ( remarks->name == NULL )
    )return NULL;/*not found*/

    return remarks->text;
}/*getTopolRem*/

/*Creates an array of remarks "to" of a length *fromTop and copies a content of
  fromRemarks to *toRemarks. Returns the number of remarks.*/
int copyTopolRem(
   unsigned int fromTop,/* the top of allocate remarks array*/
   REMARK *fromRemarks, /* the existing remarks array*/
   unsigned int *toTop,/* ptr to the destination top of remarks array*/
   REMARK **toRemarks /*ptr to the  destination remarks array*/
            )
{
   int i,n=0;

      if(fromRemarks == NULL){
         *toTop=0;return 0;
      }/*if(fromRemarks == NULL)*/

      (*toRemarks)=(REMARK *)get_mem( (*toTop = fromTop), sizeof(REMARK) );
      for(i=0;i<fromTop; i++){
         if( (fromRemarks[i]).name != NULL ){
            n++;
            ((*toRemarks)[i]).name=new_str(  (fromRemarks[i]).name  );
            ((*toRemarks)[i]).text=new_str(  (fromRemarks[i]).text  );
         }
      }/*for(i=0;i<fromTop; i++)*/
    return n;
}/*copyTopolRem*/

/*Returns 0 in success, or -1 if not found
  If oldval != NULL, old value will be returne to *oldval.
  If *oldval != NULL it is assumed that it is a buffer ( NO checking overflow!),
  if *oldval == NULL  it allocates the buffer itself.*/
int deleteTopolRem(char **oldval,unsigned int top_remarks, REMARK *remarks, char *name)
{
REMARK *tmp=lookupRemark(top_remarks,remarks,name);
   if(   (tmp==NULL)||(tmp->name==NULL)   )return -1;
   if(oldval!=NULL){/*Must return old value*/
      if( (*oldval) !=NULL ) /*assume this is a buffer*/
         s_let(tmp->text,*oldval);
      else/*Null pionter -- allocate it here*/
         *oldval=new_str(tmp->text);
   }/*if(oldval!=NULL)*/
   free_mem(&(tmp->text));
   free_mem(&(tmp->name));
   return 0;
}/*deleteRemark*/

/*Destroies an existing remark array. Resets *Top=0 and *Remarks=NULL, Returns number of
  cells were in the array:*/
int killTopolRem(
   unsigned int *Top,/* ptr to the top of allocated remarks array*/
   REMARK **Remarks /* ptr to the existing remarks array*/
                )
{
int i,n=0;
   if(*Remarks != NULL) {
      for(i=0;i<*Top; i++){
         if( ((*Remarks)[i]).name != NULL ){
            n++;
            free( ((*Remarks)[i]).name );
            free( ((*Remarks)[i]).text );
         }/*if( ((*Remarks)[i]).name != NULL )*/
      }/*for(i=0;i<*Top; i++)*/
      free(*Remarks);
      *Remarks=NULL;
   }/*if(*Remarks != NULL)*/
   *Top=0;
   return n;
}/*int killTopolRem*/

/*The following subroutine stores "name" and "val" fields into the  remarks array.
  (*ind) is the array of integers corresponding to indices of all stored remarks: */
static int l_addPair(
   unsigned int *top,/* ptr to the top of allocate remarks array*/
   REMARK **remarks, /*ptr to the  remarks array*/
   char *name,
   char *val,
   int *top_ind,/*ptr to the top of the allocated index array*/
   int **ind, /*ptr to the index array*/
   int uniqueName/*0 if new "name" may rewrite existing one*/
   )
{
  /* (*ind)[0] is the length of the array, (*top_ind) is the top of allocated space */
  int memInd=(*ind)[0];/*Store old value of the index top. If it will not change,
                         this means that new name replaces the existing one*/

   if ( (*ind)[0]+2 > (*top_ind) )/* Time to realloc*/
      if(
          ( *ind=realloc(*ind,sizeof(int)*((*top_ind)+=DELTA_TOPOLOGY_REMARKS)) )
          ==NULL
        )/*Fail! Not memory:(*/
           return REM_NOTMEMORY;

   /*Store value:*/
   setTopolRem(name, val, NULL, top, remarks, *ind);
                        /*^^^^ since we need not the old value corresponding to "name"*/

   /*Check double, if needed:*/

   if(     uniqueName && ( memInd==(*ind)[0] )     )/*Double defined name*/
      return REM_DOUBLEDEF;

   return 0;
}/*l_addPair*/

/*State identifiers for the finite-state mashine used in the function below:*/
#define stERROR 1

#define stSTART_NAME 2
/*MANDATORY! stEND_NAME=stNAME+1!*/
#define stNAME 3

#define stEND_NAME 4
/*MANDATORY! stEND_VAL=stVAL+1!*/
#define stVAL 5

#define stEND_VAL 6
#define stEND_PAIR 7
#define stSTART_VAL 8
#define stEND 9

/* Saves to the outfile pairs
   name1=val1:nam2=val2: ... :name=val
   Returns the number of saved remarks:
 */
int saveTopolRem(
   FILE *outfile,
   unsigned int Top,/* ptr to the top of allocate remarks array*/
   REMARK *Remarks, /*ptr to the  existing remarks array*/
   char *sep/*Delimiter, "" or "\n   ", printed just after ":"*/
            )
{
   int i,n=0;
   char *text;
      if(Remarks == NULL)
         return 0;

      for(i=0;i<Top; i++){
         if( (Remarks[i]).name != NULL ){
            if(   ( text=(Remarks[i]).text ) == NULL   )text="";
            if( n++ ==0 ){
               if( fprintf(outfile,"%s = %s",(Remarks[i]).name,text)<=0 )
                  return REM_CANNOTWRITETOFILE;
            }else{
               if( fprintf(outfile,":%s%s = %s",sep,(Remarks[i]).name,text)<=0 )
                  return REM_CANNOTWRITETOFILE;
            }
         }/*if( (Remarks[i]).name != NULL )*/
      }/*for(i=0;i<fromTop; i++)*/
    return n;
}/*saveTopolRem*/

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
   Returns 0 on success, or error code on error:
 */
int readTopolRemarksFromScanner(
      unsigned int *top,/* ptr to the top of allocate remarks array*/
      REMARK **remarks,/*ptr to the  remarks array*/
      int uniqueName,/*0 if new "name" may rewrite existing one*/
      GET_TOKEN_FUNC *getToken, /*Function char *getToken(void *) must return the token
                                  from the scanner*/
      void *arg/* the argument for the function getToken*/
                                  )
{
char *name,/*temporary buffer to collect the name*/
     *val,/*temporary buffer to collect the value*/
     **buf,/*ptr to name or val*/
     *theToken;
int state, top_ind=0,*ind, err=0;

   /*  ind is the array of integers corresponding to indices of all stored remarks.
       We need this array to rollback at fail.
       ind[0] is the length of the array, top_ind is the top of allocated space
   */
   /*First, create an empty array:*/
   *(     ind=get_mem( 1, sizeof(int) )    )=0;

   /*Start finite-state machine*/
   for(state=stSTART_NAME; state!=stEND;)switch(state){
      case stERROR:
         /*Must be freed, since after the automaton these texts assumed to be stored
             in remarks, but this is not the case:*/

         /*A stub: if unique_name, the previous value was overridden,
         and rollback is impossible. Correct this, function setTopolRem.
         So leave it as is*/
         if(err!=REM_DOUBLEDEF){
            free_mem(&name);
            free_mem(&val);
         }/*if(err!=REM_DOUBLEDEF)*/
         state=stEND;
         break;
      case stSTART_NAME:
         name=val=NULL;
         buf=&name;
         state=stNAME;
         /*no break!*/
      case stNAME:
      case stVAL:
         if(     (   ( theToken=getToken(arg) ) == NULL   )    ){ /*Unexpected EOF*/
            err=REM_U_EOF;
            state=stERROR;
            break;
         }/*if(     (   ( theToken=getToken(arg) ) == NULL   )    )*/
         switch(*theToken){
            case ':':
            case ';':
            case '=':
               /* Here we use that stEND_NAME=stNAME+1 and stEND_VAL=stVAL+1:*/
               state++;
               break;
            default:/*Just add a token to the current buffer:*/
               *buf=s_inc(*buf,theToken);
         }/*switch(*theToken)*/
         break;
      case stEND_NAME:
         if( *theToken == '=' ){
            if(name==NULL){/*Empty name is not allowed!*/
               err=REM_EMPTYNAME;
               state=stEND;
            }else
               state=stSTART_VAL;
         }else/*No val, i.e., val is empty.*/
            state=stEND_PAIR;
         break;
      case stEND_VAL:
         if( *theToken == '=' ){
            err=REM_UNEXPECTED_EQ;
            state=stERROR;
            break;
         }/*if( *theToken == '=' )*/
         state=stEND_PAIR;
         /*no break!*/
      case stEND_PAIR:
         /* name MAY BE NULL! This means we have to ignore it:*/
         if (name!=NULL){
            if(val==NULL)/*Must be an empty string!*/
               *(     val=get_mem( 1, sizeof(char) )    )='\0';
            if(
                (
                   err=
                      l_addPair(
                         top,
                         remarks,
                         name,
                         val,
                         &top_ind,
                         &ind,
                         uniqueName
                      )
                ) !=0
            ){
               state=stERROR;
               break;
            }
         }/*if (name!=NULL)*/

         if(*theToken == ':')
            state=stSTART_NAME;
         else/*Must be theToken==";"*/
            state=stEND;
         break;
      case stSTART_VAL:
         buf=&val;
         state=stVAL;
         break;
   }/*for(state=stSTART_NAME; state!=stEND;)switch(state)*/
   /*end finite-state machine*/
   if(err){/*Fail!*/
      REMARK *tmp;
      /*rollback. Go along ind (ind[0] is the length of ind) and clear allocated
        texts:*/
      for(;(*ind);(*ind)-- ){
         tmp = *remarks+ind[ *ind ];
         free_mem(&(tmp->text));
         free_mem(&(tmp->name));
      }/*for(;(*ind);(*ind)-- )*/
      /*"name" and "val" are cleared in stERROR state*/

   }/*if(err)*/
   free(ind);
   /* "name" and "val" usually are not NULL, this is ok. They already stored in the array.*/

   return err;
}/*readTopolRemarksFromScanner*/
