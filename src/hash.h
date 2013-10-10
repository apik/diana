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

#ifndef HASH_H
#define HASH_H

#ifdef __cplusplus
extern "C" {
#endif

/*Wrapper. Allocates new table and copies hash_table to it. Returns new table: */
#define copy_hash_table (hash_table) \
   binary_operations_under_hash_table((hash_table), NULL, -1)

/* Appends entries from "from" to "to" if they are absent into "to".
  ATTENTION!! Types of tags and cells MUST co-incide!!!*/
#define append_to_hash_table(from, to)\
      binary_operations_under_hash_table((from), (to), 1)

/* Appends entries from "from" to "to" If the corresponding entry is persent into "to",
   it will be replaced by a new one.
  ATTENTION!! Types of tags and cells MUST co-incide!!!*/
#define replace_hash_table(from, to)\
      binary_operations_under_hash_table((from), (to), -1)

/*Arbitrary hash table is the array of of the pointers to the follow
structures:*/
struct hash_cell_struct{
   struct hash_cell_struct *next;
   void *tag;
   void *cell;
};
/*Hash function. Arguments: pointer to tag and size of hash table.
It must returns integer number not more than tablesize:*/
typedef word HASH(void *tag, word tsize);
/*Comparing function. Must returns 1, if *tag1==tag2:*/
typedef int CMP(void *tag1,void *tag2);
/*Destructor. It must free tag and cell:*/
typedef void DESTRUCTOR(void **tag, void **cell);

/*Copy function. There are two such a functions , one for tag and another for cell.
  These functions must allocate a space, copy a field and return the pointer to it:*/
typedef void *CPY(void *tag);

struct hash_tab_struct{
  long n;/* number of element in table*/
  word tablesize;/* Length of the table*/
  /*Hash function. It must returns int number more than 0 and
  not more than tablesize:*/
  HASH *hash;
  /*Comparing function. Must returns 1, if *tag1==tag2:*/
  CMP *cmp;
  /*Destructor. It must free tag and cell:*/
  DESTRUCTOR *destructor;
  /*allocate a space, copy tag and return the pointer to it:*/
  CPY *new_tag;
  /*allocate a space, copy cell and return the pointer to it:*/
  CPY *new_cell;
  struct hash_cell_struct **table;/*table*/
};

/*Use this typedef for defining you own hash table:*/
typedef struct hash_tab_struct *HASH_TABLE;

typedef struct hash_cell_struct HASH_CELL;

int resize_hash_table(HASH_TABLE hash_table,word newSize);
word str_hash(void *tag, word tsize);/*Hash for strings*/
int str_cmp(void *tag1, void *tag2);/*cmp for strings*/
void c_destructor(void **tag, void **cell);/*Common destructor*/
/*The follow universal lookup function returns the reference to the pointer
to the cell in which  cell must be situated. If *lookup()==NULL, then
cell is absent in table.*/
HASH_CELL **look_up(void *tag, HASH_TABLE hash_table);

typedef int HASH_ITERATOR(void *info,HASH_CELL *m, word index_in_tbl, word index_in_chain);

/*The same as previous, but returns pointer to field cell, or NULL,
if absent:*/
void *lookup(void *tag, HASH_TABLE hash_table);
/*Allocates memory and returns pointer to new hash table:*/
HASH_TABLE create_hash_table(word number_of_cells,HASH *hash,CMP *cmp,
                              DESTRUCTOR *destructor);
/* Clear hash table recursively, do NOT delete hash table:*/
void clear_hash_table(HASH_TABLE hash_table);
/* install with replacing if cell exists. If cell is already exists in
hash table, it will be replased by new value. If it is not exists in
table, it will be installed.
Returned value: If replacement occure, it returns 1, otherwise, 0:*/
int install(void *tag,void *cell, HASH_TABLE hash_table);
/*Removes set cell. Returned value: if ok, 0, if can't, -1:*/
int uninstall(void *tag,HASH_TABLE hash_table);
/* Destroys hash table:*/
void hash_table_done(HASH_TABLE hash_table);

/*The same as create_hash_table, but stores copy procedures new_tag,new_cell:*/
HASH_TABLE create_hash_table_with_copy(word number_of_cells, HASH *hash,CMP *cmp,
                             DESTRUCTOR *destructor,
                             CPY *new_tag,
                             CPY *new_cell);

/*Applies iterator to each cell of a table. If iterator returns non-null value,
  the iterations will be cancelled, and the result will be returned by hash_foreach
  The type HASH_ITERATOR is a function returning int, accepting the following args:
  void *info -- the pointer to info_for_iterator, useful for transferring some information,
  HASH_CELL *m - clear,
  word index_in_tbl -- index of the table array
  word index_in_chain -- clear.
  */
int hash_foreach(HASH_TABLE hash_table,void *info_for_iterator,HASH_ITERATOR *iterator);

/*op>0 -> add enties to "to" from "from", only if they are absent in "to",
  op<0 -> rewrite entries in "to" by the corresponding "from",
  op==0 -> remove all entries in "to" matching "from"*/
HASH_TABLE binary_operations_under_hash_table(HASH_TABLE from, HASH_TABLE to,int op );

/*Just returns its argument:*/
void *cpy_swallow(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
