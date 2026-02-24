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
/*tt_load.c*/

#define TT_LOAD_C 1

#include <stdlib.h>
#include <stdio.h>

/* !!! */
#include "comdef.h"
#include "tools.h"
#include "types.h"
#include "headers.h"
/* !!! */

#include "hash.h"

#include "tt_type.h"
#include "tt_gate.h"

#define TT_MAX_MOMENTA 127
#define TT_DELTA_MAX 10
#define TT_DELTA_TOPOLS_MAX 100

#ifndef NUM_STR_LEN
#define NUM_STR_LEN 15
#endif

#ifndef MAX_LINE_NUM
#define MAX_LINE_NUM 50
#endif

#ifndef MAX_STR_LEN
#define MAX_STR_LEN 1024000 /* old: 1024 */
#endif

/********************************************************************************
   Prefixes are: tt_ -- common routines used in all files,
   tg_ -- gates depending on implementation,
   l_ -- local auxiliary routines.
********************************************************************************/

/* Internal identifiers for finite automaton:*/
#define INIT_STATE 0
#define END_STATE 1
#define NUMBER_OF_TOPOLOGIES 2
#define TOPOLOGY_BEGIN 3
#define COORDINATES_BEGIN 4
#define START_READING_LINE 5
#define START_READING_MOMENTA 6
#define END_TOPOLOGY_ITSELF 7
#define CONTINUE_READING_MOMENTA 8
#define TRANSLATION 9
#define COORDINATES_EV 10
#define COORDINATES_EVL 11
#define COORDINATES_IV 12
#define COORDINATES_IVL 13
#define COORDINATES_EL 14
#define COORDINATES_ELL 15
#define COORDINATES_IL 16
#define COORDINATES_ILL 17
#define REMARKS_BEGIN 18
#define NUMBER_OF_TOKENS 19
#define MOMENTA_OR_SHAPE 20
unsigned int g_defaultTokenDictSize=TT_DICTSIZE;
unsigned int g_defaultHashTableSize=DEFAULT_HASH_SIZE;
/*g_bits_in_int must be 32, and g_bits_in_int_1 must be 31 on 32 bit systems:*/
extern int g_bits_in_int_1;
extern int g_bits_in_int;

extern int g_uniqueRemarksName;

char * tt_tokenTopology=TT_TOKENTOPOLOGY;
char * tt_tokenMoS=TT_TOKENMoS;
char * tt_tokenTMax=TT_TOKENTMAX;
char * tt_tokenTrMax=TT_TOKENTMAXTRANS;
char * tt_tokenCoordinates=TT_TOKENCOORDINATES;
char * tt_tokenEv = TT_TOKENEV;
char * tt_tokenEvl = TT_TOKENEVL;
char * tt_tokenIv = TT_TOKENIV;
char * tt_tokenIvl = TT_TOKENIVL;
char * tt_tokenEl = TT_TOKENEL;
char * tt_tokenEll = TT_TOKENELL;
char * tt_tokenIl = TT_TOKENIL;
char * tt_tokenIll = TT_TOKENILL;
char * tt_tokenTrans = TT_TOKENTRANS;
char * tt_tokenRemarks = TT_TOKENREMARKS;

/*tt_newVector is some external function to store a vector,if needed.
It is initialised by NULL, but the user may provide his non-trivial
function void tt_newVector(long n, char *vec): */
void (*tt_newVector)(long n, char *vec)=NULL;

/*ATTENTION! The following variable keeps the maximal number of topologies
  allowed to be loaded. It can be re-written via "numberoftopologies"
  directive in the table. It is taken into account only if the table is loaded
  from a file! If the table is created on-the-fly, this variable will never be checked!:*/
int tt_maxTopNumber=TT_MAX_TOP_N;

tt_table_type **tt_table=NULL;

/*array of pointers to tables is controlled by the following two variables:*/
int tt_top=0;/*top of the occupied array table. New table must be alocated as [top++]*/
int tt_max=0;/*Size of allocated array.*/

/*Concatenates s1 and s2 with memory rellocation for s1:*/
char *s_inc(char *s1, char *s2);

void l_freeSubstAndDirs(tt_singletopol_type *topology);

int tg_reduceTopology(tt_singletopol_type *topology);

void free_mem(void *block);
char *invertsubstitution(char *str, char *substitution);

static void l_empty_destructor(void **tag, void **cell)
{
}/*l_empty_destructor*/

/*Hash function for reduced topology:*/
static unsigned int l_red_hash(void *tag, unsigned int tsize)
{
register  unsigned int mem=0;
register  TT_INT i;
tt_singletopol_type *t=(tt_singletopol_type *)tag;

   for(i=0;i<t->nil;i++){
      mem=(mem<<1)|(mem>>g_bits_in_int_1);
      mem ^= (t->redTopol[i]).from;
      mem=(mem<<1)|(mem>>g_bits_in_int_1);
      mem ^= (t->redTopol[i]).to;
   }
   /*WARNING! "to" is absolutely necessary!!
     In very common case topologies differ only by exchanging external "to"
     vertices, e.g.
        (-4,3)(-3,2)(-2,1)(-1,1)(3,1)(1,4)(5,2)(6,2)(3,5)(3,6)(5,4)(6,4)
        (-4,2)(-3,2)(-2,1)(-1,1)(3,1)(1,4)(5,2)(6,2)(3,5)(3,6)(5,4)(6,4)
              ^^^^^^^^^^^^^^^^^^
              this fixes the vertex 1 and 2 so the relation (-4,2) does
              not fix the vertex 2. Then, the vertex 3 appears to be
              minimal changaeble and corresponds to -4 in upper topology
              after minimization.

      "from" is also needed for pure vertex topology...
   */
   for(i=0;i<t->nel;i++){
      mem=(mem<<1)|(mem>>g_bits_in_int_1);
      mem ^= (t->extPart[i]).from;
      mem=(mem<<1)|(mem>>g_bits_in_int_1);
      mem ^= (t->extPart[i]).to;

   }
   return(mem % tsize);
}/*l_red_hash*/

/*Comparison function for topologies:*/
static int l_red_cmp(void *tag1, void *tag2)
{
tt_singletopol_type *t1=(tt_singletopol_type *)tag1;
tt_singletopol_type *t2=(tt_singletopol_type *)tag2;
register  TT_INT i;
   if(
        (t1->nv != t2->nv) ||
        (t1->nil != t2->nil) ||
        (t1->nel != t2->nel)    )return 0;
   for (i=0; i<t1->nil; i++){
      if(
          ((t1->redTopol)[i].from != (t2->redTopol)[i].from)||
          ((t1->redTopol)[i].to != (t2->redTopol)[i].to)
      ) return 0;
   }
   for (i=0; i<t1->nel; i++){
      if(
         ((t1->extPart)[i].from != (t2->extPart)[i].from)||
         ((t1->extPart)[i].to != (t2->extPart)[i].to)
      ) return 0;
   }

   return(1);/*No difference*/
}/*l_red_cmp*/

void tt_clearSingleTopology(tt_singletopol_type *t)
{
int i,j;
  if(t==NULL)return;

  if(t->remarks!=NULL){
        j=t->top_remarks;
        for(i=0;i<j;i++){
              free_mem(&((t->remarks)[i].name));
              free_mem(&((t->remarks)[i].text));
        }/*for(i=0;i<j;i++)*/
        free_mem(&(t->remarks));
  }/*if(topologies[i].remarks!=NULL)*/

  free_mem(&(t->extPart));
  free_mem(&(t->usrTopol));
  free_mem(&(t->redTopol));

  /*Clear momenta:*/
  for(i=0;i<t->nmomenta; i++){

    if(  ( (t->extMomenta)!=NULL )  && ( (t->extMomenta)[i]!=NULL )){
       for(j=0; j<t->nel; j++)
          free_mem(&((t->extMomenta)[i][j]));
       free_mem(&((t->extMomenta)[i]));
    }

    if(  ( (t->momenta)!=NULL )  && ( (t->momenta)[i]!=NULL )){
       for(j=0; j<t->nil; j++)
          free_mem(&((t->momenta)[i][j]));
       free_mem(&((t->momenta)[i]));
    }

  }/*for(i=0;i<t->nmomenta; i++)*/

  free_mem(&(t->momenta));
  free_mem(&(t->extMomenta));

  free_mem(&(t->ext_vert));
  free_mem(&(t->ext_vlbl));
  free_mem(&(t->ext_line));
  free_mem(&(t->ext_llbl));
  free_mem(&(t->int_vert));
  free_mem(&(t->int_vlbl));
  free_mem(&(t->int_line));
  free_mem(&(t->int_llbl));

  free_mem(&(t->name));

  l_freeSubstAndDirs(t);

  free(t);
}/*tt_clearSingleTopology*/

static long l_do_deleteTable(tt_table_type *tmp)
{
long i;
   hash_table_done(tmp->hred);
   hash_table_done(tmp->htrans);
   for(i=0; i<tmp->totalN; i++){
         tt_clearSingleTopology((tmp->topols)[i]);
   }
   free_mem(&(tmp->topols));
   i=tmp->totalN;
   free(tmp);
   return i;
}/*l_do_deleteTable*/

/*Free table number n; returns a number of elements stored in the table, or error <0:*/
/*n starts from 1, not from 0!:*/
long tt_deleteTable(int n)
{
int ret;
   if( (n>tt_top )||(n<1)  )return TT_NOTFOUND;
   if(tt_table[--n]==NULL)return TT_NOTFOUND;
   ret=l_do_deleteTable(tt_table[n]);
   tt_table[n]=NULL;
   return ret;
}/*tt_deleteTable*/

/*l_getNpnSps and l_getNum are just auxiliary routines for the procedure tt_str2top*/
/*returns the pointer to first non-space chracter in str:*/
static char *l_getNpnSps(char *str)
{
   while( (*str!='\0') && (!(*str>' ')))str++;
   return str;
}/*l_getNpnSps*/

/* returns the pointer to the first character after the number, or NULL if fails.
   The number is (literally) stored in buf:*/
static char *l_getNum(char *str, char *buf)
{
int i=0,j=0;
char *mem=buf;
   if(  *(str=l_getNpnSps(str)) == '\0' )return NULL;
   if(  *str == '-' ){ *mem++='-';str++;j++;}
   if(  *(str=l_getNpnSps(str)) == '\0' )return NULL;

   for(
      i=1;
      ( ( !(*str<'0') ) && ( !(*str>'9') ) && (i<NUM_STR_LEN)  );
      i++,str=l_getNpnSps(++str)
      )
      *mem++=*str;

   *mem='\0';
   /*Now in buf[j] we must have a number*/
   if(  (buf[j]<'0') || (buf[j]>'9') )/*Fail!*/
      return NULL;

   return str;
}/*l_getNum*/

/*Just auxiliary routine:*/
static int l_allocateUsr(tt_line_type *top,tt_singletopol_type *topology)
{
int n=0,tmp;
   if(topology->nel > 0){
      if( (topology->extPart=(tt_line_type*)
             malloc(sizeof(tt_line_type)*topology->nel) )==NULL   ){
              topology->nel=topology->nil=topology->nv=0;
              return TT_NOTMEM;
      }
      for( n=0,tmp=topology->nel-1;!(tmp<0);tmp-- ,n++){
         (topology->extPart)[n].from=top[tmp].from;
         (topology->extPart)[n].to=top[tmp].to;
      }
   }/*if(topology->nel > 0)*/
   if(topology->nil>0){
      if( (topology->usrTopol=(tt_line_type*)
             malloc(sizeof(tt_line_type)*topology->nil) )==NULL   ){
              topology->nel=topology->nil=topology->nv=0;
              if(topology->nel > 0)
                 free_mem(&(topology->extPart));
              return TT_NOTMEM;
      }
      for(n=0;n<topology->nil;n++){
         tmp=n+topology->nel;
         (topology->usrTopol)[n].from=top[tmp].from;
         (topology->usrTopol)[n].to=top[tmp].to;
      }
   }/*if(topology->nil>0)*/

   return 0;
}/*l_allocateUsr*/

/*Expects line with a topology like(-2,2)(-1,1)(1,2)(1,2)
  and stores it in the "topology" which must be already allocated:*/
int tt_str2top(char *str,tt_singletopol_type *topology)
{

tt_line_type top[MAX_LINE_NUM];

   topology->nel=topology->nil=topology->nv=0;
   {/*block begin*/
   char buf[NUM_STR_LEN];
   int n=0,tmp;
      while(1){
         if(  *(str=l_getNpnSps(str))=='\0'){
            break;
         }else if(*str++ !='(')
            return TT_INVALID;
         if(  (str=l_getNum(str,buf))==NULL)
            return TT_INVALID;
         if( (tmp=atoi(buf))<0 ) topology->nel++;
         else
            topology->nil++;
         top[n].from=tmp;
         if(topology->nv < tmp )topology->nv = tmp;
         if(  *(str=l_getNpnSps(str))!=',')
            return TT_INVALID;
         if(  (str=l_getNum(++str,buf))==NULL  )
            return TT_INVALID;
         if(  !((tmp=atoi(buf))>0)  )
            return TT_INVALID;
         top[n++].to=tmp;
         if(topology->nv < tmp )topology->nv = tmp;
         if(  *(str=l_getNpnSps(str))!=')')
            return TT_INVALID;
         str++;
      }/*while(1)*/
      if(n==0)return TT_INVALID;
   }/*block end*/
   return l_allocateUsr(top,topology);
}/*tt_str2top*/

void l_freeSubstAndDirs(tt_singletopol_type *topology)
{
   free_mem(&(topology->l_dirUsr2Red));
   free_mem(&(topology->l_usr2red));
   free_mem(&(topology->l_red2usr));
   free_mem(&(topology->v_usr2red));
   free_mem(&(topology->v_red2usr));
   free_mem(&(topology->v_usr2nusr));
   free_mem(&(topology->v_nusr2usr));
   free_mem(&(topology->l_usr2nusr));
   free_mem(&(topology->l_nusr2usr));
   free_mem(&(topology->l_dirUsr2Nusr));
}/*l_freeSubstAndDirs*/

/*Does not perform actual reduction, but just allocates some structures
  and invokes the gate subroutine tg_reduceTopology. The latter performs an actual job:*/
int tt_reduceTopology(tt_singletopol_type *topology)
{
int nv=topology->nv+2,
    nl=topology->nil+2,
    i;
   /*Must use calloc, not malloc for possible rollback at fail:*/
   if(
      ( (topology->v_red2usr==NULL)&&
         ( (topology->v_red2usr=(TT_INT *)calloc(nv,sizeof(TT_INT)))==NULL )   )||
      ( (topology->v_usr2red==NULL)&&
         ( (topology->v_usr2red=(TT_INT *)calloc(nv,sizeof(TT_INT)))==NULL )   )||
      ( (topology->l_red2usr==NULL)&&
         ( (topology->l_red2usr=(TT_INT *)calloc(nl,sizeof(TT_INT)))==NULL )   )||
      ( (topology->l_usr2red==NULL)&&
         ( (topology->l_usr2red=(TT_INT *)calloc(nl,sizeof(TT_INT)))==NULL )   )||
      ( (topology->l_dirUsr2Red==NULL)&&
         ( (topology->l_dirUsr2Red=(char *)calloc(nl,sizeof(char)))==NULL )  )||
      ( (topology->v_nusr2usr==NULL)&&
         ( (topology->v_nusr2usr=(TT_INT *)calloc(nv,sizeof(TT_INT)))==NULL )  )||
      ( (topology->v_usr2nusr==NULL)&&
         ( (topology->v_usr2nusr=(TT_INT *)calloc(nv,sizeof(TT_INT)))==NULL )  )||
      ( (topology->l_nusr2usr==NULL)&&
         ( (topology->l_nusr2usr=(TT_INT *)calloc(nl,sizeof(TT_INT)))==NULL )  )||
      ( (topology->l_usr2nusr==NULL)&&
         ( (topology->l_usr2nusr=(TT_INT *)calloc(nl,sizeof(TT_INT)))==NULL )  )||
      ( (topology->l_dirUsr2Nusr==NULL)&&
         ( (topology->l_dirUsr2Nusr=(char *)calloc(nl,sizeof(char)))==NULL )  )
     ){
      l_freeSubstAndDirs(topology);
      return TT_NOTMEM;
   }
   *(topology->v_red2usr)= *(topology->v_usr2red)=
   *(topology->l_red2usr)= *(topology->l_usr2red)=

   *(topology->v_nusr2usr)= *(topology->v_usr2nusr)=
   *(topology->l_nusr2usr)= *(topology->l_usr2nusr)=-1;
   *(topology->l_dirUsr2Nusr)=*(topology->l_dirUsr2Red)=-1;
   nv--;
   /*nv==nv+1!*/
   for(i=1;i<nv;i++){
      (topology->v_red2usr)[i]=
      (topology->v_usr2red)[i]=
      (topology->v_nusr2usr)[i]=
      (topology->v_usr2nusr)[i]=i;
   }
   (topology->v_red2usr)[i]=
   (topology->v_usr2red)[i]=
   (topology->v_nusr2usr)[i]=
   (topology->v_usr2nusr)[i]=0;
   nl--;
   /*nl==nl+1!*/
   for(i=1;i<nl;i++){
      (topology->l_red2usr)[i]=
      (topology->l_usr2red)[i]=
      (topology->l_nusr2usr)[i]=
      (topology->l_usr2nusr)[i]=i;
      (topology->l_dirUsr2Red)[i]=
      (topology->l_dirUsr2Nusr)[i]='\1';
   }
   (topology->l_red2usr)[i]=
   (topology->l_usr2red)[i]=
   (topology->l_nusr2usr)[i]=
   (topology->l_usr2nusr)[i]=0;

   (topology->l_dirUsr2Red)[i]=
   (topology->l_dirUsr2Nusr)[i]='\0';

   if( (i=tg_reduceTopology(topology))<0 )
      l_freeSubstAndDirs(topology);/*Fail*/
   else{/*usr2red are ready, now generate invert substitutions:*/
      invertsubstitution(topology->l_red2usr, topology->l_usr2red);
      invertsubstitution(topology->v_red2usr,topology->v_usr2red);
   }
   return i;
}/*tt_reduceTopology*/

/* Allocates and initializes new single topology data:*/
tt_singletopol_type *tt_initSingleTopology(void)
{
tt_singletopol_type *tmp;

  if(
      ( tmp=(tt_singletopol_type *)
         calloc(1,
                sizeof(tt_singletopol_type)
         )
      )
        == NULL)
     return NULL;

  return tmp;
/* The following stuff is redundant:
   tmp->n=tmp->nel=tmp->nil=tmp->nv=0;
   tmp->nmomenta=0;
   tmp->extPart=tmp->usrTopol=tmp->redTopol=NULL;
   tmp->momenta=tmp->extMomenta=NULL;
   tmp->ext_vert=
   tmp->ext_vlbl=
   tmp->ext_line=
   tmp->ext_llbl=
   tmp->int_vert=
   tmp->int_vlbl=
   tmp->int_line=
   tmp->int_llbl=NULL;

   tmp->v_red2usr=
   tmp->l_red2usr=
   tmp->v_usr2red=
   tmp->l_usr2red=
   tmp->v_usr2nusr=
   tmp->v_nusr2usr=
   tmp->l_usr2nusr=
   tmp->l_nusr2usr=
   tmp->l_dirUsr2Red=
   tmp->l_dirUsr2Nusr=NULL;
   tmp->remarks=NULL;
   tmp->top_remarks=0;
   return tmp;
*/
}/*tt_initSingleTopology*/

/* If to == NULL, allocates and returns new topology identical to given one.
   If to != NULL, modifies existing "to".
   Touches fields according to mask "op", see tt_type.h.
   Newer touch field n!:*/
tt_singletopol_type *tt_copySingleTopology(
                        tt_singletopol_type *from,
                        tt_singletopol_type *to,
                        int op
                        )
{
int i,j,nv,nl,nel;
   if(from==NULL)return NULL;
   if(       (to==NULL)&&(  ( to=tt_initSingleTopology() )==NULL  )   )return NULL;
   /*Now to is ready to use*/

   /*+2 to allocate [0] element and trailing '\0':*/
   nv=from->nv+2;
   nl=from->nil+2;

   /*Copy the name:*/
   if( op & TT_NAME ){
      free_mem(&(to->name));
      to->name=new_str(from->name);
   }/*if( op & TT_NAME )*/

   /*Allocates substitutions:*/

   if(op & TT_S_RED_USR){
      /*Free old values, if present:*/
      free_mem(&(to->v_red2usr));
      free_mem(&(to->v_usr2red));
      free_mem(&(to->l_red2usr));
      free_mem(&(to->l_usr2red));

      if(
         ( (to->v_red2usr=(TT_INT *)malloc(sizeof(TT_INT)*nv))==NULL   )  ||
         ( (to->v_usr2red=(TT_INT *)malloc(sizeof(TT_INT)*nv))==NULL   )  ||
         ( (to->l_red2usr=(TT_INT *)malloc(sizeof(TT_INT)*nl))==NULL   )  ||
         ( (to->l_usr2red=(TT_INT *)malloc(sizeof(TT_INT)*nl))==NULL   )
      )goto fail_return;
      /*Fill up vertices sustitutions:*/
      for(i=0;i<nv;i++){
         (to->v_red2usr)[i]=(from->v_red2usr)[i];
         (to->v_usr2red)[i]=(from->v_usr2red)[i];
      }/*for(i=0;i<nv;i++)*/
      /*Fill up lines sustitutions:*/
      for(i=0;i<nl;i++){
         (to->l_red2usr)[i]=(from->l_red2usr)[i];
         (to->l_usr2red)[i]=(from->l_usr2red)[i];
      }/*for(i=0;i<nl;i++)*/
   }/*if(op&TT_S_RED_USR)*/

   if(op & TT_S_USR_NUSR){
      free_mem(&(to->v_nusr2usr));
      free_mem(&(to->v_usr2nusr));
      free_mem(&(to->l_nusr2usr));
      free_mem(&(to->l_usr2nusr));
      if(
         ( (to->v_nusr2usr=(TT_INT *)malloc(sizeof(TT_INT)*nv))==NULL  )  ||
         ( (to->v_usr2nusr=(TT_INT *)malloc(sizeof(TT_INT)*nv))==NULL  )  ||
         ( (to->l_nusr2usr=(TT_INT *)malloc(sizeof(TT_INT)*nl))==NULL  )  ||
         ( (to->l_usr2nusr=(TT_INT *)malloc(sizeof(TT_INT)*nl))==NULL  )
      )goto fail_return;
      /*Fill up vertices sustitutions:*/
      for(i=0;i<nv;i++){
         (to->v_nusr2usr)[i]=(from->v_nusr2usr)[i];
         (to->v_usr2nusr)[i]=(from->v_usr2nusr)[i];
      }/*for(i=0;i<nv;i++)*/
      /*Fill up lines sustitutions:*/
      for(i=0;i<nl;i++){
         (to->l_nusr2usr)[i]=(from->l_nusr2usr)[i];
         (to->l_usr2nusr)[i]=(from->l_usr2nusr)[i];
      }/*for(i=0;i<nl;i++)*/
   }/*if(op & TT_S_USR_NUSR)*/

   if(  op & TT_D_USR_RED){
      free_mem(&(to->l_dirUsr2Red));
      if( (to->l_dirUsr2Red=(char *)malloc(sizeof(TT_INT)*nl))==NULL  )goto fail_return;
      for(i=0;i<nl;i++)(to->l_dirUsr2Red)[i]=(from->l_dirUsr2Red)[i];
   }/*if(  op & TT_D_USR_RED)*/

   if(  op & TT_D_USR_NUSR){
      free_mem(&(to->l_dirUsr2Nusr));
      if( (to->l_dirUsr2Nusr=(char *)malloc(sizeof(TT_INT)*nl))==NULL  )goto fail_return;
      for(i=0;i<nl;i++)(to->l_dirUsr2Nusr)[i]=(from->l_dirUsr2Nusr)[i];
   }/*if(  op & TT_D_USR_NUSR)*/

   /*Were +=2, so decrease them before assigning:*/
   nl-=2;
   nv-=2;
   if(op & TT_NEL) to->nel=from->nel;
   if(op & TT_NIL) to->nil=from->nil;
   if(op & TT_NV) to->nv=from->nv;

   /* Start with coordinates:*/

   /*Now multiply all numbers by 2 to use them in coordinates accounting:*/
   nel=(from->nel)*2;
   nl*=2;
   nv*=2;

   if( op & TT_INT_COORD){

      /*Clear old values:*/
      free_mem(&(to->int_llbl));
      free_mem(&(to->int_line));
      free_mem(&(to->int_vlbl));
      free_mem(&(to->int_vert));

      if(nl>0){/*internal lines & labels*/
         if(from->int_llbl != NULL){/*internal line labels*/
            if(  (to->int_llbl=(double *)malloc(sizeof(double)*nl))==NULL  )
               goto fail_return;
            for(i=0; i<nl; i++)to->int_llbl[i]=from->int_llbl[i];
         }/*if(from->int_llbl != NULL)*/
         if(from->int_line != NULL){/*internal lines*/
            if(  (to->int_line=(double *)malloc(sizeof(double)*nl))==NULL  )
               goto fail_return;
            for(i=0; i<nl; i++)to->int_line[i]=from->int_line[i];
         }/*if(from->int_line != NULL)*/
      }/*if(nl>0)*/

      if(nv>0){/*vertices & labels*/
         if(from->int_vlbl != NULL){/*internal vertices labels*/
            if(  (to->int_vlbl=(double *)malloc(sizeof(double)*nv))==NULL  )
               goto fail_return;
            for(i=0; i<nv; i++)to->int_vlbl[i]=from->int_vlbl[i];
         }/*if(from->int_vlbl != NULL)*/
         if(from->int_vert != NULL){/*internal vertices*/
            if(  (to->int_vert=(double *)malloc(sizeof(double)*nv))==NULL  )
               goto fail_return;
            for(i=0; i<nv; i++)to->int_vert[i]=from->int_vert[i];
         }/*if(from->int_vert!= NULL)*/
      }/*if(nv>0)*/
   }/*if( op & TT_INT_COORD)*/

   if(op & TT_EXT_COORD){
      /*Clear old values*/
      free_mem(&(to->ext_llbl));
      free_mem(&(to->ext_line));
      free_mem(&(to->ext_vlbl));
      free_mem(&(to->ext_vert));

      if(nel>0){/*external lines & labels; vertices & labels*/
         if(from->ext_llbl != NULL){/*external line labels*/
            if(  (to->ext_llbl=(double *)malloc(sizeof(double)*nel))==NULL  )
               goto fail_return;
            for(i=0; i<nel; i++)to->ext_llbl[i]=from->ext_llbl[i];
         }/*if(from->ext_llbl != NULL)*/
         if(from->ext_line != NULL){/*external lines*/
            if(  (to->ext_line=(double *)malloc(sizeof(double)*nel))==NULL  )
               goto fail_return;
            for(i=0; i<nel; i++)to->ext_line[i]=from->ext_line[i];
         }/*if(from->ext_line != NULL)*/
         if(from->ext_vlbl != NULL){/*external vertices labels*/
            if(  (to->ext_vlbl=(double *)malloc(sizeof(double)*nel))==NULL  )
               goto fail_return;
            for(i=0; i<nel; i++)to->ext_vlbl[i]=from->ext_vlbl[i];
         }/*if(from->ext_vlbl != NULL)*/
         if(from->ext_vert != NULL){/*external vertices*/
            if(  (to->ext_vert=(double *)malloc(sizeof(double)*nel))==NULL  )
               goto fail_return;
            for(i=0; i<nel; i++)to->ext_vert[i]=from->ext_vert[i];
         }/*if(from->ext_vert != NULL)*/
      }/*if(nel>0)*/
   }/*if(op & TT_EXT_COORD)*/

   /*Restore actual values:*/
   nel=from->nel;
   nl=from->nil;
   nv=from->nv;
   /*Note, this is safety since we will use corresp. values only under proper op's*/

   /*Copy remarks:*/
   if(op & TT_REM){
      killTopolRem( &(to->top_remarks),&(to->remarks) );
      if(from->remarks!=NULL) /*copyTopolRem see "remarks.c"*/
         copyTopolRem( from->top_remarks,from->remarks,&(to->top_remarks),&(to->remarks) );
   }/*if(op & TT_REM)*/

   /*Copy momenta:*/

   /*External momenta -- optional:*/
   if(op & TT_EXT_MOMENTA){
      if(  to->extMomenta!=NULL  ){/*Clear old values*/
            for(i=0;i<to->nmomenta; i++)
               if(  (to->extMomenta)[i]!=NULL  ){
                  for(j=0; j<nel; j++)
                     free(  ( (to->extMomenta)[i] )[j] );
                  free(  (to->extMomenta)[i] );
               }/*if(  (to->extMomenta)[i]!=NULL  )*/
            free_mem(  &(to->extMomenta)   );
      }/*(  to->extMomenta!=NULL  )*/

      if( (from->extMomenta)!=NULL  ){
         for(i=0;i<from->nmomenta; i++)
            if(  (from->extMomenta)[i]!=NULL  ){

               if(to->extMomenta==NULL){/*Allocate external momenta*/
                  if(
                     (   to->extMomenta=(char ***)calloc( from->nmomenta,sizeof(char **) )   )
                     ==NULL
                  )goto fail_return;
               }/*if(to->extMomenta==NULL)*/

               if(
               (
                  (to->extMomenta)[i]=(char **)malloc( sizeof(char *)*nel )
               )==NULL
               )goto fail_return;
               for(j=0; j<nel; j++)
                  ( (to->extMomenta)[i] )[j]=new_str(  ( (from->extMomenta)[i] )[j]  );
            }/*if(  (from->extMomenta)[i]!=NULL  )*/
         }/*if( (from->extMomenta)!=NULL  )*/
      }/*if(op & TT_EXT_MOMENTA)*/

   /*Check internal momenta. They may absent in case of pure vertex*/
   if(op & TT_MOMENTA){

      if( to->momenta!=NULL ){/*Clear old values*/
         for(i=0;i<to->nmomenta; i++)
            if(  (to->momenta)[i]!=NULL  ){
               for(j=0; j<nl; j++)
                  free(  ( (to->momenta)[i] )[j] );
               free(  (to->momenta)[i] );
            }/*if(  (to->momenta)[i]!=NULL  )*/
         free_mem(  &(to->momenta)   );
      }/*if( to->momenta!=NULL )*/
      if(  (from->momenta)!=NULL  ){
         /*Allocate top level array:*/
         if(
          (   to->momenta=(char ***)calloc( from->nmomenta,sizeof(char **) )   )
          ==NULL
         )goto fail_return;
         /*Allocate and copy momenta texts:*/
         for(i=0;i<from->nmomenta; i++)
         if(  (from->momenta)[i]!=NULL  ){
            if(
            (
               (to->momenta)[i]=(char **)malloc( sizeof(char *)*nl )
            )==NULL
            )goto fail_return;
            for(j=0; j<nl; j++)
               ( (to->momenta)[i] )[j]=new_str(  ( (from->momenta)[i] )[j]  );
         }/*if(  (from->momenta)[i]!=NULL  )*/
      }/*if(  (from->momenta)!=NULL  )*/
   }/*if(op & TT_MOMENTA)*/

   /*Number of momenta:*/
   if( op & TT_NMOMENTA)
      to->nmomenta=from->nmomenta;

   /*momenta are ready*/

   /*Now start with topologies.*/

   if(op & TT_EXT){/*External part*/
      free_mem(  &(to->extPart)  );
      if(nel > 0){
         if( (to->extPart=(tt_line_type*)
             malloc(sizeof(tt_line_type)*nel) )==NULL   ) goto fail_return;

         for( i=0;i<nel;i++){
            (to->extPart)[i].from=(from->extPart)[i].from;
            (to->extPart)[i].to=(from->extPart)[i].to;
         }/*for( i=0;i<nel;i++)*/
      }/*if(nel > 0)   )*/
   }/*if(   (op & TT_EXT) && (nel > 0)   )*/

   if(op & TT_USR){/*Internal parts -- two topologies, usr and red.*/
      free_mem(  &(to->usrTopol)  );
      if(nl>0){
         if(from->usrTopol != NULL ){
            if( (to->usrTopol=(tt_line_type*)
               malloc(sizeof(tt_line_type)*nl) )==NULL   ) goto fail_return;

            for(i=0;i<nl;i++){
               (to->usrTopol)[i].from=(from->usrTopol)[i].from;
               (to->usrTopol)[i].to=(from->usrTopol)[i].to;
            }/*for(i=0;i<nl;i++)*/
         }/*if( from->usrTopol != NULL )*/
      }/*if(nl>0)*/
   }/*if(op & TT_USR)*/

   if(op & TT_RED){/*Internal parts -- two topologies, usr and red.*/
      free_mem(  &(to->redTopol)   );
      if(nl>0){
         if( from->redTopol != NULL ){

            if( (to->redTopol=(tt_line_type*)
               malloc(sizeof(tt_line_type)*nl) )==NULL   ) goto fail_return;

            for(i=0;i<nl;i++){
               (to->redTopol)[i].from=(from->redTopol)[i].from;
               (to->redTopol)[i].to=(from->redTopol)[i].to;
            }/*for(i=0;i<nl;i++)*/

         }/*if( from->redTopol != NULL )*/
      }/*if(nl>0)*/
   }/*if(op & TT_RED)*/

   /*Orderly return:*/
   return to;/*ATTENTION! to->n=0 (for a new allocated)!*/

   fail_return:
      tt_clearSingleTopology(to);
      return NULL;
}/*tt_copySingleTopology*/

/*Auxiliary routine, used only for loading table.
  ATTENTION!! In case of using it in a different way, NOTE: it does NOT check does
  the table exist, or not:*/
static int l_fillUpHashTable(int n)
{
tt_table_type *t=tt_table[n];
long i;
   if( (t->hred==NULL)/*Not initialized yet*/
       &&
       (   (t->hred=create_hash_table(
                 nextPrime(t->totalN),
                 l_red_hash,
                 l_red_cmp,
                 l_empty_destructor))==NULL  )
      )  return TT_NOTMEM;
   for(i=0; i<t->totalN; i++){
/*      if(  (t->topols[i])->redTopol == NULL )continue;*/
      switch( install(t->topols[i],t->topols[i],t->hred) ){
         case -1:
            return TT_NOTMEM;
         case 1:
            return TT_DOUBLETOPS;
         default:;
      }/*switch( install(t->topols[i],t->topols[i],t->hred) )*/
   }/*for(i=0; i<t->totalN; i++)*/
   return 0;
}/*l_fillUpHashTable*/

/* Allocates and initialises new table:*/
tt_table_type *tt_initTable(void)
{
tt_table_type *tmp;

  if(  (tmp=(tt_table_type *)malloc(sizeof(tt_table_type))) == NULL)
     return NULL;

   tmp->totalN=tmp->extN=tmp->intN=0;
   tmp->hred=tmp->htrans=NULL;
   tmp->topols=NULL;
   tmp->momentaOrShape=0;/*0 (default)--both, >0 -- only momenta, <0 -- only shape*/
   return tmp;
}/*tt_initTable*/

/* Hash table used by tt_loadTable, initialised once forever:*/
static HASH_TABLE l_tt=NULL;

/*Destructor -- frees all about tables:*/
void tt_load_done(void)
{
int ctop=0;

   if(l_tt!=NULL){
      hash_table_done(l_tt);
      l_tt=NULL;
   }
   for(;ctop<tt_top; ctop++)if(tt_table[ctop]!=NULL)
      l_do_deleteTable(tt_table[ctop]);
   free_mem(&tt_table);
   tt_top=tt_max=0;
}/*tt_load_done*/

/*Allocates and fills up array '*buf' by a successive indicex of all
  existing tables, and returns the number of tables placed into 'buf':*/
int tt_getAllTables(int **buf)
{
int i,*tmp;

  if(  ( tmp=calloc(tt_top,sizeof(int*)) )==NULL  )return TT_NOTMEM;
  *buf=tmp;
  for(i=0;i<tt_top; i++)if(tt_table[i]!=NULL)
     *tmp++=i;

  if( (i=tmp - *buf) == 0 ){/*No tables available*/  /*64!*/
     free(*buf);
     return 0;
  }/*if( (**buf=tmp - *buf) == 1 )*/
  if(   (*buf=realloc(*buf,i))==NULL  )return TT_NOTMEM;
  return i;
}/*tt_getAllTables*/

/*Creates new table and inserts it into the array tt_table, If tokenTableSize>0,
  initializes the token table. If topolTableSize>0, initializes the reduced topology
  table:*/
int tt_createNewTable(unsigned int tokenTableSize, unsigned int topolTableSize)
{
int ctop=0;
  /*Try to find unused cell in tt_table*/
  for(;ctop<tt_top; ctop++)if(tt_table[ctop]==NULL)break;/*found*/
  if(ctop==tt_top){/*no free cells*/
     if( !(tt_top<tt_max)  ){/*tt_table is exhausted -- try to realloc:*/
        if(
        ( tt_table=(tt_table_type**)realloc(
            tt_table,sizeof(tt_table_type*)*(tt_max+=TT_DELTA_MAX)
                                   )
        ) ==NULL
        ){
           tt_max-=TT_DELTA_MAX;
           return TT_NOTMEM;/* not memory!*/
        }
     }/*if(  !(tt_top<tt_max)  )*/
     tt_top++;
  }/*if(ctop==tt_top)*/
  if(   (tt_table[ctop]=tt_initTable())==NULL  )
     return TT_NOTMEM;/* not memory!*/
  /*Now in tt_table[ctop] we have a ready to use cell*/

  if(tokenTableSize!=0)
     if  (   (tt_table[ctop]->htrans=create_hash_table_with_copy(
                 tokenTableSize,
                 str_hash,
                 str_cmp,
                 c_destructor,
                 (CPY *)NEWSTR,
                 (CPY *)NEWSTR ))==NULL  ){
                    tt_deleteTable(ctop+1);/*+1 since user numbering starts from 1*/
                    return TT_NOTMEM;
     }/*if  (   (tt_table[ctop]->htrans=create_hash_table ...*/

  if(topolTableSize!=0)
     if  (   (tt_table[ctop]->hred=create_hash_table(
                 topolTableSize,
                 l_red_hash,
                 l_red_cmp,
                 l_empty_destructor))==NULL  ){
                    tt_deleteTable(ctop+1);/*+1 since user numbering starts from 1*/
                    return TT_NOTMEM;
     }

  return ctop+1;
}/*tt_createNewTable*/

/*Just auxiliary routine for tt_loadTable:*/
static void l_failLoadTable(tt_singletopol_type **topols,long top,int cnum)
{
   tt_table[cnum]->topols=topols;
   tt_table[cnum]->totalN=top;
   tt_deleteTable(cnum+1);/*user numbering starts from 1*/
}/*l_failLoadTable*/

/*Auxiliary routine checks the validity and resets table from user specified
  values to the system ones:*/
int tt_chktbl(int *table)
{
   if( (*table >tt_top )||(*table <1)  )return TT_NOTFOUND;
   if( tt_table[--(*table)]==NULL )return TT_NOTFOUND;
   return 0;
}/*tt_chktbl*/

/*Auxiliary routine checks the validity and resets (table,top) from user specified
  values to the system ones:*/
int tt_chktop(int *table, long *top)
{
   if( (*table >tt_top )||(*table <1)  )return TT_NOTFOUND;
   (*table)--;
   if( tt_table[*table]==NULL )return TT_NOTFOUND;
   if( (*top > tt_table[*table]->totalN )||(*top <1)  )return TT_NOTFOUND;
   (*top)--;
   return 0;
}/*tt_chktop*/

char *tt_top2str(char *buf,int nel,tt_line_type *extPart,int nil,tt_line_type *intPart)
{
   int i;
   char *tmp=buf;
      *buf='\0';
      for(i=nel-1;!(i<0);i--){
        sprintf(tmp,"(%d,%d)",extPart[i].from,extPart[i].to);
        while(*++tmp!='\0');
      }

      for(i=0;i<nil;i++){
        sprintf(tmp,"(%d,%d)",intPart[i].from,intPart[i].to);
        while(*++tmp!='\0');
      }
      return buf;
}/*tt_top2str*/

char *tt_getToken(tt_scannerType *s)
{
   return tg_scannerGetToken(s);
}/*tt_getToken*/

char *tt_getNonNullToken(tt_scannerType *s)
{
char *tmp=tg_scannerGetToken(s);
   if(tmp == NULL){
      *(s->state)=END_STATE;
      *(s->err)=TT_U_EOF;
   }
   return tmp;
}/*tt_getNonNullToken*/

void tt_scDone(tt_scannerType *s)
{
   if(s==NULL)return;
   tg_scannerDone(s);/*Must free s->extrainfo!*/
   free(s->buf);
   free(s->fname);
   free(s);
}/*tt_scDone*/

/* Allocates data for the scanner and initializes it using the gate procedure
   tg_scannerInit(sc). Parameters:
  fname -- a file name; max-- the length of allocated temporary buffer.
  On error, sets *state=END_STATE and *err= error code.*/
tt_scannerType *tt_scInit(char *fname,  int max, int *state, int *err)
{
   tt_scannerType *sc=(tt_scannerType *)malloc(sizeof(tt_scannerType));
      if(sc==NULL){
         *err=TT_NOTMEM;
         return NULL;
      }
      sc->fname=sc->buf=NULL;
      sc->state=state;
      sc->err=err;
      sc->max=max;
      sc->extrainfo=NULL;
      if(fname==NULL)
         sc->fname=NULL;
      else{
         int l;
         for(l=0; fname[l]!='\0'; l++);
         l+=2;
         if(   (sc->fname=(char *)malloc(l)) == NULL   ){
            tt_scDone(sc);*err=TT_NOTMEM;return NULL;
         }
         for(l=0; fname[l]!='\0'; l++)(sc->fname)[l]=fname[l];
         (sc->fname)[l]='\0';
      }
      if(   (sc->buf=(char *)malloc(max+1)) == NULL   ){
         tt_scDone(sc);*err=TT_NOTMEM;return NULL;
      }
      if(
         tg_scannerInit(sc)/*Must allocate s->extrainfo in success and return 0
                            If fails, must set sc->err properly. */
                          ){
         *err=*(sc->err);
         tt_scDone(sc);
         return NULL;
      }
      return sc;
}/*tt_scInit*/

/*Creates the  temporary hash table for topologies names.
  On error, sets *state=END_STATE and *err= error code.*/
static HASH_TABLE l_readHashInit(unsigned int theSize,int *state,int *err)
{
HASH_TABLE ht;
   if(theSize==0)theSize=1;
   if(    (ht=create_hash_table_with_copy(
                 theSize,
                 str_hash,
                 str_cmp,
                 int_destructor,
                 (CPY *)NEWSTR,
                 (CPY *)NEWSTR))==NULL  ){
      *err= TT_NOTMEM;
      *state=END_STATE;
   }
   return ht;
}/*l_readHashInit*/

/*Resizes the  hash table .
  On error, sets *state=END_STATE and *err= error code, and returns -1. On success,
  returns 0:*/
static int l_resizeHashInit(HASH_TABLE ht, unsigned int theSize,int *state,int *err)
{
   /* Adjust the size to a prime number:*/
   theSize=nextPrime(theSize);
   if(theSize==0)theSize=1;
   if(    resize_hash_table(
                 ht,
                 theSize
                 )  ){
      *err= TT_NOTMEM;
      *state=END_STATE;
      return -1;
   }
   return 0;
}/*l_resizeHashInit*/

/*ATTENTION!! If fails, then core dumped!!*/
long *newLong(long i)
{
long *tmp=(long *)malloc(sizeof(long));
   *tmp=i;
   return tmp;
}/*newLong*/
/*ATTENTION!! If fails, then core dumped!!*/
int *newInt(int i)
{
int *tmp=(int *)malloc(sizeof(int));
   *tmp=i;
   return tmp;
}/*newLong*/

static void l_translation(HASH_TABLE ht, int *state, int *err, tt_scannerType *sc)
{
char *theToken,*tmp;
   if(  (theToken=tt_getNonNullToken(sc))==NULL)return;
   tmp=new_str(theToken);
   if(  (theToken=tt_getNonNullToken(sc))==NULL){free(tmp);return;}
   if(*theToken!='='){free(tmp);*err=TT_INVALID;return;}

   if(  (theToken=tt_getNonNullToken(sc))==NULL){free(tmp);return;}
   if( *theToken=='"' ){
      char *res=new_str("");
      while((theToken=tt_getNonNullToken(sc))!=NULL){
         if(*theToken=='"')break;
         res=s_inc(res,theToken);
      }/*while((theToken=tt_getNonNullToken(sc))!=NULL)*/
      if(theToken==NULL){free(tmp);free(res);return;}
      theToken=res;
   }/*if( *theToken=='"' )*/

   if( install(tmp,new_str(theToken),ht) ){
      *err=TT_DOUBLEDEFTOKEN;
      *state=END_STATE;
      return;
   }
   *state=INIT_STATE;
}/*l_translation*/

static int l_momentaOrShape(int *state, int *err, tt_scannerType *sc)
{
   char *theToken;
      *state=INIT_STATE;
      if(  (theToken=tt_getNonNullToken(sc))==NULL)return 0;
      if(*theToken=='=')
          if(  (theToken=tt_getNonNullToken(sc))==NULL)return 0;

      if(s_scmp(theToken,TT_ONLYMOMENTA)==0) return 1;
      if(s_scmp(theToken,TT_ONLYSHAPE)==0) return -1;
      return 0;
}/*l_momentaOrShape*/

/*ht here is just the topologies name table!:*/
static void l_numberOfTopologies(HASH_TABLE *ht, int *state, int *err, tt_scannerType *sc)
{char *theToken;
 int tmp;
   if(  (theToken=tt_getNonNullToken(sc))==NULL)return;
   if(*theToken=='=')
      if(  (theToken=tt_getNonNullToken(sc))==NULL)return;

   if(sscanf(theToken,"%d",&tmp)!=1){
      *err=TT_FORMAT;
      *state=END_STATE;
      return;
   }/*if(sscanf(theToken,"%d",&tmp)!=1)*/

   if(tmp==0)tmp=1;/*To avoid crash*/
   else if (tmp<0)
      tmp=-tmp;
   else/*tmp>=1*/
      tmp=nextPrime(tmp);

   if(tt_maxTopNumber<tmp)/*Should be extended!*/
      tt_maxTopNumber=tmp;

   if(*ht == NULL){/*Not initialized yet*/
      if(  (*ht=l_readHashInit(tmp, state, err)) == NULL   )
         return;/*err and state are set by l_readHashInit*/
   }else if((*ht)->tablesize!=tmp) {/*Must be re-sized*/
      if(  l_resizeHashInit(*ht,tmp, state, err)  )
         return;/*err and state are set by l_resizeHashInit*/
   }/*if(*ht == NULL) ... else if(*ht->n!=tmp) */
   *state=INIT_STATE;
}/*l_numberOfTopologies*/

static void l_numberOfTokens(HASH_TABLE *ht, int *state, int *err, tt_scannerType *sc)
{
char *theToken;
 int tmp;
   if(  (theToken=tt_getNonNullToken(sc))==NULL)return;
   if(*theToken=='=')
      if(  (theToken=tt_getNonNullToken(sc))==NULL)return;

   if(sscanf(theToken,"%d",&tmp)!=1){
      *err=TT_FORMAT;
      *state=END_STATE;
      return;
   }/*if(sscanf(theToken,"%d",&tmp)!=1)*/

   *state=INIT_STATE;

   if(tmp == 0){/* Just clear the token table:*/
      hash_table_done(*ht);
      *ht=NULL;
      return;
   }/*if(tmp == 0)*/

   /*By convention, to insist on exact table size, we use a negative number:*/
   if(tmp<0)tmp=-tmp;
   else/*Positive value will be ceiled up to the nearst prime:*/
      tmp=nextPrime(tmp);

   if( *ht == NULL ){/*Not initialized yet, just create a new table:*/
      if(  (*ht=create_hash_table_with_copy(
                  tmp,
                  str_hash,
                  str_cmp,
                  c_destructor,
                  (CPY *)NEWSTR,
                  (CPY *)NEWSTR) )==NULL  ){
                     *err=TT_NOTMEM;
                     *state=END_STATE;
                     return;
      }/*if(  (*ht=create_hash_table...*/
   }else if(    resize_hash_table(
                 *ht,
                 tmp
                 )  ){
      *err= TT_NOTMEM;
      *state=END_STATE;
      return;
   }
   /**state=INIT_STATE is already set up*/
}/*l_numberOfTokens*/

/*ht here is just the topologies name table!:*/
static void l_topologyBegin(
   int cnum,
   char *theToken,
   tt_singletopol_type **topology,
   tt_singletopol_type **topols,
   long *counter,
   HASH_TABLE *ht,
   int *state,
   int *err,
   tt_scannerType *sc                )
{
char *topologyName=NULL;
      if( *ht == NULL )/*Not initialized yet*/
         if(  (*ht=l_readHashInit(TT_MAX_TOP_N, state, err)) == NULL   )
            return;

       /*Topology may has several names. Only  first one must be unique.*/
       /*The topologyName is just this first unique name.*/

      if((*counter)>tt_maxTopNumber){
         (*err)=TT_TOOMANYTOPS;/* Too many topologies.*/
         (*state)=END_STATE;
         return;
       }
       if(  ((*topology)=tt_initSingleTopology())==NULL  ){ /*fail...*/
          l_failLoadTable(topols,(*counter),cnum);
          (*err)=TT_NOTMEM;
          (*state)=END_STATE;
          return;
       }
       (*topology)->n=(*counter);
       topologyName=new_str("");/*Initialization*/
       /* Now read topology name: */
       while ((*state)==TOPOLOGY_BEGIN)if((theToken=tt_getNonNullToken(sc))!=NULL){
          if( (*theToken == '=') || (*theToken == ',')  ){
             if(topologyName!=NULL){
                if(*topologyName == '\0'){/*Generate the name automatically*/
                   char tmp[NUM_STR_LEN+2];
                   sprintf(tmp,"t%ld_",(*counter));
                   topologyName=s_inc(topologyName,tmp);
                }
                if( install(topologyName,newLong((*counter)), *ht) ){
                   (*err)=TT_DOUBLEDEFNAM;
                   (*state)=END_STATE;
                }else
                (*topology)->name=topologyName;/*Store the name*/
                topologyName=NULL;/*Stored in the topology, forget about it.*/
             }
             if(  ((*state)==TOPOLOGY_BEGIN)&&(*theToken == '=') )
                (*state)=START_READING_LINE;
          }else/*Just add the token to the toplogy name:*/
             if(topologyName!=NULL)
                topologyName=s_inc(topologyName,theToken);

       }/*while ((*state)==TOPOLOGY_BEGIN)if*/
       if (topologyName!=NULL)/*Must be deleted...*/
          free(topologyName);
}/*l_topologyBegin*/

static void l_startReadingLine(
                  char *theToken,
                  tt_singletopol_type *topology,
                  int *state,
                  int *err,
                  tt_scannerType *sc                )
{
char *topologyString;

       topologyString=new_str("");/*Initialization*/
       while((*state)==START_READING_LINE)if((theToken=tt_getNonNullToken(sc))!=NULL){
          if( (*theToken == ':')||(*theToken == ';') ){
             if(
                 (  ((*err)=tt_str2top(topologyString,topology))<0  ) ||
                 (  ((*err)=tt_reduceTopology(topology))<0  )
             )
                (*state)=END_STATE;
             else
                (*state)=START_READING_MOMENTA;
          }else
             topologyString=s_inc(topologyString,theToken);
       }/*while(state==START_READING_LINE)if*/
       free(topologyString);
}/*l_startReadingLine*/

/*End topology itself - meant, together with momenta, if present:*/
static void l_endTopologyItself(
      long *counter,
      long *max,
      int *cnum,
      tt_singletopol_type **topology,
      tt_singletopol_type ***topols,
      int *err,
      int *state                      )
{
       if( (!((*counter)<(*max)))&&
          (((*topols)=(tt_singletopol_type **)realloc((*topols),
             sizeof(tt_singletopol_type*)*((*max)+=TT_DELTA_TOPOLS_MAX)))==NULL) ){
          l_failLoadTable(*topols,*counter,*cnum);
          *state=END_STATE;
          *err=TT_NOTMEM;
          return;
       }
       (*topols)[(*counter)++]=(*topology);

       if(  (*topology)->nel >0)/*Topology with external lines*/
          (tt_table[*cnum]->extN)++;
       else/*Pure internal topology*/
          (tt_table[*cnum]->intN)++;

       *topology=NULL;
       *state=INIT_STATE;
}/*l_endTopologyItself*/

static void l_swapExtLines(char **momenta, int max)
{
int i,j,k=max/2;
char *tmp;
   if(momenta == NULL)return;/*No external momenta...*/
   for(i=0,j=max-1; i<k;i++,j--){
      tmp=momenta[i];
      momenta[i]=momenta[j];
      momenta[j]=tmp;
   }
}/*l_swapExtLines*/

static char **l_clearOneMomentaSet(char **momenta, int n)
{
int i;
   for(i=0;!(i>n);i++)if (momenta[i]!=NULL) {
      free(momenta[i]);
      momenta[i]=NULL;
   }
   return NULL;
}/*l_clearOneMomentaSet*/

static char **l_momentaCheckAndReturn(int extCond,int i, int max,
                             int *err, int *state,char **momenta)
{
   if(  (extCond)||(i!=max-1)   ){
      *err = TT_INVALID;
      *state=END_STATE;
      return l_clearOneMomentaSet (momenta,i);
   }
   return momenta;
}/*l_momentaCheckAndReturn*/

static char *l_readOneMomentum(
                      long counter,
                      char *theToken,
                      tt_scannerType *sc,
                      int *state,
                      int *err,
                      HASH_TABLE tokenTable)
{
   char *cm=new_str("");
      while(1)switch(*theToken){
         case ',':
         case ':':
         case ';':
         case ']':
            return cm;
         case '{':
            if( (theToken=tt_getNonNullToken(sc))==NULL ){
               free(cm);
               return NULL;
            }
            if(*theToken== '}'){
               /*Store open brace; the closed will be added at default case:*/
               cm=s_inc(cm,"{");
            }else{
               char *tmp=new_str(theToken);
               do{
                  if(
                      (  (theToken=tt_getNonNullToken(sc))==NULL  )||
                      (*theToken==']')
                     ){
                     *err=TT_INVALID;
                     *state=END_STATE;
                     free (tmp);
                     free(cm);
                     return NULL;
                  }
                  if(*theToken == '}')break;
                  tmp=s_inc(tmp, theToken);
               }while(1);
               cm=s_inc(s_inc(cm,"{"),tmp);
               if(lookup(tmp,tokenTable) == NULL)/*new token, translated to ""*/
                  install(tmp,new_str(""),tokenTable);
               else
                  free(tmp);/*Already installed, we need not them anymore*/
            }/*if(*theToken== '}'){...}else*/
            /*no break, *theToken must be == '}'*/
         default:
            cm=s_inc(cm,theToken);
            /*tt_newVector is some external function to store a vector,if needed.
              It is initialised by NULL, but the user may provide his non-trivial
              function void tt_newVector(long n, char *vec). */
            if(tt_newVector!=NULL)tt_newVector(counter,theToken);
            if(
                                                  (cm==NULL) ||
               (  (theToken=tt_getNonNullToken(sc))==NULL  )
                                                                              ){
               free(cm);
               *err=TT_NOTMEM;
               *state=END_STATE;
               return NULL;
            }
      }/*while(1)switch(*theToken)*/
}/*l_readOneMomentum*/

static char **l_readOneMomentaSet(
                   long counter,
                   int max,
                   char **momenta,
                   int isExt,
                   char *theToken,
                   tt_scannerType *sc,
                   HASH_TABLE tokenTable,
                   int *err,
                   int *state
                                                 )
{
int i;
   for(i=0;i<max; i++){
      if(
         (momenta[i] = l_readOneMomentum(
                              counter,theToken,sc,state,err,tokenTable )
         )==NULL
      )return l_clearOneMomentaSet(momenta,i);
      switch (*theToken){
         case ':':
         case ';':
            return l_momentaCheckAndReturn(isExt,i,max,err,state,momenta);
         case ']':
            return l_momentaCheckAndReturn(!isExt,i,max,err,state,momenta);
         case ',':
            if( (theToken=tt_getNonNullToken(sc))==NULL )break;
      }/*switch (*theToken)*/
   }/*for(i=0;i<(*max); i++)*/
   *err=TT_INVALID;
   *state=END_STATE;
   return NULL;
}/*l_readOneMomentaSet*/

static int l_clearT(HASH_TABLE tt)
{
   hash_table_done(tt);
   return TT_NOTMEM;
}/*l_clearT*/

static int topology_begin=TOPOLOGY_BEGIN;
static int coordinates_begin=COORDINATES_BEGIN;
static int number_of_topologies=NUMBER_OF_TOPOLOGIES;
static int number_of_tokens=NUMBER_OF_TOKENS;
static int momenta_or_shape=MOMENTA_OR_SHAPE;
static int translation=TRANSLATION;
static int remarks_begin=REMARKS_BEGIN;

static int coordinates_ev=COORDINATES_EV;
static int coordinates_evl=COORDINATES_EVL;
static int coordinates_iv=COORDINATES_IV;
static int coordinates_ivl=COORDINATES_IVL;
static int coordinates_el=COORDINATES_EL;
static int coordinates_ell=COORDINATES_ELL;
static int coordinates_il=COORDINATES_IL;
static int coordinates_ill=COORDINATES_ILL;

static int l_fillUpLoadTableHash(HASH_TABLE tt)
{
   if(install(tt_tokenTopology,&topology_begin,tt)==-1)return l_clearT(tt);
   if(install(tt_tokenCoordinates,&coordinates_begin,tt)==-1)return l_clearT(tt);
   if(install(tt_tokenTMax,&number_of_topologies,tt)==-1)return l_clearT(tt);
   if(install(tt_tokenTrMax,&number_of_tokens,tt)==-1)return l_clearT(tt);
   if(install(tt_tokenMoS,&momenta_or_shape,tt)==-1)return l_clearT(tt);
   if(install(tt_tokenTrans,&translation,tt)==-1)return l_clearT(tt);
   if(install(tt_tokenRemarks, &remarks_begin,tt)==-1)return l_clearT(tt);

   if(install(tt_tokenEv,&coordinates_ev,tt)==-1)return l_clearT(tt);
   if(install(tt_tokenEvl,&coordinates_evl,tt)==-1)return l_clearT(tt);
   if(install(tt_tokenIv,&coordinates_iv,tt)==-1)return l_clearT(tt);
   if(install(tt_tokenIvl,&coordinates_ivl,tt)==-1)return l_clearT(tt);
   if(install(tt_tokenEl,&coordinates_el,tt)==-1)return l_clearT(tt);
   if(install(tt_tokenEll,&coordinates_ell,tt)==-1)return l_clearT(tt);
   if(install(tt_tokenIl,&coordinates_il,tt)==-1)return l_clearT(tt);
   if(install(tt_tokenIll,&coordinates_ill,tt)==-1)return l_clearT(tt);
   return 0;
}/*l_fillUpLoadTableHash*/

static int l_readOneCoordinateLine(
                            double **arr,
                            int max,
                            char *theToken,
                            tt_scannerType *sc,
                            int *err,
                            int *state
                            )
{
int c,di,i,itmp;
/* We will use the loop on i from 0 to |max|-1. The counter c will be incremented
   at each step by di. The negative max means that the cycle will be performed
   from max-1 down to 0.*/

   if (max<0){/*External lines, must be stored from the end to the begin*/
      max=-max;
      c=max-1;
      di=-1;
   }else if (max == 0){/*Nothig to store, just eat all tokens up to ':' or ';'*/
      i=1;
      do{
         if(  (theToken=tt_getNonNullToken(sc))==NULL  )return *err;/*err must be obtained
                                     from the topology, and the scanner set it properly */
         switch(*theToken){
            case ';':
            case ':':
               i=0;break;
         }
      }while(i);
      return 0;
   }else{/*INternal lines*/
      c=0;
      di=1;
   }
   itmp=max-1;
   /*Allocate an array of coordinates:*/
   if(   (*arr=(double *)calloc(2*max,sizeof(double)))==NULL  ){
      *state=END_STATE;
      return(*err=TT_NOTMEM);
   }
   /*MAin cycle. Coordinates are stored as pairs (x,y), so x is in (*arr)[c*2], and
     y is stored in (*arr)[c*2+1]:*/
   for(i=0;i<max;i++){
      if(  (theToken=tt_getNonNullToken(sc))==NULL  )break;/**err is set by tt_getNonNullToken!*/
      /*Read X coordinate:*/
      if(  (sscanf(theToken,"%lf",(*arr)+c*2))!=1  ){
         *err=TT_INVALID;*state=END_STATE;break;
      }
      /*Read ',':*/
      if(  (theToken=tt_getNonNullToken(sc))==NULL  )break;
      if( *theToken !=','  ){*err=TT_INVALID;*state=END_STATE;break;}
      if(  (theToken=tt_getNonNullToken(sc))==NULL  )break;/**err is set by tt_getNonNullToken!*/
      /*Read Y coordinate:*/
      if(  (sscanf(theToken,"%lf",(*arr)+c*2+1))!=1  ){
         *err=TT_INVALID;*state=END_STATE;break;
      }
      if(  (theToken=tt_getNonNullToken(sc))==NULL  )break;
      if (i<itmp){/*theToken must be  comma:*/
         if( *theToken !=','  ){*err=TT_INVALID;*state=END_STATE;break;}
      }else if (i == itmp ){/*theToken must be a ':' or ';'*/
         if(  ( *theToken !=':')  && ( *theToken !=';')  ){
            *err=TT_INVALID;*state=END_STATE;break;
         }
      }
      /*That's all, now increment the counter:*/
      c+=di;
   }/*for(i=0;i<max;i++)*/
   if( (*err)<0 )/*Well, an error occure...*/
      free_mem(arr);
   return (*err);
}/*l_readOneCoordinateLine*/

/*Reads topology name, looks it up in the table of known topologies and
returns the pointer to found topology. Expects "name = ". If falis, terurns NULL:*/
static tt_singletopol_type *l_readTopologyName(
                     tt_singletopol_type **topols,
                     char *theToken,
                     tt_scannerType *sc,
                     HASH_TABLE ht,
                     int *err,
                     int *state)
{
tt_singletopol_type *t;
long *top;
   if(  (theToken=tt_getNonNullToken(sc))==NULL  )return NULL;/*err must be obtained
                                     from the topology, and the scanner set it properly */

   /* theToken must contain the topology name. Look for it in the table ht:*/
   if(    (ht == NULL)||
          ( (top=lookup(theToken,ht)) == NULL )
                                                 ){
          /* Topology name is not defined*/
          *state=END_STATE;
          *err= TT_TOPUNDEF;
          return NULL;
   }/*if(    (ht == NULL)|| ...*/
   if( (t=topols[*top])==NULL ){
         /*? -- Can't be*/
         *state=END_STATE;
         *err=TT_FATAL;
         return NULL;
   }/*if( (t=topols[*top])==NULL )*/

      /*Now t points to the topology.*/

   /*Now we expect "=" :*/
   if(  (theToken=tt_getNonNullToken(sc))==NULL  )return NULL;

   if( *theToken !='='  ){
      *state=END_STATE;
      *err=TT_INVALID;
      return NULL;
   }/*if( *theToken !='='  )*/
   return t;
}/*l_readTopologyName*/

static void l_readCoordinates(
                     tt_singletopol_type **topols,
                     char *theToken,
                     tt_scannerType *sc,
                     HASH_TABLE ht,
                     HASH_TABLE tt,
                     int *err,
                     int *state)
{
tt_singletopol_type *t;
double **arr;
   if(
       (   t=l_readTopologyName(topols,theToken,sc,ht,err,state)   )==NULL
     )return;/*err must be obtained from the topology, and the scanner set it properly */

   theToken=tt_tokenRemarks;/*Arbitrary dereferenceable value, but not a pointer
                              to a string started by ';'!*/
   while(*theToken !=';'){/*Main loop*/
      int *tmp;
      int max;
      if(  (theToken=tt_getNonNullToken(sc))==NULL  )return;
      /*theToken must contain one of the coordinates type.*/
      if( (tmp=lookup(theToken,tt))!=NULL )switch(*tmp){
      /*Negative 'max' means external coordinates, see  l_readOneCoordinateLine.*/
         case COORDINATES_EV:
            arr=&(t->ext_vert);
            max=-(t->nel);
            break;
         case COORDINATES_EVL:
            arr=&(t->ext_vlbl);
            max=-(t->nel);
            break;
         case COORDINATES_IV:
            arr=&(t->int_vert);
            max=t->nv;
            break;
         case COORDINATES_IVL:
            arr=&(t->int_vlbl);
            max=t->nv;
            break;
         case COORDINATES_EL:
            arr=&(t->ext_line);
            max=-(t->nel);
            break;
         case COORDINATES_ELL:
            arr=&(t->ext_llbl);
            max=-(t->nel);
            break;
         case COORDINATES_IL:
            arr=&(t->int_line);
            max=t->nil;
            break;
         case COORDINATES_ILL:
            arr=&(t->int_llbl);
            max=t->nil;
            break;
         default:
            *state=END_STATE;*err=TT_INVALID;return;
      }else{
         *state=END_STATE; *err=TT_INVALID; return;
      }/*if()switch(){...}else*/
      /*Here we have ready arr -- points to the proper array, and |max| -- the length.*/
      if(  l_readOneCoordinateLine(arr,max,theToken,sc,err,state)<0 )
         /* *err and *state are set, *arr is cleaned by l_readOneCoordinateLine*/
         return;
      /*Everything is ok. theToken may be ':' or ';'.*/
   }/*while(*theToken !=';')*/
   *state=INIT_STATE;
}/*l_readCoordinates*/

/* Read topologies  and stores  them into the table.
   Returns index of the new table, or negative number if fails:*/
int tt_loadTable(char *fname)
{
int cnum=tt_createNewTable(g_defaultTokenDictSize,0);/*tt_createNewTable returns user defind number,
                                            or error code <0. So it will be reduced by 1
                                            further if it is >=0.*/
int state,err=0;
int cnm,isExt=0;
long counter=0,maxT=0;/*During allocation these variables will be used*/
int maxM=0;/*for the momenta set numbering*/

tt_singletopol_type *topology=NULL;/*working cell*/
tt_singletopol_type **topols=NULL;/*temporary array*/

/*Temporary table for topology names. We need them only during
  loading since coordinates are bound to the topology name:*/
HASH_TABLE ht=NULL;
/*Just a hash table to convert strings into integers suitable for a "switch": */

tt_scannerType *sc=NULL;
char *theToken=NULL;
char **cmom=NULL;

 if (cnum<0)return cnum;/*Fail creating new empty table. Must be TT_NOTMEM -- not memory*/
 cnum--;/*tt_createNewTable returns user defind number!*/
 /* Hash table used by tt_loadTable, initialised once forever (just a set of keywards):*/
 if(    (l_tt==NULL)&&
           (
             ( (l_tt=create_hash_table(
                 29,
                 str_hash,
                 str_cmp,
                 l_empty_destructor))==NULL  )||
               (l_fillUpLoadTableHash(l_tt))/*ad hoc, fill up this table*/
           )                                           ){
    tt_deleteTable(cnum+1);/*+1 since user numbering starts from 1*/
    return TT_NOTMEM;
 }

 /*Fali initialising a scanner?:*/
 if(  (sc=tt_scInit(fname, MAX_STR_LEN, &state, &err))==NULL  ){
    /*Yet empty table must be destroyed*/
    tt_deleteTable(cnum+1);/*+1 since user numbering starts from 1*/
    return err;
 }

 /*Begin the finite-state automaton:*/
 for(counter=0,state=INIT_STATE;state!=END_STATE;)switch(state){
    case INIT_STATE:
       while(state==INIT_STATE){
          if(  (theToken=tt_getToken(sc))==NULL  ){
             state=END_STATE;
          }else{int *tmp;
             if( (tmp=lookup(theToken,l_tt))!=NULL )switch(state=*tmp){
                case END_STATE:
                case TOPOLOGY_BEGIN:/*TT_TOKENTOPOLOGY "topology"*/
                case COORDINATES_BEGIN:/*TT_TOKENCOORDINATES "coordinates"*/
                case NUMBER_OF_TOPOLOGIES:/*TT_TOKENTMAX "numberoftopologies"*/
                case NUMBER_OF_TOKENS:/*TT_TOKENTMAXTRANS "numberoftokens"*/
                case TRANSLATION:/*TT_TOKENTRANS "token"*/
                case REMARKS_BEGIN: /*TT_TOKENREMARKS "remarks"*/
                case MOMENTA_OR_SHAPE:/*TT_TOKENMoS "momentaorshape"*/
                   break;
                default:
                   state=INIT_STATE;
             }/*if()switch*/
          }/*if(){..}else*/
       }/*while(state==INIT_STATE)*/
       break;
    case REMARKS_BEGIN:
       {/*block begin*/
          /*Reads topology name, looks it up in the table of known topologies and
            returns the pointer to found topology. Expects "name = ":*/
          tt_singletopol_type *tmp=l_readTopologyName(
                                      topols,
                                      theToken,
                                      sc,
                                      ht,
                                      &err,
                                      &state
                                   );
          if(tmp==NULL)break;/*state and err are set by the function*/
          /*Now tmp points to the topology.*/

          state=END_STATE; /*May be reset after at the following switch:*/

          /* Function readTopolRemarksFromScanner (remarks.c)
             reads from scanner pairs
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
          switch ( readTopolRemarksFromScanner
            (
              &(tmp->top_remarks),/* ptr to the top of allocated remarks array*/
              &(tmp->remarks),/*ptr to the  remarks array*/
              g_uniqueRemarksName,/*0 if new "name" may rewrite existing one*/
              /*Function char *getToken(void *) must return the token from the scanner:*/
              (GET_TOKEN_FUNC *) &tt_getNonNullToken,
              (void *) sc/* the argument for the function getToken*/
            )
          ){
             /*Error codes returned by readTopolRemarksFromScanner:*/
             /*Double defined name:*/
             case REM_DOUBLEDEF:
                err=TT_REM_DOUBLEDEF;break;
             /*Empty name is not allowed:*/
             case REM_EMPTYNAME:
                err=TT_REM_EMPTYNAME;break;
             /*Not enought memory:*/
             case REM_NOTMEMORY:
                err=TT_NOTMEM;break;
             /*Unexpected end of file:*/
             case REM_U_EOF:
                err=TT_U_EOF;break;
             /*Unexpected '=':*/
             case REM_UNEXPECTED_EQ:
                err=TT_REM_UNEXPECTED_EQ;break;
             default:/*No error! Must be 0!*/
                state=INIT_STATE;break;
          }/*switch ( readTopolRemarksFromScanner ...*/
       }/*block end*/

       break;
    case TRANSLATION:
       /*parses token A = B*/
       l_translation(tt_table[cnum]->htrans,&state, &err, sc);
       break;
    case NUMBER_OF_TOPOLOGIES:
       /*Parses '= number'*/
       l_numberOfTopologies(&ht,&state, &err, sc);
       break;
    case MOMENTA_OR_SHAPE:
       /*Parses '= momenta|shape|both'*/
       {int r=l_momentaOrShape(&state, &err, sc);
          if(r!=-2)
             tt_table[cnum]->momentaOrShape = r;
       }
       break;
    case NUMBER_OF_TOKENS:
       /*Parses '= number'*/
       l_numberOfTokens(&(tt_table[cnum]->htrans),&state, &err, sc);
       break;
    case COORDINATES_BEGIN:
       l_readCoordinates(
                       topols,
                       theToken,
                       sc,
                       ht,
                       l_tt,
                       &err,
                       &state
       );
       break;
    /* Start with the new topology: parsing a topology name and
       creating hashtable for topologies name, if needed:*/
    case TOPOLOGY_BEGIN:
       l_topologyBegin(   cnum,
                          theToken,
                          &topology,
                          topols,
                          &counter,
                          &ht,
                          &state,
                          &err,
                          sc
       );
       break;
    case START_READING_LINE:/*Read topology itself, e.g. (-1,1)(-2,2)(1,2)...:*/
       l_startReadingLine(    theToken,
                              topology,
                              &state,
                              &err,
                              sc
       );
       break;
    case START_READING_MOMENTA:
       /*Here may be (*theToken == ';') in case of bare topology...*/
       isExt=0;
       (topology->nmomenta)=0;
       state=CONTINUE_READING_MOMENTA;
       /*no break*/
    case CONTINUE_READING_MOMENTA:
       switch(*theToken){
          case ';':
             /*Reduce arrays according to their actual sizes:*/
             if(
                 ( (topology->momenta)!=NULL ) &&
                 ( (topology->momenta=(char ***)realloc(topology->momenta ,
                         sizeof(char **)*(topology->nmomenta)))==NULL)
               ){
                 err=TT_FATAL;
                 state=END_STATE;
                 break;
             }
             if(
                 ( (topology->extMomenta)!=NULL ) &&
                 ( (topology->extMomenta=(char ***)realloc(topology->extMomenta ,
                         sizeof(char **)*(topology->nmomenta)))==NULL)
               ){
                 err=TT_FATAL;
                 state=END_STATE;
                 break;
             }
             state=END_TOPOLOGY_ITSELF;
             break;
          case ':':
             if( (topology->nmomenta) == 0 ){/*first hit to momenta*/
                if( (topology->nil > 0) &&
                    ( (topology->momenta=(char ***)calloc(TT_MAX_MOMENTA,
                                                    sizeof(char **)))==NULL )
                  ){
                   err=TT_NOTMEM;
                   state=END_STATE;
                   break;
                }
                if( (topology->nel > 0) &&
                    ( (topology->extMomenta=(char ***)calloc(TT_MAX_MOMENTA,
                                                           sizeof(char **)))==NULL)
                  ){
                   free(topology->momenta);
                   (topology->momenta)=NULL;
                   err=TT_NOTMEM;
                   state=END_STATE;
                   break;
                }

             }else if( !( (topology->nmomenta) < TT_MAX_MOMENTA )  ){
                err=TT_TOOMANYMOMS;
                state=END_STATE;
                break;
             }
             cnm=(topology->nmomenta);
             (topology->nmomenta)++;/*starting form 1, not 0!*/
             /*no break*/
          case ']':
             if(  (theToken=tt_getNonNullToken(sc))==NULL  )  break;
             if(*theToken == '['){/*Start reading external momenta*/
                if(isExt != 0){
                   err=TT_INVALID;
                   state=END_STATE;
                   break;
                }
                isExt=1;

                if( (maxM=topology->nel)==0){/*There are no external lines...*/
                   err=TT_INVALID;
                   state=END_STATE;
                   break;
                }
                if(
                   (cmom=topology->extMomenta[cnm]=(char **)calloc(maxM,sizeof(char *)))
                   ==NULL
                   ){
                      err=TT_NOTMEM;
                      state=END_STATE;
                      break;
                }
                if(  (theToken=tt_getNonNullToken(sc))==NULL  )  break;

             }else if(  (*theToken == ':')||(*theToken == ';')  ){
             /*So, here we have something like ::*/
                if (topology->nil==1){/*Empty string as a momentum...*/
                   if(
                      (cmom=topology->momenta[cnm]=(char **)calloc(1,sizeof(char *)))
                      ==NULL
                   ){
                      err=TT_NOTMEM;
                      state=END_STATE;
                      break;
                   }/*if(...)*/
                    topology->momenta[cnm][0]=new_str("");
                    break;
                }else if (topology->nil>0){
                   err=TT_INVALID;
                   state=END_STATE;
                   break;
                }
                if( isExt!=0 ){ isExt=0;break;}
                if(    (topology->nel>0) &&
                       (     isExt==0   )    )
                                topology->extMomenta[cnm]=NULL;
             }else{
                if( (isExt == 0) && (topology->nel>0) ){
                /*There are no external momenta! Set NULL instead:*/
                   topology->extMomenta[cnm]=NULL;
                }/*(isExt == 0) && (topology->nel>0)*/
                isExt=0;
                if( (maxM=topology->nil)==0){/*There are no internal lines...*/
                   err=TT_INVALID;
                   state=END_STATE;
                   break;
                }
                if(
                   (cmom=topology->momenta[cnm]=(char **)calloc(maxM,sizeof(char *)))
                   ==NULL
                   ){
                      err=TT_NOTMEM;
                      state=END_STATE;
                      break;
                }
             }
             /*So, here 'cmom' -- the array of 'maxM' length*/
             if( l_readOneMomentaSet(
                   counter,
                   maxM,
                   cmom,/* char ** */
                   isExt,
                   theToken,
                   sc,
                   tt_table[cnum]->htrans,
                   &err,
                   &state
                  )==NULL)break;
             if(isExt==1)
                l_swapExtLines(cmom,maxM);
             break;
          default:
             err=TT_INVALID;
             state=END_STATE;
             break;
       }/*switch(*theToken)*/

       break;
    case END_TOPOLOGY_ITSELF:
       l_endTopologyItself(
                              &counter,
                              &maxT,
                              &cnum,
                              &topology,
                              &topols,
                              &err,
                              &state
       );
       break;
    default:
       state=INIT_STATE;
       break;
 }/*for(counter=0,state=INIT_STATE;state!=END_STATE;)switch(state)*/
 /*End the finite-state automaton.*/

 if (sc!=NULL){
    if(err < 0)/*There was an error...*/
        /*Output the information about position from the scanner:*/
        tg_scannerOutputMessage();/*This is a macro defined in tt_gate.h*/
    tt_scDone(sc);
    sc=NULL;
 }
 if(ht!=NULL){
    hash_table_done(ht);
    ht=NULL;
 }

 if(err < 0){/*There was an error...*/
    l_failLoadTable(topols,counter,cnum);
    if(topology!=NULL)
       tt_clearSingleTopology(topology);
    return err;
 }
 if(counter==0){/*No empty table at present! May be, not a good idea?*/
    l_failLoadTable(topols,counter,cnum);
    return TT_EMPTYTABLE;
 }
 /*Reduce  the size of topologies array exactly to its actual length:*/
 if (   (topols=(tt_singletopol_type **)realloc(
              topols,sizeof(tt_singletopol_type*)*counter))==NULL  )
       return TT_FATAL;/*This is impossible...*/

 tt_table[cnum]->totalN=counter;
 tt_table[cnum]->topols=topols;
 /*And now the time to initialise all hash tables:*/
 if( (err=l_fillUpHashTable(cnum))<0 ){/*fail...*/
      tt_deleteTable(cnum+1);/*User numering start from 1*/
      return err;
 }
 return cnum+1;/*User numering start from 1*/
}/*tt_loadTable*/

/*Adds top to the specified table. If top->redTopol!=null, neither reduces topology nor
  makes substitutions, i.e. stores them "as is".
  if deep != 0 then create new copy before storing. If deep==0 and top->redTopol == NULL,
  it returns TT_NONFATAL. On sucess, returns number of topology in resulting table:*/
long tt_addToTable(int table, tt_singletopol_type *top, int deep)
{
tt_singletopol_type *newTop;
int i;
   if(top==NULL)return TT_INVALID;
   if( tt_chktbl(&table) )return TT_NOTFOUND;

   if(deep){/*Must perform a deep copy:*/
      if(  ( newTop=tt_copySingleTopology(top,NULL,
         TT_NEL|TT_NIL|TT_NV|TT_EXT|TT_RED|TT_USR|TT_NMOMENTA|
         TT_MOMENTA|TT_EXT_MOMENTA|TT_EXT_COORD|TT_INT_COORD|
         TT_S_RED_USR|TT_S_USR_NUSR|TT_D_USR_RED|TT_D_USR_NUSR|TT_REM|TT_NAME
      ) )==NULL  )return TT_NOTMEM;
   }else{/*Swallow copy*/
      if( top->redTopol == NULL )return TT_NONFATAL;/*Can't swallow unreduced topology!*/
      newTop=top;/*Use it instead of top for universality*/
   }

   /*What about reduced topology?:*/

   if(   (top->redTopol == NULL)&&/*No reduced topology -- must be reduced by this function*/
         ( (i=tt_reduceTopology(newTop))<0 )  ){/*Fail reducing*/
      if(deep)tt_clearSingleTopology(newTop);/*Clear allocated memory*/
      return i;
   }

   /*Now we have the reduced topology in newTop->redTopol. Store it in the hash table:*/
   if(tt_table[table]->hred==NULL){/*Not initialized yet*/
         /*Initialise the table by default table size:*/
         if(   (tt_table[table]->hred=create_hash_table(
                 g_defaultHashTableSize,
                 l_red_hash,
                 l_red_cmp,
                 l_empty_destructor))==NULL
           ){
              if(deep)tt_clearSingleTopology(newTop);/*Clear allocated memory*/
              return TT_NOTMEM;
         }
   }else /*Check if the table is too small:*/
      if ( ((tt_table[table]->hred)->tablesize)*2<tt_table[table]->totalN ){
         /*Too small, really. Try te resize it:*/
         if(    resize_hash_table(
                 tt_table[table]->hred,
                 nextPrime(tt_table[table]->totalN)
                 )  ){
              if(deep)tt_clearSingleTopology(newTop);/*Clear allocated memory*/
              return TT_NOTMEM;
         }
      }

   /*Ok, the table is ready to use, in any case. Now store the reduced topology:*/
   switch( install(newTop,newTop,tt_table[table]->hred) ){
         case -1:/*Fail*/
            if(deep)tt_clearSingleTopology(newTop);/*Clear allocated memory*/
            return TT_NOTMEM;
         case 1:/*There is the same topology in the table.*/
            if(deep)tt_clearSingleTopology(newTop);/*Clear allocated memory*/
            return TT_DOUBLETOPS;
         default:;/*Ok.*/
   }/*switch( install(newTop,newTop,tt_table[table]->hred) )*/

   newTop->n=tt_table[table]->totalN;
   /*Expand topology array:*/
   if(
       ( top=/*Reallocated pointer is casted to tt_singletopol_type * instead of
              tt_singletopol_type ** -- really, does not a matter! We use top
              just to store the pointer temporarily!*/
            (tt_singletopol_type *)realloc(
              tt_table[table]->topols,(tt_table[table]->totalN+1)*sizeof(tt_singletopol_type*)
                                           )
       )==NULL
     ){/*Fail expanding*/
       if(deep)tt_clearSingleTopology(newTop);/*Clear allocated memory*/
       return TT_NOTMEM;
   }/*if*/

   tt_table[table]->topols=(tt_singletopol_type **)top;/*Cast to correct type*/

   if(  newTop->nel >0)/*Topology with external lines*/
      (tt_table[table]->extN)++;
   else/*Pure internal topology*/
      (tt_table[table]->intN)++;
   (tt_table[table]->topols)[(tt_table[table]->totalN)]=newTop;

   return ++(tt_table[table]->totalN);

}/*tt_addToTable*/

/* adds topology "topol" from a table "fromTable" to the end of the table "toTable".
   If addTransTbl!=0, tries to add translations from "fromTable" to "toTable".
   Returns the index of added topology in  the table  "toTable", or error code <0:*/
long tt_appendToTable(int fromTable, long topol,int toTable, int addTransTbl)
{
long retcode=0;
   if( tt_chktop(&fromTable,&topol) ) return TT_NOTFOUND;
   if(
      (
         (retcode=tt_addToTable(toTable,(tt_table[fromTable]->topols)[topol], 1))
         > 0
      )&&
      (addTransTbl!=0)&&
      ( tt_table[fromTable]->htrans != NULL )
   ){
      if( tt_chktbl(&toTable) )return TT_NONFATAL;
      tt_table[toTable]->htrans =
           append_to_hash_table(tt_table[fromTable]->htrans, tt_table[toTable]->htrans);
   }

   return retcode;
}/*tt_appendToTable*/

