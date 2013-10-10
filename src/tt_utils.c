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

/*tt_utils.c*/

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

/*Length of the buffer for a momenta line building:*/
#define MAX_BUF_LEN 256

#ifndef NUM_STR_LEN
#define NUM_STR_LEN 15
#endif

extern tt_table_type **tt_table;
extern int tt_top;
extern int tt_max;

int tt_chktbl(int *table);
int tt_chktop(int *table, long *top);
int tt_reduceTopology(tt_singletopol_type *topology);
tt_singletopol_type *tt_initSingleTopology(void);
void tt_clearSingleTopology(tt_singletopol_type *t);
char *tt_top2str(char *buf,int nel,tt_line_type *extPart,int nil,tt_line_type *intPart);
int tt_str2top(char *str,tt_singletopol_type *topology);
/*Concatenates s1 and s2 with memory rellocation for s1:*/
char *s_inc(char *s1, char *s2);

/*Returns total number of remarks in topology, or TT_NOTFOUND if fails :*/
int tt_howmanyRemarks(int table, long top)
{
tt_singletopol_type *t;
   if(tt_chktop(&table,&top) ) return TT_NOTFOUND;
   if(  (t=(tt_table[table]->topols)[top]) == NULL )return TT_NOTFOUND;
   return(howmanyRemarks(t->top_remarks,t->remarks));
}/*tt_howmanyRemarks*/

/*mask is OR'ed ids: INT_VERT INT_VLBL INT_LINE INT_LLBL EXT_VERT EXT_VLBL
  EXT_LINE EXT_LLBL. The function returns 0 if all mentioned types of coordinates
  are present, error code <0, or 1, if there are no some of required coordinates.
  It DOES NOT consider "momentaOrShape" filed or special remarks:
 */
int tt_coordinatesOk(int table, long top,int mask)
{
tt_singletopol_type *t;
   if(tt_chktop(&table,&top) ) return TT_NOTFOUND;
   if(  (t=(tt_table[table]->topols)[top]) == NULL )return TT_NOTFOUND;
   if(  (t->int_vert==NULL)&&( mask & INT_VERT )  ) return 1;
   if(  (t->int_vlbl==NULL)&&( mask & INT_VLBL )  ) return 1;
   if(  (t->int_line==NULL)&&( mask & INT_LINE )  ) return 1;
   if(  (t->int_llbl==NULL)&&( mask & INT_LLBL )  ) return 1;
   if(  (t->ext_vert==NULL)&&( mask & EXT_VERT )  ) return 1;
   if(  (t->ext_vlbl==NULL)&&( mask & EXT_VLBL )  ) return 1;
   if(  (t->ext_line==NULL)&&( mask & EXT_LINE )  ) return 1;
   if(  (t->ext_llbl==NULL)&&( mask & EXT_LLBL )  ) return 1;
   return 0;
}/*tt_coordinatesOk*/

/*Returns the name of the remark number "num". "num" is counted from 0*/
char *tt_nameOfRemark(char *buf,int table, long top, unsigned int num)
{
tt_singletopol_type *t;

   if(
      (tt_chktop(&table,&top) ) ||
      (  (t=(tt_table[table]->topols)[top]) == NULL )||
      /* Use "table" as a remarks index:*/
      (  ( table=indexRemarks(t->top_remarks,t->remarks,num) )<0  )
     )  *buf='\0';
   else/*Note if indexRemarks >=0, then (t->remarks)[num].name!=NULL*/
     s_let( (t->remarks)[table].name, buf);
   return buf;
}/*tt_nameOfRemark*/

/*Returns the value of the remark number "num". "num" is counted from 0*/
char *tt_valueOfRemark(char *buf,int table, long top, unsigned int num)
{
tt_singletopol_type *t;

   if(
      (tt_chktop(&table,&top) ) ||
      (  (t=(tt_table[table]->topols)[top]) == NULL )||
      /* Use "table" as a remarks index:*/
      (  ( table=indexRemarks(t->top_remarks,t->remarks,num) )<0  )
     )  *buf='\0';
   else/*Note if indexRemarks >=0, then (t->remarks)[num].text!=NULL*/
     s_let( (t->remarks)[table].text, buf);
   return buf;
}/*tt_valueOfRemark*/

/*Gets remark. Returns the value of the remark in buf, or "" if fails :*/
char *tt_getRemark(char *buf,int table, long top, char *name)
{
tt_singletopol_type *t;
REMARK *tmp;
     if(
      (tt_chktop(&table,&top) )
      ||
      (  (t=(tt_table[table]->topols)[top]) == NULL )
      ||
      ( (tmp=
           lookupRemark(
              t->top_remarks,
              t->remarks,
              name
           )
        ) == NULL )
      ||
      ( tmp->name == NULL )
    )*buf='\0';/*not found*/
  else
     s_let(tmp->text,buf);
  return(buf);
}/*tt_getRemark*/

/*Sets remark. Returns 0, if ok, or or TT_NOTFOUND if fails.
   ATTENTION! Both "name" and "val" must be allocated :*/
int tt_setRemark(int table, long top,char *name, char *val)
{
tt_singletopol_type *t;

   if(tt_chktop(&table,&top) ) return TT_NOTFOUND;
   if(  (t=(tt_table[table]->topols)[top]) == NULL )return TT_NOTFOUND;
/* setTopolRem(char *name, char *newtext,char *arg, word *top_remarks,
   REMARK **remarks, int *ind),  remarks.c
   Sets remark "name " to "newtext". Previous value of "name" is returned in the buffer
   "arg".
   ATTENTION! "val" is ALLOCATED and must be stored as is while "name" will be
   allocated by setTopolRem. This is due to ind==NULL, see comments to "setTopolRem"
   in remarks.h.
  arg is NULL since we wold not like to get the previous value,
  ind is NULL since we wold not like to trace the array:*/

  setTopolRem(name, val,NULL,
                                &(t->top_remarks),
                                &(t->remarks),
                                NULL);
  return 0;
}/*tt_setRemark*/

/*Removes remark. Returns 0, if ok, or or TT_NOTFOUND if fails :*/
int tt_killRemark(int table, long top,char *name)
{
tt_singletopol_type *t;
  if(
      (tt_chktop(&table,&top) )
      ||
      (  (t=(tt_table[table]->topols)[top]) == NULL )
      ||
      (  deleteTopolRem(NULL, t->top_remarks, t->remarks, name) <0 )
    )return TT_NOTFOUND;/*not found*/
  return 0;
}/*tt_killRemark*/

/* Returns number of topologies in the table, or TT_NOTFOUND:*/
long tt_howmany(int table)
{
   if( tt_chktbl(&table) )return TT_NOTFOUND;
   return tt_table[table]->totalN;
}/*tt_howmany*/

/*Returns original(incoming) topology:*/
char *tt_origtopology(char *buf,int table, long top)
{
tt_singletopol_type *t;
   *buf='\0';
   if(tt_chktop(&table,&top) ) return buf;
   if(  (t=(tt_table[table]->topols)[top]) == NULL )return buf;
   return(tt_top2str(buf,t->nel,t->extPart,t->nil,t->usrTopol));
}/*tt_origtopology*/

/*Returns reduced topology:*/
char *tt_redtopology(char *buf,int table, long top)
{
tt_singletopol_type *t;
   *buf='\0';
   if(tt_chktop(&table,&top) ) return buf;
   if(  (t=(tt_table[table]->topols)[top]) == NULL )return buf;
   if(  t->redTopol == NULL ){
      if(t->nil > 0)/*If there are no internal lines at all,
                  then redTopol==NULL, so we can just output external part.*/
         return buf;
   }
   return(tt_top2str(buf,t->nel,t->extPart,t->nil,t->redTopol));
}/*tt_redtopology*/

static char *l_top2str(
                TT_INT *lsubst,/*nusr2usr*/
                TT_INT *vsubst,/*usr2nusr*/
                char *dirs,/*usr2nusr*/
                char *buf,
                int nel,
                tt_line_type *extPart,
                int nil,
                tt_line_type *intPart)
{
   int i,l;
   char *tmp=buf;
      *buf='\0';
      /*External lines -- no substitution for external vertices,
        lo line reordering:*/
      for(i=nel-1;!(i<0);i--){
        sprintf(tmp,"(%d,%d)",extPart[i].from,vsubst[extPart[i].to]);
        while(*++tmp!='\0');
      }
      /*Internal lines -- both vertex and lines substitutions:*/
      for(i=0;i<nil;i++){
         if(  dirs[(l=lsubst[i+1]-1)+1] == '\1')/* Right direction*/
            sprintf(tmp,"(%d,%d)",vsubst[intPart[l].from],
                              vsubst[intPart[l].to]);
         else/*Opposit direction*/
            sprintf(tmp,"(%d,%d)",vsubst[intPart[l].to],
                                  vsubst[intPart[l].from]);

         while(*++tmp!='\0');
      }
      return buf;
}/*l_top2str*/

/*Returns topology reordered by tt_linesReorder/tt_vertexReorder:*/
char *tt_transformedTopology(char *buf,int table, long top)
{
tt_singletopol_type *t;
   *buf='\0';
   if(tt_chktop(&table,&top) ) return buf;
   if(  (t=(tt_table[table]->topols)[top]) == NULL )return buf;
   return(l_top2str(t->l_nusr2usr,
                    t->v_usr2nusr,
                    t->l_dirUsr2Nusr,
                    buf,
                    t->nel,
                    t->extPart,
                    t->nil,
                    t->usrTopol));
}/*tt_transformedTopology*/

/*returns number of internal lines:*/
int tt_ninternal(int table, long top)
{
   if( tt_chktop(&table,&top) ) return TT_NOTFOUND;
   return (tt_table[table]->topols)[top]->nil;
}/*tt_ninternal*/

/*returns number of external lines:*/
int tt_nexternal(int table, long top)
{
   if( tt_chktop(&table,&top)  )return TT_NOTFOUND;
   return (tt_table[table]->topols)[top]->nel;
}/*tt_nexternal*/

/*returns number of vertices:*/
int tt_vertex(int table, long top)
{
   if( tt_chktop(&table,&top)  )return TT_NOTFOUND;
   return (tt_table[table]->topols)[top]->nv;
}/*tt_nexternal*/

/*Low level function, looks up the topology:*/
long tt_lookup(int table, tt_singletopol_type *topology)
{
int retcode;
   if(topology == NULL)return TT_NOTFOUND;
   if( tt_chktbl(&table) )return TT_NOTFOUND;
   if(  (topology->redTopol == NULL)&&
        (retcode=(tt_reduceTopology(topology)<0))
                                      ) return retcode;
   if( (topology=lookup(topology,tt_table[table]->hred))==NULL )return TT_NOTFOUND;
   return topology->n + 1;
}/*tt_lookup*/

/*Reduces the topology top and looks it up in the table:*/
long tt_lookupFromStr(int table, char *top)
{
tt_singletopol_type *t;
int retcode = -1;
   if( (top==NULL)||(table >tt_top )||(table <1)||(tt_table[table-1]==NULL)  )
      return TT_NOTFOUND;
   if(   (t=tt_initSingleTopology())==NULL   )return TT_NOTFOUND;
   if(  (retcode=tt_str2top(top,t)) == 0 )
      retcode=tt_lookup(table,t);
   tt_clearSingleTopology(t);
   return retcode;
}/*tt_lookupFromStr*/

/*Returns number of momenta set:*/
int tt_nmomenta(int table, long top)
{
   if(tt_chktop(&table,&top) ) return TT_NOTFOUND;
   return (tt_table[table]->topols)[top]->nmomenta;
}/*tt_nmomenta*/

/*Expects tokenBegin to be a token ended by '}'. It translates
  the token using HASH_TABLE ht,  stored it into buf, and returns the pointer to the
  tokenBegin at the closing '}'.
  If the token is absent in the table, it returns "".
  The length of the token is returned  to the variable *l, and if *l is >= maxL,
  then the procedure fails: it retunrs NULL. */
static char *l_translateToken(char *tokenBegin, char *buf, HASH_TABLE ht, int *l, int maxL)
{
char *tmp=buf;
   for((*l)=0;*tokenBegin!='}';tokenBegin++){
      if( !(++(*l)<maxL) )return NULL;
      *tmp++=*tokenBegin;
   }
   *tmp='\0';
   *l = 0;
   if(  (tmp=(char *)lookup(buf,ht)) == NULL  )
      *buf='\0';
   else
      while( (*buf++ = *tmp++)!='\0' )if( !(++(*l)<maxL) )return NULL;
   return tokenBegin;
}/*l_translateToken*/

/*Expects txt is a text with tokens of kind
  blah-blah-blah ... {token} ... blah-blah-blah ... {token} ... blah-blah-blah ...
  Replaces all {token} according to the table from the table "table" and puts the result
  into buf. If the token is absent, it will be translated into empty string.
  If fails (built line is too long, >maxLen) returns  TT_TOOLONGSTRING.
  In the latter case (too long line) it correctly terminates "buf":*/
int tt_translateText(char *txt, char *buf,HASH_TABLE ht, int maxLen)
{
char *b=buf;
int tl=0;/*Built length counter*/

   for(;*txt!='\0';txt++){
      if(++tl>maxLen){/*Too long string*/
         *b='\0';/*Newertheless, terminate the buffer!*/
         return TT_TOOLONGSTRING;/*Fail*/
      }/*if(++tl>maxLen)*/
      if(*txt == '{'){/*Well, the token. Translate it:*/
       int l;
          if(
             (txt=l_translateToken(txt+1,b,ht, &l, maxLen-tl))==NULL
          ){ /*Too long string*/
             *b='\0';/*Newertheless, terminate the buffer!*/
             return TT_TOOLONGSTRING;/*Fail*/
          }/*if( (txt=l_translateToken(...))==NULL )*/
          b+=l;/*move building momentum to the end of the token*/
          tl+=(l-1);/*May be negative -- '{' already accounted!*/
      }else{
            *b++=*txt;
      }/*if(...){...}else*/
   }/*for(;*txt!='\0';txt++)*/
   *b='\0';/*terminate the string*/
   return 0;
}/*tt_translateText*/

/*Wrapper to tt_translateText, accepts int table instead of HASH_TABLE ht.
  If fails ( table not found, or built line is too long, >maxLen) returns TT_NOTFOUND
  or TT_TOOLONGSTRING, respectively:*/
int tt_translateTokens(char *txt, char *buf,int table, int maxLen)
{
   if( tt_chktbl(&table) )return TT_NOTFOUND;
   return tt_translateText(txt,buf,tt_table[table]->htrans,maxLen);
}/*tt_translateTokens*/

/* lsubst == new2old: old=lsubst[new], ldir==ldir[old].
   l is the number of internal lines.
   If ht == NULL, then  we assume that the request is for original (untranslated) tokens:*/
static int l_buildMomentaLine(TT_INT *lsubst,char *ldir, HASH_TABLE ht,
                        int maxLen, char *momentum, char ***momenta, int n,int l)
{
char **mm, *m,*tmp=momentum,*mem;
int i,tl=0;
   *momentum='\0';
   if(momenta == NULL)return TT_NOTFOUND;
   if( (mm=momenta[n-1])==NULL)/*we are dealing with empty
                                 momenta set, e.g., external momenta.*/
      return TT_NOTFOUND;
   maxLen-=1;
   for(i=0;i<l;i++){
      /*lsubst=;-1,i,j,k,...] so that old n= lsubst[new n]
        where n is counted from 1 :*/
      if( (m=mm[lsubst[i+1]-1])==NULL )
         m="";/*To avoid possible crash in case some inconsistensies*/
      /*ldir[old] defines the OLD line direction, so
        we must use ldit[lsubst[old]] where old is from 1:*/
      if(
          ldir[lsubst[i+1]]==-1 /*Opposit direction*/
                            ){
         switch(*m){
            case '\0':
            case '-':
            case '+':break;/*the first sign will be changed by swapCharSng*/
            default: *tmp++ = '-';break;/*'+' was implicit, so change
                                           the sign by hand*/
         }/*switch*/
         /*store the beginning of the momentum without leading '-' if set by hand:*/
         mem=tmp;
      }else
         mem=NULL;/*use it as an indicator*/
      for(*tmp=*m;*m!='\0';m++){
         if(++tl>maxLen){
            *momentum='\0';
            return TT_TOOLONGSTRING;
         }/*if(++tl>maxLen)*/
         if(
             (ht!=NULL)&& /*Token translation required*/
             (*m == '{')/*Well, the token. Translate it:*/
            ){ int l;
               if(
                   (m=l_translateToken(m+1,tmp,ht, &l, maxLen-tl))==NULL
                 ){
                    *momentum='\0';
                    return TT_TOOLONGSTRING;
               }
               tmp+=l;/*move building momentum to the end of the token*/
               tl+=(l-1);/*May be negative -- '{' already accounted!*/
         }else{
            *tmp++=*m;
         }/*if(...){...}else*/
      }/*for(*tmp=*m;*m!='\0';)*/
      if(mem!=NULL){/*We must change the direction...*/
         *tmp='\0';/*First, terminate the string*/
         swapCharSng(mem);/*mem must points to the beginning of the momentum. If
                        the leading '+' was implicit, then mem points after it.*/
      }
      *tmp++=',';tl++;
   }/*for(i=0;i<l;i++)*/
   if(tl>0)*--tmp='\0';/*Remove trailing comma*/
   return 0;
}/*l_buildMomentaLine*/

static int l_alloctmpsubst(int l, TT_INT ** lsubst,char **ldir, int *err)
{
   if(  (*lsubst=(TT_INT *)calloc(l+2,sizeof(TT_INT *)))==NULL){
      *err=TT_NOTMEM;return(-1);
   }
   if(ldir==NULL)return 0;

   if(  (*ldir=(char *)calloc(l+2,sizeof(char *)))==NULL){
      free (*lsubst);
      *err=TT_NOTMEM;return(-1);
   }
   return 0;
}/*l_alloctmpsubst*/

/*Low level:
  kind=0...3:   momenta, nusrmomenta, extMomenta, extNusrmomenta
  table and top are indices, NOT user numbers! This procedure does NOT
  perform any checkup about table and top! :
  */
char *tt_momenta(char *buf,int kind, int table, long top,int n,int *err)
{
int l,i,j;
char ***tmp,*ldir;
TT_INT *lsubst;
HASH_TABLE ht;
   if(
      n> (tt_table[table]->topols)[top]->nmomenta
                                      ){
      *err=TT_NOTFOUND;
      return NULL;
   }
   switch(kind){
      case 0:/*momenta*/
         tmp=(tt_table[table]->topols)[top]->momenta;
         l=(tt_table[table]->topols)[top]->nil;
         if(l_alloctmpsubst(l, &lsubst,&ldir, err))return NULL;
         ht=NULL;/*Do not translate tokens*/
         for(i=1;!(i>l);i++){
            lsubst[i]=i;
            ldir[i]='\1';
         }

         break;
      case 1:/*nusrmomenta*/
         tmp=(tt_table[table]->topols)[top]->momenta;
         l=(tt_table[table]->topols)[top]->nil;
         lsubst=(tt_table[table]->topols)[top]->l_nusr2usr;
         ldir=(tt_table[table]->topols)[top]->l_dirUsr2Nusr;
         ht=tt_table[table]->htrans;
         break;
      case 2:/*extMomenta*/
      case 3:/*extNusrmomenta*/
         tmp=(tt_table[table]->topols)[top]->extMomenta;
         l=(tt_table[table]->topols)[top]->nel;
         if(l_alloctmpsubst(l, &lsubst,&ldir, err))return NULL;
         for(j=l,i=1;j>0;i++,j--){
            lsubst[i]=j;
            ldir[i]='\1';
         }
         if(kind==2)/*extMomenta*/
            ht=NULL;/*Do not translate tokens*/
         else/*extNusrmomenta*/
            ht=tt_table[table]->htrans; /*Translate tokens*/
         break;
      default:return NULL;
   }/*switch(kind)*/
   if(   (*err=l_buildMomentaLine(lsubst,ldir,ht,
                                   MAX_BUF_LEN, buf, tmp, n, l)   )<0  ){
      buf=NULL;
   }
   if(*lsubst == 0 ){/*0 must be set by calloc; in the table we have -1*/
      free(lsubst);
      free(ldir);
   }
   return buf;
}/*tt_momenta*/

/* l_getMomenta ALWAYS return allocated string, may be, "".
   isOrig==0 -> translated nusr; otherwise, original, i.e. non-translated usr
   "table" and "top" are indices, NOT user numbers! This procedure does NOT
   perform any checkup about table and top! */
char *l_getMomenta(int isOrig,int table, long top,int n,int *err)
{
char *ret=NULL;
char buf[MAX_BUF_LEN];
int saverr=*err;
   if(isOrig) isOrig= 2; else isOrig=3;
   /*What about external part?:*/
   if( tt_momenta(buf,isOrig,table,top,n,err)!=NULL ){/*External momenta are present*/
       ret=new_str("[");
       ret=s_inc(s_inc(ret,buf),"]");
   }else{/*External momenta are absent*/
       *err=saverr;/*tt_momenta canged it to not_found!*/
       ret=new_str("");
   }
   if( tt_momenta(buf,isOrig-2,table,top,n,err)!=NULL )/*Internal momenta are present*/
       ret=s_inc(ret,buf);
   else
      *err=saverr;/*tt_momenta canged it to not_found!*/
   return ret;
}/*l_getMomenta*/

/*tt_getMomenta ALWAYS return allocated string, may be, "".
   isOrig==0 -> translated nusr; otherwise, original, i.e. non-translated usr
   This is just a wrapper to l_getMomenta:*/
char *tt_getMomenta(int isOrig,int table, long top,int n,int *err)
{

   /*Check and reduce table and top here, neither l_getMomenta nor tt_momenta will
     NOT check them:*/
   if( tt_chktop(&table,&top) ){
      *err=TT_NOTFOUND;
      return new_str("");
   }/*if( tt_chktop(&table,&top) )*/
   return l_getMomenta(isOrig,table,top,n,err);
}/*tt_getMomenta*/

TT_INT *tt_l_usr2red(int table, long top)
{
   if( tt_chktop(&table,&top)  )return NULL;
   return (tt_table[table]->topols)[top]->l_usr2red;
}/*tt_l_usr2red*/

TT_INT *tt_v_usr2red(int table, long top)
{
   if( tt_chktop(&table,&top)  )return NULL;
   return (tt_table[table]->topols)[top]->v_usr2red;
}/*tt_v_usr2red*/

TT_INT *tt_l_dirUsr2Red(int table, long top)
{
   if( tt_chktop(&table,&top)  )return NULL;
   return (tt_table[table]->topols)[top]->l_dirUsr2Red;
}/*tt_l_dirUsr2Red*/

/*returns 1 if it stores new token (i.e., such token is absent in the table),
  TT_NOTFOUND if there is no such a table, 0 if it replaces existing token:*/
int tt_translate(char *token, char *translation,int table)
{
   if( tt_chktbl(&table) )return TT_NOTFOUND;
   switch(
            install(  new_str(token),
                      new_str(translation),
                      tt_table[table]->htrans
                   )
         ){
      case -1:return TT_NONFATAL;/*The table does not exists. Why?*/
      case  1:return 0;
      case  0:return 1;
   }/*switch*/
   return TT_FATAL;/*Can not be...*/
}/*tt_translate*/

/*Looks up the token in the translation table and returns it.
  WARNING: it returns a pointe to the table, it belongs to the table,
  newer manipulate them!*/
char *tt_lookupTranslation(char *token, int table)
{
   if( tt_chktbl(&table) )return NULL;
   return (char *) lookup(token, tt_table[table]->htrans);
}/*tt_lookupTranslation*/

/*Auxiliarry routine copies beginning of str up to ser or '\0' into buf
  (mot more then max characters), returns a pointer to the next after
  sep in str, or NULL:*/
static char *l_nextToken(char sep, char *str, char *buf, int max)
{
char *tmp=buf;
int i=0;
   for(*tmp='\0';i<max;i++,str++,tmp++){
      if(*str == sep ){
         *tmp='\0';return str+1;
      }
      if( (*tmp=*str)=='\0' )
         return NULL;
   }/*for(;i<max;i++,str++,tmp++)*/
   return NULL;
}/*l_nexToken*/

/*returns 0 if substitution was implemented, otherwise returns error code:*/
static int l_str2subst(char sep, int max, char *str, TT_INT *subst, char *dir)
{
char buf[NUM_STR_LEN];
TT_INT *ch;/*for checking*/
int itmp,i=0,mmax=max+2;
   if(  !(max>0)  ) return TT_INVALIDSUBS;
   if(  (ch=calloc(mmax,sizeof(TT_INT)))==NULL)
      return TT_NOTMEM;
   do{
      if(++i > max)break;
      str=l_nextToken(sep,str,buf,NUM_STR_LEN);
      if(sscanf(buf,"%d",&itmp)!=1){i=-1;break;}
      if(itmp<0){
         itmp=-itmp;
         if(dir!=NULL)dir[i]=-1;
      }else{
         if(dir!=NULL)dir[i]=1;
      }
      if( (itmp>max)||(itmp == 0) ){i=-1;break;}
      ch[subst[i]=itmp]++;
   }while(str!=NULL);
   /*The number of elements == i must be equal max*/
   if(i==max){
      for(i=1;!(i>max);i++)if(ch[i]!=1)i=max+2;/*instead of break*/
      i--;
   }
   free(ch);/*Not needed anymore*/
   if(i!=max)
      return TT_INVALIDSUBS;
   return 0;
}/*str2subst*/

static char tt_sep=':';
/*Sets new separator for line/vertex substitutions:*/
void tt_setSep(char sep)
{
   tt_sep=sep;
}/*tt_setSep*/
/*Returns current separator for line/vertex substitutions:*/
char tt_getSep(void)
{
   return tt_sep;
}/*tt_getSep*/

/*!Previous substitution is NOT taken into account!
  Reorders internal lines enumerating. Pattern:
  positions -- old, values -- new, separated by tt_sep, e.g.
  "2:1:-3:5:-4" means that line 1 > 2, 2 ->1, 3 just upsets,
  4->5, 5 upsets and becomes of number 4.
  Returns 0 if substitution was implemented, or error code, if fails.
  In the latter case substitutions will be preserved:*/
int tt_linesReorder(char *str,  int table, long top )
{
tt_singletopol_type *t;
TT_INT *tmps;
char *tmpd;
int i,max;

   if( tt_chktop(&table,&top)  )return TT_NOTFOUND;
   t=tt_table[table]->topols[top];

   if( (t->nil) <= 0 )return 0;

   max=(t->nil)+2;
   /*Allocate temporary arrays to restore substitutions on fail:*/
   if(   (tmps=(TT_INT *)calloc(max,sizeof(TT_INT)))==NULL  )return TT_NOTMEM;
   if(   (tmpd=(char *)calloc(max,sizeof(char)))==NULL  ){
      free(tmps);
      return TT_NOTMEM;
   }
   /*Copy original substitutions to temporary ones:*/
   for(i=1;i<=(t->nil);i++){
      tmpd[i]=t->l_dirUsr2Nusr[i];
      tmps[i]=t->l_usr2nusr[i];
   }
   tmpd[i]='\0';
   tmps[i]=0;

   /*Try to get new substitutions, the exitcode will be saved to max:*/
   if(  (max=l_str2subst(tt_sep,t->nil,str, t->l_usr2nusr,t->l_dirUsr2Nusr))<0){
      /*Fail! Restore old values:*/
      for(i=1;i<=(t->nil);i++){
         t->l_dirUsr2Nusr[i]=tmpd[i];
         t->l_usr2nusr[i]=tmps[i];
      }
      t->l_dirUsr2Nusr[i]='\0';
      t->l_usr2nusr[i]=0;
   }else/*Success. Build invert substitution:*/
      invertsubstitution(t->l_nusr2usr, t->l_usr2nusr);
   free(tmpd);
   free(tmps);
   return max;/*exit code returned by l_str2subst*/
}/*tt_linesReorder*/

/*!Previous substitution is NOT taken into account!
  Reorders  vertex enumerating. Pattern:
  positions -- old, values -- new, separated by tt_sep, e.g.
  "2:1:3:5:4" means that vertices 1 > 2, 2 ->1, 3->3,
  4->5, 5 ->4.
  Returns 0 if substitution was implemented, or error code, if fails.
  In the latter case the vertex substitution will be preserved:*/
int tt_vertexReorder(char *str,  int table, long top )
{
tt_singletopol_type *t;
TT_INT *tmps;
int i,max;

   if( tt_chktop(&table,&top)  )return TT_NOTFOUND;
   if(  (t=tt_table[table]->topols[top])==NULL  )return TT_NOTFOUND;

   if( (t->nv) <= 0 )return 0;

   max=(t->nv)+2;
   /*Allocate temporary arrays to restore substitutions on fail:*/
   if(   (tmps=(TT_INT *)calloc(max,sizeof(TT_INT)))==NULL  )return TT_NOTMEM;
   /*Copy original substitutions to temporary ones:*/
   for(i=1;i<=(t->nv);i++)
      tmps[i]=t->v_usr2nusr[i];
   tmps[i]=0;

   /*Try to get new substitutions, the exitcode will be saved to max:*/
   if(  (max=l_str2subst(tt_sep,t->nv,str, t->v_usr2nusr,NULL))<0){
      /*Fail! Restore old values:*/
      for(i=1;i<=(t->nv);i++)
         t->v_usr2nusr[i]=tmps[i];
      t->v_usr2nusr[i]=0;
   }else/*Success. Build invert substitution:*/
      invertsubstitution(t->v_nusr2usr, t->v_usr2nusr);
   free(tmps);
   return max;/*exit code returned by l_str2subst*/
}/*tt_vertexReorder*/

/*Auxiliary routine, used by tt_getCoords. Stores string representation of
  double *coords re-positioned according to TT_INT *subst into char *buf.
  If subst[0]==0 then frees subst.
  maxLen is the length of the buf.
  ATTENTION! Checks possible overflow only before processing each pair!(MUST BE FIXED!)
  Returns length of created string, or error code <0:*/
static  int l_double2str(char *buf, int maxLen, int n, TT_INT *subst,double *coords)
{
int i,j;
char *bb=buf;/*Store the beginning of the buffer*/
   if( (maxLen-=8) < 0)return 0;/*8=length("0.0,0.0")+1*/
   *buf='\0';
   if(subst==NULL){
      if(coords!=NULL)
         return TT_INVALIDSUBS;
      return 0;/*Nothing to do, coordinates are absent!*/
   }/*if(subst==NULL)*/
   if(coords!=NULL)for(i=1; !(i>n); i++){
      if(buf-bb>maxLen)return buf-bb;
      j=(subst[i]-1)*2;
      if(i<n)
         sprintf(buf,"%.1f,%.1f,",coords[j],coords[j+1]);
      else
         sprintf(buf,"%.1f,%.1f",coords[j],coords[j+1]);
      while(*++buf);
   }
   if( *subst == 0 )/*Was allocated "by hand"*/
      free(subst);
   return buf-bb;
}/*l_double2str*/

static int l_createExternalSubst(int l, TT_INT **subst)
{
TT_INT di,i,s;
   if(l<0){
      l=-l;di=-1;s=l;
   }else if (l == 0){
      (*subst)=NULL;return 0;
   }else {
      di=1;s=1;
   }
   if(  ((*subst)=(TT_INT *)calloc(l+2,sizeof(TT_INT *)))==NULL)return TT_NOTMEM;

   for(i=1;!(i>l);i++){
      (*subst)[i]=s;
      s+=di;
   }
   (*subst)[i]=0;
   return 0;
}/*l_createExternalSubst*/

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
   Returns length of created string, or error code <0

   l_getCoords is the main routine, it does not check table and top.
   tt_getCoords is a wrapper.
  */
int l_getCoords( char *buf,int maxLen,int table, long top, int n)
{
tt_singletopol_type *t;
TT_INT *subst;
   if(  (t=(tt_table[table]->topols)[top]) == NULL )return TT_NOTFOUND;
   switch (n){  /*For external coordinates we create special substitution passing
                  negative number to l_createExternalSubst*/
      case EXT_VERT:
      case ORIG_EXT_VERT:
         if(l_createExternalSubst( -(t->nel), &subst))return TT_NOTMEM;
         return l_double2str(buf,maxLen, t->nel, subst,t->ext_vert);
      case EXT_VLBL:
      case ORIG_EXT_VLBL:
         if(l_createExternalSubst( -(t->nel), &subst))return TT_NOTMEM;
         return l_double2str(buf,maxLen, t->nel, subst,t->ext_vlbl);
      case EXT_LINE:
      case ORIG_EXT_LINE:
         if(l_createExternalSubst( -(t->nel), &subst))return TT_NOTMEM;
         return l_double2str(buf,maxLen, t->nel, subst,t->ext_line);
      case EXT_LLBL:
      case ORIG_EXT_LLBL:
         if(l_createExternalSubst( -(t->nel), &subst))return TT_NOTMEM;
         return l_double2str(buf,maxLen, t->nel, subst,t->ext_llbl);
      case INT_VERT:
         return l_double2str(buf,maxLen, t->nv, t->v_nusr2usr,t->int_vert);
      case INT_VLBL:
         return l_double2str(buf,maxLen, t->nv, t->v_nusr2usr,t->int_vlbl);
      case INT_LINE:
         return l_double2str(buf,maxLen, t->nil, t->l_nusr2usr,t->int_line);
      case INT_LLBL:
         return l_double2str(buf,maxLen, t->nil, t->l_nusr2usr,t->int_llbl);
      case ORIG_INT_VERT:
         if(l_createExternalSubst(t->nv, &subst))return TT_NOTMEM;
         return l_double2str(buf,maxLen, t->nv, subst,t->int_vert);
      case ORIG_INT_VLBL:
         if(l_createExternalSubst(t->nv, &subst))return TT_NOTMEM;
         return l_double2str(buf,maxLen, t->nv, subst,t->int_vlbl);
      case ORIG_INT_LINE:
         if(l_createExternalSubst(t->nil, &subst))return TT_NOTMEM;
         return l_double2str(buf,maxLen, t->nil, subst,t->int_line);
      case ORIG_INT_LLBL:
         if(l_createExternalSubst(t->nil, &subst))return TT_NOTMEM;
         return l_double2str(buf,maxLen, t->nil, subst,t->int_llbl);
      default:
         return TT_NOTFOUND;
   }/*switch (n)*/
}/*l_getCoords*/
/*Wrapper:*/
int tt_getCoords( char *buf,int maxLen,int table, long top, int n)
{
   *buf='\0';
   if(tt_chktop(&table,&top) ) return TT_NOTFOUND;
   return l_getCoords(buf,maxLen,table, top, n);
}/*tt_getCoords*/

/*Iterator -- types out a cell in the form 'token name = value'. Must return 0!!! :*/
static int l_printTransIterator(void *info,HASH_CELL *m, word index_in_tbl, word index_in_chain)
{
   return
    (fprintf( (FILE *)info, "%s %s = \"%s\"\n",tt_tokenTrans,(char*)m->tag,(char*)m->cell)<0);
           /*In success, must return FALSE.*/
}/*l_printTransIterator*/

/*id# - further must be changed */
static char *l_get_ttName(char *id,char *buf, int maxLen, tt_singletopol_type *top)
{
  /*ATTENTION!!! This realisation does NOT check possible overflow! */
  sprintf(buf,"%s%ld",id,top->n+1);
  return buf;
}/*l_get_ttName*/

/*Length of the buffer for the topology name:*/
#define NAMEBUFMAX 128
/*outfile must be opened, and will be opened after this function.
  isOrig!=0 -- usr, non-translated; isOrig==0 -- nusr, translated.
  Returns number of saved topologies:*/
long tt_saveTableToFile(FILE *outfile, int table, int isOrig)
{
long i;
int retcode;
char buf[NAMEBUFMAX];
char *id;
   if( tt_chktbl(&table) )return TT_NOTFOUND;
   if(tt_table[table]->totalN == 0 )return 0;
   if(
      fprintf(outfile,"%s = %ld\n",tt_tokenTMax,tt_table[table]->totalN)
      <=0 ) return TT_CANNOT_WRITE;

   if(
      (tt_table[table]->htrans != NULL)&&
      (  (tt_table[table]->htrans)->n > 0 )
      ){
      if(
      fprintf(outfile,"%s = -%u\n",tt_tokenTrMax,(tt_table[table]->htrans)->tablesize)
      <=0) return TT_CANNOT_WRITE;

      if( hash_foreach(tt_table[table]->htrans,(void*)outfile,&l_printTransIterator)!=0 )
         return TT_CANNOT_WRITE;
   }/*if(tt_table[table]->htrans != NULL)*/

   for(i=0; i<tt_table[table]->totalN; i++){
      if(  (tt_table[table]->topols)[i]->nel != 0 )
         id="f";
      else
         id="i";
      if(
           fprintf(outfile,"%s %s = \n   ",tt_tokenTopology,
                                         l_get_ttName(id,buf, NAMEBUFMAX,
                                         (tt_table[table]->topols)[i] )
           )
              <=0
        ) return TT_CANNOT_WRITE;
      if(  (retcode=tt_saveTopology(outfile,table,(tt_table[table]->topols)[i],isOrig ))<0  )
         return retcode;
      fprintf(outfile,"\n");
   }/*for(i=0; i<tt_table[table]->totalN; i++)*/
   return tt_table[table]->totalN;
}/*tt_saveTableToFile*/

/*   "table" and "top" are indices, NOT user numbers! This procedure does NOT
   perform any checkup about table and top! isOrig!=0 -- usr, isOrig==0 -- nusr.
   "sep" is a delimiter printed after each trailing ':'*/
int  l_saveCoordsToFile(
      FILE *outfile,
      char *buf,
      int maxLen,
      int table,
      long top,
      int isOrig,
      char *sep)
{
int howmany=0;/*The problem is that we can't finish the line since we do not know
             which trailing delimiter (':' or '') we must use.  Use this counter to
             finish the line further, and to construct number to be return*/
int retcode=0;/*Used to store return value of l_getCoords*/

   isOrig=(isOrig)?0:1;/*constants for l_getCoords: smth+1 = ORIG_smth, see tt_types.h:*/

   if ( ( retcode=l_getCoords( buf, maxLen, table, top,  EXT_VERT+isOrig) ) <0 )
      return retcode;
   if(retcode){/*Non-empty string is generated */
      /*Finish previous line, it is always present as "coordinates top =":*/
      if( fprintf(outfile,"%s",sep)<=0 )return TT_CANNOT_WRITE;
      howmany++;
      if( fprintf(outfile,"%s %s",tt_tokenEv ,buf) <= 0 )
         return TT_CANNOT_WRITE;
   }/*if(retcode)*/

   if ( ( retcode=l_getCoords( buf, maxLen, table, top,  EXT_VLBL+isOrig) ) <0 )
      return retcode;
   if(retcode){/*Non-empty string is generated */
      /*Finish previous line, if present:*/
      if( (howmany) && (fprintf(outfile,":%s",sep)<=0) )return TT_CANNOT_WRITE;
      howmany++;
      if( fprintf(outfile,"%s %s",tt_tokenEvl ,buf) <= 0 )
         return TT_CANNOT_WRITE;
   }/*if(retcode)*/

   if ( ( retcode=l_getCoords( buf, maxLen, table, top,  EXT_LINE+isOrig) ) <0 )
      return retcode;
   if(retcode){/*Non-empty string is generated */
      /*Finish previous line, if present:*/
      if( (howmany) && (fprintf(outfile,":%s",sep)<=0) )return TT_CANNOT_WRITE;
      howmany++;
      if( fprintf(outfile,"%s %s",tt_tokenEl ,buf) <= 0 )
         return TT_CANNOT_WRITE;
   }/*if(retcode)*/

   if ( ( retcode=l_getCoords( buf, maxLen, table, top,  EXT_LLBL+isOrig) ) <0 )
      return retcode;
   if(retcode){/*Non-empty string is generated */
      /*Finish previous line, if present:*/
      if( (howmany) && (fprintf(outfile,":%s",sep)<=0) )return TT_CANNOT_WRITE;
      howmany++;
      if( fprintf(outfile,"%s %s",tt_tokenEll, buf) <= 0 )
          return TT_CANNOT_WRITE;
   }/*if(retcode)*/

   if ( ( retcode=l_getCoords( buf, maxLen, table, top,  INT_VERT+isOrig) ) <0 )
      return retcode;
   if(retcode){/*Non-empty string is generated */
      /*Finish previous line, if present:*/
      if( (howmany) && (fprintf(outfile,":%s",sep)<=0) )return TT_CANNOT_WRITE;
      howmany++;
      if( fprintf(outfile,"%s %s",tt_tokenIv, buf) <= 0 )
         return TT_CANNOT_WRITE;
   }/*if(retcode)*/

   if ( ( retcode=l_getCoords( buf, maxLen, table, top,  INT_VLBL+isOrig) ) <0 )
      return retcode;
   if(retcode){/*Non-empty string is generated */
      /*Finish previous line, if present:*/
      if( (howmany) && (fprintf(outfile,":%s",sep)<=0) )return TT_CANNOT_WRITE;
      howmany++;
      if( fprintf(outfile,"%s %s",tt_tokenIvl, buf) <= 0 )
         return TT_CANNOT_WRITE;
   }/*if(retcode)*/

   if ( ( retcode=l_getCoords( buf, maxLen, table, top,  INT_LINE+isOrig) ) <0 )
      return retcode;
   if(retcode){/*Non-empty string is generated */
      /*Finish previous line, if present:*/
      if( (howmany) && (fprintf(outfile,":%s",sep)<=0) )return TT_CANNOT_WRITE;
      howmany++;
      if( fprintf(outfile,"%s %s",tt_tokenIl, buf) <= 0 )
         return TT_CANNOT_WRITE;
   }/*if(retcode)*/

   if ( ( retcode=l_getCoords( buf, maxLen, table, top,  INT_LLBL+isOrig) ) <0 )
      return retcode;
   if(retcode){/*Non-empty string is generated */
      /*Finish previous line, if present:*/
      if( (howmany) && (fprintf(outfile,":%s",sep)<=0) )return TT_CANNOT_WRITE;
      howmany++;
      if( fprintf(outfile,"%s %s",tt_tokenIll, buf) <= 0 )
         return TT_CANNOT_WRITE;
   }/*if(retcode)*/

   return howmany;
}/*l_saveCoordsToFile*/
/*Wrapper:*/
int  tt_saveCoordsToFile(
      FILE *outfile,
      char *buf,
      int maxLen,
      int table,
      long top,
      int isOrig,
      char *sep)
{
   if(tt_chktop(&table,&top) ) return TT_NOTFOUND;
   return l_saveCoordsToFile(outfile,buf,maxLen,table,top,isOrig,sep);
}/*tt_saveCoordsToFile*/

/*outfile must be opened, and will be opened after this function.
  isOrig!=0 -- usr, non-translated; isOrig==0 -- nusr, translated.*/
int tt_saveTopology( FILE *outfile, int table, tt_singletopol_type *top, int isOrig)
{
char *buf=NULL;
int retcode=0;
int i,maxLen=(top->nel+top->nil+1)*19;/*(ext+int+1)*length( "-xxxxx.xx,-xxxxx.xx" )*/
char *id=NULL;
   if(    ( buf=calloc( maxLen, sizeof(TT_INT) )    )==NULL      )return TT_NOTMEM;

   if(isOrig){
      tt_top2str(buf,top->nel,top->extPart,top->nil,top->usrTopol);
   }else{
      l_top2str(top->l_nusr2usr,
                top->v_usr2nusr,
                top->l_dirUsr2Nusr,
                buf,
                top->nel,
                top->extPart,
                top->nil,
                top->usrTopol);
   }
   if(
      /*Can't finish topology here, unknown momenta presence:*/
      fprintf(outfile,"    %s",buf)<=0
      )goto fail_return;

   if( top->nel != 0 )
      id="f";
   else
      id="i";

   /*l_getMomenta ALWAYS return allocated string, may be, "".
   "table" and "top" are indices, NOT user numbers! This procedure does NOT
   perform any checkup about table and top! */
   if(top->nmomenta>0){
      int *err=&retcode;
      char *momenta;
      if(/*Must finish topology:*/
         fprintf(outfile,":\n")<=0
            )goto fail_return;
      for(i=1; i<= top->nmomenta; i++){
         momenta=l_getMomenta(isOrig,table, top->n,i,err);
         if(retcode >= 0){
            if(
             fprintf(outfile,"    %s%c\n",momenta,(i==top->nmomenta)?';':':')<=0
              ){
                 free(momenta);/*Was allocated by l_getMomenta*/
                 goto fail_return;
            }
         }/*if(retcode >= 0)*/
         free(momenta);/*Was allocated by l_getMomenta*/
         if(retcode<0)goto fail_return;
      }/*for(i=1; i<= top->nmomenta; i++)*/
   }else /*Must finish topology:*/
      if(
         fprintf(outfile,";\n")<=0
            )goto fail_return;

   {/*Block begin*/
      char buf[NAMEBUFMAX];

      i=  (top->int_vert!=NULL)||
          (top->int_vlbl!=NULL)||
          (top->int_line!=NULL)||
          (top->int_llbl!=NULL)||
          (top->ext_vert!=NULL)||
          (top->ext_vlbl!=NULL)||
          (top->ext_line!=NULL)||
          (top->ext_llbl!=NULL);

      /*Now i !=0 if al least one type of coordinates is present*/
      if(
           (i)&&
           (
            fprintf(outfile,"%s %s =",tt_tokenCoordinates,l_get_ttName(id,buf,NAMEBUFMAX,top) )
            <=0
           )
      )goto fail_return;
   }/**Block end*/
   if(i){
      if( (retcode=l_saveCoordsToFile(outfile,buf,maxLen,table,top->n,isOrig,"\n   "))
              <0 ){
                 retcode=TT_CANNOT_WRITE;
                 goto fail_return;
      }/*if( (retcode=l_saveCoordsToFile(outfile,buf,maxLen,table,top->n,isOrig,"\n   "))*/

      /*Finish previous line, it is always present (at least, "coordinates top ="):*/
      if( fputs(";\n", outfile)<0)goto fail_return;
   }/*if(i)*/
   if(top->top_remarks != 0){
       {/*Block begin*/
          char buf[NAMEBUFMAX];
          if(
             fprintf(outfile,"%s %s =\n   ",tt_tokenRemarks,
                                             l_get_ttName(id,buf,NAMEBUFMAX,top) ) <=0
          )goto fail_return;
       }/*Block end*/
       if( (retcode=saveTopolRem(outfile,top->top_remarks,top->remarks,"\n   "))
              <0 ){
                 retcode=TT_CANNOT_WRITE;
                 goto fail_return;
       }/*if( (retcode=saveTopolRem(outfile,top->top_remarks,top->remarks,"\n   ")) )*/
       if( fputs(";\n", outfile)<0)goto fail_return;
   }/*if(top->top_remarks != 0)*/
   retcode = 0;
   fail_return:/*Just keep retcode untouched*/
   free(buf);
   return retcode;
}/*tt_saveTopology*/
