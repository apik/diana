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

/*tt_gate.c*/

#include <stdlib.h>

#include "tools.h"
#include "comdef.h"
#include "types.h"
#include "hash.h"
#include "tt_gate.h"
#include "tt_type.h"

void free_mem(void *block);

int reduce_topology_full(pTOPOL i_topology, pTOPOL r_topology,
                    char *l_subst, char *v_subst,char *l_directions);
int reduce_internal_topology(pTOPOL i_topology, pTOPOL r_topology,
                    char *l_subst, char *v_subst,char *l_directions);
void *lookup(void *tag, HASH_TABLE hash_table);
void tt_clearSingleTopology(tt_singletopol_type *t);
/* Allocates and initialises new single topology data:*/
tt_singletopol_type *tt_initSingleTopology(void);

/*See tt_load.c:*/
extern tt_table_type **tt_table;
/*top of the occupied array table. New table must be alocated as [top++]:*/
extern int tt_top;
/*Size of allocated array:*/
extern int tt_max;

/*Ok, this is just an auxiliary, it copies fields from pTOPOL top.
  If nv<0 it will be calculated, if not, it will be accepted
  Idetifiers for "op" parameters see tt_types.h, use TT_NEL|TT_NIL|TT_NV|TT_EXT
*/
int l_pTOPOL2singletop(pTOPOL top,tt_singletopol_type *t, int nv, unsigned int op);

/*If TT_INT != char, use the function instead of the macro:*/
char *invertsubstitution(char *str, char *substitution);
#define tt_invertsubstitution(str,substitution) invertsubstitution( (str), (substitution))
/*
TT_INT *tt_invertsubstitution(TT_INT *str, char *substitution)
{
 register int i;
    *str=-1
    for(i=1; substitution[i]!= '\0'; i++)
        str[substitution[i]]=i;

    str[i]=0;
    return(str);
}
*/

/*Returns 0 on succes, or <0 if fails:*/
int tg_reduceTopology(tt_singletopol_type *topology)
{
  int i,j;
  pTOPOL i_top,r_top;

  /*Allocate temporary structures:*/
  if(  ( i_top=malloc(sizeof(tTOPOL)) )== NULL  )
     return(TT_NOTMEM);
  if(  ( r_top=malloc(sizeof(tTOPOL)) )== NULL  ){
     free(i_top);
     return(TT_NOTMEM);
  }
  /*Create i_top from topology:*/
  i_top->e_n=topology->nel;
  i_top->i_n=topology->nil;
  for(i=1,j=0;j<i_top->e_n; i++,j++){
     i_top->e_line[i].from=(topology->extPart)[j].from;
     i_top->e_line[i].to=(topology->extPart)[j].to;
  }
  for(i=1,j=0;j<i_top->i_n; i++,j++){
     i_top->i_line[i].from=(topology->usrTopol)[j].from;
     i_top->i_line[i].to=(topology->usrTopol)[j].to;
  }
  /**/
  /*!!!*/
  /*ATTENTION! TT_INT assumed to be char, see Here :*/
     if(i_top->e_n>0)
        reduce_topology_full(i_top,r_top,
                     topology->l_usr2red,/*Here*/
                     topology->v_usr2red,/*Here*/
                     topology->l_dirUsr2Red
                     );
     else
        reduce_internal_topology(i_top,r_top,
                              topology->l_usr2red,/*Here*/
                              topology->v_usr2red,/*Here*/
                              topology->l_dirUsr2Red
                             );

  /*Now we have to create redTopol:*/
  if(topology->nil>0){
     if( (topology->redTopol=(tt_line_type*)
             malloc(sizeof(tt_line_type)*topology->nil) )!=NULL   ){
        for(i=0,j=1;i<topology->nil;i++,j++){
          (topology->redTopol)[i].from=(r_top->i_line)[j].from;
          (topology->redTopol)[i].to=(r_top->i_line)[j].to;
        }
/* To change ext part -- nonsense, of course:
        for(i=0,j=1;i<topology->nel;i++,j++){
          (topology->extPart)[i].from=(r_top->e_line)[j].from;
          (topology->extPart)[i].to=(r_top->e_line)[j].to;
        }
*/
        i=0;
      }else
        i=TT_NOTMEM;
   }/*if(topology->nil>0)*/

  /*Free temporary structures:*/
  free(r_top);
  free(i_top);
  return i;
}/*tg_reduceTopology*/

extern int is_scanner_init;
extern int is_scanner_double_init;
static int mem_q_char=0;
extern char  esc_char;
extern char mem_esc_char;

/*Must allocate s->extrainfo in success and return 0
  If fails, must set sc->err properly: */
int tg_scannerInit(tt_scannerType *s)
{
   set_of_char spaces,delimiters;
   char j;
      set_sub(spaces,spaces,spaces);/* clear spasces */
      /*assume all ASCII codes <=' ' as spaces:*/
      for (j=0;!(j>' ');j++)set_set(j,spaces);

      /* set separators:*/
        /* 1. set all NOT separators: */
        /* +-. for numbers!*/
      set_str2set("+-._0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ",
                  delimiters);

      set_set(0,delimiters); /* 2. add 0 */
      /* 3. inverse d -- now d containes separators and spaces */
      set_not(delimiters,delimiters);
      /* 4. remove spaces from d. That's all. */
      set_sub(delimiters,delimiters,spaces);
      /* 0-wihtout comments, 1-hash_enable, 0-without escape character.*/
      mem_q_char=q_char;
      if(sc_init(s->fname,spaces,delimiters,'*',1,0)){
         if(double_init(s->fname,spaces,delimiters,'*',1,0)){
            *(s->err)=TT_CANTINITSCAN;
            return -1;
         }
         is_scanner_double_init=1;
      }
      /*Ignore s->extrainfo*/

      EOF_denied=0;/* EOF now enable*/
      q_char=0;/*Avoid quotation mechanism*/
      is_scanner_init=1;
      return 0;
}/*tg_scannerInit*/

void tg_scannerDone(tt_scannerType *s)/*Must free s->extrainfo!*/
{
  if(!is_scanner_init)return;

  if(is_scanner_double_init){
    double_done();
    is_scanner_double_init=0;
  }else
    sc_done();
  is_scanner_init=0;
  esc_char=mem_esc_char;
  q_char=mem_q_char;
  /*Ignore s at all;)*/
}/*tg_scannerDone*/

char *tg_scannerGetToken(tt_scannerType *s)
{
 if(!is_scanner_init){
    *(s->err)=TT_SCANNER_NOTINIT;
    return NULL;
 }
 if(sc_get_token(s->buf)==NULL)
   return NULL;

 return s->buf;
}/*tg_scannerGetToken*/

/*?!!*/
/*Looks up the topology top in the table. NO REDUCTION!
  The topology is assumed to be reduced!
  nv is the number of vertices. If it is <0, then it will be evaluated by the
  subroutine.
  The variable t is already allocate (tt_singletopol_type *tt_initSingleTopology())
  If top!=NULL then it assumed that the topology t must be copied from top,
  otherwice it is assumed to be ready :*/
long tt_lookupFromTop(tt_singletopol_type *t,int table, pTOPOL top, int nv)
{
tt_singletopol_type *tptr=NULL;
int i;
   if( (table >tt_top )||(table <1)||(tt_table[table-1]==NULL)  )
      return TT_NOTFOUND;

   /*If top==NULL we assume that t is ready to use:*/
   if(top!=NULL){/*Copy fields from top to t*/
      /*We fill up only fields needed for hash (l_red_hash from tt_load.c) and cmp
         l_red_cmp from tt_load.c), i.e. nel,nil, nv, extPart and redTopol:*/
     if( (i=l_pTOPOL2singletop( top,t,nv,TT_NEL|TT_NIL|TT_NV|TT_EXT|TT_RED))<0 )
        return i;
   }/*if(top!=NULL)*/

   if( (tptr=lookup(t,tt_table[table-1]->hred))==NULL )return TT_NOTFOUND;

   return tptr->n + 1;
}/*tt_lookupFromTop*/

#ifdef TRY_CODE
extern aTOPOL *topologies;

extern unsigned int cn_topol;
extern unsigned int top_topol;
void try_code(void)
{
int i,tbl=tt_createNewTable(17,17);
long n=0,o=0;
tt_singletopol_type *t=tt_initSingleTopology();

printf("number=%ld\n",tt_lookupFromTop(t,3,topologies[cn_topol].topology,-1));
tt_clearSingleTopology(t);

  printf("Collecting to %d\n",tbl);
  if(tbl<=0)return;
  for(i=0;i<top_topol;i++){
     t=tt_initSingleTopology();
     o=tt_lookupFromTop(t,tbl,topologies[i].topology,topologies[i].max_vertex);
/*     tt_clearSingleTopology(t);*/
     if(o!=TT_NOTFOUND){ tt_clearSingleTopology(t);continue;}
     else
     if(tt_topol2singletop(topologies+i,t,
TT_USR|TT_NMOMENTA|TT_MOMENTA|TT_EXT_MOMENTA|
TT_EXT_COORD|
TT_INT_COORD|TT_S_RED_USR|TT_S_USR_NUSR|TT_D_USR_RED|TT_D_USR_NUSR|TT_REM
        )<0 ){printf("Fail!\n");break;}

/*     t=tt_initSingleTopology();
     if(tt_topol2singletop(topologies+i,t,
TT_NEL|TT_NIL|TT_NV|TT_EXT|TT_RED|TT_USR|TT_NMOMENTA|TT_MOMENTA|TT_EXT_MOMENTA|
TT_EXT_COORD|
TT_INT_COORD|TT_S_RED_USR|TT_S_USR_NUSR|TT_D_USR_RED|TT_D_USR_NUSR|TT_REM
        )<0 ){printf("Fail!\n");break;}

*/
      if(  (n=tt_addToTable(tbl,t, 0) ) <  0  ){printf("Fail at i=%d,%ld!\n",i,n);break;}
  }
  printf("Success, n=%ld\n Save:",n);
  {FILE *uuu;
     uuu=fopen("aa.top","w");
     printf( "%ld\n",tt_saveTableToFile(uuu,tbl,0));
     fclose(uuu);
  }

}
#endif

/*Ok, this is just an auxiliary, it copies fiels from pTOPOL top.
  If nv<0 it will be calculated, if not, it will be accepted
  Idetifiers for "op" parameters see tt_types.h, use TT_NEL|TT_NIL|TT_NV|TT_EXT
*/
int l_pTOPOL2singletop(pTOPOL top,tt_singletopol_type *t, int nv, unsigned int op)
{
int i,j,retcode=0;

   if(op & TT_NEL)t->nel=top->e_n;
   if(op & TT_NIL)t->nil=top->i_n;

   if(op & TT_NV){
      if(nv<0){/*Number of vertices must be calculated*/
         t->nv=1;/*NOT 0, important!*/
         for(j=1;j<=t->nil; j++){
            if( (top->i_line)[j].from > t->nv)
               t->nv=(top->i_line)[j].from;
            if( (top->i_line)[j].to > t->nv)
               t->nv=(top->i_line)[j].to;
         }/*for(j=1;j<=t->nil; j++)*/
         /*So, if there are no internal lines at all, t->nv will be 1.
         This is just the case for pure vertex.*/
      }else
         t->nv=nv;
   }/*if(op&TT_NV)*/

   if(op & TT_EXT){
      free_mem(    &(t->extPart)   );
      if(top->e_n > 0){/*Of course, top-e_n rather then t->nel!*/
         if(  ( t->extPart=calloc(top->e_n,sizeof(tt_line_type)) )==NULL)/*Nothig to free*/
            return TT_NOTMEM;
         for(j=0,i=1;j<top->e_n; j++,i++){
            t->extPart[j].from=top->e_line[i].from;
            t->extPart[j].to=top->e_line[i].to;
         }/*for(j=0,i=1;j<t->nel; j++,i++)*/
      }/*if(top->e_n > 0)*/
   }/*if(TT_EXT)*/
   if(op & TT_RED){
      free_mem(    &(t->redTopol)   );
      if(top->i_n>0){
         if(    ( t->redTopol=calloc(top->i_n,sizeof(tt_line_type)) )==NULL  ){
            retcode=TT_NOTMEM;
            goto fail_return;
         }/*if(  ( t->redTopol=calloc(t->nil,sizeof(tt_line_type)) )==NULL  )*/

         for(j=0,i=1;j<top->i_n; j++,i++){
            t->redTopol[j].from=(top->i_line)[i].from;
            t->redTopol[j].to=(top->i_line)[i].to;
         }/*for(j=0,i=1;j<t->nil; j++,i++)*/
      }/*if(top->i_n>0)*/
   }/*if(op & TT_RED)*/
   if(op & TT_USR){
      free_mem(    &(t->usrTopol)   );
      if(top->i_n>0){
         if(    ( t->usrTopol=calloc(top->i_n,sizeof(tt_line_type)) )==NULL  ){
            retcode=TT_NOTMEM;
            goto fail_return;
         }/*if(  ( t->usrTopol=calloc(t->nil,sizeof(tt_line_type)) )==NULL  )*/

         for(j=0,i=1;j<top->i_n; j++,i++){
            t->usrTopol[j].from=(top->i_line)[i].from;
            t->usrTopol[j].to=(top->i_line)[i].to;
         }/*for(j=0,i=1;j<t->nil; j++,i++)*/
      }/*if(top->i_n>0)*/
   }/*if(op & TT_USR)*/
   return 0;
   fail_return:/*rollback*/
      if(  (op & TT_EXT)&&(top->e_n > 0)&&(t->extPart!=NULL)  )
         free(t->extPart);
      if(  (op & TT_RED)&&(top->i_n > 0)&&(t->redTopol!=NULL)  )
         free(t->redTopol);
      if(  (op & TT_USR)&&(top->i_n > 0)&&(t->usrTopol!=NULL)  )
         free(t->usrTopol);

      return retcode;
}/*l_pTOPOL2singletop*/

/*Concatenates s1 and s2 with memory rellocation for s1:*/
char *s_inc(char *s1, char *s2);

extern MOMENT *vec_group;

/*m is an array of the length m[0]. The function allocates built momenta and returns it:*/
char *tg_get_text_from_group(int *m,int sign)
{
int i,l;
char *buf=NULL;
   if(m==NULL)return NULL;

   for(i=1;i<= *m; i++){
      if( (l=m[i]*sign)<0 ){ /*"-"*/
         buf=s_inc(buf,"-");
         l=-l;
      }else/*"+"*/
         buf=s_inc(buf,"+");
      buf=s_inc(buf, vec_group[l].text );
   }/*for(i=1;i<= *m; i++)*/
   return buf;
}/*tg_get_text_from_group*/

/*
 The function fills up fields in "t" from "topol" according to a binary mack "op".
 Full set is:
TT_NEL|TT_NIL|TT_NV|TT_EXT|TT_RED|TT_USR|TT_NMOMENTA|TT_MOMENTA|TT_EXT_MOMENTA|
TT_EXT_COORD|TT_INT_COORD|TT_S_RED_USR|TT_S_USR_NUSR|TT_D_USR_RED|TT_D_USR_NUSR|
TT_REM|TT_NAME
 see tt_type.h. "t" must be allocated, all internal memory will be allocated by this function.
 Returns 0 on success, or error code <0.
 */
int tt_topol2singletop(aTOPOL *topol,tt_singletopol_type *t, unsigned int op)
{
int i,j,retcode,nel,nil,nv,nm;
#ifdef DEBUG
   if(topol == NULL )
      return TT_NOTFOUND;
#endif
   if(t==NULL)/*Assume online call to tt_initSingleTopology()*/
       return TT_NOTMEM;

   /*Old values for momenta must be cleared before l_pTOPOL2singletop
     since l_pTOPOL2singletop may modify t->nel and t->nil:*/
   if(  (op & TT_EXT_MOMENTA)&&(t->extMomenta!=NULL)  ){/*Clear old values*/
            for(i=0;i<t->nmomenta; i++)
               if(  (t->extMomenta)[i]!=NULL  ){
                  for(j=0; j<t->nel; j++)
                     free(  ( (t->extMomenta)[i] )[j] );
                  free(  (t->extMomenta)[i] );
               }/*if(  (to->extMomenta)[i]!=NULL  )*/
            free_mem(  &(t->extMomenta)   );
   }/*if(  (op & TT_EXT_MOMENTA)&&(t->extMomenta!=NULL)  )*/
   if(  (op & TT_MOMENTA)&&(t->momenta!=NULL)  ){/*Clear old values*/
         for(i=0;i<t->nmomenta; i++)
            if(  (t->momenta)[i]!=NULL  ){
               for(j=0; j<t->nil; j++)
                  free(  ( (t->momenta)[i] )[j] );
               free(  (t->momenta)[i] );
            }/*if(  (t->momenta)[i]!=NULL  )*/
         free_mem(  &(t->momenta)   );
   }/*if(  (op & TT_MOMENTA)&&(t->momenta!=NULL)  )*/

   /*Ok, external part and name we steal from reduced topology since it is always present*/
   if( (retcode=l_pTOPOL2singletop( topol->topology,t,topol->max_vertex,
                             op & (TT_NEL|TT_NIL|TT_NV|TT_EXT|TT_RED|TT_NAME)))<0 )
        goto fail_return;
   /*Now cope usrTopol:*/
   if(op & TT_USR){
      if( (retcode=l_pTOPOL2singletop( topol->orig,t,-1,/*Does not a matter*/ TT_USR))<0 )
         goto fail_return;
   }/*if(op & TT_USR)*/

   /*Deternime nel, nil and nv to be independent on operation:*/
   /*Reduced topology always exists:*/
   nel=(topol->topology)->e_n;
   nil=(topol->topology)->i_n;

   nv=topol->max_vertex;
   nm=topol->n_momenta;
   /*We will use determined values only under correspondin op.
     Hence, it is safety against possible inconsistensies bound to
     mixing external/ internal topologies.*/

   /*Substitutions must be defined before momenta:*/
   if( op & TT_S_RED_USR){
      TT_INT *buf;
      free_mem(  &(t->l_usr2red)  );
      free_mem(  &(t->l_red2usr)  );
      if( topol->l_subst!=NULL){
         /*Attention! Here TT_INT is assumed to coincide with char:*/
         t->l_usr2red=new_str(topol->l_subst);
         if( (buf=calloc(nil+2,sizeof(TT_INT)))==NULL){
            retcode=TT_NOTMEM;
            goto fail_return;
         }/*if(buf==NULL)*/
         *(t->l_red2usr=tt_invertsubstitution(buf, topol->l_subst))=-1;
      }/*if( topol->l_subst!=NULL)*/

      free_mem(  &(t->v_usr2red)  );
      free_mem(  &(t->v_red2usr)  );
      if( topol->v_subst!=NULL){
         /*Attention! Here TT_INT is assumed to coincide with char:*/
         t->v_usr2red=new_str(topol->v_subst);
         if( (buf=calloc(nv+2,sizeof(TT_INT)))==NULL){
            retcode=TT_NOTMEM;
            goto fail_return;
         }/*if(buf==NULL)*/
         *(t->v_red2usr=tt_invertsubstitution(buf, topol->v_subst))=-1;
      }/*if( topol->v_subst!=NULL)*/
   }/*if( op & TT_S_RED_USR)*/

   if( op & TT_S_USR_NUSR){/*Just INITIALIZATION!*/
      TT_INT tmp=nv+2;
      free_mem(  &(t->v_nusr2usr)  );
      free_mem(  &(t->v_usr2nusr)  );
      if(
          (  (t->v_nusr2usr=(TT_INT *)calloc(tmp,sizeof(TT_INT)))==NULL )||
          (  (t->v_usr2nusr=(TT_INT *)calloc(tmp,sizeof(TT_INT)))==NULL )
        ){
          retcode=TT_NOTMEM;goto fail_return;
      }/*if( (t->v_nusr2usr=(TT_INT *)calloc(tmo,sizeof(TT_INT)))==NULL )*/
      *(t->v_nusr2usr)=*(t->v_usr2nusr)=-1;
      tmp--;
      for(i=1;i<tmp;i++)
         (t->v_nusr2usr)[i]=(t->v_usr2nusr)[i]=i;
      (t->v_nusr2usr)[i]=(t->v_usr2nusr)[i]=0;

      tmp=nil+2;
      free_mem(  &(t->l_nusr2usr)  );
      free_mem(  &(t->l_usr2nusr)  );
      if(
          (  (t->l_nusr2usr=(TT_INT *)calloc(tmp,sizeof(TT_INT)))==NULL )||
          (  (t->l_usr2nusr=(TT_INT *)calloc(tmp,sizeof(TT_INT)))==NULL )
        ){
          retcode=TT_NOTMEM;goto fail_return;
      }/*if( (t->v_nusr2usr=(TT_INT *)calloc(tmo,sizeof(TT_INT)))==NULL )*/
      *(t->l_nusr2usr)=*(t->l_usr2nusr)=-1;
      tmp--;
      for(i=1;i<tmp;i++)
         (t->l_nusr2usr)[i]=(t->l_usr2nusr)[i]=i;
      (t->l_nusr2usr)[i]=(t->l_usr2nusr)[i]=0;
   }/*if( op & TT_S_USR_NUSR)*/

   if(  (op & TT_D_USR_RED)&&(topol->l_dir!=NULL)  ){
      free_mem(  &(t->l_dirUsr2Red)   );
      t->l_dirUsr2Red=new_str(topol->l_dir);/*Here not TT_INT, just char!*/
   }/*if(  (op & TT_D_USR_RED)&&(topol->l_dir!=NULL)  )*/

   if(op & TT_D_USR_NUSR){/*-1,1,1,1, etc*/
      TT_INT tmp=nil+2;
      free_mem(  &(t->l_dirUsr2Nusr)  );
      if(   ((t->l_dirUsr2Nusr)=calloc(tmp,sizeof(char)))==NULL   ){
         retcode=TT_NOTMEM;goto fail_return;
      }/*if(   (l_dirUsr2Nusr=calloc(tmp,sizeof(char)))==NULL   )*/
      tmp--;
      for(i=1;i<tmp;i++)(t->l_dirUsr2Nusr)[i]=1;
      (t->l_dirUsr2Nusr)[i]=0;
      *(t->l_dirUsr2Nusr)=-1;
   }/*if(op & TT_D_USR_NUSR)*/

   if( op & TT_EXT_MOMENTA){
      int k;
      if(  ( (topol->ext_momenta)!=NULL) && (nm>0)  ){
         /*Allocate and copy external momenta texts:*/
         for(i=0;i<nm; i++){
            if(  (topol->ext_momenta)[i]!=NULL  ){

               if(t->extMomenta==NULL){
                  /*Allocate external momenta array:*/
                  if( /*NOTE! not malloc, must be initialized by NULLs:*/
                     (   t->extMomenta=(char ***)calloc(nm, sizeof(char **) )   )
                     ==NULL
                  ){
                     retcode=TT_NOTMEM;
                     goto fail_return;
                  }/*if(...)*/
               }/*if(t->extMomenta==NULL)*/

               if(
                  (
                     (t->extMomenta)[i]=(char **)malloc( sizeof(char *)*nel )
                  )==NULL
               ){
                   retcode=TT_NOTMEM;
                  goto fail_return;
               }/*if(...)*/
               for(j=0,k=1; j<nel;k++,j++)
                  ( (t->extMomenta)[i] )[j]=
                              tg_get_text_from_group(/*Allocates buffer itself*/
                                   ( (topol->ext_momenta)[i] )[k],1
                              );
            }/*if(  (topol->ext_momenta)[i]!=NULL  )*/
         }/*for(i=0;i<nm; i++)*/
      }/*if(  (topol->ext_momenta)!=NULL  )*/
   }/*if( op & TT_EXT_MOMENTA)*/

   if( op & TT_MOMENTA){
      int k;
      if(  ((topol->momenta)!=NULL) && (nm>0)   ){
         /*Allocate  momenta array:*/
         if(
          (   t->momenta=(char ***)malloc( sizeof(char **)*(nm) )   )
          ==NULL
         ){
            retcode=TT_NOTMEM;
            goto fail_return;
         }/*if(...)*/

         /*Allocate and copy  momenta texts:*/
         for(i=0;i<nm; i++){
            if(  (topol->momenta)[i]!=NULL  ){
               if(
                  (
                     (t->momenta)[i]=(char **)malloc( sizeof(char *)*nil )
                  )==NULL
               ){
                   retcode=TT_NOTMEM;
                  goto fail_return;
               }/*if(...)*/
               for(j=0,k=1; j<nil; k++,j++)
                  ( (t->momenta)[i] )[j]=
                              tg_get_text_from_group(/*Allocates buffer itself*/
                                   ( (topol->momenta)[i] )[topol->l_subst[k]],
                                   topol->l_dir[k]
                              );
            }/*if(  (topol->momenta)[i]!=NULL  )*/
         }/*for(i=0;i<nm; i++)*/
      }/*if(  (topol->momenta)!=NULL  )*/
   }/*if( op & TT_MOMENTA)*/

   if( op & TT_NMOMENTA)
      t->nmomenta=topol->n_momenta;

   /*momenta are ready*/

   if(op & TT_EXT_COORD){/*Orderings in t-ext and topol->e are reverse!*/
      /*Clear old values*/
      free_mem(&(t->ext_llbl));
      free_mem(&(t->ext_line));
      free_mem(&(t->ext_vlbl));
      free_mem(&(t->ext_vert));

      if(nel>0){
         int tmp=nel*2;/*Pairs!!*/
         if(topol->ell != NULL){/*external line labels*/
            if(  (t->ext_llbl=(double *)malloc(tmp*sizeof(double)))==NULL  ){
               retcode=TT_NOTMEM;
               goto fail_return;
            }/*if(  (t->ext_llbl=(double *)malloc(tmp*sizeof(double)))==NULL  )*/
            for(i=0,j=tmp-2; i<tmp; i+=2,j-=2){
               t->ext_llbl[i]=topol->ell[j];
               t->ext_llbl[i+1]=topol->ell[j+1];
            }/*for(i=0,j=tmp-2; i<tmp; i+=2,j-=2)*/
         }/*if(topol->ell != NULL)*/
         if(topol->el != NULL){/*external lines*/
            if(  (t->ext_line=(double *)malloc(tmp*sizeof(double)))==NULL  ){
               retcode=TT_NOTMEM;
               goto fail_return;
            }/*if(  (t->ext_llbl=(double *)malloc(tmp*sizeof(double)))==NULL  )*/
            for(i=0,j=tmp-2; i<tmp; i+=2,j-=2){
               t->ext_line[i]=topol->el[j];
               t->ext_line[i+1]=topol->el[j+1];
            }/*for(i=0,j=tmp-2; i<tmp; i+=2,j-=2)*/
         }/*if(topol->el != NULL)*/
         if(topol->evl != NULL){/*external vertex labels*/
            if(  (t->ext_vlbl=(double *)malloc(tmp*sizeof(double)))==NULL  ){
               retcode=TT_NOTMEM;
               goto fail_return;
            }/*if(  (t->ext_vlbl=(double *)malloc(tmp*sizeof(double)))==NULL  )*/
            for(i=0,j=tmp-2; i<tmp; i+=2,j-=2){
               t->ext_vlbl[i]=topol->evl[j];
               t->ext_vlbl[i+1]=topol->evl[j+1];
            }/*for(i=0,j=tmp-2; i<tmp; i+=2,j-=2)*/
         }/*if(topol->evl != NULL)*/
         if(topol->ev != NULL){/*external vertices*/
            if(  (t->ext_vert=(double *)malloc(tmp*sizeof(double)))==NULL  ){
               retcode=TT_NOTMEM;
               goto fail_return;
            }/*if(  (t->ext_vert=(double *)malloc(tmp*sizeof(double)*nel))==NULL  )*/
            for(i=0,j=tmp-2; i<tmp; i+=2,j-=2){
               t->ext_vert[i]=topol->ev[j];
               t->ext_vert[i+1]=topol->ev[j+1];
            }/*for(i=0,j=tmp-2; i<tmp; i+=2,j-=2)*/
         }/*if(topol->el != NULL)*/
      }/*if(nel>0)*/
   }/*if(op & TT_EXT_COORD)*/

   if( op & TT_INT_COORD){

      /*Clear old values:*/
      free_mem(&(t->int_llbl));
      free_mem(&(t->int_line));
      free_mem(&(t->int_vlbl));
      free_mem(&(t->int_vert));

      if(nil>0){
         int tmp=nil*2;/*Pairs!!*/
         if(topol->ill != NULL){/*internal line labels*/
            if(  (t->int_llbl=(double *)malloc(sizeof(double)*tmp))==NULL  ){
               retcode=TT_NOTMEM;
               goto fail_return;
            }/*if(  (t->int_llbl=(double *)malloc(sizeof(double)*tmp))==NULL  )*/
            for(i=0; i<tmp; i++)t->int_llbl[i]=topol->ill[i];
         }/*if(topol->ill != NULL)*/
         if(topol->il != NULL){/*internal lines*/
            if(  (t->int_line=(double *)malloc(sizeof(double)*tmp))==NULL  ){
               retcode=TT_NOTMEM;
               goto fail_return;
            }/*if(  (t->int_llbl=(double *)malloc(sizeof(double)*tmp))==NULL  )*/
            for(i=0; i<tmp; i++)t->int_line[i]=topol->il[i];
         }/*if(topol->el != NULL)*/
      }/*if(nil>0)*/
      if(nv>0){
          int tmp=nv*2;/*Pairs!!*/
         if(topol->ivl != NULL){/*internal vertex labels*/
            if(  (t->int_vlbl=(double *)malloc(sizeof(double)*tmp))==NULL  ){
               retcode=TT_NOTMEM;
               goto fail_return;
            }/*if(  (t->int_vlbl=(double *)malloc(sizeof(double)*tmp))==NULL  )*/
            for(i=0; i<tmp; i++)t->int_vlbl[i]=topol->ivl[i];
         }/*if(topol->ivl != NULL)*/
         if(topol->iv != NULL){/*internal vertices*/
            if(  (t->int_vert=(double *)malloc(sizeof(double)*tmp))==NULL  ){
               retcode=TT_NOTMEM;
               goto fail_return;
            }/*if(  (t->int_vert=(double *)malloc(sizeof(double)*nv))==NULL  )*/
            for(i=0; i<tmp; i++)t->int_vert[i]=topol->iv[i];
         }/*if(topol->il != NULL)*/
      }/*if(nv>0)*/
   }/*TT_INT_COORD*/

   if(op & TT_REM){
      if(t->remarks != NULL)/*Clear old values*/
         killTopolRem( &(t->top_remarks),&(t->remarks) );
      if( (topol->remarks)!=NULL )
          copyTopolRem( topol->top_remarks,topol->remarks,&(t->top_remarks),&(t->remarks) );
   }/*if(op & TT_REM)*/
   return 0;

   fail_return:
      tt_clearSingleTopology(t);
      return retcode;
}/*tt_topol2singletop*/

