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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define TOOLS
#include "tools.h"
#include "texts.h"

#define bitDONTRESETCOL 0
/*
#define bitDONTRESETSTR 1
#define bitFIRSTTIME 2
*/
struct stack_cell{
  char *red_string;
  long current_pos;
  long fposition;
  long current_line;
  int current_col;
  char *inp_f_name;
};

static struct stack_cell *include_stack=NULL;
static long current_line=0;
static int current_col=0;
static int q_begin=0;
static char b_get(void);
static int b_put(char ch);
/*???static int b_free(void);*/
static int read_string_mode=0;

static char b_buf[SCAN_BUF_SIZE];
static char *b_tail=b_buf,
            *b_head=b_buf,
            *b_begin=b_buf,
            *b_end=b_buf+SCAN_BUF_SIZE;
static int b_status=-1;
static set_of_char spaces_s,delimiters_s;
static char red_string[MAX_STRING_LENGTH];
static char inp_f_name[MAX_NAME_LENGTH];
static char comment=0;
static FILE *inp_file=NULL;
static int is_init=0;
static int include_depth=0;
static long current_pos=0;

#ifdef DEBUG
void s_error(s1,s2)
const char *s1;
char *s2;
{
 char tmp[256];
   if ((s1==NULL)?0:s_len(s1) + (s2==NULL)?0:s_len(s2) >255)
       s_let("Too long error message. Program terminated.",tmp);
   else
      sprintf(tmp,s1,s2);
   halt(tmp,NULL);
}/*s_error*/
#endif

#ifdef NOSTRINGS
/*If defined, we will use our realization of these functons:*/
void *memset(void *s, int c, size_t n)
{
register int i;
register char *tmp=s;
   for(i=0;i<n;i++)*tmp++=(char)c;
   return s;
}/*memset*/
void *memcpy(void *dest, const void *src, size_t n);
{
register int i;
register char *s=src;
register char *d=dest;

   for(i=0;i<n;i++)*d++=*s++;
   return dest;
}/*memset*/
#endif

static void done(void)
{
 int i;
  for(i=0; i<include_depth;i++){
     free(include_stack[i].red_string);
     free(include_stack[i].inp_f_name);
  }
  free_mem(&include_stack);
  include_depth=0;
  unlink_all_files();
}/*done*/

char *s_copy(char *from, char *to, int index,int count)
{
 register int i=0;
 register   char *f,*t=to;

#ifdef DEBUG
     if ((from==NULL)||(to==NULL))s_error("Argument s_copy is NULL",NULL);
     if (s_len(from)<index)s_error("Argument s_copy too short: %s",from);
     if (index<0) s_error("Index in s_copy is negative: %d",index);
     if (count<0) s_error("Count in s_copy is negative: %d",count);
#endif
    for (f=(char*)from+index;((*f))&&(i++<count);*t++=*f++);
    *t=0;
    return(to);
}/*s_copy*/

char *s_let(char *from,char *to)
{
register  char *t=to;
#ifdef DEBUG
     if ((from==NULL)||(to==NULL))s_error("Argument s_let is NULL",NULL);
#endif
    while((*t++=*from++)!=0);
    return(to);
}/*s_let*/

char *s_letn(char *from,char *to, word n)
{
register   word i=1;
#ifdef DEBUG
     if ((from==NULL)||(to==NULL))s_error("Argument s_letn is NULL",NULL);
#endif
    for(i=0;i<n;i++) if((to[i]=from[i])=='\0')break;
    if((i==n)&&(i))to[i-1]=0;
    return(to);
}/*s_letn*/

char *s_letc( char *from,char *to, char c)
{
register  word i=1;
#ifdef DEBUG
     if ((from==NULL)||(to==NULL))s_error("Argument s_letc is NULL",NULL);
#endif
    for(i=0;from[i];i++)if((to[i]=from[i])==c)break;
    to[i]=0;
    return(to);
}/*s_letc*/

int s_pos(char *substr,char *str)
{
register   char *s=(char*)str,*p=(char*)substr;
#ifdef DEBUG
     if ((substr==NULL)||(str==NULL))s_error("Argument s_pos is NULL",NULL);
#endif
    if (!((*s)&&(*p))) return(-1);
    do{
 while(*s!=*p)if(!(*s++))return(-1);
 while(*s==*p){
   if(!(*++p))return((int)((s-(char*)str)-(p-(char*)substr))+1);
           if(!(*++s))return(-1);
         }
         p=(char*)substr;
      }while(1);
}/*s_pos*/

char *s_insert(char *pattern,char *target,int index)
{
 register int c_p,i;
 register char *p=(char*)pattern,*t=target;

#ifdef DEBUG
     if ((pattern==NULL)||(target==NULL))
         s_error("Argument s_insert is NULL",NULL);
     if (s_len(target)<index)
        s_error("Argument s_insert too short: %s",target);
     if (index<0) s_error("Index in s_insert is negative: %d",index);
#endif
     c_p=s_len(pattern);
    for (t+=(i=s_len(target)),i-=index;i-->-1;t--)*(t+c_p)=*t;
    for (t=&target[index],i=0;i++<c_p;*t++=*p++);
    return(target);
}/*s_insert*/

/* Replace 1st occurance of the substr into str to newsubstr:*/
char *s_replace(char *substr, char *newsubstr, char *str)
{
  int i;
  if(      ( i=s_pos(substr,str) ) == -1   ) return(str);
  return(s_insert(newsubstr,s_del(str,str,i,s_len(substr)),i));
}/*s_replace*/

/* Replace ALL substr into str to newsubstr:*/
char *s_replaceall(char *substr, char *newsubstr, char *str)
{
  int i;
  while(      ( i=s_pos(substr,str) ) != -1   )
    s_insert(newsubstr,s_del(str,str,i,s_len(substr)),i);
  return(str);
}/*s_replaceall*/

int s_len(char *pattern)
{
register  char *p=(char*)pattern;
#ifdef DEBUG
     if (pattern==NULL)s_error("Argument s_len is NULL",NULL);
#endif
    while(*p)p++;
    return((int) ((p-(char*)pattern)) );
}/*s_len*/
int s_lenc(char *pattern,char term)
{
register   char *p=(char*)pattern;
#ifdef DEBUG
     if (pattern==NULL)s_error("Argument s_lenc is NULL",NULL);
#endif
    while((*p)&&(*p!=term))p++;
    return((int) ((p-(char*)pattern)) );
}/*s_lenc*/

char *s_cat(char *out,char *pattern1,char *pattern2)
{
register  char *p=(char*)pattern1,*o=out;
#ifdef DEBUG
     if ((out==NULL)||(pattern1==NULL)||(pattern2==NULL))
          s_error("Argument s_cat is NULL",NULL);
#endif
     if(out==pattern2)return(s_insert(pattern1,out,0));
     while((*o++=*p++)!=0);
     for(o--,p=(char*)pattern2;(*o++=*p++)!=0;);
     return(out);
}/*s_cat*/

char *s_del(char *from,char *to,int index,int count)
{
 register int i;
 register char *f=(char*)from,*t=to;

#ifdef DEBUG
     if ((from==NULL)||(to==NULL))s_error("Argument s_del is NULL",NULL);
     if (s_len(from)<index)
     s_error("Argument s_del too short: %s",from);
     if (index<0) s_error("Index in s_del is negative: %d",index);
     if (count<0) s_error("Count in s_del is negative: %d",count);
#endif
    for (i=0;i++<index;*t++=*f++);
    i=s_len(from);
    f+=(i>index+count)?count:i-index;
    while((*t++=*f++)!=0);
    return(to);
}/*s_del*/

int s_cmp(char *sample1,char *sample2)
{
register  char *s1=(char*)sample1,*s2=(char*)sample2;
#ifdef DEBUG
     if ((sample1==NULL)||(sample2==NULL))
          s_error("Argument s_cmp is NULL",NULL);
#endif
    while(*s1==*s2++){if(!(*s1++))return(0-*--s2);}
    return(*s1-*--s2);
}/*s_cmp*/

int s_gcmp(char *a, char *b,char term)
{
  while (*a&&*b){
     if( (*a==term)&&(*b==term) )
         return(1);
     if(*a!=*b)return(0);
     a++;b++;
  }
  return(((*a==0)||(*a==term))&&((*b==0)||(*b==term)));
}/*s_gcmp*/

int s_count(char *substr,char *str)
{
register char *s=(char*)str,*p=(char*)substr;
register int count=0;
#ifdef DEBUG
     if ((substr==NULL)||(str==NULL))s_error("Argument s_count is NULL",NULL);
#endif
    if (!((*s)&&(*p))) return(0);
    do{
       while(*s!=*p)if(!(*s++))return(count);
       while(*s==*p){
          if(!(*++s)){if(!(*(p+1)))count++;return(count);}
          if(!(*++p)){count++;break;}
       }
       p=(char*)substr;
    }while(1);
}/*s_count*/

/*Compare two strings, s1 and s2. Retuns the length of s1, if the begin of s2
coincides with s1. If not, returns -1. Example: s_bcmp("123","1234") returns 3:*/
int s_bcmp(char *s1, char *s2)
{
int n;
   for(n=0;*s1!='\0'; s1++,s2++,n++)
      if(  (*s1)!=(*s2) ) return -1;
   return n;
}/*s_bcmp*/

int s_scmp(char *s1, char *s2)/* Fast version of s_cmp.
 Returns 0 if strings are coincide*/
{
  while (*s1||*s2)if(*(char*)s1++!=*(char*)s2++)return(1);
  return(0);
}/*s_scmp*/

int *set_bit(int *bitset, char n)
{
register int mask=1;
(*bitset)|=(mask<<n);
return(bitset);
}/*set_bit*/

int *unset_bit(int *bitset, char n)
{
register int mask=1;
(*bitset)&=~(mask<<n);
return(bitset);
}/*set_bit*/

int is_bit_set(int *bitset, char n)
{
register int mask=1;
return((mask<<n)&(*bitset));
}/*is_bit_set*/

int set_in(unsigned char ch, set_of_char set)
{
  set += ch/8;
  switch (ch % 8){
     case 0: return(set->bit_0);
     case 1: return(set->bit_1);
     case 2: return(set->bit_2);
     case 3: return(set->bit_3);
     case 4: return(set->bit_4);
     case 5: return(set->bit_5);
     case 6: return(set->bit_6);
     case 7: return(set->bit_7);
  }
  return(-1);
}/*set_in*/

one_byte set_set(unsigned char ch, set_of_char set)
{
  one_byte tmp=(one_byte)set;
  set += ch/8;
  switch (ch % 8){
     case 0: set->bit_0=1;break;
     case 1: set->bit_1=1;break;
     case 2: set->bit_2=1;break;
     case 3: set->bit_3=1;break;
     case 4: set->bit_4=1;break;
     case 5: set->bit_5=1;break;
     case 6: set->bit_6=1;break;
     case 7: set->bit_7=1;break;
  }
  return(tmp);
}/*set_set*/

one_byte set_del(unsigned char ch, set_of_char set)
{
  one_byte tmp=(one_byte)set;
  set += ch/8;
  switch (ch % 8){
     case 0: set->bit_0=0;break;
     case 1: set->bit_1=0;break;
     case 2: set->bit_2=0;break;
     case 3: set->bit_3=0;break;
     case 4: set->bit_4=0;break;
     case 5: set->bit_5=0;break;
     case 6: set->bit_6=0;break;
     case 7: set->bit_7=0;break;
  }
  return(tmp);
}/*set_del*/

one_byte set_or(set_of_char set, set_of_char set1, set_of_char set2)
{
  one_byte tmp=(one_byte)set;
  int i=0,j=0;
  while(j=0,i++<32)
  while(j<9)
     switch (j++){
        case 0: set->bit_0=(set1->bit_0||set2->bit_0);break;
        case 1: set->bit_1=(set1->bit_1||set2->bit_1);break;
        case 2: set->bit_2=(set1->bit_2||set2->bit_2);break;
        case 3: set->bit_3=(set1->bit_3||set2->bit_3);break;
        case 4: set->bit_4=(set1->bit_4||set2->bit_4);break;
        case 5: set->bit_5=(set1->bit_5||set2->bit_5);break;
        case 6: set->bit_6=(set1->bit_6||set2->bit_6);break;
        case 7: set->bit_7=(set1->bit_7||set2->bit_7);break;
        case 8: set++;set1++;set2++;
     };
  return(tmp);
}/*set_or*/

one_byte set_and(set_of_char set, set_of_char set1, set_of_char set2)
{
  one_byte tmp=(one_byte)set;
  int i=0,j=0;
  while(j=0,i++<32)
  while(j<9)
     switch (j++){
        case 0: set->bit_0=(set1->bit_0&&set2->bit_0);break;
        case 1: set->bit_1=(set1->bit_1&&set2->bit_1);break;
        case 2: set->bit_2=(set1->bit_2&&set2->bit_2);break;
        case 3: set->bit_3=(set1->bit_3&&set2->bit_3);break;
        case 4: set->bit_4=(set1->bit_4&&set2->bit_4);break;
        case 5: set->bit_5=(set1->bit_5&&set2->bit_5);break;
        case 6: set->bit_6=(set1->bit_6&&set2->bit_6);break;
        case 7: set->bit_7=(set1->bit_7&&set2->bit_7);break;
        case 8: set++;set1++;set2++;
     };
  return(tmp);
}/*set_and*/

one_byte set_xor(set_of_char set, set_of_char set1, set_of_char set2)
{
  one_byte tmp=(one_byte)set;
  int i=0,j=0;
  while(j=0,i++<32)
  while(j<9)
     switch (j++){
        case 0: set->bit_0=(set1->bit_0+set2->bit_0);break;
        case 1: set->bit_1=(set1->bit_1+set2->bit_1);break;
        case 2: set->bit_2=(set1->bit_2+set2->bit_2);break;
        case 3: set->bit_3=(set1->bit_3+set2->bit_3);break;
        case 4: set->bit_4=(set1->bit_4+set2->bit_4);break;
        case 5: set->bit_5=(set1->bit_5+set2->bit_5);break;
        case 6: set->bit_6=(set1->bit_6+set2->bit_6);break;
        case 7: set->bit_7=(set1->bit_7+set2->bit_7);break;
        case 8: set++;set1++;set2++;
     };
  return(tmp);
}/*set_xor*/

one_byte set_sub(set_of_char set, set_of_char set1, set_of_char set2)
{
  one_byte tmp=(one_byte)set;
  int i=0,j=0;
  while(j=0,i++<32)
  while(j<9)
     switch (j++){
        case 0: set->bit_0=(set1->bit_0&&(!set2->bit_0));break;
        case 1: set->bit_1=(set1->bit_1&&(!set2->bit_1));break;
        case 2: set->bit_2=(set1->bit_2&&(!set2->bit_2));break;
        case 3: set->bit_3=(set1->bit_3&&(!set2->bit_3));break;
        case 4: set->bit_4=(set1->bit_4&&(!set2->bit_4));break;
        case 5: set->bit_5=(set1->bit_5&&(!set2->bit_5));break;
        case 6: set->bit_6=(set1->bit_6&&(!set2->bit_6));break;
        case 7: set->bit_7=(set1->bit_7&&(!set2->bit_7));break;
        case 8: set++;set1++;set2++;
     };
  return(tmp);
}/*set_sub*/

one_byte set_not(set_of_char set1, set_of_char set)
{
  one_byte tmp=(one_byte)set;
  int i=0,j=0;
  while(j=0,i++<32)
  while(j<9)
     switch (j++){
        case 0: set->bit_0=(!set1->bit_0);break;
        case 1: set->bit_1=(!set1->bit_1);break;
        case 2: set->bit_2=(!set1->bit_2);break;
        case 3: set->bit_3=(!set1->bit_3);break;
        case 4: set->bit_4=(!set1->bit_4);break;
        case 5: set->bit_5=(!set1->bit_5);break;
        case 6: set->bit_6=(!set1->bit_6);break;
        case 7: set->bit_7=(!set1->bit_7);break;
        case 8: set++;set1++;
     };
  return(tmp);
}/*set_not*/

one_byte set_copy(set_of_char set1, set_of_char set)
{
  one_byte tmp=(one_byte)set;
  int i=0,j=0;
  while(j=0,i++<32)
  while(j<9)
     switch (j++){
        case 0: set->bit_0=set1->bit_0;break;
        case 1: set->bit_1=set1->bit_1;break;
        case 2: set->bit_2=set1->bit_2;break;
        case 3: set->bit_3=set1->bit_3;break;
        case 4: set->bit_4=set1->bit_4;break;
        case 5: set->bit_5=set1->bit_5;break;
        case 6: set->bit_6=set1->bit_6;break;
        case 7: set->bit_7=set1->bit_7;break;
        case 8: set++;set1++;
     };
  return(tmp);
}/*set_copy*/

int set_cmp(set_of_char set1, set_of_char set2)

/* -2 if no common element; -1 if set1 in set2;
   0 if set1==set2; 1 if set2 in set1; 2 if there are
   common elements, and another elements are present. */

{
  int i=0,j=0,k0=0,k1=0,k2=0;
  while(j=0,i++<32)
  while(j<9)
     switch (j++){
        case 0: if(set1->bit_0&&(!set2->bit_0))k1=1;else
                if(set1->bit_0&&set2->bit_0)k0=1;else
                if(!(set1->bit_0)&&set2->bit_0)k2=1; break;
        case 1: if(set1->bit_1&&(!set2->bit_1))k1=1;else
                if(set1->bit_1&&set2->bit_1)k0=1;else
                if(!(set1->bit_1)&&set2->bit_1)k2=1; break;
        case 2: if(set1->bit_2&&(!set2->bit_2))k1=1;else
                if(set1->bit_2&&set2->bit_2)k0=1;else
                if(!(set1->bit_2)&&set2->bit_2)k2=1; break;
        case 3: if(set1->bit_3&&(!set2->bit_3))k1=1;else
                if(set1->bit_3&&set2->bit_3)k0=1;else
                if(!(set1->bit_3)&&set2->bit_3)k2=1; break;
        case 4: if(set1->bit_4&&(!set2->bit_4))k1=1;else
                if(set1->bit_4&&set2->bit_4)k0=1;else
                if(!(set1->bit_4)&&set2->bit_4)k2=1; break;
        case 5: if(set1->bit_5&&(!set2->bit_5))k1=1;else
                if(set1->bit_5&&set2->bit_5)k0=1;else
                if(!(set1->bit_5)&&set2->bit_5)k2=1; break;
        case 6: if(set1->bit_6&&(!set2->bit_6))k1=1;else
                if(set1->bit_6&&set2->bit_6)k0=1;else
                if(!(set1->bit_6)&&set2->bit_6)k2=1; break;
        case 7: if(set1->bit_7&&(!set2->bit_7))k1=1;else
                if(set1->bit_7&&set2->bit_7)k0=1;else
                if(!(set1->bit_7)&&set2->bit_7)k2=1; break;
        case 8: set1++;set2++;
     }
     switch(k0+k1+k2){
        case 0: return(0);
        case 1: return((k0)?0:-2);
        case 2: return((k0)?(k1)?1:-1:-2);
        case 3: return(2);
     }
     return(255);
}/*set_cmp*/

char *set_set2str(set_of_char set, char *str)
{
register  char *tmp=str;
register  int i=0,j=0;
#ifdef DEBUG
     if (str==NULL)s_error("Argument set_set2str is NULL",NULL);
#endif
  while(j=0,i++<32)
  while(j<9)
     switch (j++){
        case 0: if (set->bit_0) *str++=(i-1)*8+j-1;break;
        case 1: if (set->bit_1) *str++=(i-1)*8+j-1;break;
        case 2: if (set->bit_2) *str++=(i-1)*8+j-1;break;
        case 3: if (set->bit_3) *str++=(i-1)*8+j-1;break;
        case 4: if (set->bit_4) *str++=(i-1)*8+j-1;break;
        case 5: if (set->bit_5) *str++=(i-1)*8+j-1;break;
        case 6: if (set->bit_6) *str++=(i-1)*8+j-1;break;
        case 7: if (set->bit_7) *str++=(i-1)*8+j-1;break;
        case 8: set++;
     };
  *str=0;
  return(tmp);
}/*set_set2str*/

one_byte set_sset(char *str, set_of_char set)
{
register  char *tmp= (char*)str;
#ifdef DEBUG
     if (str==NULL)s_error("Argument set_sset is NULL",NULL);
#endif
 while(*tmp)set_set(*tmp++,set);
 return(set);
}/*set_sset*/

one_byte set_str2set(char *str, set_of_char set)
{
#ifdef DEBUG
     if (str==NULL)s_error("Argument set_str2set is NULL",NULL);
#endif
  set_sub(set,set,set);
  return(set_sset(str,set));
}/*set_str2set*/

one_byte set_sdel(char *str, set_of_char set)
{
register char *tmp= (char*)str;
#ifdef DEBUG
     if (str==NULL)s_error("Argument set_sdel is NULL",NULL);
#endif
 while(*tmp)set_del(*tmp++,set);
 return(set);
}/*set_sdel*/

int set_scmp(char *str, set_of_char set)
{
 set_of_char tmp;
#ifdef DEBUG
     if (str==NULL)s_error("Argument set_scmp is NULL",NULL);
#endif
 return(set_cmp(set_str2set(str,tmp),set));
}/*set_scmp*/

void close_file(void *file)/* closes file if it not NULL and sets file=NULL*/
{
 if(*(FILE**)file == NULL)return;
 if(!(
       (*(FILE**)file == stdout)||
       (*(FILE**)file == stdin)||
       (*(FILE**)file == stderr)
     )
  ){

      fclose(*(FILE**)file); /* Can't call halt here -- danger of infinite loop */
  }
  *(FILE**)file = NULL;
}/*close_file*/

FILE *open_file(char *file_name, char *mode)/* attempts open file
                  and returns pointer to opened file. If fail, halt process*/
{
 FILE *tmp;
  if((tmp=fopen(file_name,mode))==NULL)
                                     halt(CANNOTOPEN ,(char*)file_name);
  return(tmp);
}/*open_file*/

#ifndef MTRACE_DEBUG
void *get_mem(size_t nitems, size_t size)/* the same as calloc but
         halts process if fail*/
{
 void *tmp;
   /*One note: according to CURRENT standarts, malloc returns NOT NULL
     when required to alllocate 0 byte. But on some systems it returns NULL.
     But this is NOT an error!:*/
   if( ((tmp=calloc(nitems,size))==NULL)&&(nitems!=0) )halt(NOTMEMORY,NULL);
   return(tmp);
}/*get_mem*/
#endif

void free_mem(void *block) /* If *block=NULL, do nothing;
           otherwice, call to free and set *block=NULL*/
{
 if (*(void**)block==NULL) return;

 free(*(void**)block);
 (*(void**)block)=NULL;
}/*free_mem*/

#ifndef MTRACE_DEBUG
char *new_str(char *s)
#else
/*Macro new_str() will be defined, and new_str_function() will stay as a function*/
char *new_str_function(char *s)
#endif
{
  if(s==NULL)return(NULL);
  return(s_let(s,get_mem((size_t)(s_len(s)+2),sizeof(char))));
}/*new_str*/

#ifndef MTRACE_DEBUG
int *new_int(int *s)
{
register int *tmp,i;
    if(s==NULL)return(NULL);
    tmp=get_mem(*s+1,sizeof(int));
    for(i=*s;!(i<0);i--)tmp[i]=s[i];
    return(tmp);
}/*new_int*/
#else
/*The following function will be used in a macro  new_int. It just copies the content
  of 'from' int 'to' and returns a pointer to 'to':*/
int *copy_int_vec(int *from, int*to)
{
register int i;
   for(i=*from;!(i<0);i--)to[i]=from[i];
   return to;
}/*copy_int_vec*/

static void *l_dup_ptr=NULL;

/*The followind two functions are used to emulate new_str and new_int through macros.
  The problem is that we can't evaluate macro argument more then one time!:*/
/*Copies the argument to internal variable and returns it value:*/
void *dup_ptr( void *s)
{
   return l_dup_ptr=s;
}/*dup_ptr*/
/*Returns the value stored by dup_ptr:*/
void *get_dup_ptr(void)
{
   return l_dup_ptr;
}/*get_dup_ptr*/
/*ATTENTION! Using of the above two functions is NOT re-enter-able!*/
#endif

static void print_scanneractive_message(FILE *thestream)
{
   fprintf(thestream,SCANERACTIVE,inp_f_name, current_line, current_col);
   if(*inp_f_name==0)
      fprintf(thestream,POSITIONINFOUNAVAIL);
   if(*red_string){
        int i;
        if(red_string[s_len(red_string)-1]=='\n')
           red_string[s_len(red_string)-1]=0;
        fprintf(thestream,"%s\n",red_string);
        for(i=1;i<current_col;i++) fprintf(thestream," ");
        fprintf(thestream,"^\n");
   }/*if(*red_string)*/

}/*print_scanneractive_message*/

void output_scanner_msg(void)
{
   print_scanneractive_message(stderr);
   if(log_file!=NULL)
      print_scanneractive_message(log_file);
}/*output_scanner_msg*/

void tools_done(void)
{
  if(is_init){
     output_scanner_msg();
     sc_done();
  }/*if(is_init)*/
}/*tools_done*/

static char b_get(void)
{
 char tmp;
   if (b_status==-1) return(0);
   tmp=*b_tail++;
   if (b_tail==b_end)b_tail=b_begin;
   if (b_tail==b_head)b_status=-1;else b_status=0;
   return(tmp);
}/*b_get*/

static int b_put(char ch)
{
   *b_head++=ch;
   if (b_head==b_end)b_head=b_begin;
   if (b_head==b_tail)b_status=1;else b_status=0;
   return(b_status);
}/*b_put*/

#ifdef SKIP
static int b_free(void)
{
register int i;
    if (b_status==1) return(0);
    return(((i=(int)(b_tail-b_head))>0)?i:(i+=(int)(b_end-b_begin)));
}/*b_free*/
#endif

void break_include(void)
{
char tmp[MAX_STRING_LENGTH];
  if(isdebug){
     if(*inp_f_name==0)
       sprintf(tmp,ENDFILE,current_line,current_col,red_string);
     else
       sprintf(tmp,ENDFILE,current_line,current_col,inp_f_name);
     message(tmp,NULL);
  }
  if (include_depth < 1)
     halt(CNATRETURNFROMINCLUDE,NULL);
  s_let(include_stack[--include_depth].red_string,red_string);
  free_mem(&(include_stack[include_depth].red_string));
  current_pos=include_stack[include_depth].current_pos;
  current_line=include_stack[include_depth].current_line;
  current_col=include_stack[include_depth].current_col;
  if(*inp_f_name!=0)
     unlink_file(inp_f_name);

  s_let(include_stack[include_depth].inp_f_name,inp_f_name);
  if(*(include_stack[include_depth].inp_f_name) != 0)
        inp_file=link_stream(inp_f_name,
                 include_stack[include_depth].fposition);
  else
     inp_file=NULL;
  free_mem(&(include_stack[include_depth].inp_f_name));
  if(isdebug)
    message(BACKTOFILE,inp_f_name);
}/*break_include*/

static char *end_include(void)
{
  break_include();
  if(isdebug==0)
    message(BACKTOFILE,inp_f_name);
  return(red_string);
}/*end_include*/

static char *read_string(void)
{
 current_line++;
 if(!is_bit_set(&read_string_mode,bitDONTRESETCOL))
    current_col=0;
 else
    unset_bit(&read_string_mode,bitDONTRESETCOL);
 if(inp_file==NULL){
       if (include_depth){
          break_include();
          return(red_string);
       }
       if (EOF_denied)halt(UNEXPECTEDEOF,NULL);
       return(NULL);
 }else{
     current_pos=ftell(inp_file);
     if(fgets(red_string, SCAN_BUF_SIZE, inp_file)==NULL){
       if (include_depth)return(end_include());
       if (EOF_denied)halt(UNEXPECTEDEOF,NULL);
       return(NULL);
     }
 }
 return(red_string);
}/*read_string*/

static void verb(char **str)
{
register int balance=0;
   while(!((red_string[current_col]==')')&&(balance==0))){
     if(red_string[current_col]=='(')balance++;
     else if(red_string[current_col]==')')balance--;
     else if(red_string[current_col]==0)halt(CBRACKET,NULL);
     *((*str)++)=red_string[current_col++];
   }
   q_begin=0;
}/*verb*/

static char *direct_scan(char *str)
{
register char *begin_string=str;
   if (hash_enable){
     if(!((q_begin)&&(current_col==q_begin)))
      while(set_in(red_string[current_col],spaces_s)){
         if(red_string[current_col]==0){
            do{
               if(read_string()==NULL)return(NULL);
            }while((*inp_f_name)*(*red_string==comment));
         }else
           current_col++;
      }
      do{
         if((red_string[current_col]==q_char)&&(q_begin==0)&&
             (red_string[current_col+1]=='('))q_begin=current_col+2;
         if((q_begin)&&(current_col==q_begin)){
           verb(&str);
           *str=0;
           current_col--;
         }else if (set_in(red_string[current_col],spaces_s)){
            *str=0;
         }else{
           *str=red_string[current_col];

           if(set_in(red_string[current_col],delimiters_s))
             *++str=0;
         }
        if(red_string[current_col]!=0)
             if(set_in(red_string[++current_col],delimiters_s))
                *++str=0;
      }while(*str++);
   }else{
      while(((red_string[current_col])==0)||
            ((red_string[0])==comment))
         if(read_string()==NULL)return(NULL);
      do{
         if((red_string[current_col]==q_char)&&(q_begin==0)&&
             (red_string[current_col+1]=='('))q_begin=current_col+2;
         if((q_begin)&&(current_col==q_begin))verb(&str);
         if((red_string[current_col])==esc_char){
#ifdef DEV_VERV
               /*Check */
               if( (g_verbatim)&&(*g_verb_term==red_string[current_col+1]) ){
                 char *a=red_string+current_col+1;
                 char *b=g_verb_term;
                 while( (*a > ' ')||(*b > ' ') ){
                    if (*a!=*b) break;
                    a++;b++;
                 }/*while( (*a > ' ')&&(*b > ' ') )*/
                 if(     (*a==*b)||( (*a<=' ')&&(*b<=' ')  )    )/*g_verb_term*/
                   hash_enable=1;
                   !!!!
                   Too complicated!
                   I must terminate g_verbatim, check if *a !=0,
                   re-set current_col, and so on.
                   And, I must undersand, whch kind of terminator should I use:
                   \term \term()?
                   To which position should I move?
                   May be, "<t' '" better to change: NOT delimiter?
                   !!!
               }else/*if( (g_verbatim)&&(*g_verb_term==red_string[current_col+1]) */
                  hash_enable=1;
         }/*if((red_string[current_col])==esc_char)*/
         if(hash_enable){/*Enter hash mode at the next cycle*/
#endif
            hash_enable=1;
            *str=0;
            current_col--;
         }else{/*Continue non-hash mode*/
            *str=red_string[current_col];
            if(!(*str))current_col--;
         }
         current_col++;
      }while(*str++);
   }

   return(begin_string);

}/*direct_scan*/

static char *scan_and_copy(char *str)
{
  register char *tmp=str;
      if(direct_scan(str)==NULL) return(NULL);
      do
        if(b_put(*tmp))halt(BUFEROVERFLOW,NULL);
      while(*tmp++);
      return(str);
}/*scan_and_copy*/

static char *from_buf_scan(char *str)
{
 register char *tmp=str;
    while((*tmp++=b_get())!=0);
    if (b_status==-1)sc_get_token=direct_scan;
    return(str);
}/*from_buf_scan*/

void sc_mark(void)
{
#ifdef DEBUG
 if (!is_init) s_error("sc_mark: scaner not initialised.",NULL);
#endif
    if(b_status!=-1) halt("sc_mark: buffer busy.", NULL);
    sc_get_token=scan_and_copy;
}/*sc_mark*/

void sc_release(void)
{
#ifdef DEBUG
 if(!is_init)s_error("sc_release: scaner not initialised.",NULL);
#endif
   b_tail=b_buf,
   b_head=b_buf,
   b_status=-1;
   sc_get_token=direct_scan;
}/*sc_release*/

void sc_repeat(void)
{
#ifdef DEBUG
 if(!is_init)s_error("sc_repeate: scaner not initialised.",NULL);
#endif
   sc_get_token=from_buf_scan;

}/*sc_repeat*/

void realloc_include(void)
{
   if(include_depth>max_include-2)halt(NOREALLOCINCLUDE,NULL);
   if(  (include_stack=realloc(include_stack,
          sizeof(struct stack_cell)*max_include)
        ) == NULL
     )halt(NOTMEMORY,NULL);
}/*realloc_include*/

void tryexpand_include( int n)
{
  if(max_include<n){
     max_include=n;
     realloc_include();
  }
}/*tryexpand_include*/

void multiplylastinclude(int n)
{
register int i;
#ifdef DEBUG
 if (!is_init) s_error("include: scaner not initialised.",NULL);
#endif
  if(n<1)return;
  if((include_stack==NULL)||(!include_depth))
     halt(NOINCLUDESAVAIL,NULL);
  if(include_depth+n>max_include-2)halt(NOINCLUDE,NULL);
  i=include_depth-1;
  while(n--){
     include_stack[include_depth].red_string=
        new_str(include_stack[i].red_string);
     include_stack[include_depth].current_pos=
        include_stack[i].current_pos;
     include_stack[include_depth].fposition=
        include_stack[i].fposition;
     include_stack[include_depth].current_line=
        include_stack[i].current_line;
     include_stack[include_depth].current_col=
        include_stack[i].current_col;
     link_file(include_stack[i].inp_f_name,
                 include_stack[i].current_pos);
     include_stack[include_depth++].inp_f_name=
        new_str(include_stack[i].inp_f_name);

  }
}/*multiplylastinclude*/

/* Goes to the specified position of the file. If position<0 interpretes fname as a
   string to be scanned:*/
int gotomacro(char *fname,long position,long new_line,int new_col)
{
#ifdef DEBUG
 if (!is_init) s_error("include: scaner not initialised.",NULL);
#endif
  if(include_stack==NULL)realloc_include();
  if(include_depth>max_include-2)halt(NOINCLUDE,NULL);
  include_stack[include_depth].red_string=new_str(red_string);
  include_stack[include_depth].current_line=current_line;
  if(inp_file!=NULL)
     include_stack[include_depth].fposition=ftell(inp_file);
  else
     include_stack[include_depth].fposition=0;
  include_stack[include_depth].current_pos=current_pos;
  include_stack[include_depth].current_col=current_col;
  include_stack[include_depth++].inp_f_name=new_str(inp_f_name);
     if(isdebug){
        if(*inp_f_name==0){char tmp[MAX_STRING_LENGTH];
          sprintf(tmp,LEAVEFILE,current_line,current_col,red_string);
          message(tmp,NULL);
        }else{
          sprintf(red_string,LEAVEFILE,current_line,current_col,inp_f_name);
          message(red_string,NULL);
        }
        message(GOTOFILE,fname);
     }

  *red_string=0;
  if(position<0){
     *inp_f_name=0;
     inp_file=NULL;
     s_letn(fname,red_string,MAX_STRING_LENGTH-1);
     current_pos=current_col=current_line=0;
  }else{
     inp_file=link_file(fname,position);
     s_let(fname,inp_f_name);
     red_string[current_col=new_col]=0;
     current_pos=0;
     current_line=new_line;
     set_bit(&read_string_mode,bitDONTRESETCOL);
  }
  return(0);
}/*gotomacro*/

int include(char *fname)
{
  if (isdebug==0)
     message(GOTOFILE,fname);
  return(gotomacro(fname,0,0,0));
}/*include*/

one_byte add_spaces(char *thespaces)
{
  return(set_sset(thespaces,spaces_s));
}/*add_spaces*/
one_byte rm_spaces(char *thespaces)
{
  return(set_sdel(thespaces,spaces_s));
}/*rm_spaces*/
one_byte add_delimiters(char *thedelimiters)
{
  return(set_sset(thedelimiters,delimiters_s));
}/*add_delimiters*/
one_byte rm_delimiters(char *thedelimiters)
{
  return(set_sdel(thedelimiters,delimiters_s));
}/*rm_delimiters*/

int sc_init(char *init_f_name,set_of_char init_spaces,
            set_of_char init_delimiters,char init_comment,
            int init_hash_enable, char init_esc_char)
{
   if (is_init) return(1);
   set_copy(init_spaces,spaces_s);
   set_copy(init_delimiters,delimiters_s);
   set_set(0,spaces_s);
   set_sub(delimiters_s,delimiters_s,spaces_s);
   set_not(delimiters_s,regular_chars);
   set_sub(regular_chars,regular_chars,spaces_s);
   s_let(init_f_name,inp_f_name);
   inp_file=link_file(inp_f_name,0);
   comment=init_comment;
   hash_enable=init_hash_enable;
   esc_char=init_esc_char;
   red_string[0]=0;

   current_line=0;
   current_col=0;
   current_pos=0;
   q_begin=0;
   sc_get_token=direct_scan;
   is_init=1;
   return(0);
}/*sc_init*/

static int double_is_init=0;
static char *double_red_string=NULL;
static long double_current_line=0;
static long double_current_pos=0;
static long double_fposition=0;
static int double_current_col=0;
static char *double_inp_f_name=NULL;
static set_of_char double_spaces_s;
static set_of_char double_delimiters_s;
static set_of_char double_regular_chars;
static char double_comment;
static int double_hash_enable;
static char *(*double_sc_get_token)(char *str);
static char double_esc_char;
static char double_q_char;
static char double_EOF_denied;
static int double_q_begin;

int double_init(char *init_f_name,set_of_char init_spaces,
            set_of_char init_delimiters,char init_comment,
            int init_hash_enable, char init_esc_char)
{
   if (double_is_init)return(1);
   double_red_string=new_str(red_string);
   double_current_line=current_line;
   double_current_col=current_col;
   double_current_pos=current_pos;
   double_inp_f_name=new_str(inp_f_name);
   double_fposition=ftell(inp_file);
   unlink_file(inp_f_name);
   set_copy(spaces_s,double_spaces_s);
   set_copy(delimiters_s,double_delimiters_s);
   set_copy(regular_chars,double_regular_chars);
   double_comment=comment;
   double_hash_enable=hash_enable;
   double_sc_get_token=sc_get_token;
   double_esc_char=esc_char;
   double_q_char=q_char;
   double_EOF_denied=EOF_denied;
   double_q_begin=q_begin;
   set_copy(init_spaces,spaces_s);
   set_copy(init_delimiters,delimiters_s);
   set_set(0,spaces_s);
   set_sub(delimiters_s,delimiters_s,spaces_s);
   set_not(delimiters_s,regular_chars);
   set_sub(regular_chars,regular_chars,spaces_s);
   s_let(init_f_name,inp_f_name);

   inp_file=link_file(inp_f_name,0);
   comment=init_comment;
   hash_enable=init_hash_enable;
   esc_char=init_esc_char;
   red_string[0]=0;
   current_line=0;
   current_col=0;
   current_pos=0;
   q_begin=0;
   q_char=0;
   sc_get_token=direct_scan;
   double_is_init=1;

   return(0);

}/*double_init*/

int double_done(void)
{
   if (double_is_init==0) return(1);
   s_let(double_red_string,red_string);
   free(double_red_string);
   double_red_string=NULL;
   current_line=double_current_line;
   current_col=double_current_col;
   current_pos=double_current_pos;
   unlink_file(inp_f_name);
   s_let(double_inp_f_name,inp_f_name);
   free(double_inp_f_name);
   double_inp_f_name=NULL;
   inp_file=link_file(inp_f_name,double_fposition);
   set_copy(double_spaces_s,spaces_s);
   set_copy(double_delimiters_s,delimiters_s);
   set_copy(double_regular_chars,regular_chars);
   comment=double_comment;
   hash_enable=double_hash_enable;
   sc_get_token=double_sc_get_token;
   esc_char=double_esc_char;
   q_char=double_q_char;
   EOF_denied=double_EOF_denied;
   q_begin=double_q_begin;
   return(double_is_init=0);
}/*double_done*/

int sc_done(void)
{
   if(double_is_init)double_done();
   if (is_init==0) return(1);
   b_tail=b_buf,
   b_head=b_buf,
   b_status=-1;
   current_line=0;
   current_pos=0;
   sc_get_token=NULL;
   hash_enable=1;
   esc_char=ESC_CHAR;
   done();
   *inp_f_name=0;
   return(is_init=0);
}/*sc_done*/

char *get_input_file_name(void)
{
   return(inp_f_name);
}/*get_input_file_name*/
long get_current_line(void)
{
  return(current_line);
}/*get_current_line*/
int get_current_col(void)
{
  return(current_col);
}/*get_current_col*/
long get_current_pos(void)
{
  return(current_pos);
}/*get_current_pos*/

char *get_inkl_name(char *str)
{
 char tmp[MAX_STRING_LENGTH];
 int len=0;
   *str=0;
   if(*sc_get_token(tmp)!='(')halt(OBRACKEDEXPECTED,tmp);
   while(*sc_get_token(tmp)!=')'){
      if(!((len+=s_len(tmp))<MAX_STRING_LENGTH))halt(TOOLONGSTRING,NULL);
      s_cat(str,str,tmp);
   }
   return(str);
}/*get_inkl_name*/

int m_cmp(char *p1,char *p2) /* returns 1 if sets of
 identifiers are coicide*/
{
 char *a[MAX_LINES_IN_VERTEX],*b[MAX_LINES_IN_VERTEX],j=1,i=*p1;
 char k;
 if (*p1!=*p2)return(0);
 a[0]=(char*)++p1;b[0]=(char*)++p2;
 while (i-- >1){
    /* Let a[j] contain identifiers from p1:*/
    while(*(char*)p1++);a[j]=(char*)p1;
    /* Let b[j] contain identifiers from p2*/
    while(*(char*)p2++);b[j++]=(char*)p2;
 }/*now j=<number of id>, and i=0*/
 for(;i<j;i++){
    k=-1;
    while(1){
      do{
         if(++k==j)return(0);/*a[i] does not coincides with any b[k]*/
      }while(b[k]==NULL);/*skip marked b[k]*/
      if(!s_scmp(a[i],b[k])){/*mark b[k] and exit cycle*/ b[k]=NULL;break;}
    }
 }
 return(1);
}/*m_cmp*/

word m_len(char *s)/*returns number of chars required to allocate set
                          s in memory */
{
 char i=*s, tmp=0;
    while(i--)
      while(tmp++,*(char*)s++);
 return((tmp)?tmp:1);/*If s is empty, while(i--) do not executed, but
s occupais 1 byte due to terminator */
}/*m_len*/

char *m_let(char *from, char *to) /* copeis set of id <from> to <to> */
{
 char i=*from, *tmp=to;
 if (!i)*to=0;/* empty set */
 while(i--)
    while((*to++=*(char*)from++)!=0);
 return(tmp);
}/*m_let*/

char *m_cat(char *dest, char *id1, char *id2)/* concatenetes
sets id1 and id2 into dest */
{
 char *tmp, i=(*id2);
    m_let(id1,dest);
    if(*id2++){
       tmp=dest+m_len(dest);/*now tmp points just after dest terminator */
       (*dest)+=i;
       while(i--)
          while((*tmp++=*(char*)id2++)!=0);
    }
    return(dest);
}/*m_cat*/

char *m2s(char *s)/*converts set of id into sequence of id separated by
             commas. */
{
 char *tmp=(char*)s;
    while((*tmp)-- > 1){
        while(*(char*)++s); (*(char*)s)=',';
    }
    return(tmp+1);
}/*m2s*/

char *encode(long val, char *buf, int base)
{
  long a,b,c,d;
     d=base*base;/*base^2*/
     c=d*base;/*base^3*/
     a=val/c;/*val / base^3 */
     b=(c=(val % c))/d;/* (val % base ^3 ) / base ^2 */
     c=(d=(c % d))/base; /*(val % base^3 % base^2) / base */
     d=d % base; /* val % base^3 % base^2 % base */
     buf[0]=a+' ';buf[1]=b+' '; buf[2]=c+' ';buf[3]=d+' ';buf[4]=0;
     return(buf);
}/*encode*/

long decode(char *buf, int base)
{
register  long tmp=0, pw=1;
register  int i;
     for(i=3;!(i<0);i--){
        tmp+=(buf[i]-' ')*pw;
        pw*=base;
     }
     return(tmp);
}/*decode*/

char get_comment(void)
{
  return(comment);
}/*get_comment*/

void new_comment(char newcomment)
{
 comment=newcomment;
}/*new_comment*/

/*Swaps '+' and '-'; returns 0 if result is ready, or 1 if it shuld be prependen by '-':*/
int swapCharSng(char *m)
{
int i;
   switch(*m){
      case '\0':
      case '-':
      case '+':i=0;break;
      default:i=1;break;
   }/*switch*/
   for(;*m!='\0';m++)switch(*m){
      case '+':*m='-';break;
      case '-':*m='+';break;
   }/*for(;*m!='\0';m++)switch(*m)*/
   return i;
}/*swapCharSng*/

/* Set the FD_CLOEXEC  flag of desc if value is nonzero,
 or clear the flag if value is 0.
 Return 0 on success, or -1 on error with errno  set. */
int set_cloexec_flag (int desc, int value)
{
int oldflags = fcntl (desc, F_GETFD, 0);
   /* If reading the flags failed, return error indication now.*/
   if (oldflags < 0)
      return oldflags;
   /* Set just the flag we want to set. */
   if (value != 0)
      oldflags |= FD_CLOEXEC;
   else
     oldflags &= ~FD_CLOEXEC;
   /* Store modified flag word in the descriptor. */
   return fcntl (desc, F_SETFD, oldflags);
}/*set_cloexec_flag*/
