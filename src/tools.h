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
/*TOOLS.H*/
#include <stdio.h>

#ifndef NOSTRINGS
/*If defined, we will use our realization of these functons,
 see tools.c*/
void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
#endif

#ifndef UPSET_CH
#define UPSET_CH(a) {char *ch=(a); for(;*ch!='\0';ch++) (*ch)=-(*ch);}
#endif


/*#define _HPUX_SOURCE*/
#define SCAN_BUF_SIZE 1024
#define MAX_STRING_LENGTH 10480 /* old: 1048 */
#define MAX_NAME_LENGTH 255
#define MAX_LINES_IN_VERTEX 8
#define ESC_CHAR '\\'
#define Q_CHAR '\\'
typedef unsigned int word;
typedef unsigned char byte;

struct  bit_field {
  unsigned int bit_0        : 1;
  unsigned int bit_1        : 1;
  unsigned int bit_2        : 1;
  unsigned int bit_3        : 1;
  unsigned int bit_4        : 1;
  unsigned int bit_5        : 1;
  unsigned int bit_6        : 1;
  unsigned int bit_7        : 1;
};

typedef struct bit_field set_of_char[32];
typedef struct bit_field *one_byte;

/*Bitsort from large to small positive numbers:*/
#define L2Ssort(base,buf,n,m)\
   {int i,j=0; for(i=0; i<(n); i++)(buf)[(base)[i]]++;\
    for(i=(m)-1;!(i<0); i--)while((buf)[i]--)(base)[j++]=i;}
/* base-- input array (not too large integers!) of the length n.
 *       There are no special restrictions on n.
 * m-1 -- maximal possible number.
 * buf -- working array of length m.
 *       It must be initialized by all zeroes!!!
 * ARBITRARY integer type is available.
 */

char *s_copy(char *from, char *to, int index,int count);/*return(*to)*/
char *s_let(char *from,char *to);/*return(*to)*/
char *s_letn(char *from,char *to, word n);/*return(*to)*/
int s_pos(char *substr,char *str);/*return pos*/
char *s_insert(char *pattern,char *target,int index);/*return(*target)*/
/* Replace 1st occurance of the substr into str to newsubstr:*/
char *s_replace(char *substr, char *newsubstr, char *str);
/* Replace ALL substr into str to newsubstr:*/
char *s_replaceall(char *substr, char *newsubstr, char *str);
/*Copying until c (except c):*/
char *s_letc( char *from,char *to, char c);
int s_len(char *pattern);
/* Returns length of pattern using both 0 and term as string terminator:*/
int s_lenc(char *pattern,char term);
char *s_cat(char *out,char *pattern1,char *pattern2);/*point to out*/
char *s_del(char *from,char *to,int index,int count);/*point to to*/
/*Compare two strings, s1 and s2. Retuns the length of s1, if the begin of s2
coincides with s1. If not, returns -1. Example: s_bcmp("123","1234") returns 3:*/
int s_bcmp(char *s1, char *s2);
/*Returns -1 if sample1<sample2, 0 if ==, +1 if >:*/
int s_cmp(char *sample1,char *sample2);
/*Compares a and b until term; returns 1 (not 0!) if they coincide
BEFORE term:*/
int s_gcmp(char *a, char *b,char term);
/*Counts number of occurence "substr" into "str":*/
int s_count(char *substr,char *str);
/*Returns 0 if sample1 == sample2:*/
int s_scmp(char *sample1,char *sample2);
int *set_bit(int *bitset, char n);
int *unset_bit(int *bitset, char n);
int is_bit_set(int *bitset, char n);

int set_in(unsigned char ch, set_of_char set);
/* 1 if ch in set ; 0 if ch not in set */

one_byte set_set(unsigned char ch, set_of_char set);
/* sets ch into set; returns *set */

one_byte set_del(unsigned char ch, set_of_char set);
/* deletes ch from set; returns *set */

one_byte set_or(set_of_char set, set_of_char set1, set_of_char set2);
/* returns *set = set1 .OR. set2 */

one_byte set_and(set_of_char set, set_of_char set1, set_of_char set2);
/* returns *set = set1 .AND. set2 */

one_byte set_xor(set_of_char set, set_of_char set1, set_of_char set2);
/* returns *set = set1 .XOR. set2 */

one_byte set_sub(set_of_char set, set_of_char set1, set_of_char set2);
/* returns *set = set1 - set2 */

one_byte set_not(set_of_char set1, set_of_char set);
/* returns *set = .NOT. set1 */

one_byte set_copy(set_of_char set1, set_of_char set);
/* Copies set1 into set2. Returns *set */

int set_cmp(set_of_char set1, set_of_char set2);
/* Returns: -2 if no common element; -1 if set1 in set2;
   0 if set1==set2; 1 if set2 in set1; 2 if both sets
   have different elements.*/

char *set_set2str(set_of_char set, char *str);
/* Returns string imagination of the set */

one_byte set_sset(char *str, set_of_char set);
/* Includes all elements str into set */

one_byte set_str2set(char *str, set_of_char set);
/* Sets set=empty and includes all elements str into set */

one_byte set_sdel(char *str, set_of_char set);
/* Deletes all elements str from set */

int set_scmp(char *str, set_of_char set);
/* The same as set_cmp but first argument is a string. */

#ifdef DEBUG
/*int s_error(s1,s2);*/
#endif

void close_file(void *file);/*closes file if it not NULL and sets file=NULL*/

FILE *open_file(char *file_name, char *mode);/* attempts open
             file and returns pointer to opened file. If fail, halt process*/
#ifndef MTRACE_DEBUG
void *get_mem(size_t nitems, size_t size);/* the same as calloc but
         halts process if fail*/
char *new_str(char *s);
int *new_int(int *s);
#else
/* The file "mtrace_macro.h" must be included in case of compilation with mtarce.
   This file contains several macro definitions used to trace through
   get_mem, new_str, and new_int:
 */
#include "mtrace_macro.h"

#endif

void free_mem(void *block); /* If *block=NULL, do nothing;
           otherwice, call to free and set *block=NULL*/

/* The following 4 functions add or removes all elemens
of their first arguments from scanner spaces or delimiters:*/
one_byte add_spaces(char *thespaces);
one_byte rm_spaces(char *thespaces);
one_byte add_delimiters(char *thedelimiters);
one_byte rm_delimiters(char *thedelimiters);

void tools_done(void);/*destructor*/

/*Outputs a scanner message to stderr and log file, if opened:*/
void output_scanner_msg(void);

#ifdef TOOLS
char *(*sc_get_token)(char *str)=NULL;
char EOF_denied = 0;
int hash_enable=1;
char esc_char=ESC_CHAR;
char q_char=Q_CHAR;
set_of_char regular_chars;

extern int isdebug;
extern void halt(char *fmt, ...);/*halt.c*/
extern void message(char *s1,...);/*halt.c*/
extern FILE *link_file(char *name, long pos);/*utils.c*/
extern FILE *link_stream(char *name, long pos);/*utils.c*/
extern int unlink_file(char *name);/*utils.c*/
extern void unlink_all_files(void);/*utils.c*/
extern int errorlevel;
extern FILE *log_file;
extern int max_include;
#else
extern set_of_char regular_chars;
extern char *(*sc_get_token)(char *str);
extern char EOF_denied;
extern int hash_enable;
extern char esc_char;
extern char q_char;
#endif
void multiplylastinclude(int n);/* Myltiplies las include n times*/
int include(char *fname);/* Includes file into scanning stream.*/
/* Goes to the specified position of the file. If position<0 interpretes fname as a
   string to be scanned:*/
int gotomacro(char *fname,long position,long new_line,int new_col);
void break_include(void);

/*Scaner constructor:*/
int sc_init(char *init_f_name,set_of_char init_spaces,
                      set_of_char init_delimiters,char init_comment,
                      int init_hash_enable, char init_esc_char);
int sc_done(void);/* Scaner destructor*/
/*Double scanner constructor:*/
int double_init(char *init_f_name,set_of_char init_spaces,
            set_of_char init_delimiters,char init_comment,
            int init_hash_enable, char init_esc_char);
/*Double scanner destructor:*/
int double_done(void);
void sc_mark(void);/* Begin copying tokens into buffer*/
void sc_release(void);/* Stop copying tokens, release buffer*/
void sc_repeat(void);/*Stop copying tokens, repepeat buffer to sc_get_token*/

char *get_input_file_name(void);
long get_current_line(void);
int get_current_col(void);
long get_current_pos(void);

char *get_inkl_name(char *str);/*Returns name include file using
                                standart scaner in HASH mode.*/
int m_cmp(char *p1,char *p2); /* returns 1 if sets of
                                            identifiers are coicide*/
word m_len(char *s);/*returns number of chars required to allocate set
                          s in memory */
char *m_let(char *from, char *to); /* copeis set of id <from> to <to> */
char *m_cat(char *dest, char *id1, char *id2);/* concatenetes
                                   sets id1 and id2 into dest */
char *m2s(char *s);/*converts set of id into sequence of id separated
                                    by commas. */
char *encode(long val, char *buf, int base);/*converts long val into
    4-byte string. ' ' is minimum character, base+' ' is maximum. Note,
    maximal possible value depend on base.*/
long decode(char *buf, int base);/*converts 4-byte string into long value*/

void realloc_include(void);
void tryexpand_include( int n);

char get_comment(void);

void new_comment(char newcomment);

/*Swaps '+' and '-'; returns 0 if result is ready, or 1 if it shuld be prependen by '-':*/
int swapCharSng(char *m);

/* Set the FD_CLOEXEC  flag of desc if value is nonzero,
 or clear the flag if value is 0.
 Return 0 on success, or -1 on error with errno  set. */
int set_cloexec_flag (int desc, int value);
