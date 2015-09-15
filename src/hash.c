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
/*HASH.C*/

#include <stdlib.h>
#include <stdio.h>
#include "tools.h"
#include "comdef.h"
#include "types.h"
#include "headers.h"

/*
Arbitrary hash table is the array of the pointers to the follow
structures:
struct hash_cell_struct{
   struct hash_cell_struct *next;
   void *tag;
   void *cell;
};
Here tag-- pointer to the tag field, cell -- pointer to arbitrary data.
!!!NOTE: do not set cell=NULL for the strings, use cell=tag.
To create new hash table:
1.Define hash function word HASH(void *tag, word tsize);
  It must returns integer number >=0 and <=tsize accordingly *tag.
2.Define function int CMP(void *tag1,void *tag2);
  It must return 1 if *tag1 == *tag2., otherwise, 0.
3.Define function void DESTRUCTOR(void **tag, void **cell);
  It must free memory occupated by *tag and *cell.
  It may be NULL, then it will not be applied at all.
4.Call to function
   HASH_TABLE create_hash_table(word number_of_cells,HASH *hash,CMP *cmp,
                              DESTRUCTOR *destructor).
  It returns variable of type HASH_TABLE, it is the hahdle of created
  hash table.
Then you may use the follow functions:

1. void *lookup(void *tag, HASH_TABLE hash_table);
   It returns pointer to field 'cell' according to 'tag'.
   If lookup()==NULL, then the cell is absent in table.

2. void clear_hash_table(HASH_TABLE hash_table);
   It clears hash table recursively, but does NOT delete hash table.
3. int install(void *tag,void *cell, HASH_TABLE hash_table);
   It installs new cell with replacing if cell exists. If cell is already
   exists in hash table, it will be replased by new value. If it is not
   exists in table, it will be installed.
   Fields *tag and *cell must be allocated in memory; this function
   only stores the pointers to these fields.
   Returned value: If replacement occure, it returns 1, otherwise, 0.
4. void hash_table_done(HASH_TABLE hash_table); Destroys hash table.
5. void c_destructor(void **tag, void **cell);
   This is the common destructor. It does "free_mem" for both
   tag and cell.
6. word str_hash(void *tag, word tsize);
   You may use this function if tag field is the ASCII-Z string.
7. int str_cmp(void *tag1,void *tag2);
   You may use this function if both tag fields ares the ASCII-Z string.

*/

/*    The low level function. Normally, you never use it.
 *    No error checking.
 *    It returns the pointer to the pointer to the hash cell
 *    which should containes the field tag. If the second pointer is NULL,
 *    i.e., (*look_up(...))==NULL, the cell is absent in the table.
 *    So the value to the "next" field is assigned after assigning a value
 *    to the variable returned by this function :
 */
HASH_CELL **look_up(void *tag,HASH_TABLE hash_table)
{
 HASH_CELL **tmp;
   tmp=&(hash_table->table[(hash_table->hash)(tag,hash_table->tablesize)]);
   do{
      if((*tmp==NULL)||((hash_table->cmp)((*tmp)->tag,tag)))
        return(tmp);
      else
         tmp=&((*tmp)->next);
   }while(1);
}/*look_up*/

void *lookup(void *tag, HASH_TABLE hash_table)
{
 HASH_CELL *tmp;
  if(hash_table==NULL)
      return(NULL);
   tmp=hash_table->table[(hash_table->hash)(tag,hash_table->tablesize)];
   do{
      if((tmp==NULL)||((hash_table->cmp)(tmp->tag,tag)))
        return((tmp==NULL)?NULL:tmp->cell);
      else tmp=tmp->next;
   }while(1);
}/*lookup*/

HASH_TABLE create_hash_table(word number_of_cells, HASH *hash,CMP *cmp,
                             DESTRUCTOR *destructor)
{
  HASH_TABLE tmp;
    tmp=get_mem(1,sizeof(struct hash_tab_struct));
    tmp->n=0;
    tmp->tablesize=number_of_cells;
    tmp->hash=hash;
    tmp->cmp=cmp;
    tmp->destructor=destructor;
    tmp->table=get_mem(number_of_cells,sizeof(HASH_CELL *));
    return(tmp);
}/*create_hash_table*/

/* The same as create_hash_table, but stores information about copy functions.
   There are two such a functions , one for tag and another for cell.
   These functions must allocate a space, copy a field and return the pointer to it*/
HASH_TABLE create_hash_table_with_copy(word number_of_cells, HASH *hash,CMP *cmp,
                             DESTRUCTOR *destructor,
                             CPY *new_tag,
                             CPY *new_cell)
{
  HASH_TABLE tmp=create_hash_table(number_of_cells,hash,cmp,destructor);
    tmp->new_tag=new_tag;
    tmp->new_cell=new_cell;
    return(tmp);
}/*create_hash_table_with_copy*/

void clear_hash_table(HASH_TABLE hash_table)
{
 word i;
 HASH_CELL *m1,*m2;
 if(hash_table==NULL)return;
 if(hash_table->table==NULL)return;
 for(i=0;i<hash_table->tablesize;i++)
   if((m1=(hash_table->table)[i])!=NULL){
     if(hash_table->destructor != NULL)
        do{
           (hash_table->destructor)(&(m1->tag),&(m1->cell));
           m1=(m2=m1)->next;
           free_mem(&m2);
        }while(m1!=NULL);
     else
        do{
           m1=(m2=m1)->next;
           free_mem(&m2);
        }while(m1!=NULL);
     (hash_table->table)[i]=NULL;
   }/*if((m1=(hash_table->table)[i])!=NULL)*/
   hash_table->n=0;
}/*clear_hash_table*/

/*Resizes an existing hash table. Returns 0 in success:*/
int resize_hash_table(HASH_TABLE hash_table,word newSize)
{
 word i;
 HASH_CELL *m1,**tmp;
 struct hash_tab_struct tmpTable;

 if(hash_table==NULL)return -1;
 if(newSize==0)return -2;

 if(hash_table->table==NULL){
   hash_table->table=get_mem(newSize,sizeof(HASH_CELL *));
   hash_table->tablesize=newSize;
   return 0;
 }/*if(hash_table->table==NULL)*/

 /*Allocate new table with the same attributes as incoming hash_table
   except tablesize:*/
 tmpTable.tablesize=newSize;
 tmpTable.hash=hash_table->hash;
 tmpTable.cmp=hash_table->cmp;
 tmpTable.destructor=hash_table->destructor;
 tmpTable.new_tag=hash_table->new_tag;
 tmpTable.new_cell=hash_table->new_cell;

 /* The table itself must be of a new size:*/
 tmpTable.table=get_mem(newSize,sizeof(HASH_CELL *));

 for(i=0;i<hash_table->tablesize;i++)
   if((m1=(hash_table->table)[i])!=NULL){
     do{
       tmp=look_up(m1->tag,&tmpTable);
       (*tmp)=m1;/*Here the "next" field probably is assigned!*/
       m1=m1->next;
       (*tmp)->next=NULL;
     }while(m1!=NULL);
   }
 /*Clear old table:*/
 free_mem(&(hash_table->table));
 /*And set up the new one:*/
 hash_table->table=tmpTable.table;
 hash_table->tablesize=newSize;
 return 0;
}/*resize_hash_table*/

int install(void *tag,void *cell, HASH_TABLE hash_table)
{
  HASH_CELL **tmp;
    if(hash_table==NULL)return(-1);
    if((*(tmp=look_up(tag,hash_table)))==NULL){
       /*new element*/
       (*tmp)=get_mem(1,sizeof(HASH_CELL));/*Here the "next" field probably is assigned!*/
       (*tmp)->next=NULL;(*tmp)->cell=cell;(*tmp)->tag=tag;
       (hash_table->n)++;
       return(0);
    }else{
       /*replace existing element*/
       if(hash_table->destructor!=NULL)
          hash_table->destructor(&((*tmp)->tag),&((*tmp)->cell));
       (*tmp)->cell=cell;(*tmp)->tag=tag;
       return(1);
    }
}/*install*/

int uninstall(void *tag,HASH_TABLE hash_table)
{
 HASH_CELL **tmp,*mem;
    if((hash_table==NULL)||((mem=*(tmp=look_up(tag,hash_table)))==NULL))
        return(-1);
    if(hash_table->destructor!=NULL)
       hash_table->destructor(&(mem->tag),&(mem->cell));
    (*tmp)=(*tmp)->next;
    free(mem);
    (hash_table->n)--;
    return(0);
}/*uninstall*/

void hash_table_done(HASH_TABLE hash_table)
{
  if(hash_table==NULL)return;
  clear_hash_table(hash_table);
  free_mem(&(hash_table->table));
  free_mem(&hash_table);
}/*hash_table_done*/

/*variabls.h:*/
extern int g_bits_in_int;
extern int g_bits_in_int_1;

word str_hash(void *tag, word tsize)
{
 char *str=(char *)tag;
 word mem=0;
   while(*str){
     mem=(mem<<1)|(mem>>g_bits_in_int_1);
     mem^=*str++;
   }
   return(mem % tsize);
}/*str_hash*/

int str_cmp(void *tag1, void *tag2)
{
return(!s_scmp(tag1,tag2));
}/*str_cmp*/

void c_destructor(void **tag, void **cell)
{
#ifdef DEBUG
  if ((*tag==NULL)||(*cell==NULL))halt("c_destructor:NULL argument.",NULL);
#endif
  if(*tag==*cell){
    free_mem(tag);
  }else{
    free_mem(tag);
    free_mem(cell);
  }
}/*c_destructor*/

void *cpy_swallow(void *ptr){return ptr;}

/*copy_iterator will be used several times. This is the structure for it's parameters:*/
typedef struct {
   HASH_TABLE to_table;
   CPY *copy_tag;
   CPY *copy_cell;
   int from_to_priors;/*<0 -> from prioritet, >0 -> to prioritet, 0 -> delet matches from "from"*/
} COPY_ITERATOR_PARAMETERS;

static int copy_iterator(void *info,HASH_CELL *m, word index_in_tbl, word index_in_chain)
{
      if( ((COPY_ITERATOR_PARAMETERS *)info)->from_to_priors<0)/*from overrides*/
         install(
          ((COPY_ITERATOR_PARAMETERS *)info)->copy_tag(m->tag),
          ((COPY_ITERATOR_PARAMETERS *)info)->copy_cell(m->cell),
          ((COPY_ITERATOR_PARAMETERS *)info)->to_table
         );
      else if( ((COPY_ITERATOR_PARAMETERS *)info)->from_to_priors>0){/*to overrides*/
         if(
         *look_up(
         m->tag,
         ((COPY_ITERATOR_PARAMETERS *)info)->to_table) == NULL)/*absent!*/
         install(
          ((COPY_ITERATOR_PARAMETERS *)info)->copy_tag(m->tag),
          ((COPY_ITERATOR_PARAMETERS *)info)->copy_cell(m->cell),
          ((COPY_ITERATOR_PARAMETERS *)info)->to_table
         );
      }else /* == 0, just delete "from" from "to": */
          uninstall(m->tag,((COPY_ITERATOR_PARAMETERS *)info)->to_table);
      return 0;
}/*copy_iterator*/

/*op>0 -> add enties to "to" from "from", only if they are absent in "to",
  op<0 -> rewrite entries in "to" by the corresponding "from",
  op==0 -> remove all entries in "to" matching "from"*/
HASH_TABLE binary_operations_under_hash_table(HASH_TABLE from, HASH_TABLE to,int op )
{

 COPY_ITERATOR_PARAMETERS iterator_params;

    if(from == NULL)
       return to;
    if(to == NULL)
       to=create_hash_table_with_copy(
          from->tablesize,
          from->hash,
          from->cmp,
          from->destructor,
          from->new_tag,
          from->new_cell
       );

    /*ATTENTION!! VERY dangerous!!
      Here we use cpy_swallow in case of from->new_tag or from->new_cell === NULL!
      This may lead to TWICE applying of free() in case of nontrivial destructor!*/
    if(  from->new_tag!=NULL  )
       iterator_params.copy_tag=from->new_tag;
    else
       iterator_params.copy_tag=&cpy_swallow;

    if(  from->new_cell !=NULL  )
       iterator_params.copy_cell=from->new_cell;
    else
       iterator_params.copy_cell=&cpy_swallow;

    iterator_params.to_table=to;
    iterator_params.from_to_priors=op;

    hash_foreach(from,&iterator_params,&copy_iterator);
    return to;
}/*binary_operations_under_hash_table*/

int hash_foreach(HASH_TABLE hash_table,void *info_for_iterator,HASH_ITERATOR *iterator)
{
   register HASH_CELL *m;
   register word ret,n,i;

      if((hash_table==NULL)||(hash_table->table==NULL)){
         return(-1);
      }
      for(i=0;i<hash_table->tablesize;i++)/* Cycle on all array cells */
         if((m=(hash_table->table)[i])!=NULL){/* Follow the list*/
            n=0;
            do{  /*Applay iterator for each cell:*/
               if( (ret=iterator(info_for_iterator,m,i,n++))!=0)/* Error.*/
                  return(ret);
               m=m->next;/* Follow the link*/
            }while(m!=NULL);
         }
      return(0);
}/*hash_foreach*/

