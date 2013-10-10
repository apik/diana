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
/*MACRO.C*/

/*!ATTENTION!
Through this file the term "macro" is used to denominate "operator"! 
(historical reasons).*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>
#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include "tools.h"
#include "comdef.h"
#include "types.h"
#include "variabls.h"
#include "headers.h"
#include "texts.h"
#include "tt_type.h"
#include "tt_gate.h"
#include "rproto.h"
#define MACROS
#include "trunsrun.h"

/*!ATTENTION!
Through this file the term "macro" is used to denominate "operator"! 
(historical reasons).*/

/* To create new macros, you should:
1. Write function char *MEXPAND(char *arg).
   This function must returns pointer to "arg".
2. Register your function using function
   void register_macro(char *name,char arg_numb,
                            int flags,MEXPAND *mexp)
   in macro_init().
   Here "name" is the name of your macro, "agr_numb" is the amount of
   its arguments, "flags" is flag ( if runtime error
   may occure, bit 0 is set to 1, if not, set it to 0; bit 1 is set to 1
   for goto and labelsreset), mexp -- your macro
   function.

   All arguments available by means of function char *get_string(char *buff)
   in form of ASCII-Z string.
   The function get_string() returns successive argument from the stack.
   REMEMBER: arguments pop from the stack by this function in inverse
   order, i.e., the first argument is actually the last.

   Pure numerical (integer) arguments can be extracted by means of the
   function long get_num(char *name,char *buf), where "name" iz the name
   of macro which invokes this function, and "buf" is a buffer for error
   message. In case of invalid numeric format, this function halts a
   system complaining of the macro "name".

   If you finde impossible situation, for example, if
   get_string() returns impossible argument, you may generate runtime
   error using function void halt(char *fmt, ...).
   This function types diagnostic as fprintf(stderr,s1,s2)  and halts
   program. You need not in your message '\n', or "error". This texts
   will appears automatically.
*/

extern char *currentRevisionNumber;/*pilot.c*/
extern int total_length;/*run.c*/

static int scanner_eof=0;
static int readln_eof=0;

static int stackdepth=0;
static char mem_q_char;
static set_of_char check_set;
static char delimiter_char=':';

extern tt_table_type **tt_table;

static int str2dir(char *str, char *dir, char pluschar, char minuschar);

#ifdef SKIP
/*The following function maps hexadecimal string kinda 0FAD01... to string
  representation subst,dir. It expets each pair of input string 'str' be a
  hexadecimal representation of SIGNED char. Its absolute value is stored
  into 'subst' while its sign is stored into 'dir' (the latter is stored
  only if dir!=NULL). First element of 'subst' and 'dir' is always -1.
  This function controls the validity of substitution.

  The function returns the length of substitution. In case of an error
  it returns error code <0:
  -1 Invalid input character
  -2 Wrong alingment
  -3 Too long pattern (the length of a built substitution does not fit maxN chars)
  -4 Invalid pattern

  On error, both subst and dir are not terminated!
 */
static int hex2substAndDir(char *str, char *subst, char *dir,int maxN)
{
int l=0,/*The length*/
    b=16,/*The base shift: 1 or 16*/
    n,/*one radix digit*/
    t=0;/*Character to be added*/
char maxC='\0';
set_of_char uniqueCh;

   set_sub(uniqueCh,uniqueCh,uniqueCh);/* clear uniqueCh*/

   /*First byte must be -1:*/
   *subst=-1;
   if(dir!=NULL)*dir=-1;

   for(;l<maxN;l++){
      switch(*str++){
         case '\0':/*Input string is expired.*/
            /*terminate building strings:*/
            *++subst='\0';
            if(dir!=NULL)*++dir='\0';

            /*Note, in the following errors built strings are terminated:*/
            if(b!=1)return -2;/*Wrong alingment*/
            if(l!=maxC)
               return -4;/*Invalid pattern*/

            /*Ok, normal return:*/
            return l;/*Return the number of elements */

         case '0':n=0;break;
         case '1':n=1;break;
         case '2':n=2;break;
         case '3':n=3;break;
         case '4':n=4;break;
         case '5':n=5;break;
         case '6':n=6;break;
         case '7':n=7;break;
         case '8':n=8;break;
         case '9':n=9;break;
         case 'a':case 'A':n=10;break;
         case 'b':case 'B':n=11;break;
         case 'c':case 'C':n=12;break;
         case 'd':case 'D':n=13;break;
         case 'e':case 'E':n=14;break;
         case 'f':case 'F':n=15;break;
         default:return -1;/*Invalid input character*/
      }/*switch(*str)*/

      if(b==16){/**/
         t = n * 16;
         b=1;
      }else{/*b==1*/
         t+=n;
         b=16;
         subst++;
         if(dir!=NULL)dir++;
         if(t<128){
            if(t==0)return -4;/*Invalid pattern*/
            *subst=t;
            if(dir!=NULL)*dir=1;
         }else{
            *subst=256-t;
            if(dir!=NULL)*dir=-1;
         }/*if(t<128)...else...*/
         if( set_in(*subst,uniqueCh) )
            return -4;/*Invalid pattern*/
         set_set(*subst,uniqueCh);

         if(maxC < *subst)maxC=*subst;

      }/*if(b==16)...else...*/
   }/*for(;l<maxN;l++)*/
   return -3; /* Too long pattern*/
}/*hex2substAndDir*/
#endif

static void callstackexpand(void)
{
 if((callstack=realloc(callstack,
                        (max_callstacktop+=DELTA_STACK_SIZE)*sizeof(char)
                      ))==NULL) halt(NOTMEMORY,NULL);
}/*callstackexpand*/

void string2callstack(char *str)
{
  if (!(callstacktop<max_callstacktop))callstackexpand();
  callstack[callstacktop++]=TERM;
  while(*str!=0){
     if (!(callstacktop<max_callstacktop))callstackexpand();
     callstack[callstacktop++]=*str++;
  }
}/*string2callstack*/

char *popstring( char *str)
{
  int tmp=callstacktop,i;
  char *mem=str;
    while(callstack[--callstacktop]!=TERM);
    for(i=callstacktop+1;i<tmp;i++)
                 *str++ =callstack[i];
    *str=0;
    return(mem);
}/*popstring*/

static void domodesave(int *tmp)
{
   *tmp=0;
   if(offleadingspaces)
      set_bit(tmp,0);
   if(offtailspaces)
      set_bit(tmp,1);
   if(offblanklines)
      set_bit(tmp,2);
   if(offout)
      set_bit(tmp,3);
}/*domodesave*/

static void domoderestore(int *tmp)
{
   if(is_bit_set(tmp,0))
      offleadingspaces=1;
   else
      offleadingspaces=0;
   if(is_bit_set(tmp,1))
      offtailspaces=1;
   else
      offtailspaces=0;
   if(is_bit_set(tmp,2))
      offblanklines=1;
   else
      offblanklines=0;
   if(is_bit_set(tmp,3))
      offout=1;
   else
      offout=0;
}/*domoderestore*/

/*Here the special images (i.e., the images specified or some linemark) will be stored:*/
static char *l_specialImages[MAX_I_LINE +MAX_E_LINE+1];
static int l_specialImagesTop=0;

/************** Begin macro ******************/
static char *macversion(char *arg)
{
   return(s_let(currentRevisionNumber,arg));
}/*macversion*/

static int l_strcmp(const void *sample1,const void *sample2)
{
register char *s1=*(char**)sample1,*s2=*(char**)sample2;
    while(*s1==*s2++){if(!(*s1++))return(0-*--s2);}
    return(*s1-*--s2);
}/*l_strcmp*/

/*\qsort(strings,newseparator) sorts 'strings'
lexicographically. Assumes that 'strings' is an array of strings
separated by a symbol ':'. After sorting it returns the result as
array of strings separated by a charater 'newseparator'.
*/
static char *macqsort(char *arg)
{
char **arr,*tmp;
char *buf;
char sep;
int i,l=0;
   sep=*get_string(arg);
   buf=get_mem(MAX_STR_LEN,sizeof(char));
   if( (i=s_len(get_string(buf)))>0 ){
      arr=get_mem(i,sizeof(char*));
      arr[l++]=tmp=buf;
      while(*tmp!='\0')switch(*tmp){
         case ':':
           arr[l++]=tmp+1;
           *tmp='\0';
         default:
           tmp++; 
           break;
         case '\0':
           break;
      }
      qsort(arr,l,sizeof(char*),l_strcmp);

      tmp=arg;
      for(i=0;i<l;i++){
         if(i)
            *tmp++=sep;
         s_let(arr[i],tmp);
         while(*tmp!='\0')tmp++;
      }
      free(arr);
   }else
     *arg='\0';

   free(buf);
   return arg;
}/*macqsort*/

/*\random(A,B) - returns random integer between A and B*/
static char *macrandom(char *arg)
{
double up=get_num("random",arg);
double low=get_num("random",arg);
struct timeval t;
   /*Initialize random seed by timer:*/
   gettimeofday(&t,NULL); srand((unsigned int)t.tv_usec);
   return long2str(arg,(int)(low+rint((up-low)/RAND_MAX*rand())));
}/*macrandom*/

/*parseParticleImage(particle,string) - converts Diana-specific string into a PS form:
   e.g.:
   W{y(10){f(Symbol)(10)a}}  -> $W^\alpha$ ( in TeX notations!  Actually will be outputted
   in PS notations.)

   All modifications are local agains block. Block is started by:
   {x(#) ... } - paints the content with shift along x. After the block, the current point
                is set to (old x, new y)
   {y(#) ... } - paints the content with shift along y. After the block, the current point
                 is set to (new x, old y)
   {xy(#)(#) ... } -  paints the content with shift along  both x and y. After the block, the current point
                   is set to (old x, old y)
   {f(fontname)(#) ... } set font "fontname" scaled by # (in 1/multiplier fractions
                         of fsize)
   {s(#) ... } - scale current font by # (in 1/multiplier fractions of fsize)
   {c(#)(#)(#) ... }  set RGB color

   show is the PS command to paint a string,  fsize is the base font size.
   All sizes are in 1/multiplier fractions of fsize.
*/
static char *macparseParticleImage(char *arg)
{
int nb,i;
char *img,*partid=arg+1;
char **fonts=NULL;
   nb=parse_particle_image(FSIZE,MULTIPLIER,SHOW,get_string(arg),&img,&fonts);

   get_string(partid);/*arg+1 must be a particle id, or something else.*/
      *arg='\1';

   if( lookup(arg,main_id_table)==NULL ){/*This is some special!*/
      if(l_specialImagesTop >MAX_I_LINE +MAX_E_LINE)
         halt(TOOMANYSPECIALIMAGES);
      l_specialImages[l_specialImagesTop++]=new_str(partid);
   }/*if( lookup(arg,main_id_table)==NULL )*/

   if(nb<0){/* Parsing error*/
      char buf[MAX_STR_LEN];
      nb=-nb-1;/*nb is a position at which parsing failes*/
      for(i=0;i<nb;i++)buf[i]=' ';
      buf[i++]='^';buf[i]='\0';
      /* on error, img is a pointer to a STATIC string containing diagnostics:*/
      halt("%s:\n%s\n%s ",img,partid,buf);
   }/*if(nb<0)*/
   i=0;
   if(fonts!=NULL){
      /*Set information about each font (export var _PSf_<Particle>_# , font)*/
      char buf[MAX_STR_LEN];
      for(;fonts[i]!=NULL;i++){
         sprintf(buf,"_PSf_%s_%d",partid,i+1);
         set_export_var(buf,fonts[i]);
         free(fonts[i]);
      }/*for(i=0;fonts[i]!=NULL;i++)*/
      free(fonts);
   }/*if(fonts!=NULL)*/
   /*Now i is equal to <number of non-standart fonts> + 1*/
   /*Clear old information, if present:*/
   {/*block*/
   char buf[MAX_STR_LEN];
      do
         sprintf(buf,"_PSf_%s_%d",partid,++i);
      while(uninstall(buf,export_table)==0);
   }/*block*/

   if(s_len(img)>MAX_STR_LEN-64){
      free(img);halt(TOOLONGSTRING,NULL);
   }/*if(s_len(img)>MAX_STR_LEN-64)*/

   s_let(img,arg);/*Save the result*/
   free(img);
   return arg;
}/*macparseParticleImage*/

static char *macgetenv(char *arg)/*"\getenv(name)*/

{
char *tmp=getenv(get_string(arg));
  if(tmp==NULL)
     *arg=0;
  else
     s_let(tmp,arg);
  return(arg);
}/*macgetenv*/

/*\fullcmdline() returns full command line Diana is invoked:*/
static char *macfullcmdline(char *arg)
{
  s_letn(full_command_line,arg,MAX_STR_LEN);
  return(arg);
}/*macfullcmdline*/

/*\syscmdparam(n) returns cmd line parameter number "n" passed to Diana,
  or empty string. In contrast, \cmdline(n)  returns n cmd line passed to run.*/
static char *macsyscmdparam(char *arg)
{
long num;
  num=get_num("syscmdparam",arg);
  if( (num<0) || (!(num<g_argc)) )
     *arg=0;
  else
     s_let(g_argv[num],arg);
  return(arg);
}/*macsyscmdparam*/

static double current_time[10]={0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
static int current_clock=0;
static int is_ticks=0;
static clock_t start_time[10];

static char *macclockreset(char *arg)
{
   if(is_bit_set(&is_ticks,current_clock)){
     current_time[current_clock] += (double)(clock()-start_time[current_clock]);
     unset_bit(&is_ticks,current_clock);
   }
   sprintf(arg,"%f",current_time[current_clock]/CLOCKS_PER_SEC);
   current_time[current_clock]=0.0;
   return(arg);
}/*macclockreset*/

static char *macclockstart(char *arg)
{
   if(is_bit_set(&is_ticks,current_clock))
     current_time[current_clock] += (double)(clock()-start_time[current_clock]);
   sprintf(arg,"%f",current_time[current_clock]/CLOCKS_PER_SEC);
   set_bit(&is_ticks,current_clock);
   start_time[current_clock]=clock();
   return(arg);
}/*macclockstart*/

static char *macclockstop(char *arg)
{
   if(is_bit_set(&is_ticks,current_clock)){
     current_time[current_clock] += (double)(clock()-start_time[current_clock]);
     unset_bit(&is_ticks,current_clock);
   }
   sprintf(arg,"%f",current_time[current_clock]/CLOCKS_PER_SEC);
   return(arg);
}/*macclockstop*/

static char *macclocksetactive(char *arg)
{
long int num;
  if(((num=get_num("clocksetactive",arg))>10)||(!(num>0)))
     halt(INVALIDOUTNUMBER,arg);
  current_clock=num-1;
  *arg=0;
  return(arg);
}/*macclocksetactive*/

/* \tr(from, to, str) -- Changes all characters
of 'str' matches 'from' to corresponding from 'to':*/
static char *mactr(char *arg)
{
char tbl[256],from[MAX_STR_LEN],to[MAX_STR_LEN];
register int i;
   get_string(arg);
   get_string(to);
   get_string(from);
   for(i=0; i<256; i++)tbl[i]=i;
   for(i=0; i<256; i++){
      if ((from[i]=='\0')||(to[i]=='\0'))break;
      tbl[from[i]] = to[i];
   }
   for(i=0;arg[i]!='\0'; i++)
      arg[i]=tbl[arg[i]];
   return(arg);
}/*mactr*/

/*\cut(string,n) returns field of index n, n started from 1, fields are separated arbitrary
numbers of spaces:*/
static char *maccut(char *arg)
{
int n=get_num("cut",arg);
char *ptr=get_string(arg);

   for(;n>0;n--){
      while(*ptr<=' ')if(*ptr++=='\0'){
         *arg='\0';
         return arg;
      }
      if(n==1)break;
      while(*ptr>' ')ptr++;
   }/*for(;n>0;n--)*/
   for(n=0;*ptr>' ';ptr++)
      arg[n++]=*ptr;
   arg[n]='\0';
   return arg;
}/*maccut*/

/* \delete(str,chars) -- delete all characters
of 'str' matches 'chars':*/
static char *macdelete(char *arg)
{
char str[MAX_STR_LEN];
set_of_char chars;
register char *s=str,*a=arg;
   set_str2set(get_string(arg),chars);
   get_string(str);
   for(;*s != '\0';s++)if(!set_in(*s,chars))*a++=*s;
   *a='\0';
   return(arg);
}/*macdelete*/

static char *macmodesave(char *arg)
{
int *tmp;
   if(!(top_modestack<max_modestack)){
      if(max_modestack>MAXMODESTACK)
         halt(MODESAVESTACKOVERFLOW,NULL);
      if(
         (modestack=realloc(modestack,
           (max_modestack+=DELTAMODESTACK)*sizeof(int))
         )==NULL
        )halt(NOTMEMORY,NULL);
   }
   tmp=modestack+top_modestack;
   top_modestack++;
   domodesave(tmp);
   *arg=0;
   return(arg);
}/*macmodesave*/

static char *macmoderestore(char *arg)
{
int *tmp;
   if (!(top_modestack>0))
      halt(NOSAVEDMODES,NULL);
   top_modestack--;
   tmp=modestack+top_modestack;
   domoderestore(tmp);
   *arg=0;
   return(arg);
}/*macmoderestore*/

static char *mac_modesave(char *arg)
{
int *tmp;
   if(!(p_top_modestack<p_max_modestack)){
      if(p_max_modestack>MAXMODESTACK)
         halt(MODESAVESTACKOVERFLOW,NULL);
      if(
         (p_modestack=realloc(p_modestack,
           (p_max_modestack+=DELTAMODESTACK)*sizeof(int))
         )==NULL
        )halt(NOTMEMORY,NULL);
   }
   tmp=p_modestack+p_top_modestack;
   p_top_modestack++;
   domodesave(tmp);
   *arg=0;
   return(arg);
}/*mac_modesave*/

static char *mac_moderestore(char *arg)
{
int *tmp;
   if (p_top_modestack>0){
      p_top_modestack--;
      tmp=p_modestack+p_top_modestack;
      domoderestore(tmp);
   }
   *arg=0;
   return(arg);
}/*macmodesave*/

static char *macsetcheck(char *arg)/*"setcheck(string)-sets set for "check."*/
{
  set_str2set(get_string(arg), check_set);
  *arg=0;
  return(arg);
}/*macsetcheck*/

static char *macgetcheck(char *arg)/*"getcheck(string)-returns current
                                            set for "check."*/
{
  return(set_set2str(check_set,arg));
}/*macgetcheck*/

static char *maccheck(char *arg)/*"check(string) - checks
 are all sumbols in the set defined by "setcheck."*/
{
  char *tmp;
  for(tmp=get_string(arg);*tmp;tmp++)
    if(!set_in(*tmp,check_set)){
       return(s_let("false",arg));
    }
  return(s_let("true",arg));
}/*maccheck*/

static char *macsymbolpresent(char *arg)/*"symbolpresent(string) - checks
 are there sumbols in the set defined by "setcheck."*/
{
  char *tmp;
  for(tmp=get_string(arg);*tmp;tmp++)
    if(set_in(*tmp,check_set)){
       return(s_let("true",arg));
    }
  return(s_let("false",arg));
}/*macsymbolpresent*/

static char *maccompile(char *arg)/*"compile(filename) - run-time compiler."*/
{
long number_of_trunslated_lines=0;
char **array_of_trunslated_lines=NULL;
   scaner_init(get_string(arg),cnf_comment);
   esc_char=q_char=mem_esc_char;
   do{
      EOF_denied=1;
      if(truns(&number_of_trunslated_lines,
                  &array_of_trunslated_lines))
         halt(CANNOTRESETPRG,NULL);
      sc_mark();
      EOF_denied=0;
      if(sc_get_token(arg)==NULL)break;
      sc_repeat();
   }while(1);

   macro_done();/*module macro.c*/
   clear_label_stack();/*module truns.c*/
   for_done();/*module truns.c*/
   IF_done();/*module truns.c*/
   sc_done();
   *arg=0;
   return(arg);
}/*maccompile*/

static char *macbackslash(char *arg)
{
   *arg='\\';arg[1]=0;
   return(arg);
}/*macbackslash*/

/*\scannerinit(filename,regularcharacters, comment) */
static char *macscannerinit(char *arg)
{
   set_of_char spaces,delimiters;
   char j, comment;
      comment=*get_string(arg);
      set_sub(spaces,spaces,spaces);/* clear spasces */
      /*assume all ASCII codes <=' ' as spaces:*/
      for (j=0;!(j>' ');j++)set_set(j,spaces);

      /* set separators:*/
        /* 1. set all NOT separators: */
      set_str2set(get_string(arg),delimiters);

      set_set(0,delimiters); /* 2. add 0 */
      /* 3. inverse d -- now d containes separators and spaces */
      set_not(delimiters,delimiters);
      /* 4. remove spaces from d. That's all. */
      set_sub(delimiters,delimiters,spaces);
      /* 0-wihtout comments, 1-hash_enable, 0-without escape character.*/
      mem_q_char=q_char;
      if(sc_init(get_string(arg),spaces,delimiters,comment,1,0)){
         if(double_init(arg,spaces,delimiters,comment,1,0))
            halt(SCANNERBUSY,NULL);
         is_scanner_double_init=1;
      }
      EOF_denied=0;/* EOF now enable*/
      q_char=0;/*Avoid quotation mechanism*/
      *arg=0;
      is_scanner_init=1;
      scanner_eof=0;
      return(arg);
}/*macscannerinit*/

static char *macscannerdone(char *arg)
{
  if(!is_scanner_init)
     halt(SCANNERNOTINIT,NULL);
  if(is_scanner_double_init){
    double_done();
    is_scanner_double_init=0;
  }else
    sc_done();
  is_scanner_init=0;
  esc_char=mem_esc_char;
  q_char=mem_q_char;
  *arg=0;
  return(arg);
}/*maccannerdone*/

static char *macgetscanline(char *arg)
{
 if(!is_scanner_init)
    halt(SCANNERNOTINIT,NULL);
 return(long2str(arg,get_current_line()));
}/*macgetscanline*/

static char *macgettoken(char *arg)
{
 if(!is_scanner_init)
    halt(SCANNERNOTINIT,NULL);
 if(sc_get_token(arg)==NULL){
     if(scanner_eof)
        halt(UNEXPECTEDEOF,NULL);
     scanner_eof=1;
     *arg=chEOF;arg[1]=0;
 }
 return(arg);
}/*macgettoken*/

static char *maccall(char *arg)/* \call(proc) -- call procedure*/
{
 char seg, offset;
    {/*Block begin*/
    struct macro_hash_cell *cellptr;
       /* Looking for procedure name:*/
       if((cellptr=lookup(get_string(arg),proc_table))==NULL){
          if((cellptr=lookup(s_let("default",arg),proc_table))==NULL)
             halt(PROCNOTFOUND,arg);
       }
       /*Now cellptr points to the cell with procedure.*/

       /* Check number of arguments:*/
       if(cellptr->arg_numb>stackdepth)
          halt(STACKUNDERFLOW,NULL);
       /*Copy arguments from special stack to datastack:*/
       if(cellptr->arg_numb)
            stack2stack(cellptr->arg_numb);

       stackdepth-=cellptr->arg_numb;
       /*Calculate segment:*/
       seg=(cellptr->num_mexpand/127)+1;
       /*Calculate offset:*/
       offset=(cellptr->num_mexpand % 127)+1;

    }/*Block end*/
    return(pexec(seg, offset, arg));
}/*maccall*/

/* Pushs its argument in the special stack and returns it:*/
static char *macpush(char *arg)
{
  stackdepth++;
  string2callstack(get_string(arg));
  return(arg);
}/*macpush*/

/*Pops a string from the special stack and returns it:*/
static char *macpop(char *arg)
{
  if((stackdepth--) == 0)
     halt(STACKUNDERFLOW,NULL);
  return(popstring(arg));
}/*macpop*/

/*"replace(substr,newsubstr,str)":
 replace 1st occurance of the substr into str to newsubstr:*/
static char *macreplace(char *arg)
{
 char arg1[MAX_STR_LEN],arg2[MAX_STR_LEN];
 int totlen=0;
   get_string(arg);totlen+=total_length;
   get_string(arg2);totlen+=total_length;
   get_string(arg1);totlen-=total_length;
   if (totlen>MAX_STR_LEN)
      halt(TOOLONGSTRING,NULL);
   return(s_replace(arg1,arg2,arg));
}/*macreplace*/

static char *maclen(char *arg)/*"len": length of a string*/
{
   get_string(arg);/*get_string sets global variable total_length:*/
   return(long2str(arg,total_length-1));
}/*maclen*/

static char *macpos(char *arg)/*"pos": first occurrence of a substring
                     (first argument) in another string (second argument)*/
{
 int i;
 char arg2[MAX_STR_LEN];
   get_string(arg2);get_string(arg);
   i=s_pos(arg,arg2);
   return(long2str(arg,i));
}/*macpos*/

static char *machowmany(char *arg)/*"howmany(substr,str)":
                                  how many times substr occures in str.*/
{
 int i;
 char arg1[MAX_STR_LEN];
   get_string(arg1);get_string(arg);
   i=s_count(arg,arg1);
   return(long2str(arg,i));
}/*machowmany*/

static char *l_copy(char *from, char *to, int index,int count)
{
 char *f,*t=to;
 register int i=0;
    if  (  (s_len(from)<index) || (index<0) || (count<0)  )
        *to=0;
    else{
      for (f=(char*)from+index;((*f))&&(i++<count);*t++=*f++);
      *t=0;
    }
    return(to);
}/*l_copy*/

static char *maccopy(char *arg)/*"copy". Arguments: string, index,count.
             Returned varue: substring of the "string", starting from "index"
             and length of "count" .*/
{
 int index,count;
 char arg1[MAX_STR_LEN];
   get_string(arg1);
   if(sscanf(arg1,"%d",&count)!=1)halt(INVCOUNT,NULL);
   get_string(arg1);
   if(sscanf(arg1,"%d",&index)!=1)halt(INVINDEX,NULL);
   get_string(arg1);
   return(l_copy(arg1,arg,index,count));
}/*maccopy*/

static char *maceol(char *arg)/*"eol". No arguments, returns EOL symbol.*/
{
   return(s_let("\n",arg));
}/*maceol*/

static char *maceof(char *arg)/*"eof". No arguments, returns EOF symbol.*/
{
*arg=chEOF;arg[1]=0;
   return(arg);
}/*maceof*/

static char *maclet(char *arg)/*"let". Lets second argument to variable
                                 named as first argument and returns it.*/
{
  char arg1[MAX_STR_LEN];
     if (variables_table==NULL)variables_table=create_hash_table(
                      var_hash_size,str_hash,str_cmp,c_destructor);
     get_string(arg);get_string(arg1);
     install(new_str(arg1),new_str(arg),variables_table);
     return(arg);
}/*maclet*/

/*Comman part of seveal procedures, just clears hash table:*/
static char *l_freeTable(char *arg,HASH_TABLE *tbl)
{
    if ( (*tbl)!=NULL){
       hash_table_done(*tbl);
       (*tbl)=NULL;
    }
    *arg=0;
    return(arg);
}/*l_freeTable*/

/*"clear".Clears all variables, returns empty srting:*/
static char *macclear(char *arg)
{
   return l_freeTable(arg,&variables_table);
}/*macclear*/

static char *macexport(char *arg)/*"expotr". Stores second argument to
                           variable named as first argument and returns it.*/
{
  char arg1[MAX_STR_LEN];
     if (export_table==NULL)export_table=create_hash_table(
                      export_hash_size,str_hash,str_cmp,c_destructor);
     get_string(arg);get_string(arg1);
     install(new_str(arg1),new_str(arg),export_table);
     return(arg);
}/*macexport*/

/*"free".Clears all exported variables, returns empty srting:*/
static char *macfree(char *arg)
{
   return l_freeTable(arg,&export_table);
}/*macfree*/

static char *macblank(char *arg)/*"blank". Returns empty string.*/
{
  *get_string(arg)=0;
  return(arg);
}/*macblank*/

static char *macget(char *arg)/*"get". Returns value of its argument stored
                                 by "let".*/
{
  char *tmp;
     if ((tmp=lookup(get_string(arg),variables_table))==NULL)
        if ((tmp=lookup("default",variables_table))==NULL)
            halt(UNDEFINEDVAR,arg);
     return(s_let(tmp,arg));
}/*macget*/

static char *macgoto(char *arg)/*"goto". Transfers control to the label
                                     which is coincides with its argument.
                                  Returns empty string.*/
{
  long *tmp;
  char arg1[MAX_STR_LEN];
     get_string(arg);/*Current labels level*/
     arg[4]=chLABELSEP;
     get_string(arg+5);/*!!! Unchecked! Possible overflow!!*/
     if ((tmp=lookup(arg,labels_tables))==NULL){
        s_let(arg, arg1);
        s_let("default",arg1+5);
        if ((tmp=lookup(arg1,labels_tables))==NULL)
            halt(UNDEFINEDLABEL,arg+5);
     }
     *current_instruction_address=*tmp;/* Change numebr of current
                    inctruction!*/
     *arg=0;
     return(arg);
}/*macgoto*/

static char *macrelabel(char *arg)/*"relabel(oldlabel, newlabel)"
                             changes "oldlabel" to "newlabel" in current
                             label context. Returns empty string.*/
{
  long *tmp;
  char arg1[MAX_STR_LEN];
     get_string(arg);/*Current labels level*/
     arg[4]=chLABELSEP;arg[5]=0;
     s_let(arg, arg1);
     get_string(arg1+5);/*newlabel*//*!!! Unchecked! Possible overflow!!*/
     get_string(arg+5);/*oldlabel*/
     if ((tmp=lookup(arg,labels_tables))!=NULL){
        long position=*tmp;
           uninstall(arg,labels_tables);
           *(tmp=get_mem(1,sizeof(long)))=position;
           install(new_str(arg1),tmp,labels_tables);
     }
     *arg=0;
     return(arg);
}/*macrelabel*/

static char *macrmlabel(char *arg)/*"rmlabel(label)" removes label.
                                                    Returns empty string*/
{
   get_string(arg);/*Current labels level*/
   arg[4]=chLABELSEP;
   get_string(arg+5);/*get label*//*!!! Unchecked! Possible overflow!!*/
   uninstall(arg,labels_tables);
   *arg=0;
   return(arg);
}/*macrmlabel*/

static char *macislabelexist(char *arg)/*"islabelexist(label)"
                returns true ifthe label exists. Othrwise, returns false */
{
   get_string(arg);/*Current labels level*/
   arg[4]=chLABELSEP;
   get_string(arg+5);/*get label*//*!!! Unchecked! Possible overflow!!*/
   if (lookup(arg,labels_tables)==NULL)
       s_let("false",arg);
     else
       s_let("true",arg);
   return(arg);
}/*macislabelexist*/

static char *macimport(char *arg)/*"import". Returns value of its argument
                                   stored by "export".*/
{
  char *tmp;
     if ((tmp=lookup(get_string(arg),export_table))==NULL)
        if ((tmp=lookup("default",export_table))==NULL)
           halt(UNDEFINEDVAR,arg);
     return(s_let(tmp,arg));
}/*macimport*/

static char *macexist(char *arg)/*"exist". Returns 'true' if
               argument is  exists in table of variables,otherwice,
               returns 'false'.*/
{
     if (lookup(get_string(arg),variables_table)==NULL)
        s_let("false",arg);
     else
        s_let("true",arg);
     return(arg);
}/*macexist*/

static char *macnumcmp(char *arg)/*"numcmp": Compares numerical values of
  arguments. Returns '<', '>' or '='.*/
{
 long int num;
   num=get_num("numcmp",arg) - get_num("numcmp",arg);
   arg[1]='\0';
   if(num<0)*arg='>';/*s_let(">",arg);*/
   else if(num>0)*arg='<';/*s_let("<",arg);*/
   else *arg='=';/*s_let("=",arg);*/
   return(arg);
}/*macnumcmp*/

static char *macsum(char *arg)/*"sum": returms numerical sum of arguments.*/
{
   return long2str(arg,get_num("sum",arg)+get_num("sum",arg));
}/*macsum*/

static char *macsub(char *arg)/*"sub": returms numerical subtraction
                                                         of arguments.*/
{
 long a,b;
   b=get_num("sub",arg);
   a=get_num("sub",arg);
   return(long2str(arg,a-b));
}/*macsub*/

static char *macmul(char *arg)/*"mul": returns numerical product
                                                         of arguments.*/
{
   return(long2str(arg,get_num("mul",arg)*get_num("mul",arg)));
}/*macmul*/

static char *macdiv(char *arg)/*"div": returns result of integer division
                                                         of arguments.*/
{
 long a,b;
   b=get_num("div",arg);
   if (b==0)halt(DIVBYZERO,"div");
   a=get_num("div",arg);
   return(long2str(arg,a / b));
}/*macdiv*/

static char *macmod(char *arg)/*"mod": returns remander of integer division
                                                         of arguments.*/
{
 long a,b;
   b=get_num("mod",arg);
   if (b==0)halt(DIVBYZERO,"mod");
   a=get_num("mod",arg);
   return(long2str(arg,a % b));
}/*macmod*/

static int l_bitwise(char *arg,long int b)
{
char c[5]={'\0','\0','\0','\0','\0'};
long int a;
   s_letn(arg,c,5);
   {/*Block*/
      char *tmp;
      a=strtol(get_string(arg),&tmp,10);
      if(   ( (*arg)=='\0')||( (*tmp) !='\0' )   ){
          switch(*c){
             case 'n':case 'N':
                switch(c[1]){
                   case 'o':case 'O':
                      switch(c[2]){
                         case 't':case 'T':
                            if (c[3]=='\0')
                               return ~b;
                      }/*switch(c[2])*/
                }/*switch(c[1]:)*/
          }/*switch*/
          halt(NUMERROR,"bitwise",arg);
      }/*if( (*tmp) !='\0' )*/
   }/*Block*/

   switch(*c){
      case 'a':case 'A':
         switch(c[3]){
            case '\0':/*and*/
               return a&b;
            case 'n': case 'N':/*andn*/
               return a&(~b);
         }/*switch(c[3])*/
         break;
      case 'o':case 'O':
         switch(c[2]){
            case 'n':case 'N':/*orn*/
               return a|(~b);
            case '\0':/*or*/
               return a|b;
         }/*switch(c[2])*/
         break;
      case 'x':case 'X':
         switch(c[3]){
            case '\0':/*xor*/
               return a^b;
            case 'n':case 'N':/*xorn*/
               return a^(~b);
         }/*switch(c[3])*/
         break;
      case 'n':case 'N':
         switch(c[2]){
            case 'n':case 'N':/*nand*/
               return ~(a&b);
            case 'r':case 'R':/*nor*/
               return ~(a|b);
            case 'o':case 'O':/*nxor*/
               return ~(a^b);
            case 't':case 'T':/*not*/
               return ~b;
            case 'p':case 'P':/*nop*/
               return a;
         }/*switch(c[2]){*/
   }/*switch(*c)*/
   return b;
}/*l_bitwise*/

/*\bitwise(A, op, B) returns result of bitwise operation between integers A and B
(A may be omitted for unary operation "not"):
  \bitwise(A,and,B)
  \bitwise(A,or,B)
  \bitwise(A,xor,B)
  \bitwise(,not,B)
  \bitwise(A,nand,B)
  \bitwise(A,nor,B)
  \bitwise(A,nxor,B)
  \bitwise(A,xorn,B)
  \bitwise(A,andn,B)
  \bitwise(A,orn,B)
  \bitwise(A,nop,B) */
static char *macbitwise(char *arg)
{
long int i=get_num("bitwise",arg);
   return long2str(arg,l_bitwise(get_string(arg),i));
}/*macbitwise*/

static char *macsuspendout(char *arg)/*"suspendout": suspend output*/
{
  struct mem_outfile_struct *tmp=suspendedfiles;
  suspendedfiles=get_mem(1,sizeof(struct mem_outfile_struct));
  suspendedfiles->next=tmp;
  suspendedfiles->outfile=outfile;
  flush();
  outfile=NULL;
  *arg='\0';
  return arg;
}/*macsuspendout*/

static char *macrestoreout(char *arg)/*"restoreout": restore last
                                         suspended output*/
{
  struct mem_outfile_struct *tmp=suspendedfiles;
  if (suspendedfiles==NULL)
     halt(NOSUSPENDEDFILES,NULL);
  suspendedfiles=suspendedfiles->next;
  flush();
  if((outfile!=NULL)&&(outfile!=stdout))close_file(&outfile);
  outfile=tmp->outfile;
  free_mem(&tmp);
  *arg='\0';
  return arg;
}/*macrestoreout*/

static char *macsetout(char *arg)/*"setout":redirect output*/
{
  flush();
  if((outfile!=NULL)&&(outfile!=stdout))close_file(&outfile);
  letnv(get_string(arg),outputfile_name,MAX_STR_LEN);
  if(!s_scmp(outputfile_name,"null")){
     outfile=NULL;
  }else if(*outputfile_name)
     outfile=open_file(outputfile_name,"w+");
  else
     outfile=stdout;
  *arg='\0';
  return arg;
}/*macsetout*/

static char *macwrite(char *arg)/*"\write(filename,string)":
      appends string to file. On success, return true, otherwise, false.*/
{
FILE *file=NULL;
char str[MAX_STR_LEN];

  get_string(str);
  if (*get_string(arg))
     file=fopen(arg,"a+");
  else
     file=stdout;
  if( file == NULL)
     s_let("false",arg);
  else{
     if (fputs(str,file)<0)/*PPP*//*?is it important?*/
        s_let("false",arg);
     else
        s_let("true",arg);
     close_file(&file);
  }
  return(arg);
}/*macwrite*/

static char *macappendout(char *arg)/*"appendout":appends output to a file.*/
{
  flush();
  if((outfile!=NULL)&&(outfile!=stdout))close_file(&outfile);
  letnv(get_string(arg),outputfile_name,MAX_STR_LEN);
  if(!s_scmp(outputfile_name,"null")){
     outfile=NULL;
  }else if(*outputfile_name)
     outfile=open_file(outputfile_name,"a+");
  else
     outfile=stdout;
  *arg='\0';
  return arg;
}/*macappendout*/

static char *macflush(char *arg)/*"flush". Flushes the internal buffer
                                   and returns the current position at
                                   the output file. In the file iz NULL
                                   or stdout, returns -1.*/
{
  flush();
  if((outfile!=NULL)&&(outfile!=stdout)){
     fflush(outfile);
     long2str(arg,ftell(outfile));
  }else{
     s_let("-1",arg);
  }
  return(arg);
}/*macflush*/

static char *macwritetofile(char *arg)/*"writetofile(fname,pos,srting)".
                                     Writes its argument at a specified
                                     position to a named file. */
{
FILE *file=NULL;
char str[MAX_STR_LEN];
long pos;

  get_string(str);/*get argument to write*/
  pos=get_num("writetofile",arg);/* get the position */

  if (  (file=fopen(get_string(arg),"r+"))==NULL)
    return(s_let("false",arg));
  if(fseek(file,pos,SEEK_SET)){
     close_file(&file);
     return(s_let("false",arg));
  }
  if (fputs(str,file)<0)
     s_let("false",arg);
  else
     s_let("true",arg);
  close_file(&file);
  return(arg);
}/*macmacwritetofile*/

static char *macasksystem(char *arg)/*"\asksystem(cmd,input)". Creates two pipes,
     invokes system shell and executes command, passing "input" as a stdin.
    Returns the stdout of the executed command.*/
{
char str[MAX_STR_LEN],*tmp;
char *argv[4];

FILE *send=NULL,*receive=NULL;
pid_t childpid;
int status;
mysighandler_t oldPIPE;

   get_string(str);/*get input*/
   get_string(arg);/*get cmd*/

   /* Temporary ignore this signal:*/
   /* if compiler fails here, try to change the definition of
      mysighandler_t on the beginning of the file types.h*/
   oldPIPE=signal(SIGPIPE,SIG_IGN);

   /* Prepare arguments for execvp:*/
   argv[0]="sh";
   argv[1]="-c";
   argv[2]=arg;
   argv[3]=NULL;
   /* invoke swallowed shell, last 0 means that we will wait for the swallowed
      process tell us anythig:*/
   if ( (childpid= swallow_cmd(&send,&receive,PATHTOSHELL,argv,0))!=-1 ){
      /*Success. Command is running.*/
      /*Send the input only if it is not empty one:*/
      if( *str!='\0' ){
         fputs(str,send);
         fputs("\n",send);
         fflush(send);
      }
      /* And now read the response:*/
      while(
             (     ( tmp=fgets(arg,MAX_STR_LEN,receive) )==NULL    )&&
             (errno == EINTR)
           );

      if(tmp==NULL)
         *arg='\0';

      /* Drop out trailing eol:*/
      for(tmp=arg; *tmp != '\0' ; tmp++)
         if(*tmp == '\n' )
            *tmp='\0';
      /* So, now that's all, we will close the pipe:*/
      fclose(receive);
      fclose(send);
      /* Wait for the child will finish:*/
      waitpid(childpid, &status, 0);
   }else/*if ( (childpid= swallow_cmd(&send,&receive,"/bin/sh",argv,0))!=-1 )*/
      arg='\0';/*s_let("",arg); Fail...*/
   /* And now, restore old signals:*/
   signal(SIGPIPE,oldPIPE);

   return(arg);
}/*macasksystem*/

/*\pipe(cmd) - starts a command cmd and returns its PID. The command runs in a shell,
stdin/stdout are kept internally. See \frompipe(), \topipe(), \checkpipe(), \closepipe():*/
static char *macpipe(char *arg)
{
int fdsend,fdreceive;
char *argv[4];
mysighandler_t oldPIPE;
pid_t pid;
   /* Prepare arguments for execvp:*/
   argv[0]="sh";
   argv[1]="-c";
   argv[2]=get_string(arg);
   argv[3]=NULL;
   /* Temporary ignore this signal:*/
   /* if compiler fails here, try to change the definition of
      mysighandler_t on the beginning of the file types.h*/
   oldPIPE=signal(SIGPIPE,SIG_IGN);

   pid=run_cmd(
      &fdsend,
      &fdreceive,
      7,/*0 - nothing, &1 - reopen stdin &2 - reopen stdout &4 - demonize*/
      PATHTOSHELL,
      argv
   );
   signal(SIGPIPE,oldPIPE);

   if(pid<1) return s_let("",arg);

   store_pid(pid,fdsend,fdreceive);
   return long2str(arg,pid);

}/*macpipe*/

/*\checkpipe(pipe,signum) - send a signal to a pipe. If pipe <=0, do nothing.
 If signum <1 then it assumed to be 0.*/
static char *maccheckpipe(char *arg)
{
int signum=get_num("checkpipe",arg);
pid_t pid=get_num("checkpipe",arg);
   *arg='\0';
   if(signum<0) signum=0;
   if(pid>0){
        if(
              (  lookup(&pid,g_allpipes)!=NULL  )
           && (!kill(pid,signum))
          )
             s_let("ok",arg);
   }else/*if(pid>0)*/
     s_let("ok",arg);/*Stdin/stdout?*/
   return arg;
}/*maccheckpipe*/

/*\setpipeforwait(pipe): The operator \_waitall() returns the control if there are no more
running jobs, or if the timeout is expired. But, there is third possibility.
The operator will return the control if some information is available on
some definite pipe. The pipe must be set up by means of the operator
\setpipeforwait(pipe), where pipe is the PID returned by the
operator \pipe(), or 0. In the latter case the operator
\_waitall() will return the control if there is something typed on
the keyboard:*/
static char *macsetpipeforwait(char *arg)
{
ALLPIPES *thepipe;
pid_t pid=get_num("setpipeforwait",arg);
   *arg='\0';
   if(pid>0){
        if(
              (  ( thepipe=lookup(&pid,g_allpipes) )!=NULL  )
           && (!kill(pid,0))
          ){
             s_let("ok",arg);
             g_pipeforwait=(thepipe->r);
        }/*if(...)*/
   }else if(pid == 0){
     s_let("ok",arg);/*Stdin?*/
     g_pipeforwait=0;
   }else{
      s_let("ok",arg);
      g_pipeforwait=-1;/*Forget about the descriptor*/
   }
   return arg;
}/*macsetpipeforwait*/

/*\getpipeforwait() returns PID of the pipe which has been set by \setpipeforwait(pipe),
or an empty string, if there is no active pipe for \_waitall()*/
static char *macgetpipeforwait(char *arg)
{
   if(g_pipeforwait>-1)
      long2str(arg,g_pipeforwait);
   else
      *arg='\0';
   *arg='\0';
   return arg;
}/*macgetpipeforwait*/

/*\closepipe(pipe)(here pipe is the PID of the external
program returned by \pipe()) terminates the external program. It
closes the programs' IO channels, sends to the program a KILL
signal*/
static char *macclosepipe(char *arg)
{
mysighandler_t oldCHLD;
pid_t pid=get_num("closepipe",arg);
   *arg='\0';

   /* Temporary set this signal to default:*/
   /* if compiler fails here, try to change the definition of
      mysighandler_t on the beginning of the file types.h*/
   oldCHLD=signal(SIGCHLD,SIG_DFL);
   if(  (pid>0) && ( uninstall(&pid,g_allpipes)==0 )   ){
      /*Removed: the strategy is changed, now I pass pipe to he init:*/
      /*
      int i;
        Must wait() to avoid zombie:
      waitpid(pid, &i, 0);
      */
      s_let("ok",arg);
   }/*if(  (pid>0) && ( uninstall(&pid,g_allpipes)==0 )   )*/
   signal(SIGCHLD,oldCHLD);
   return arg;
}/*macclosepipe*/

/*\linefrompipe(pipe) reads from the pipe and return a whole line:*/
static char *maclinefrompipe(char *arg)
{
pid_t pid=get_num("linefrompipe",arg);
FILE *f;
ALLPIPES *thepipe;
mysighandler_t oldPIPE;

   if(pid<=0)
      f=stdout;
   else{
      if(  (thepipe=lookup(&pid,g_allpipes))==NULL  )
         halt(PIPEXPECTED,arg);
      f=(thepipe->strr);
   }/*if(pid<=0) ... else*/

   /* Temporary ignore this signal:*/
   /* if compiler fails here, try to change the definition of
      mysighandler_t on the beginning of the file types.h*/
   oldPIPE=signal(SIGPIPE,SIG_IGN);

   if(fgets(arg,MAX_STR_LEN,f)==NULL)
      *arg='\0';
   else{
      char *ptr=arg;
      for(;*ptr!='\0';ptr++)if(*ptr=='\n'){
         *ptr='\0';break;
      }/*for(;*ptr!='\0';ptr++)if(*ptr=='\n')*/
   }/*if(fgets(arg,MAX_STR_LEN,strr)==NULL)...else*/

   signal(SIGPIPE,oldPIPE);
   return arg;
}/*maclinefrompipe*/

/*\frompipe(pipe) reads partially ready lines. It stops reading if the line is completed,
at the end-of-file condition, or if there is no more data available in the pipe:*/
static char *macfrompipe(char *arg)
{
pid_t pid=get_num("frompipe",arg);
int r,i,flags;
ALLPIPES *thepipe;
mysighandler_t oldPIPE;
ssize_t res;
char *ptr;
   if(pid<=0)
      r=0;
   else{
      if(  (thepipe=lookup(&pid,g_allpipes))==NULL  )
         halt(PIPEXPECTED,arg);
      r=thepipe->r;
   }/*if(pid<=0) ... else*/

   /* Temporary ignore this signal:*/
   /* if compiler fails here, try to change the definition of
      mysighandler_t on the beginning of the file types.h*/
   oldPIPE=signal(SIGPIPE,SIG_IGN);

   if( (res=read(r,arg,1)) !=1 )/*EOF or read is interrupted by a signal?:*/
       while( (errno == EINTR)&&(res == -1) )
          /*The call was interrupted by  a  signal  before  any data was read, try again:*/
          res=read(r,arg,1);

   /* make the descriptor non-blocking:*/
   flags = fcntl(r, F_GETFL,0);/*First, save the original mode*/
   /*Add O_NONBLOCK:*/
   if(  (res!=1)/* read() fails */
      ||(fcntl(r, F_SETFL, flags | O_NONBLOCK) == -1)/*Fail setting non-blocking*/
     ){
      *arg='\0';goto frompipe_fails;
   }/*if( (res!=1)||(fcntl(r, F_SETFL, flags | O_NONBLOCK) == -1) )*/

   for(ptr=arg+1,i=MAX_STR_LEN-2;i>0;i--)switch(read(r,ptr,1)){
       case -1:
         if(errno == EINTR)/*The call was interrupted by  a  signal*/
            i++;/*Ignore this entry*/
         else
          /*if(errno == EAGAIN) - no data are immediately available for reading, otherwise,
            I/O error. Just give it up:*/
            i=-1;/*Quit the cycle*/
            /*Note, no data were stored in *ptr, need not to increment it.*/
         break;
       case 0 :/*EOF*/
          i=-1;/*Quit the cycle*/
          ptr++;
          break;
       default:/*1*/
          switch(*ptr){
             case '\n':
                *ptr='\0';
                /*No break!*/
             case '\0':
                i=-1;/*Quit the cycle*/
                break;
             default:
                ptr++;
          }/*switch(*ptr)*/
          break;
   }/*for(ptr=arg+1,i=MAX_STR_LEN-2;i>0;i--)switch(read(r,ptr,1))*/
   *ptr='\0';

   /*Now in arg we have a string read from the pipe.*/
   frompipe_fails:
      signal(SIGPIPE,oldPIPE);
      /*Restore blocking mode:*/
      fcntl(r, F_SETFL, flags);
      return arg;
}/*macfrompipe*/

static char *l_do_topipe(char *arg,char *name, int addeol)
{
pid_t pid;
int d,l;
ALLPIPES *thepipe;
mysighandler_t oldPIPE;
   get_string(arg);
   /*!!! On error, the program will be halted, otherwise, get_num  will not touch arg:*/
   pid=get_num(name,arg);

   if(pid<=0)
      d=1;
   else{
      if(  (thepipe=lookup(&pid,g_allpipes))==NULL  )
         halt(PIPEXPECTED,arg);
      d=(thepipe->w);
   }/*if(pid<=0) ... else*/

   /* Temporary ignore this signal:*/
   /* if compiler fails here, try to change the definition of
      mysighandler_t on the beginning of the file types.h*/
   oldPIPE=signal(SIGPIPE,SIG_IGN);

   l=s_len(arg);
   if(addeol)/*Add EOL, if absent*/
      if(     (l==0)/*Empty string, can't subtract the pointer*/
           || (arg[l-1]!='\n')/*EOL is absent, add them*/
        )arg[l++]='\n'; /*Note, we do not add a terminator! This garantees
                          us against overflow, but this is a bit dangerous...*/
   /*ATT! Now if addeol!=0 the arg may be not zero-terminated!*/

   if(writexactly(d,arg,l))
      *arg='\0';
   else
      s_let("ok",arg);

   signal(SIGPIPE,oldPIPE);
   return arg;
}/*l_do_topipe*/

/*\topipe(pipe,text) writes text to the pipe as is:*/
static char *mactopipe(char *arg)
{
   return l_do_topipe(arg,"topipe", 0);
}/*mactopipe*/

/*\linetopipe(pipe,text) appends a new line symbol to the text placed into the pipe:*/
static char *maclinetopipe(char *arg)
{
   return l_do_topipe(arg,"linetopipe", 1);
}/*mactopipe*/

/*\ispipeready(pipe) returns "ok", if \frompipe() will not block, othervise returns "":*/
static char *macispipeready(char *arg)/**/
{
pid_t pid=get_num("ispipeready",arg);
int r,continuepolling=1;
ALLPIPES *thepipe;
mysighandler_t oldPIPE;
fd_set rfds;
struct timeval tv;

   if(pid<=0)
      r=0;
   else{
      if(  (thepipe=lookup(&pid,g_allpipes))==NULL  )
         halt(PIPEXPECTED,arg);
      r=thepipe->r;
   }/*if(pid<=0) ... else*/

   /* Temporary ignore this signal:*/
   /* if compiler fails here, try to change the definition of
      mysighandler_t on the beginning of the file types.h*/
   oldPIPE=signal(SIGPIPE,SIG_IGN);

   FD_ZERO(&rfds);
   FD_SET(r, &rfds);
   tv.tv_sec = 0;
   tv.tv_usec = 0;

   *arg='\0';

   while(continuepolling){
      FD_ZERO(&rfds);
      FD_SET(r,  &rfds);
      tv.tv_sec = 0;
      tv.tv_usec = 0;

      switch( select(r+1, &rfds, NULL, NULL, &tv) ){
         case 1:
             s_let("ok",arg);continuepolling=0;
             break;
         case 0:
             continuepolling=0;
             break;
         default:/*-1*/
           if (errno != EINTR)
              continuepolling=0;
           break;
      }/*switch( select(r+1, &rfds, NULL, NULL, &tv) )*/
   }/*for(;;)*/

   signal(SIGPIPE,oldPIPE);
   return arg;
}/*macispipeready*/

static char *macsystem(char *arg)/*"system". Invokes system shell.*/
{
  flush();
  if((outfile!=NULL)&&(outfile!=stdout))fflush(outfile);
  return(long2str(arg,system(get_string(arg))));
}/*macsystem*/

static char *macread(char *arg)/*"read". Types its argument into standart
                              output, waits for user input and return it.*/
{
  printf(get_string(arg));/*PPP*/
  fgets(arg, MAX_STR_LEN,stdin);/*PPP*/
  arg[s_len(arg)-1]=0;/*Avoid '\n' at the end of line*/
  return(arg);
}/*macread*/

static char *mactraceOn(char *arg)/*\traceOn() start tracing*/
{
#ifndef NO_DEBUGGER
  g_trace_on=g_trace_on_really=1;
#endif
  *arg='\0';
  return arg;
}/*mactraceOn*/

static char *macexit(char *arg)/*"exit". Stops run executing*/
{
   get_string(arg);
   if(sscanf(arg,"%d",&runexitcode)!=1)
      runexitcode=-1;
   isrunexit=1;
   *arg=0;
   return(arg);
}/*macexit*/

static char *macsubst2dir(char *arg)
{
char tmp[MAX_STR_LEN];
   if(str2dir(get_string(arg),tmp,'+','-')==0)
      tmp[1]='\0';/*Can't convert!*/
   return(s_let(tmp+1,arg));
}/*macsubst2dir*/

static char *macrunerror(char *arg)/*"runerror". Generates run time error.*/
{
   halt(get_string(arg),NULL);
   return(arg);
}/*macrunerror*/

static char *mactrueprototype(char *arg)/*"trueprototype". Returns "true",
                             if current prototype agree with pattern from
                             command line."*/
{
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
  if(skip_prototype())
    s_let("false",arg);
  else
    s_let("true",arg);
  return(arg);
}/*mactrueprototype*/

static char *maccoefficient(char *arg)
{
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
  return(letnv(coefficient,arg,NUM_STR_LEN));
}/*maccoefficient*/

static char *macrightspinor(char *arg)/*\rightspinor(#) -  returns index of
  external line (negative!) which is # left  multiplier. If returned value is 0,
  then there is no corresponding fermion external leg:*/
{
register int l;
  if(islast)
        halt(CANNOTCREATEOUTPUT,NULL);
  if(
     ((l=get_num("rightspinor",arg)) > ext_lines)||
     (l<1)
    )return s_let("0",arg);
   return(long2str(arg,g_right_spinors[l]));
}/*macrightspinor*/

static char *macleftspinor(char *arg)/*\leftspinor(#) -  returns index of
  external line (negative!) which is # left  multiplier. If returned value is 0,
  then there is no corresponding fermion external leg:*/
{
register int l;
  if(islast)
        halt(CANNOTCREATEOUTPUT,NULL);
  if(
     ((l=get_num("leftspinor",arg)) > ext_lines)||
     (l<1)
    )return s_let("0",arg);
   return(long2str(arg,g_left_spinors[l]));
}/*macleftspinor*/

static char *maccreateinput(char *arg)/*"createinput". Builds FORM output.
                                          Returns number of created lines*/
{
int i,k,l;
word curId;
char *ptr,ch[2];
set_of_char d;

   set_str2set("-+*,",d);

   if( (islast)||(is_bit_set(&mode,bitBROWS))||(is_bit_set(&mode,bitTERROR)) )
      halt(CANNOTCREATEOUTPUT,NULL);
   if(text_created)
      halt(CANNOTRESETEXISTOUTPUT,NULL);
   text_created=1;
   ch[1]=0;
   define_momenta();

   for(i=1; !(i>int_lines);i++){

      create_line_text(i);

      curId=dtable[0][i]-1;

      if(id[curId].Nfflow!='\0'){
         /*Process fflow:*/
         /*see comment in pilot.c, seek  'MAJORANA fermions'*/
         long2str(arg,output[l_outPos[i]].majorana);
         for(k=id[curId].Nfflow; k>0;k--)
            s_replace(p16,arg,text[ wrktable[0][i]-1 ]);
      }/*if(id[curId].Nfflow!='\0')*/

      /* The following two lines are used for both nums and marks:*/
      k=l_subst[i];
      long2str(arg,k);
      /*Do not touch k in nums!*/

      for(l=id[curId].Nnum;l>0;l--)/*Process num:*/
         s_replace(p11,arg,text[ wrktable[0][i]-1 ]);

      if(id[curId].Nmark!='\0'){
         /*Process marks:*/
         if(linemarks != NULL)
            s_let(linemarks[k],arg);
         /*else -- it is already set by long2str(arg,k); for nums*/

         /*Avoid too ling linemarks:*/
         arg[max_marks_length-1]=0;

         for(l=id[curId].Nmark;l>0;l--)
            s_replace(p15,arg,text[ wrktable[0][i]-1 ]);
      }/*if(id[curId].Nmark)*/

      if(id[curId].Nfromv!='\0'){
         /*Process fromvertex:*/
         if((k=fromvertex(i))==0)
               halt(CURRUPTEDINPUT,input_name);
         k=v_subst[k];
         long2str(arg,k);
         for(l=id[curId].Nfromv;l>0;l--)
            s_replace(p13,arg,text[ wrktable[0][i]-1 ]);
      }/*(id[curId].Nfromv!='\0')*/

      if(id[curId].Ntov!='\0'){
         /*Process tovertex:*/
         if((k=tovertex(i))==0)
            halt(CURRUPTEDINPUT,input_name);
         k=v_subst[k];
         long2str(arg,k);
         for(l=id[curId].Ntov;l>0;l--)
         s_replace(p14,arg,text[ wrktable[0][i]-1 ]);
      }/*if(id[curId].Ntov!='\0')*/

   }/*for(i=1; !(i>int_lines);i++)*/

   for(i=1; !(i>vcount);i++){
      create_vertex_text(i);

      curId=dtable[i][0]-1;

      if(id[curId].Nfflow!='\0'){
         /*Process fflow:*/
         /*see comment in pilot.c, seek  'MAJORANA fermions'*/
         long2str(arg,output[v_outPos[i]].majorana);
         /*while(s_pos(p16,text[ wrktable[i][0]-1 ])!=-1)*/
         for(k=id[curId].Nfflow; k>0;k--)
            s_replace(p16,arg,text[ wrktable[i][0]-1 ]);
      }/*if(id[curId].Nfflow!='\0')*/

      /* The following two lines are used for both nums and marks:*/
      k=v_subst[i];
      long2str(arg,k);
      /*Do not touch k in nums!*/

      /*Process num:*/
      for(l=id[curId].Nnum; l>0;l--)
         s_replace(p11,arg,text[ wrktable[i][0]-1 ]);

      if(id[curId].Nmark!='\0'){
         /*Process marks:*/
         if(vertexmarks != NULL)
            s_let(vertexmarks[k],arg);
         /*else -- it is already set by long2str(arg,k); for nums*/

         /*Avoid too ling lvmarks:*/
         arg[max_marks_length-1]=0;

         /*while(s_pos(p15,text[ wrktable[i][0]-1 ])!=-1)*/
         for(l=id[curId].Nmark;l>0;l--)
            s_replace(p15,arg,text[ wrktable[i][0]-1 ]);
      }/*if(id[curId].Nmark)*/

   }/*for(i=1; !(i>vcount);i++)*/

   if((*coefficient=='1')&&(coefficient[1]==0))
      s_let(coefficient,arg);
   else{
      sprintf(arg,"(%s)",coefficient);
   }
   for(k=0,i=0;i<top_out;i++){
      *ch='*';
      ptr=text[output[i].i];
      do{
         if(s_len(arg)+(k=s_len(ptr))+3>MAX_OUTPUT_LEN){
            if( !( top_formout<max_top_formout  )  ){
               if(
                  (formout=realloc(formout,
                    (max_top_formout+=DELTA_FORMOUT)*sizeof(char*))
                  )==NULL
                 )halt(NOTMEMORY,NULL);
            }
            formout[top_formout++]=new_str(s_cat(arg,arg,ch));
            if(k+3>MAX_OUTPUT_LEN){
                 for(l=MAX_OUTPUT_LEN-3;l>0;l--)
                    if(set_in(ptr[l],d))break;
              if(l<k-3)l++;
              k=l;
              { int i;
                for(i=0;!(i>k);i++)if((arg[i]=ptr[i])==0) break;
              }
              arg[k]='\0';
              if(ptr[k]=='\0'){
                k=0;*ch='*';
              }else{
                *ch='\0';
                ptr+=k;
              }
            }else{
               k=0;
               s_let(ptr,arg);
            }
         }else{
            s_cat(arg,arg,"*");
            s_cat(arg,arg,ptr);
            k=0;
         }
      }while(k);
   }
   if( !( top_formout<max_top_formout  )  ){
      if(
         (formout=realloc(formout,
                    (max_top_formout+=DELTA_FORMOUT)*sizeof(char*))
         )==NULL
      )halt(NOTMEMORY,NULL);
   }
   formout[top_formout++]=new_str(arg);

   return(long2str(arg,top_formout));
}/*maccreateinput*/

static char *macgetformstr(char *arg)/*"getformstr".Returns n FORM line*/
{
 long num;
  if(!text_created)
     halt(OUTPUTEXPRESSIONNOTCREATED,NULL);
  if(((num=abs(get_num("getformstr",arg)))>top_formout)||(!(num>0)))
     *arg=0;
  else
     s_let(formout[num-1],arg);
  return(arg);
}/*macgetformstr*/

static char *macsetmomenta(char *arg)/*"setmomenta". Sets alternative
                                        momenta set. Returns empty line.*/
{
 long num;
  num=abs(get_num("setmomenta",arg));
  if((g_nocurrenttopology)||(is_bit_set(&mode,bitTERROR)))
      halt(CANNOTCREATEOUTPUT,NULL);
  if(text_created)
    halt(CANNOTRESETEXISTOUTPUT,NULL);
  if(num<0)num=1;
  num--;
  if(num>max_momentaset)
     cn_momentaset=max_momentaset;
  else
     cn_momentaset=num;
  *arg=0;
  return(arg);
}/*macsetmomenta*/

static char *macfirstdiagramnumber(char *arg)/*"firstdiagramnumber" --
                        returns number of first diagram set from cmd line*/
{
  sprintf(arg, "%u",start_diagram);
  return(arg);
}/*macfirstdiagramnumber*/

static char *maclastdiagramnumber(char *arg)/*"lastdiagramnumber" --
                        returns number of last diagram set from cmd line*/
{
   sprintf(arg, "%u",finish_diagram);
   return(arg);
}/*maclastdiagramnumber*/

static char *maccurrentdiagramnumber(char *arg)/*"currentdiagramnumber" --
                        returns current number of diagram*/
{
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
     sprintf(arg, "%u",diagram_count);
     return(arg);
}/*maccurrentdiagramnumber*/

static char *macnumberofvertex(char *arg)/*"numberofvertex" -- returns number
                of vertices*/
{
     return(long2str(arg,vcount));
}/*macnumberofvertex*/

static char *macnumberofinternallines(char *arg)/*"numberofinternallines"-
     returns number of internal lines.*/
{
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);

  return(long2str(arg,int_lines));
}/*macnumberofinternallines*/

static char *macnumberofexternallines(char *arg)/*"numberofexternallines"--
   returns number of external lines.*/
{
     return(long2str(arg,ext_lines));
}/*macnumberofexternallines*/

static char *maccmdline(char *arg)/*"cmdline" -- returns n cmd
                                                        line passed to run.*/
{
long num;
   num=get_num("cmdline",arg);
   if((num<1)||(num>run_argc))*arg=0;
   else s_let(run_argv[num-1],arg);
   return(arg);
}/*maccmdline*/

static char *macnumberofid(char *arg)/*"numberofid" -- returns number of
                 identifiers occure in current diagram.*/
{
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
   sprintf(arg, "%u",top_identifiers);
   return(arg);
}/*macnumberofid*/

static char *macgetid(char *arg)/*"getid" -- returns n identifier occure in
   current diagram*/
{
long num;
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
  if(((num=get_num("getid",arg))>top_identifiers)||(!(num>0)))
        halt(INVALIDIDNUMBER,arg);
   return(let2b(text[identifiers[num-1].i],arg));
}/*macgetid*/

static char *macnumberofoutterms(char *arg)/*"numberofoutterms" --
    number of terms in created output*/
{
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
   sprintf(arg, "%u",top_out);
   return(arg);
}/*macnumberofoutterms*/

static char *macoutterm(char *arg)/*"outterm" -- returns begin (to '(') of
                                        n term in output*/
{
long num;
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
  if(((num=get_num("outterm",arg))>top_out)||(!(num>0)))
        halt(INVALIDOUTNUMBER,arg);
   return(let2b(text[output[num-1].i],arg));
}/*macoutterm*/

static char *macfulloutterm(char *arg)/*"fulloutterm" -- returns full
                                        expression of n term in output*/
{
long num;
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
  if(!text_created)
     halt(OUTPUTEXPRESSIONNOTCREATED,NULL);
  if(((num=get_num("fulloutterm",arg))>top_out)||(!(num>0)))
     halt(INVALIDOUTNUMBER,arg);
   return(s_let(text[output[num-1].i],arg));
}/*macfulloutterm*/

static char *macposofoutterm(char *arg)/*"posofoutterm" -- returns number of
                                     line or vertex in of n term in output*/
{
  int i;
  long num;
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
  if(((num=get_num("posofoutterm",arg))>top_out)||(!(num>0)))
     halt(INVALIDOUTNUMBER,arg);
  num--;
   if (output[num].t == 'v')
      i=v_subst[output[num].pos];
   else
      i=l_subst[output[num].pos];
   return(long2str(arg,i));
}/*macposofoutterm*/

static char *macvloutterm(char *arg)/*"vloutterm" -- returns 'v' or 'l' type
                                      of n term in output*/
{
  long num;
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
  if(((num=get_num("vloutterm",arg))>top_out)||(!(num>0)))
     halt(INVALIDOUTNUMBER,arg);
  *arg=output[num-1].t;arg[1]=0;
  return(arg);
}/*macvloutterm*/

static char *macvertexorline(char *arg)/*"vertexorline" -- returns 'v' or 'l'
                  type  of identifier, or 'undefined' if id is undefined.*/
{
int i;
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
  get_string(arg);
  for(i=0;i<top_identifiers;i++)
     if(cmp2b(arg,text[identifiers[i].i])){
        *arg=identifiers[i].lv;arg[1]=0;
        return(arg);
     }
  return(s_let("undefined",arg));
}/*macvertexorline*/

/*"fermcont" -- returns index of fermion line continuing line l from
vertex v. If it is impossible, returns 0:*/
static char *macfermcont(char *arg)
{
 char l,v;
 int i;
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
    v=iv_subst[get_num("fermcont",arg)];

    if((l=get_num("fermcont",arg))>0){
       l=il_subst[l];
    }
    if((l=fcont(l,v))>0)
       i=l_subst[l];
    else
       i=l;
    return(long2str(arg,i));
}/*macfermcont*/

/*returns index of fermion line ingoing from vertex v.
If it is impossible, returns 0:*/
static char *macfermescape(char *arg)
{
 char v;
 int i;
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
    v=iv_subst[get_num("fermescape",arg)];
    if((i=outfline(v))>0)
       i=l_subst[i];
    return(long2str(arg,i));
}/*macfermescape*/

/*"endline" -- returns vertex index of the second end of line l in vertex v:
If it is impossible, returns 0:*/
static char *macendline(char *arg)
{
 char l,v;
 int i;
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
    if  (!((v=get_num("endline",arg))>0))
       return(s_let("0",arg));
    v=iv_subst[v];

    if  (!((l=get_num("endline",arg))>0))
       return(s_let("0",arg));

    l=il_subst[l];
    if((v=endline(l,v))>0)
       i=v_subst[v];
    else
      i=0;
    return(long2str(arg,i));
}/*macendline*/

/*fromvertex -- returns vertex index which is a source of internal line l.
  If it is impossible, returns 0:*/
static char *macfromvertex(char *arg)
{
 char l;
 int i;
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
    if  ((l=get_num("fromvertex",arg))>0){
       l=il_subst[l];
    }
    l=fromvertex(l);
    i=(l>0)?v_subst[l]:l;
    return(long2str(arg,i));
}/*macfromvertex*/

/*tovertex -- returns vertex index which is end of internal line l.
  If it is impossible, returns 0:*/
static char *mactovertex(char *arg)
{
 char l;
 int i;
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
    if  ((l=get_num("tovertex",arg))>0){
       l=il_subst[l];
    }
    l=tovertex(l);
    i=(l>0)?v_subst[l]:l;
    return(long2str(arg,i));
}/*mactovertex*/

static char *readsubststring(char *arg)
{
int i,j;
char *beg,*end,tmp[MAX_STR_LEN],pattern[MAX_ITEM],*p;
  if(g_nocurrenttopology)
      halt(CANNOTCREATEOUTPUT,NULL);
   if(text_created)
      halt(CANNOTRESETEXISTOUTPUT,NULL);
   s_let(beg=end=arg,tmp);
   i=1;*(p=pattern)=0;
   do{
      while(*end!=':'){
         if(*end==0){
            i=0;
            break;
         }else
         if(!set_in(*end,digits))
            halt(INVALIDREORDERING,tmp);
         end++;
      }
      *end=0;
      if(sscanf(beg,"%d",&j)!=1)
         halt(INVALIDREORDERING,tmp);
      *p++=j;
      if(!(p-pattern<MAX_ITEM))
         halt(INVALIDREORDERING,tmp);
      if(i)beg=++end;
   }while(i);
   if(*pattern==0)
      halt(INVALIDREORDERING,tmp);
   *p=0;
   return(s_let(pattern,arg));
}/*readsubststring*/

/*"vsubstitution" -- reorders vertices enumerating. Pattern:
  positions - old, values - new, separated by ':'.
  Returns empty string :*/
static char *macvsubstitution(char *arg)
{
  if(g_nocurrenttopology)
      halt(CANNOTCREATEOUTPUT,NULL);
   if(substitutel(v_subst,readsubststring(get_string(arg))))
       halt(INVALIDREORDERINGSTRING,NULL);
   invertsubstitution(iv_subst,v_subst);
   *arg=0;
   return(arg);
}/*macvsubstitution*/

/*"lsubstitution" -- reorders internal lines enumerating. Pattern:
  positions -- old, values -- new, separated by ':'Prototypes reordered too..
  Returns empty string :*/
static char *maclsubstitution(char *arg)
{
  char tmps[MAX_STR_LEN];
  word tmp;
  if(g_nocurrenttopology)
      halt(CANNOTCREATEOUTPUT,NULL);
  get_string(tmps);
  {/*Block begin*/
     char i=0,k=1,l=0;
     do{
        switch(tmps[i]){
           case  '-':
              l_dir[il_subst[k]]*=-1;
              break;
           case  ':':
              if(++k > int_lines)
                 halt(INVALIDREORDERINGSTRING,NULL);
           default:
              arg[l++]=tmps[i];
        }
     }while(tmps[i++]!='\0');
  }/*Block end*/
  if(substitutel(l_subst,readsubststring(arg)))
       halt(INVALIDREORDERINGSTRING,NULL);
  invertsubstitution(il_subst,l_subst);
  if(  ( !islast) &&
       ((tmp=set_prototype(p_reoder(s_let(prototypes[cn_prototype],tmps),arg)))
            !=cn_prototype)
    ){
        cn_prototype=tmp;
        if(is_bit_set(&mode,bitBROWS))
           diagram[count-1].prototype=cn_prototype;
   }
  /*s_let(invertsubstitution(arg, l_subst),lt_subst);This is the BUG!!!*/
  /*See "The substituions map" in "variables.h"*/
  *arg=0;
   return(arg);
}/*maclsubstitution*/

static char *prcheck(char *prototype, char *pattern)
{
 char buf[MAX_ITEM];
 int i,l,k;
    i=l=s_len(prototype)-1;
    s_let(prototype,buf);
    for(;!(i<0);i--){
       if( (!(pattern[i]>'\0')) || ((k=pattern[i]-1)>l) )
          halt(INVALIDREORDERINGSTRING,NULL);
       prototype[k]=buf[i];
    }
    return(prototype);
}/*prcheck*/

/*"reorderedprototype" -- returns a prototype obtained from the current
   one by line substitution. Pattern:
  positions -- old, values -- new, separated by ':'
  */
static char *macreorderedprototype(char *arg)
{
  char tmps[MAX_I_LINE];

  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
   return(s_let(prcheck(s_let(prototypes[cn_prototype],tmps)
                  ,readsubststring(get_string(arg))
               ),arg));
}/*macreorderedprototype*/

static char *macinvertline(char *arg)
{
 long num;

  num=abs(get_num("invertline",arg));
  if(g_nocurrenttopology)
      halt(CANNOTCREATEOUTPUT,NULL);
  if(text_created)
    halt(CANNOTRESETEXISTOUTPUT,NULL);
  if((num<0)||(num>int_lines))
      halt(INVALIDNUMBER,arg);
  num=il_subst[num];
  l_dir[num]=l_dir[num]*(-1);
  *arg=0;
  return(arg);
}/*macinvertline*/

static char *macisbrowser(char *arg)/*"isbrowser" -- returns "true" if
                                       bitBROWS is set.*/
{
  if(is_bit_set(&mode,bitBROWS))
    s_let("true",arg);
  else
    s_let("false",arg);
  return(arg);
}/*macisbrowser*/

static char *macislast(char *arg)/*"islast" -- returns "true" if
                                       it is extra call.*/
{
  if(islast)
    s_let("true",arg);
  else
    s_let("false",arg);
  return(arg);
}/*macislast*/

static char *macprototypereset(char *arg)/*"prototypereset" -- resets
    current prototype, returns empty string.*/
{
  word tmp;
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
   if(text_created)
      halt(CANNOTRESETEXISTOUTPUT,NULL);
      get_string(arg);
     if(total_length>MAX_I_LINE)
        halt(TOOLONGNEWPROTOTYPE,arg);
     if((tmp=set_prototype(arg))!=cn_prototype){
        cn_prototype=tmp;
        if(is_bit_set(&mode,bitBROWS))
           diagram[count-1].prototype=cn_prototype;
     }
     return(arg);
}/*macprototypereset*/

/*Returns empty string:*/
static char *macsetcurrenttopology(char *arg)
{
 long num;
 int j;
 aTOPOL *theTopology=NULL;
  if(!islast)/*Changing current topology during evaluation?*/
      halt(CANNOTCREATEOUTPUT,NULL);

  num=abs(get_num("setcurrenttopology",arg));
  if(num<1)num=1;
  num--;
  if(!(num<top_topol))
     cn_topol=top_topol-1;
  else
     cn_topol=num;

  theTopology=topologies+cn_topol;
  max_momentaset=theTopology->n_momenta-1;
  max_topologyid=theTopology->n_id-1;
  int_lines=theTopology->topology->i_n;
  ext_lines=theTopology->topology->e_n;

  cn_momentaset=0;
  cn_topologyid=0;
  canonical_topology=NULL;

  /*!!!Look these up:*/
  free_mem(&l_subst);
  free_mem(&il_subst);
  free_mem(&lt_subst);
  free_mem(&vt_subst);
  free_mem(&v_subst);
  free_mem(&iv_subst);
  free_mem(&l_dir);

  *(l_subst=get_mem(int_lines+2,sizeof(char)))=-1;
  *(il_subst=get_mem(int_lines+2,sizeof(char)))=-1;
  *(lt_subst=get_mem(int_lines+2,sizeof(char)))=-1;

  *(vt_subst=get_mem(vcount+2,sizeof(char)))=-1;
  *(v_subst=get_mem(vcount+2,sizeof(char)))=-1;
  *(iv_subst=get_mem(vcount+2,sizeof(char)))=-1;
  *(l_dir=get_mem(int_lines+2,sizeof(char)))=-1;

  /*Do not forget reset l_subst and l_dir: */
  for(j=1;!(j>int_lines);j++){
     l_subst[topologies[cn_topol].l_subst[j]]=
     lt_subst[topologies[cn_topol].l_subst[j]]=j;
     l_dir[j]=1;
  }/*for(j=1;!(j>int_lines);j++)*/
  invertsubstitution(il_subst,l_subst);
  /*Do not forget reset v_subst:*/
  for(j=1;!(j>vcount);j++)
     v_subst[topologies[cn_topol].v_subst[j]]=
     vt_subst[topologies[cn_topol].v_subst[j]]=j;
  invertsubstitution(iv_subst,v_subst);

  clear_lvmarks();
  /* Set new linemarks, if present:*/
  if(topologies[cn_topol].linemarks != NULL){
        j=*(topologies[cn_topol].linemarks[0]);
        linemarks=get_mem(j+1,sizeof(char*));
        for(j=0;!(j>int_lines);j++)
           linemarks[j]=new_str(topologies[cn_topol].linemarks[j]);
  }/*if(topologies[cn_topol].linemarks != NULL)*/
  /* Set new vertexmarks, if present:*/
  if(topologies[cn_topol].vertexmarks != NULL){
        j=*(topologies[cn_topol].vertexmarks[0]);
        vertexmarks=get_mem(j+1,sizeof(char*));
        for(j=0;!(j>vcount);j++)
           vertexmarks[j]=new_str(topologies[cn_topol].vertexmarks[j]);
  }/*if(topologies[cn_topol].vertexmarks != NULL)*/

  /*Canonical topology is not set, mark it properly:*/
  *lsubst=*vsubst=*ldir=*ltsubst=*vtsubst='\0';

  /*Similar to islast, but specially for topologies:*/
  g_nocurrenttopology=0;/*Allow topology related operators*/
  *arg='\0';
  return(arg);
}/*macsetcurrenttopology*/

/*istopologyoccur() -- "yes" if this topology occurs in the diagram set, or "no":*/
static char *macistopologyoccur(char *arg)
{

  if(g_nocurrenttopology)
      halt(CANNOTCREATEOUTPUT,NULL);

  if(is_bit_set(&(topologies[cn_topol].label),0))
     s_let("yes",arg);
  else
     s_let("no",arg);
  return(arg);
}/*macistopologyoccur*/

/*Each of topology may be supplied bya set of "remarks".
  Reamarks are named strings.*/

/*setTopolRem(name, text) -- sets value "text" to the remark of name "name" in current
  topology. Returns previous value of a remark, or "":*/
static char *macsetTopolRem(char *arg)
{
char *newtext;

  if(g_nocurrenttopology)
      halt(CANNOTCREATEOUTPUT,NULL);
  newtext=new_str(get_string(arg));/*This must be allocated in any cases.*/
/*char *setTopolRem(char *name, char *newtext, char *arg,
                                REMARK **remarks,
                                unsigned int *top_remarks, int *ind):
 Sets remark "name " to "newtext". Previous value of "name" is returned in the buffer
 "arg". If arg == NULL, returns NULL.
 If ind!=NULL, then it is an array of length ind[0]. This function will add found
 index to the end of ind and increments ind[0].
 And, see the following remark:
 ATTENTION! "newtext" is ALLOCATED and must be stored as is.
  If ind==NULL then "name" will be allocated by setTopolRem. Otherwise (if ind!=NULL)
  "name" is assumed as already allocated. */
 return setTopolRem(get_string(arg), newtext,arg,
                                &(topologies[cn_topol].top_remarks),
                                &(topologies[cn_topol].remarks),
                                NULL);
                              /*ind==NULL*/
 /*Note, setTopolRem can't fails non-fatally!*/
}/*macsetTopolRem*/

/*\getTopolRem(name) -- Returns the value of the remark of name "name" in current
  topology, or "", if there is no such a remark:*/
static char *macgetTopolRem(char *arg)
{
char *tmp;
  if(g_nocurrenttopology)
      halt(CANNOTCREATEOUTPUT,NULL);

  /*getTopolRem  see remarks.c:*/
  if(
     (
       tmp=getTopolRem(get_string(arg),
                       topologies[cn_topol].top_remarks,
                        topologies[cn_topol].remarks
                      )
     )==NULL
  )return s_let("",arg);
  return s_let(tmp,arg);
}/*macgetTopolRem*/

/*\killTopolRem(name) clears a named remark in current topology. Returns used
value of a remark, or "" if there is no such a remark:*/
static char *mackillTopolRem(char *arg)
{

  if(g_nocurrenttopology)
      halt(CANNOTCREATEOUTPUT,NULL);

  if( deleteTopolRem(&arg,
                     topologies[cn_topol].top_remarks,
                     topologies[cn_topol].remarks,
                     get_string(arg)) <0 ) *arg='\0';/*not found*/
  return(arg);
}/*mackillTopolRem*/
/*valueOfTopolRem(num) -- returns the value of the remark number "num" of current topology,
 or empty line. "num" is counted from 1:*/
static char *macvalueOfTopolRem(char *arg)
{
 int i=indexRemarks(
    topologies[cn_topol].top_remarks,
    topologies[cn_topol].remarks,
    get_num("valueOfTopolRem",arg)-1   );

    if(i<0)*arg='\0';
    else
       s_let( (topologies[cn_topol].remarks)[i].text, arg);
    return arg;
}/*macvalueOfTopolRem*/

/*nameOfTopolRem(num) -- returns the name of the remark number "num" of current topology,
 or empty line. "num" is counted from 1:*/
static char *macnameOfTopolRem(char *arg)
{
 int i=indexRemarks(
    topologies[cn_topol].top_remarks,
    topologies[cn_topol].remarks,
    get_num("nameOfTopolRem",arg)-1   );

    if(i<0)*arg='\0';
    else
       s_let( (topologies[cn_topol].remarks)[i].name, arg);
    return arg;
}/*macnameOfTopolRem*/

/* \howmanyTopolRem() -- returns number of remarks for current topology:*/
static char *machowmanyTopolRem(char *arg)
{
   return(
      long2str(arg,
         howmanyRemarks(topologies[cn_topol].top_remarks,topologies[cn_topol].remarks)
      )
   );
}/*machowmanyTopolRem*/

static char *maccurrenttopologynumber(char *arg)/*"currenttopologynumber" --
                        returns number of topology of current diagram*/
{
  if(g_nocurrenttopology)
      halt(CANNOTCREATEOUTPUT,NULL);
     return(long2str(arg,cn_topol+1));
}/*maccurrenttopologynumber*/

static char *macnumberoftopologies(char *arg)
{
   return long2str(arg,top_topol);
}/*macnumberoftopologies*/

static char *mactopologyidreset(char *arg)/*"topologyidreset" -- sets
    new (alternativ) topology identifier , returns it.*/
{
 long num;
  if((g_nocurrenttopology)||(is_bit_set(&mode,bitTERROR)))
      halt(CANNOTCREATEOUTPUT,NULL);
   if(text_created)
      halt(CANNOTRESETEXISTOUTPUT,NULL);
  num=abs(get_num("toplogyidreset",arg));
  if(num<1)num=1;
  num--;
  if(num>max_topologyid)
     cn_topologyid=max_topologyid;
  else
     cn_topologyid=num;
  if(  is_bit_set(&mode,bitBROWS)&& (!islast) )
     diagram[count-1].n_id=cn_topologyid;
  return(s_let(topologies[cn_topol].id[cn_topologyid],arg));
}/*mactopologyidreset*/

static char *macnoncurrenttopologyid(char *arg)/*"noncurrenttopologyid"
       -- returns topology identifier of number of its argument.*/
{
 long num;
  if((g_nocurrenttopology)||(is_bit_set(&mode,bitTERROR)))
      halt(CANNOTCREATEOUTPUT,NULL);
  num=abs(get_num("noncurrenttopologyid",arg));
  if(num<1)num=1;
  num--;
  if(num>max_topologyid)
     num=max_topologyid;
  return(s_let(topologies[cn_topol].id[num],arg));
}/*macnoncurrenttopologyid*/

static char *macnumberofmomentaset(char *arg)/*"numberofmomentaset" -- returns
             maximal number of alternative momenta set.*/
{
  if(g_nocurrenttopology)
      halt(CANNOTCREATEOUTPUT,NULL);
  if(is_bit_set(&mode,bitTERROR))
     s_let("0",arg);
  else
    long2str(arg,max_momentaset+1);
  return(arg);
}/*macnumberofmomentaset*/

static char *mactopologyisgotfrom(char *arg)/**/
{

  if(g_nocurrenttopology)
      halt(CANNOTCREATEOUTPUT,NULL);

  if(is_bit_set(&(topologies[cn_topol].label),2))
      s_let("automatic",arg);
  if(   is_bit_set(&mode,bitTERROR)||(topologies[cn_topol].orig==NULL)  )
     s_let("unspecified",arg);
  else{
    if(is_bit_set(&(topologies[cn_topol].label),1))
      s_let("generic",arg);
    else
      s_let("user",arg);
  }
  return(arg);
}/*mactopologyisgotfrom*/

static char *maccurrentmomentaset(char *arg)/*"currentmomentaset" -- returns
             current number of alternative momenta set.*/
{
  if(g_nocurrenttopology)
      halt(CANNOTCREATEOUTPUT,NULL);
  if(is_bit_set(&mode,bitTERROR))
     s_let("0",arg);
  else
    long2str(arg, cn_momentaset+1);
  return(arg);
}/*maccurrentmomentaset*/

static char *macnumberoftopologyid(char *arg)/*"numberoftopologyid" -- returns
             maximal number of alternative topology id.*/
{
  if(g_nocurrenttopology)
      halt(CANNOTCREATEOUTPUT,NULL);
  return(long2str(arg,max_topologyid+1));
}/*macnumberoftopologyid*/

static char *maccurrenttopologyid(char *arg)/*"currenttopologyid" -- returns
             current number of alternative topology id.*/
{
  if(g_nocurrenttopology)
      halt(CANNOTCREATEOUTPUT,NULL);
  return(long2str(arg,cn_topologyid+1));
}/*maccurrenttopologyid*/

static char *mactopologyid(char *arg)/*"topologyid" -- returns current
  id of current topology*/
{
  if(g_nocurrenttopology)
      halt(CANNOTCREATEOUTPUT,NULL);
  return(s_let(topologies[cn_topol].id[cn_topologyid],arg));
}/*mactopologyid*/

static char *mactopology(char *arg)/*"topology" -- returns
   string representation of current topology*/
{
  if(g_nocurrenttopology)
      halt(CANNOTCREATEOUTPUT,NULL);
  if(topologies[cn_topol].orig!=NULL)
     top2str(topologies[cn_topol].orig, arg);
  else
     top2str(topologies[cn_topol].topology, arg);
  return(arg);
}/*mactopology*/

static int l_eliminatedLegs=0;

/*\rmExtLeg(n) mark the external leg number n to be removed */
static char *macrmExtLeg(char *arg)
{
int num;
  num=get_num("rmExtLeg",arg);
  if(num<0)num=-num;
  if( (num>ext_lines)||(num==0) )
     halt(WRONGEXTERNALLINE,arg);
  set_bit(&l_eliminatedLegs,num);
  (*arg)='\0';
  return arg;
}/*macrmExtLeg*/

static int l_sOp(char **tbl, int v, int l, int vcount)
{
   int i;
   for(i=1; !(i>vcount); i++)if( (i!=v)&&(tbl[i][l]!='\0') ) return i;
   return v;/*Tadpole*/
}/*l_sOp*/

static void l_proc_momentarefs_list(char *thelist,int n, char **lists, char **thematrix)
{
int i;
char *tmp;
   if(thelist!=NULL){
      for(i=1;!(i>n);i++)if(lists[i]!=NULL){
         for(tmp=lists[i];(*tmp)!='\0';tmp++)
            if(*tmp < 0)
               thelist[-*tmp]=-thematrix[0][i];
            else
               thelist[*tmp]=thematrix[0][i];
         free(lists[i]);
      }/*for(i=1;!(i>n);i++)if(lists[i]!=NULL)*/
      free(lists);
   }/*if(thelist!=NULL)*/
}/*l_proc_momentarefs_list*/

/*Auxiliary routine, amputates marked external lines (they are marked by macrmExtLeg,
see above) from current topology, the result is stored in ampTopol.
The following arrays (returned values!)amputated[original] are indices of line 
of amputated topology which corresponds to original. Absolute value is an index, 
sign corresponds to direction:
elist - external line of original <-> exetrnal line of amputated
ielist   internal line of original <-> internal line of amputated
ilist internal line of original <-> exetrnal line of amputated:
*/
int l_createAmputatedTopology(
                                         pTOPOL ampTopol,
                                         char *elist,
                                         char *ilist,
                                         char *ielist
                                            )
{
pTOPOL t=topologies[cn_topol].orig;
int i,j,k,l,nV,e_n,i_n;
char **inT, /*Incident matrix for internal lines. inT[vertex][line]='\0', or 'io, or 'o',
      or 't'. Here 't' stays for "tadpole", 'i' for "incoming", 'o' - for "out
      going", so the line is directed from 'o' to 'i'. inT[0][l]= '\0' for eliminated 
      lines, or '\1' (at the last step '\1' becomes an order number)*/
     **extT, /*Incident matrix for external lines. extT[vertex][line]='\0' or '\1' 
      if -line is connected with vertex. extT[0][l]= '\0' for eliminated 
      lines, or order number for actual lines.*/     
     *vrtx, /*'x' for eliminated vertices, '\0' for others*/
     *lns, /*Auxiliary array - for each vertex it specializes indices of coincident lines*/
     *tmp,
     **iel=NULL,
     **il=NULL;

   if(t==NULL)/*Automatic?*/
     t=topologies[cn_topol].topology; 
   e_n=t->e_n; i_n=t->i_n;

   if(i_n==0)/*No internal lines!*/
       return 0;

   if(e_n==0)/*No external lines!*/
       return 0;

   vrtx=get_mem(vcount+1,sizeof(char));
   lns= get_mem( i_n*2 + e_n + 1,sizeof(char));

   inT=get_mem(vcount+1,sizeof(char*));
   extT=get_mem(vcount+1,sizeof(char*));
   
   if(elist!=NULL){
      for(i=1;!(i>e_n);i++)
         elist[i]='\0';
   }
   if(ielist!=NULL){
      iel= get_mem( e_n+1,sizeof(char*));
      for(i=1;!(i>e_n);i++)/*Note, here i_n since it concerns INTERNAL lines:*/
         iel[i]=get_mem( i_n+1,sizeof(char));
      for(i=1;!(i>i_n);i++)
         ielist[i]='\0';
   }

   if(ilist!=NULL){
      il= get_mem( i_n+1,sizeof(char*));
      for(i=1;!(i>i_n);i++){
         *(il[i]=get_mem( i_n+1,sizeof(char)))=i;
         ilist[i]='\0';
      }
   }

   /* Allocation of incident matrices:*/
   for(i=0;!(i>vcount); i++){
      inT[i]= get_mem(i_n+1,sizeof(char));
      extT[i]=get_mem(e_n+1,sizeof(char));
   }          

   /* Creation of incident matrices:*/
   /*internal:*/
   for(i=1;!(i>t->i_n);i++){
     if( (t->i_line)[i].to==(t->i_line)[i].from )
        inT[(t->i_line)[i].to][i]='t';
     else{ 
        inT[(t->i_line)[i].to][i]='i'; 
        inT[(t->i_line)[i].from][i]='o'; 
     }
     inT[0][i]=1;
   }
   /*external. Note, we ignore eliminated lines!:*/
   for(l=i=1;!(i>e_n);i++)if(!is_bit_set(&l_eliminatedLegs,i)){
      extT[(t->e_line)[i].to][i]=1;
      if(elist!=NULL)elist[i]=l;
      extT[0][i]=l++;
   }

   ampTopol->e_n=l-1;/*forever*/
   ampTopol->i_n=i_n;/*temporarily*/
   
   (ampTopol->i_line)[0].to=(t->i_line)[0].to;
   (ampTopol->i_line)[0].from=(t->i_line)[0].from;

   do{/*Removing isolated vertex may lead to decreasing degree of vertex*/
   nV=0;/*Fill up lns (see above):*/
   for(i=1;!(i>vcount); i++){
      l=0;
      /*Note, here the vertex can't be deleted!*/
      for(j=1;!(j>e_n);j++)
         if(extT[i][j]!='\0')lns[l++]=-j;
      for(j=1;!(j>i_n);j++)switch(inT[i][j]){
         case 'i':/*ingoing, 'to'*/
           lns[l++]=j;
           break;
        case 't':/*tadpole:*/
           lns[l++]=j;
           /*no break*/
         case 'o':/*outgoing, 'from'*/
           lns[l++]=j;
           break;
      }
      /*Now l is the degree of interaction in the vertex 'i'*/
      switch(l){
         case 2:/*two lines vertex */
            if(lns[0]>0){/*between two internal lines*/
              int lno=l_sOp(inT,i,lns[1],vcount);
              if(inT[lno][lns[0]]!='\0')/*Doublle line -> tadplole*/
                 inT[lno][lns[0]]='t';
              else
                inT[lno][lns[0]]=inT[i][lns[0]];

              if(ilist!=NULL){
                 if( inT[i][lns[0]] == inT[i][lns[1]]) /*opposite*/
                   UPSET_CH(il[lns[1]]);
                 /*il[lns[0]]+=il[lns[1]]:*/
                 s_cat(il[lns[0]],il[lns[0]],il[lns[1]]);
                 free(il[lns[1]]);il[lns[1]]=NULL;
              }/*if(ilist!=NULL)*/

              inT[i][lns[0]]=inT[i][lns[1]]='\0';
              
            }else{/*between external and internal lines*/
              extT[i][-lns[0]]='\0';
              if(ielist!=NULL){
                 char c=(ext_particles[-lns[0]-1].is_ingoing)?'i':'o';
                 if( inT[i][lns[1]] == c )/*opposite*/
                    UPSET_CH(il[lns[1]]);
                 /*iel[-lns[0]]+=il[lns[1]]:*/
                 s_cat(iel[-lns[0]],iel[-lns[0]],il[lns[1]]);
                 free(il[lns[1]]);il[lns[1]]=NULL;
              }/*if(ielist!=NULL)*/
              extT[l_sOp(inT,i,lns[1],vcount)][-lns[0]]=1;
            }
            /*no break*/
         case 1:/*isolated vertex - remove it together with line*/
            /*kill last line:*/
            inT[0][lns[l-1]]='\0';
            inT[l_sOp(inT,i,lns[l-1],vcount)][lns[l-1]]='\0';
            inT[i][lns[l-1]]='\0';
            if(l==1)nV=1;
            /*no break*/
         case 0:/*Not a vertex at all!*/
            vrtx[i]='x';/*remove the vertex*/
            break;
      }/*switch*/
   }/*for(i=1;!(i>vcount); i++)*/
   }while(nV==1);/*Removing isolated vertex may lead to decreasing degree of vertex*/

   /*Remove killed vertices:*/
   for(nV=i=1; !(i>vcount); i++){ 
      if(vrtx[i]=='x')
         free(extT[i]);
      else{ 
         if(i!=nV){
            extT[nV]=extT[i];
            inT[nV]=inT[i];
         }
         nV++;
      }
   }
   nV--;

   /*Re-use lns:*/
   free(lns);
   lns=get_mem(e_n+1,sizeof(char));

   /*Sort vertices to fit to external lines:*/
   k=1;
   for(i=1;!(i>e_n);i++)if(  (extT[0][i]!='\0')&&(lns[i]=='\0')  ){
      for(j=k; !(j>vcount); j++)
         if(extT[j][i]!='\0')break;
      /*Now j corresponds to the vertex 'to'*/
      lns[i]=k;
      /*Look for the next possible ext. lines attached to the same vertex:*/
      for(l=i+1;!(l>e_n);l++)if(extT[j][l]!='\0')lns[l]=k;
      if(j!=k){/*Exchange vertices j and k (note, j>=k by construction!)*/
         tmp=extT[k];extT[k]=extT[j];extT[j]=tmp;
         tmp=inT[k];inT[k]=inT[j];inT[j]=tmp;
      }/*if(j!=k)*/
      k++;
   }/*for(i=1;!(i>e_n);i++)if(  (extT[0][i]!='\0')&&(lns[i]=='\0')  )*/

   /*Now read a matrix:*/

   /*For external lines can be used lns:*/
   j=0;
   for(i=1;!(i>e_n);i++)if(extT[0][i]!='\0'){
     j++;
     (ampTopol->e_line)[j].to=lns[i];/*Here 'i' is correct!*/
     (ampTopol->e_line)[j].from=-j;
   }/*for(i=1;!(i>ampTopol->e_n);i++)if(extT[0][i]!='\0')*/

   /*For shortness:*/
   t=ampTopol;

   /*Internal lines:*/
   j=0;
   for(i=1;!(i>i_n);i++)if(inT[0][i]!='\0'){
      j++;
      (t->i_line)[j].from=(t->i_line)[j].to=0;
      for(k=1; !(k>nV);k++)switch(inT[k][i]){
         case 't':
            (t->i_line)[j].from=(t->i_line)[j].to=k;k=nV+1;break;
         case 'i':
            (t->i_line)[j].to=k;
            if(  (t->i_line)[j].from !=0  )k=nV+1;break;
         case 'o':
            (t->i_line)[j].from=k;
            if(  (t->i_line)[j].to !=0  )k=nV+1;break;
      }/*for(k=1; !(k>vcount);k++)switch(inT[k][i])*/
      inT[0][i]=j;
   }/*for(i=1;!(i>i_n);i++)if(inT[0][i]!='\0')*/
   t->i_n=j;

   /*ampTopol is ready! Possibly, lists :*/

   l_proc_momentarefs_list(ielist,e_n,iel,extT);
   l_proc_momentarefs_list(ilist,i_n,il,inT);

   /*That's all, clear everything:*/

   /*Note, el and il, if allocated, are freed by l_proc_momentarefs_list*/

   for(i=0;!(i>nV); i++){
      free(inT[i]);
      free(extT[i]);
   }

   free(extT);
   free(inT);

   free(vrtx);
   free(lns);

   return nV;

}/*l_createAmputatedTopology*/

/*Auxiliary routine, used to creale arrays and store them in local variables in
  macamputateTopology and macamputatedTopology*/
static void l_setList(int n, char *arg, char *buf,char *thelist)
{
   register int i,j;
   for(j=0,i=1; i<=n; i++){
      sprintf(arg+j,"%d,",thelist[i]);
      while(arg[j]!='\0')j++;
   }
   if(j>0)arg[j-1]='\0';/*Remove trailing ','*/
   set_run_var(buf,arg);
}/*l_setList*/

/*
\amputateTopology(varname,tablename) tries to find an amputated current 
topology in the table "tablename" and returns it. It adds all momenta 
from the table topolgy to momenta of current topology and save the
result to the local variable <varname>_M. On failure returns an 
empty sring. If it can't load the table it halts a program. Creates local 
variables:
<varname>_IN - number of internal lines in the amputated topology.
<varname>_N - name of the found topology
<varname>_M sum of momenta from the current topology and from the table 
topology.

The following arrays amputated[original] are indices of line of amputated
topology which corresponds to original. Absolute value is an index, sign 
corresponds to direction:
<varname>_E - external line of original <-> exetrnal line of amputated 
<varname>_IE   internal line of original <-> internal line of amputated
<varname>_I internal line of original <-> exetrnal line of amputated
*/
static char *macamputateTopology(char *arg)
{
static int idScratched=0;

tTOPOL ampTopol;
int nv=0,basevarlen;
tt_singletopol_type *t=NULL;
char *err=NULL;
char *varname;
char *elist, *ilist, *ielist;

   get_string(arg);

   if(idScratched == 0)
      if( (idScratched=tt_loadTable(arg))<0 ){
         char buf[MAX_IMG_LEN];
         get_string(arg);/*Remove the first argument from the stack befor halt*/
         halt(FAIL_LOADING_TABLE,arg,tt_error_txt(buf,idScratched));
      }


   {/*Block*/
      pTOPOL t=topologies[cn_topol].topology;
      elist=get_mem(t->e_n+2,sizeof(char));
      ilist=get_mem(t->i_n+2,sizeof(char));
      ielist=get_mem(t->e_n+2,sizeof(char));
      /*Get the base name for local variables:*/
      if(   ( basevarlen=s_len(get_string(arg)) ) >MAX_STR_LEN/2  )
         halt(TOOLONGSTRING,NULL);
      /*Allocate varname to fit the base name plus several positions for qualifyers:*/
      varname=get_mem(basevarlen+5,sizeof(char));
      /*Copy the base name to varname:*/
      s_let(arg,varname);
   }/*Block*/
   {/*Block*/
      char ls[MAX_I_LINE],vs[MAX_I_LINE],ld[MAX_I_LINE];
      char group[5];
      int *momentum;
      tTOPOL redAmpTopol;
      long ind;
      int i,l,dir,iscommon,tlength;
      char *savedZeroVec=g_zeroVec;
      char *tmp, *momentumBegin;
        if( (nv=l_createAmputatedTopology(&ampTopol,elist,ilist,ielist))==0 ){
           *arg='\0';goto theend;
        }/*if( (nv=l_createAmputatedTopology(&ampTopol,elist,ilist,ielist))==0 )*/

        {/*Block*/
           pTOPOL t=topologies[cn_topol].topology;
           s_let("_IN",varname+basevarlen);
           set_run_var(varname,long2str(arg,ampTopol.i_n));

           s_let("_E",varname+basevarlen);
           l_setList(t->e_n,arg,varname,elist);

           s_let("_IE",varname+basevarlen);
           l_setList(t->i_n,arg,varname,ielist);

           s_let("_I",varname+basevarlen);
           l_setList(t->i_n,arg,varname,ilist);
        }/*Block*/

        *vs=*ls=*ld=-1;
     
        for(i=1;!(i>nv);i++)
          vs[i]=i;
        vs[i]='\0';
        for(i=1;!(i>ampTopol.i_n);i++){ls[i]=i;ld[i]='\1';}
        ls[i]=ld[i]='\0';

        reduce_topology_full(&ampTopol,&redAmpTopol,ls,vs,ld);
        /*now ls[orig]=red, vs[orig]=red, ld[orig] <->red */
        if(  (t=tt_initSingleTopology())==NULL   ){
           err=NOTMEMORY; goto theend;
        }
        ind=tt_lookupFromTop(t,
                             idScratched,
                             &redAmpTopol,
                             nv);
        tt_clearSingleTopology(t);

        if(ind<0){
           *arg='\0';goto theend;
        }
        t=( (tt_table[idScratched-1])->topols )[ind-1];
        /*Now t points to the topology found in the table*/

        /*output the table topology name into the local variable:*/
        s_let("_N",varname+basevarlen);
        set_run_var(varname,t->name);

        group[0]=3;group[1]=group[2]=1;group[3]='\n';group[4]=0;
        g_zeroVec="";tlength=1; *arg='[',tmp=arg+1;

        iscommon=!(
                   (topologies[cn_topol].ext_momenta!=NULL) &&
                   (topologies[cn_topol].ext_momenta[0]!=NULL)
                 );

        /*Build momenta.*/
        /*Build external momenta:*/
        for(i=ext_lines; i>0;i--){
           /*From topology:*/
           if(iscommon)/* Common external momenta*/
              momentum=ext_particles[i-1].momentum;
           else
              momentum=topologies[cn_topol].ext_momenta[0][i];
           /*dir=(ext_particles[i-1].is_ingoing)?1:-1;*/
           dir=1;
           momentumBegin=tmp;
           build_momentum(group,momentum,tmp,dir);
           for(;(*tmp)!='\0';tmp++)
              if(++tlength +1 >=MAX_STR_LEN)
                 halt(TOOLONGSTRING,NULL);

           if(elist[i]!='\0'){/*Add momenta from the table:*/
               char *ptr;
               if(   
                   (t->extMomenta != NULL)&&
                   ( (t->extMomenta)[0]!= NULL)&&
                   (   ( ptr=(t->extMomenta)[0][elist[i]-1] )!=NULL   )
                 ){
                  switch(*ptr){
                     case '-':case'+':
                        s_letn(ptr,tmp,MAX_STR_LEN - ++tlength);
                        break;
                     default:
                        (*tmp)='+';
                        s_letn(ptr,tmp+1,MAX_STR_LEN - ++tlength);
                        break;
                  }/*switch(*ptr)*/
                  if(dir < 0 )
                     swapCharSng(tmp);/* must return 0*/
                 for(;(*tmp)!='\0';tmp++)
                    if(++tlength +1 >=MAX_STR_LEN)
                       halt(TOOLONGSTRING,NULL);
               }else{/*if(...)*/
                  *arg='\0';
                  goto theend;
               }
           }/*if(elist[i]!='\0')*/
           if(*momentumBegin == '\0'){/*empty*/
             s_letn(savedZeroVec,momentumBegin,MAX_STR_LEN - tlength);
             for(;(*tmp)!='\0';tmp++)
                if(++tlength +1 >=MAX_STR_LEN)
                   halt(TOOLONGSTRING,NULL);
           }/*if(*momentumBegin == '\0')*/
           if(i == 1)
              *(tmp++)=']';
           else
              *(tmp++)=',';
           if(++tlength +1 >=MAX_STR_LEN)
                 halt(TOOLONGSTRING,NULL);
           *tmp='\0';
        }/*for(i=ext_lines; i>0;i--)*/
        /*Build internal momenta:*/

        for(i=1;!(i>int_lines);i++){

           /*From topology:*/
           momentum=topologies[cn_topol].momenta[0][topologies[cn_topol].l_subst[i]];
           dir=topologies[cn_topol].l_dir[i];

           momentumBegin=tmp;
           build_momentum(group,momentum,tmp,dir);
           for(;(*tmp)!='\0';tmp++)
              if(++tlength +1 >=MAX_STR_LEN)
                 halt(TOOLONGSTRING,NULL);

           if(ielist[i]!='\0'){/*Add external momenta from the table:*/
               char *ptr;
               int dir2=(ielist[i]>0)?1:-1;
               if(   
                   (t->extMomenta != NULL)&&
                   ((t->extMomenta)[0]!=NULL)&&
                   (   ( ptr=(t->extMomenta)[0][abs(ielist[i])-1] )!=NULL   )
                 ){
                  switch(*ptr){
                     case '-':case'+':
                        s_letn(ptr,tmp,MAX_STR_LEN - ++tlength);
                        break;
                     default:
                        (*tmp)='+';
                        s_letn(ptr,tmp+1,MAX_STR_LEN - ++tlength);
                        break;
                  }/*switch(*ptr)*/
                  if(dir2 < 0 )
                     swapCharSng(tmp);/* must return 0*/
                 for(;(*tmp)!='\0';tmp++)
                    if(++tlength +1 >=MAX_STR_LEN)
                       halt(TOOLONGSTRING,NULL);
               }else{/*if(...)*/
                  *arg='\0';
                  goto theend;
               }
           }/*if(ielist[i]!='\0')*/

           if(ilist[i]!='\0'){/*Add internal momenta from the table:*/
               char *ptr;
               int dir2=ld[abs(ilist[i])]*( (ilist[i]>0)?1:-1 );
               l=t->l_red2usr[ls[abs(ilist[i])]];/*l - usr table*/

               if(   
                   (t->momenta != NULL)&&
                   ((t->momenta)[0]!=NULL)&&
                   (   ( ptr=(t->momenta)[0][l-1] )!=NULL   )
                 ){
                  switch(*ptr){
                     case '-':case'+':
                        s_letn(ptr,tmp,MAX_STR_LEN - ++tlength);
                        break;
                     default:
                        (*tmp)='+';
                        s_letn(ptr,tmp+1,MAX_STR_LEN - ++tlength);
                        break;
                  }/*switch(*ptr)*/
                  if(  (dir2*( (t->l_dirUsr2Red)[l] ))< 0 )
                     swapCharSng(tmp);/* must return 0*/
                 for(;(*tmp)!='\0';tmp++)
                    if(++tlength +1 >=MAX_STR_LEN)
                       halt(TOOLONGSTRING,NULL);
               }/*if(...)*/
           }/*if(ilist[i]!='\0')*/

           if(*momentumBegin == '\0'){/*empty*/
             s_letn(savedZeroVec,momentumBegin,MAX_STR_LEN - tlength);
             for(;(*tmp)!='\0';tmp++)
                if(++tlength +1 >=MAX_STR_LEN)
                   halt(TOOLONGSTRING,NULL);
           }/*if(*momentumBegin == '\0')*/
           if(i !=int_lines)
              *(tmp++)=',';
           if(++tlength +1 >=MAX_STR_LEN)
                 halt(TOOLONGSTRING,NULL);
           *tmp='\0';
        }/*for(i=1;!(i>int_lines);i++)*/
        s_let("_M",varname+basevarlen);
        set_run_var(varname,arg);
        g_zeroVec=savedZeroVec;
   }/*Block*/

   if(topologies[cn_topol].orig!=NULL)
      top2str(topologies[cn_topol].orig, arg);
   else
      top2str(topologies[cn_topol].topology, arg);

   theend:
      free(elist);free(ilist);free(ielist);free(varname);
      if(err!=NULL)
         halt(err,NULL);
      return arg;

}/*macamputateTopology*/

/*
\amputatedTopology(varname) creates and returns topology with amputatred
external legs. Creates local variables:
<varname>_IN - number of internal lines in a newly created amputated topology.

The following arrays amputated[original] are indices of line of amputated
topology which corresponds to original. Absolute value is an index, sign
corresponds to direction:
<varname>_E - external line of original <-> exetrnal line of amputated
<varname>_IE   internal line of original <-> internal line of amputated
<varname>_I internal line of original <-> exetrnal line of amputated
*/
static char *macamputatedTopology(char *arg)
{
tTOPOL ampTopol;

pTOPOL t=topologies[cn_topol].topology;
/*Note, t is 'topology', not 'orig!' We use only fields i_n,e_n, 
they are the same as in topologies[cn_topol].orig (if present), but
l_createAmputatedTopology uses 'topology', if possible!*/

int l;
char *elist=get_mem(t->e_n+2,sizeof(char)),
     *ilist=get_mem(t->i_n+2,sizeof(char)),
     *ielist=get_mem(t->e_n+2,sizeof(char)),
     *buf=get_mem(MAX_STR_LEN,sizeof(char));
   l_createAmputatedTopology(&ampTopol,elist,ilist,ielist);

   if( (l=s_len(get_string(buf)))>MAX_STR_LEN/2 )
      halt(TOOLONGSTRING,NULL);
   
   s_let("_IN",buf+l);
   set_run_var(buf,long2str(arg,ampTopol.i_n));

   s_let("_E",buf+l);
   l_setList(t->e_n,arg,buf,elist);

   s_let("_IE",buf+l);
   l_setList(t->i_n,arg,buf,ielist);


   s_let("_I",buf+l);
   l_setList(t->i_n,arg,buf,ilist);

   top2str(&ampTopol, arg);

   free(buf);free(elist);free(ilist);free(ielist);
   return arg;
}/*macamputatedTopology*/

/*returns number of lines:*/
static int str2dir(char *str, char *dir, char pluschar, char minuschar)
{
   char *sbuf=str,*dbuf=dir;
   int i,c=0;
   *dbuf++=-1;
     while(*sbuf!='\0'){
        if(sscanf(sbuf,"%d",&i)!=1)return(0);
        *dbuf++=(i<0)?(minuschar):(pluschar);c++;
        while(!set_in(*sbuf,digits))sbuf++;
        while(set_in(*sbuf,digits))sbuf++;
        if(*sbuf == delimiter_char)sbuf++;
        else if(*sbuf != '\0')return(0);
     }
     *dbuf='\0';
   return(c);
}/*str2dir*/

static int str2subst(char *str, char *subst)
{
   char *sbuf=str,*dbuf=subst,j;
   int i,c=0;
   set_of_char chk;
   set_sub(chk,chk,chk);

   *dbuf++=-1;

     while(*sbuf!='\0'){
        if(sscanf(sbuf,"%d",&i)!=1)return(0);
        if (i<0)i=-i;
        *dbuf=i;set_set(*dbuf,chk);dbuf++;c++;
        while(!set_in(*sbuf,digits))sbuf++;
        while(set_in(*sbuf,digits))sbuf++;
        if(*sbuf == delimiter_char)sbuf++;
        else if(*sbuf != '\0')return(0);
     }
     *dbuf='\0';
     for (j=1;!(j>c);j++){
        if(!set_in(j,chk))return(0);
     }
   return(c);
}/*str2subst*/

static char *subst2str(char *str, char *subst)
{
   register int i;
   register char *buf=str;
   *buf=0;
   if(*subst=='\0')return(str);
   subst++;
   while(*subst!='\0'){
      i=*subst++;
      sprintf(buf,"%d%c",i,delimiter_char);
      while(*buf!='\0')buf++;
   }
   *--buf='\0';
   return(str);
}/*subst2str*/
/*ilsubst(lsubst) -- takes the substitution as obtained by the operator

\reducetopologycanonical, and returns the substitution suitable to use
in the operator \setinternaltopology, by combining them with the existent
substitution "lsubst".*/
static char *macilsubst(char *arg)
{
char ls[MAX_I_LINE],ld[MAX_I_LINE],tmp[MAX_I_LINE];
int i;
if(islast)
         halt(CANNOTCREATEOUTPUT,NULL);

/*???
  *(l_subst=get_mem(int_lines+2,sizeof(char)))=-1;
  for(i=1;!(i>int_lines);i++)
      l_subst[i]=i;
*/
  *arg='\0';
  if(canonical_topology==NULL)return(arg);

  if (str2subst(get_string(arg),ls)!=int_lines)
        halt(INVALIDREORDERINGSTRING,NULL);
  str2dir(arg,ld,1,-1);
  tmp[0]=-1;
  for (i=1;!(i>int_lines);i++){
     tmp[i]=ls[lsubst[i]]*ld[lsubst[i]]*ldir[i];
  }
  tmp[i]='\0';
  return(subst2str(arg,tmp));
}/*macilsubst*/

/*ivsubst(vsubst) -- takes the substitution as obtained by the operator
\reducetopologycanonical, and returns the substitution suitable to use
in the operator \setinternaltopology, by combining them with the existent
substitution "vsubst".*/
static char *macivsubst(char *arg)
{
char vs[MAX_I_LINE],tmp[MAX_I_LINE];
int i;
  if(islast)
         halt(CANNOTCREATEOUTPUT,NULL);
  *arg='\0';
  if(canonical_topology==NULL)return(arg);
  if (str2subst(get_string(arg),vs)!=vcount)
        halt(INVALIDREORDERINGSTRING,NULL);
  tmp[0]=-1;
  for (i=1;!(i>vcount);i++){
     tmp[i]=vs[vsubst[i]];
  }
  tmp[i]='\0';
  return(subst2str(arg,tmp));
}/*macivsubst*/

/*"setinternaltopology(lsubst,vsubst)" -- sets canonical topology
according to lsubt, vsubst from the current topology. Return produced
topology:*/
static char *macsetinternaltopology(char *arg)
{
     if(g_nocurrenttopology)
         halt(CANNOTCREATEOUTPUT,NULL);
     if (str2subst(get_string(arg),vsubst)!=vcount)
        halt(INVALIDREORDERINGSTRING,NULL);

     if (str2subst(get_string(arg),lsubst)!=int_lines)
        halt(INVALIDREORDERINGSTRING,NULL);

     str2dir(arg,ldir,1,-1);
     canonical_topology = &buf_canonical_topology;
     reset_topology(topologies[cn_topol].topology,canonical_topology,
        lsubst,vsubst,ldir);
     invertsubstitution(ltsubst,lsubst);
     invertsubstitution(vtsubst,vsubst);

     top2str(canonical_topology, arg);
     return(arg);
}/*macsetinternaltopology*/

static char *maccanonicaltopology(char *arg)/*"canonicaltopology" -- returns
   string representation of the canonical form of the current topology*/
{

     if(g_nocurrenttopology)
         halt(CANNOTCREATEOUTPUT,NULL);
     if(canonical_topology ==NULL)
        make_canonical_topology();/*utils.c*/
     top2str(canonical_topology, arg);
     return(arg);
}/*maccanonicaltopology*/

/* converts internal line number to user defined:*/
static char *maci2u_line(char *arg)
{
 long num;
 char tmp[MAX_I_LINE];
  num=get_num("i2u_line",arg);
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
  if((num>int_lines)||(!(num>0)))
      halt(INVALIDNUMBER,arg);
  *arg=0;
  if(canonical_topology==NULL)return(arg);
  /*See "The substituions map" in "variables.h"*/
  /*??? Why invertsubstitution(tmp, l_subst) instead of il_subst??:*/
  substituter(s_let(lsubst,arg),invertsubstitution(tmp, l_subst)+1);
  invertsubstitution(tmp,arg);
  num=tmp[num];
  return(long2str(arg,num));
}/*maci2u_line*/

/* converts internal vertex number to user defined:*/
static char *maci2u_vertex(char *arg)
{
 long num;
 char tmp[MAX_I_LINE];
  num=get_num("i2u_vertex",arg);
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
  if((num>vcount)||(!(num>0)))
      halt(INVALIDNUMBER,arg);
  *arg=0;
  if(canonical_topology==NULL)return(arg);
  /*See "The substituions map" in "variables.h"*/
  substituter(s_let(vsubst,arg),invertsubstitution(tmp, v_subst)+1);
  invertsubstitution(tmp,arg);
  num=tmp[num];
  return(long2str(arg,num));
}/*maci2u_vertex*/

/* Compares internal line direction with the corresponding user defined:*/
static char *maci2u_sign(char *arg)
{
 long num;
  num=get_num("i2u_sign",arg);
  if(g_nocurrenttopology)
      halt(CANNOTCREATEOUTPUT,NULL);
  if((num>int_lines)||(!(num>0)))
      halt(INVALIDNUMBER,arg);
  *arg=0;
  if(canonical_topology==NULL)return(arg);
  /*See "The substituions map" in "variables.h"*/
  num=ldir[ltsubst[num]]*l_dir[ltsubst[num]]*
  topologies[cn_topol].l_dir[lt_subst[ltsubst[num]]];
  if(num<0l)*arg='-';else*arg='+';
  arg[1]='\0';
  return(arg);
}/*mai2u_sign*/

/* converts user defined line number to internal:*/
static char *macu2i_line(char *arg)
{
 long num;
 char tmp[MAX_I_LINE];
  num=get_num("u2i_line",arg);
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
  if((num>int_lines)||(!(num>0)))
      halt(INVALIDNUMBER,arg);
  *arg=0;
  if(canonical_topology==NULL)return(arg);
  /*See "The substituions map" in "variables.h"*/
  substituter(s_let(lsubst,tmp),invertsubstitution(arg, l_subst)+1);
  num=tmp[num];
  return(long2str(arg,num));
}/*macu2i_line*/

/* converts user defined vertex number to internal:*/
static char *macu2i_vertex(char *arg)
{
 long num;
 char tmp[MAX_I_LINE];
  num=get_num("u2i_vertex",arg);
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
  if((num>vcount)||(!(num>0)))
      halt(INVALIDNUMBER,arg);
  *arg=0;
  if(canonical_topology==NULL)return(arg);
  /*See "The substituions map" in "variables.h"*/
  substituter(s_let(vsubst,tmp),invertsubstitution(arg, v_subst)+1);
  num=tmp[num];
  return(long2str(arg,num));
}/*macu2i_vertex*/

/* Compares user defined line direction with the corresponding internal:*/
static char *macu2i_sign(char *arg)
{
 long num;
  num=get_num("u2i_sign",arg);
  if(g_nocurrenttopology)
      halt(CANNOTCREATEOUTPUT,NULL);
  if((num>int_lines)||(!(num>0)))
      halt(INVALIDNUMBER,arg);
  *arg=0;
  if(canonical_topology==NULL)return(arg);
  /*See "The substituions map" in "variables.h"*/
  invertsubstitution(arg, l_subst);*arg=-1;/*Now arg[Vnusr]=Vred*/
  num=ldir[arg[num]]*l_dir[arg[num]]*
  topologies[cn_topol].l_dir[lt_subst[arg[num]]];

  if(num<0l)*arg='-';else*arg='+';
  arg[1]='\0';

  return(arg);
}/*macu2i_sign*/
/*Returns vertex number of internal topology connected
with given external leg:*/
static char *macwhereleg(char *arg)
{
 long num,tmp;
 int i;
  num=get_num("whereleg",arg);
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
  if((num>ext_lines)||(!(num>0)))
      halt(INVALIDNUMBER,arg);
  *arg=0;
  if(canonical_topology==NULL)return(arg);
  for (i=1; !(i>canonical_topology->e_n);i++){
     tmp=0 - canonical_topology->e_line[i].from;
     if(tmp == num){
       tmp=canonical_topology->e_line[i].to;
       long2str(arg,tmp);
       break;
     }
  }
  return(arg);
}/*macwhereleg*/

static char *macexternalpartoftopology(char *arg)/*"externalpartoftopology"
   -- returns external part of the canonical form of current topology*/
{
  char i;
  char tmp[12];

     if(islast)
         halt(CANNOTCREATEOUTPUT,NULL);
     *arg=0;
     if(canonical_topology==NULL)return(arg);
     for (i=canonical_topology->e_n; i>0;i--){
        sprintf(tmp,"(%hd,%hd)",canonical_topology->e_line[i].from,
                                  canonical_topology->e_line[i].to);
        s_cat(arg,arg,tmp);
     }
     return(arg);
}/*macexternalpartoftopology*/

static char *macinternalpartoftopology(char *arg)/*"internalpartoftopology"
   -- returns internal part of the canonical form of current topology*/
{
  char i;
  char tmp[12];
     if(islast)
         halt(CANNOTCREATEOUTPUT,NULL);
     *arg=0;
     if(canonical_topology==NULL)return(arg);

     for (i=1;!(i>canonical_topology->i_n);i++){
        sprintf(tmp,"(%hd,%hd)",canonical_topology->i_line[i].from,
                                  canonical_topology->i_line[i].to);
        s_cat(arg,arg,tmp);
     }
     return(arg);
}/*macinternalpartoftopology*/

typedef int (*RTOP)(pTOPOL,pTOPOL,char*,char*,char*);

static char *performreducetopology(char *arg,RTOP reducetop)
{
char ls[MAX_I_LINE],vs[MAX_I_LINE],ld[MAX_I_LINE],tmps[MAX_I_LINE];
char tmp[MAX_STR_LEN];
int maxv;
tTOPOL originaltopology, reducedtopology;
     get_string(tmp);/*get the name of the variable*/
     *vs=*ls=*ld=-1;
     if((maxv=string2topology(get_string(arg),&originaltopology))==-1)
        return(s_let("",arg));
     *arg='\0';maxv++;
     if(!( maxv< MAX_I_LINE ))return(arg);
     { int i;
       for(i=1;i<maxv;i++)vs[i]=i;vs[i]='\0';
       for(i=1;!(i>originaltopology.i_n);i++){ls[i]=i;ld[i]='\1';}
       ls[i]=ld[i]='\0';
     }
     reducetop(&originaltopology,&reducedtopology,ls,vs,ld);
      /*Now all arguments of substitutions are of originaltopology*/
      /* reset ld[originaltopology]->ld[reducedtopology]:*/
     { int i;
        for(i=1;!(i>originaltopology.i_n);i++)
           tmps[ls[i]]=ld[i];
        tmps[i]='\0';
     }
     tmps[0]=-1;
     s_let(tmps,ld);
     /* reset ls[originaltopology]->ls[reducedtopology]:*/
     invertsubstitution(tmps,ls);
     /*Set signes to new ls according to ld:*/
     { int i;
        for(i=1;!(i>originaltopology.i_n);i++)
           ls[i] = tmps[i]*ld[i];
     }
     set_run_var(s_cat(tmp,tmp,"_L"),subst2str(arg, ls));
     tmp[s_len(tmp)-2]='\0';
     /* reset vs[originaltopology]->vs[reducedtopology]:*/
     invertsubstitution(tmps,vs);
     /*Store vs:*/
     set_run_var(s_cat(tmp,tmp,"_V"),subst2str(arg, tmps));
     top2str(&reducedtopology, arg);
     return(arg);
}/*performreducetopology*/

/*"\reducetopology(topology,varname) -- returns reduced form of "topology",
 * and sets local variables varname_L and varname_V to line and vertex
 * substitutions, respectively, according to lubst[reduced]=top:*/
static char *macreducetopology(char *arg)
{
     return(performreducetopology(arg,reduce_topology_full));
}/*macreducetopology*/

/*"\reducetopologycanonical(topology,varname) -- returns canonical form of
 * "topology", and sets local variables varname_L and varname_V to line and
 * vertex substitutions, respectively, according to lubst[canonical]=top:*/
static char *macreducetopologycanonical(char *arg)
{
     return(performreducetopology(arg,reduce_internal_topology));
}/*macreducetopologytopologycanonical*/

static char *macprototype(char *arg)/*"prototype" -- returns current
   prototype.*/
{
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
  return(s_let(prototypes[cn_prototype],arg));
}/*macprototype*/

static char *macmomentatext(char *arg)/*"\momentatext(n) -- returns
   momenta set number "n" as it is set in current topology, or "",
   if something wrong*/
{
char group[NUM_STR_LEN];
char buf[MAX_STR_LEN];
int i,n,s,*momentum;

   *arg='\0';*buf='\0';
   if((g_nocurrenttopology)||(is_bit_set(&mode,bitTERROR)))
      return(arg);
   if(((n=get_num("momentatext",arg))<1)||(n>topologies[cn_topol].n_momenta))
     return(arg);
   n--;/* Momenta sets are accounted from 0*/
   group[0]=3;group[1]=group[2]=1;group[3]='\n';group[4]=0;
   *arg='[';arg[1]='\0';
   /* s is initialized by 1 since it is 1 for all external legs*/
   for(s=1,i=-ext_lines; !(i>int_lines);i++){
      if (i<0){/*external line*/
        if(
         (topologies[cn_topol].ext_momenta!=NULL) &&
         (topologies[cn_topol].ext_momenta[n]!=NULL)
        )
           momentum=topologies[cn_topol].ext_momenta[n][-i];
        else/* Common external momenta*/
           momentum=ext_particles[-i-1].momentum;
      }else if (i>0){
         momentum=topologies[cn_topol].momenta[n][topologies[cn_topol].l_subst[i]];
         s=topologies[cn_topol].l_dir[i];
      }else{/* Just 0, only add ']':*/
         s_cat(arg,arg,"] ");
         continue;
      }
      if((i!=-ext_lines)&&(i!=1))s_cat(arg,arg,",");
      s_cat(arg,arg,build_momentum(group,momentum,buf, s));
   }/*for(i=-ext_lines; !(i>int_lines);i++)*/
   return(arg);
}/*macmomentatext*/

/*Working routine for macmomentum (isLogic=0) and macvectorOnLine (isLogic=1),
  see below:*/
static char *l_momentum(char *arg,int isLogic,char *name)
{
char group[MAX_STR_LEN],l;
char i;
int dir=1;
int **tmpMomenta;
  if((g_nocurrenttopology)||(is_bit_set(&mode,bitTERROR)))
      halt(CANNOTCREATEOUTPUT,NULL);

  if(islast ){/*Pure topology, we have to build momenta first...*/
     if( max_momentaset <0 )/*There are no momenta in this topology!*/
        halt(UNDEFMOM,NULL);
     tmpMomenta=topologies[cn_topol].momenta[cn_momentaset];
  }else{
     if(!text_created)/*Which momentum? It is not built!*/
        halt(OUTPUTEXPRESSIONNOTCREATED,NULL);
     tmpMomenta=momenta;
  }
  group[0]=3;group[1]=127;

  {/*block*/
  char *ptr=arg;
     switch( *get_string(arg) ){
        case '-':
           dir=-1;
           /*No break*/
        case '+':
           ptr++;
           /*No break!*/
        default:
           if(*ptr!='\0'){
              s_letn(ptr,group+3,MAX_STR_LEN-4);
              group[2]=2;
              arg[0]=1;arg[1]='\n';arg[2]=0;
              s_cat(group,group,arg);
           }else{
              group[2]=1;group[3]='\n';group[4]=0;
           }
           break;
     }/*switch( *get_string(arg) )*/
  }/*block*/

  if(((l=get_num(name,arg))>int_lines)||(l==0)||(l<-ext_lines))
     halt(INVALIDNUMBER,arg);
  if(l>0){
    i=il_subst[l];/*Now i is red!*/

    if(g_flip_momenta_on_lsubst)/*Upset momenta on negative lsubst:*/
       dir*=l_dir[i];

    if(isLogic)
       dir*=topologies[cn_topol].l_dir[lt_subst[i]]*l_dir[i];/*Last l_dir
          is needed because the line is upset on negative lsubst! This is correct!*/
    else
       dir*=direction(i);
    /*build_momentum(group,tmpMomenta[i],arg, dir*topologies[cn_topol].l_dir[lt_subst[i]]);*/
    build_momentum(group,tmpMomenta[i],arg, dir);

    /*See "The substituions map" in "variabls.h"*/
  }else{ /*External line*/
    int *momentum;
    if(
         (topologies[cn_topol].ext_momenta!=NULL) &&
         (topologies[cn_topol].ext_momenta[cn_momentaset]!=NULL)
       )
       momentum=topologies[cn_topol].ext_momenta[cn_momentaset][-l];
    else
      momentum=ext_particles[-l-1].momentum;
    if(!isLogic)
       dir*=direction(l);/*For external lines  - we must take into account
                           fermion number flow!*/

    build_momentum(group,momentum,arg, dir);
  }

  return(arg);
}/*l_momentum*/

static char *macmomentum(char *arg)/*"momentum(line,vec)" -- returns
   momentum on line l corresponding to vector vec. If vec is empty,
      returns full momentum. Returned value is the same as in created
      text for propagators:*/
{
   return l_momentum(arg,0,"momentum");
}/*macmomentum*/

static char *macvectorOnLine(char *arg)/*"vectorOnLine(line,vec)" -- returns
   momentum on line l corresponding to vector vec. If vec is empty,
   returns full momentum. Returned value is directed along the nusr line:*/
{
    return l_momentum(arg,1,"vectorOnLine");
}/*macvectorOnLine*/

static char *macexternalmomentum(char *arg)/*"\externalmomentum(line,vec)" -- returns
   momentum on external line l corresponding to vector vec. If vec is empty,
   returns full momentum. All external momenta assumed to be ingoing!*/
{
char group[MAX_STR_LEN];
long l;

  if((g_nocurrenttopology)||(is_bit_set(&mode,bitTERROR)))
      halt(CANNOTCREATEOUTPUT,NULL);
/*???
  if(!text_created)
      halt(OUTPUTEXPRESSIONNOTCREATED,NULL);
*/

  group[0]=3;group[1]=127;group[2]=2;
  arg[0]=1;arg[1]='\n';arg[2]=0;

  if(*get_string(group+3)==0){
     group[1]=group[2]=1;group[3]='\n';group[4]=0;
  }else
    s_cat(group,group,arg);

  if( (l=get_num("externalmomentum",arg))<0)l=-l;
  if( (l==0)||(l > ext_lines))
     halt(INVALIDNUMBER,arg);
  else{ int i, *momentum;
    if(
         (topologies[cn_topol].ext_momenta!=NULL) &&
         (topologies[cn_topol].ext_momenta[cn_momentaset]!=NULL)
       )
       momentum=topologies[cn_topol].ext_momenta[cn_momentaset][l];
    else
      momentum=ext_particles[l-1].momentum;

    if (ext_particles[l-1].is_ingoing==0)/* Outgoing*/
      i=-1;
    else
      i=1;
    build_momentum(group,momentum,arg, i);
  }
  return(arg);
}/*macexternalmomentum*/


/*\extD2Q(n) (if n<0) maps Diana numbering of external legs to QGRAF
model. If n>0, it just returns n. If n==0, returns an empty string.

   QGRAF numerates external legs as follows: INgoing legs have odd
numbers like -1,-3,-5, etc.  OUTgoing legs have even numbers like
-2,-4, etc.

   Diana numerates external legs as follows:
First, ingoing legs in natural order e.g.:
-1,-2
then outgoing legs:
-3,-4
*/
static char *macextD2Q(char *arg)
{
int l=get_num("extD2Q",arg);
   if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
   if(l>0)
      return long2str(arg,l);
   if(l==0)
      return s_let("",arg);
   l=-l;

   /*Now l>0*/

   if(l>g_last_ingoing)/*Outgoing - even according to QGRAF*/
     return long2str(arg,2*(g_last_ingoing-l));

   /*Ingoing: odd according to QGRAF*/
   return long2str(arg,-2*l+1);
}/*macextD2Q*/


static char *macvertexInfo(char *arg)/*\vertexInfo(v,root) returns number of tails 
                                      * coinciding this vertex. Sets local variables:
                                      * rootv1,rootv2,etc., rootl1,rootl2,etc.
                                      * where rootv# is an identifier of the corresponding
                                      * particle and rootl# is the line number (nusr, see
                                      * the comment "The substitutions map" in variabls.h)
                                      */
{
  int v;
  int i;
  int n;/*number of tails*/
  char *eor=/*eor stands for "End Of Root"*/
         get_string(arg);/*get "root"*/
  char buf[NUM_STR_LEN];

    /*get v:*/
    if( (v=get_num("vertexInfo",buf))<1 )
       halt(INVALIDNUMBER,buf);

    if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);

    if(v>vcount)/*Permit this operator to check is this v index valid*/
       return s_let("",arg);   

    /* v is nusr, convert to red (see the comment "The substitutions map" in variabls.h):*/
    v=iv_subst[v];

    /*Adjust eor:*/
    while( (*eor)!='\0' )eor++;
    n=*(id[dtable[v][0]-1].id);/*id is m-string!*/

    for (i=0; i<n; i++){
       /* Array vertices is a global variable of types struct vertices_struct
          it is set up in read_inp.c in create_indices_table()*/

       /*Set rootl# to the line number:*/
       long2str(s_let("l",eor)+1,i+1);
       {/*Block*/
          int l=vertices[v][i].n;
          if(l>0)/*internal - convert red to nusr*/
            l=l_subst[l];
          set_run_var(arg,long2str(buf, l));
       }/*Block*/
       /*Set rootp# to the particle identifier:*/
       (*eor)='p';
       set_run_var(arg,vertices[v][i].id);
    }/*for (i=0; i<n; i++)*/
    return long2str(arg,n);
}/*macvertexInfo*/

static char *macisingoing(char *arg)/*"\isingoing(extline)" -- returns "+","-" or ""
                                     * according ingoing,outgoing or no such
                                     * external line. If extline<0 then extline=-extline
                                     */
{
long l;
  if( (l=get_num("isingoing",arg))<0)l=-l;
  if( (l==0)||(l > ext_lines))
    *arg='\0';
  else if (ext_particles[l-1].is_ingoing==0)/* Outgoing*/
    *arg='-';
  else
    *arg='+';
  arg[1]='\0';
  return (arg);
}/*macisingoing*/

/*Returns previous mark, or "":*/
static char *macsetlinemark(char *arg)
{
char *newmark;
int j,n;
   if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
   newmark=new_str(get_string(arg));/*Get new linemark*/
   if(((n=get_num("setlinemark",arg))>int_lines)||(n<1))
       halt(INVALIDNUMBER,arg);
   /* Now we have n -- "nusr" line number, see
    * "The substituions map" in variabls.h*/
   if(linemarks == NULL){
       linemarks=get_mem(int_lines+1,sizeof(char*));
       linemarks[0]=new_str(long2str(arg,int_lines));
       for(j=1;!(j>int_lines);j++){
          if(j==n)
             linemarks[j]=newmark;
          else
             linemarks[j]=new_str(long2str(arg,j));
       }/*for(j=1;!(j>int_lines);j++)*/
       *arg='\0'; /* Return "" -- no previous linemarks!*/
   }else{
      s_let(linemarks[n],arg);
      free_mem(&(linemarks[n]));
      linemarks[n]=newmark;
   }
   return(arg);
}/*macsetlinemarks*/

/*Returns previous mark, or "":*/
static char *macsetvertexmark(char *arg)
{
char *newmark;
int j,n;
   if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
   newmark=new_str(get_string(arg));/*Get new vertexmark*/
   if(((n=get_num("setvertexmark",arg))>vcount)||(n<1))
       halt(INVALIDNUMBER,arg);
   /* Now we have n -- "nusr" vertex number, see
    * "The substituions map" in variabls.h*/
   if(vertexmarks == NULL){
       vertexmarks=get_mem(vcount+1,sizeof(char*));
       vertexmarks[0]=new_str(long2str(arg,vcount));
       for(j=1;!(j>vcount);j++){
          if(j==n)
             vertexmarks[j]=newmark;
          else
             vertexmarks[j]=new_str(long2str(arg,j));
       }/*for(j=1;!(j>vcount);j++)*/
       *arg='\0'; /* Return "" -- no previous vertexmarks!*/
   }else{
      s_let(vertexmarks[n],arg);
      free_mem(&(vertexmarks[n]));
      vertexmarks[n]=newmark;
   }
   return(arg);
}/*macsetvertexmarks*/

/* \linemarks(#) -- returns "linemark" corresponding to the line # ( nusr,
 * look "substitutions map " in the file variabls.h):*/
static char *maclinemarks(char *arg)
{
int n;
   if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
   if(((n=get_num("linemarks",arg))>int_lines)||(n==0))
       halt(INVALIDNUMBER,arg);
   if(n<0){/*external line? Just repeat it:*/
      long2str(arg,n);
      return arg;
   }/*if(n<0)*/
   /* Now we have n -- "nusr" line number, see
    * "The substituions map" in variabls.h.
    * Get "usr" number:*/
   /*n=lt_subst[il_subst[n]];!!! -- change the behaviour! -- 19.03.00*/
   /* Now n is "usr" number (19.03.00 changed -- n is nusr!)*/
   if(linemarks == NULL)
      long2str(arg,n);
   else
      s_let(linemarks[n],arg);
   return(arg);
}/*maclinemarks*/

/* \vertexmarks(#) -- returns "vertexmark" corresponding to the vertex # ( nusr,
 * look "substitutions map " in the file variabls.h):*/
static char *macvertexmarks(char *arg)
{
int n;
   if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
   if(((n=get_num("vertexmarks",arg))>vcount)||(n<1))
       halt(INVALIDNUMBER,arg);
   /* Now we have n -- "nusr" vertex number, see
    * "The substituions map" in variabls.h.
    * Get "usr" number:*/
   /*n=vt_subst[iv_subst[n]];!!! -- change the behaviour! -- 19.03.00*/
   /* Now n is "usr" number (19.03.00 changed -- n is nusr!)*/
   if(vertexmarks == NULL)
      long2str(arg,n);
   else
      s_let(vertexmarks[n],arg);
   return(arg);
}/*macvertexmarks*/

static char *macmass(char *arg)/*"mass(l)" :returns mass on line l.*/
{
 int i;
 char l;
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
    if(((l=get_num("mass",arg))>int_lines)||(l<-ext_lines)||(l==0))
       halt(INVALIDNUMBER,arg);
    if(l>0){
      i=il_subst[l];
    }else
      i=l;
    return(s_let(id[dtable[0][i]-1].mass,arg));
}/*macmass*/

static char *macfcountoutterm(char *arg)/*"fcountoutterm(n)" -- returns
                                   fermion line counter of n term in output*/
{
long num;
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
  if(!text_created)
     halt(OUTPUTEXPRESSIONNOTCREATED,NULL);
  if(((num=get_num("fcountoutterm",arg))>top_out)||(!(num>0)))
        halt(INVALIDOUTNUMBER,arg);
   return(long2str(arg,output[num-1].fcount));
}/*macfcountoutterm*/

static char *maclinefcount(char *arg)/*"linefcount(l)" :returns fermion line
        counter of identifier on line l.*/
{
 int i;
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
    if(((i=get_num("linefcount",arg))>int_lines)||(i==0)||(i<-ext_lines))
       halt(INVALIDNUMBER,arg);
    if(i>0)/*Internal line*/
       return(long2str(arg,output[l_outPos[il_subst[i]]].fcount));
    /*else -- external line*/
    return(long2str(arg,ext_particles[-i-1].fcount));
}/*maclinefcount*/

static char *macvertexfcount(char *arg)/*"vertexfcount(v)" :returns fermion
    line counter of identifier on vertex v.*/
{
 int i;
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
    if(((i=get_num("vertexfcount",arg))>vcount)||(i<1))
       halt(INVALIDNUMBER,arg);
    return long2str(arg,output[v_outPos[iv_subst[i]]].fcount);
}/*macvertexfcount*/

static char *macmaxfcount(char *arg)/*"maxfcount()" :returns maximal
   fermion line counter of identifiers in current diagram.*/
{
 int i,max=0;
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
    for(i=0;i<top_out;i++)
       if(output[i].fcount>max)
          max=output[i].fcount;
    return(long2str(arg,max));
}/*macmaxfcount*/

static char *maclinefflow(char *arg)/*"linefflow(l)" :'0',-1,+1 depending on co-incide
fermion number flow with the fermion flow (+1), opposite (-1), or undefined (0). */
{
 int i;
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
    if(((i=get_num("linefflow",arg))>int_lines)||(i==0)||(i<-ext_lines))
       halt(INVALIDNUMBER,arg);
    if(i>0)/*Internal line*/ /*see comment in pilot.c, seek  'MAJORANA fermions'*/
       return(long2str(arg,output[l_outPos[il_subst[i]]].majorana));
    /*else -- external line*/
    return(long2str(arg,ext_particles[-i-1].majorana));
}/*maclinefflow*/

static char *macvertexfflow(char *arg)/*"vertexfflow(v)" : '0',-1,+1 depending on co-incide
fermion number flow with the fermion flow (+1), opposite (-1), or undefined (0). For
vertices it  will be -1, if at least one line has (-1).*/
{
 int i;
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
    if(((i=get_num("vertexfflow",arg))>vcount)||(i<1))
       halt(INVALIDNUMBER,arg);
    /*see comment in pilot.c, seek  'MAJORANA fermions'*/
    return long2str(arg,output[v_outPos[iv_subst[i]]].majorana);
}/*macvertexfflow*/

static char *maclinetype(char *arg)/*"linetype(l)" :returns type of line l.*/
{
 int i;
 char l;
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
    if(((l=get_num("linetype",arg))>int_lines)||(l<-ext_lines)||(l==0))
       halt(INVALIDNUMBER,arg);
    arg[1]=0;
    if(l<0)
       i=l;
    else
       i=il_subst[l];
    *arg=id[dtable[0][i]-1].type;
    return(arg);
}/*maclinetype*/

static char *macvertextype(char *arg)/*"vertextype(v)" :returns type of
     vertex v.*/
{
 char v;
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
    if(((v=get_num("vertextype",arg))>vcount)||(v<1))
       halt(INVALIDNUMBER,arg);
    arg[1]=0;
    *arg=id[dtable[iv_subst[v]][0]-1].type;
    return(arg);
}/*macvertextype*/

static char *macgetstr(char *arg)/*"getstr(n)" -- returns n string from text
                                     block stored by "find"*/
{
long n=0;
  get_string(arg);/*Get the number of string*/
  if(sscanf(arg,"%ld",&n)!=1){
      char tmp[MAX_STR_LEN];
      sprintf(tmp,NUMERROR,"getstr",arg);
      halt(tmp,NULL);
  }
  if(top_txt<n)
      halt(TOOBIGSTRINGNUMBER,arg);
  return(s_let(text_block[n-1],arg));
}/*macgetstr*/

static char *maccount(char *arg)/*"count(expr)": counts number of
                                   occurence expr in texts*/
{
int count=0,i;
   get_string(arg);
   if (text_block==NULL)return(s_let("0",arg));
   for(i=0;i<top_txt;i++)
      count+=s_count(arg,text_block[i]);
   return(long2str(arg,count));
}/*maccount*/

static char *macsetfind(char *arg)/*"setfind(from_expr,to_expr,end_expr)":
                                 "find" will find starting from from_expr,
                                  till "to_expr" and will
                               use end_expr as the expression terminator*/
{
  get_string(end_expr);
  get_string(to_expr);
  get_string(from_expr);
  *arg=0;
  return(arg);
}/*macsetfind*/

static int specreadflag=1;/*1:begin; 0:process is going; -1:end*/

static int specread(char *str)
{
int i=0;
  if(specreadflag==-1)
     return(-1);
  str=fgets(str, MAX_STR_LEN,ext_file);
  if(specreadflag==1){
     if(str==NULL)
        return(specreadflag=-1);
     if(*from_expr==0)specreadflag=0;
     else if((i=s_pos(from_expr,str))==-1)
        return(1);
     else{/* from_expr occure*/
        s_del(str,str,0,i);
        specreadflag=0;
     }
  }
  if(specreadflag==0){
     if(str==NULL)
        return(specreadflag=-1);

     if((*to_expr)&&((i=s_pos(to_expr,str))!=-1)){
        str[i+s_len(to_expr)]=0;
        specreadflag=-1;
     }
     return(0);
  }
  return(-1);
}/*specread*/

static char *macfind(char *arg)/*"find(pattern,n)": find n occurrence of
                                  pattern and store expression, using
                                  end_expr as terminator, returns number of
                                  found lines*/
{
int i,j,n;
long fp=0,savedpos=0;
char tmp[MAX_STR_LEN];
  if(ext_file==NULL)
     halt(NOOPENEDFILE,NULL);
  savedpos=ftell(ext_file);/*Store file position for readln*/
  fseek(ext_file, 0, SEEK_SET);
  specreadflag=1;

  do{
     fp=ftell(ext_file);/*Store file position*/
  }while((i=specread(tmp))==1);/*Skip ext_file till from_expr*/
  /*Now fp points to the string with first from_expr occurence*/

  if(i==-1){/* EOF */
     get_string(arg);/*Get the number of occurence*/
     get_string(arg);/*Get the pattern*/
     fseek(ext_file, savedpos, SEEK_SET);
     return(s_let("0",arg));
  }
  get_string(arg);/*Get the number of occurence*/
  if(sscanf(arg,"%d",&n)!=1){
      sprintf(tmp,NUMERROR,"find",arg);
      halt(tmp,NULL);
  }
  if(n<0) n=0;
  if(*get_string(arg)==0){/*Get the pattern*/
     fseek(ext_file, savedpos, SEEK_SET);
     return(s_let("0",arg));/*If it is equal to zero, returns emty srting*/
  }
  /* Count how many times pattern occures:*/
  j=0;
  do{
     if((i=s_pos(arg,tmp))!=-1)j++;
     /*Now j is equal to number of pattern occurence*/
  }while(specread(tmp)!=-1);
  if(
     (!j)/* No pattern occurence*/
     ||(n>j)/*Pattern occures less than n times*/
    ){
     fseek(ext_file, savedpos, SEEK_SET);
     return(s_let("0",arg));
  }
  if(!n) n=j;/*If n==0, then find last occurence*/
  /* Clear previous results:*/
  for(j=0;j<top_txt;j++)
     free_mem(&(text_block[j]));
  top_txt=0;
  fseek(ext_file, fp, SEEK_SET);/*Pointer points just before from_expr*/
  specreadflag=1;
  j=0;
  specread(tmp);
  fp=0;/*use fp as break flag*/
  do{
     if((i=s_pos(arg,tmp))!=-1)j++;
     /*Now j is equal to number of pattern occurence*/
     if(!(j<n)){
        if((i!=-1)&&(j==n))
           s_del(tmp,tmp,0,i);

        if((*end_expr)&&((i=s_pos(end_expr,tmp))!=-1)){
           tmp[i+s_len(end_expr)]=0;
           fp=1;
        }
        if(!(top_txt<max_top_txt))/*possible expansion text_block*/
           if((text_block=(char **)realloc(text_block,
               (max_top_txt+=DELTA_TEXT_BLOCK)*sizeof(char*)))==NULL)
                  halt(NOTMEMORY,NULL);
        text_block[top_txt++]=new_str(tmp);
        if(fp)break;
     }
  }while(specread(tmp)!=-1);
  long2str(arg,top_txt);
  fseek(ext_file, savedpos, SEEK_SET);
  return(arg);
}/*macfind*/

static char *mackillexp(char *arg)/*"killexp(var)" - kills global
variable var, returns "ok" if it exists, or "none".*/
{
  if(uninstall(get_string(arg),export_table))
     s_let("none",arg);
  else
     s_let("ok",arg);
  return(arg);
}/*mackillexp*/

static char *mackillvar(char *arg)/*"killvar(var)" - kills local variable
var, returns "ok" if it exists, or "none".*/
{
  if(uninstall(get_string(arg),variables_table))
     s_let("none",arg);
  else
     s_let("ok",arg);
  return(arg);
}/*mackillvar*/

static char *macremovefile(char *arg)/*"removefile(fname)" removes the file*/
{
  if(remove(get_string(arg)))s_let("none",arg);else s_let("ok",arg);
  return(arg);
}/*macremovefile*/

static char *macopen(char *arg)/*"open":open file for "readln" and "find"*/
{
    close_file(&ext_file);
    if(!s_scmp(get_string(arg),"null")){
      s_let("ok",arg);
    }else if(  (ext_file= open_file_follow_system_path(arg,"rt"))==NULL  ){
       s_let("none",arg);
    }else{
       readln_eof=0;
       s_let("ok",arg);
    }
    return(arg);
}/*macopen*/

static char *macreadln(char *arg)/*"readln". Reads string from ext_file*/
{
char *e;
  if(ext_file==NULL)
     halt(NOOPENEDFILE,NULL);
  if(fgets(arg, MAX_STR_LEN,ext_file)==NULL){
     if(readln_eof)
        halt(UNEXPECTEDEOF,NULL);
     readln_eof=1;
     *arg=chEOF;arg[1]=0;
     return(arg);
  }
  if(*(e=arg+s_len(arg)-1)=='\n')*e=0;/*Avoid '\n' at the end of line*/
  return(arg);
}/*macreadln*/

static char *macmessage(char *arg)/*"message". Types its argument into
                                    standart output*/
{
int oldmsg=message_enable;
  message_enable=1;
  message(get_string(arg),NULL);
  message_enable=oldmsg;
  return(arg);
}/*macmessage*/

static char *macreturn(char *arg)/*"return". Stops run executing,puts its
    argument into returnedvalue, sets isreturn=1, returns empty string.*/
{
   isreturn=1;
   letnv(get_string(arg),returnedvalue,MAX_STR_LEN);
   *arg=0;
   return(arg);
}/*macreturn*/

static char *macobracket(char *arg)/*"obracket" -- returns "(".*/
{
   return(s_let("(",arg));
}/*macobracket*/

static char *maccbracket(char *arg)/*"cbracket" -- returns ")".*/
{
   return(s_let(")",arg));
}/*maccbracket*/

static char *macinc(char *arg)/*"inc(var,val)" -- increments value of "val"
   by "val". Returns new value.*/
{
long i,j;
char arg1[MAX_STR_LEN],*tmp;
   i=get_num("inc",arg);
   get_string(arg);
   if ((variables_table==NULL)||((tmp=lookup(arg,variables_table))==NULL))
         halt(UNDEFINEDVAR,arg);
   if(sscanf(tmp,"%ld",&j)!=1){
      sprintf(arg,NUMERROR,"inc",tmp);
      halt(arg,NULL);
   }
   long2str(arg1,i+j);
   install(new_str(arg),new_str(arg1),variables_table);

   return(s_let(arg1,arg));
}/*macinc*/

static char *macparticle(char *arg)/*"particle(l)" :returns particle
                                      on line l.*/
{
 int i;
 char l;
 char *p;
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
    if(((l=get_num("particle",arg))>int_lines)||(l<-ext_lines)||(l==0))
       halt(INVALIDNUMBER,arg);
    if(l<0){
      int n;
      i=l;
      for(n=1;!(n>vcount);n++)
         if (dtable[n][i]!=0)break;
      if (ext_particles[-l-1].is_ingoing==0)
         p=id[id[dtable[n][i]-1].link[0]].id;
      else
         p=id[dtable[n][i]-1].id;
      l=1;
    }else{
      i=il_subst[l];
      p=id[dtable[0][i]-1].id;
      if ((l=pdirection(i))==0)
         halt(INVALIDNUMBER,arg);
    }
    if(l==1)
       l=0;
    else
       for(l=1;*(p+l)!=0;l++);
    return(s_let(p+l+1,arg));
}/*macparticle*/

static char *l_beginpropagator(int l)
{
int i;
char *p;
    if(l<0){
      int n;
      i=l;
      for(n=1;!(n>vcount);n++)
         if (dtable[n][i]!=0)break;

      p=id[id[id[dtable[n][i]-1].link[1]].link[0]].id;
      l=1;
    }else{
      i=il_subst[l];
      if ((l=pdirection(i))==0)
         halt(INTERNALERROR);
      p=id[id[id[dtable[0][i]-1].link[1]].link[0]].id;
    }
    return p+1;

}/*l_beginpropagator*/

static char *macbeginpropagator(char *arg)/*"begipropagator(l)":
                                   returns begin of propagator on line l.*/
{

int l;
  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
    if(((l=get_num("beginpropagator",arg))>int_lines)||
                                          (l<-ext_lines)||(l==0))
       halt(INVALIDNUMBER,arg);
    return(s_let(l_beginpropagator(l),arg));
}/*macbeginpropagator*/

static void l_outimage(char *str)
{
char *tmp=str;
  flush();
  for(;(*tmp)!='\0';tmp++)
     outPS(tmp,NULL);
  outputstr(str);
  outputstr("\n");
}/*l_outimage*/

/*Converts line number to linemark:*/
static char *l_l2mark( int l, char *tmp)
{

  if(  (linemarks == NULL)||(l<1)  )
      return long2str(tmp,l);

   return s_let(linemarks[l],tmp);
}/*l_l2mark*/

/* \_PDiaImages(formatstring) - used to build procedures of a particle drawing in
 .eps files It loops at all particles in current diagram.
 Builds:
 /Img<part><proc> bind def <- "/Img%s%s bind def\n",p,theproc
 ATTENTION! This operator outputs several lines to standart output stream!
 Returns an empty string.*/
static char *mac_PDiaImages(char *arg)
{
int i;
char *p,*theproc;
char *fmt;
HASH_TABLE img=NULL;

  if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);
   fmt=new_str(get_string(arg));/*Now in fmt  we have a format string*/
   outPSinit(NULL);
   img=create_hash_table(17,str_hash,str_cmp,c_destructor);
   for(i=-ext_lines; i<=int_lines; i++){
      if(i==0)continue;
      p=new_str(  (id[id[id[dtable[0][i]-1].link[1]].link[0]].id)+1  );
      if( install(p,p,img) == 0  ){/*This particle is absent*/
         sprintf(arg,"_PSImage%s",p);/*now arg = "_PSImage<part>"*/
         if(   (theproc=lookup(arg,export_table))==NULL  ){/*No such image!?*/
            char *ptr=new_str(arg);/*detach*/
            sprintf(arg,"{(%s) %s}",p,SHOW);
            theproc=new_str(arg);
            install(ptr,theproc,export_table);
            /*Note, allocated memory is absorbed by the table! No free!*/
         }/*if(   (theproc=lookup(arg,export_table))==NULL  )*/
         sprintf(arg,fmt,p,theproc);
         l_outimage(arg);
      }/*if( install(p,p,img) != 0  )*/

      /*Now try special pseudoparticles:*/
      p=s_let("_PSImage",arg);
      while(*p!='\0')p++;/*Now p points to the end of arg*/
      l_l2mark(i, p);/*Now arg contains "_PSImage<lrem>" where <lrem> is a line remark*/

      if(   (theproc=lookup(arg,export_table))!=NULL  ){/*Such a routine is defined*/
         p=new_str(p);/*Detach from arg*/
         sprintf(arg,fmt,p,theproc);
         free(p);
         l_outimage(arg);
      }
   }/*for(i=1; i<=int_lines; i++)*/
   hash_table_done(img);
   outPSend();
   free(fmt);
   *arg='\0';
   return arg;
}/*mac_PDiaImages*/

/* \_PAllImages(formatstring) - used to build procedures of a particle drawing in
 .ps files It loops at all internal id entries and looks for a particle.
 If found, the proper procedure is outputted.
 Builds:
 /Img<part><proc> bind def <- "/Img%s%s bind def\n",p,theproc
 ATTENTION! This operator outputs several lines to standart output stream!
 Returns an empty string.*/
static char *mac_PAllImages(char *arg)
{
word i;
char *p,*theproc;
char *fmt;

   fmt=new_str(get_string(arg));/*Now in fmt  we have a format string*/
   outPSinit(NULL);

   for(i=0; i<top_id; i++){

      if(
           ( *(id[i].id)!='\1' )/*Not a particle*/
           ||(id[i].kind >'\1')/* Not a begin of propagator*/
        )
           continue;
      p=  (id[i].id)+1 ;
      sprintf(arg,"_PSImage%s",p);/*now arg = "_PSImage<part>"*/
      if(   (theproc=lookup(arg,export_table))==NULL  ){/*No such image!?*/
         char *ptr=new_str(arg);/*detach*/
         sprintf(arg,"{(%s) %s}",p,SHOW);
         theproc=new_str(arg);
         install(ptr,theproc,export_table);
         /*Note, allocated memory is absorbed by the table! No free!*/
      }/*if(   (theproc=lookup(arg,export_table))==NULL  )*/
      sprintf(arg,fmt,p,theproc);
      l_outimage(arg);
   }/*for(i=1; i<=int_lines; i++)*/
   for(i=0; i<l_specialImagesTop; i++){
      sprintf(arg,"_PSImage%s",l_specialImages[i]);
      if(   (theproc=lookup(arg,export_table))!=NULL  ){
         sprintf(arg,fmt,l_specialImages[i],theproc);
         l_outimage(arg);
      }/*if(   (theproc=lookup(arg,export_table))!=NULL  )*/
   }/*for(i=0; i<l_specialImagesTop; i++)*/
   outPSend();
   free(fmt);
   *arg='\0';
   return arg;
}/*mac_PAllImages*/

/*\_PImage(l,id):(l - line number, id - a driver id): returns the image of a
   particle at line 'l' and sets  several variables according to <id> :
   (__PS<id>_<part>,)
   (__PS<id>_u_p,#) - total number of particles used in imagining
   (__PS<id>_u_#p,<part>) - #-order number corresponding to <part>
*/
#define IEID "ie"
static char *mac_PImage(char *arg)/*Wrapper to \begipropagator(l)*/
{
   char id[MAX_OUTPUT_LEN];
   int n,l;
   char *buf,*ptr,*quasipart;
   int len=0;

   if(islast)
      halt(CANNOTCREATEOUTPUT,NULL);

   s_letn(get_string(arg),id,MAX_OUTPUT_LEN-10);
   /*id now is the driver id*/

   if(((l=get_num("_PImage",arg))>int_lines)||
                                          (l<-ext_lines)||(l==0))
       halt(INVALIDNUMBER,arg);

   quasipart=new_str(l_l2mark(l, arg));
   sprintf(arg,"_PSImage%s",quasipart);

   if(  lookup(arg,export_table)==NULL  )
      free_mem(&quasipart);
   /*Now if quasipart!=NULL we deals with a special case*/

   s_let(l_beginpropagator(l),arg);

   /*arg now contains the particle id*/
   if(quasipart!=NULL){
      int lp=s_len(id)+s_len(quasipart);
      len=s_len(id)+s_len(arg);
      if(lp>len)len=lp;
   }else
      len=s_len(id)+s_len(arg);
   if(!s_scmp(id,IEID)){/*IEID - id for the infoEPS driver*/
      /*export some varables (namely, shift from the line)*/
      /*ATTENTION! This (+9) depends on the "_PSIEPS%s_s" template:*/
      buf=get_mem(len+9,sizeof(char));
      sprintf(buf,"_PSIEPS%s_s",arg);
      if(  (ptr=lookup(buf,export_table))==NULL){/* No such a variable*/
          if(  (ptr=lookup("_PSIEPS_s",export_table))==NULL)
             /*Default shift multiplier is undefined!*/
             install(new_str("_PSIEPS_s"),ptr=new_str("10"),export_table);
          install(new_str(buf),new_str(ptr),export_table);
      }/*if(  (ptr=lookup(buf,export_table))==NULL)*/
   }else
      /*ATTENTION! This (+6) depends on the "__PS%s_%s" template:*/
      buf=get_mem(len+6,sizeof(char));

   if(quasipart!=NULL){
     s_let(quasipart,arg);
     free_mem(&quasipart);
   }/*if(quasipart!=NULL)*/

   sprintf(buf,"__PS%s_%s",id,arg);
   if(! install(buf,new_str(""),export_table) ){/*New entry*/
      /*ATTENTION! This (+12) depends on the "__PS%s_u_%dp" template:*/
      buf=get_mem(len+12,sizeof(char));
      sprintf(buf,"__PS%s_u_p",id);
      if(  (ptr=lookup(buf,export_table))==NULL)/**first occurence*/
         n=1;
      else{/*if(  (ptr=lookup(buf,export_table))==NULL)*/
         if(  (n=atoi(ptr))<=0  )
            halt(INVALIDCONTENT_PS,buf,ptr);
         n++;
      }/*if(  (ptr=lookup(buf,export_table))==NULL) ... else*/
      ptr=new_long2str(n);
      install(new_str(buf),ptr,export_table);
      sprintf(buf,"__PS%s_u_%dp",id,n);
      install(buf,new_str(arg),export_table);
   }/*if(! install(buf,new_str(""),export_table) )*/

   return arg;
}/*mac_PImage*/
/*
   (__PS<id>_<part>,)
   (__PS<id>_u_p,#)  - total number of particles used in imagining
   (__PS<id>_u_#p,<part>)- #-order number corresponding to <part>
   (_PSf_<part>_#) - fonts installed \parseParticleImage or \_PSabbrev (tml)
   (_PSF_<id>_#) - main fonts
*/
/*
\_PDocFonts(id) returns "", but works as a procedure. Writes to the output
   %%DocumentFonts and %%DocumentNeededFonts and clears allocated variables.
*/

static char *mac_PDocFonts(char *arg)
{
   char id[MAX_OUTPUT_LEN];
   char *ptr, *particle,**output=NULL;
   int i,n,j,thetop=0,wasMainFint=0;
   HASH_TABLE readyFonts=NULL;

      s_letn(get_string(arg),id,MAX_OUTPUT_LEN-10);
      /*Now id is the id of a driver*/

      sprintf(arg,"__PS%s_u_p",id);
      if(  (ptr=lookup(arg,export_table))==NULL)
         /*No images - no fonts. But there are STANDART fonts!*/
         n=0;
      else{
         if(   (n=atoi(ptr))<=0   )
            return s_let("",arg);/*Number of particles is not a number :)?*/

            uninstall(arg,export_table);/*Clear the variable*/
      }/*if(  (ptr=lookup(arg,export_table))==NULL) ... else*/
      /*Now n is the number total of particles used in imagining */

      readyFonts=create_hash_table(17,str_hash,str_cmp,c_destructor);

      /* Try to get main fonts, _PSF_<id>_#:*/
      j=1;
      do{
         sprintf(arg,"_PSF_%s_%d",id,j);
         if(  (ptr=lookup(arg,export_table))!=NULL){
            ptr=new_str(ptr);/*Detach*/
            if(thetop==0 ){
               thetop=1;
               install(ptr,ptr,readyFonts);
               *(output=get_mem(2,sizeof(char*)))=new_str(ptr);
            }else{
                 if( install(ptr,ptr,readyFonts) == 0  ){
                    if(  (output=realloc(output,(thetop+1)*sizeof(char*)))==NULL  )
                       halt(NOTMEMORY,NULL);
                    output[thetop]=new_str(" ");
                    output[thetop]=s_inc(output[thetop],ptr);
                    thetop++;
                 }/*if( install(ptr,ptr,readyFonts) == 0  )*/
            }/*if(thetop==0 )...else*/
            uninstall(arg,export_table);
         }/*if(  (ptr=lookup(arg,export_table))!=NULL)*/
         j++;
      }while(ptr!=NULL);

      wasMainFint=thetop;
      /*Now if there was no main fonts, then thetop = wasMainFint = 0, we need not
        fonts at all. Bu even in this case we have to clean up  variables!:*/

      /*Loop at all particles:*/
      for(i=1;i<=n;i++){
         /*Get a particle "number i":*/
         sprintf(arg,"__PS%s_u_%dp",id,i);
         if(  (ptr=lookup(arg,export_table))==NULL)/*?*/
            continue;
         particle=new_str(ptr);
         /*and remove the variables __PS<id>_u_#p and __PS<id>_<part>:*/
         uninstall(arg,export_table);
         sprintf(arg,"__PS%s_%s",id,particle);
         uninstall(arg,export_table);

         j=1;
         do{
           sprintf(arg,"_PSf_%s_%d",particle,j);
           if(  (ptr=lookup(arg,export_table))!=NULL){
              ptr=new_str(ptr);/*Detach*/
              if(thetop==0 ){
                 thetop=1;
                 install(ptr,ptr,readyFonts);
                 sprintf(arg," %s",ptr);
                 *(output=get_mem(1,sizeof(char*)))=new_str(arg);
              }else{
                 if( install(ptr,ptr,readyFonts) == 0  ){
                    if(  (output=realloc(output,(thetop+1)*sizeof(char*)))==NULL  )
                       halt(NOTMEMORY,NULL);

                    if(thetop>2)
                       sprintf(arg,"\n%%%%+ %s",ptr);
                    else
                       sprintf(arg," %s",ptr);

                    output[thetop++]=new_str(arg);
                 }/*if( install(ptr,ptr,readyFonts) == 0  )*/
              }/*if(thetop==0 ) ... else*/
           }/*if(  (ptr=lookup(arg,export_table))!=NULL)*/
           j++;
           /*Do NOT remove _PSf_<part>_#, they are shared!*/
         }while(ptr!=NULL);
      }/*for(i=1;i<=n;i++)*/
      /*And now output the built strings:*/
      if(!wasMainFint){
         /*EPS or IEPS driver wit 0 size, no fonts needed! But they may come from images*/
         if(thetop){/*Just clear*/
            for(i=0; i<thetop; i++)free(output[i]);
            free(output);
         }/*if(thetop)*/
      }else if(thetop){
         outputstr("%%DocumentFonts: ");
         for(i=0; i<thetop; i++)
            outputstr(output[i]);
         outputstr("\n%%DocumentNeededFonts: ");
         for(i=0; i<thetop; i++){
            outputstr(output[i]);
            free(output[i]);
         }/*for(i=0; i<thetop; i++)*/
         free(output);
      }/*else if(thetop)*/
      free(particle);
      hash_table_done(readyFonts);
      return s_let("",arg);
}/*mac_PDocFonts*/

static char *macskip(char *arg)/*"skip()" -- omit current diagram*/
{
  set_bit(&mode,bitSKIP);
  runexitcode=0;
  isrunexit=1;
  *arg=0;
  return(arg);
}/*macskip*/
static char *macantiparticle(char *arg)/*"antiparticle(part)" --
returns antiparticle of particle "part", if it exists, or "none".*/
{
word *n;
   if((n=lookup(s_cat(arg,"\1",get_string(arg)),main_id_table))==NULL)
     s_let("none",arg);
  else
     s_let(id[ id[*n].link[0] ].id+1,arg);
  return(arg);
}/*macantiparticle*/
#ifdef CHECKMEM
static char *macfreemem(char *arg)/*"freemem()" -- returns coreleft.*/
{
  return(long2str(arg,coreleft()));
}/*macfreemem*/
#endif

/* Returns 0, if direction of propagator coinside with direction of
line l. If it opposite, the function returns -1. If tadpole, return 0.
The line directions assumed  to be in nusr topology (See "The
substituions map" in "variables.h").
For external lines returns 0 if it is the begin of a propagator, or
-1, if it is the end of a propagator.
*/
static int pdir(int l)
{
 word i;
 int dir;
    if( (l>int_lines)||(l==0) ) return(0);/*Error? -- do nothing...*/
    if(l<0){/* External lines: invert line if it is ANTI-particle:*/
      for(i=1;!(i>vcount);i++)
         if (dtable[i][l]!=0)break;/* found*/
      if(   (dir=id[dtable[i][l]-1].kind)==0)
         return(0);/* Particle and antiparticle are identical*/
      return(1 - dir);/* 0, if dir == 1, and -1, if dir == 2*/
/*
      if (ext_particles[ext2my[-l]].is_ingoing==0)
         return( (dir==1)?(-1):0);
      else
         return( (dir==1)?0:(-1));
*/
    }/*if(l<0)*/
    /* Here l>0, the line is internal.*/
    for(i=1;!(i>vcount);i++)
       if(dtable[i][l]){
          if(*(id[ i=(dtable[i][l]-1) ].id)==2)/*Tadpole*/
             return(0);
          dir=topologies[cn_topol].l_dir[lt_subst[l]];
          /*See "The substituions map" in "variables.h"*/
          if(id[i].kind==0)/* Particle and antiparticle are identical*/
             dir *= l_dir[l];/* Use directions as it is in nusr topology*/
          if(id[i].kind==1)
              dir = -dir;
          if(dir == 1) dir = 0;
          return(dir);
       }
    return(0);
}/*pdir*/

/* For internal lines: returns 0, if direction of the line  in red topology coinsides with  direction
 * of the same line in nusr topology, otherwise, -1.
 * For external: returns 0, if ingoing, otherwise, -1.
 */
static int ndir(int l)/*Expects l is red*/
{

    if(l<0){ /*external*/
       if (ext_particles[-l-1].is_ingoing==0)/* Outgoing*/
          return(-1);/*Need revert*/
       else
          return(0);/*As is*/
    }/*if(l<0)*/
    if( l_dir[l]>0 )
          return(0);
    return(-1);
}/*ndir*/

static double hyp( double x1, double y1, double x2, double y2)
{
double res = sqrt( (x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
  if( res< SMALL_RAD)res=SMALL_RAD;
  return( res );
}/*hyp*/

static void t360( double *a,double *b)
{
   *a -= (floor( *a / 360.0) * 360.0);
   *b -= (floor( *b / 360.0) * 360.0);
}/*t360*/

static double getCircleLen(double r,double a_s, double a_e)
{
double tmp;
   if(a_s>a_e){
      if(r<0.0)tmp=a_s-a_e;else tmp=360.0 +a_e-a_s;
   }else{
      if(r>0.0)tmp=a_e-a_s;else tmp=360.0 +a_s-a_e;
   }
   tmp=fabs(r*tmp*2.0*3.1416/360.0);
   if (tmp<1.0) tmp=fabs(r)*2.0*3.1416;/* Full arc */
   return tmp;
}/*getCircleLen*/

static char *macgetPSarclabel(char *arg)/*"getPSarclabel(l)" :
                                      returns X Y coordinate
                                      of the label of line l.*/
{
 int i,int_lines,ext_lines;/*This hides global variables!*/

 char l;
 aTOPOL *theTopology=topologies+cn_topol;

  if( (theTopology==NULL)||
      (theTopology->orig==NULL)||
      (theTopology->coordinates_ok == 0)   )
     return (s_let("",arg));

  if(islast)
    halt(CANNOTCREATEOUTPUT,NULL);
  int_lines=theTopology->orig->i_n;
  ext_lines=theTopology->orig->e_n;

  if(((l=get_num("getPSarclabel",arg))>int_lines)||(l<-ext_lines)||(l==0))
    halt(INVALIDNUMBER,arg);

  if(l<0){
     i=(ext_lines+l)*2;
     sprintf(arg," %.2f %.2f ",
        theTopology->ell[i],
        theTopology->ell[i+1]
     );
  }else{
    i=(lt_subst[il_subst[l]]-1)*2;
     sprintf(arg," %.2f %.2f ",
        theTopology->ill[i],
        theTopology->ill[i+1]
     );
  }

  return(arg);
}/*macgetPSarclabel*/

static char *macgetPSvertexlabel(char *arg)/*"getPSvertexlabel(v)" :
                                      returns X Y coordinate
                                      of the label of vertex v.*/
{
 int i,
     vcount,ext_lines;/*This hides global variables!*/
 char l;
 aTOPOL *theTopology=topologies+cn_topol;

  if( (theTopology==NULL)||
      (theTopology->orig==NULL)||
      (theTopology->coordinates_ok == 0)   )
     return (s_let("",arg));

  if(islast)
    halt(CANNOTCREATEOUTPUT,NULL);
  ext_lines=theTopology->orig->e_n;
  vcount =theTopology->max_vertex;
  if(((l=get_num("getPSvertexlabel",arg))>vcount)||(l<-ext_lines)||(l==0))
    halt(INVALIDNUMBER,arg);

  if(l<0){
     i=(ext_lines+l)*2;
     sprintf(arg," %.2f %.2f ",
        theTopology->evl[i],
        theTopology->evl[i+1]
     );
  }else{
    i=iv_subst[l];
    for(l=1;!(l>vcount);l++)
             if((theTopology->v_subst)[l]==i) break;
    i=(l-1)*2;
     sprintf(arg," %.2f %.2f ",
        theTopology->ivl[i],
        theTopology->ivl[i+1]
     );
  }

  return(arg);
}/*macgetPSvertexlabel*/
static char *macgetPSvertex(char *arg)/*"getPSvertex(v)" :
                                         returns X Y coordinate
                                         of the vertex v.*/
{
 int i,
     vcount,ext_lines;/*This hides global variables!*/
 char l;
 aTOPOL *theTopology=topologies+cn_topol;

  if( (theTopology==NULL)||
      (theTopology->orig==NULL)||
      (theTopology->coordinates_ok == 0)   )
     return (s_let("",arg));

  if(islast)
    halt(CANNOTCREATEOUTPUT,NULL);
  ext_lines=theTopology->orig->e_n;
  vcount =theTopology->max_vertex;
  if(((l=get_num("getPSvertexlabel",arg))>vcount)||(l<-ext_lines)||(l==0))
    halt(INVALIDNUMBER,arg);

  if(l<0){
     i=(ext_lines+l)*2;
     sprintf(arg," %.2f %.2f ",
        theTopology->ev[i],
        theTopology->ev[i+1]
     );
  }else{
    i=iv_subst[l];
    for(l=1;!(l>vcount);l++)
             if((theTopology->v_subst)[l]==i) break;
    i=(l-1)*2;

     sprintf(arg," %.2f %.2f ",
        theTopology->iv[i],
        theTopology->iv[i+1]
     );
  }

  return(arg);
}/*macgetPSvertex*/

/* "getPSarc(l), getPSInfoArc(l) -- returns the PostScript coordinates
 * of the line number l in the form: " ox oy r as ae l "
 * Here ox,oy -- the center coordinates, |r| -- radius, as,ae --
 * initial and target angles. If r<0.0, the angle is clockwise.
 * If r = 0.0, the line is stright, from (ox,oy) to (as,ae).
 * l is the length of the arc.
 * ATTENTION! The system coordinates is that Y is directed UPwards!
 * getPSarc(l) assumes the line directions along directed propagators
 * while getPSInfoArc(l) -- along nusr tolology (see the substitutions
 * map in the file variabls.h). Implementation: both these functions
 * call doGetPSarc(...) with the proper parameters.
 */
typedef int PDIR(int);
static char *doGetPSarc(PDIR *pdr, char *arg, char *name)
{
int l,
    int_lines, ext_lines;/*This hides global variables!*/
 aTOPOL *theTopology=topologies+cn_topol;

  if( (theTopology==NULL)||
      (theTopology->orig==NULL)||
      (theTopology->coordinates_ok == 0)   )
     return (s_let("",arg));

  if(islast)
    halt(CANNOTCREATEOUTPUT,NULL);
  int_lines=theTopology->orig->i_n;
  ext_lines=theTopology->orig->e_n;
  if(((l=get_num(name,arg))>int_lines)||(l<-ext_lines)||(l==0))
    halt(INVALIDNUMBER,arg);

  if(l<0){
     int fromV, toV,usr;
     double fromX,fromY, toX, toY;

     usr=-l;/* External line are STRIGHT*/

     /*if (ext_particles[ext2my[usr]].is_ingoing==0){*//* Outgoing*/
     if(pdr(l)){/* revert*/
        /* from internal to external vertices:*/
        toV=ext_lines+theTopology->orig->e_line[usr].from;/*external*/
        fromV=theTopology->orig->e_line[usr].to-1;/*internal*/
        toX=theTopology->ev[toV*2];
        toY=theTopology->ev[toV*2+1];
        fromX=theTopology->iv[fromV*2];
        fromY=theTopology->iv[fromV*2+1];
     }else{/* As is */
        fromV=ext_lines+theTopology->orig->e_line[usr].from;/*external*/
        toV=theTopology->orig->e_line[usr].to-1;/*internal*/
        toX=theTopology->iv[toV*2];
        toY=theTopology->iv[toV*2+1];
        fromX=theTopology->ev[fromV*2];
        fromY=theTopology->ev[fromV*2+1];
     }
     sprintf(arg," %.2f %.2f %.2f %.2f %.2f %.2f ",
           fromX, fromY, 0.0, toX, toY, hyp(fromX,fromY,toX,toY)
        );
  }else{/* l>0*/
     int red, usr,theCircle;/*If 0 -- stright, +1 -- the circle*/
     double r,ox,oy,as,ae, len;
     /* Determine red number:*/
     red=il_subst[l];
     /* Determine usr number:*/
     usr=lt_subst[red]-1;
     r=theTopology->rad[usr];
     if(pdr(red)){/* From and To must be interchanged*/
        if (r<MIN_POSSIBLE_COORDINATE){/* Straight line*/
           r=0.0;
           theCircle=0;
           ox=theTopology->start_angle[usr];
           oy=theTopology->end_angle[usr];
           as=theTopology->ox[usr];
           ae=theTopology->oy[usr];
           len=hyp(ox,oy,as,ae);
        }else{
           theCircle=1;
           r=-r;/* Change the direction of the angle*/
           /* Exchange start and target angles:*/
           as=theTopology->end_angle[usr];
           ae=theTopology->start_angle[usr];
           ox=theTopology->ox[usr];
           oy=theTopology->oy[usr];
        }
     }else{/* As is*/
        ae=theTopology->end_angle[usr];
        as=theTopology->start_angle[usr];
        ox=theTopology->ox[usr];
        oy=theTopology->oy[usr];
        if (r<MIN_POSSIBLE_COORDINATE){/* Straight line*/
           theCircle=0;
           r=0.0;
           len=hyp(ox,oy,as,ae);
        }else{
           theCircle=1;
        }
     }/*if(pdr(red))*/
     if(theCircle){
        double tmp;
        t360(&as,&ae);/* Reduce angles to the mod 360 */
        tmp=ae-as;
        if(fabs(tmp)<1.0){/*Tadpole!*/
           if(r<0.0){
              as=ae;
              ae=as+1.0;
           }else{
              ae=as+359.0;
           }
        }/*f(fabs(tmp)<1.0)*/
        len=getCircleLen(r,as,ae);
     }/*if(theCircle)*/
     sprintf(arg," %.2f %.2f %.2f %.2f %.2f %.2f ",
                 ox,oy,r,as,ae,len);
  }/* else */

  return(arg);
}/*doGetPSarc*/

static char *macgetPSarc(char *arg)
{
  return(doGetPSarc(&pdir,arg,"getPSarc"));
}/*macgetPSarc*/

static char *macgetPSInfoArc(char *arg)
{
  return(doGetPSarc(&ndir,arg,"getPSInfoArc"));
}/*macgetPSInfoArc*/

double sXmax,sXmin,sYmax,sYmin;
static int topologyNumberForMinMaxXYisCalculated=-1;
/* Defines maximal/minimal X,Y coordinates in the current topoogy
 * and stores them into the static variables sXmax,sXmin,sYmax,sYmin:*/
static void getMinMax(void)
{
double r;
int i,j,k,
    int_lines, ext_lines, vcount;/*This hides global variables!*/
aTOPOL *theTopology=topologies+cn_topol;

  int_lines=(theTopology->orig->i_n)*2;
  ext_lines=(theTopology->orig->e_n)*2;
  vcount =(theTopology->max_vertex)*2;

  /* Determine max and min coordinates:*/

  /* There MUST be at least one internal vertex!:*/
  sXmax=sXmin=*(theTopology->iv);
  sYmax=sYmin=(theTopology->iv)[1];

  for(i=0,j=1;i<int_lines; i+=2,j++){
     if((theTopology->il)[i]>sXmax)sXmax=(theTopology->il)[i];
     if((theTopology->ill)[i]>sXmax)sXmax=(theTopology->ill)[i];
     if((theTopology->il)[i]<sXmin)sXmin=(theTopology->il)[i];
     if((theTopology->ill)[i]<sXmin)sXmin=(theTopology->ill)[i];
     if((theTopology->il)[i+1]>sYmax)sYmax=(theTopology->il)[i+1];
     if((theTopology->ill)[i+1]>sYmax)sYmax=(theTopology->ill)[i+1];
     if((theTopology->il)[i+1]<sYmin)sYmin=(theTopology->il)[i+1];
     if((theTopology->ill)[i+1]<sYmin)sYmin=(theTopology->ill)[i+1];
     /* Tadpoles occupy the square:*/
     if(
         (
          (theTopology->orig)->i_line[j].from ==
          (theTopology->orig)->i_line[j].to
         )&&
         (
            /* Check is tadpol degenerated, i.e all points are too close:*/
            (r=(theTopology->rad)[k=j-1])>MIN_POSSIBLE_COORDINATE
         )
     ){/*Tadpole!*/
        if(   (r=(theTopology->rad)[k])<0.0  )r=-r;
        if( (theTopology->ox)[k] + r > sXmax )sXmax=(theTopology->ox)[k] + r;
        if( (theTopology->ox)[k] - r < sXmin )sXmin=(theTopology->ox)[k] - r;
        if( (theTopology->oy)[k] + r > sYmax )sYmax=(theTopology->oy)[k] + r;
        if( (theTopology->oy)[k] - r < sYmin )sYmin=(theTopology->oy)[k] - r;
      }

  }/*for(i=0,j=1;i<int_lines; i+=2,j++)*/

  for(i=0;i<ext_lines; i+=2){
     if((theTopology->ev)[i]>sXmax)sXmax=(theTopology->ev)[i];
     if((theTopology->evl)[i]>sXmax)sXmax=(theTopology->evl)[i];
     if((theTopology->el)[i]>sXmax)sXmax=(theTopology->el)[i];
     if((theTopology->ell)[i]>sXmax)sXmax=(theTopology->ell)[i];

     if((theTopology->ev)[i]<sXmin)sXmin=(theTopology->ev)[i];
     if((theTopology->evl)[i]<sXmin)sXmin=(theTopology->evl)[i];
     if((theTopology->el)[i]<sXmin)sXmin=(theTopology->el)[i];
     if((theTopology->ell)[i]<sXmin)sXmin=(theTopology->ell)[i];

     if((theTopology->ev)[i+1]>sYmax)sYmax=(theTopology->ev)[i+1];
     if((theTopology->evl)[i+1]>sYmax)sYmax=(theTopology->evl)[i+1];
     if((theTopology->el)[i+1]>sYmax)sYmax=(theTopology->el)[i+1];
     if((theTopology->ell)[i+1]>sYmax)sYmax=(theTopology->ell)[i+1];

     if((theTopology->ev)[i+1]<sYmin)sYmin=(theTopology->ev)[i+1];
     if((theTopology->evl)[i+1]<sYmin)sYmin=(theTopology->evl)[i+1];
     if((theTopology->el)[i+1]<sYmin)sYmin=(theTopology->el)[i+1];
     if((theTopology->ell)[i+1]<sYmin)sYmin=(theTopology->ell)[i+1];
  }
  for(i=0;i<vcount; i+=2){
     if((theTopology->iv)[i]>sXmax)sXmax=(theTopology->iv)[i];
     if((theTopology->ivl)[i]>sXmax)sXmax=(theTopology->ivl)[i];

     if((theTopology->iv)[i]<sXmin)sXmin=(theTopology->iv)[i];
     if((theTopology->ivl)[i]<sXmin)sXmin=(theTopology->ivl)[i];

     if((theTopology->iv)[i+1]>sYmax)sYmax=(theTopology->iv)[i+1];
     if((theTopology->ivl)[i+1]>sYmax)sYmax=(theTopology->ivl)[i+1];

     if((theTopology->iv)[i+1]<sYmin)sYmin=(theTopology->iv)[i+1];
     if((theTopology->ivl)[i+1]<sYmin)sYmin=(theTopology->ivl)[i+1];
  }
  topologyNumberForMinMaxXYisCalculated=cn_topol;
}/*getMinMax*/

/* This is \adjust(fontsize, x,y)
 * It is used to adjust the size of diagram so that it fits to
 * 0 0 x y bounding box, together with up line of "fontsize" height.
 * ATTENTION! It leaves 2 parameters in the postscript stack
 * which can be used for typing the text:*/
static char *macadjust(char *arg)
{
double Xmax,Xmin,Ymax,Ymin,dx,dy,x,y,fs;
aTOPOL *theTopology=topologies+cn_topol;
  if( (theTopology==NULL)||
      (theTopology->orig==NULL)||
      (theTopology->coordinates_ok == 0)   )
     return (s_let("",arg));
  if(islast)
    halt(CANNOTCREATEOUTPUT,NULL);
  /* Determine max and min coordinates:*/
  if(topologyNumberForMinMaxXYisCalculated != cn_topol)
     getMinMax();
  /* Read parameters:*/
  if(
    (sscanf(get_string(arg),"%lf",&y)!=1)||
    (sscanf(get_string(arg),"%lf",&x)!=1)||
    (sscanf(get_string(arg),"%lf",&fs)!=1)  ){
        char buf[MAX_STR_LEN];
        sprintf(buf,NUMERROR,"adjust",arg);
        halt(buf,NULL);
  }

  Xmax=sXmax+fs;Ymax=sYmax+fs; Xmin=sXmin-fs;
  Ymin=sYmin-(fs*2);/* One is conventional, and one for inscribtion*/
  dx=Xmax-Xmin; dy=Ymax-Ymin;/* >=0.0 by construction!*/
  if(dx<SMALL_RAD)dx=SMALL_RAD;if(dy<SMALL_RAD)dy=SMALL_RAD;
  /* Choose the smallest coefficient:*/
  {double kx=x/dx,ky=y/dy; if(kx<ky) dx=kx; else dx=ky;}
  sprintf(arg,"%.2f %.2f scale %.2f %.2f translate %.2f %.2f",
             dx,dx,-Xmin,y/dx-Ymax,Xmin+SMALL_RAD, Ymin+fs);
  return(arg);
}/*macadjust*/

/* \getXfromY(fontsize, dY) -- returns the proper dX value,
 * for EPS BoundingBox:*/
static char *macgetXfromY(char *arg)
{
double Xmax,Xmin,Ymax,Ymin,dx,dy,y,fs;
aTOPOL *theTopology=topologies+cn_topol;
  if( (theTopology==NULL)||
      (theTopology->orig==NULL)||
      (theTopology->coordinates_ok == 0)   )
     return (s_let("",arg));
  if(islast)
    halt(CANNOTCREATEOUTPUT,NULL);

  /* Determine max and min coordinates:*/
  if(topologyNumberForMinMaxXYisCalculated != cn_topol)
     getMinMax();
  if(
    (sscanf(get_string(arg),"%lf",&y)!=1)||
    (sscanf(get_string(arg),"%lf",&fs)!=1)  ){
        char buf[MAX_STR_LEN];
        sprintf(buf,NUMERROR,"getXfromY",arg);
        halt(buf,NULL);
  }

  Xmax=sXmax+fs;Ymax=sYmax+fs; Xmin=sXmin-fs;
  Ymin=sYmin-(fs*2);/* One is conventional, and one for inscribtion*/
  dx=Xmax-Xmin; dy=Ymax-Ymin;/* >=0.0 by construction!*/
  if(dy<0.1)dy=1;
  y *= (dx/dy);
  sprintf(arg,"%.2f",y);
  return(arg);
}/*macgetXfromY*/

static char *macinitps(char *arg)/*initps(header,target)-- returns "" if  ok,
                                   or "none", if there are NO topologies with
                                   distributed coordinates.*/
{
char target[MAX_STR_LEN];
  get_string(target);
  if(initPS(get_string(arg), target))
     s_let("none",arg);
  else
     *arg='\0';
  return(arg);
}/*macinitps*/
static char *maccoordinatesok(char *arg)/*coordinatesok() -- returns "false,
                                           if there are NO topologies with
                                           distributed coordinates. Othervise,
                                           returns "true" */
{
if (atleast_one_topology_has_coordinates) s_let("true",arg);
else s_let("false",arg);
return(arg);
}/*maccoordinatesok*/
static char *macthiscoordinatesok(char *arg)/*thiscoordinatesok() --
                                            returns "false,
                                           if current topology does not have coordinates.
                                           Othervise, returns "true" */
{
  if(g_nocurrenttopology)
    halt(CANNOTCREATEOUTPUT,NULL);
if (topologies[cn_topol].coordinates_ok) s_let("true",arg);
else s_let("false",arg);
return(arg);
}/*macthiscoordinatesok*/

static char *macisinteger(char *arg)
{
char *b;
   strtol(get_string(arg),&b,10);
   if( ( (*arg)=='\0')||( (*b) !='\0' )  )
      return(s_let("false",arg));
    return(s_let("true",arg));
}/*macisinteger*/

static char *macisfloat(char *arg)
{
char *b;
   strtod(get_string(arg),&b);
   if( (*b) !='\0' )
      return(s_let("false",arg));
    return(s_let("true",arg));
}/*macisfloat*/

/*\maxindexgroup() -- returns the total number of indices groups*/
static char *macmaxindexgroup(char *arg)
{
   return(long2str(arg,numberOfIndicesGroups));
}/*macmaxindexgroup*/

/* \getfirstusedindex(group) and  \getnextusedindex(group):
  These function are used to obtain ONLY indices allocated for current diagram
  from the indices table.
  \getfirstusedindex(group) initialzes iterator and returns the first index
  belonging to the specified group.
  Then each successive call of \getnextusedindex(group) will returns the
  next index from the indices table until all indices are exhausted.
  Then the latter function will returning "" (empty string) until
  new \getfirstusedindex(group):*/

static struct indices_struct **usedindex_it=NULL;/*current*/

static char *macgetfirstusedindex(char *arg)
{
long group;
   if( ( (group=get_num("getfirstusedindex",arg))<1)||
         (!(--group<numberOfIndicesGroups)))
              halt(INVALIDNUMBER,arg);
   if(usedindex_it==NULL)
      usedindex_it=get_mem(numberOfIndicesGroups,sizeof(struct indices_struct *));
   if(
     (  (usedindex_it[group]=indices[group])==NULL  )||
     (  (usedindex_it[group]=usedindex_it[group]->next)==NULL)||
     (  iNdex[group]->id == NULL  )
   ){
      *arg='\0';
      usedindex_all=1;
   }else{
      s_let(usedindex_it[group]->id,arg);
      usedindex_all=0;
   }
   return(arg);
}/*macgetfirstusedindex*/

/* See comment to the previous function*/
static char *macgetnextusedindex(char *arg)
{
long group;
   if( ( (group=get_num("getnextusedindex",arg))<1)||
         (!(--group<numberOfIndicesGroups)))
              halt(INVALIDNUMBER,arg);
   if( (usedindex_all)||
       (usedindex_it[group] ==iNdex[group])||
       (usedindex_it[group] == NULL)||
       (  (usedindex_it[group]=usedindex_it[group]->next)==NULL )
   ){
      *arg='\0';
      usedindex_all=1;
   }else{
      s_let(usedindex_it[group]->id,arg);
   }
   return(arg);
}/*macgetnextusedindex*/

/* \getfirstindex(group) and  \getnextindex(group):
  These function are used to obtain indices from the indices table.
  ALL INDICES, not only indices allocated for current diagram!
  \getfirstindex(group) initialzes iterator and returns the first index
  belonging to the specified group.
  Then each successive call of \getnextindex(group) will returns the
  next index from the indices table until all indices are exhausted.
  Then the latter function will returning "" (empty string) until
  new \getfirstindex(group):*/

static struct indices_struct **index_it=NULL;/*current*/

static char *macgetfirstindex(char *arg)
{
long group;
   if( ( (group=get_num("getfirstindex",arg))<1)||
         (!(--group<numberOfIndicesGroups)))
              halt(INVALIDNUMBER,arg);
   if(index_it==NULL)
      index_it=get_mem(numberOfIndicesGroups,sizeof(struct indices_struct *));
   if(
     (  (index_it[group]=indices[group])==NULL  )||
     (  (index_it[group]=index_it[group]->next)==NULL)
   ){
      *arg='\0';
      index_all=1;
   }else{
      s_let(index_it[group]->id,arg);
      index_all=0;
   }
   return(arg);
}/*macgetfirstindex*/

/* See comment to the previous function*/
static char *macgetnextindex(char *arg)
{
long group;
   if( ( (group=get_num("getnextindex",arg))<1)||
         (!(--group<numberOfIndicesGroups)))
              halt(INVALIDNUMBER,arg);
   if( (index_all)||
       (index_it[group] == NULL)||
       (  (index_it[group]=index_it[group]->next)==NULL )
   ){
      *arg='\0';
      index_all=1;
   }else{
      s_let(index_it[group]->id,arg);
   }
   return(arg);
}/*macgetnextindex*/

/*\getexternalindex(group,line) -- returns the index of the group
          'group' at the external line 'line':*/
static char *macgetexternalindex(char *arg)
{
char *ch,group;
   {/*Block 1 begin*/
      long tmp;
      /* Get the line number:*/
      if(   (tmp=get_num("getexternalindex",arg)) ==0)
         halt(INVALIDNUMBER,arg);
      /* We will use the module of the line number:*/
      if (tmp<0)tmp=-tmp;
      if(tmp > ext_lines)
         halt(INVALIDNUMBER,arg);

      ch=ext_particles[tmp-1].ind;

      /* Get the group number:*/
      if( ( (tmp=get_num("getexternalindex",arg))<1)||
         (!(--tmp<numberOfIndicesGroups)))
              halt(INVALIDNUMBER,arg);
      group=tmp+'0';
   }/*Block 1 end*/
   {/*Block 2 begin*/
      int n,num;
      if( ch==NULL ){
         *arg='\0';
      }else{
         num=*ch++;/*Get the number of indices allocated in this external particle
                     and skip this number in the pointer*/
         for(n=0; n<num;n++){
            /*ch points to the group number*/
            if( *ch++ == group )/*found!*/
               /* The group number already skipped, just skip '/1':*/
               return(s_let(++ch,arg));
            while(*ch++!=0);
         }/*for(n=0; n<num;n++)*/
         /* If the control comes here, then something is wrong...*/
         *arg='\0';
      }/*if( ch==NULL )...else*/
   }/*Block 2 end*/
   return(arg);
}/*macgetexternalindex*/

/*\numberofexternalindex(group) -- returns the number of the
          indices of group 'group' allocated for external legs:*/
static char *macnumberofexternalindex(char *arg)
{
char *ch,group;
long tmp;
int n,num,nindex=0;
    /* Get the group number:*/
      if( ( (tmp=get_num("numberofexternalindex",arg))<1)||
         (!(--tmp<numberOfIndicesGroups)))
              halt(INVALIDNUMBER,arg);

      group=tmp+'0';
      for(tmp=1;!(tmp>ext_lines);tmp++){
         ch=ext_particles[tmp-1].ind;
         if( ch!=NULL ){
            num=*ch++;/*Get the number of indices allocated in this
                        external particle and skip this number in the pointer*/
            for(n=0; n<num;n++){
               /*ch points to the group number*/
               if( *ch++ == group ){/*found!*/
                    nindex++;
                    break;
               }
               while(*ch++!=0);
            }/*for(n=0; n<num;n++)*/
         }/*if( ch!=NULL )...else*/
      }/*for(tmp=1,!(tmp>ext_lines);tmp++)*/
   return(long2str(arg,nindex));
}/*macnumberofexternalindex*/

/*All tt prefixed operators relate to topology table mechanism.*/

/* \ttLoad(filename) loads a table from specified file and returns it's id >0,
    or error code <0, see tt_type.h:*/
static  char *macttLoad(char *arg)
{
   return long2str(arg,tt_loadTable(get_string(arg)));
}/*macttLoad*/

/* \ttSave(filename, table) saves a table to specified file.
  Returns number of saved topologies, or error code <0, see tt_type.h:*/
static  char *macttSave(char *arg)
{
   int tbl=(int)get_num("ttSave",arg);
   FILE *outf=NULL;

   if(*get_string(arg) == '\0' )
      outf= stdout;
   else if(  (outf=fopen(arg,"w"))==NULL  )
      return long2str(arg,TT_CANNOT_WRITE);

   tbl=tt_saveTableToFile(outf,tbl,1);/*last 1 means original non-translated*/
   if(outf != stdout)
      fclose(outf);
 return long2str(arg,tbl);
}/*macttSave*/

/* \ttDelete(table) deletes specified table, returns number of topologies,
  or error code <0 -- see tt_type.h: */
static  char *macttDelete(char *arg)
{
   long tmp=get_num("ttDelete",arg);
   tmp=tt_deleteTable((int)tmp);
   return long2str(arg,tmp);
}/*macttDelete*/

/* \ttHowmany(table) -- returns number of topologies,
  or error code <0 -- see tt_type.h: */
static  char *macttHowmany(char *arg)
{
   return long2str(arg,tt_howmany((int)get_num("ttHowmany",arg)));
}/*macttHowmany*/

/* \ttOrigTopology(table, topology) -- returns original (incoming) topology,
   or error code <0 -- see tt_type.h:*/
static  char *macttOrigtopology(char *arg)
{
int tbl;
long top;
   top=get_num("ttOrigTopology",arg);
   tbl=(int)get_num("ttOrigTopology",arg);
   return tt_origtopology(arg,tbl,top);
}/*macttOrigtopology*/

/*\ttRedTopology(table, topology) -- returns reduced topology,
   or error code <0 -- see tt_type.h:*/
static  char *macttRedtopology(char *arg)
{
int tbl;
long top;
   top=get_num("ttRedTopology",arg);
   tbl=(int)get_num("ttRedTopology",arg);
   return tt_redtopology(arg,tbl,top);
}/*macttRedtopology*/

/*\ttTopology(table, topology) -- returns transformed (nusr) topology,
   or error code <0 -- see tt_type.h:*/
static  char *macttTopology(char *arg)
{
int tbl;
long top;
   top=get_num("ttTopology",arg);
   tbl=(int)get_num("ttTopology",arg);
   return tt_transformedTopology(arg,tbl,top);
}/*macttTopology*/

/*\ttNInternal(table, topology) -- returns number of internal lines,
   or error code <0 -- see tt_type.h:*/
static  char *macttNInternal(char *arg)
{
int tbl;
long top;
   top=get_num("ttNInternal",arg);
   tbl=(int)get_num("ttNInternal",arg);
   return(long2str(arg,tt_ninternal(tbl,top)));
}/*macttNInternal*/

/*\ttExternal(table, topology) -- returns number of external lines,
   or error code <0 -- see tt_type.h:*/
static  char *macttNExternal(char *arg)
{
int tbl;
long top;
   top=get_num("ttExternal",arg);
   tbl=(int)get_num("ttExternal",arg);
   return(long2str(arg,tt_nexternal(tbl,top)));
}/*macttNExternal*/

/*\ttNVertex(table, topology) -- returns number of vertices,
   or error code <0 -- see tt_type.h:*/
static  char *macttNVertex(char *arg)
{
int tbl;
long top;
   top=get_num("ttNVertex",arg);
   tbl=(int)get_num("ttNVertex",arg);
   return(long2str(arg,tt_vertex(tbl,top)));
}/*macttNVertex*/

/*\ttNMomenta(table, topology) -- returns number of momenta set,
   or error code <0 -- see tt_type.h:*/
static  char *macttNMomenta(char *arg)
{
int tbl;
long top;
   top=get_num("ttNMomenta",arg);
   tbl=(int)get_num("ttNMomenta",arg);
   return(long2str(arg,tt_nmomenta(tbl,top)));
}/*macttNMomenta*/

/*\ttLookup(topology,table) -- expects "topology" to be a string representation of a
   topology, reduces it and looks it up in the table number "table".
   Returns its number, or <0 error code, see tt_type.h:*/
static  char *macttLookup(char *arg)
{
int tbl;
   tbl=(int)get_num("ttLookup",arg);
   long2str(arg,tt_lookupFromStr(tbl,get_string(arg)));
   return arg;
}/*macttLookup*/

/* \ttMomenta(n,table, top) -- returns string of momenta (nusr, translated) of a set
 "n" of a topology number "top" of a table number "table",or <0 error code, see tt_type.h:*/
static  char *macttMomenta(char *arg)
{
int tbl;
long top;
int n;
int err=0;
char *tmp;
   top=get_num("ttMomenta",arg);
   tbl=(int)get_num("ttNomenta",arg);
   n=(int)get_num("ttMomenta",arg);
   tmp=tt_getMomenta(0,tbl,top,n,&err);
   /*tt_getMomenta ALWAYS return allocated string, may be, ""*/
   switch(err){
      case 0:s_let(tmp,arg);break;
      default: *arg=0;break;
   }/*switch(err)*/
   free(tmp);/*This space was allocated by tt_getMomenta*/
   return(arg);
}/*macttMomenta*/

/* \ttTranslate(token,translation,table) -- set the token translation,
   returns "1" if it stores new token (i.e., if such a token is absent in the table),
   TT_NOTFOUND (see tt_type.h) if there is no such a table, "" if it replaces
   existing token:*/
static  char *macttTranslate(char *arg)
{
char translation[MAX_STR_LEN];
int tbl=(int)get_num("ttTranslate",arg);
   get_string(translation);
   long2str(arg,tt_translate(get_string(arg), translation,tbl));
   if( *arg == '0') *arg = '\0';
   return arg;
}/*macttTranslate*/

/*\ttLookupTranslation(token,table)
Looks up the token in the translation table and returns the translation,
or "" if not found:*/
static  char *macttLookupTranslation(char *arg)
{
int tbl=(int)get_num("ttLookupTranslation",arg);
char *tmp;
   if(  (tmp=tt_lookupTranslation(get_string(arg),tbl))==NULL  )
      *arg='\0';
   else
      s_letn(tmp,arg,MAX_STR_LEN);
   return arg;
}/*macttLookupTranslation*/

/*\ttTranslateTokens(text,table)
  Expects "text" is a text with tokens of kind
  blah-blah-blah ... {token} ... blah-blah-blah ... {token} ... blah-blah-blah ...
  Replaces all {token} according to the table from the table "table" and puts the result
  into buf. If the token is absent, it will be translated into empty string.
  If the table is not found, returns text "as is".
  If the built line is too long, fails. and halts the program*/
static  char *macttTranslateTokens(char *arg)
{
char buf[MAX_STR_LEN];
int tbl=(int)get_num("ttNMomenta",arg);

  switch( tt_translateTokens(get_string(buf),arg,tbl, MAX_STR_LEN) ){
     case TT_NOTFOUND:
        return s_let(buf,arg);
     case TT_TOOLONGSTRING:
        halt(TOOLONGSTRING,NULL);
  }/*switch( tt_translateTokens(get_string(buf),arg,tbl, MAX_STR_LEN) )*/
  return arg;
}/*macttTranslateTokens*/

/*\ttSetSep(newsep) - sets new separator for line/vertex substitutions,
returns empty string:*/
static  char *macttSetSep(char *arg)
{
   tt_setSep(*get_string(arg));
   *arg='\0';
   return arg;
}/*macttSetSep*/

/*\ttGetSep() returns current separator for line/vertex substitutions:*/
static  char *macttGetSep(char *arg)
{
   *arg=tt_getSep();
   arg[1]='\0';
   return arg;
}/*macttGetSep*/

/*Must be removed, I think:*/
#ifdef SKIP
static  char *macttSync(char *arg)
{
int tbl=(int)get_num("ttSync",arg);

   if(g_nocurrenttopology)
      halt(CANNOTCREATEOUTPUT,NULL);
   return arg;
}/*macttSync*/

static  char *macttSyncInternal(char *arg)
{
int tbl=(int)get_num("ttSyncInternal",arg);

   if(g_nocurrenttopology)
      halt(CANNOTCREATEOUTPUT,NULL);
   if(canonical_topology == NULL){ /*Canonical topoogy is not set! */
      /*First, try to build it using proper remarks:*/

/*   int hex2substAndDir(char *str, char *subst, char *dir,int maxN)*/

   }/*if(canonical_topology == NULL)*/
   return arg;
}/*macttSyncInternal*/
#endif

/*\ttAppendToTable(fromTable, topol, toTable) -- adds topology "topol" from a table
   "fromTable" to the end of the table "toTable". Returns the index of added topology in
   the table  "toTable", or error code <0:*/
static  char *macttAppendToTable(char *arg)
{
int fromTable,toTable,addTransTbl=0;
long topol;
   if(  (toTable=(int)get_num("ttAddToTable",arg))<0  ){
      toTable=-toTable; addTransTbl=1;
   }
   if(  (topol=get_num("ttAddToTable",arg))<0  ){
      topol=-topol; addTransTbl=1;
   }
   if(  (fromTable=(int)get_num("ttAddToTable",arg))<0  ){
      fromTable=-fromTable; addTransTbl=1;
   }
   return long2str(arg,tt_appendToTable(fromTable, topol, toTable, addTransTbl) );
}/*macttAppendToTable*/

/*\ttNewTable() creates new (empty) table and returns identifier of the created table,
  or error code < 0:*/
static  char *macttNewTable(char *arg)
{
   return long2str(arg,tt_createNewTable(g_defaultTokenDictSize,g_defaultHashTableSize));
}/*static  char *macttNewTable*/

/*\ttLinesReorder(pattern,table,top) reorders internal lines enumerating. Pattern:
  positions -- old, values -- new, separated by tt_sep, e.g.
  "2:1:-3:5:-4" means that line 1 > 2, 2 ->1, 3 just upsets,
  4->5, 5 upsets and becomes of number 4.
  Returns "" if substitution was implemented, or error code, if fails.
  In the latter case substitutions will be preserved:*/
static  char *macttLinesReorder(char *arg)
{
int tbl;
long top;
   top=get_num("ttLinesReorder",arg);
   tbl=(int)get_num("ttLinesReorder",arg);
   long2str(arg,tt_linesReorder(get_string(arg),tbl,top));
   if( *arg == '0') *arg = '\0';
   return arg;
}/*macttLinesReorder*/

/*\ttVertexReorder(pattern,table,top) reorders  vertex enumerating. Pattern:
  positions -- old, values -- new, separated by tt_sep, e.g.
  "2:1:3:5:4" means that vertices 1 > 2, 2 ->1, 3->3,
  4->5, 5 ->4.
  Returns "" if substitution was implemented, or error code, if fails.
  In the latter case the vertex substitution will be preserved:*/
static  char *macttVertexReorder(char *arg)
{
int tbl;
long top;
   top=get_num("ttVertexReorder",arg);
   tbl=(int)get_num("ttVertexReorder",arg);
   long2str(arg,tt_vertexReorder(get_string(arg),tbl,top));
   if( *arg == '0') *arg = '\0';
   return arg;
}/*macttVertexReorder*/

/* \ttGetCoords(type,table,top) returns coordiates -- double pairs x,y,x,y,....
  The value of "type"  must be one of the following identifiers:
     ev,evl,el,ell,iv,ivl,il,ill, -- for transformed cooredinates;
     oev,oevl,oel,oell,oiv,oivl,oil,oill -- for original coordinates.
  On error, returns "". If "type" is invalid, halts the system!!
 */
static  char *macttGetCoords(char *arg)
{
int tbl;
long top;
char *tmp;
int theType;
   top=get_num("ttGetCoords",arg);
   tbl=(int)get_num("ttGetCoords",arg);

   if(  *(tmp=get_string(arg)) == 'o' ){
      theType=1;
      tmp++;
   }else
      theType=0;
   /*Now tmp is one of ev,evl,el,ell,iv,ivl,il,ill;
     theType is 1 or 0 depending on was the first charachter 'o', or not
    */
   switch(*tmp){
      case 'e':
         switch (tmp[1]){
            case 'v':
               switch(tmp[2]){
                  case 'l':
                     theType+=EXT_VLBL;break;
                  case '\0':
                     theType+=EXT_VERT;break;
                  default:halt(UNEXPECTED,arg);
               }/*switch(tmp[2])*/
               break;
            case 'l':
               switch(tmp[2]){
                  case 'l':
                     theType+=EXT_LLBL;break;
                  case '\0':
                     theType+=EXT_LINE;break;
                  default:halt(UNEXPECTED,arg);
               }/*switch(tmp[2])*/
               break;
               default:halt(UNEXPECTED,arg);
         }/*switch (tmp[1])*/
         break;
      case 'i':
         switch (tmp[1]){
            case 'v':
               switch(tmp[2]){
                  case 'l':
                     theType+=INT_VLBL;break;
                  case '\0':
                     theType+=INT_VERT;break;
                  default:halt(UNEXPECTED,arg);
               }/*switch(tmp[2])*/
               break;
            case 'l':
               switch(tmp[2]){
                  case 'l':
                     theType+=INT_LLBL;break;
                  case '\0':
                     theType+=INT_LINE;break;
                  default:halt(UNEXPECTED,arg);
               }/*switch(tmp[2])*/
               break;
            default:halt(UNEXPECTED,arg);
         }/*switch (tmp[1])*/
         break;
      default:halt(UNEXPECTED,arg);
   }/*switch(*tmp)*/
   if( tt_getCoords(arg, MAX_STR_LEN, tbl, top, theType) < 0 )
      *arg='\0';
   return arg;
}/*macttGetCoords*/

/*\ttHowmanyRemarks(table, topology) -- returns total number of remarks
in topology, or error code <0 -- see tt_type.h:*/
static  char *macttHowmanyRemarks(char *arg)
{
int tbl;
long top;
   top=get_num("ttHowmanyRemarks",arg);
   tbl=(int)get_num("ttHowmanyRemarks",arg);
   return long2str(arg,tt_howmanyRemarks(tbl,top));
}/*macttHowmanyRemarks*/

/*\ttGetRemark(name,table, topology) -- gets remark or "" if fails:*/
static  char *macttGetRemark(char *arg)
{
int tbl;
long top;
   top=get_num("ttGetRemark",arg);
   tbl=(int)get_num("ttGetRemark",arg);
   return tt_getRemark(arg,tbl,top,get_string(arg));
}/*macttGetRemark*/

/*\ttSetRemark(name, value, table, topology) --Sets remark. Returns "",
or error code <0 -- see tt_type.h:*/
static  char *macttSetRemark(char *arg)
{
int tbl;
long top;
char *val;
   top=get_num("ttSetRemark",arg);
   tbl=(int)get_num("ttSetRemark",arg);
   val=new_str(get_string(arg));/*Get "value"*/
   if(  (top=tt_setRemark(tbl,top,get_string(arg),val))<0  ){/*Error? Free allocated value!*/
      free(val);
      return long2str(arg,top);
   }/*if(  (top=tt_setRemark(tbl,top,name,val))<0  )*/
   *arg='\0';
   return arg;
}/*macttSetRemark*/

/*\ttKillRemark(name,table, topology) -- removes remark. Returns "", if ok,
   or or error code <0 -- see tt_type.h:*/
static  char *macttKillRemark(char *arg)
{
int tbl;
long top;
     top=get_num("ttKillRemark",arg);
     tbl=(int)get_num("ttKillRemark",arg);
     if(  (top=tt_killRemark(tbl,top,get_string(arg)))<0  )
        return long2str(arg,top);
     *arg='\0';
     return arg;
}/*macttKillRemark*/

/*\ttValueOfRemark(num,table,topology) -- returns the value of the remark
   number "num" from (table, topology) or empty line, "num" is counted from 1:*/
static char *macttValueOfRemark(char *arg)
{
int tbl;
long top;
word num;
     top=get_num("ttValueOfRemark",arg);
     tbl=(int)get_num("ttValueOfRemark",arg);
     num=(word)get_num("ttValueOfRemark",arg);
     return tt_valueOfRemark(arg,tbl,top,num-1);
}/*macttValueOfRemark*/

/*\ttNameOfRemark(num,table, topology) -- returns the name of the remark
   number "num" from (table, topology) or empty line, "num" is counted from 1:*/
static char *macttNameOfRemark(char *arg)
{
int tbl;
long top;
word num;
     top=get_num("ttNameOfRemark",arg);
     tbl=(int)get_num("ttNameOfRemark",arg);
     num=(word)get_num("ttNameOfRemark",arg);
     return tt_nameOfRemark(arg,tbl,top,num-1);
}/*macttnameOfRemark*/

/* \nexp() returns number of global variables defined at the moment:*/
static char *macnexp(char *arg)
{
   if (export_table==NULL)return s_let("0",arg);
   return long2str(arg,export_table->n);
}/*macnexp*/

/* \nvar() returns number of local variables defined at the moment:*/
static char *macnvar(char *arg)
{
  if (variables_table==NULL)return s_let("0",arg);
  return long2str(arg,variables_table->n);
}/*macnvar*/

/* \sizeExp() returns the size of table of global variables. If it is less then
the number of defined variables, then an algorithm is inefficient:*/
static char *macsizeExp(char *arg)
{
   if (export_table==NULL)return s_let("0",arg);
   return long2str(arg,export_table->tablesize);
}/*macsizeExp*/

/* \sizeVar() returns the size of table of local variables. If it is less then
the number of defined variables, then an algorithm is inefficient:*/
static char *macsizeVar(char *arg)
{
  if (variables_table==NULL)return s_let("0",arg);
  return long2str(arg,variables_table->tablesize);
}/*macsizeVar*/

/*actual routine for macresizeExp and macresizeVar:*/
static char *l_resizeTable(char *arg,char *name,HASH_TABLE *tbl)
{
long n;
word i;
   /*Determine maximal possible value of n:*/
   n=-1;
   i=n;
   /*Now binary i is "11111....", so it is of maximal possible value.*/

   n=get_num(name,arg);/*Get an argument*/

   if (n==0) return l_freeTable(arg,tbl);
   else if (n>0)
      n=nextPrime(n);/*By default, ceil n by prime*/
   else/*n<0*/
      n=-n;/*If negative, use it as is*/

   if (n>i)n=i;/*Paranoia*/

   i=n;/*cast the argument*/

   if ((*tbl)==NULL)
      (*tbl)=create_hash_table(i,str_hash,str_cmp,c_destructor);
   else
      resize_hash_table((*tbl),i);

   *arg='\0';
   return arg;
}/*l_resizeTable*/

/*\resizeExp(n) resizes the table of global variables.
The new size 'n' must be a prime number not less then the number of defined variables.
If it is less then the number of defined variables, then an algorithm is inefficient
If n==0 then the operator clears the table. If n>0, it will be ceiled by a prime.
If it <0, then the new size -n will be used as is. Returns empty string:*/
static char *macresizeExp(char *arg)
{
   return l_resizeTable(arg,"resizeExp",&export_table);
}/*macresizeExp*/

/*\resizeVar(n) resizes the table of local variables.
The new size 'n' must be a prime number not less then the number of defined variables.
If it is less then the number of defined variables, then an algorithm is inefficient
If n==0 then the operator clears the table. If n>0, it will be ceiled by a prime.
If it <0, then the new size -n will be used as is. Returns empty string:*/
static char *macresizeVar(char *arg)
{
   return l_resizeTable(arg,"resizeVar",&variables_table);
}/*macresizeVar*/

static char *macttErrorCodes(char *arg)
{
long i;
  get_string(arg);
  if(sscanf(arg,"%ld",&i)!=1)/*Not an integer*/
     return arg;/*Just repeat an argument*/

  return tt_error_txt(arg,(int)i);
}/*macttErrorCodes*/

/*\saveIntTbl(filename) saves wrk table (if present) into a named file.
  Returns number of saved topologies, or error code <0*/
static char *macsaveIntTbl(char *arg)
{
   /* save_wrk_table(char *) see utils.c*/
  return long2str(arg,save_wrk_table(get_string(arg)));
}/*macsaveIntTbl*/

/*\loadIntTbl(filename) loads wrk table from a named file.
  Returns number of loaded topologies, or error code <0*/
static char *macloadIntTbl(char *arg)
{

  return long2str(arg,load_wrk_table(get_string(arg)));
}/*macloadIntTbl*/

/*'n' is the table usr vertex; cn_momentaset overrides global cn_momentaset!
  Always allocates returned value !*/
static int *l_get_sum_incoming_momenta(char *vtsubst,
                                       aTOPOL *topology,
                                       tt_singletopol_type *top,
                                       int n,
                                       int cn_momentaset)
{
int i, *ret=get_mem(1,sizeof(int)),nel=(topology->topology)->e_n;

   n=
    vtsubst[/*vtsubst[can]=red, i.e. this line converts topology 'can' to 'red'*/
      /*convert to table 'red', i.e., can, see the substitutions tables in variabls.h:*/
      (top->v_usr2red)[n]
    ];
    /*Now n is the topology 'red'.*/

    if(n>nel)return ret;/*Ok, get_mem(0,...)=0;*/
    for(i=1; i<=nel;i++)if(topology->topology->e_line[i].to == n){
       int *momentum;
       if(  (topology->ext_momenta!=NULL)&&(topology->ext_momenta[cn_momentaset]!=NULL) )
          momentum=topology->ext_momenta[cn_momentaset][i];
       else
          momentum=ext_particles[i-1].momentum;
       /*external line 'i' connects with vertex 'n'*/
       ret=int_inc(ret,momentum,ext_particles[i-1].is_ingoing);
    }/*for(i=1, i<=nel;i++)if(topology->topology->e_line[i].to == n)*/

    return ret;
}/*l_get_sum_incoming_momenta */

static void l_storemomenta(int *momentum)
{
int l;
   if(g_wrk_vectors_table == NULL)
      g_wrk_vectors_table=create_hash_table(vectors_hash_size,
                                             str_hash,str_cmp,c_destructor);
      for (l=1;l<=(*momentum);l++){/*Loop on all momenta*/
         char *vec=new_str(vec_group[abs(momentum[l])].vec);
         install(vec,vec,g_wrk_vectors_table);
      }/*for (l=1;l<=(*momentum);l++)*/
}/*l_storemomenta*/

static void l_storetxtmomenta(char *momentum)
{

   if(g_wrk_vectors_table == NULL)
      g_wrk_vectors_table=create_hash_table(vectors_hash_size,
                                             str_hash,str_cmp,c_destructor);
   momentum=new_str(momentum);/*detach*/
   install(momentum,momentum,g_wrk_vectors_table);
}/*l_storemomenta*/

/* make sustitution to convert table usr into (internal) topology usr:*/
static char *l_mksubst(
         char *bufret,/*Return buffer*/
         char *bufsubst,/*Wrk buffer*/

         char *usr2red,/*table usr2red[usr]=red*/
         char *dirUsr2Red,/*table dir[usr] usr<->red*/

         char *can2red,/*topology can2red[can]=red*/
         char *dirCan2Red,/*topolody dirCan2Red[red] red<->can*/

         char *subst, /* topology subst[usr]=red*/
         char *ldir,/* topology ldir[usr]  usr<->red*/
         int n)/*number of entries*/
{
   char *tmp=bufret;
   int i,res,dir=1;
      invertsubstitution(bufsubst, subst);
      for(i=1; i<=n; i++){
         /*i is table usr*/
         res=usr2red[i];/*Now 'res' is table red => topology can*/

         if(can2red!=NULL)
            res=can2red[res];/*now 'res' is topology red*/

         if(dirCan2Red!=NULL)/*topolody dirCan2Red[red] red<->can*/
            dir=dirCan2Red[res];

         res=bufsubst[res];/*Now 'res' is topology usr*/

         if(ldir!=NULL)
            dir*=ldir[res];

         if(dirUsr2Red!=NULL)
            dir*=dirUsr2Red[i];

         sprintf(tmp,"%d:",res*dir);
         while(*(++tmp) !='\0');
      }
      *(tmp-1)='\0';
   return bufret;
}/*l_mksubst*/

/*reorders lines/vertices of a table to be as in topology, nusr(tbl)=usr(top)*/
static char *l_usrTbl2usrTopol(
         char *bufret,/*Return buffer*/
         char *bufsubst,/*Wrk buffer*/

         char *tblUsr2red,/*table usr2red[usr]=red*/
         char *topUsr2red,/*topology usr2red[usr]=red*/
         int n)/*number of entries*/
{
   char *tmp=bufret;
   int i,res;
      invertsubstitution(bufsubst, topUsr2red);/*now bufsubst is top red2usr*/
      for(i=1; i<=n; i++){
         res=tblUsr2red[i];/*Now 'res' is table red */
         res=bufsubst[res];/*Now 'res' is topology usr*/
         sprintf(tmp,"%d:",res);
         while(*(++tmp) !='\0');
      }
      *(tmp-1)='\0';
   return bufret;
}/*l_usrTbl2usrTopol*/

/*Makes nusr such that nusr == red
 * BUT with the same directions as in usr:
 */
static char *l_mkFullSubst(
         char *bufret,/*Return buffer*/
         char *usr2red,/*table usr2red[usr]=red*/
/* Not used anymore         char *dirUsr2Red,*//*table dir[usr] usr<->red*/
         int n)/*number of entries*/
{
   char *tmp=bufret;
   int i,res;
      for(i=1; i<=n; i++){
         res=usr2red[i];
/*         if(dirUsr2Red!=NULL) res*=dirUsr2Red[i]; */
         sprintf(tmp,"%d:",res);
         while(*(++tmp) !='\0');
      }/*for(i=1; i<=n; i++)*/
      *(tmp-1)='\0';
   return bufret;
}/*l_mkFullSubst*/

#ifdef SKIP
static char *l_mkOldNewSubst(
         char *bufret,/*Return buffer*/
         char *newUsr2red,/*new table usr2red[usr]=red*/
         char *oldRed2usr,/*old table red2usr[red]=usr*/
         int n)/*number of entries*/
{
   char *tmp=bufret;
   int i;
      for(i=1; i<=n; i++){
         sprintf(tmp,"%d:",oldRed2usr[newUsr2red[i]]);
         while(*(++tmp) !='\0');
      }/*for(i=1; i<=n; i++)*/
      *(tmp-1)='\0';
   return bufret;
}/*l_mkOldNewSubst*/
#endif

static int l_outCoord(FILE *outf,char *name, double *arr, int n, char *ep)
{
int j;

         if(   fputs(name,outf)<0 )return -1;
         if(arr!=NULL) for(j=0;j<n;j++){
            if(  (j>0)&&(   fputc(',',outf)<=0 )  )return -1;
            if(   fprintf(outf,"%.1f",arr[j])<=0   ) return -1;
         }/*if(arr!=NULL) for(j=0;j<n;j++)*/
         if(   fputs(ep,outf)<0 )return -1;
         return 0;
}/*l_outCoord*/

static int l_process_vec(int n, char *vec)
{
   install(vec,vec,g_wrk_vectors_table);
   /*It is possibe here, if the table size is not enough, expand it*/
   return 0;
}/*l_process_vec*/

static void l_storemomenta_fromstr(char *str)
{
   if(g_wrk_vectors_table == NULL)
      g_wrk_vectors_table=create_hash_table(vectors_hash_size,
                                             str_hash,str_cmp,c_destructor);

   parse_tokens_id(g_regchars,digits,str,&l_process_vec);/*see utils.c*/
}/*l_storemomenta_fromstr*/

/* Aliges lines directions of topol->orig to be coinciding with a table topology.
 * If it (topol->orig) is absent,
 * creates topology co-inciding with topol->topology (i.e., it is
 * "red"), but with directions co-inciding with the corresponding lines
 * in the table, and assignes it to topol->orig. Also re-set,
 * v_subst and l_dir.
 *   Note, this is safety w.r.t. ltsubst,vtsubst since they are w.r.t.
 * red, but not usr.
 */
static void l_createPseudoTopology(aTOPOL *topol,tt_singletopol_type *top,char *buf)
{
int i,l,r,u;
char *can2red,/*topology can2red[can]=red*/
     *dirCan2Red;/*topolody dirCan2Red[red] red<->can:*/
   /*Each topology dealt with internal momenta table must contain lines substitution and
     directions mapping it to the canonical topology:*/
   if(
       (  ( can2red=getTopolRem("ltsubst",topol->top_remarks,topol->remarks) )==NULL  )||
       (  ( dirCan2Red=getTopolRem("ldir",topol->top_remarks,topol->remarks) )==NULL  )
     )
      return;/*Can't map topology!*/

   if(topol->orig==NULL)
      topol->orig=newTopol( topol->topology );

   /*Independently on previous status, now topol->orig contains the proper topology
     with proper l_dir,l_subst and v_subst*/

   /* See The substitutions map in file variabls.h:
      topologies[i].l_subst[Vusr]=Vred (lines)
      topologies[i].l_dir[Vusr]:  usr<->red

      See tt_type.h, assuming table red is can:
      top->l_dirUsr2Red[usr]:  table usr<->can
      top->l_red2usr[can]=usr
    */

   l=topol->topology->i_n+1;

   invertsubstitution(buf, topol->l_subst);/*now buf is topology red2usr*/

   for(i=1;i<l;i++){
      /*Assume i is can*/
      r=can2red[i]; /*topology red*/
      u=buf[r]; /*topology usr*/
      /* topol->l_dir[u] topology usr<->red*/
      /* dirCan2Red[r] topology can<->red*/

      if(  (
           dirCan2Red[r]*topol->l_dir[u] /*topology usr <->can */
           *
           top->l_dirUsr2Red[
                             top->l_red2usr[i] /*table usr*/
                            ] /*table usr<->can*/
           )<0
        ){/* Opposite signes!*/
           (topol->l_dir[u])*=-1;/*change sign of the direction*/

           /*Upset usr topology line physically:*/
           r=topol->orig->i_line[u].from;/*Use r as a temporar variable*/
           topol->orig->i_line[u].from=topol->orig->i_line[u].to;
           topol->orig->i_line[u].to=r;
        }/*if*/
   }/*for(i=1;i<l;i++)*/
}/*l_createPseudoTopology*/

/*
  \loadLoopMarks(filename)
  Returns number of successfully processed topologies.
  Every loaded topology must contain the remark "t" which is equal to the number of
  the topology which it represents, otherwise it will be ignored

  */
static char *macloadLoopMarks(char *arg)
{
long n,tn=0;
int i,k,id,rid;
   get_string(arg);

   if(top_topol==0)return s_let("0",arg);

   if(   (id=tt_loadTable(arg))<=0  )
      return long2str(arg,id);
   if(  (n=tt_howmany(id)) <=0  )
         return long2str(arg,n);

   /*Here the table is loaded with at least one topology*/

   /*Add it into the full tables pool:*/
   if(!(g_tt_top_full<g_tt_max_top_full))/*possible expansion*/
      if((g_tt_full=(int *)realloc(g_tt_full,
         (g_tt_max_top_full+=DELTA_TT_TBL)*sizeof(int*)))==NULL)
            halt(NOTMEMORY,NULL);

   g_tt_full[g_tt_top_full++]=id;

   rid=id-1;/*the REAL index*/
   /*Loop at all topologies in the table:*/
   for(i=0; i<n; i++){
      tt_singletopol_type *top=( (tt_table[rid])->topols )[i];
      aTOPOL *topol;
      char *tmp;
      long ntop;
      int nil;
      int j;
      if(
         (  (tmp=getTopolRem("t",top->top_remarks,top->remarks)) == NULL  ) ||
                                                           /*Number of top is absent*/
         (  sscanf(tmp,"%ld",&ntop)!=1  )||/*Not a number*/
         (ntop<0) || (ntop >= top_topol)/*Range check*/
        )/*Something is wrong?*/
         continue;
      topol=topologies+ntop;

      /* Here we assume that topologies are co-incide, but to avoid possible chash
         we check the number of internal lines - th only meninngfull value in
         such a sense:*/
      if(   (nil=top->nil) != (topol->topology)->i_n   )
         continue;

      /* The shape can be edited, so we create the links to the shape table:*/
      /*Link to table:*/
      setTopolRem(new_str("Ctbl"), new_long2str(id),NULL,
                                  &(topol->top_remarks),
                                  &(topol->remarks),
                                NULL);
      /*Link to topology in the table:*/
      setTopolRem(new_str("Ctop"), new_long2str(i+1),NULL,
                                  &(topol->top_remarks),
                                  &(topol->remarks),
                                NULL);
      if(nil == 0)/*Pure vertex, nothing to do*/
          continue;

      /*Now set up "orig" as it would come from the table:*/

      if(topol->orig == NULL)
         topol->orig=get_mem(1, sizeof(tTOPOL));
      topol->orig->e_n=topol->topology->e_n;
      topol->orig->i_n=topol->topology->i_n;
      for(k=topol->topology->e_n;k>0;k--){
         (topol->orig->e_line[k]).from=(topol->topology->e_line[k]).from;
         (topol->orig->e_line[k]).to=(topol->topology->e_line[k]).to;
      }/*for(k=topol->topology->e_n;k>0;k--)*/
      for(k=1,j=0; k<=nil;k++,j++){
         (topol->orig->i_line[k]).from=(top->usrTopol[j]).from;
         (topol->orig->i_line[k]).to=(top->usrTopol[j]).to;
         (topol->l_subst)[k]=(top->l_usr2red)[k];
         (topol->l_dir)[k]=(top->l_dirUsr2Red)[k];
      }/*for(k=0; k<nil;k++)*/
      for(k=top->nv; k>0; k--)
         (topol->v_subst)[k]=(top->v_usr2red)[k];

      /*Now try to build the substitution using tmp as a buffer*/
      /* We assume that momenta come from the topologyeditor as follows:
         only ONE momenta set, everithing is "" except the momenta which must be
         assigned by the loop momenta. These momenta are marked by "1", "2", etc.
       */
      /*Distriute it :*/
      *(tmp=get_mem(nil+2,sizeof(char)))=-1;

      if(top->nmomenta>0){/* If not, there are no momenta marked. Use NULL*/
        char **m=top->momenta[0];
        int i;
        if(m!=NULL){
           for(i=0; i<nil;i++){/*assume i is table usr*/

              if(
                   (m[i]==NULL)||
                   ( *(m[i])=='\0' )||
                   (  (k=atoi(m[i]))==0  )||
                   (k<1)||(k>nil)
                )
                 tmp[top->l_usr2red[i+1]]='\0';
              else
                 tmp[top->l_usr2red[i+1]]=k;
           }/*for(i=0; i<nil;i++)*/
           /*Now tmp is suitable to be used as mloopMarks in automatic_momenta_group*/
           /*Note, tmp[red]*/
        }/*if(m!=NULL)*/
      }/*if(top->nmomenta>0)*/
      /*Distribute momenta for current topology*/
      if(automatic_momenta_group(tmp,ntop)==1){/*read_inp.c*/
         /*Now momenta are distributed automatically:*/
            set_bit(&(topol->label),2);
            /*Bit 0: 0, if topology not occures, or 1;
              bit 1: 1, if topology was produced from generic, or 0.
              bit 2: 1, automatic momenta is implemented, or 0
             */
         tn++;/*Account this case*/

         for(k=1; k<=nil;k++)(tmp[k])++;
         /*Now tmp is ASCII-Z string suitable to be stored as a remark*/
         setTopolRem(new_str("MLoop"), tmp,NULL,
                                           &(topol->top_remarks),
                                           &(topol->remarks),
                                           NULL);
         /*tmp now is swallowed by remarks, no free!*/
      }else /*if(automatic_momenta_group(tmp,ntop)==1)*/
         free_mem(&tmp);/*Just free it!*/
   }/*for(i=0; i<n; i++)*/
   return long2str(arg,tn);
}/*macloadLoopMarks*/

void l_specRemErr(int s,int **momentum, char **txtmom, char *mem, int id,long ind)
{
   free_mem(momentum);
   free_mem(txtmom);

   switch(s){
      case -3:/*error digit expected*/
         halt(SPECIALREMARKS,mem,id,ind,DIGITEXPECTED);
      case -4:/*error toolarge number*/
         halt(SPECIALREMARKS,mem,id,ind,TOOLARGENUM);
      case -5:/*error parsing*/
         halt(SPECIALREMARKS,mem,id,ind,UNEXPECTEDSYM);
      case -6:/*error empty line*/
         halt(SPECIALREMARKS,mem,id,ind,EMPTYREMARK);
      default:
         halt(SPECIALREMARKS,mem,id,ind,INTERNALERROR);
   }/*switch(s)*/
}/*l_specRemErr*/

/*Returns:
   -1: Bridge, no loop momenta
    1: Chord
    0: bare loop
       Note, in the latter case we perform additional checkup - if isBare == 0,
       we assume that it is not a bare loop, but something is present but contracted.
       So we put zero in *rest_m and return 1.
 */
static int l_splitMomentum(int *in_m, int **loop_m,int **rest_m,int isBare)
{
int i,j,l=0,r=0;
   /*Allocate maximal possible length here:*/
   *loop_m=get_mem( (*in_m)+1+(*g_zeromomentum),sizeof(int) );
   *rest_m=get_mem( (*in_m)+1+(*g_zeromomentum),sizeof(int) );
   /*Note, the zero momentum may be very long!*/
   for(i=(*in_m); i>0; i--){
      j=abs(in_m[i]);
      if(   (j<=g_max_loop_group )&&( g_loopmomenta_r[j]!=0 )   )
         (*loop_m)[++l]=in_m[i];
      else
         (*rest_m)[++r]=in_m[i];
   }/*for(i=*in_m; i>0; i--)*/

   **loop_m=l;
   **rest_m=r;

   if(  (l>0)&&(r>0) )return 1;
   if(l==0) return -1;
   /*Here l>0, r==0 - bare loop momentum. BUT! May be, everything is just contracted?
     Check it:*/
   if(!isBare){/*No, there IS something!*/
      /*So, for the "rest" we set zero momentum here*/
      for(i=*g_zeromomentum; i>0;i--)
         (*rest_m)[i]=g_zeromomentum[i];
      **rest_m=*g_zeromomentum;
      return 1;
   }/*if(!isBare)*/
   return 0;
}/*l_splitMomentum*/

/*Just auxiliary routine used in l_saveTopologies:*/
static int l_mk_autosubst(char *subst,int ndef, int k, char *arg, char *buf,
                   char *group, int *rest, char **rem, int sign )
{
   int *ptr=NULL;
   sprintf(arg, "%s%d",subst,k);
   if(lookup(arg,vectors_table)==NULL)
            halt(UNDEFINEDID,arg);
   if(vec_group_table==NULL)
              vec_group_table=
                 create_hash_table(
                        vec_group_hash_size,str_hash,
                        str_cmp,int_destructor);

   if((ptr=lookup(arg, vec_group_table))==NULL){
      /*Such cell absent, create new one:*/
      MOMENT *cell;
         /*Allocate new cell in vec_group:*/
         if(!(top_vec_group<max_top_vec_group))
            if(
               (vec_group=realloc(vec_group,
               (max_top_vec_group+=MAX_VEC_GROUP)*
                   sizeof(MOMENT)))==NULL
              )halt(NOTMEMORY,NULL);
         cell=vec_group+top_vec_group;
         /*Fill up allocated cell:*/
         cell->vec=new_str(arg);
         cell->text=new_str(arg);
         /*Store info in vec_group_table:*/
         *(ptr=get_mem(1,sizeof(int)))=top_vec_group++;
         install(cell->text,ptr,vec_group_table);
   }/*if((ptr=lookup(arg, vec_group_table))==NULL)*/
   /*Now *ptr is the index of proper vec_group*/

   /*Store momentum 'b#' for saving*/
   l_storetxtmomenta(arg);

   /*Q#=b#:*/
   sprintf(buf,"\n   %s%d = %s:",MOMENTATABLE_LEFT,ndef,arg);
   *rem=s_inc(*rem,buf);
   /*P#=sum_ext:*/
   sprintf(arg,"\n   %s%d = %s:",MOMENTATABLE_RIGHT,ndef,
     build_momentum(group,rest,buf, sign) );
   *rem=s_inc(*rem,arg);
   /*Note, we return bridge of chord, while in rem the rest is stored!*/
   return (*ptr)*sign;
}/*l_mk_autosubst*/

/*Makes topol->orig == topol->topology and flips lines clached with top:*/
static void l_redirectAsInFull(aTOPOL *topol,tt_singletopol_type *top)
{
int i;
   /*Note, topol->orig must be NULL here! Indeed, the function is invoked only if
     momenta come from full topol table, that means therer are no momenta=>no orig*/
   topol->orig=newTopol( topol->topology );

   for(i=topol->orig->i_n; i>0; i--){/*'i' is topol usr==red*/
      if(  top->l_dirUsr2Red[top->l_red2usr[i]] <0 ){
         /*Lines are clashed*/
         int tmp=topol->orig->i_line[i].from;
         topol->orig->i_line[i].from=topol->orig->i_line[i].to;
         topol->orig->i_line[i].to=tmp;
      }/*if(  (   (topol->l_dir[i]) * (top->l_dirUsr2Red[top->l_red2usr[red]])   )<0 )*/

   }/*for(i=1; !(i>topol->i_n); i++)*/
}/*l_redirectAsInFull*/

/*This function tries to construct a unique topology name using 'thename' come
from the table, see comment in the beginning of l_saveTopologies:*/
static char *l_mkUniqName(char ForI,/*'f' or 'i'*/
                          char *idTbl,/*A number - the table index*/
                          char *thename,/*name come from the table, may be NULL*/
                          char *buf,/*just a buffer*/
                          HASH_TABLE ht/*hash table to store unique names*/
                         )
{
char *ptr=buf;
int i;
   if(thename==NULL)/*This may be!*/
      *buf='\0';
   else if(*thename != '\0' ){/*Try the orig. name first:*/
      char *tmp=new_str(s_let(thename,buf));
      if(!install(tmp,tmp,ht)) return buf;/*Ok, it is unique.*/
      /*This name already occurs, append it by '_':*/
      for(;*ptr!='\0';ptr++);
      *ptr++='_';*ptr='\0';
   }/*else if(*thename != '\0' )*/
   /*Here ptr points to the end of original name, appended by '_', if it is not empty*/
   /*Append it by ForI and idTbl:*/
   sprintf(ptr,"%c%s",ForI,idTbl);
   while(*ptr!='\0')ptr++;
   /*Now ptr points to the end of the name.*/
   /*Append the name by 1,2,3... while it is not unique:*/
   for(i=1;lookup(buf,ht)!=NULL;i++)
      long2str(ptr,i);
   /*Put constructed name to the tabe:*/
   ptr=new_str(buf);
   install(ptr,ptr,ht);
   return buf;
}/*l_mkUniqName*/

static char *l_saveTopologies(char *arg, int exportForMarks)
{
/* Names for topologies from table are built in such a way:
     First, the name from the table is tried. If it exists, then:
        If it is unique, it will used.
        Else it will be appended by _f<ntbl> for full table or _i<ntbl>
        for internal table.
     If the original name does not exists, or empty, the name
     f<ntbl> for full table or i<ntbl> for internal table will be created.

     The constructed name will be appended by 1,2,3... until it will not
     be unique.

     Here <ntbl> is the number of the topology table.

     NOTE: names are bound to MOMENTA tables!
 */
word i;
int nil,nel,nv;
int j,k;
FILE *outf;
char *rem;
char *fnam;
char *buf;
int *emom;
long ret=0;
HASH_TABLE uniqueNames=NULL;
;

   g_wasAbstractMomentum=0;

   get_string(arg);

   if(top_topol==0)return s_let("0",arg);

   if(   (outf=fopen(arg,"w"))==NULL   )
         halt(CANNOTWRITETOFILE,arg);

   emom=get_mem(1,sizeof(int));
   fnam=new_str(arg);

   for(k=0,i=1; i<= ext_lines;i++){
      /*Bulid sum of incoming momenta:*/
      emom=int_inc(emom,
                        ext_particles[i-1].momentum,
                        ext_particles[i-1].is_ingoing
                  );
      if (ext_particles[i-1].is_ingoing==0)/* Outgoing*/
         k++;
   }/*for(k=0,i=1; i<= ext_lines;i++)*/
   if(   fprintf(outf,"outlines = %d\n",k)<=0 )goto fail_write;

   buf=get_mem(MAX_STR_LEN,sizeof(char));

   uniqueNames=create_hash_table(nextPrime(top_topol),str_hash,str_cmp,c_destructor);

   for(i=0; i<top_topol; i++){
      aTOPOL *topol=topologies+i;
      unsigned int top_remarks=topol->top_remarks;
      REMARK *remarks=topol->remarks;
      tt_singletopol_type *top;
      int wasReduced=0;
      int vertexmarksready=0,linemarksready=0;
      int ComeMomentaFromFullId=0;
      int ComeMomentaFromFullInd=0;
      int selfMomenta=0;
      int id;
      long ind;
      pTOPOL topology;/*Auxiliary variable*/
      if(    !is_bit_set( &(topol->label),0 )    ) /*Newer occur!*/
          continue;
      if(
           (exportForMarks)&&
          (! is_bit_set( &(topol->label),2 ) )/* Not automatic*/
        )
          continue;

      ret++;/*Counter processed topologies*/

      nil=(topol->topology)->i_n;
      nel=(topol->topology)->e_n;
      nv=topol->max_vertex;
      if( fputs("topology ",outf)<0 )goto fail_write;
      /*Build a name:*/
      if(  (rem=getTopolRem("Mtbl",top_remarks,remarks)) != NULL  ){
         /*Momenta came from the table*/
         char ForI;
         char *ntop=getTopolRem("Mtop",top_remarks,remarks);
         if(ntop==NULL)
            halt(INTERNALERROR,NULL);

         id=atoi(rem);
         if(id==0)/*g_tt_wrk is always stored as 0*/
            id=g_tt_wrk;
         else
            ComeMomentaFromFullId=id;

         ind=atol(ntop);

         if(ComeMomentaFromFullId)
            ComeMomentaFromFullInd=ind;
         /*!!! Check the range!:*/
         checkTT(tt_chktop(&id, &ind),FAILSAVETOP);
         /*Now id = id-1; ind = ind-1*/
         top=( (tt_table[id])->topols )[ind];
         /*Push back id and ind:*/
         id++;ind++;

         g_wasMomentaTable=1;

         if((topol->id)!=NULL){
           for(j=0; j<topologies[i].n_id;j++)
              free((topol->id)[j]);
           free(topol->id);
         }/*if((topol->id)!=NULL)*/

         if( !ComeMomentaFromFullId  ){/*wrk table, internal topology*/
            /* Now we create PSEUDO topology and assign it to topol->orig.
             * Note, here we have momenta come from a table, so topol->orig must be
             * undefined.
             *   The created topology co-incides with topol->topology (i.e., it is
             *   "red"), but with directions co-inciding with the corresponding lines
             *   in the table!
             *
             * We need this feature to keep momenta "as is" in the table. The problem
             * is that sometimes users want to trust even to the momenta signs.
             */
             l_createPseudoTopology(topol,top,buf);
             ForI='i';
         }else/*Full topology table*/
             ForI='f';

         l_mkUniqName( ForI, rem, top->name, buf,uniqueNames);

         topol->id=get_mem(1,sizeof(char*));
         *(topol->id)=new_str(buf);
         topol->n_id=1;
      }/*if(  (rem=getTopolRem("Mtbl",top_remarks,remarks)) != NULL  )*/
      k=topol->n_id-1;
      for(j=0; j<topol->n_id;j++){
         if(   fputs( (topol->id)[j],outf )<0   )goto fail_write;
         if(j<k)
               if(   fputc(',',outf)<=0 )goto fail_write;
      }/*for(j=0; j<topol->n_id;j++)*/

      if(   fputs("=\n",outf)<0 )goto fail_write;

      if(ComeMomentaFromFullId){/*full topology table*/
         /* Create substitution to transorm table usr to red,
          * BUT with the same directions as in usr:*/

         l_mkFullSubst(arg,top->l_usr2red,/*top->l_dirUsr2Red,*/nil);
         checkTT(tt_linesReorder(arg,  id,  ind),FAILSAVETOP);
         l_mkFullSubst(arg,top->v_usr2red,/*NULL,*/nv);

         checkTT(tt_vertexReorder(arg,  id,  ind),FAILSAVETOP);

         l_redirectAsInFull(topol,top);

         topology=topol->orig;

      }else{

         if(topol->orig!=NULL)
            topology=topol->orig;
         else
            topology=topol->topology;

      }/*if(ComeMomentaFromFullId) ... else ... */

      if(   fputs(top2str(topology, arg),outf)<0   )goto fail_write;
      /*Finish topology:*/
      if(   fputs(":\n",outf)<0 )goto fail_write;

      if(  rem == NULL  ){ /*Self-contained or auto momenta*/
         char group[NUM_STR_LEN];
         int s,*momentum;
         int ndef=0;
         int n=topol->n_momenta-1;
         char *mloops=NULL;
         group[0]=3;group[1]=group[2]=1;group[3]='\n';group[4]=0;
         selfMomenta=1;

         if(topol->n_momenta>0){/*Self-contained or
                                  auto momenta mechanism is implemented*/

            int autosubst=(exportForMarks==0)&&
                          (g_autosubst)&&
                          (topol->n_momenta==1)&&
                          is_bit_set( &(topol->label),2 ) ;
            ndef=0;
            if(autosubst)
               mloops=getTopolRem("MLoop",top_remarks,remarks);

            for(j=0;j<topol->n_momenta;j++){
               if(   fputs("   [",outf)<0 )goto fail_write;
               for(s=1,k=-nel;k<=nil; k++){
                  if(k<0){/*external line*/
                     if(
                        (topol->ext_momenta!=NULL) &&
                        (topol->ext_momenta[j]!=NULL)
                     )
                        momentum=topol->ext_momenta[j][-k];
                     else/* Common external momenta*/
                        momentum=ext_particles[-k-1].momentum;
                  }else if(k>0){
                     s=topol->l_dir[k];/*The sign*/
                     if(autosubst){
                        int **currentmomentum=&(topol->momenta[j][(topol->l_subst)[k]]);
                        int *loop_m=NULL;
                        int *rest_m=NULL;
                        /*If NO loop marks, assume that pure loop is always bare!*/
                        int isBare=1;
                        if(mloops!=NULL)
                           isBare=mloops[(topol->l_subst)[k]]-1;
                        switch(l_splitMomentum(*currentmomentum, &loop_m,&rest_m,isBare)){
                           case -1:/*Bridge, no loop momenta*/
                              if(g_bridge_subst!=NULL){
                                 int res=l_mk_autosubst(g_bridge_subst,
                                       ++ndef,k,arg,buf,group,rest_m, &rem,g_bridge_sign*s);
                                 free(*currentmomentum);
                                 *(*currentmomentum=get_mem(2,sizeof(int)))=1;
                                 (*currentmomentum)[1]=res;
                              }/* else -Leave momentum as is*/
                              momentum=*currentmomentum;
                              break;
                           case 1:/*Chord*/
                              if(g_chord_subst!=NULL){
                                 int sign=(loop_m[1]<0)?(-1):1;
                                 int res=l_mk_autosubst(g_chord_subst,
                                    ++ndef,k,arg,buf,group,rest_m, &rem,
                                    g_chord_sign*sign);
                                 free(*currentmomentum);
                                 *(momentum=get_mem(2,sizeof(int)))=1;
                                 momentum[1]=res;
                                 /*Attention! DoNOT exchange args in following call:*/
                                 *currentmomentum=int_inc(momentum,loop_m,1);
                                 /*Note, momentum now points to nowhere!*/
                              }/*else - leave momentum as is*/
                              momentum=*currentmomentum;
                              break;
                           default:/*0, pure loop momentum*/
                              momentum=*currentmomentum;
                              break;
                        }/*switch(l_splitMomentum(*currentmomentum, &loop_m,&rest_m))*/
                        free_mem(&loop_m);free_mem(&rest_m);
                     }else
                        momentum=topol->momenta[j][(topol->l_subst)[k]];
                  }else{/* Just 0, only add ']':*/
                     if(   fputc(']',outf)<=0 )goto fail_write;
                        continue;
                  }

                  l_storemomenta(momentum);

                  if(   (k!=-nel)&&(k!=1)  )
                     if(   fputc(',',outf)<=0 )goto fail_write;
                  if(   fputs(build_momentum(group,momentum,arg, s),outf)<0 )goto fail_write;
               }/*for(s=1,k=-nel;k<=nil; k++)*/
               if(j<n)
                  if(   fputs(":\n",outf)<0 )goto fail_write;
            }/*for(j=0;j<topol->n_momenta;j++)*/
         }/*if(topol->n_momenta>0)*/
         if(   fputs(";\n",outf)<0 )goto fail_write;
         if(rem!=NULL){/*Remarks were created to export*/
            int res=fprintf(outf,"remarks %s=%s",*(topol->id),rem);
            free_mem(&rem);
            if(res<=0 )goto fail_write;
            if( fprintf(outf,"\n   %s = %d;\n",MOMENTA_TABLE_ROWS,ndef) <=0 )
               goto fail_write;
         }/*if(rem!=NULL)*/
      }else{/*if(...)*/
         /*Momenta came from the table*/
         char *l_can2red=NULL,*v_can2red=NULL;
         if(ComeMomentaFromFullId){/*full topology*/
            int err=0;
            char *tmp;
            l_can2red=v_can2red=NULL;
            k=top->nmomenta;
            for(j=1;j<=k;j++){
               tmp=tt_getMomenta(0,id, ind,k,&err);
               checkTT(err,FAILSAVETOP);
               if(   fputs(tmp,outf)<0 )goto fail_write;
               /*add momenta into g_wrk_vectors_table:*/
               l_storemomenta_fromstr(tmp);
               free(tmp);
               if(j!=k)
                  if(   fputs(":\n",outf)<0 )goto fail_write;
            }/*for(j=1;j<=k;j++)*/
            if(   fputs(";\n",outf)<0 )goto fail_write;

            /*linemarks:*/
            if(!linemarksready){
               if( fprintf(outf,"linemarks %s =",*(topol->id))<=0 )goto fail_write;
               for(k=1; k<=nil; k++){
                  if(   fprintf(outf,"%d",
                                (top->l_red2usr)[k]
                                                        )<=0 )goto fail_write;
                  if( k!=nil )
                  if(   fputc(',',outf)<=0 )goto fail_write;
               }/*for(k=1; k<=nil; k++)*/
               if(   fputs(";\n",outf)<0 )goto fail_write;
               linemarksready=1;
            }/*if(!linemarksready)*/

            /*vertexmarks:*/
            if(!vertexmarksready){
               if( fprintf(outf,"vertexmarks %s =",*(topol->id))<=0 )goto fail_write;
               for(k=1; k<=nv; k++){
                  if(   fprintf(outf,"%d",
                                (top->v_red2usr)[k]
                                                        )<=0 )goto fail_write;
                  if( k!=nv )
                  if(   fputc(',',outf)<=0 )goto fail_write;
               }/*for(k=1; k<=nv; k++)*/
               if(   fputs(";\n",outf)<0 )goto fail_write;
               vertexmarksready=1;
            }/*if(!vertexmarksready)*/

         }else{/*wrk topology*/
            /*External momenta from originaltopology!*/
            char group[5];
            int *momentum,err=0;
            char *tmp;
            /*First, make substitutions:*/

            l_can2red=getTopolRem("ltsubst",top_remarks,remarks);
            l_mksubst(
               arg,/*Return buffer*/
               buf,/*Wrk buffer*/

               top->l_usr2red,/*table usr2red[usr]=red*/
               top->l_dirUsr2Red,/*table dir[usr] usr<->red*/

               l_can2red,/*topology can2red[can]=red*/
               /*topolody dirCan2Red[red] red<->can:*/
               getTopolRem("ldir",top_remarks,remarks),

               topol->l_subst, /* topology subst[usr]=red*/
               topol->l_dir,/* topology ldir[usr]  usr<->red*/
               nil/*number of entries*/
            );
            /* Reorders internal lines enumerating. Pattern:
               positions -- old, values -- new, separated by tt_sep, e.g.
               "2:1:-3:5:-4" means that line 1 > 2, 2 ->1, 3 just upsets,
               4->5, 5 upsets and becomes of number 4.
               Returns 0 if substitution was implemented, or error code, if fails.
               In the latter case substitutions will be preserved:*/
            checkTT(tt_linesReorder(arg,  id,  ind),FAILSAVETOP);

            /*vertex reordering:*/
            v_can2red=getTopolRem("vtsubst",top_remarks,remarks);
            l_mksubst(
               arg,/*Return buffer*/
               buf,/*Wrk buffer*/

               top->v_usr2red,/*table usr2red[usr]=red*/
               NULL,/*table dir[usr] usr<->red*/

               v_can2red,
               NULL,

               topol->v_subst, /* topology subst[usr]=red*/
               NULL,/* topology ldir[usr]  usr<->red*/
               nv/*number of entries*/
            );
            checkTT(tt_vertexReorder(arg,  id,  ind),FAILSAVETOP);
            wasReduced=ind;
            /*Now v_can2red relates to vertices*/

            group[0]=3;group[1]=group[2]=1;group[3]='\n';group[4]=0;
            /*Here we may use only common momenta, and only the first momenta set
              from the table!*/
            if(   fputc('[',outf)<=0 )goto fail_write;

            for(k=nel;k>0;k--){
               momentum=ext_particles[k-1].momentum;

               l_storemomenta(momentum);

               if(k!=nel)
                  if(   fputc(',',outf)<=0 )goto fail_write;
               if(   fputs(build_momentum(group,momentum,arg, 1),outf)<0 )
                     goto fail_write;
            }/*for(k=nel,k>0;k--)*/
            if(   fputc(']',outf)<=0 )goto fail_write;
            tmp=tt_getMomenta(0,id, ind,1,&err);
            checkTT(err,FAILSAVETOP);

            if(   fputs(tmp,outf)<0 )goto fail_write;
            /*add momenta into g_wrk_vectors_table:*/
            l_storemomenta_fromstr(tmp);
            free(tmp);
            if(   fputs(";\n",outf)<0 )goto fail_write;
            if( tt_howmanyRemarks(id,ind)>0 ){
               unsigned int top_remarks=top->top_remarks;
               REMARK *remarks=top->remarks;
               if(  (tmp=getTopolRem(s_cat(arg,MOMENTATABLE_RIGHT,"1")
                                           ,top_remarks,remarks)) != NULL  ){
                  if(v_can2red!=NULL){/*v_can2red either NULL or vtsubst*/
                     int pows10[3]={1,10,100};
                     int ndef,s,sm,n,ntop,txt,nstack[3],qe;
                     char *txtmom=NULL;

                     if( fprintf(outf,"remarks %s =\n",*(topol->id))<=0 )goto fail_write;

                     /*#define MOMENTATABLE_RIGHT "P"*/
                     /*#define MOMENTATABLE_LEFT "Q"*/
                     /*#define MOMENTA_TABLE_ROWS "NDEF"*/
                     /*#define MOMENTATABLE_EXT "Qe"*/
                     ndef=0;
                     for(k=1; ;k++){/*Try "Q#" or Qe#*/
                         char *mem;
                         momentum=NULL;/*Will be collected parsing a remark*/
                         txtmom=NULL;
                         sprintf(arg,"%s%d",MOMENTATABLE_LEFT,k);
                         /*try "Q":*/
                         if( (tmp=getTopolRem(arg,top_remarks,remarks))==NULL ){
                            /*try "Qe":*/
                            sprintf(buf,"%s%d",MOMENTATABLE_EXT,k);
                            /*Use buf here since we need to have "Q" in arg*/
                            /* The previous comment is obsolete, but I didn't change */
                            if( (tmp=getTopolRem(buf,top_remarks,remarks))==NULL )
                               break;/*That's all*/
                            /*Parse Qe:*/
                            s=1;qe=1;
                            n=strtol(tmp,&mem,10);
                            if( ( (*tmp)=='\0')||( (*mem) !='\0' )  )
                               l_specRemErr(-3,&momentum,&txtmom,mem,id,ind);

                            if(n<0){n=-n; qe=-1;}else qe=1;
                            if( (n<1)|| (n>nv) )
                               l_specRemErr(-4,&momentum,&txtmom,mem,id,ind);
                            /*l_get_sum_incoming_momenta always allocates returned value!
                              Last 0 is cn_momentaset:*/
                            momentum = l_get_sum_incoming_momenta(v_can2red,topol,top,n,0);
                            if(
                               (*momentum!=1) ||/*Only one vector may be substituted*/
                               /*int_eq returns 0 if NOT coincide:*/
                               int_eq(momentum,g_zeromomentum)/*zero momentum*/
                              )
                                 continue;/* Left is empty, but no left - no right!*/

                            if(momentum[1] < 0){/*We always need momentum without sign!*/
                               qe=-qe;
                               momentum[1] = -momentum[1];
                            }/*if(momentum[1] < 0)*/
                            build_momentum(group,momentum,buf, 1);
                            free_mem(&momentum);
                            /*Now in buf is the  text corresponding a single momentum.
                              But with leading sign, which is '+'!*/
                            if(*buf == '+')for(tmp=buf; *tmp != '\0'; tmp++)
                               *tmp=*(tmp+1);
                            /*Now in buf is the ready text and qe contains the sign*/
                         }else{/*if( (tmp=getTopolRem(arg,top_remarks,remarks))==NULL )*/
                            /*Common "Q", just translate it*/
                            qe=0;/*this means, this is a common "Q". For "Qe"
                                   this will be +1 or -1 */
                            tt_translateTokens(tmp,buf,id,MAX_STR_LEN);
                         }/*else*/
                         if( (*buf)=='\0' )
                            continue;/*No left - no right!*/

                         ndef++;/*increment counter*/

                         if(ndef>1)
                            if( fputs(":\n",outf)<0 )goto fail_write;

                         if(
                               (   fputs("   ",outf)<0 )||
                               (   fprintf(outf,"%s%d",MOMENTATABLE_LEFT,ndef)<=0 )||
                               (   fputs(" = ",outf)<0 )||
                               (   fputs(buf,outf)<0 )
                           )goto fail_write;

                         /*left is ready - now right:*/
                         sprintf(arg,"%s%d",MOMENTATABLE_RIGHT,k);
                         if( (mem=tmp=getTopolRem(arg,top_remarks,remarks))==NULL )
                            break;
                         /*now tmp is something like +2-1+3...*/
                         s=1;
                         switch(*tmp){
                            case '-':
                               s=0;/*no break*/
                            case '+':
                               tmp++;
                         }/*switch(*tmp)*/
                         if(*tmp=='\0')s=-6;/*error empty line*/
                         n=0;ntop=0;txt=0;
                         for(;!(s<0);tmp++)switch(*tmp){
                            case '\0':sm=-1;
                            case '-':sm-=1;
                            case '+':
                               if(txt)
                                 txt=0;
                               else{
                                  if(ntop==0){/*error 'digit expected'*/
                                     s=-3; break;
                                  }/*if(ntop==0)*/
                                  if(ntop>3){/*error too large number*/
                                     s=-4; break;
                                  }/*if(ntop>3)*/
                                  for(ntop--,j=0; j<=ntop; j++)
                                     n+=(nstack[j]*pows10[ntop-j]);

                                  if( (n<1)|| (n>nv) ){
                                     s=-4; break;
                                  }/*if( (n<1)|| (n>nv) )*/
                                  /*l_get_sum_incoming_momenta always allocates
                                    returned value! Last 0 is cn_momentaset:*/
                                  momentum=int_inc(
                                     momentum,
                                     l_get_sum_incoming_momenta(v_can2red,topol,top,n,0),
                                     s
                                  );
                               }/*if(txt)... else*/
                               n=0;ntop=0;s=sm;sm=1;break;
                            case '1':nstack[ntop++]=1;break;
                            case '2':nstack[ntop++]=2;break;
                            case '3':nstack[ntop++]=3;break;
                            case '4':nstack[ntop++]=4;break;
                            case '5':nstack[ntop++]=5;break;
                            case '6':nstack[ntop++]=6;break;
                            case '7':nstack[ntop++]=7;break;
                            case '8':nstack[ntop++]=8;break;
                            case '9':nstack[ntop++]=9;break;
                            case '0':nstack[ntop++]=0;break;
                            default:
                               if(ntop==0){
                                 char *inipos=tmp;
                                 set_of_char validid;
                                 set_sub(validid,validid,validid);
                                 set_str2set(VALIDID,validid);
                                 set_sset("{}", validid);
                                 if(!set_in(*tmp,validid))
                                    s=-5;/*error parsing*/
                                 else{
                                    char ch;
                                    txtmom=s_inc(txtmom,(s==1)?("+"):("-"));
                                    set_sset("0123456789", validid);
                                    while( set_in(*tmp,validid))tmp++;
                                    ch=*tmp;
                                    *tmp=0;
                                    txtmom=s_inc(txtmom,inipos);
                                    *tmp=ch;
                                    tmp--;
                                    txt=1;
                                 }/*if(!set_in(*tmp,validid))... else*/
                               }else/*error parsing*/
                                  s=-5;
                               break;
                         }/*for(;!(s<0);tmp++)switch(*tmp)*/
                         if(s<-2)/*An error*/
                            l_specRemErr(s,&momentum,&txtmom,mem,id,ind);
                         momentum=
                            reduce_momentum(/*see topology.c*/
                               momentum,
                               g_zeromomentum,
                               emom/*Built before the main cycle*/
                            );

                         if(txtmom!=NULL){/*Textual information is present*/
                            /*int int_eq(int *i1, int *i2), utils.c:
                               returns 0 if NOT coincide, or 1:*/

                            if( int_eq(momentum,g_zeromomentum)  )/*zero momentum*/
                               tt_translateTokens(txtmom,buf,id,MAX_STR_LEN);
                            else{
                               char *tmp=buf;
                               build_momentum(group,momentum,buf, 1);
                               while(*tmp!='\0')tmp++;
                               tt_translateTokens(txtmom,tmp,id,MAX_STR_LEN-(tmp-buf+1));
                            }
                            free_mem(&txtmom);
                         }else
                            build_momentum(group,momentum,buf, 1);
                         /*Note herE, txtmom is not stored! Ok, it will be stored somewere
                           else.*/
                         l_storemomenta(momentum);
                         free_mem(&momentum);

                         if(qe<0){/*"Qe" with negative sign! Upset ight-hand value:*/
                            /*Swaps '+' and '-'; returns 0 if result is ready, or 1 if
                              it shuld be prependen by '-', tools.c:*/
                            if(swapCharSng(buf)){/*Must be prependen by '-'. In principle,
                                                  this situation is impossible!*/
                               tmp=new_str(buf);
                               sprintf(buf, "-%s",tmp);
                               free(tmp);
                            }/*if(swapCharSng(buf))*/
                         }/*if(qe<0)*/

                         /*In buf now is ready right-hand value*/

                         if(
                               (   fputs(":\n   ",outf)<0 )||
                               (   fprintf(outf,"%s%d",MOMENTATABLE_RIGHT,ndef)<=0 )||
                               (   fputs(" = ",outf)<0 )||
                               (   fputs(buf,outf)<0 )
                         )goto fail_write;
                     }/*for(k=1; ;k++)*/
                     if(ndef>0){
                         if( fprintf(outf,":\n   %s = %d",MOMENTA_TABLE_ROWS,ndef) <=0 )
                            goto fail_write;
                         g_wasAbstractMomentum++;
                     }/*if(ndef>0)*/
                     if(   fputs(";\n",outf)<0 )goto fail_write;
                  }/*if(v_can2red!=NULL)*/
               }/*if(  (tmp=getTopolRem("P1",top_remarks,remarks)) != NULL  )*/
            }/*if( tt_howmanyRemarks(id,ind)>0 )*/

            /*linemarks:*/
            if(  (!linemarksready)&&( l_can2red!=NULL )  ){
               char *red2can=new_str(l_can2red);
               invertsubstitution(red2can,l_can2red);
               if( fprintf(outf,"linemarks %s =",*(topol->id))<=0 )goto fail_write;
               for(k=1; k<=nil; k++){
                  if(   fprintf(outf,"%d",
                                (top->l_red2usr)[
                                   red2can[
                                      (topol->l_subst)[k]/* topol red*/
                                   ]/*topol can, i.e. table red */
                                ]
                                                        )<=0 )goto fail_write;
                  if( k!=nil )
                  if(   fputc(',',outf)<=0 )goto fail_write;
               }/*for(k=1; k<=nil; k++)*/
               if(   fputs(";\n",outf)<0 )goto fail_write;
               linemarksready=1;
               free(red2can);
            }/*if(  (!linemarksready)&&( (topol->linemarks)!=NULL )  )*/

            /*vertexmarks:*/
            if(  (!vertexmarksready)&&( v_can2red!=NULL )  ){
               char *red2can=new_str(v_can2red);
               invertsubstitution(red2can,v_can2red);
               if( fprintf(outf,"vertexmarks %s =",*(topol->id))<=0 )goto fail_write;
               for(k=1; k<=nv; k++){
                  if(   fprintf(outf,"%d",
                                (top->v_red2usr)[
                                   red2can[
                                      (topol->v_subst)[k]/* topol red*/
                                   ]/*topol can, i.e. table red */
                                ]
                                                        )<=0 )goto fail_write;
                  if( k!=nv )
                  if(   fputc(',',outf)<=0 )goto fail_write;
               }/*for(k=1; k<=nv; k++)*/
               if(   fputs(";\n",outf)<0 )goto fail_write;
               vertexmarksready=1;
               free(red2can);
            }/*if(  (!vertexmarksready)&&( (topol->vertexmarks)!=NULL )  )*/
         }/*if(ComeMomentaFromFullId)... else ...*/
      }/*if(...)...else...*/
      /*end momenta*/

      if(  (rem=getTopolRem("Ctbl",top_remarks,remarks)) == NULL  ){
         if(topol->coordinates_ok !=0){/*Self-contained coords*/
            if(   fprintf(outf,"coordinates %s =\n",*(topol->id))<=0 )goto fail_write;
            k=nel*2;
            if( l_outCoord(outf,"   ev ",topol->ev,k,":\n") )goto fail_write;
            if( l_outCoord(outf,"   evl ",topol->evl,k,":\n") )goto fail_write;
            if( l_outCoord(outf,"   el ",topol->el,k,":\n") )goto fail_write;
            if( l_outCoord(outf,"   ell ",topol->ell,k,":\n") )goto fail_write;

            k=nil*2;
            if( l_outCoord(outf,"   il ",topol->il,k,":\n") )goto fail_write;
            if( l_outCoord(outf,"   ill ",topol->ill,k,":\n") )goto fail_write;

            k=nv*2;
            if( l_outCoord(outf,"   iv ",topol->iv,k,":\n") )goto fail_write;
            if( l_outCoord(outf,"   ivl ",topol->ivl,k,"") )goto fail_write;

            if(   fputs(";\n",outf)<0 )goto fail_write;
         }/*if(topol->coordinates_ok !=0)*/
      }else if (rem!=NULL){
         int id;
         long ind;
         char *can2red;
         int isFull=0;
         tt_singletopol_type *top;

         id=atoi(rem);
         if(id==0)/*g_tt_wrk is always stored as 0*/
            id=g_tt_wrk;
         if(  (rem=getTopolRem("Ctop",top_remarks,remarks)) == NULL  )
            halt(INTERNALERROR,NULL);
         ind=atol(rem);
         top=( (tt_table[id-1])->topols )[ind-1];
         if(tt_table[id-1]->extN==tt_table[id-1]->totalN){/*Full topology*/
            isFull=1;
            if(
              (ComeMomentaFromFullId)&&
              ( (id!=ComeMomentaFromFullId)||(ind!=ComeMomentaFromFullInd))
            ){/*Coordinates come from the full topology different then momenta did*/
               /* Make a substitution*/
               l_mkFullSubst(arg,top->l_usr2red/*,top->l_dirUsr2Red*/,nil);
               checkTT(tt_linesReorder(arg,  id,  ind),FAILSAVETOP);
               l_mkFullSubst(arg,top->v_usr2red,/*NULL,*/nv);
               checkTT(tt_vertexReorder(arg,  id,  ind),FAILSAVETOP);

            }else{/* if (topol->n_momenta>0){*/
               /*Self-contained or auto momenta mechanism is implemented
                 (  if(topol->n_momenta>0)  ) or momenta come from wrk,
                 while  coordinates comes from a full table. */
               /*We must reduce table coords into topology*/
               l_usrTbl2usrTopol(
                  arg,/*Return buffer*/
                  buf,/*Wrk buffer*/

                  top->l_usr2red,/*table usr2red[usr]=red*/
                  topol->l_subst, /* topology subst[usr]=red*/
                  nil/*number of entries*/
               );
               checkTT(tt_linesReorder(arg,  id,  ind),FAILSAVETOP);

               l_usrTbl2usrTopol(
                  arg,/*Return buffer*/
                  buf,/*Wrk buffer*/

                  top->v_usr2red,/*table usr2red[usr]=red*/
                  topol->v_subst, /* topology subst[usr]=red*/

                  nv/*number of entries*/
               );
               checkTT(tt_vertexReorder(arg,  id,  ind),FAILSAVETOP);
            }/*if( ComeMomentaFromFullId...) ... else */
         }else /* int topology */ if(wasReduced!=ind){/*different then momenta*/

            /*Lines reordering*/
            if(id==g_tt_wrk)/*Internal topologies*/
               can2red=getTopolRem("ltsubst",top_remarks,remarks);
            else/*full topology*//*debug*/
               halt("id!=g_tt_wrk",NULL);

            l_mksubst(
               arg,/*Return buffer*/
               buf,/*Wrk buffer*/

               top->l_usr2red,/*table usr2red[usr]=red*/
               top->l_dirUsr2Red,/*table dir[usr] usr<->red*/

               can2red,
               /*topolody dirCan2Red[red] red<->can:*/
               getTopolRem("ldir",top_remarks,remarks),

               topol->l_subst, /* topology subst[usr]=red*/
               topol->l_dir,/* topology ldir[usr]  usr<->red*/
               nil/*number of entries*/
            );
            checkTT(tt_linesReorder(arg,  id,  ind),FAILSAVETOP);

            /*vertex reordering*/
            if(id==g_tt_wrk)/*Internal topologies*/
               can2red=getTopolRem("vtsubst",top_remarks,remarks);
            else/*full topology*/
               can2red=NULL;

            l_mksubst(
               arg,/*Return buffer*/
               buf,/*Wrk buffer*/

               top->v_usr2red,/*table usr2red[usr]=red*/
               NULL,/*table dir[usr] usr<->red*/

               can2red,
               NULL,

               topol->v_subst, /* topology subst[usr]=red*/
               NULL,/* topology ldir[usr]  usr<->red*/
               nv/*number of entries*/
            );
            checkTT(tt_vertexReorder(arg,  id,  ind),FAILSAVETOP);
         }/*if(!wasReduced)*/

         if(   fprintf(outf,"coordinates %s =\n",*(topol->id))<=0 )goto fail_write;

         if(isFull){/*Full topology*/
            checkTT(tt_getCoords( arg,MAX_STR_LEN,id,ind,EXT_VERT),FAILSAVETOP);
            if( fprintf(outf,"   ev %s:\n",arg)<=0 )goto fail_write;
            checkTT(tt_getCoords( arg,MAX_STR_LEN,id,ind,EXT_VLBL),FAILSAVETOP);
            if( fprintf(outf,"   evl %s:\n",arg)<=0 )goto fail_write;
            checkTT(tt_getCoords( arg,MAX_STR_LEN,id,ind,EXT_LINE),FAILSAVETOP);
            if( fprintf(outf,"   el %s:\n",arg)<=0 )goto fail_write;
            checkTT(tt_getCoords( arg,MAX_STR_LEN,id,ind,EXT_LLBL),FAILSAVETOP);
            if( fprintf(outf,"   ell %s:\n",arg)<=0 )goto fail_write;
         }else{/*if(tt_table[id-1]->extN==tt_table[id-1]->totalN)*/
            /*What about external coordinates?*/
            if( g_ext_coords_ev!=NULL )
               if( fprintf(outf,"   ev %s:\n",g_ext_coords_ev)<=0 )goto fail_write;
            if( g_ext_coords_evl!=NULL )
               if( fprintf(outf,"   evl %s:\n",g_ext_coords_evl)<=0 )goto fail_write;
            if( g_ext_coords_el!=NULL )
               if( fprintf(outf,"   el %s:\n",g_ext_coords_el)<=0 )goto fail_write;
            if( g_ext_coords_ell!=NULL )
               if( fprintf(outf,"   ell %s:\n",g_ext_coords_ell)<=0 )goto fail_write;
         }/**/
         checkTT(tt_getCoords( arg,MAX_STR_LEN,id,ind,INT_VERT),FAILSAVETOP);
         if( fprintf(outf,"   iv %s:\n",arg)<=0 )goto fail_write;
         checkTT(tt_getCoords( arg,MAX_STR_LEN,id,ind,INT_VLBL),FAILSAVETOP);
         if( fprintf(outf,"   ivl %s:\n",arg)<=0 )goto fail_write;
         checkTT(tt_getCoords( arg,MAX_STR_LEN,id,ind,INT_LINE),FAILSAVETOP);
         if( fprintf(outf,"   il %s:\n",arg)<=0 )goto fail_write;
         checkTT(tt_getCoords( arg,MAX_STR_LEN,id,ind,INT_LLBL),FAILSAVETOP);
         if( fprintf(outf,"   ill %s;\n",arg)<=0 )goto fail_write;
      }/*else if (rem!=NULL) */

      if(  ComeMomentaFromFullId || selfMomenta ){
         unsigned int top_remarks=topol->top_remarks;
         REMARK *remarks=topol->remarks;
         /*Output remarks, translated!*/
         tt_singletopol_type *top=NULL;
         {/*block*/
            char *rem=getTopolRem(MOMENTA_TABLE_ROWS,top_remarks,remarks);
            if(rem != NULL) {
                int n=atoi(rem);
                if(n>0)
                   g_wasAbstractMomentum++;
            }/*if(rem != NULL)*/
         }/*block*/
         /*First, remove duty remarks: safety since a full topology
         may be used only once!*/
         deleteTopolRem(NULL,top_remarks,remarks, "ltsubst");
         deleteTopolRem(NULL,top_remarks,remarks, "vtsubst");
         deleteTopolRem(NULL,top_remarks,remarks, "ldir");
         deleteTopolRem(NULL,top_remarks,remarks, "Mtbl");
         deleteTopolRem(NULL,top_remarks,remarks, "Mtop");
         deleteTopolRem(NULL,top_remarks,remarks, "Ctbl");
         deleteTopolRem(NULL,top_remarks,remarks, "Ctop");
         deleteTopolRem(NULL,top_remarks,remarks, "MLoop");
         /*Check are there topology remarks in the topology itself:*/
         for(k=0,j=0;j<top_remarks;j++)
            if( remarks[j].name != NULL ){/*Occupied cell*/
                  k=1;break;
            }/*Occupied cell*/

         if(ComeMomentaFromFullId){
            top=( (tt_table[ComeMomentaFromFullId-1])->topols )[ComeMomentaFromFullInd-1];
            /*Check are there topology remarks in the table:*/
            if(k==0)
               k=(top->top_remarks!=0);
         }/*if(ComeMomentaFromFullId)*/
         if(k||exportForMarks){
            int n=0;
            if( fprintf(outf,"remarks %s =\n",*(topol->id))<=0 )goto fail_write;
            if( exportForMarks ){
               if( fprintf(outf,"   t = %u",i)<=0 )
                  goto fail_write;
               n++;
            }/*if( exportForMarks )*/
            for(k=0; k<2; k++){/*need not k anymore, use it to repeat treanslation/
                                 outputting remarks for top after processing topol:*/
               /*Here k == 0 for topolo, k=1 for top:*/
               for(j=0;j<top_remarks; j++){
                  if( (remarks[j]).name != NULL ){
                     if(   (remarks[j]).text  == NULL   ){
                        *arg='\0';
                        rem=arg;
                     }else{
                        if(g_tt_wrk){
                           checkTT(
                              tt_translateTokens(
                                (remarks[j]).text,arg,g_tt_wrk,MAX_STR_LEN
                              ),
                              FAILSAVETOP
                           );
                           rem=arg;
                        }else
                           rem=(remarks[j]).text;
                     }/*if(   (remarks[j]).text  == NULL   )...else...*/
                     if( n++ ==0 ){
                        if( fprintf(outf,"   %s = %s",(remarks[j]).name,rem)<=0 )
                           goto fail_write;
                     }else{
                        if( fprintf(outf,":\n   %s = %s",(remarks[j]).name,rem)<=0 )
                           goto fail_write;
                     }/*if( n++ ==0 ) ... else ... */
                  }/*if( (remarks[j]).name != NULL )*/
               }/*for(i=0;i<top_remarks; i++)*/
               if(top==NULL)/*No table!*/
                  break;
               top_remarks=top->top_remarks;
               remarks=top->remarks;
            }/*for(k=0; k<2; k++)*/
            if(   fputs(";\n",outf)<0 )goto fail_write;
         }/*if(k)*/
      }/*if(ComeMomentaFromFullId)*/

      /*linemarks:*/
      if(  (!linemarksready)&&( (topol->linemarks)!=NULL )  ){
         if( fprintf(outf,"linemarks %s =",*(topol->id))<=0 )goto fail_write;
         for(k=1; k<=nil; k++){
           if(   fputs(topol->linemarks[k],outf)<0 )goto fail_write;
           if( k!=nil )
              if(   fputc(',',outf)<=0 )goto fail_write;
         }/*for(k=1; k<=nil; k++)*/
         if(   fputs(";\n",outf)<0 )goto fail_write;
         linemarksready=1;
      }/*if(  (!linemarksready)&&( (topol->linemarks)!=NULL )  )*/

      /*vertexmarks:*/
      if(  (!vertexmarksready)&&( (topol->vertexmarks)!=NULL )  ){
         if( fprintf(outf,"vertexmarks %s =",*(topol->id))<=0 )goto fail_write;
         for(k=1; k<=nv; k++){
           if(   fputs(topol->vertexmarks[k],outf)<0 )goto fail_write;
           if( k!=nv )
              if(   fputc(',',outf)<=0 )goto fail_write;
         }/*for(k=1; k<=nv; k++)*/
         if(   fputs(";\n",outf)<0 )goto fail_write;
         vertexmarksready=1;
      }/*if(  (!vertexmarksready)&&( (topol->vertexmarks)!=NULL )  )*/

   }/*for(i=0; i<top_topol; i++)*/

   hash_table_done(uniqueNames);
   free(buf);
   free(fnam);
   fclose(outf);
   free(emom);
   return long2str(arg,ret);
   fail_write:
      hash_table_done(uniqueNames);
      free(buf);
      s_let(fnam,arg);
      free(fnam);
      fclose(outf);
      free(emom);
      halt(CANNOTWRITETOFILE,arg);
      return arg;
}/*l_saveTopologies*/

/*\expTopLoopMark(filename) saves topologies with automatically distributed momenta
  into a named file.  Returns number of saved topologies.*/
static char *macexpTopLoopMark(char *arg)
{
 return l_saveTopologies(arg,1);
}/*macexpTopLoopMark*/

/*\saveTopologies(filename) saves topologies into a named file.
  Returns number of saved topologies.*/
static char *macsaveTopologies(char *arg)
{
 return l_saveTopologies(arg,0);
}/*macsaveTopologies*/

typedef struct{
   char *outstr;
   int count;
   int maxlen;
}VInfoType;

/*#endif*/
/*Iterator -- saves a list of (comma-separated) "tags". Must return 0!!! :*/
/* No overflow control yet!*/
static int l_printVectorIterator(void *info,HASH_CELL *m, word index_in_tbl, word index_in_chain)
{
char *tmp=(  (VInfoType *)info  )->outstr;
   if(  (     ( (  (VInfoType *)info  )->maxlen )-=(s_len((char*)m->tag)+1)     )< 1 )
      /*1: ", " 1:'\0'*/
      return -1;
   if(  (  (VInfoType *)info  )->count  > 0 ){/*Not first occurence*/
      s_let(",",tmp); while(*tmp!='\0')tmp++;
      if(   (  ( (  (VInfoType *)info  )->count )+1  )>=MAX_OUTPUT_LEN  ){
         /*1:", "*/
         if( (     ( (  (VInfoType *)info  )->maxlen )-=4     ) < 1 ) return -1;
         s_let("\n   ",tmp);while(*tmp!='\0')tmp++;
         (     (  (VInfoType *)info  )->count    )=0;
      }/*if(   (  ( (  (VInfoType *)info  )->count )+1  )>=MAX_OUTPUT_LEN  )*/
   }/*if(  (  (VInfoType *)info  )->count >0 )*/
   s_let( (char*)m->tag, tmp);while(*tmp!='\0')tmp++;
   (     (  (VInfoType *)info  )->count    )+=s_len((  (VInfoType *)info  )->outstr);
   (  (VInfoType *)info  )->outstr=tmp;
   return 0;
           /*In success, must return FALSE.*/
}/*l_printVectorIterator*/

/*\allVectors() returns all vectors appearing in a working table and in the model*/
static char *macallVectors(char *arg)
{
   *arg='\0';
   if(g_topVectorsInTheModel>0){/*Add momenta from the model to g_wrk_vectors_table*/
      int i;char *tmp;

      if(g_wrk_vectors_table == NULL)/*First create the table:*/
         g_wrk_vectors_table=create_hash_table(vectors_hash_size,
                                             str_hash,str_cmp,c_destructor);
      for(i=0; i<g_topVectorsInTheModel; i++){
         tmp=new_str(g_vectorsInTheModel[i]);
         install(tmp,tmp,g_wrk_vectors_table);
      }/*for(i=0; i<g_topVectorsInTheModel; i++)*/
      free_mem(&g_vectorsInTheModel);
      g_topVectorsInTheModel=0;
   }/*if(g_topVectorsInTheModel>0)*/
   
   if(g_wrk_vectors_table != NULL){
      VInfoType VInfo;
      VInfo.outstr=arg;
      VInfo.count=0;
      VInfo.maxlen=MAX_STR_LEN;
      if( hash_foreach(g_wrk_vectors_table,&VInfo,&l_printVectorIterator)!=0 )
         halt(TOOLONGSTRING,NULL);
   }/*if(g_wrk_vectors_table != NULL)*/

   return arg;
}/*macallVectors*/

static char *macisVectorPresent(char *arg)
{
   if (lookup(get_string(arg),g_wrk_vectors_table)==NULL)
       return s_let("no",arg);
   return s_let("yes",arg);
}/*macisVectorPresent*/

static char *macvectors(char *arg)
{
VInfoType VInfo;
int i,j, *l;
char *vec;
HASH_TABLE vectors_table=NULL;

  if(g_nocurrenttopology)
      halt(CANNOTCREATEOUTPUT,NULL);
   *arg='\0';
   vectors_table=create_hash_table(vectors_hash_size,
                                          str_hash,str_cmp,c_destructor);

    if(
         (topologies[cn_topol].ext_momenta!=NULL) &&
         (topologies[cn_topol].ext_momenta[cn_momentaset]!=NULL)
       ){
          for(i=1;i<=ext_lines;i++){
             l=topologies[cn_topol].ext_momenta[cn_momentaset][i];
             for(j=1;j<=*l;j++){
                vec=new_str(vec_group[abs(l[j])].vec );
                install(vec,vec,vectors_table);
             }/*for(j=1;j<=*l;j++)*/
          }/*for(i=1;i<=ext_lines;i++)*/
    }else{
          for(i=1;i<=ext_lines;i++){
             l=ext_particles[i-1].momentum;
             for(j=1;j<=*l;j++){
                vec=new_str(vec_group[abs(l[j])].vec );
                install(vec,vec,vectors_table);
             }/*for(j=1;j<=*l;j++)*/
          }/*for(i=1;i<=ext_lines;i++)*/
    }/*if(...) ... else ... */

    if(
       (topologies[cn_topol].momenta!=NULL) &&
       (topologies[cn_topol].momenta[cn_momentaset]!=NULL)
    )for(i=1; i<=int_lines;i++){
       l=topologies[cn_topol].momenta[cn_momentaset][i];
       for(j=1;j<=*l;j++){
          vec=new_str(vec_group[abs(l[j])].vec );
          install(vec,vec,vectors_table);
       }/*for(j=1;j<=*l;j++)*/
    }/*if(...)for(i=1; i<=int_lines;i++)*/

    if(  (l=g_zeromomentum)!=NULL)
       for(j=1;j<=*l;j++){
          vec=new_str(vec_group[abs(l[j])].vec );
          install(vec,vec,vectors_table);
       }/*for(j=1;j<=*l;j++)*/

    VInfo.outstr=arg;
    VInfo.count=0;
    VInfo.maxlen=MAX_STR_LEN;
    if( hash_foreach(vectors_table,&VInfo,&l_printVectorIterator)!=0 )
       halt(TOOLONGSTRING,NULL);

    hash_table_done(vectors_table);

    return arg;
}/*macvectors*/

#ifdef SKIP
/*Old version, using getindexes.*/
static char *macvectors(char *arg)
{
int i;
   *arg='\0';

    if(
         (topologies[cn_topol].ext_momenta!=NULL) &&
         (topologies[cn_topol].ext_momenta[cn_momentaset]!=NULL)
       ){
          for(i=1;i<=ext_lines;i++)
          if(
                   getindexes(
                      arg,
                      topologies[cn_topol].ext_momenta[cn_momentaset][i],
                      MAX_STR_LEN,
                      MAX_OUTPUT_LEN
                   )  /*maximal possible number of eol is MAX_STR_LEN/MAX_OUTPUT_LEN+1*/
              <0
          ) halt(TOOLONGSTRING,NULL);
    }else{
          for(i=1;i<=ext_lines;i++)
          if(
                   getindexes(
                      arg,
                      ext_particles[i-1].momentum,
                      MAX_STR_LEN,
                      MAX_OUTPUT_LEN
                   )  /*maximal possible number of eol is MAX_STR_LEN/MAX_OUTPUT_LEN+1*/
              <0
          )halt(TOOLONGSTRING,NULL);
    }/*else*/

    for(i=1; i<=int_lines;i++)
          if(
                   getindexes(
                      arg,
                      topologies[cn_topol].momenta[cn_momentaset][i],
                      MAX_STR_LEN,
                      MAX_OUTPUT_LEN
                   )  /*maximal possible number of eol is MAX_STR_LEN/MAX_OUTPUT_LEN+1*/
              <0
          ) halt(TOOLONGSTRING,NULL);

    return arg;
}/*macvectors*/
#endif

/*The same as a "new_str"", but checks that exactly n symbols '%' are present in
  a combination "%s". No checking about NULL!:*/
static char *check_format(char *str, int n)
{
register char *ret=str,*buf;
 while(*ret!='\0'){
    if(*ret == '%')
       switch (ret[1]){
          case '%':ret++;break;
          case 's':n--;break;
          default:halt(INVALIDTEMPLATE,str);
    }/*switch (*ret)*/
    ret++;
 }/*while(*ret!='\0')*/
 if(n!=0)halt(INVALIDTEMPLATE,str);
 ret=buf=get_mem(ret-str+1,sizeof(char));
 while(  (*buf++=*str++)!='\0'  );
 return ret;
}/*check_format*/

/*\createMomentaTable(var,format) --
  sets local variables var1...varN according to topology remarks P1 ... PN, Q1 ... QN
  and var0 is the number of created variables. Returns this number. If returned
  number is 0, then the local variables are NOT created. The string "format"
  must be a valid format line containg two '%s' specifiers, or be an empty line.
  In latter case the default value (#define %s "(%s)") will be used AND other variables,
  (var'P#',#) will be created.
 */
static char *maccreateMomentaTable(char *arg)
{
char *buf,*left,*right,*fmt=NULL, *var;
int i,n;
  if(g_nocurrenttopology)
      halt(CANNOTCREATEOUTPUT,NULL);

  buf=get_mem(MAX_STR_LEN,sizeof(char));
  if(*get_string(arg)!='\0')
     fmt=check_format(arg,2);

  var=new_str(get_string(arg));

  /*getTopolRem  see remarks.c:*/
  if(
     (
       left=getTopolRem(MOMENTA_TABLE_ROWS,
                       topologies[cn_topol].top_remarks,
                        topologies[cn_topol].remarks
                      )
     )==NULL
  ){
     s_let("0",arg);
     goto ret;
  }/*if(...)*/

  if( (n=atoi(left))==0 ){
     long2str(arg,0);
     goto ret;
  }/*if( (n=atoi(left))==0 )*/

  sprintf(buf,"%d",n);
  set_run_var(s_cat(arg,var,"0"),buf);

  for(i=1;i<=n;i++){
     sprintf(arg,"%s%d",MOMENTATABLE_RIGHT,i);
     right=getTopolRem( arg,
                       topologies[cn_topol].top_remarks,
                       topologies[cn_topol].remarks
                      );

     if(right==NULL)
        right=g_zeroVec;

     sprintf(arg,"%s%d",MOMENTATABLE_LEFT,i);
     left=getTopolRem( arg,
                       topologies[cn_topol].top_remarks,
                       topologies[cn_topol].remarks
                      );

     if(right==NULL){
        set_run_var(arg,"");
        continue;
     }/*if(right==NULL)*/
     sprintf(arg,"%s%d",var,i);

     if(fmt==NULL){
        sprintf(buf,"#define %s \"(%s)\"",left,right);
        set_run_var(arg,buf);
     }else{
        sprintf(buf,fmt,left,right);
        set_run_var(arg,buf);
        /* And create 'var[left] = i'*/
        sprintf(arg,"%s%s",var,left);
        set_run_var(arg,long2str(buf,i));
     }/*if(fmt==NULL)...else */

  }/*for(i=1;i<=n;i++)*/

  long2str(arg,n);

  ret:
  free(var);
  free_mem(&fmt);
  free(buf);
  return arg;
}/*maccreateMomentaTable*/

/*\dummyMomenta(var) --
  sets local variables var1...varN according to the contents of topology remarks
  Q1 ... QN  and var0 is the number of created variables. Returns this number.
  If returned  number is 0, then the local variables are NOT created.
 */
static char *macdummyMomenta(char *arg)
{
char *val,*var;
int i,n;
  if(g_nocurrenttopology)
      halt(CANNOTCREATEOUTPUT,NULL);

  var=new_str(get_string(arg));

  /*getTopolRem  see remarks.c:*/
  if(
     (
       val=getTopolRem(MOMENTA_TABLE_ROWS,
                       topologies[cn_topol].top_remarks,
                        topologies[cn_topol].remarks
                      )
     )==NULL
  ){
     s_let("0",arg);
     goto ret;
  }/*if(...)*/

  if( (n=atoi(val))==0 ){
     long2str(arg,0);
     goto ret;
  }/*if( (n=atoi(num))==0 )*/

  {/*block*/
     char buf[NUM_STR_LEN];
     sprintf(buf,"%d",n);
     set_run_var(s_cat(arg,var,"0"),buf);
  }/*block*/

  for(i=1;i<=n;i++){
     sprintf(arg,"%s%d",MOMENTATABLE_LEFT,i);
     val=getTopolRem( arg,
                       topologies[cn_topol].top_remarks,
                       topologies[cn_topol].remarks
                      );

     sprintf(arg,"%s%d",var,i);

     if(val==NULL)
        set_run_var(arg,"");
     else
        set_run_var(arg,val);
  }/*for(i=1;i<=n;i++)*/

  long2str(arg,n);

  ret:
  free(var);
  return arg;
}/*macdummyMomenta*/

/*Prototype:*/
/*The function may be used to adjust a timeout for select() call. Parameter sv must contain
a time returned gettimeofday() at start, the parameter t -s the timeout in milliseconds.
Based on these two parameters, the function evaluates remaining time, puts it into tv and
returns a pointer to it. If the timeout is already expired, it returns 0. If t<0,
the function returns NULL leaving tv untouched.*/
struct timeval *adjust_timeout(
                                 struct timeval *sv,/* Start time*/
                                 struct timeval *tv,/* Produced timeout*/
                                 long int t/*initial timeout in milliseconds*/
                              );

/*
int readHex(int fd, char *buf, size_t len, char *cmd, size_t cmdlen);
int hexwide(unsigned long int j);
ssize_t writeFromb(int fd, char *buf, size_t count);
ssize_t read2b(int fd, char *buf, size_t count);
int readexactly(int fd, char *buf, size_t count);
ssize_t read2b(int fd, char *buf, size_t count);
int writexactly(int fd, char *buf, size_t count);
*/

/*\int2hex(int,width) - converts integer decimal reprezentation of int into hexadecimal.
width is a width of the result (it will be left-padded by zeroes), if width <1,
then the width will be calculated.
*/
static char *macint2hex(char *arg)
{
long int w=get_num("int2hex",arg);
long int i=get_num("int2hex",arg);
   if(w<1)
      w=hexwide(i);
   return int2hex(arg,i,w);
}/*macint2hex*/

/*\hex2int(hex) - converts hexadecimal reprezentation into decimal.*/
static char *machex2int(char *arg)
{
   return long2str(arg,hex2int(get_string(arg),-1));
}/*machex2int*/

/*\_waitall(timeout), waits for all executing jobs will be finished,
  or until timeout (millisecs) is expiried. If timeout<0, waits for infinity.
  Returns empty string( if all jobs are finished before timeout) ot number
  of active jobs. If it was interruted by information from the pipe
  (see \setpipeforwait()), the output is prepended by '-':*/
static char *mac_waitall(char *arg)
{
long int timeout=get_num("_waitall",arg);
fd_set rfds;
struct timeval tv;
struct timeval sv;
int i,maxd=(g_fromserver>g_pipeforwait)?(g_fromserver+1):(g_pipeforwait+1);
mysighandler_t oldPIPE;
   /* if compiler fails here, try to change the definition of
      mysighandler_t on the beginning of the file types.h*/
   oldPIPE=signal(SIGPIPE,SIG_IGN);

   *arg='\0';
   if(g_toserver<0)
      goto macwaitallfails;
   if( writexactly(g_toserver,d2cREPALLJOB,2) !=0)
       goto macwaitallfails;

   /*Wait for confirmation:*/
   gettimeofday(&sv, NULL);/*Store start time*/

   do{
      FD_ZERO(&rfds);
      FD_SET(g_fromserver,  &rfds);
      if(g_pipeforwait>-1)
         FD_SET(g_pipeforwait,  &rfds);
      switch( select(maxd, &rfds, NULL, NULL,
                                adjust_timeout(&sv,&tv,timeout) )
      ){
         case 2:
         case 1:
            if(FD_ISSET(g_fromserver, &rfds)){
               switch(readHex(g_fromserver,arg,2, NULL,0)){
                  /*case -1: s_let(SERVERFAILS,arg);goto macwaitallfails;*/
                  case 0:
                  case c2dALLJOB_i:s_let("",arg);goto macwaitallfails;
                  default:/*Something is wrong!*/
                     s_let(SERVERFAILS,arg);goto macwaitallfails;
                     /*continue;*/
               }/*switch(readHex())*/
            }/*if(FD_ISSET(g_fromserver, &l.rfds))*/

            if(
                   (g_pipeforwait<0)
                || (!FD_ISSET(g_pipeforwait, &rfds))
              )
               break;

         case 0:/*Timeout*/
            /*Or, g_pipeforwait is in rfds*/
            if(   writexactly(g_toserver,d2cNOREPALLJOB,2)/*cancel "reporting" state*/
               || writexactly(g_toserver,d2cGETNREM,2)/* get the number of remaining jobs*/
              ){/*fails*/
               s_let(SERVERFAILS,arg);goto macwaitallfails;
            }/*if( writexactly(g_toserver,d2cGETNREM,2) )*/

            if(  (i=readHex(g_fromserver,arg,2, NULL,0))<1 ){
                  s_let(SERVERFAILS,arg);goto macwaitallfails;
            }/*if(  (i=readHex(g_fromserver,arg,2, NULL,0))<1 )*/

            /*Here may be only c2dREMJOBS_i or c2dALLJOB_i, in latter case
              we must re-read the pipe:*/
            switch(i){
               default:
               case c2dREMJOBS_i:
                  break;
               case c2dALLJOB_i:
                  i=readHex(g_fromserver,arg,2, NULL,0);
                  break;
            }/*switch(i)*/
            if(i != c2dREMJOBS_i){
               s_let(SERVERFAILS,arg);goto macwaitallfails;
            }/*if(i != c2dREMJOBS_i)*/

            /*Read a width:*/
            if(  (i=readHex(g_fromserver,arg,1, NULL,0))<1 ){
               s_let(SERVERFAILS,arg);goto macwaitallfails;
            }/*if(  (i=readHex(g_fromserver,arg,1, NULL,0))<1 )*/

            if(  (i=readHex(g_fromserver,arg,i, NULL,0))<0 ){
               /*0 may be!*/
               s_let(SERVERFAILS,arg);goto macwaitallfails;
            }/*if(  (i=readHex(g_fromserver,arg,i, NULL,0))<1 )*/
            {/*Block*/
               char *ptr;
               if(
                      (g_pipeforwait>-1)
                   && FD_ISSET(g_pipeforwait, &rfds)
                 ){
                  ptr=arg+1;
                  *arg='-';
               }else
                  ptr=arg;
               if(i==0)
                  *ptr='\0';
               else{
                  long2str(ptr,i);
               }
            }/*Block*/
            goto macwaitallfails;

         default:/*-1*/
            if (errno != EINTR){
               /*a "select" error*/
               s_let(INTERNALERROR,arg);
               goto macwaitallfails;
            }/*if (errno != EINTR)*/
            /*Else - select was interrupted by a signal, nothing special...*/
            continue;
      }/*switch(select())*/
   }while(1);
   macwaitallfails:
      /* And now, restore old signals:*/
      signal(SIGPIPE,oldPIPE);
      return arg;
}/*mac_waitall*/

/*\pingServer(ip) checks is DIANA server available on a given IP address.
The argument ip is an IP address in standard dot notation. Returns
alive if the serever responds, or an empty string, if the server
can't be found or does not respond the ping.*/
static char *macpingServer(char *arg)
{
   /*1 if alive, otherwise, 0:*/
  if( pingSrv(get_string(arg))  )
     return s_let("alive",arg);
  return s_let("",arg);
}/*macpingServer*/

/*\killServer(ip) kills the DIANA server running on a given IP address.
The argument ip is an IP address in standard dot notation. Returns
ok on success, or an empty string if fails.*/
static char *mackillServer(char *arg)
{
   /*1 if alive, otherwise, 0:*/
  if( killSrv(get_string(arg))  )
     return s_let("ok",arg);
  return s_let("",arg);
}/*mackillServer*/

/*\lastjobname() returns the name of the last job started by the
operator \_exec()*/
static char *maclastjobname(char *arg)
{
   if(g_lastjobname==NULL)
      halt(LASTJOBNAMEISUNDEFINED, NULL);

   return s_let(g_lastjobname,arg);
}/*maclastjobname*/

/*\lastjobnumber() returns the order number of the last job started by
the operator \_exec()*/
static char *maclastjobnumber(char *arg)
{
  return long2str(arg,g_jobscounter);
}/*maclastjobnumber*/

/*Processing routine for _execattr and _exec, clearoldpars - if !=0, clear
  the old attribute parameters. Returns the index of  maximal allocated parameter,
  or -1; if no parameters where allocated:
  */
static int l_do_execattr(int clearoldpars, EXECATTR *execattr,char *arg)
{
int i,j=0,n, mask, npar=-1;
char *par[32];

   par[0]=NULL;/*If changed, it will point to the beginning of parameters*/

   if(*get_string(arg)!='\0'){
      char *ptr=new_str(arg);
      for(n=0; n<32;n++){
         par[n]=ptr;
         for(;*ptr!=chEOF;ptr++)if(*ptr == '\0') goto mac_execattr_theend;
         *ptr++='\0';
      }/*for(i=0; i<32;i++)*/
      mac_execattr_theend:
      if(n<32)
         n++;
   }/*if(*get_string(arg)!='\0')*/
   /*Now n is equal to the number of parameters while par[0] points to the allocate array
    (if any)*/

   get_string(arg);

   for(i=0; (i<32)&&(arg[i]!='\0');i++){
       mask=1l << i;
       /*set status bit according to attr:*/
       switch(arg[i]){
          case '0':/*Clear a corresponding attribute:*/
             execattr->attr &= ~mask;
             break;
          case '1':/*Set up a corresponding attribute:*/
             execattr->attr |= mask;
             if(execattr->par[i]!=NULL){/*Parameter here*/
                /*NULL means no par, "" menas default*/
                if(clearoldpars)
                   free_mem(execattr->par + i);/*Clear old value*/
                if(n>0){
                   execattr->par[i]=new_str(par[j++ %n]);
                }else
                   execattr->par[i]=new_str("");
                npar=i;/*Stroe the index - value to be returned!*/
             }/*if(execattr->par[i]!=NULL)*/
             break;
          default:
             continue;
       }/*switch(arg[i])*/
   }/*for(i=0; i<32;i++)*/

   free_mem(par);/*==free_mem(&(par[0]))*/

   return npar;
   /*"arg" is unspecified*/
}/*l_do_execattr*/

/*\_execattr(attr,par) - attr - 32 symbols corresponding exec attribytes.
 0 - clear, 1 - rise up, anythihg else - leave it as is. If attr is longer
 then 32, ignore the rest. If shorter, assume the tail is "leave as is".
    The second argument are possible parameters. Parameters may be separated
 by \eof(). They will be cycled around all set attributes requiring a parameter,
 e.g. \_execattr(1!011,255\eof()10) - the fourth attribute will get a parameter
 "255", the fifth attribute will get a parameter "10" (since the first does not
 require parametes, and the second is not set). Another example:
 \_execattr(10011,10) - both fourth and fifth attrs. will get "10".
 The operator returns an empty string.

 At present, the following attrs. are used:
 0  SYNC_JOB_T - must be performed after all previously started jobs finish
 2  STICKY_JOB_T - must be performed at the same handler as job to stick was
 4  FAIL_STICKY_JOB_T - fail if job to stick failed
 8  ERRORLEVEL_JOB_T - 0--255 - success if errlevel <=; -1 (default) -
                success if finished, -2 - success if started
 16 RESTART_JOB_T - number of job restarts if files, 0 if not set up

 */
static char *mac_execattr(char *arg)
{
   l_do_execattr(1,&g_execattr,arg);
   return s_let("",arg);
}/*mac_execattr*/

/*\killServers kills all the DIANA servers running.
Returns empty stringon success, or "none" if fails.*/
static char *mackillServers(char *arg)
{
mysighandler_t oldPIPE;
   /* if compiler fails here, try to change the definition of
      mysighandler_t on the beginning of the file types.h*/
   oldPIPE=signal(SIGPIPE,SIG_IGN);

   if(g_pipetoclient>-1){/*Client is started, but not activated*/
      activateClient(g_pipetoclient);
   }else if(g_toserver<0){/*Client is not started yet*/
      g_pipetoclient=1;/*Start it and activate*/
      startclient(&g_pipetoclient);
   }/*if(g_pipetoclient>-1)...else*/

   if(
        (g_toserver<0)
      ||(writexactly(g_toserver,d2cKILLALL,2) !=0)
     )s_let("none",arg);
   else
      *arg='\0';

   /* And now, restore old signals:*/
   signal(SIGPIPE,oldPIPE);
   return arg;
}/*static char *mackillServers*/

/*if answerlength < 0,  read 2 bytes from the server as an integer and converts it to
  answerlength; if answerlength == 0 do not read answer.
  if querylength>0, send it to the server as a query length. BUT! Anyway,
  send th the server WHOLE arg, independently on querylength.
  */
static char *l_askNbytes(char *arg, char *cmd, int querylength, int answerlength)
{
char tmp[3];
int l=s_len(arg);
mysighandler_t oldPIPE;
   /* if compiler fails here, try to change the definition of
      mysighandler_t on the beginning of the file types.h*/
   oldPIPE=signal(SIGPIPE,SIG_IGN);

  if(g_toserver<0) goto askNbytesFails;
  if( writexactly(g_toserver,cmd,2) !=0  )goto askNbytesFails;
  if(l>0){
     if(querylength<1)querylength=l;
     if( writexactly(g_toserver,int2hex(tmp,querylength,2),2) !=0  )goto askNbytesFails;
     if( writexactly(g_toserver,arg,l) !=0  )goto askNbytesFails;
  }/*if(l>0)*/

  if(answerlength <0){
     if(  ( answerlength=readHex(g_fromserver,arg,2, NULL,0) )<1  )goto askNbytesFails;
  }/*if(answerlength <1)*/

  if(answerlength>0)
     if( readexactly(g_fromserver,arg,answerlength) !=0  )goto askNbytesFails;

  arg[answerlength]=0;
  /* And now, restore old signals:*/
  signal(SIGPIPE,oldPIPE);
  return arg;

  askNbytesFails:
     *arg='\0';
     /* And now, restore old signals:*/
     signal(SIGPIPE,oldPIPE);
     return arg;
}/*l_askNbytes*/

/*\rmjob(jobname) remove the named job. Returns an empty string:*/
static char *macrmjob(char *arg)
{
   return l_askNbytes(get_string(arg),d2cRMJOB,0,0);
}/*macrmjob*/

/*\whichIP(jobname) returns IP in 8 HEX digits or "" if failed|undifined*/
static char *macwhichIP(char *arg)
{
   l_askNbytes(get_string(arg),d2cGETIP,0,8);
   if(!s_scmp(arg, "00000000"))
     return s_let("",arg);/*The client returns 0 IP if not defined*/
   {int i;
    char *ptr=arg;
    char ip[9];

      ip[8]='\0';
      s_let(arg,ip);
      for(i=0; i<8; i+=2){
         long2str(ptr,hex2int(ip+i,2));
         while(*ptr!='\0')ptr++;
         if(i!=6)
            *(ptr++)='.';
      }/*for(i=0; i<8; i+=2)*/
   }
   return arg;
}/*macwhichIP*/

/*\whichPID(jobname) returns PID as a HEX number or "" if failed|undifined:*/
static char *macwhichPID(char *arg)
{

 l_askNbytes(get_string(arg),d2cGETPID,0,-1);
 /*Now in arg there is HEX pid*/
 return long2str(arg,hex2int(arg,s_len(arg)));
}/*macwhichPID*/

/*\getnametostick(jobname) returns the name of the "master" job to
which the specified "sticky" job jobname has to stick. If fails,
returns an empty string.*/
static char *macgetnametostick(char *arg)
{
  return l_askNbytes(get_string(arg),d2cGETNAME2STICK,0,-1);
}/*macgetnametostick*/

/*\jobtime(jobname) returns 8 HEX digits, running time in millisecs:*/
static char *macjobtime(char *arg)
{
   return long2str(arg,hex2int(l_askNbytes(get_string(arg),d2cGETTIME,0,8),8));
}/*macjobtime*/

/*\jobstime(jobname) returns 8 HEX digits, starting time in millisecs:*/
static char *macjobstime(char *arg)
{
   return long2str(arg,hex2int(l_askNbytes(get_string(arg),d2cGETsTIME,0,8),8));
}/*macjobstime*/

/*\jobftime(jobname) returns 8 HEX digits, finishing time in millisecs:*/
static char *macjobftime(char *arg)
{
   return long2str(arg,hex2int(l_askNbytes(get_string(arg),d2cGETfTIME,0,8),8));
}/*macjobftime*/

/*\jobstatus(jobname) returns 8 HEX digits, see a comment to 2pFIN, rproto.h*/
static char *macjobstatus(char *arg)
{
   return l_askNbytes(get_string(arg),d2cGETST,0,8);
}/*macjobstatus*/

/*\jobhits(jobname) returns 6 HEX digits, hitTimeout, hitFail ,placement
  (JOBS_Q 1, SYNCH_JOBS_Q 2, FINISHED_JOBS_Q 3, FAILED_JOBS_Q 4, RUNNING_JOBS_Q 5,
    PIPELINE_Q 6, STICKY_PIPELINE_Q 7):*/
static char *macjobhits(char *arg)
{
   return l_askNbytes(get_string(arg),d2cGETHITS,0,6);
}/*macjobhits*/

/*\failedN() returns number of failed jobs, or an empty string, if fails*/
static char *macfailedN(char *arg)
{
*arg='\0';/*ask l_askNbytes do NOT send a command to the client*/
   return long2str(arg,hex2int(l_askNbytes(arg,d2cGETFAILQN,0,8),8));
}/*macfailedN*/

static int l_sendAnumber(char *cmd,int n,int w,char *arg)
{
mysighandler_t oldPIPE;
char tmp[9];
int ret=-1;

   /* if compiler fails here, try to change the definition of
      mysighandler_t on the beginning of the file types.h*/
   oldPIPE=signal(SIGPIPE,SIG_IGN);
  if(g_toserver<0) goto sendAnumberFails;
  if( writexactly(g_toserver,cmd,2) !=0 )
     goto sendAnumberFails;

  if(w<1)
     w=hexwide(n);

  if( writexactly(g_toserver,int2hex(tmp,n,w),w) !=0  )
     goto sendAnumberFails;
  /* And now, restore old signals:*/
  signal(SIGPIPE,oldPIPE);

  ret=0;

  sendAnumberFails:
     /* And now, restore old signals:*/
     signal(SIGPIPE,oldPIPE);
     return ret;
}/*l_sendAnumber*/

/*\clearjobs() clears all jobs. Returns an empty string on success,
or the string "Client is died."*/
static char *macclearjobs(char *arg)
{
mysighandler_t oldPIPE;

   free_mem(&g_lastjobname);
   g_jobscounter=0;

   s_let(CLIENTISDIED,arg);
   if(g_toserver<1)
      return arg;
   /* if compiler fails here, try to change the definition of
      mysighandler_t on the beginning of the file types.h*/
   oldPIPE=signal(SIGPIPE,SIG_IGN);
   if( writexactly(g_toserver,d2cFINISH,2) ==0 )
      s_let("",arg);
  /* And now, restore old signals:*/
  signal(SIGPIPE,oldPIPE);
   return arg;
}/*macclearjobs*/

extern long int g_startjob_timeout;

/*\startjobtimeout(#) sets timeout to start a job (millisecs). After timeout is expired,
  if the job is not running, it will be moved to another handler, if
  a total number of timeout hits < rexec.c::MAX_HIT_START_TIMEOUT.
  If >, the job fails. Time accounted starting from the moment of
  the job is send to the remote (or local) server.
     Returns previous value of the timeout, or empty string if fails.
*/
static char *macstartjobtimeout(char *arg)
{
long int to=g_startjob_timeout;
   g_startjob_timeout=get_num("startjobtimeout",arg);
   if( l_sendAnumber(d2cSETSTART_TO,g_startjob_timeout,8,arg) )
      return s_let("",arg);
   return long2str(arg,to);
}/*macstartjobtimeout*/

extern long int g_connect_timeout;

/*\connecttimeout(#) sets timeout to connect with a remote server (millisecs).
  Returns previous value of the timeout, or empty string if fails.
*/
static char *macgconnecttimeout(char *arg)
{
long int to=g_connect_timeout;
   g_connect_timeout=get_num("connecttimeout",arg);
   if( l_sendAnumber(d2cSETCONNECT_TO,g_connect_timeout,8,arg) )
      return s_let("",arg);
   return long2str(arg,to);
}/*macgconnecttimeout*/

/*\hex2sig(sig) converts DIANA's internal signals hexadecimal representation
to local system ones (decimal numbers).*/
static char *machex2sig(char *arg)
{
   return long2str(arg,int2sig(hex2int(get_string(arg),-1)));
}/*machex2sig*/

/*\sendsig(jobname,signal)
Returns an empty string on sucess; on fail returns:
"10" - job is not run' by a the client; "20" - can't read a signal from Diana;
<10:
          01 - An invalid signal was specified.
          02 - The pid or process group does not exist.
          03 - The  process  does  not have permission to send the signal
          04 - Unknown error.
*/
static char *macsendsig(char *arg)
{
char thesig[4];
int l;
   if(reduce_signal(get_string(arg))!=NULL )
      s_let(arg,thesig);
   else
      halt(INVSIG,NULL);
   l=s_len(get_string(arg));/*A job name*/
   l_askNbytes(s_cat(arg,arg,thesig),d2cSENDSIG,l,2);
   /*On success, arg=="00" - convert it to "":*/
   if(   (*arg == '0')&& (arg[1] == '0') )
      *arg='\0';
   return arg;
}/*macsendsig*/

/*\tracejobson(filename) - starts jobs tracing into files (filename.var filename.nam),
   returns an empty string. If client is not running, nothig happen. If tracing is already
   in progress, nothing happen. Actually, tracing will start at first \_exec(). If
   one \_exec() is already occured, nothing happen.

  For each newly started job, the following line will be placed in the
  file filename.nam:
    No Name Cmdline
  Here No is the order number of the job,
  Name is the job name,
  Cmdline is a  structure:"XXXXxxx...XXxxx... ..."
  where first two characters XX are the HEX representation of the number of arguments (n),
  and the rest is the  repeated n times construction XXxxx..., where first two
  characters XX are the HEX representation of the length of an argument, and xxx... is
  the argument. Example: "ls -l" will be represented as "0202ls02-l"

  The file filename.var contains one line per each job:
  X XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XX XX XX XX XXXXXXXX
  where space-separated fields are:
  X(1)        - HEX number, current placement:
                 1 - in the initial asynchronous queue,
                 2 - in the initial synchronous queue,
                 3 - finished jobs.
                 4 - failed jobs,
                 5 - running jobs,
                 6 - non-sticky pipeline,
                 7 - sticky pipeline.
  XXXXXXXX(8) - return code, 8 HEX digits:
                XX  : if normal exit, then the first two digits repersents exit()
                      argument (in HEX notation), otherwise 00,
                XX  : 00 - notmal exit, 01 - non-caught signal, 02 - the job was not run,
                      03 - no such a job, 04 - job is finished, but status is lost (often
                      occur after debugger detached), otherwise - ff,
                XXXX: - signal number (if previous was 01), otherwise - 0000. If signal
                   is unknown for Diana (see int2sig.c), it will be  0000.
  XXXXXXXX(8) - IP  (in HEX notation).
  XXXXXXXX(8) - PID (in HEX notation).
  XXXXXXXX(8) - attributes ("type") - HEX representation of binary flags
  XX(2)       - (HEX) timeout hits - how many times the job failed due to a start timeout
  XX(2)       - (HEX) fail hits - how many times the job fails (except start timeouts).
  XX(2)       - (HEX) max fail hits - maximal number of job retarts.
  XX(2)       - (HEX) errorlevel, or "-1", or "-2".
  XXXXXXXX(8) - (HEX) address of a corresponding line in the file filename.nam.
*/
static char *mactracejobson(char *arg)
{
   if(g_jobinfovar!=NULL)
      free(g_jobinfovar);
   g_jobinfovar=new_str(get_string(arg));/*Store the name*/
   g_jobinfovar_d=-1;/*Clear possible flag raised by \tracejobsoff()*/
   *arg='\0';
   return arg;
}/*mactracejobson*/

static char *mactracejobsoff(char *arg)
{
   if(g_jobinfovar!=NULL)/*The command was not sent to the client yet*/
      free_mem(&g_jobinfovar);/*clear name allocated by \tracejobson()*/
   else/*No trace or the command is already sent*/
     g_jobinfovar_d=1;/*Raise the flag*/
   *arg='\0';

   return arg;
}/*mactracejobsoff*/

/*\pad(str,len) formats str to fit a field of length len.
If len >0, then the text in the field will be right
justified and padded on the left with blanks. If len<0, then the text
in the field will be left justified and padded on the right with blanks.
If the length of str is larger than len, then str will be truncated.
*/
static char *macpad(char *arg)
{
int i,al,sl,l=get_num("pad",arg);
char *ptr=arg,*buf;
   *arg='\0';
   if(  (al=abs(l)) > MAX_STR_LEN-10  )
      halt(TOOLONGSTRING,NULL);
   if( l == 0 )
      return arg;
   sl=s_len(buf=new_str(get_string(arg)));
   if(sl>al)
      sl=al;

   if(l>0){/*right justification, left padding*/
      for(i=al-sl;i>0;i--)
         *ptr++=' ';
      s_letn(buf,ptr,sl+1);
   }else{/*left justification, right padding*/
      s_letn(buf,ptr,sl+1);
      ptr+=sl;
      for(i=al-sl;i>0;i--)
         *ptr++=' ';
      *ptr='\0';
   }/*if(l>0) ... else*/
   free(buf);
   return arg;
}/*macpad*/

/*
\getJobInfo(jobn, format) returns information about a job number
jobn, formatted according to format. The latter is a string of
type n1/w1:...nn/wn' where n# is a field number and w#
is its width.  If w# >0, then the text in the field will be right
justified and padded on the left with blanks. If w#<0, then the text
in the field will be left justified and padded on the right with blanks.
If w#=0, then the width of the field will be of the text length.
Here is a complete list of all available fields:
1 current placement, see above;
2 return code, 8 hexadecimal digits, see above;
3 IP address (in dot notation);
4 PID (decimal number);
5 attributes (decimal integer);
6 timeout hits (decimal number), see above;
7 fail hits (decimal number), see above;
8 max fail hits (decimal number), see above;
9 errorlevel (decimal number), see above;
10 the job name;
11 number of command line arguments;
12 all command line arguments startind from a second one (spaces separated);
13 all command line arguments starting from a third one (spaces separated);
14 all command line arguments starting from a fourth one (spaces separated).
If the number is zero or negative, then its absolute value treats of a single
command line argument. Example:
\getJobInfo(1, 4/10:3/15)
returns PID of the external program of the first job and IP address of the
computer on which the job runs. PID will be returned as a decimal number
padded by spaces on the left to fit a field of width 10, and IP will be
returned in dot notation padded by spaces on the left to fit a field of
width 15.
*/
static char *macgetJobInfo(char *arg)
{
int l;

   l=s_len(get_string(arg));
   if(l+10>MAX_STR_LEN){
      get_num("getJobInfo",arg);
      *arg='\0';
      return arg;
   }/*if(l+10>MAX_STR_LEN)*/

   int2hex(arg+l,get_num("getJobInfo",arg+l),8);

   return l_askNbytes(arg,d2cGETJINFO,l,-1);
}/*macgetJobInfo*/

/*\_exec(name,attr,par) -  attr - 32 symbols corresponding exec attribytes,
  see comment to \_execattr() above.

    name is the job name, it must be unique. If it is empty, the name will
 be assigned automatically.

     Arguments must be placed into the stack by means of \push() operator.
     The first must be \push(\eof()), then the command to execute, and then the
     arguments to the command.
  On success, returns the empty string "". On error, returns some diagnostics.
 */
static char *mac_exec(char *arg)
{
int i,l,k,n=0,ret=0,npar;
char **args=NULL;
char *jobname=NULL;
EXECATTR execattr;
mysighandler_t oldPIPE;
   /* if compiler fails here, try to change the definition of
      mysighandler_t on the beginning of the file types.h*/
   oldPIPE=signal(SIGPIPE,SIG_IGN);

   npar=l_do_execattr(0,memcpy(&execattr, &g_execattr, sizeof(EXECATTR)),arg)+1;

   args=get_mem(stackdepth+1,sizeof(char*));
   for(; stackdepth>0;n++){
      popstring(arg);
      stackdepth--;
      if (*arg == chEOF)break;
      args[n]=new_str(arg);
   }/*for(; stackdepth>0;n++)*/

   if (*arg == chEOF)
      i=2;
   else
      i=1;

   if(n<i)
      halt(STACKUNDERFLOW,NULL);

   /*Twice: first - prg name and second - argc[0]:*/
   args[n]=new_str(args[n-1]);
   n++;

   /*Set the value ret before operations which may fail:*/
   ret=1;/*Can't start a client*/

   if(g_pipetoclient>-1){/*Client is started, but not activated*/
      if(activateClient(g_pipetoclient))goto macexec_fails;
   }else if(g_toserver<0){/*Client is not started yet*/
      g_pipetoclient=1;/*Start it and activate*/
      if(startclient(&g_pipetoclient)<1)goto macexec_fails;
   }/*if(g_pipetoclient>-1)...else*/

   if(g_jobinfovar!=NULL){/*Send a command 'start tracing' to the cleint:*/
      l_askNbytes(g_jobinfovar,d2cSTARTTRACE,0,0);
      free_mem(&g_jobinfovar);
   }/*if(g_jobinfovar!=NULL)*/

   if(g_jobinfovar_d>-1){/*Send a command 'stop tracing' to the cleint:*/
     writexactly(g_toserver,d2cSTOPTRACE,2);
     g_jobinfovar_d=-1;
   }/*if(g_jobinfovar_d>-1)*/

   /*From this point only client error may occur.*/
   ret=2;/*Can't write to client*/
   if( writexactly(g_toserver,d2cNEWJOB,2) !=0)goto macexec_fails;

   ret=3;/*job name is too long.*/
   /*Get a job name:*/
   if(*get_string(arg)== '\0'){/*Job name is empty, assign the job name automatically:*/
      /*g_jobscounter as a job name:*/
      jobname=new_str(int2hex(arg,g_jobscounter+1,(l=hexwide(g_jobscounter+1))));
   }else{
      l=s_len(jobname=new_str(arg));
   }

   if(l>255){/*Fail, job name is too long.*/
      /*Reading zero length for a job name, client gives up the job:*/
      writexactly(g_toserver,"00",2);
      goto macexec_fails;
   }/*if(l>255)*/

   ret=4;/*Can't write to client*/
   /*Send the job name length:*/
   if( writexactly(g_toserver,int2hex(arg,l,2),2) !=0)goto macexec_fails;
   /*Send the job name:*/
   if( writexactly(g_toserver,jobname,l) !=0)goto macexec_fails;
   /*Job name is sent!*/

   /*Send a status:*/
   if( writexactly(g_toserver,int2hex(arg,execattr.attr,8),8) !=0)goto macexec_fails;

   if( execattr.attr & STICKY_JOB_T){
      /*Sticky job, send a name to stick to:*/
      char *nametostick=execattr.par[STICKY_JOB_I];

      ret=5;/*Wrong sticky name*/

      if(*nametostick=='\0')/*Default is the last job name*/
         nametostick=g_lastjobname;

      if(nametostick == NULL)/*No name to stick!*/
         goto macexec_fails;

      l=s_len(nametostick);

      ret=4;/*Can't write to client*/

      /*Send the length:*/
      if( writexactly(g_toserver,int2hex(arg,l,2),2)  !=0)goto macexec_fails;
      /*Send the name*/
      if( writexactly(g_toserver,nametostick,l) !=0)goto macexec_fails;
   }/*if( g_execattr & STICKY_JOB_T)*/

   if( execattr.attr & ERRORLEVEL_JOB_T){
      /*Send errorlevel(signed HEX of width2):*/
      if(*dec2hex(execattr.par[ERRORLEVEL_JOB_I],arg,2) == '\0')/*Default!*/
         s_let(ERRORLEVEL_JOB_D,arg);
      if( writexactly(g_toserver,arg,2)  !=0)goto macexec_fails;
   }/*if( execattr.attr & ERRORLEVEL_JOB_T)*/

   if( execattr.attr & RESTART_JOB_T){
      /*Send number of hits before unseccess(unsigded):*/
      if(*dec2hex(execattr.par[RESTART_JOB_I],arg,2) == '\0')/*Default!*/
         s_let(RESTART_JOB_D,arg);

      if(*arg == '-'){
         ret = 6;
         goto macexec_fails;
      }

      if( writexactly(g_toserver,arg,2)  !=0)goto macexec_fails;
   }/*if( execattr.attr & RESTART_JOB_T)*/

   /*Number of args:*/
   /*Put the number of args:*/
   if( writexactly(g_toserver,int2hex(arg,n,-2),2)!=0  )goto macexec_fails;

   /*Now - args:*/
   for(i=n-1; i>-1;i--){
      k=s_len(args[i]);
      /*Send the length:*/
      if( writexactly(g_toserver,int2hex(arg,k,-2),2)!=0  )goto macexec_fails;
      /*Send the name itself:*/
      if( writexactly(g_toserver,args[i],k)!=0  )goto macexec_fails;
      free(args[i]);args[i]=NULL;
   }/*for(i=n-1; i>-1;i--)*/
   /*Ready!*/

   ret=100;/*Client can't start a job:*/
   /*Wait for confirmation:*/
   if( (i=readHex(g_fromserver,arg,2, NULL,0)) != c2dJOBRUNNING_i )goto macexec_fails;
   /*Ok, the job is running*/
   /* Increment jobcounter independently on its usage as a job name:*/
   g_jobscounter++;
   *arg='\0';
   free(g_lastjobname);
   g_lastjobname=jobname;
   jobname=NULL;
   ret=1000;

   macexec_fails:
         /* And now, restore old signals:*/
         signal(SIGPIPE,oldPIPE);

         /*Some of execattr.par's may be allocated by l_do_execattr, must be cleaned:*/
         for(l=0; l<npar;l++)
            if(execattr.par[l]!=g_execattr.par[l])/*Modified!*/
               free_mem(execattr.par+l);

         if(args!=NULL){
            for(l=0; l<n;l++)
               free_mem(args+l);
            free(args);
         }/*if(args!=NULL)*/
         if(jobname!=NULL)
            free(jobname);
         if(ret < 3)/*"name" still is in the stack*/
            get_string(arg);

         switch(ret){
            case 1000:/**Success*/
               return arg;
            case 1:/*Can't start a client*/
               return s_let("Can't start a client",arg);
            case 2:case 4:/*Can't write to client*/
               return s_let("Can't write to client",arg);
            case 3:/*Too long job name*/
               return s_let("Too long job name",arg);
            case 5:/*Wrong sticky name*/
               return s_let(LASTJOBNAMEISUNDEFINED,arg);
            case 6:
                return s_let("Number of hits before fail must be non-negative!",arg);
            case 100:/*Client can't start a job:*/
               switch(i){
                  case c2dWRONGJOBNAME_i:/*Double job name*/
                     return s_let("Double job name",arg);
                  case c2dWRONGSTICKYNAME_i:/*Sticky job raquires unknown name*/
                     return s_let("Unknown sticky name",arg);
                  /*Sticky job reqires to stick to a job which already failed:*/
                  case c2dFAILDTOSTICK_i:
                     return s_let("Job to stick failed",arg);
                  default:/*Unknown*/
                     break;
               }/*switch(i)*/
               /*No break*/
            default:
               break;
         }/*switch(ret)*/
         return s_let("Unknown error",arg);
}/*mac_exec*/

/*\getip(host.domain) or \getip() - tries to find the current host IP,
if the argument is "", otherwise it determines the IP of named host:*/
static char *macgetip(char *arg)
{
   if(defip(get_string(arg),MAX_STR_LEN)==NULL)
      *arg='\0';
   return arg;
}/*macgetip*/

/*\wasMomentaTable() returns "yes" if at least one momenta comes from a table.
  Otherwise, returns "":*/
static char *macwasMomentaTable(char *arg)
{
   if(g_wasMomentaTable!=0)
       s_let("yes",arg);
   else
      *arg='\0';
   return arg;
}/*macwasMomentaTable*/
/*\wasAbstractMomenta() returns "yes" if at least one abstract momentum was saved
  during the last \saveTopologies(). Otherwise, returns "":*/
static char *macwasAbstractMomenta(char *arg)
{
   if(g_wasAbstractMomentum!=0)
      s_let("yes",arg);
   else
      *arg='\0';
   return arg;
}/*macwasAbstractMomenta*/

/************** End macro ******************/

/************* ATTENTION! ******************************
 The following operator serves only as temporary container of code to be debugged.
 This is no useful code and used only for debuggin:
 */
#ifdef TRY_CODE
 void try_code(void);/*Some function defined somewhere*/
 /*Operator \Try():*/
 static  char *macTry(char *arg) {try_code();return s_let("",arg); }
#endif

void macro_init(void)
{
int tmp;
  if (macro_table!=NULL) return;
  *from_expr=0;
  *end_expr=0;
  *to_expr=0;
  set_str2set(VALIDID, check_set);
#ifdef TRY_CODE
   register_macro("Try",0,0,macTry);
#endif
  register_macro("int2hex",2,isdebug,macint2hex);
  register_macro("hex2int",1,0,machex2int);

  register_macro("pingServer",1,0,macpingServer);
  register_macro("killServer",1,0,mackillServer);
  register_macro("rmjob",1,0,macrmjob);

  register_macro("whichIP",1,0,macwhichIP);
  register_macro("whichPID",1,0,macwhichPID);
  register_macro("getnametostick",1,0,macgetnametostick);

  register_macro("jobtime",1,0,macjobtime);
  register_macro("jobstime",1,0,macjobstime);
  register_macro("jobftime",1,0,macjobftime);
  register_macro("failedN",0,0,macfailedN);
  register_macro("jobhits",1,0,macjobhits);
  register_macro("jobstatus",1,0,macjobstatus);
  register_macro("lastjobname",0,isdebug,maclastjobname);

  register_macro("killServers",0,0,mackillServers);
  register_macro("clearjobs",0,0,macclearjobs);

  register_macro("lastjobnumber",0,0,maclastjobnumber);
  register_macro("startjobtimeout",1,isdebug,macstartjobtimeout);
  register_macro("connecttimeout",1,isdebug,macgconnecttimeout);
  register_macro("sendsig",2,isdebug,macsendsig);
  register_macro("hex2sig",1,0,machex2sig);

  register_macro("tracejobson",1,0,mactracejobson);
  register_macro("tracejobsoff",0,0,mactracejobsoff);

  register_macro("getJobInfo",2,isdebug,macgetJobInfo);

  register_macro("_execattr",2,isdebug,mac_execattr);
  register_macro("_exec",3,isdebug,mac_exec);

  register_macro("getip",1,0,macgetip);

  register_macro("_waitall",1,isdebug,mac_waitall);
  register_macro("getenv",1,0,macgetenv);
  register_macro("random",2,isdebug,macrandom);
  register_macro("version",0,0,macversion);
  register_macro("qsort",2,isdebug,macqsort);
  register_macro("fullcmdline",0,0,macfullcmdline);
  register_macro("syscmdparam",1,isdebug,macsyscmdparam);
  register_macro("clockreset",0,0,macclockreset);
  register_macro("clockstart",0,0,macclockstart);
  register_macro("clockstop",0,0,macclockstop);
  register_macro("clocksetactive",1,isdebug,macclocksetactive);
  register_macro("subst2dir",1,0,macsubst2dir);
  register_macro("tr",3,0,mactr);
  register_macro("cut",2,isdebug,maccut);
  register_macro("delete",2,0,macdelete);
  register_macro("parseParticleImage",2,isdebug,macparseParticleImage);
  register_macro("reducetopologycanonical",2,isdebug,
                                               macreducetopologycanonical);
  register_macro("reducetopology",2,isdebug,macreducetopology);
  register_macro("modesave",0,isdebug,macmodesave);
  register_macro("moderestore",0,isdebug,macmoderestore);
  register_macro("_modesave",0,isdebug,mac_modesave);
  register_macro("_moderestore",0,isdebug,mac_moderestore);
  register_macro("check",1,0,maccheck);
  register_macro("symbolpresent",1,0,macsymbolpresent);
  register_macro("setcheck",1,0,macsetcheck);
  register_macro("getcheck",0,0,macgetcheck);
  register_macro("compile",1,isdebug,maccompile);
  register_macro("backslash",0,0,macbackslash);
  register_macro("scannerinit",3,isdebug,macscannerinit);
  register_macro("scannerdone",0,isdebug,macscannerdone);
  register_macro("getscanline",0,isdebug,macgetscanline);
  register_macro("gettoken",0,isdebug,macgettoken);
  register_macro("call",1,isdebug,maccall);
  register_macro("push",1,0,macpush);
  register_macro("pop",0,isdebug,macpop);
  register_macro("replace",3,isdebug,macreplace);
  register_macro("len",1,0,maclen);
  register_macro("pos",2,0,macpos);
  register_macro("howmany",2,0,machowmany);
  register_macro("copy",3,isdebug,maccopy);
  register_macro("eol",0,0,maceol);
  register_macro("eof",0,0,maceof);
  register_macro("let",2,0,maclet);
  register_macro("clear",0,0,macclear);
  register_macro("export",2,0,macexport);
  register_macro("free",0,0,macfree);
  register_macro("get",1,isdebug,macget);
  register_macro("pad",2,isdebug,macpad);

  tmp=isdebug;
  set_bit(&tmp,bitLABELSUPDATE);
  register_macro("goto",1,tmp,macgoto);
  register_macro("relabel",2,tmp,macrelabel);
  register_macro("rmlabel",1,tmp,macrmlabel);
  register_macro("islabelexist",1,tmp,macislabelexist);

  register_macro("import",1,isdebug,macimport);
  register_macro("exist",1,0,macexist);
  register_macro("numcmp",2,isdebug,macnumcmp);
  register_macro("sum",2,isdebug,macsum);
  register_macro("sub",2,isdebug,macsub);
  register_macro("mul",2,isdebug,macmul);
  register_macro("div",2,isdebug,macdiv);
  register_macro("mod",2,isdebug,macmod);
  register_macro("bitwise",3,isdebug,macbitwise);
  register_macro("suspendout",0,isdebug,macsuspendout);
  register_macro("restoreout",0,isdebug,macrestoreout);
  register_macro("setout",1,isdebug,macsetout);
  register_macro("write",2,0,macwrite);
  register_macro("appendout",1,isdebug,macappendout);
  register_macro("flush",0,isdebug,macflush);
  register_macro("writetofile",3,isdebug,macwritetofile);
  register_macro("blank",1,0,macblank);
  register_macro("pipe",1,0,macpipe);
  register_macro("setpipeforwait",1,isdebug,macsetpipeforwait);
  register_macro("getpipeforwait",0,0,macgetpipeforwait);
  register_macro("checkpipe",2,isdebug,maccheckpipe);
  register_macro("closepipe",1,isdebug,macclosepipe);
  register_macro("ispipeready",1,isdebug,macispipeready);
  register_macro("frompipe",1,isdebug,macfrompipe);
  register_macro("linefrompipe",1,isdebug,maclinefrompipe);
  register_macro("topipe",2,isdebug,mactopipe);
  register_macro("linetopipe",2,isdebug,maclinetopipe);

  register_macro("asksystem",2,0, macasksystem);
  register_macro("system",1,0,macsystem);
  register_macro("read",1,0,macread);
  register_macro("traceOn",0,0,mactraceOn);
  register_macro("exit",1,0,macexit);
  register_macro("runerror",1,isdebug,macrunerror);
  register_macro("cmdline",1,isdebug,maccmdline);
  register_macro("getstr",1,isdebug,macgetstr);
  register_macro("count",1,0,maccount);
  register_macro("setfind",3,0,macsetfind);
  register_macro("find",2,isdebug,macfind);
  register_macro("killvar",1,0,mackillvar);
  register_macro("killexp",1,0,mackillexp);

  register_macro("nexp",0,0,macnexp);
  register_macro("nvar",0,0,macnvar);
  register_macro("sizeExp",0,0,macsizeExp);
  register_macro("sizeVar",0,0,macsizeVar);
  register_macro("resizeExp",1,isdebug,macresizeExp);
  register_macro("resizeVar",1,isdebug,macresizeVar);

  register_macro("open",1,0,macopen);
  register_macro("removefile",1,0,macremovefile);
  register_macro("readln",0,isdebug,macreadln);
  register_macro("message",1,0,macmessage);
  register_macro("return",1,0,macreturn);
  register_macro("obracket",0,0,macobracket);
  register_macro("cbracket",0,0,maccbracket);
  register_macro("inc",2,isdebug,macinc);
  register_macro("isinteger",1,0,macisinteger);
  register_macro("isfloat",1,0,macisfloat);
  register_macro("ttLoad",1,0,macttLoad);
  register_macro("ttSave",2,isdebug,macttSave);
  register_macro("ttDelete",1,isdebug,macttDelete);
  register_macro("ttHowmany",1,isdebug,macttHowmany);
  register_macro("ttOrigTopology",2,isdebug,macttOrigtopology);
  register_macro("ttRedTopology",2,isdebug,macttRedtopology);
  register_macro("ttTopology",2,isdebug,macttTopology);
  register_macro("ttNInternal",2,isdebug,macttNInternal);
  register_macro("ttExternal",2,isdebug,macttNExternal);
  register_macro("ttNVertex",2,isdebug,macttNVertex);
  register_macro("ttNMomenta",2,isdebug,macttNMomenta);
  register_macro("ttLookup",2,isdebug,macttLookup);
  register_macro("ttMomenta",3,isdebug,macttMomenta);
  register_macro("ttTranslate",3,isdebug,macttTranslate);
  register_macro("ttLookupTranslation",2,isdebug,macttLookupTranslation);
  register_macro("ttTranslateTokens",2,isdebug,macttTranslateTokens);
  register_macro("ttSetSep",1,0,macttSetSep);
  register_macro("ttGetSep",0,0,macttGetSep);
  register_macro("ttLinesReorder",3,isdebug,macttLinesReorder);
  register_macro("ttVertexReorder",3,isdebug,macttVertexReorder);
  register_macro("ttGetCoords",3,isdebug,macttGetCoords);
  register_macro("ttHowmanyRemarks",2,isdebug,macttHowmanyRemarks);
  register_macro("ttGetRemark",3,isdebug,macttGetRemark);
  register_macro("ttSetRemark",4,isdebug,macttSetRemark);
  register_macro("ttKillRemark",3,isdebug,macttKillRemark);
  register_macro("ttValueOfRemark",3,isdebug,macttValueOfRemark);
  register_macro("ttNameOfRemark",3,isdebug,macttNameOfRemark);
  register_macro("ttErrorCodes",1,0,macttErrorCodes);
  register_macro("ttNewTable",0,0,macttNewTable);
  register_macro("ttAppendToTable",3,isdebug,macttAppendToTable);
#ifdef CHECKMEM
  register_macro("freemem",0,0,macfreemem);
#endif

  if(!is_bit_set(&mode,bitONLYINTERPRET)){
     register_macro("coefficient",0,isdebug,maccoefficient);
     register_macro("trueprototype",0,isdebug,mactrueprototype);

     register_macro("rightspinor",1,isdebug,macrightspinor);
     register_macro("leftspinor",1,isdebug,macleftspinor);

     register_macro("createinput",0,isdebug,maccreateinput);
     register_macro("getformstr",1,isdebug,macgetformstr);
     register_macro("setmomenta",1,isdebug,macsetmomenta);
     register_macro("firstdiagramnumber",0,0,macfirstdiagramnumber);
     register_macro("lastdiagramnumber",0,0,maclastdiagramnumber);
     register_macro("currentdiagramnumber",0,isdebug,maccurrentdiagramnumber);
     register_macro("numberofvertex",0,isdebug,macnumberofvertex);
     register_macro("numberofinternallines",0,isdebug,macnumberofinternallines);
     register_macro("numberofexternallines",0,isdebug,macnumberofexternallines);
     register_macro("numberofid",0,isdebug,macnumberofid);
     register_macro("getid",1,isdebug,macgetid);
     register_macro("numberofoutterms",0,isdebug,macnumberofoutterms);
     register_macro("outterm",1,isdebug,macoutterm);
     register_macro("fulloutterm",1,isdebug,macfulloutterm);
     register_macro("posofoutterm",1,isdebug,macposofoutterm);
     register_macro("vloutterm",1,isdebug,macvloutterm);
     register_macro("vertexorline",1,isdebug,macvertexorline);
     register_macro("fermcont",2,isdebug,macfermcont);
     register_macro("fermescape",1,isdebug,macfermescape);
     register_macro("endline",2,isdebug,macendline);
     register_macro("fromvertex",1,isdebug,macfromvertex);
     register_macro("tovertex",1,isdebug,mactovertex);
     register_macro("vsubstitution",1,isdebug,macvsubstitution);
     register_macro("lsubstitution",1,isdebug,maclsubstitution);
     register_macro("invertline",1,isdebug,macinvertline);
     register_macro("isbrowser",0,0,macisbrowser);
     register_macro("islast",0,0,macislast);
     register_macro("prototypereset",1,isdebug,macprototypereset);
     register_macro("topologyidreset",1,isdebug,mactopologyidreset);
     register_macro("numberofmomentaset",0,isdebug,macnumberofmomentaset);
     register_macro("topologyisgotfrom",0,isdebug,mactopologyisgotfrom);
     register_macro("currentmomentaset",0,isdebug,maccurrentmomentaset);
     register_macro("numberoftopologyid",0,isdebug,macnumberoftopologyid);
     register_macro("currenttopologyid",0,isdebug,maccurrenttopologyid);
     register_macro("topologyid",0,isdebug,mactopologyid);
     register_macro("topology",0,isdebug,mactopology);

     register_macro("rmExtLeg",1,isdebug,macrmExtLeg);
     register_macro("amputatedTopology",1,isdebug,macamputatedTopology);
     register_macro("amputateTopology",2,isdebug,macamputateTopology);

     register_macro("ilsubst",1,isdebug,macilsubst);
     register_macro("ivsubst",1,isdebug,macivsubst);
     register_macro("setinternaltopology",2,isdebug,macsetinternaltopology);
     register_macro("canonicaltopology",0,isdebug,maccanonicaltopology);
     register_macro("whereleg",1,isdebug,macwhereleg);
 register_macro("internalpartoftopology",0,isdebug,macinternalpartoftopology);
 register_macro("externalpartoftopology",0,isdebug,macexternalpartoftopology);
     register_macro("i2u_line",1,isdebug, maci2u_line);
     register_macro("i2u_vertex",1,isdebug,maci2u_vertex);
     register_macro("i2u_sign",1,isdebug,maci2u_sign);
     register_macro("u2i_line",1,isdebug,macu2i_line);
     register_macro("u2i_vertex",1,isdebug,macu2i_vertex);
     register_macro("u2i_sign",1,isdebug,macu2i_sign);
     register_macro("prototype",0,isdebug,macprototype);
     register_macro("momentatext",1,isdebug,macmomentatext);
     register_macro("momentum",2,isdebug,macmomentum);
     register_macro("vectorOnLine",2,isdebug,macvectorOnLine);
     register_macro("externalmomentum",2,isdebug,macexternalmomentum);
     register_macro("extD2Q",1,isdebug,macextD2Q);
     register_macro("vertexInfo",2,isdebug,macvertexInfo);
     register_macro("isingoing",1,isdebug,macisingoing);
     register_macro("mass",1,isdebug,macmass);
     register_macro("fcountoutterm",1,isdebug,macfcountoutterm);
     register_macro("linefcount",1,isdebug,maclinefcount);
     register_macro("vertexfcount",1,isdebug,macvertexfcount);
     register_macro("maxfcount",0,isdebug,macmaxfcount);
     register_macro("linefflow",1,isdebug,maclinefflow);
     register_macro("vertexfflow",1,isdebug,macvertexfflow);
     register_macro("linetype",1,isdebug,maclinetype);
     register_macro("vertextype",1,isdebug,macvertextype);
     register_macro("particle",1,isdebug,macparticle);
     register_macro("beginpropagator",1,isdebug,macbeginpropagator);
     register_macro("skip",0,0,macskip);
     register_macro("antiparticle",1,0,macantiparticle);
     register_macro("reorderedprototype",1,isdebug,macreorderedprototype);
     register_macro("noncurrenttopologyid",1,isdebug,macnoncurrenttopologyid);
     register_macro("getPSarclabel",1,isdebug,macgetPSarclabel);
     register_macro("getPSvertexlabel",1,isdebug,macgetPSvertexlabel);
     register_macro("getPSvertex",1,isdebug,macgetPSvertex);
     register_macro("getPSarc",1,isdebug,macgetPSarc);
     register_macro("getPSInfoArc",1,isdebug,macgetPSInfoArc);
     register_macro("_PDiaImages",1,isdebug,mac_PDiaImages);
     register_macro("_PAllImages",1,isdebug,mac_PAllImages);
     register_macro("_PImage",2,isdebug,mac_PImage);
     register_macro("_PDocFonts",1,isdebug,mac_PDocFonts);
     register_macro("adjust",3,isdebug,macadjust);
     register_macro("initps",2,isdebug,macinitps);
     register_macro("getXfromY",2,isdebug,macgetXfromY);
     register_macro("thiscoordinatesok",0,0,macthiscoordinatesok);
     register_macro("coordinatesok",0,0,maccoordinatesok);
     register_macro("maxindexgroup",0,0,macmaxindexgroup);
     register_macro("getfirstusedindex",1,isdebug,macgetfirstusedindex);
     register_macro("getnextusedindex",1,isdebug,macgetnextusedindex);
     register_macro("getfirstindex",1,isdebug,macgetfirstindex);
     register_macro("getnextindex",1,isdebug,macgetnextindex);
     register_macro("getexternalindex",2,isdebug,macgetexternalindex);
     register_macro("numberofexternalindex",1,isdebug,macnumberofexternalindex);
     register_macro("linemarks",1,isdebug,maclinemarks);
     register_macro("vertexmarks",1,isdebug,macvertexmarks);
     register_macro("setlinemark",2,isdebug,macsetlinemark);
     register_macro("setvertexmark",2,isdebug,macsetvertexmark);
     register_macro("setcurrenttopology",1,isdebug,macsetcurrenttopology);
     register_macro("istopologyoccur",0,isdebug,macistopologyoccur);

     register_macro("valueOfTopolRem",1,isdebug,macvalueOfTopolRem);
     register_macro("nameOfTopolRem",1,isdebug,macnameOfTopolRem);
     register_macro("howmanyTopolRem",0,0,machowmanyTopolRem);

     register_macro("setTopolRem",2,isdebug,macsetTopolRem);
     register_macro("getTopolRem",1,isdebug,macgetTopolRem);
     register_macro("killTopolRem",1,isdebug,mackillTopolRem);
     register_macro("numberoftopologies",0,0,macnumberoftopologies);
     register_macro("currenttopologynumber",0,isdebug,maccurrenttopologynumber);

     register_macro("vectors",0,isdebug,macvectors);
     register_macro("isVectorPresent",1,0,macisVectorPresent);
     register_macro("saveIntTbl",1,isdebug,macsaveIntTbl);
     register_macro("loadIntTbl",1,isdebug,macloadIntTbl);
     register_macro("expTopLoopMark",1,isdebug,macexpTopLoopMark);
     register_macro("saveTopologies",1,isdebug,macsaveTopologies);
     register_macro("loadLoopMarks",1,isdebug,macloadLoopMarks);
     register_macro("allVectors",0,isdebug,macallVectors);
     register_macro("createMomentaTable",2,isdebug,maccreateMomentaTable);
     register_macro("dummyMomenta",1,isdebug,macdummyMomenta);
     register_macro("wasMomentaTable",0,0,macwasMomentaTable);
     register_macro("wasAbstractMomenta",0,0,macwasAbstractMomenta);

  }/*if(!is_bit_set(&mode,bitONLYINTERPRET))*/
}/*macro_init*/

void macro_done(void)
{
int i;
  if (macro_table!=NULL){
     hash_table_done(macro_table);
     macro_table=NULL;
  }
  free_mem(&usedindex_it);
  free_mem(&index_it);

  for(i=0; i<l_specialImagesTop;i++)
     free_mem(l_specialImages+i);
}/*macro_done*/
