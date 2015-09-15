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
/*TOPOLOGY.C*/

/* The basic idea of topology represetnation:

  An arbitrary topology may be represented in
   standart form like the follow:
  (-3,3)(-2,2)(-1,1)(1,2)(1,4)(1,5)(2,3)(2,5)(3,4)(3,5)(4,5)

  The corresponding diagram is:
             -1
              |
              |
              | 1
            / | \
       2  /   |   \  4
  -2___ /_____|_____\
        \     |5    /
          \   |   /
            \ | /
              | 3
              |
              |
             -3
All (psevdo) vertices with negatve indices are external.
The only abitrariness remains is arranging internal vertices, in this
case they are 4 and 5.
  The minimal (lexicographycs) form of such reprezentation is assumed
  as a canonical form of current topology.  In this case it is

     (-3,3)(-2,2)(-1,1)(1,2)(1,4)(1,5)(2,3)(2,4)(3,4)(3,5)(4,5)

according to the follow indices distribution:
             -1
              |
              |
              | 1
            / | \
       2  /   |   \  5
  -2____/_____|_____\
        \     |4    /
          \   |   /
            \ | /
              | 3
              |
              |
             -3
Line (-3,3) is named as line number -3,
(1,2) is the line number 1, line (1,5) has number 3.

*/
#include <stdlib.h>
#include <stdio.h>
#include "tools.h"
#include "comdef.h"
#include "types.h"
#include "variabls.h"
#include "headers.h"
#include "texts.h"
static tTOPOL tmp_topology;

static int top_LEG(pTOPOL t1,pTOPOL t2);/* returns -1 if t1<t2, 1, if t1>t2,
                                          and 0, if t1 == t2 */

pTOPOL get_orig_topology(pTOPOL topology)
{
  return(top_let(&tmp_topology,topology));
}/*get_orig_topology*/

/*Defines line substitution according to r_topology and v_subst:*/
static void get_l_subst(pTOPOL i_topology, pTOPOL r_topology,
                 char *l_subst,char *v_subst, char *l_directions);

static void mutation(pTOPOL topology, char one_vert, char two_vert);

pTOPOL sortinternalpartoftopology (pTOPOL topology)
{int i;
 char tmp;
   for(i=1;!(i>topology->i_n);i++){
      if( (topology->i_line)[i].to < (topology->i_line)[i].from ){
         tmp=(topology->i_line)[i].from;
         (topology->i_line)[i].from=(topology->i_line)[i].to;
         (topology->i_line)[i].to = tmp;
      }
   }
   qsort((void *)((topology->i_line)+1),topology->i_n,
                                      sizeof(tLINE),top_cmp);
   return(topology);
}/*sortinternalpartoftopology*/

int top_cmp(const void *a,const void *b)
{
 if (((pLINE)a)->from == ((pLINE)b)->from)
    return(((pLINE)a)->to - ((pLINE)b)->to);
 else
    return(((pLINE)a)->from - ((pLINE)b)->from);
}/*top_cmp*/

static int top_LEG(pTOPOL t1,pTOPOL t2)
{
  char i;
   for(i=1;!(i>t1->i_n);i++){
     if ((t1->i_line)[i].from<(t2->i_line)[i].from)return(-1);
     if ((t1->i_line)[i].from>(t2->i_line)[i].from)return(1);
     if ((t1->i_line)[i].to<(t2->i_line)[i].to)return(-1);
     if ((t1->i_line)[i].to>(t2->i_line)[i].to)return(1);
   }
   return(0);
}/*top_LEG*/

pTOPOL top_let(pTOPOL t1,pTOPOL t2)
{
  char i;
   t2->i_n=t1->i_n;
   t2->e_n=t1->e_n;
   for(i=1;!(i>t1->i_n);i++){
     (t2->i_line)[i].from=(t1->i_line)[i].from;
     (t2->i_line)[i].to=(t1->i_line)[i].to;
   }
   for(i=1;!(i>t1->e_n);i++){
     (t2->e_line)[i].from=(t1->e_line)[i].from;
     (t2->e_line)[i].to=(t1->e_line)[i].to;
   }
   return(t2);
}/*top_let*/

static void get_l_subst(pTOPOL i_topology, pTOPOL r_topology,
                  char *l_subst,char *v_subst,char *l_directions)
{
  char i,j,k,tmp[MAX_I_LINE+2];
  int direct;
  set_of_char markline;
     set_sub(markline,markline,markline);/* clear markline */
     s_let(l_subst+1, tmp+1);
     { char *tmp;
       for(tmp=l_subst+1;*tmp;tmp++)*tmp=-1;
     }

     for (i=1;!(i>i_topology->i_n);i++)
       for(j=1;!(j>r_topology->i_n);j++)
          if(
            (
              ((direct=(
               (
                 r_topology->i_line[j].from==
                    v_subst[i_topology->i_line[i].from]
               )&&(
                 r_topology->i_line[j].to==
                    v_subst[i_topology->i_line[i].to]
               )
              ))==1)
               ||
              (
               (
                 r_topology->i_line[j].to==
                    v_subst[i_topology->i_line[i].from]
               )&&(
                 r_topology->i_line[j].from==
                    v_subst[i_topology->i_line[i].to]
               )
              )
          )&&
            (!set_in(j,markline))
          )
          {
            set_set(j,markline);
            for(k=1;!(k>i_topology->i_n);k++){
               if(tmp[k]==i){
                  l_subst[k]=j;
                  if(!direct)l_directions[k]*=-1;
                  break;
               }
            }
             break;
          }

}/*get_l_subst*/

char *top2str(pTOPOL topology, char *str)
{
  char tmp[MAX_STR_LEN];
  char i;
     *str=0;
     for (i=topology->e_n; i>0;i--){
        sprintf(tmp,"(%hd,%hd)",topology->e_line[i].from,
                                  topology->e_line[i].to);
        s_cat(str,str,tmp);
     }
     for (i=1;!(i>topology->i_n);i++){
        sprintf(tmp,"(%hd,%hd)",topology->i_line[i].from,
                                  topology->i_line[i].to);
        s_cat(str,str,tmp);
     }
     return(str);
}/*top2str*/

static void help(char *a, char *b)
{
   message(THELP1,NULL);
   message(THELP2,NULL);
   message("(-4,1)(-3,2)(-2,2)(-1,1)(1,3)(1,4)(2,3)(2,4)(3,4)",NULL);
   message(THELP3,NULL);
   message(THELP4,NULL);
   halt(a,b);
}/*help*/

pTOPOL read_topol(pTOPOL topology,
                  char *l_subst,char *v_subst,char *l_directions)
{
 char tmp[MAX_STR_LEN];

  max_vertex=1;
  if( (topology->e_n==0)&&(topology->i_n==0)){
    /* Read line using standard scanner*/
    char l_count=-1-ext_lines;
    pLINE c_line;
    int int_char;
       topology->e_n=topology->i_n=0;
       /* Read tokens until ':'*/
       while(1){
            if(*sc_get_token(tmp)==':')break;
            l_count++;
            if(*tmp != '(') help(OBRACKEDEXPECTED,tmp);
            sc_get_token(tmp);
            if (l_count<0){
               if(*tmp != '-')
                   help(MINUSEXPECTED,tmp);
               s_cat(tmp,"-",sc_get_token(tmp));
               c_line=(topology->e_line)-l_count;
               topology->e_n++;
            }else{
               if (!l_count) l_count=1;
               if(++(topology->i_n)>MAX_I_LINE)
                  help(TOOLONGTOPOLOGY,NULL);
               c_line=(topology->i_line)+l_count;
            }
            if(!(sscanf(tmp,"%d",&int_char)>0))
                    help(UNEXPECTED,tmp);
            if (max_vertex<(c_line->from=int_char))
               max_vertex=c_line->from;
            if((l_count<0)&&(c_line->from!=l_count))
                 help(INVVERTEX,tmp);
            if(*sc_get_token(tmp)!=comma_char)
               help(COMMAEXPECTED,tmp);

            if(!(sscanf(sc_get_token(tmp),"%d",&int_char)>0))
                    help(UNEXPECTED,tmp);
            if (max_vertex<(c_line->to=int_char))
               max_vertex=c_line->to;
            if(*sc_get_token(tmp)!=')')
               help(CBRACKEDEXPECTED,tmp);
       }/*while*/
  }else{/* We already HAVE the topology! */
        /* Define max_vertex:*/
     int i;
     max_vertex=1;
     for(i=1;!(i>topology->i_n);i++){
        if( max_vertex< topology->i_line[i].from)
           max_vertex=topology->i_line[i].from;
        if( max_vertex< topology->i_line[i].to)
           max_vertex=topology->i_line[i].to;
     }
  }/*if( (topology->e_n==0)&&(topology->i_n))... else...*/

  {/*block 2 begin*/
      /* Some initializations:*/
    char i;
      *v_subst=-1;
      *l_subst=-1;
      *l_directions=-1;
      for (i=1;!(i>max_vertex); i++)
         *(v_subst+i)=i;
      *(v_subst+i)=0;
      for (i=1;!(i>topology->i_n); i++){
         *(l_subst+i)=i;
         *(l_directions+i)=1;
      }
      *(l_subst+i)=0;
      *(l_directions+i)=0;
  }/*block 2 end*/

  /*The static variable tmp_topology is used get_orig_topology!:*/
  top_let(topology, &tmp_topology);

  if(topology->i_n > 0 ){/*block 3 begin*/
    char i,j,tmp,*l_s=l_subst+1;
      for(i=topology->i_n;i>0;i--)
         if(topology->i_line[i].from>topology->i_line[i].to){
            tmp=topology->i_line[i].from;
            topology->i_line[i].from=topology->i_line[i].to;
            topology->i_line[i].to=tmp;
            l_directions[i]=-1;
         }

      qsort((void *)((topology->i_line)+1),topology->i_n,
                                      sizeof(tLINE),top_cmp);

      {/*block 3a begin*/
         set_of_char markline;
         set_sub(markline,markline,markline);/* clear markline */
           for(i=1;!(i>tmp_topology.i_n);i++)
             for(j=1;!(j>topology->i_n);j++){
               if(
                  (
                    (topology->i_line[j].from==tmp_topology.i_line[i].from)&&
                    (topology->i_line[j].to==tmp_topology.i_line[i].to)
                  )||
                  (
                    (topology->i_line[j].from==tmp_topology.i_line[i].to)&&
                    (topology->i_line[j].to==tmp_topology.i_line[i].from)
                  )
                 ){
                   if(!set_in(j,markline)){
                      *(l_s)++ = j;
                      set_set(j,markline);
                      break;
                   }
               }
             }
      }/*block 3a end*/
  }/*block 3 end*/

  {/*block 4 begin*/
    /* Now we have ready topology. Check it:*/
    char i, max_from=1, max_to,c_group;

      for(i=max_vertex; i>0; i--)tmp[i]=0;
      i=1;
      /*verify internal lines:*/
      if (topology->i_n > 0 )
      do{
         c_group=(topology->i_line)[i].from;
         if((topology->i_line)[i].from<max_from)
            help(INVTOPOLOGY,NULL);
         if((topology->i_line)[i].from>(topology->i_line)[i].to)
            help(INVTOPOLOGY,NULL);

         max_from=(topology->i_line)[i].from;
         max_to=(topology->i_line)[i].from;
         do{
            if((topology->i_line)[i].to<max_to)
               help(INVTOPOLOGY,NULL);

            max_to=(topology->i_line)[i].to;
            tmp[(topology->i_line)[i].from]++;
            tmp[(topology->i_line)[i].to]++;
            if(++i>topology->i_n)break;
         }while((topology->i_line)[i].from==c_group);
      }while(!(i>topology->i_n));

      /* Verify external lines:*/
      tmp[max_to=(topology->e_line)[1].to]++;
      if(max_to!=(-(topology->e_line)[1].from))
         help(INVEXTLINE,NULL);

      for(i=2;!(i>topology->e_n);i++){
        if((topology->e_line)[i].to>max_to){
           if((topology->e_line)[i].to!=++max_to)
              help(INVEXTLINE,NULL);
        }
        tmp[(topology->e_line)[i].to]++;
      }

      if(restrict_tails){/*Only 3- and 4- tails may exist:*/
         for(i=max_vertex;i>0;i--)	   if ((tmp[i]<3)||(tmp[i]>4))
               help(INVTOPOLOGY,NULL);
      }
  }/*block 4  end*/
  return(topology);
}/*read_topol*/

void mutation(pTOPOL topology, char one_vert, char two_vert)
{
  int i;
  char tmp;
    for(i=1;!(i>topology->i_n);i++){
       if (topology->i_line[i].from == one_vert)
          topology->i_line[i].from = two_vert;
       else if (topology->i_line[i].from == two_vert)
          topology->i_line[i].from = one_vert;
       if (topology->i_line[i].to == one_vert)
          topology->i_line[i].to = two_vert;
       else if (topology->i_line[i].to == two_vert)
          topology->i_line[i].to = one_vert;
       if(topology->i_line[i].from>topology->i_line[i].to){
          tmp=topology->i_line[i].from;
          topology->i_line[i].from=topology->i_line[i].to;
          topology->i_line[i].to=tmp;
       }
    }
    qsort((void *)((topology->i_line)+1),topology->i_n,
                                      sizeof(tLINE),top_cmp);
}/*mutation*/

/*Note -- in contrast to reduce_topology, this procedure makes
preliminary internal sort itself:*/
int reduce_topology_full(pTOPOL i_topology, pTOPOL r_topology,
                    char *l_subst, char *v_subst,char *l_directions)
{
int ret;
pTOPOL m_top=malloc(sizeof(tTOPOL));

   if(m_top==NULL)
      halt(NOTMEMORY,NULL);
   top_let(i_topology,m_top);
   sortinternalpartoftopology(m_top);
   ret=reduce_topology(0,m_top, r_topology,l_subst, v_subst,NULL);
/*   sortinternalpartoftopology(r_topology);*/
   get_l_subst(i_topology, r_topology,l_subst,v_subst,l_directions);
   free(m_top);
   return(ret);
}/*reduce_topology_full*/

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*
 * int per_init(char *str) - constructor, str is  a pointer to initial string;
 * char *get_per_str(void) - returns pointer to next permutation.
*/

#define MAX_V 20

static int p_i[MAX_V];
static char per_str[MAX_V+2];
#ifdef DEBUG
static int is_init=0;
#endif

static int exch(void) /* returns -1, if all chains are generated.*/
{
 int i,j=0,l=*per_str;
 char ch;
    if (l==1)
       return(1);
       /*return(!(p_i[l]=!p_i[l]));*/

    for(i=1;per_str[i];i++)if(per_str[i]!=-1)
      if(++j==p_i[l])
             break;
    ch=per_str[i];per_str[i]=-1;(*per_str)--;
    j=exch();
    (*per_str)++;
    per_str[i]=ch;
    if(j){
         if (++(p_i[l])>l)
            return(p_i[l]=1);
         for(j=i+1;per_str[j];j++) if (per_str[j]!=-1)
           break;
         ch=per_str[i];per_str[i]=per_str[j];per_str[j]=ch;
    }
   return(0);
}/*exch*/

char *get_per_str(void)
{
#ifdef DEBUG
   if (!is_init)return(NULL);
#endif
   if (exch()){
#ifdef DEBUG
     is_init=0;
#endif
     return(NULL);
   }
   return(&(per_str[1]));
}/*get_per_str*/

int per_init(char *str)
{
  char l;
#ifdef DEBUG
   if (str==NULL) return(-1);
#endif
   l= s_len(str);
#ifdef DEBUG
   if (l<1) return(-1);
   if (l>MAX_V) return(-1);
#endif
   p_i[per_str[0]=l]=1;
   s_let(str, &(per_str[1]));
   while(l--)
       p_i[l]=1;
#ifdef DEBUG
   is_init=1;
#endif
   return(0);
}/*per_init*/

static int top_lessthen(pTOPOL t1,pTOPOL t2)
{
  char i;
   for(i=1;!(i>t1->i_n);i++){
     if ((t1->i_line)[i].from<(t2->i_line)[i].from)return(1);
     if ((t1->i_line)[i].from>(t2->i_line)[i].from)return(-1);
     if ((t1->i_line)[i].to<(t2->i_line)[i].to)return(1);
     if ((t1->i_line)[i].to>(t2->i_line)[i].to)return(-1);
   }
   for(i=1;!(i>t1->e_n);i++){
     if ((t1->e_line)[i].to<(t2->e_line)[i].to)return(1);
     if ((t1->e_line)[i].to>(t2->e_line)[i].to)return(-1);
   }
   return(0);
}/*top_lessthen*/

/* change_topology(from,to, vsubst) -- substitutes vertices from and stores result in to.
   Sorts internal part.
   currentsubstitution: to[i]=currentsubstitution [from[i]-1]
     a b c d e f g -- currentsubstitution
     0 1 2 3 4 5 6 -- index-1

   ATTENTION!! "to" vertices at external legs are substituted, too!
 */
static  pTOPOL  change_topology(pTOPOL i_topology,
                   pTOPOL r_topology, char *currentsubstitution)
{
  char i,tmp;
   for(i=1;!(i>i_topology->i_n);i++){
      (r_topology->i_line)[i].from=
          currentsubstitution[(i_topology->i_line)[i].from-1];
      (r_topology->i_line)[i].to=
          currentsubstitution[(i_topology->i_line)[i].to-1];
      if( (r_topology->i_line)[i].to < (r_topology->i_line)[i].from ){
         tmp=(r_topology->i_line)[i].from;
         (r_topology->i_line)[i].from=(r_topology->i_line)[i].to;
         (r_topology->i_line)[i].to = tmp;
      }
   }
   for(i=1;!(i>i_topology->e_n);i++){
      (r_topology->e_line)[i].to=
          currentsubstitution[(i_topology->e_line)[i].to-1];
   }
   qsort((void *)((r_topology->i_line)+1),r_topology->i_n,
                                      sizeof(tLINE),top_cmp);
   return(r_topology);
}/*change_topology*/

#define stINIT 0
#define stNEWLINE 1
#define stEND 2

int string2topology(char *str,pTOPOL topology)
{
int state,statusofexternalline,/*0--not defined yet, -1 --ext, +1-- int*/
    maxvertex=0,lcount=1;
 for(state=stINIT; state!=stEND;)switch(state){
    case stINIT:
       topology->e_n=topology->i_n=
       (topology->e_line)[0].from=(topology->e_line)[0].to=
       (topology->i_line)[0].from=(topology->i_line)[0].to=0;
       statusofexternalline=0;
    case stNEWLINE:
       if(*str++!='(')return(-1);
       if(sscanf(str,"%d",&state)!=1)return(-1);
       if(state<0){
          if(statusofexternalline==0){char *tmp=str;
             statusofexternalline=-1;
             topology->e_n=0;
             while(*tmp!='\0')/*Count number of '-' occuring in the str*/
                if(*tmp++ == '-')(topology->e_n)++;
             lcount=topology->e_n;
          }else if(statusofexternalline>0)
              return(-1);
          if(lcount < 0) return(-1);
          (topology->e_line)[lcount].from=state;
          str++;
       }else{
          if(statusofexternalline<0){
             if (lcount!=0)return(-1);
             lcount=1;
             statusofexternalline=1;
          }else if(statusofexternalline==0)
             statusofexternalline=1;
          (topology->i_n)++;
          if(((topology->i_line)[lcount].from=state)>maxvertex)
              maxvertex=state;
       }
       while(set_in(*str,digits))str++;
       if(*(str)!=',')return(-1);
       str++;
       if(sscanf(str,"%d",&state)!=1)return(-1);
       if(!(state>0))return(-1);
       if(statusofexternalline<0)
          (topology->e_line)[lcount--].to=state;
       else
          (topology->i_line)[lcount++].to=state;
       if(maxvertex<state)
          maxvertex=state;
       while(set_in(*str,digits))str++;
       if(*(str)!=')')return(-1);
       if(*(++str)!='\0')state=stNEWLINE;else state=stEND;
       break;
 }
 return(maxvertex);
}/*string2topology*/

/*???
extern int factorialreordering;
*/

/*Note -- in contrast to reduce_topology, this procedure makes
preliminary internal sort itself:*/
int reduce_internal_topology(pTOPOL i_topology, pTOPOL r_topology,
                    char *l_subst, char *v_subst,char *l_directions)
{
 char i,was_exchange,n_v;
 char *currentsubstitution,*memsubstitution;

 /*Determine max vert number and temporary store it in n_v:*/
 n_v=1;
 for(was_exchange=1;!(was_exchange>i_topology->i_n);was_exchange++){
    if( n_v< i_topology->i_line[was_exchange].from)
       n_v=i_topology->i_line[was_exchange].from;
    if( n_v< i_topology->i_line[was_exchange].to)
       n_v=i_topology->i_line[was_exchange].to;
 }
 was_exchange='\0';

 top_let(i_topology, r_topology);/*Copy original topology to the new one*/
 sortinternalpartoftopology(r_topology);/*What about identical substitution?*/

 if(n_v==1)
    return(0);/*How to reduce the dot?*/

 memsubstitution=new_str(v_subst);

 for(i='\1';v_subst[i]!='\0';i++)memsubstitution[i]=i;

 /*ok, now in i we have the number of vertices. Choose the algorithm:
   if i<4 then the factorial exhaustive search is faster:
  */
 /*if(factorialreordering)*/
 if(n_v<4){/*factorial exhaustive search start*/

    tTOPOL buf_topology;
    pTOPOL tmp_topol[2];

    int wrk_topol=0;

   tmp_topol[0]=&buf_topology;
   tmp_topol[1]=r_topology;
   top_let(r_topology, &buf_topology);/*Copy original topology to the new*/

   /*get_per_str() gets new substitution each time it is called.
     After all permutations are exhausted it returns NULL. The starting permutation
     mut be set by per_init:*/
   per_init(memsubstitution+1);
   {int tag;
   while((currentsubstitution=get_per_str())!=NULL)
       if( (tag=top_lessthen(
               change_topology(i_topology,
                   tmp_topol[wrk_topol],currentsubstitution),
               tmp_topol[!wrk_topol]
             )
            )!=-1
       ){/*New topology less or equal stored one*/
         /* The next line: equal (tag==0) topologies are sorted be substitutions:*/
         if( (tag==1)||(s_cmp(currentsubstitution,memsubstitution+1)<0) ){
            wrk_topol=!wrk_topol;/* Now !wrk_topology corresponds
                                  to less topology*/
            s_let(currentsubstitution,memsubstitution+1);
            was_exchange='\1';
          }
       }
   }
   *memsubstitution=-1;

   if(was_exchange){
      if(wrk_topol)/* !wrk_topol == 0 => less topology is in buf_topology */
         top_let(&buf_topology, r_topology);
   }
   /*factorial exhaustive search end*/
 }else{/*red_topology algorithm start*/
   struct mstruct{int n; char *m;} *mlen;
   char nv_1=n_v+1;
   char n_l=i_topology->i_n,d;
   char nl_1=n_l+1;
   char nv2=n_v*2;
   char nv2_1=nv2+1;
   char *buf, *symmetry,sym_n=1;
   char j;
   pTOPOL min_topology, tmp_topology, sorted_topology;
      /*Initialization:*/
      if(  ( symmetry=malloc(nv_1) )==NULL  )
         halt(NOTMEMORY,NULL);

      if(  ( buf=malloc(nl_1) )==NULL  )
         halt(NOTMEMORY,NULL);

      if(   (mlen=malloc(sizeof(struct mstruct)*(nv_1) ))==NULL  )
         halt(NOTMEMORY,NULL);

      /*mlen[i].n is an index number "N" of incoming vertex,
        mlen[i].m is an array "A" for bitwise algorithm.

        The idea: we must determine the proper N to start the algorithm
        reduce_topology. It must be of
              maximal tadpole, or
              maximal multilines.
        Let's call "length of line"  the distance between two ends of the line,
        i.e., the length d=(fromvertex-tovertex). This length is associated to
        the vertices fromvertex -> d and tovertex -> -d
        We will collect tadpoles at A[0],
        lines of length +d in A[i], and absolute values of lengths of -d at A[d+n_l].
        Obviously, multiply lines will have the same lengths.
        So, afterwards we will sort A[1, ..., 2*n_l] arrays
        from large to small numbers, and try to start algorithm with
        all of largest arrays.
        */
      /*Initialization of mlen:*/
      for(i=1;i<nv_1;i++){
         mlen[i].n=i;
         if(  ( mlen[i].m=malloc(nv2_1) )==NULL  )
            halt(NOTMEMORY,NULL);
         mlen[i].m[0]='\1';
         for(j=1;j<nv2_1;j++)mlen[i].m[j]='\0';
      }

      /*Collecting tadpoles and lines:*/
      for(i=1;i<nl_1;i++){/*in r-topology we have sorted variant of i_topology*/
         /*evaluate the line length d:*/
         if(  (d=(r_topology->i_line)[i].to-(r_topology->i_line)[i].from)!=0 ){
            /*lines:*/
            mlen[(r_topology->i_line)[i].from].m[d]++;
            mlen[(r_topology->i_line)[i].to].m[d+n_v]++;
         }else/**/
            mlen[(r_topology->i_line)[i].from].m[0]++;
      }/*for(i=1;i<nl_1;i++)*/
      /*the array symmetry[ 1 ,..., sym_n-1 ] will contain indices of highest equal mlen:*/
      symmetry[1]=1;sym_n=1;/*Initial 1 is just to sort the first enter.*/
      for(i=1;i<nv_1;i++){/*loop on all mlen*/
         char *tmp=mlen[i].m+1;
         for(j=0;j<nl_1;j++)buf[j]='\0';/*buf is a working array for bitwise sorting*/
         L2Ssort(tmp,buf,nv2,nl_1);/*bitwise sorting macro, see tools.h*/

         /*Note, the length of mlen.m is at least twice more than n_v, so after
         sorting there will be enough trailing zeroes to safety consider each mlen[i].m
         as a zero-terminating string:*/
         if( (d=s_cmp(mlen[i].m,mlen[symmetry[1]].m))>0){
            /*Found new maximal, re-initializing symmetry:*/
            symmetry[1]=i;
            sym_n=2;
         }else if ( d == '\0' )/*Add a new candidate:*/
            symmetry[sym_n++]=i;/*Note, at first step this will just re-set symmetry[1]=1*/
      }
      /*Now in symmetry[ 1 ,..., sym_n-1 ] we have indices of all candidates to be
      a vertex number 1.*/
      currentsubstitution=new_str(v_subst);
      if(   (min_topology=malloc(sizeof(tTOPOL)))==NULL  )
         halt(NOTMEMORY,NULL);
      if(   (tmp_topology=malloc(sizeof(tTOPOL)))==NULL  )
         halt(NOTMEMORY,NULL);

      if(   (sorted_topology=malloc(sizeof(tTOPOL)))==NULL  )
         halt(NOTMEMORY,NULL);

      top_let(r_topology,sorted_topology);
      top_let(sorted_topology,min_topology)->e_n='\0';
      /*e_n=0 to prevent changing external vertices by reduce_topology*/
      for(i=1;i<sym_n;i++){/*try all candidates*/
         /*initialisation, we will use r_topology as incoming one:*/
         top_let(sorted_topology,r_topology)->e_n='\0';
         /*e_n=0 to prevent changing external vertices by reduce_topology*/

         /*According this candidate, we must exchange elements 1 and mlen[symmetry[i]].n:*/
         mutation(r_topology,mlen[symmetry[i]].n,1);

         /*Initialization of the vertex substitution:*/
         for(j='\1';v_subst[j]!='\0';j++)currentsubstitution[j]=j;

         /*Try to reduce the topology starting from a vertex number 2:*/
         reduce_topology(2,r_topology, tmp_topology,NULL,currentsubstitution,NULL);
         /*Compare the result with a previous minimal topology:*/
         if( top_LEG(tmp_topology,min_topology)==-1 ){
            /*New topology is less then the stored one.
              Store it with the corresponding substitution.*/

            /* Store topology:*/
            top_let(tmp_topology,min_topology);
            was_exchange='\1';/*set the flag*/

            /*Store the substitution:*/
            s_let(currentsubstitution,memsubstitution);
            /*And now we have to exchange 1 and mlen[symmetry[i]].n element:*/
            memsubstitution[mlen[symmetry[i]].n]=memsubstitution[1];
            /*note, currentsubstitution == memsubstitution:*/
            memsubstitution[1]=currentsubstitution[mlen[symmetry[i]].n];

            for(j='\1';v_subst[j]!='\0';j++)currentsubstitution[j]=j;
         }/*if( top_LEG(tmp_topology,min_topology)==-1 )*/
      }/*for(i=1;i<sym_n;i++)*/
      top_let(i_topology,r_topology);/*restore external part*/
      /*restore internal part:*/
      top_let(min_topology,r_topology)->e_n=i_topology->e_n;
      /*Last assignment is necessary since e_n was broken in min_topology*/

      /*Here we have to change "to" vertices of the external part according to
        memsubstitution:*/
      for(i=1;!(i>r_topology->e_n);i++){
         (r_topology->e_line)[i].to=
             memsubstitution[(i_topology->e_line)[i].to];
      }

      /*Free allocated memory:*/
      free(sorted_topology);
      free(tmp_topology);
      free(min_topology);
      free(currentsubstitution);
      for(i=1;i<nv_1;i++)
         free(mlen[i].m);
      free(mlen);
      free(buf);
      free(symmetry);
      /*red_topology algorithm end*/
 }/*if(n_v<4)...else*/

 *memsubstitution=-1;/*On the off-chance*/

 if(l_directions!=NULL)/*We must retrieve line substitutions and directions*/
    get_l_subst(i_topology, r_topology,l_subst,memsubstitution,l_directions);
 if(was_exchange)
    substitutel(v_subst,memsubstitution+1);

 free(memsubstitution);
 return(was_exchange);
}/*reduce_internal_topology*/

static pTOPOL setdir(pTOPOL t, char *ldir)
{
  char i,tmp;
   for(i=1;!(i>t->i_n);i++)if (ldir[i]<0){
     tmp=(t->i_line)[i].to;
     (t->i_line)[i].to=(t->i_line)[i].from;
     (t->i_line)[i].from=tmp;
   }
   return(t);
}/*setdir*/
static pTOPOL setlines(pTOPOL t, char *lsubst)
{
  char i;
  tTOPOL tmp;
   top_let(t,&tmp);
   for(i=1;!(i>t->i_n);i++){
      (t->i_line)[lsubst[i]].to=(tmp.i_line)[i].to;
      (t->i_line)[lsubst[i]].from=(tmp.i_line)[i].from;
   }
   return(t);
}/*setlines*/
static pTOPOL setvertices(pTOPOL t, char *vsubst)
{
  char i;
  tTOPOL tmp;
   top_let(t,&tmp);
   for(i=1;!(i>t->i_n);i++){
      (t->i_line)[i].from=vsubst[(tmp.i_line)[i].from];
      (t->i_line)[i].to=vsubst[(tmp.i_line)[i].to];
   }
   for(i=1;!(i>t->e_n);i++)
      (t->e_line)[i].to=vsubst[(tmp.e_line)[i].to];
   return(t);
}/*setvertices*/

pTOPOL reset_topology(pTOPOL i_topology, pTOPOL r_topology,
                    char *l_s, char *v_s,char *l_d)
{
   return(
     setvertices(
       setlines(
          setdir(
             top_let(i_topology,r_topology),l_d
          ),l_s
       ),v_s
     )
   );
}/*reset_topology*/

pTOPOL newTopol( pTOPOL topol )
{
pTOPOL tmpTopol=(pTOPOL)calloc(1,sizeof(tTOPOL));
   if(tmpTopol == NULL ) halt(NOTMEMORY,NULL);
   return top_let(topol,tmpTopol);
}/*newTopol*/

typedef struct {
   long n;         /* number of symmetries*/
   char **vsubst; /* corresponding vertices substitutions*/
      /*vsubst[i][j]: i = 0...n-1; j=1...maxVert; vsubst[i][0]==-1*/
   pTOPOL topol;  /* topology*/
} tr_infType;

tr_infType *tr_reduceTopology(
   pTOPOL topol, /*incoming topology*/
   char chble,   /*first changeable vertex*/
   char maxVert, /*number of vertices*/
   long maxSyms  /*maximal numbers of symmetries*/
                               )
{
char NmaxCurSyms,/*Number of changaeble vertices*/
     NCurSyms;/* number of pretenders*/
char *curSyms, /*Array with  vertices-pretenders*/
     tmpCh,
     *markSyms;/*working array*/

int d;

long CSyms=0,/*Current number of symmetries, workin variable.*/
     NSyms=0,  /*symmetries counter*/
     i,j,k;   /*counters*/
pTOPOL minTopol=topol;

tr_infType **tmpInfo,/*returned values for recursive calls*/
           *ret;/*terurned value*/

   if (maxVert<chble)return NULL;/*Nothig to do, seems to be internal error...*/

   if(  (ret=(tr_infType *)calloc(1,sizeof(tr_infType)))==NULL  )
      halt(NOTMEMORY,NULL);

   if(  (
          NmaxCurSyms=maxVert-chble+1/*Number of changaeble vertices*/
        )==1  ){/*End of recursion! ALL vsubst[i] will be allocated at this place!*/
      char *tmp;
      if(  (ret->vsubst=(char**)calloc(1,sizeof(char*)))==NULL  )halt(NOTMEMORY,NULL);
      if(  (tmp=(char*)calloc(maxVert+2,sizeof(char)))==NULL  )halt(NOTMEMORY,NULL);
      *tmp=-1;
      for(tmpCh='\1';!(tmpCh>maxVert);tmpCh++)tmp[tmpCh]=tmpCh;
      tmp[tmpCh]='\0';
      ret->vsubst[0]=tmp;
      ret->n=1;
      ret->topol=newTopol(topol);
      return ret;
   }/*if*/

   /*curSyms[] is an array of all vertices -- pretenders to be a vertex
     number chble. This routine tries every element of curSyms to be
     of number "chble".
    */

   /* start filling up curSyms by preteders:*/
   if(NmaxCurSyms<3){
      /*Factorial exhaustive search:*/
     curSyms=get_mem(NmaxCurSyms,sizeof(char));
      for(i=0; i<NmaxCurSyms;i++)
         curSyms[i]=i+chble;
      NCurSyms=NmaxCurSyms;
   }else{
      /* This block tries to sort all changaeble vertices along their relations
       * on: first, fixed vertices, and second, other changaeble vertices.
       *
       * (14.05.01: relations to other changaeble vertices are removed, this
       * was an error. Only relations to itself, i.e., tadpoles, were remained.)
       *
       * The relations will be collected in array "mlen", one element per
       *  each changaeble vertex. The array is of the following structures:
       */
      typedef struct {
         int n;   /*Number of vertex*/
         char **m;/*Relations*/
      }mlenType;

      /*Local variables:*/
      mlenType *mlen;
      char c_1=chble-1,nl_1=topol->i_n + 1,i,j,k,l,to,from,d,*syms[2],
           syms_n[2],cs=0,ncs=1,*buf,*tmp;
      /*:local variables*/
      /*Initialization:*/
      mlen=get_mem(NmaxCurSyms, sizeof(mlenType));
      for(i=0;i<NmaxCurSyms; i++){
         mlen[i].n=i+chble;
         mlen[i].m=get_mem(chble,sizeof(char*));
         for(j=0; !(j>c_1); j++){/*c_1=chble-1*/
            mlen[i].m[j]=get_mem(maxVert,sizeof(char));
            mlen[i].m[j][0]='\1';/*This element will never be changed for j<c_1
                                   since here we cannot have tadpoles! But, for
                                   j == c_1, only this element will be used...*/
         }/*for(j=0; j<c_1; j++)*/
      }/*for(i=0;i<NmaxCurSyms; i++)*/

      /*Now -- loop over all internal lines:*/
      for(i=1; i<nl_1; i++){
         to=(topol->i_line)[i].to;
         from=(topol->i_line)[i].from;
         d=to-from;/*the line "length"*/
         if(from < chble){/*relation on fixed block?*/
            if (to >= chble) /*Yes!*/
               mlen[to-chble].m[from-1][d]++;
         }else{/*Relation on non-fixed vertex*/
            /*14.05.01: relations to other changaeble vertices are removed, this
             *was an error. Only relations to itself, i.e., tadpoles, were remained:
             */
            if(d==0)/*We can take into account only tadpoles!*/
                mlen[from-chble].m[c_1][d]++;
         }/*if(from < chble){..}else*/
      }/*for(i=0; i<nl_1; i++)*/

      /*Now mlen is ready, we have to sort "m" arrays and compare everithing.*/
      buf=get_mem(nl_1,sizeof(char));/*Allocate array for bitsorting*/
      /*First, all vertices assumed to be equivalent candidates:*/
      syms[0]=get_mem(NmaxCurSyms,sizeof(char));syms_n[0]=NmaxCurSyms;
      for(i=0; i<NmaxCurSyms; i++)syms[0][i]=i;

      /*Second array we only allocate:*/
      syms[1]=get_mem(NmaxCurSyms,sizeof(char));

      /*Cycle over all "m" arrays:*/
      for(i=0; i<chble;i++){
         /*Initial value -- one of the previos maximal candidate:*/
         syms[ncs][0]=syms[cs][0];
         syms_n[ncs]=1;

         /*cycle over all equivalent candidates obtained on previous pass:*/
         for(j=0; j<syms_n[cs];j++){
            k=syms[cs][j];/*index of a candidate number "j", an index of "mlen"*/

            /*Sort of corresponding "m" array:*/
            tmp=mlen[k].m[i]+1;/*Zero elements are always untouched*/
            for(l=0; l<nl_1; l++)buf[l]='\0';/*initialization of working array*/
            if(i<c_1){
               l=maxVert-1;/*-1 since tmp=mlen[].m[]+1*/
               L2Ssort(tmp, buf, l, nl_1);
            }
            /*Last array is used just to collect tadpoles, we need not sort them*/
            /*So, it is sorted...*/

            if(j>0){/*if j == 0 then we only need to sort an array.*/
               /*Compare this relation with the previous maximal one:*/
               d=s_cmp(mlen[k].m[i],mlen[syms[ncs][0]].m[i]);
               if (d>0){/*New maximum!*/
                  syms[ncs][0]=k;
                  syms_n[ncs]=1;
               }else if(d==0){/*New candidate with the same parametes, just add it*/
                  syms[ncs][syms_n[ncs]]=k;
                  syms_n[ncs]++;
               }
            }/*if(j>0)*/
         }/*for(j=0; j<syms_n[cs];j++)*/
         /*Upset cs/ncs:*/
         if(cs==0){
            cs=1;ncs=0;
         }else{
            cs=0;ncs=1;
         }/*if(cs==0)*/

         /*Now syms[cs] contains indices of candidates, the nuber of
           candidates are syms_n[cs]*/

         if(syms_n[cs] == 1 )/*Only one candidate, that's all!*/
            break;
      }/*for(i=0; i<chble;i++)*/
      /*Now we only need to store the result:*/
      NCurSyms=syms_n[cs];
      curSyms=get_mem(NCurSyms,sizeof(char));
      for(i=0; i<NCurSyms; i++)
         curSyms[i]=mlen[syms[cs][i]].n;
      /*and free all temporary allocated memory:*/
      free(syms[1]);
      free(syms[0]);
      free(buf);
      for(i=0; i<NmaxCurSyms; i++){
         for(j=c_1; j>='\0'; j--)
            free(mlen[i].m[j]);
         free(mlen[i].m);
      }/*for(i=0; i<NmaxCurSyms; i++)*/
      free(mlen);
   }/*if(g_factorial){...}else*/
   /* end filling up curSyms by preteders*/

   /*Now 1<=NCurSyms <= NmaxCurSyms is a number of pretenders*/
   if( (markSyms=calloc(NCurSyms,sizeof(char)))==NULL)halt(NOTMEMORY,NULL);
   if( (tmpInfo=calloc(NCurSyms,sizeof(tr_infType *)))==NULL)halt(NOTMEMORY,NULL);

   /*First pass:*/
   for(i=0; i<NCurSyms; i++){/*Recursive call on all pretenders*/
      pTOPOL tmpTopol;
      if(curSyms[i]!=chble){/*Create new topology and exchange chble with pretender:*/
         tmpTopol=newTopol(topol);
         mutation(tmpTopol, chble, curSyms[i]);
      }else{/*Topology itself is suitable, just use it:*/
         tmpTopol=topol;
      }/*if(curSyms!=chble){...}else*/
      /*Recursion:*/
      tmpInfo[i]=tr_reduceTopology(tmpTopol,chble+1,maxVert,maxSyms);

      if(curSyms[i]!=chble){
         free(tmpTopol); /*Not need it anymore*/
      }/*if(curSyms[i]!=chble)*/

      d=top_LEG(tmpInfo[i]->topol,minTopol);/*Compare old and new topologies.*/

      if(d==0){/*One more symmetry*/
         markSyms[i]=CSyms;/*Mark this pretender*/
         NSyms+=tmpInfo[i]->n;/*Add numbers of symmetries for this pretender*/
      }else if(d==-1){/*New minimal topology found*/
         minTopol=tmpInfo[i]->topol;/*Reset minimal topology*/
         CSyms++;/*create new marker*/
         NSyms=tmpInfo[i]->n;/*Number of symmetries in new pretender*/
         markSyms[i]=CSyms;/*Mark this pretender*/
      }/*if(d==-1){...}else if(d==-1)*/
   }/*for(i=0; i<NCurSyms; i++)*/
   if ( NSyms == 0){/*There is no valid pretenders!
                      This means that the algorithm is invalid...*/
      halt(INTERNALERROR,NULL);
   }
   /*Here we have 1<=NSyms*/
   /*There may be too many symmetries. On the other hand,
   sometimes we are not interested in them at all:*/
   if(NSyms > maxSyms)NSyms=maxSyms;
   /*Now we can allocate pointers to all symmetries:*/
   if(  (ret->vsubst=(char**)calloc(NSyms,sizeof(char*)))==NULL  )
      halt(NOTMEMORY,NULL);
   ret->n=NSyms;
   /*Second pass:*/
   for(j=i=0; i<NCurSyms; i++){/*Loop on all pretenders*/
      if( markSyms[i]==CSyms ){/*One of minimal topology.*/
         if( (ret->topol)==NULL )/*So, we have to create minimal topology here:*/
           (ret->topol) = newTopol(tmpInfo[i]->topol);
         free(tmpInfo[i]->topol);/*And delete stored topology*/
         for(k=0; k<tmpInfo[i]->n; k++){/*Loop on all symmetries*/
            if(j<NSyms){/*if not, we are out of bound:(*/
               /*Store the symmetry:*/
               (ret->vsubst)[j]=(tmpInfo[i]->vsubst)[k];
               /*ATTENTION! Just the next four lines create ALL substitutions:*/
               if(chble!=curSyms[i]){
                  /*We must exchange chble and curSyms[i] vertices:*/
                  (ret->vsubst)[j][chble]=curSyms[i];
                  for(tmpCh=chble+1;(ret->vsubst)[j][tmpCh] != curSyms[i];tmpCh++)
                     if(tmpCh>maxVert)halt(INTERNALERROR,NULL);
                  (ret->vsubst)[j][tmpCh]=chble;
               }/*if(chble!=curSyms[i])*/
               /******************************************/
               j++;
            }else{
               free(  (tmpInfo[i]->vsubst)[k]  );
            }/*if(j<NSyms){...}else*/
         }/*for(k=0; k<tmpInfo[i]->n; k++)*/
         free(tmpInfo[i]->vsubst);/*Not needed*/
         /*Now tmpInfo[i] is not needed anymore, we can delete it safely.*/
      }else{/*Just deletel everything*/
         free(tmpInfo[i]->topol);
         /*delete all symmetries:*/
         for(k=0; k<tmpInfo[i]->n; k++)
            free(  (tmpInfo[i]->vsubst)[k]  );
         free(tmpInfo[i]->vsubst);
         /*tmpInfo[i] is clean. We can delete it safely.*/
      }/*if( markSyms[i]==CSyms ){...}else*/
      /* Free all returned structure itself:*/
      free(tmpInfo[i]);

   }/*for(i=0; i<NCurSyms; i++)*/
   /*Now we have ready ret. So just clear all temporary allocated variables and return*/
   free(tmpInfo);
   free(markSyms);
   free(curSyms);
   return ret;
}/*tr_reduceTopology*/

/* If first_changed_vertex==0 then this function will determine this itself.
  i_topology -- initial, r_topology -- lexicograph. reduced.
  Note, i_topology must be sorted as by sortinternalpartoftopology():*/
int reduce_topology(int first_changed_vertex,pTOPOL i_topology, pTOPOL r_topology,
                    char *l_subst, char *v_subst,char *l_directions)
{
 char was_exchange=0,last_vertex=0;
 tr_infType *infTop=NULL;
 int i;

   if(!(first_changed_vertex>0)){/*We must determine first changeable vertex*/
      /* Looking for the first changeable number of vertex:*/
      for(i=i_topology->e_n;i>0;i--){
         if(i_topology->e_line[i].to>first_changed_vertex)

         first_changed_vertex=i_topology->e_line[i].to;
      }
      first_changed_vertex++;
   }

   /* Looking for last vertex number:*/
   for( i=i_topology->i_n;i>0;i--)
      if(last_vertex<i_topology->i_line[i].to)
         last_vertex=i_topology->i_line[i].to;

   /*Checking can topology be changed:*/
   if (first_changed_vertex>last_vertex){
      top_let(i_topology, r_topology);/*Copy original topology to new */
      return(0);
   }/*if (first_changed_vertex>last_vertex)*/

   if(
       (infTop=tr_reduceTopology(i_topology,first_changed_vertex,last_vertex,1))
        ==NULL
     )halt(INTERNALERROR,NULL);

#ifdef SKIP
/*!!! 1: change 100->1 previously! 2: remove the following group:*/
   {
     int i,j;
     for(i=0; i<infTop->n; i++){
        fprintf(stderr, "sym. %d=",i+1);
        for(j=1;j<=last_vertex; j++)
           fprintf(stderr, "%d",(infTop->vsubst)[i][j]);
        fprintf(stderr, "\n");
     }
   }
#endif

   s_let( (infTop->vsubst)[0],v_subst );
   *invertsubstitution(v_subst, (infTop->vsubst)[0])=-1;
   top_let(infTop->topol, r_topology);
   free(infTop->topol);
   for(i=0; i<infTop->n; i++)
      free((infTop->vsubst)[i]);
   free(infTop->vsubst);
   free(infTop);
   for( was_exchange=i_topology->i_n;was_exchange>'\0';was_exchange--)
      if(v_subst[was_exchange]!=was_exchange)break;/*And was_exchange>0!!*/

   if((was_exchange!='\0')&&(l_directions!=NULL))
      get_l_subst(i_topology, r_topology,l_subst,v_subst,l_directions);

   if(was_exchange)was_exchange=1;/*Reduce to 1, if "true".*/
   return(was_exchange);
}/*reduce_topology*/

/*i2 - conventional momentum (array of integers - +- indices of groups, 
   i2[0] is the length. wm: wm[i2[i]] is the coefficient 
   for the corresponding group.
*/
static void l_addMom2wm(int *wm, int *i2, int s)
{
   register int i,v;
   if (i2==NULL) return;/*Second summand is NULL, do nothing*/
   if(s==0)/*upset*/
      for(i=1; i<=*i2 ;i++){
         v=i2[i];
         if(v<0)
            wm[-v]+=1;
         else
            wm[v]-=1;
      }/*for(i=0; i<*i2 ;i++)*/
   else /*as is*/
      for(i=1; i<=*i2 ;i++){
         v=i2[i];
         if(v<0)
            wm[-v]-=1;
         else
            wm[v]+=1;
      }/*for(i=0; i<*i2 ;i++)*/
}/*addMom2wm*/

static void l_mergewm( int *wm1, int *wm2, int s)
{
register int i;
   if(s==0)/*upset*/ for(i=1;i<top_vec_group;i++) wm1[i]-=wm2[i];
   else for(i=1;i<top_vec_group;i++) wm1[i]+=wm2[i];
}/*l_mergewm*/

static int *l_wm2mom(int *wm, int *zmom,int *msk,int *ptrs, int elen)
{
register int i,j,k,l=1;
int *ret;

   if(elen>0){
      k=wm[ptrs[0]]/msk[0];/*all elements msk !=0 by construction!*/
      if(k!=0)/*All 0, it IS possible!*/
         i=1;
      else
         i=elen+1;
      for(;i<elen;i++)
         if(wm[ptrs[i]]/msk[i]!=k)
            break;
      /*Now if there are external momenta i==elen*/
      if(i==elen)/*Remove submomenta proportional sum of external momenta:*/
         for(i=0;i<elen;i++)
            wm[ptrs[i]]=0;
   }/*if(elen>0)*/

#ifdef SKIP
   /*Switch it off - this stuff seems to be an extra one!*/

   if(zmom[0] == 1){/*Zero momentum consists in one group - try to eliminate it*/
      j=abs(zmom[1]);
      for(i=1;i<top_vec_group;i++){
         k=abs(wm[i]);
         if(k!=j)/*Non-zero momentum*/
            l+=abs(wm[i]);
         else/*Zero momentum - eliminate it:*/
            wm[i]=0;
      }/*for(i=1;i<top_vec_group;i++)*/
   }else/*Can't eliminate zero momentum since it is complicated*/
      for(i=1;i<top_vec_group;i++)l+=abs(wm[i]);
#endif

   for(i=1;i<top_vec_group;i++)l+=abs(wm[i]);

   if(l==1)/*empty momentum*/
      return int_inc(NULL,zmom,1);

   ret=get_mem(l,sizeof(int));
   for(i=1;i<top_vec_group;i++)if((k=wm[i])!=0){
      if(k<0){j=-k;k=-i;}else{j=k;k=i;}
      for(;j>0;j--)
         ret[++*ret]=k;
   }/*for(i=1;i<top_vec_group;i++)if((k=wm[i])!=0)*/

   return ret;
}/*l_wm2mom*/

/*Returns: <0 - error, 0 -pure vertex, successively distributed >0*/
int distribute_momenta_groups(
   pLINE elines,/*external lines*/
   pLINE ilines,/*internal lines*/
   char nel,/*number of external lines*/
   char nil,/*number of  internal lines*/
   int **mloop,/*loop momenta (starting from 0!)*/
   int nloop,/*number of loop momenta*/
   int *zmom,/*zero momentum*/
   int **emom,/*external momenta (starting from 1!)*/
   int **imom,/*internal momenta. This is RETURNED value. It must be initialised:
               allocated int *imom[nil+1], all NULL.*/
   char *ldirs,/*directions ldirs[usr] usr <-> red, starting from 1*/
   char *mloopMarks/*the array of length nil+1, NOT an ASCII-Z! E.g :
                     -1 0 0 2 0 1 -- means that the first loop momenta must be at the 5
                        1 2 3 4 5    and the second - at 3.
                     This pointer may be NULL, if the order of integration momenta is not
                     a matter.*/
   )
{
   char from,to;
   char newn;
   char s;
   int top_loop=0;
   pLINE lines=NULL;
   char **rulers=NULL;
   int **wm=NULL;
   char *subst;
   int i,j,ii,rindex;

   if(nil==0)return 0;/*Pure vertex, nothing to distribute!*/
   if(
      (elines==NULL)||
      (ilines==NULL)||
      (emom==NULL)||
      (imom==NULL)||
      (nel<'\1')||
      (nil<'\1')
   )return -1;
   /*mloop may by NULL for trees, e.g.*/
   /*Copy input topology into local buffers:*/
   lines=get_mem(nel+1,sizeof(tLINE));
   for(i=1; i<=nel; i++){
      lines[i].to=elines[i].to;
      lines[i].from=elines[i].from;
   }/*for(i=1; i<=nel; i++)*/
   elines=lines;

   lines=get_mem(nil+1,sizeof(tLINE));

   *(subst=get_mem(nil+2,sizeof(char)))=-1;

   if(mloopMarks==NULL)/*Just 1234...*/
      for(i=1; i<=nil; i++)subst[i]=i;
   else
      /* Make a substitution so that the line with the first loop momenta required will
         be the last, 2 - penult, and so on. Other lines just shifted to the begin. E.g.:
         mloopMarks: -1 0 0 2 0 1 0, then the subst: -1 1 2 4 6 3 5
       */
      for(i=1,j=1; i<=nil; i++){
         if(mloopMarks[i]=='\0')
            subst[j++]=i;
         else
            subst[nil-mloopMarks[i]+1]=i;
      }/*for(i=1,j=1; i<=nil; i++)*/
   /* So, now subst[new]=old.*/

   /*Now copy topology lines into the local working array, reordering them according to
     subst. This reordering leads to the fact that the lines with smallest loop momenta
     required appear at the end:*/
   for(i=1; i<=nil; i++){/*Assume i is new*/
      lines[i].to=ilines[subst[i]].to;
      lines[i].from=ilines[subst[i]].from;
   }/*for(i=1; i<=nel; i++)*/
   ilines=lines;

   rulers=get_mem(nil,sizeof(char *));
   wm=get_mem(nil,sizeof(int*));
   for(i=0; i<nil; i++){/*Loop on all internal lines*/
      ii=i+1;/*The line index*/
      wm[i]=get_mem(top_vec_group,sizeof(int));
      from=ilines[ii].from; to=ilines[ii].to;
      if(from==to)/*A tadpole, rulers[i] == NULL*/
         continue;
      if(from<to)newn=from; else newn=to;
      rulers[i]=get_mem(nel+nil-ii,sizeof(char));
      for(j=0; j<nel; j++){
         lines=elines+j+1;/* Get the working line*/
         if(lines->to==from){
            lines->to=newn;/*summing up*/
            rulers[i][j]=1;/*external line are always ingoing*/
         }/*No tadpoles here, so lines->to may be "to" only if lines->to!=from:*/
         else{
            if( lines->to==to) lines->to=newn;/*summing up*/
         }/*else*/
      }/*for(j=0; j<nel; j++)*/
      /*Ok, external lines are performed. Now turn to internal ones:*/
      for(rindex=nel,j=ii; j<nil; j++,rindex++){
         lines=ilines+j+1;/* Get the working line*/
         if(lines->from==from){
            lines->from=newn;/*summing up*/
            rulers[i][rindex]=-1;/*Momentum is upset!*/
         }/*No tadpoles here, so lines->from may be "to" only if lines->from!=from:*/
         else{
            if( lines->from==to) lines->from=newn;/*summing up*/
         }/*else*/
         if(lines->to==from){
            lines->to=newn;/*summing up*/
            rulers[i][rindex]+=1;/*correct momentum, in case of a tadpole must be 0!*/
         }/*No tadpoles here, so lines->to may be "to" only if lines->to!=from:*/
         else{
            if( lines->to==to) lines->to=newn;/*summing up*/
         }/*else*/
      }/*for(j=0; j<nil; j++)*/
   }/*for(i=0; i<nil; i++)*/
   /* That's all, rulers are ready*/
   for(i=nil-1;i>=0;i--){
      if(rulers[i]==NULL){/*A tadpole!*/
         if(top_loop>=nloop)
            halt(UNSUFFICIENTLOOPMOMS,NULL);
         /*l_addMom2wm(wm[i], mloop[top_loop++], 1); !=0-as is, 0 - upset:*/
         l_addMom2wm(wm[i], mloop[top_loop++],ldirs[subst[i+1]]+1);
         if(mloopMarks!=NULL){
            mloopMarks[subst[i+1]]=top_loop;/*top_loop is counted from 0!*/
         }/*if(mloopMarks!=NULL)*/
         continue;
      }/*if(rulers[i]==NULL)*/
      for(j=0; j<nel;j++){/*External lines*/
         if(  (s=rulers[i][j])!='\0' )/*Source!*/
            l_addMom2wm(wm[i], emom[j+1], s+1);/* Here s=+-1; s==0->upset before appending*/
      }/*for(j=0; j<nel;j++)*/
      /*Internal lines:*/
      for(rindex=nel,j=i+1; j<nil; j++,rindex++){
         if(  (s=rulers[i][rindex])!='\0' )/*Source!*/
            l_mergewm(wm[i], wm[j], s+1);
      }/*for(rindex=nel,j=i+1; j<nil; j++,rindex++)*/
      free_mem(rulers+i);
   }/*for(i=nil-1;i>=0;i--)*/

   /*Now convert mw into imom:*/
   {/*block*/
    int *msk, *ptrs, elen=0,*ewm;
       /*We will try to remove subexpressions proportional to a sum of
         the external momenta:*/
       ewm=get_mem(top_vec_group,sizeof(int));
       msk=get_mem(top_vec_group,sizeof(int));
       ptrs=get_mem(top_vec_group,sizeof(int));
       for(i=1; i<=nel; i++)
          l_addMom2wm(ewm,emom[i],1);

       for(i=1;i<top_vec_group;i++)if(ewm[i]!=0){
           msk[elen]=ewm[i];
           ptrs[elen]=i;
           elen++;
       }/*for(i=1;i<top_vec_group;i++)if(ewm[i])*/
      /*Now msk[] is a subexpression -- sum of external momenta,
        ptrs[] corresponding indices, elen is the number of terms.*/
      for(i=0; i<nil; i++){/*Loop on all internal lines*/
         /* Do not forget to restore original momenta numbering!:*/
         imom[subst[i+1]]=l_wm2mom(wm[i],zmom,msk,ptrs,elen);
         free(wm[i]);
      }/*for(i=0; i<nil; i++)*/
      free(ptrs);
      free(msk);
      free(ewm);
   }/*block*/
   free(wm);
   free(rulers);
   free(ilines);
   free(subst);
   free(elines);
   return 1;
}/*distribute_momenta_groups*/

/*Reduces momentum by simplifying groups and (possible) extracting emom.
  If the resulting momentum appears to be 0, replaces it by zmom.
  This function destroys old value of momentum and allocates new one.*/
int *reduce_momentum(
                        int *momentum,
                        int *zmom,/*zero momentum*/
                        int *emom/*Sum of incoming external momenta*/
                    )
{
    int *msk, *ptrs, elen=0,*ewm,*wm,i;

       /*Wi will try to remove subexpressions proportional to a sum of
         the external momenta:*/
       wm=get_mem(top_vec_group,sizeof(int));
       ewm=get_mem(top_vec_group,sizeof(int));
       msk=get_mem(top_vec_group,sizeof(int));
       ptrs=get_mem(top_vec_group,sizeof(int));

       l_addMom2wm(wm,momentum,1);
       free(momentum);

       l_addMom2wm(ewm,emom,1);

       for(i=1;i<top_vec_group;i++)if(ewm[i]!=0){
           msk[elen]=ewm[i];
           ptrs[elen]=i;
           elen++;
       }/*for(i=1;i<top_vec_group;i++)if(ewm[i])*/
      /*Now msk[] is a subexpression -- sum of external momenta,
        ptrs[] corresponding indices, elen is the number of terms.*/

      momentum=l_wm2mom(wm,zmom,msk,ptrs,elen);

      free(ptrs);
      free(msk);
      free(ewm);
      free(wm);
      return momentum;
}/*reduce_momentum*/

char *check_momenta_balance(
   char *returnedbuf,
   pLINE elines,/*external lines*/
   pLINE ilines,/*internal lines*/
   char nel,/*number of external lines*/
   char nil,/*number of  internal lines*/
   char nv,/*number of vertices*/
   char *is_ingoing,/*0 if outgoing, 1 if ingoing*/
   int *zmom,/*zero momentum*/
   int **emom,/*external momenta*/
   int **imom/*internal momenta*/
   )
{
int **wrkmom,/*In this array we will collect all momenta incoming to vertices*/
    *sumEmom,/*Sum*/
    i,
    from,to;
char *ptr=returnedbuf;
   if(nil == '\0')return s_let("",returnedbuf);/*Pure vertex, nothing to check!*/
   wrkmom=get_mem(nv+1,sizeof(int*));/*Started from 1, not 0!*/
   sumEmom=get_mem(1,sizeof(int));/*Will be a sum of incoming momenta*/

   for(i=1; i<= nel;i++){
      to=elines[i].to;
      /*Bulid sum of incoming momenta:*/
      sumEmom=int_inc(sumEmom,
                        emom[i],
                        is_ingoing[i]
                  );
      wrkmom[to]=int_inc(wrkmom[to],emom[i],is_ingoing[i]);
   }/*for(i=1; i<= nel;i++)*/

   for(i=1; i<= nil;i++){
      to=ilines[i].to;
      from=ilines[i].from;
      wrkmom[to]=int_inc(wrkmom[to],imom[i],1);
      wrkmom[from]=int_inc(wrkmom[from],imom[i],0);
   }/*for(i=1; i<= nil;i++)*/
   for(i=1; i<= nv;i++){
      wrkmom[i]=
         reduce_momentum(
            reduce_momentum(wrkmom[i],zmom,sumEmom),
            NULL,
            zmom);
      if( wrkmom[i][0]!=0 )*ptr++=i;
      free(wrkmom[i]);
   }/*for(i=1; i<= nv;i++)*/
   *ptr='\0';
   free(sumEmom);
   free(wrkmom);
   return ptr;
}/*check_momenta_balance*/

