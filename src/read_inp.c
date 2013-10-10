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
#include <stdlib.h>
#include <stdio.h>
#include "tools.h"
#include "comdef.h"
#include "types.h"
#include "variabls.h"
#include "headers.h"
#include "texts.h"
#include "tt_type.h"
#include "tt_gate.h"
#define stINIT 0
#define stEND 1
#define stMINUS 2
#define stBRACKET 3
#define stDIGIT 4
#define stREADID 5
#define stREADLINENUM 6
#define stREADSEPARATOR 7

extern tt_table_type **tt_table;/*tt_load.c*/

void skip_until_first_actual_diagram(void)
{
   char str[MAX_STR_LEN];
   long l=0,tmp=start_diagram-1;
   int flag=0;
     while(1){
       if(
          (*sc_get_token(str)=='-')&&
          (*sc_get_token(str)=='-')&&
          (*sc_get_token(str)=='#')
       ){
          if(start_diagram==1)return;
          if(flag)sc_mark();

          if(*sc_get_token(str)=='['){
              if(*sc_get_token(str)!='d')
                halt(CURRUPTEDINPUT,input_name);
              sscanf(str+1,"%ld",&l);
              if(l==start_diagram){
                sc_repeat();
                return;
              }else if (l>start_diagram)
                 halt(CURRUPTEDINPUT,input_name);
          }else if((*str==']')&&(!(l<tmp))){
            flag=1;
          }
       }
     }
}/*skip_until_first_actual_diagram*/

void skip_to_new_diagram(void)
{
  char str[MAX_STR_LEN];
     while(*sc_get_token(str)!='[');
}/*skip_to_new_diagram*/

void detect_number_and_coefficient(void)
{
 char tmp[MAX_STR_LEN];
 int state=stINIT;
    sc_get_token(tmp);
    sscanf(&(tmp[1]),"%u",&diagram_count);/* skip first 'd'
                                         and read number*/

    sc_get_token(tmp);/*Skip ':' */
    sc_get_token(tmp);/*Skip '*' */

    while(state!=stEND)switch(state){
        case stINIT:
           if(is_digits(sc_get_token(tmp))){
              state = stDIGIT;
              letnv(tmp,coefficient,NUM_STR_LEN);
           }else if((*tmp)=='('){
               state=stBRACKET;
           }else if((*tmp)=='-')state=stMINUS;
           break;
        case stMINUS:
            s_let("-",coefficient);
           /*no break -- continue process '-':*/
        case stBRACKET:
           if(is_digits(sc_get_token(tmp))){
              s_cat(coefficient,coefficient,tmp);/* 'cat' , not 'let' ! */
              state=stDIGIT;
           } else if((*tmp)=='-')state=stMINUS;
           break;
        case stDIGIT:
           if(*sc_get_token(tmp)=='/'){
              s_cat(coefficient,coefficient,tmp);
              state=stBRACKET;/* expect new digit */
           }else state=stEND;
           break;
    }
}/*detect_number_and_coefficient*/

void new_diagram(void)
{
  int i,j;
  *coefficient=0;
  vcount=0;
  int_lines=0;
  thisistadpole=0;
  unset_bit(&mode,bitTERROR);
  for(i=0;!(i>MAX_VERTEX);i++)
    for(j=-ext_lines;!(j>MAX_I_LINE);j++)
       dtable[i][j]=wrktable[i][j]=0;

  free_mem(&l_outPos);
  free_mem(&v_outPos);

  free_mem(&g_left_spinors);
  free_mem(&g_right_spinors);

  if(new_topol==NULL)
     new_topol=get_mem(1,sizeof(tTOPOL));
  if(old_topol==NULL)
     old_topol=get_mem(1,sizeof(tTOPOL));

  if(text!=NULL){
    for(i=0; i<top_text;i++)
       free_mem(&(text[i]));
    top_text=0;
  }

  if(formout!=NULL){
    for(i=0; i<top_formout;i++)
       free_mem(&(formout[i]));
    top_formout=0;
  }

  for(i=0; i<numberOfIndicesGroups;i++)
     iNdex[i]=indices[i];

  clear_lvmarks();

  usedindex_all=1;
  index_all=1;

  text_created=0;

  top_identifiers=0;

  cn_momentaset=0;
  cn_topologyid=0;
  canonical_topology=NULL;
  internal_canonical_topology=NULL;
  g_topologyLabel=NULL;
}/*new_diagram*/

void create_primary_diagram_table(void)
{
  int state;
  word *intid;
  int cline;
  char str[MAX_STR_LEN];
    for(state=stINIT; state!=stEND;)switch(state){/*Begin finite automaton*/
       case stINIT:
          if(*sc_get_token(str)!='v'){
             if(*str!='*')
                halt(CURRUPTEDINPUT,input_name);
             if(*sc_get_token(str)=='*'){
                state=stEND;
                break;
             }else{
                if(*str!='v')
                   halt(CURRUPTEDINPUT,input_name);
                if(*sc_get_token(str)!='(')
                   halt(CURRUPTEDINPUT,input_name);
             }
          }else{
            if(*sc_get_token(str)!='(')
               halt(CURRUPTEDINPUT,input_name);
          }
          vcount++;
          if(!(vcount<MAX_VERTEX))
             halt(TOOMANYVETICES,NULL);
          state=stREADID;
          break;
       case stREADID:
          if(
             (intid=lookup(s_cat(str,"\1",sc_get_token(str)),main_id_table))
           ==NULL)halt(UNDEFINEDID,str+1);
          if(*sc_get_token(str)!='(')
             halt(CURRUPTEDINPUT,input_name);
          state=stREADLINENUM;
          /*No break!*/
       case stREADLINENUM:
          cline=2048;
          if(*sc_get_token(str)=='-'){/*External line*/
            sscanf(sc_get_token(str),"%d",&cline);
            if(((cline=0-qgraf_ext[cline]-1)==0)||(cline>ext_lines))
               halt(WRONGEXTERNALLINE,str);
            if(s_scmp(ext_particles[-cline-1].id,id[*intid].id))
               halt(WRONGEXTERNALPART,NULL);
            dtable[vcount][cline]=(*intid)+1;
          }else{/*Internal line*/
            sscanf(str,"%d",&cline);
            if(!(cline<MAX_I_LINE))
               halt(TOOMANYLINES,NULL);
            if(dtable[vcount][cline]){/*Tadpole -- set id of propagator*/
               dtable[vcount][cline]=id[dtable[vcount][cline]-1].link[1]+1;
               thisistadpole=1;
            }else
               dtable[vcount][cline]=(*intid)+1;
            if(int_lines<cline)int_lines=cline;
          }
          if(*sc_get_token(str)!=')')
               halt(CURRUPTEDINPUT,input_name);

          state=stREADSEPARATOR;
          /*No break!*/
       case stREADSEPARATOR:
          if(*sc_get_token(str)==comma_char)
            state=stREADID;
          else if (*str==')')
            state=stINIT;
          else
            halt(CURRUPTEDINPUT,input_name);
          break;
    }/*End finite automaton*/
}/*create_primary_diagram_table*/

void define_topology(void)
{
  int i,j,k;
  word *tmp;

    /* Reduce external particles:*/

    /* First step: order of the vertices:*/
    k=1;/*We will set particle "i" to the vertex "k".*/
    for(i=1;!(i>ext_lines);i++)
       for(j=k;!(j>vcount);j++)
          if(dtable[j][-i]){/* Reduce particle "i" to the vertex "k"*/
            if(j!=k){/*Exchange vertices j and k */
               tmp=dtable[j];
               dtable[j]=dtable[k];
               dtable[k]=tmp;
            }
            k++;/*Following particle will be reduced to following vertex.*/
            break;/*External particle is found, there are no more at this
                    line!*/
          }

    /*Now external particles are placed so we may read topology.*/
    /*External lines:*/
    for(i=1;!(i>ext_lines);i++){
       for(j=1;!(j>vcount);j++)
          if(dtable[j][-i])
             break;
       old_topol->e_line[i].from=new_topol->e_line[i].from=-i;
       old_topol->e_line[i].to=new_topol->e_line[i].to=j;
    }
   /* Internal lines:*/
   for(j=1; !(j>vcount);j++){
      for(i=1;!(i>int_lines);i++){
         if(dtable[j][i]){
            if(  *(id[dtable[j][i]-1].id)==2  )/*Tadpole*/
               old_topol->i_line[i].from=old_topol->i_line[i].to=
               new_topol->i_line[i].from=new_topol->i_line[i].to=j;
            else{
               if(j!=vcount)for(k=j+1;!(k>vcount);k++){
                  if(dtable[k][i]){
                     old_topol->i_line[i].from=new_topol->i_line[i].from=j;
                     old_topol->i_line[i].to=new_topol->i_line[i].to=k;
                  }
               }
            }
         }
      }
   }
   old_topol->i_n=new_topol->i_n=int_lines;
   old_topol->e_n=new_topol->e_n=ext_lines;
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

   for(i=1;!(i>int_lines);i++){
      l_subst[i]=il_subst[i]=i;
      l_dir[i]=1;
   }
   for(i=1;!(i>vcount);i++){
       v_subst[i]=iv_subst[i]=i;
   }
   /* Now we should sort internal lines:*/
   qsort((void *)((old_topol->i_line)+1),old_topol->i_n,sizeof(tLINE),top_cmp);
   /* Now we must create l_subst according to sorted topology:*/
   {/*block begin*/
     set_of_char markline;
     char j,*l_s=l_subst+1;
      set_sub(markline,markline,markline);/* clear markline */
        for(i=1;!(i>new_topol->i_n);i++)
          for(j=1;!(j>old_topol->i_n);j++){
            if(
               (
                 (old_topol->i_line[j].from==new_topol->i_line[i].from)&&
                 (old_topol->i_line[j].to==new_topol->i_line[i].to)
               )||
               (
                 (old_topol->i_line[j].from==new_topol->i_line[i].to)&&
                 (old_topol->i_line[j].to==new_topol->i_line[i].from)
               )
              ){
                if(!set_in(j,markline)){
                   *(l_s)++ = j;
                   set_set(j,markline);
                   break;
                }
            }
          }
   }/*block end*/
}/*define_topology*/

static char *s123etc(char *str)
{
register int i;
   for(i=1; str[i]!='\0'; i++)str[i]=i;
   return(str);
}/*s123etc*/

static char *s111etc(char *str)
{
register int i;
   for(i=1; str[i]!='\0'; i++)str[i]='\1';
   return(str);
}/*s111etc*/

/*checks g_ignoreMomenta,momentaOrShape field, "NoMom" and "NoShape", compares
  them with mOrC and RETURNs 0, if this topology does not fit, or 1, it it fits:*/
int chskShapeMom(int id, long ind,int mOrC/*momenta 0 coords 1*/)
{

   if(mOrC==0){
      /*Momenta required*/
      if(g_ignoreMomenta) return 0;
      if( (tt_table[--id])->momentaOrShape<0 ) return 0;
      {/*Block*/
                                   /*id was decremented!:*/
         tt_singletopol_type *t=(tt_table[id]->topols)[ind-1];
         /*getTopolRem  see remarks.c:*/
         if(getTopolRem("NoMom", t->top_remarks, t->remarks ) !=NULL)
            return 0;
      }/*Block*/
   }else{
      /*Shape required*/
      tt_singletopol_type *t=(tt_table[id-1]->topols)[ind-1];
      int themode=INT_VERT;
      if( (tt_table[id-1])->momentaOrShape>0 ) return 0;
      /*getTopolRem  see remarks.c:*/
      if(getTopolRem("NoShape", t->top_remarks, t->remarks ) !=NULL)
          return 0;
      /*tt_coordinatesOk returns 0 if all mentioned types of coordinates
        are present, otherwise, !=0 :*/
      if( (t->nil)>0  )/*Internal lines must be here*/
         themode|=INT_LINE;
      if(tt_coordinatesOk(id, ind,themode)!=0)
         return 0;
   }/*if(mOrC)...else*/
   return 1;
}/*chskShapeMom*/

/*Reurns 0 - not found, 1- full, 2- wrk, 3- int, 4 - pure vertex topology
  not found among full topology tables;
  *t may be used for adding to the table.
  *id -- id of table, *ind -- toplogy index, *wrkind  ==0 if not found in wrk, or wrk
  index, if found (may be, with incompatible type):*/
int l_lookup_tbl(tt_singletopol_type **t,
                 int *id, long *ind,long *wrkind,int mOrC/*momenta 0 coords 1*/)
{
int i;
int mv=topologies[cn_topol].max_vertex;
   /**t is assumed to be NULL.*/
   *wrkind=0;/*Reset the wrk table flag*/

   if( (g_tt_full!=NULL)&&(*g_tt_full>0) ){
      if(  (*t=tt_initSingleTopology())==NULL   )halt(NOTMEMORY,NULL);
      /*Initialise t via tt_lookupFromTop first time:*/
      *id=*g_tt_full;/*Set the table to the first full topology table*/
      *ind=tt_lookupFromTop(*t,
                       *id,
                       topologies[cn_topol].topology,
                       mv);
      if( *ind>0) {
         /*The topology is found in the first full table*/
         if( chskShapeMom(*id,*ind,mOrC)!=0 )
                return 1;
      }/*if( *ind>0)*/
      /* Now process through the table:*/
      for(i=1; i<g_tt_top_full; i++)
         if(   (*ind=tt_lookupFromTop(*t,g_tt_full[i],NULL,mv)) > 0  ){/*Found*/
            *id=g_tt_full[i];
            if(chskShapeMom(*id,*ind,mOrC)!=0)
               return 1;
         }/*if(   (*ind=tt_lookupFromTop(*t,g_tt_full[i],NULL,mv)) > 0  )*/

      /*Will be re-initilised:*/
      tt_clearSingleTopology(*t);
   }/*if(g_tt_full>0)*/
   /*Not found in full.*/

   if( (topologies[cn_topol].topology)->i_n == 0 ){
         /*pure vertex topology*/
         *t=NULL;
         return 4;
   }/*if( (topologies[cn_topol].topology)->i_n == 0 )*/

   /*Try wrk:*/

   /*Re-initilise, it must be internal now!*/
   if(  (*t=tt_initSingleTopology())==NULL   )halt(NOTMEMORY,NULL);
   if(canonical_topology == NULL)/*Canonical topology not ready. Create it*/
         make_canonical_topology();
   /*Now canonical topolgy is ready together with ltsubst, lsubst,
      vtsubst, vsubst; See "The substituions map" in "variables.h"*/

   if(   (*ind=tt_lookupFromTop(*t,g_tt_wrk,internal_canonical_topology,mv))>0  ){/*found*/
         *id=g_tt_wrk;
         *wrkind=*ind;
         if(chskShapeMom(*id,*ind,mOrC)!=0)
            return 2;
   }/*if(   (*ind=tt_lookupFromTop(*t,g_tt_wrk,canonical_topology,mv))>0  )*/
   /*Ok, not found in wrk*/
   /*Try internal tables. *t is initialised already.*/
   for(i=0; i<g_tt_top_int; i++)
         if(   (*ind=tt_lookupFromTop(*t,g_tt_int[i],NULL,mv)) > 0  ){/*Found*/
            *id=g_tt_int[i];
            if(chskShapeMom(*id,*ind,mOrC)!=0)
               return 3;
         }/*if(   (*ind=tt_lookupFromTop(*t,g_tt_int[i],NULL,mv)) > 0  )*/

   /*Nothig found. Return 0:*/
   *id=0;*ind=0;
   return 0;
}/*l_lookup_tbl*/

/*Auxiliary procedure, used mainly to allocate a structure:*/
static aTOPOL *l_canonical2top(void)
{
aTOPOL *wrk=get_mem(1,sizeof(aTOPOL));
   wrk->topology=internal_canonical_topology;
   wrk->orig=internal_canonical_topology;

   return wrk;
}/*l_canonical2top*/

static void l_create_links(int id,long ind,int thetype,int mOrC/*momenta 0 coords 1*/)
{
char *tbl,*top;
   if(id==g_tt_wrk)id=0;/*g_tt_wrk may be changed!*/
   /*Use 0 instead of g_tt_wrk!!*/

   if(mOrC){/*coords*/
      tbl=new_str("Ctbl");top=new_str("Ctop");
   }else{/*momenta*/
      tbl=new_str("Mtbl");top=new_str("Mtop");
   }

   /*Link to table:*/
   setTopolRem(tbl, new_long2str(id),NULL,
                                  &(topologies[cn_topol].top_remarks),
                                  &(topologies[cn_topol].remarks),
                                NULL);
   /*Link to topology in the table:*/
   setTopolRem(top, new_long2str(ind),NULL,
                                  &(topologies[cn_topol].top_remarks),
                                  &(topologies[cn_topol].remarks),
                                NULL);

   if( (id==0)&&(mOrC==0)  ){/*Momenta for wrk -- add a counter*/
      long *c=&(   (tt_table[g_tt_wrk-1]->topols)[ind-1]->duty   );
      if(*c==0)/*First occurence*/
         g_wrk_counter++;
      (*c)++;
   }/*if( (id==g_tt_wrk)&&(mOrC==0)  )*/

   /*Now store substituitions for the canonical topology, if present:*/
   if(canonical_topology != NULL){
      if(
          /*getTopolRem  see remarks.c:*/
          getTopolRem("ltsubst",
                   topologies[cn_topol].top_remarks,
                   topologies[cn_topol].remarks
                     ) ==NULL
        )
         setTopolRem("ltsubst", new_str(ltsubst),NULL,
                                  &(topologies[cn_topol].top_remarks),
                                  &(topologies[cn_topol].remarks),
                                NULL);

      if(
          /*getTopolRem  see remarks.c:*/
          getTopolRem("vtsubst",
                   topologies[cn_topol].top_remarks,
                   topologies[cn_topol].remarks
                     ) ==NULL
        )
         setTopolRem("vtsubst", new_str(vtsubst),NULL,
                                  &(topologies[cn_topol].top_remarks),
                                  &(topologies[cn_topol].remarks),
                                NULL);

      if(
          /*getTopolRem  see remarks.c:*/
          getTopolRem("ldir",
                   topologies[cn_topol].top_remarks,
                   topologies[cn_topol].remarks
                     ) ==NULL
        )
         setTopolRem("ldir", new_str(ldir),NULL,
                                  &(topologies[cn_topol].top_remarks),
                                  &(topologies[cn_topol].remarks),
                                NULL);

   }/*if(canonical_topology != NULL)*/
}/*l_create_links*/

/*Returns :-1 if fails, 0 if pure vertex, +1 in success.
  Allocates momenta for the topology cn_top. NO CHECKING about cn_top!:*/
int automatic_momenta_group(
        char *mloopMarks,/*the array of length nil+1, NOT an ASCII-Z! E.g :
                      -1 0 0 2 0 1 -- means that the first loop momenta must be at the 5
                         1 2 3 4 5    and the second - at 3.
                      This pointer may be NULL, if the order of integration momenta is not
                      a matter.*/
        word cn_top
    )
{
int *tmp, **emom,i,j,k;

   /*Clear former momenta, if present:*/
   if(topologies[cn_top].momenta!=NULL){
     for(k=0; k<topologies[cn_top].n_momenta;k++){
        for(j=1; !(j>(topologies[cn_top].topology)->i_n); j++)
           free(topologies[cn_top].momenta[k][j]);
        free(topologies[cn_top].momenta[k]);
     }/*for(k=0; k<topologies[cn_top].n_momenta;k++)*/
     free(topologies[cn_top].momenta);
   }/*if(topologies[cn_top].momenta!=NULL)*/

   /*Allocate space for one momenta set:*/
   topologies[cn_top].momenta=get_mem(1,sizeof(int **));
   topologies[cn_top].momenta[0]=
        get_mem((topologies[cn_top].topology)->i_n+1,sizeof(int *));
   topologies[cn_top].n_momenta=1;

   if(g_nloop == 0){/*No loop momenta - implement "stupid" algoritm: p1,p2,etc.*/
       int *ptr=NULL;
       if(  (k=(topologies[cn_top].topology)->i_n)==0 )
          return 0;/*pure vertex*/

       /*The group corresponding to momentum may not exist. But, there may be that 
         there are no topologies at all! (But, ext. momenta exist, of course).
         Just for reliability - for future development, if one day I will start to
         work with bubbles:*/
       if(vec_group_table==NULL)
            vec_group_table=create_hash_table(vec_group_hash_size,
                                        str_hash,str_cmp,int_destructor);

       for(i=1; i<=k; i++){
          char *tmp=vec_id[(i-1)%top_vec_id];
          if((ptr=lookup(tmp, vec_group_table))==NULL){/*No such group, create and store:*/
             MOMENT *cell;
             if(!(top_vec_group<max_top_vec_group))
                if(
                    (vec_group=realloc(vec_group,
                    (max_top_vec_group+=MAX_VEC_GROUP)*sizeof(MOMENT)))==NULL
                  )halt(NOTMEMORY,NULL);
             cell=vec_group+top_vec_group;
             cell->vec=new_str(tmp);
             cell->text=new_str(tmp);
             *(ptr=get_mem(1,sizeof(int)))=top_vec_group++;
             install(cell->text,ptr,vec_group_table);
          }/*if((ptr=lookup(tmp, vec_group_table))==NULL)*/
          /* Now *ptr is the proper momentum*/
          *(topologies[cn_top].momenta[0][i]=get_mem(2,sizeof(int)))=1;
          topologies[cn_top].momenta[0][i][1]=*ptr;
       }/*for(i=1; i<=k; i++)*/
       return 1;
    }/*if(g_nloop == 0)*/

   /* Build external momenta, all particles are ingoing!:*/
   emom=get_mem(ext_lines+1,sizeof(int *));
   for(i=1; i<=ext_lines; i++){
      if( (topologies[cn_top].ext_momenta!=NULL)&&
          (topologies[cn_top].ext_momenta[0]!=NULL) ){
         tmp=int_inc(NULL,
                     topologies[cn_top].ext_momenta[0][i],
                     ext_particles[i-1].is_ingoing );
      }else{
         tmp=int_inc(NULL,
                     ext_particles[i-1].momentum,
                     ext_particles[i-1].is_ingoing );
      }/*if( topologies[cn_top].ext_momenta[i]!=NULL )... else ...*/
      emom[i]=tmp;
   }/*for(i=1; i<=ext_lines; i++)*/

   /*And distribute momenta:*/
   {/*block*/
      int nil=(topologies[cn_top].topology)->i_n;
      char *ldirs=get_mem( nil+2,sizeof(char));
      *ldirs=-1;
      for(i=1;i<=nil; i++)/*Assume i is usr*/
         ldirs[topologies[cn_top].l_subst[i]]=topologies[cn_top].l_dir[i];
      /*Now ldirs[red] is direction red<->usr in terms of +-1*/
      k=distribute_momenta_groups(/*topology.c*/
            (topologies[cn_top].topology)->e_line,
            (topologies[cn_top].topology)->i_line,
            (topologies[cn_top].topology)->e_n,
            (topologies[cn_top].topology)->i_n,
            g_loopmomenta,
            g_nloop,
            g_zeromomentum,
            emom,
            topologies[cn_top].momenta[0],
            ldirs,
            mloopMarks
      );
      free(ldirs);
   }/*block*/
   /*Clean auxiliary external momenta:*/
   for(i=1; i<=ext_lines; i++)
      free(emom[i]);
   free(emom);
   /*Now current topology has one momenta set!*/
   return k;
}/*automatic_momenta_group*/

/*Must allocate and returns textual representation of empty momenta,
  i.e. nil empty strings:*/
char **empty_momenta(void)
{
int nil=(topologies[cn_topol].topology)->i_n;
char ** newmomenta=get_mem( nil ,sizeof(char*) );
int i;

#ifdef SKIP
   for(i=0; i<nil;i++){

      if(top_vec_id!=0){/*Just add sucessive id*/
            newmomenta[i]=new_str(vec_id[j%top_vec_id]);
            j++;
      }else{
            sprintf(buf,"p%d",i+1);
            newmomenta[i]=new_str(buf);
      }/*if(top_vec_id!=0) ... else ... */
   }/*for(i=0; i<nil;i++)*/
#endif
   for(i=0; i<nil;i++)
      newmomenta[i]=new_str("");
   return newmomenta;
}/*empty_momenta*/

#ifdef SKIP
      /*There is a problem with momenta. I can't just add them using the function
        l_addInternalMomenta (see below) since I may need special remarks related to
        some definite directions/numbering both lines and vertices. That is why I
        destroy the shape-containing entry and set momenta entry, instead. So,
        this stuff is obsolete:
       */

/*Adds momenta topology "from" to "to".
  This routine assumes that both of these topologies are internal and has THE SAME
  reduced form! It deletes old "to" momenta, and allocates new ones.*/
static int l_addInternalMomenta(tt_singletopol_type *from, tt_singletopol_type *to)
{
int i,j,k,l;
char *buf;

   l=from->nil;

   /*Free old momenta:*/
   if(to->momenta!=NULL){
      for(i=0;i<to->nmomenta; i++) if(to->momenta[i] !=NULL ){
          for(j=0; j<l;j++) if( to->momenta[i][j]!=NULL )
             free(to->momenta[i][j]);
          free(to->momenta[i]);
      }/*for(i=0;i<to->nmomenta; i++) if(to->momenta[i] !=NULL )*/
      free(to->momenta);
   }/*if(to->momenta!=NULL)*/

   /*Allocate space for new momenta:*/
   to->momenta=get_mem(from->nmomenta,sizeof(char**));

   if(from->momenta!=NULL)for(i=0;i<from->nmomenta; i++){
      if(from->momenta[i]==NULL)continue;
      to->momenta[i]=get_mem(l,sizeof(char*));
      for(j=0,k=1; j<l;j++,k++){
         char **to_mom;/*will be a pointer to the to->momenta line*/
         /*Assume "k" is "red"*/
         to_mom=to->momenta[i] + to->l_red2usr[k]-1;
         if(
            (  buf=
               *to_mom=
               from->momenta[i][
                  from->l_red2usr[k]-1
               ]
            )
               ==NULL
           )continue;/*NULL is just a NULL*/
         buf=*to_mom=new_str(buf);/*swallow to deep*/
         if(
            (from->l_dirUsr2Red[k])*(to->l_dirUsr2Red[k])>0
         )

            continue;/*Directions are coincide*/
         /*directions of the lines "k" (red numbers) are opposite each other*/

         if(swapCharSng(buf)!=0){/*Must be prepended by '-'*/
            *to_mom=malloc(s_len(buf)+2);
            **to_mom='-';
            s_let(buf,(*to_mom)+1);
         }/*if(swapCharSng(buf)!=0)*/
      }/*for(j=0,k=1; j<l;j++,k++)*/
   }/*if(from->momenta!=NULL)for(i=0;i<from->nmomenta; i++)*/
   return 0;
}/*l_addInternalMomenta*/
#endif

/*Adds  coordinates from topology "from" to "to".
  This routine assumes that both of these topologies are internal and has THE SAME
  reduced form! It does not re-allocate the coordinate arrays, if they already
  allocated. If not, it allocates it.*/
static int l_addInternalCoordinates(tt_singletopol_type *from, tt_singletopol_type *to)
{
int i,frI,toI;

   /* If at least one set of internal coordinates is not defined,
      we assume that internal coordinates are not defined at all:*/
   if(
        (from->int_vert ==NULL)||
        (from->int_vlbl ==NULL)||
        (from->int_line ==NULL)||
        (from->int_llbl ==NULL)
   ){
      free_mem(&(to->int_vert));
      free_mem(&(to->int_vlbl));
      free_mem(&(to->int_line));
      free_mem(&(to->int_llbl));
      return 0;/* NULL is just a NULL!*/
   }

   /*Allocate coordinates, if they are not alloocated yet:*/
   if(
       (
         (to->int_vert ==NULL)&&
         (  (to->int_vert=(double *)malloc(sizeof(double)*(to->nv)*2))==NULL  )
       )||
       (
         (to->int_vlbl ==NULL)&&
         (  (to->int_vlbl=(double *)malloc(sizeof(double)*(to->nv)*2))==NULL  )
       )||
       (
         (to->int_line ==NULL)&&
         (  (to->int_line=(double *)malloc(sizeof(double)*(to->nil)*2))==NULL  )
       )||
       (
         (to->int_llbl ==NULL)&&
         (  (to->int_llbl=(double *)malloc(sizeof(double)*(to->nil)*2))==NULL  )
       )
   ){/*Fail*/
      free_mem(&(to->int_vert));
      free_mem(&(to->int_vlbl));
      free_mem(&(to->int_line));
      free_mem(&(to->int_llbl));
      return TT_NOTMEM;
   }

   /*Copy lines/line labels coordinates:*/
   for(i=1; i<=from->nil;i++){
      /*Assume i is red*/
      frI=(from->l_red2usr[i]-1)*2;
      toI=(to->l_red2usr[i]-1)*2;
      (to->int_line)[toI]=(from->int_line)[frI];
      (to->int_line)[toI+1]=(from->int_line)[frI+1];
      (to->int_llbl)[toI]=(from->int_llbl)[frI];
      (to->int_llbl)[toI+1]=(from->int_llbl)[frI+1];
   }/*for(i=1; i<=from->nil;i++)*/

   /*Copy vertices/vertices labels coordinates:*/
   for(i=1; i<=from->nv;i++){
      /*Assume i is red*/
      frI=(from->v_red2usr[i]-1)*2;
      toI=(to->v_red2usr[i]-1)*2;
      (to->int_vert)[toI]=(from->int_vert)[frI];
      (to->int_vert)[toI+1]=(from->int_vert)[frI+1];
      (to->int_vlbl)[toI]=(from->int_vlbl)[frI];
      (to->int_vlbl)[toI+1]=(from->int_vlbl)[frI+1];
   }/*for(i=1; i<=from->nv;i++)*/
   return 0;
}/*l_addInternalCoordinates*/

/*Adds momenta (mOrC==0) or coordinates from topology table(id,ind) to (wrkid,wrkind).
  This routine assumes that both of these topologies are internal and has THE SAME
  reduced form!*/
static void l_addMomentaOrShape(int id,long ind,int wrkid,long wrkind,
                                int mOrC,/*momenta 0 coords 1*/int msId,long msInd)
{
   checkTT(tt_chktop(&id,&ind),FAILCREATINGWRKTABLE);
   checkTT(tt_chktop(&wrkid,&wrkind),FAILCREATINGWRKTABLE);
   if(mOrC==0){/*momenta*/
      /*There is a problem with momenta. I can't just add them using the function
        l_addInternalMomenta (see above) since I may need special remarks related to
        some definite directions/numbering both lines and vertices. That is why I
        destroy the shape-containing entry here and set momenta entry, instead.
        (actually, this job is performed by tt_copySingleTopology, see the
        file tt_load.c). Then I use addInternalCoordinates to add coordinates - I know
        proper table, msId and msInd:*/

      /*Replace shape-containing entry by momenta-containing entry:*/
      if(
               tt_copySingleTopology(
                  (tt_table[id]->topols)[ind],/*from, momenta entry*/
                  (tt_table[wrkid]->topols)[wrkind],/*to, wrk entry*/
                  /*everything including internal coordinates - they may be absent, if
                    msId==msInd==0:*/
                  TT_NEL|TT_NIL|TT_NV|TT_EXT|TT_RED|TT_USR|TT_NMOMENTA|
                  TT_MOMENTA|TT_EXT_MOMENTA|TT_EXT_COORD|TT_INT_COORD|
                  TT_S_RED_USR|TT_S_USR_NUSR|TT_D_USR_RED|TT_D_USR_NUSR|TT_REM|TT_NAME
               )==NULL
      )
         checkTT(TT_NOTMEM,FAILCREATINGWRKTABLE);

      if( (msId!=0)&&(msInd!=0) ){
         /*Check and REDUCE indices:*/
         checkTT(tt_chktop(&msId,&msInd),FAILCREATINGWRKTABLE);
         /*add coordinates which were formerly*/
         checkTT( l_addInternalCoordinates((tt_table[msId]->topols)[msInd],
                                  (tt_table[wrkid]->topols)[wrkind]),
                                  FAILCREATINGWRKTABLE);
      }/*if( (msId!=0)&&(msInd!=0) )*/
      /*
         checkTT( l_addInternalMomenta((tt_table[id]->topols)[ind],
                                  (tt_table[wrkid]->topols)[wrkind]),
                                  FAILCREATINGWRKTABLE);
      */
   }else/*shape*/
      checkTT( l_addInternalCoordinates((tt_table[id]->topols)[ind],
                                  (tt_table[wrkid]->topols)[wrkind]),
                                  FAILCREATINGWRKTABLE);
}/*l_addMomentaOrShape*/

/*Note, this function tries to produce canonical topology independently on the current
  topology status. It looks for the topology into full tables, and, if fail, it makes
  canonical topology. The functin is invoked from pilot.c:main, if origtopology == NULL
  with mOrC=0 and then if coordinates_ok==0 with mOrC=1 :*/
void process_topol_tables(int mOrC/*momenta 0 coords 1*/)
{
tt_singletopol_type *t=NULL;
int id,thetype;
long ind,wrkind;
aTOPOL *ctop=NULL;
   switch(thetype=l_lookup_tbl(&t,&id,&ind,&wrkind,mOrC)){
      case 0:/*not found*/
         if(mOrC == 0){
            /*Distribute momenta for current topology*/
            if(automatic_momenta_group(NULL,cn_topol)==1){
               /*Now momenta are distributed automatically:*/
               set_bit(&(topologies[cn_topol].label),2);
               /*Bit 0: 0, if topology not occures, or 1;
                 bit 1: 1, if topology was produced form generic, or 0.
                 bit 2: 1, automatic momenta is implemented, or 0
               */
            }/*if(automatic_momenta_group(NULL,cn_topol)==1)*/
            /*Need not topology created by l_lookup_tbl anymore:*/
            tt_clearSingleTopology(t);
            return;/*That's all, no links!*/
         }/*if(mOrC == 0)*/

         /*Here ONLY shape, mOrC==1!*/
         if(wrkind==0){/*No such an entry, create it:*/
            /* Here the wrk enty is created as a SHAPE source! So it is assumed that
               it will be edited by "tedi".*/

            /*Extract the canonical (internal) topology (it was created byl_lookup_tbl):*/
            ctop=l_canonical2top();
            /*Now ctop is the canonical topology. Convert it into the table
              representation:*/
            checkTT(
               tt_topol2singletop(
                  ctop,t,
                  /*TT_NEL|TT_NIL|TT_NV|TT_EXT|TT_RED -- was performed by l_lookup_tbl*/
                  TT_USR
               ),
               FAILCREATINGWRKTABLE
            );

            /*Distribute momenta (empty) and substitutions in t:*/
            /*Here we assume than TT_INT is char!!:*/
            t->v_red2usr=alloc123Or111etc(t->nv);
            t->l_red2usr=alloc123Or111etc(t->nil);
            t->v_usr2red=alloc123Or111etc(t->nv);
            t->l_usr2red=alloc123Or111etc(t->nil);
            t->v_usr2nusr=alloc123Or111etc(t->nv);
            t->v_nusr2usr=alloc123Or111etc(t->nv);
            t->l_usr2nusr=alloc123Or111etc(t->nil);
            t->l_nusr2usr=alloc123Or111etc(t->nil);
            /*Directions here:*/
            t->l_dirUsr2Red=alloc123Or111etc(-t->nil);
            t->l_dirUsr2Nusr=alloc123Or111etc(-t->nil);
            t->nmomenta=1;

            *(t->momenta=get_mem(1,sizeof(char**)))=empty_momenta();

            /*Create a new wrk entry:*/
            /*Swallow copy (last 0 in the tt_addToTable agrs):*/
            checkTT( ind=tt_addToTable(g_tt_wrk,t, 0),FAILCREATINGWRKTABLE);
            /*topology t created by l_lookup_tbl is swallowed!*/

            /*Set NoMom:*/
            setTopolRem("NoMom", new_str(""),NULL,
                   &((tt_table[g_tt_wrk-1]->topols)[ind-1]->top_remarks),
                   &((tt_table[g_tt_wrk-1]->topols)[ind-1]->remarks),NULL);

            /*Free auxiliary buffer:*/
            free(ctop);
         }else{/*Such an antry already exists in wrk table!*/
            /*Assume the shape is defined in wrk. Clear NoShape:*/
            deleteTopolRem(NULL,
                (tt_table[g_tt_wrk-1]->topols)[wrkind-1]->top_remarks,
                (tt_table[g_tt_wrk-1]->topols)[wrkind-1]->remarks,
                "NoShape"
            );
            ind=wrkind;/*Reset ind to found wrk entry*/
            /*Need not topology created by l_lookup_tbl anymore:*/
            tt_clearSingleTopology(t);
         }/*if(wrkind==0) ... else*/
         id=g_tt_wrk;
         thetype=2;/*Now the shape is defined by wrk topology...*/
         break;
      case 3:/*Found in int*/
         if(wrkind == 0){/*Absent in WRK! Create new enter, set NoMom/NoShape
                           and store the information about source INT table*/
            char *theType,*tblN,*indN;
            if(mOrC == 0){/*Momenta required*/
               theType=new_str("NoShape");
               tblN=new_str("WMtbl");
               indN=new_str("WMind");
            }else{/*shape required*/
               theType=new_str("NoMom");
               tblN=new_str("WCtbl");
               indN=new_str("WCind");
            }/*if(mOrC == 0)...else*/
            /*Add it into wrk:*/
            checkTT( wrkind=tt_addToTable(g_tt_wrk,
                                (tt_table[id-1]->topols)[ind-1],
                                1),/*deep copy*/
                  FAILCREATINGWRKTABLE
            );
            /*Set NoMom/NoShape:*/
            setTopolRem(theType, new_str(""),NULL,
                   &((tt_table[g_tt_wrk-1]->topols)[wrkind-1]->top_remarks),
                   &((tt_table[g_tt_wrk-1]->topols)[wrkind-1]->remarks),NULL
            );
            /*Store id of int table momenta come from:*/
            setTopolRem(tblN, new_long2str(id),NULL,
                   &((tt_table[g_tt_wrk-1]->topols)[wrkind-1]->top_remarks),
                   &((tt_table[g_tt_wrk-1]->topols)[wrkind-1]->remarks),NULL
            );
            /*Store ind of int table momenta come from:*/
            setTopolRem(indN, new_long2str(ind),NULL,
                   &((tt_table[g_tt_wrk-1]->topols)[wrkind-1]->top_remarks),
                   &((tt_table[g_tt_wrk-1]->topols)[wrkind-1]->remarks),NULL
            );
         }else{/*Already present in WRK! Add complementary info and clear remarks*/
            char *theType,*tblN,*indN,*msIdStr,*msIndStr;
            int msId=0;
            long msInd=0;
            if(mOrC == 0){/*Momenta required*/
               theType=new_str("NoMom");
               tblN=new_str("WCtbl");
               indN=new_str("WCind");
            }else{/*shape required*/
               theType=new_str("NoShape");
               tblN=new_str("WMtbl");
               indN=new_str("WMind");
            }/*if(mOrC == 0)...else*/

            /*Is this topology come from the same source?:*/
            if(

                 (
                    (msIdStr=getTopolRem(tblN,
                         (tt_table[g_tt_wrk-1]->topols)[wrkind-1]->top_remarks,
                         (tt_table[g_tt_wrk-1]->topols)[wrkind-1]->remarks)
                    )==NULL
                 )||
                 (
                    (msIndStr=getTopolRem(indN,
                         (tt_table[g_tt_wrk-1]->topols)[wrkind-1]->top_remarks,
                         (tt_table[g_tt_wrk-1]->topols)[wrkind-1]->remarks)
                    )==NULL
                 )||
                 (
                     (msId=atoi(msIdStr))==0
                 )||
                 (
                     (msInd=atoi(msIndStr))==0
                 )||
                 (msId!=id)||
                 (msInd!=ind)
            ){/*Need to add the shape/momenta*/
                l_addMomentaOrShape(id,ind,g_tt_wrk,wrkind,mOrC,msId,msInd);
            }/* if(... msId,msInd...)*/
            /*else -- the same topology, nothing to do*/
            deleteTopolRem(NULL,
                  (tt_table[g_tt_wrk-1]->topols)[wrkind-1]->top_remarks,
                  (tt_table[g_tt_wrk-1]->topols)[wrkind-1]->remarks,
                  tblN
            );
            deleteTopolRem(NULL,
                  (tt_table[g_tt_wrk-1]->topols)[wrkind-1]->top_remarks,
                  (tt_table[g_tt_wrk-1]->topols)[wrkind-1]->remarks,
                  indN
            );

            /*Remove NoMom/NoShape:*/
#ifdef DEBUG
            if(
#endif
            deleteTopolRem(NULL,
               (tt_table[g_tt_wrk-1]->topols)[wrkind-1]->top_remarks,
               (tt_table[g_tt_wrk-1]->topols)[wrkind-1]->remarks,
               theType
            )
#ifdef DEBUG
            !=0) halt("No remark %s",theType)
#endif
            ;
         }/*if(wrkind == 0)...else*/
         /*Reset id to wrk table, for create_links:*/
         thetype=2;
         id=g_tt_wrk;
         ind=wrkind;
         /*"t" must be cleared, there was a deep copy!*/
         /*No break!*/
      case 1:/*Found in full*/
      case 2:/*Found in wrk*/
         tt_clearSingleTopology(t);
         break;
      case 4:/*Pure vertex not found in full*/
         return;/*Skip the topology*/
   }/*switch(l_lookup_tbl(&t,&id,&ind,mOrC))*/
   l_create_links(id,ind,thetype,mOrC);
   /*That's all, folks!*/
}/*process_topol_tables*/

char *get_undefinedtopologyid(char *firstid,
                              char *mkuniqueid,
                              HASH_TABLE *thetable,
                              word thesize,
                              char *str,
                              word top_topol)
{

static word i=0;

   if (*thetable == NULL)
      *thetable=create_hash_table(thesize,str_hash,str_cmp,int_destructor);

   sprintf(str,firstid,top_topol+1);
   while(lookup(str,*thetable)!=NULL)
      sprintf(str,mkuniqueid,top_topol+1,i++);

   return (str);
}/*get_undefinedtopologyid*/

int compare_topology(void)
{
   word *tmp;
   char str[MAX_STR_LEN];
   int i,j,k,ml,r;
      reduce_topology(0,old_topol,new_topol,l_subst, v_subst,l_dir);
      /* Find among user defined topologies:*/
      if(
         (usertopologies_table!=NULL)&&
         ((tmp=lookup(new_topol,usertopologies_table))!=NULL)
          ){/*Found!*/
            cn_topol=*tmp;

            if( !is_bit_set(&(topologies[cn_topol].label),0) )/*First occurence*/
               g_topologyLabel=&(topologies[cn_topol].label);

            max_momentaset=topologies[cn_topol].n_momenta-1;
            max_topologyid=topologies[cn_topol].n_id-1;
      }else{/*Not found -- topology is undefined.*/
         set_bit(&mode,bitTERROR);
         max_topologyid=0;
         max_momentaset=-1;
         if(nousertopologies_table==NULL)
            nousertopologies_table=create_hash_table(topology_hash_size,
                               hash_topology,topology_cmp,int_destructor);
         if((tmp=lookup(new_topol,nousertopologies_table))!=NULL){
            cn_topol=*tmp;/*Such topology already occurs as undefined.*/
         }else{/* Create new nouser topology*/
            if (!(top_topol<max_top_topol))
              if(
                  (topologies=realloc(topologies,
                   (max_top_topol+=topology_hash_size)*sizeof(aTOPOL)))==NULL
                      )halt(NOTMEMORY,NULL);
            /*Coordinates, relating to user topology:*/
            topologies[top_topol].ev=NULL;  /* external vertices */
            topologies[top_topol].evl=NULL; /* external vertices labels*/
            topologies[top_topol].iv=NULL;  /* internal vertices*/
            topologies[top_topol].ivl=NULL; /* internal vertices labels*/
            topologies[top_topol].el=NULL;  /* external lines */
            topologies[top_topol].ell=NULL; /* external lines labels */
            topologies[top_topol].il=NULL;  /* internal lines */
            topologies[top_topol].ill=NULL; /* internal lines labels */
            topologies[top_topol].rad=NULL;
            topologies[top_topol].ox=NULL;
            topologies[top_topol].oy=NULL;
            topologies[top_topol].start_angle=NULL;
            topologies[top_topol].end_angle=NULL;
            topologies[top_topol].linemarks=NULL;
            topologies[top_topol].vertexmarks=NULL;
            topologies[top_topol].remarks=NULL;
            topologies[top_topol].top_remarks=0;

            topologies[top_topol].topology=get_mem(1, sizeof(tTOPOL));
            top_let(new_topol,topologies[top_topol].topology);
            topologies[top_topol].orig=NULL;
            topologies[top_topol].momenta=NULL;
            topologies[top_topol].ext_momenta=NULL;
            if( (topologies[top_topol].topology)->i_n == 0)/*pure vertex*/
               /*Momenta ARE completely defined by external ones*/
               topologies[top_topol].n_momenta=1;
            else
               topologies[top_topol].n_momenta=0;
            topologies[top_topol].max_vertex=vcount;
            topologies[top_topol].label=0;
            g_topologyLabel=&(topologies[top_topol].label);

            topologies[top_topol].num=top_topol;

            get_undefinedtopologyid(UNDEFINEDTOPOLOGYID,
                                    EXTRA_UNDEFINEDTOPOLOGYID,
                                    &topology_id_table,
                                    topology_hash_size,
                                    str,top_topol);
            topologies[top_topol].id=get_mem(1,sizeof(char*));
            topologies[top_topol].id[0]=new_str(str);
            install(topologies[top_topol].id[0],
                       newword(top_topol),
                       topology_id_table
            );
            topologies[top_topol].n_id=1;
            *(tmp=get_mem(1,sizeof(word)))=top_topol;
            topologies[top_topol].l_subst=s123etc(new_str(l_subst));
            topologies[top_topol].v_subst=s123etc(new_str(v_subst));
            topologies[top_topol].l_dir=s111etc(new_str(l_dir));

            /* No coordinates:*/
            topologies[top_topol].coordinates_ok=0;
            /* Todo:
             * allocate_topology_coordinates(topologies +top_topol);
             * Coordinates distribute
             */
            install(topologies[top_topol].topology,tmp,nousertopologies_table);
            cn_topol=*tmp;
            top_topol++;
         }
      }
      if ((is_bit_set(&mode,bitTERROR))&&(!is_bit_set(&mode,bitBROWS)))
         out_undefined_topology();
      {/*begin block*/
        struct template_struct *tmp=template_t;
        r=0;
        do{
           if(cmp_template(topologies[cn_topol].id[0],tmp->pattern)){
             r=1;break;
           }
        }while((tmp=tmp->next)!=NULL);
        r=!r;
      }/*end block*/
      if(r==0){
         if(
             (  !is_bit_set(&mode,bitTERROR)  )&&
             (  is_bit_set(&mode,bitFORCEDTRUNSLATION) ||
                (!is_bit_set(&mode,bitBROWS))
             )
           ){
            max_vec_length=0;
            for(i=0; !(i>max_momentaset); i++){
              for(j=1; !(j>int_lines); j++){
                 ml=0;
                 for(k=1; !(k>*(topologies[cn_topol].momenta[i][j])); k++){
                   ml+=s_len(vec_group[
                        abs(topologies[cn_topol].momenta[i][j][k])].text)+1;
                 }
                 if(max_vec_length<ml)
                    max_vec_length=ml;
              }
              if (topologies[cn_topol].ext_momenta[i] == NULL)
                 continue;
              for(j=1; !(j>ext_lines); j++){
                 ml=0;
                 for(k=1; !(k>*(topologies[cn_topol].ext_momenta[i][j])); k++){
                   ml+=s_len(vec_group[
                        abs(topologies[cn_topol].ext_momenta[i][j][k])].text)+1;
                 }
                 if(max_vec_length<ml)
                    max_vec_length=ml;
              }
            }
         }
      }
      return(r);
}/*compare_topology*/

void create_diagram_table(void)
{
  char i,j,k;
  char from[MAX_I_LINE+2],to[MAX_I_LINE+2], order[MAX_I_LINE+2];
  char mem[MAX_LINES_IN_VERTEX],
       type[MAX_LINES_IN_VERTEX],top;
  word number[MAX_LINES_IN_VERTEX];
     /*Create wrktable according to new topology:*/
     for(i=1;!(i>vcount);i++)
       for(j=1;!(j>int_lines);j++)
          /*wrktable[i][j]=dtable[v_subst[i]][l_subst[j]];*/
          wrktable[v_subst[i]][l_subst[j]]=dtable[i][j];
     /*Do not forget reset l_subst and l_dir: */
     for(j=1;!(j>int_lines);j++){
        l_subst[topologies[cn_topol].l_subst[j]]=
        lt_subst[topologies[cn_topol].l_subst[j]]=j;
        l_dir[j]=1;
     }
     invertsubstitution(il_subst,l_subst);
     /*Do not forget reset v_subst:*/
     for(j=1;!(j>vcount);j++)
        v_subst[topologies[cn_topol].v_subst[j]]=
        vt_subst[topologies[cn_topol].v_subst[j]]=j;
     invertsubstitution(iv_subst,v_subst);
     /* Set new linemarks, if present:*/
     if(topologies[cn_topol].linemarks != NULL){
        k=*(topologies[cn_topol].linemarks[0]);
        linemarks=get_mem(k+1,sizeof(char*));
        for(j=0;!(j>int_lines);j++)
           linemarks[j]=new_str(topologies[cn_topol].linemarks[j]);
     }/*if(topologies[cn_topol].linemarks != NULL)*/
     /* Set new vertexmarks, if present:*/
     if(topologies[cn_topol].vertexmarks != NULL){
        k=*(topologies[cn_topol].vertexmarks[0]);
        vertexmarks=get_mem(k+1,sizeof(char*));
        for(j=0;!(j>vcount);j++)
           vertexmarks[j]=new_str(topologies[cn_topol].vertexmarks[j]);
     }/*if(topologies[cn_topol].vertexmarks != NULL)*/

     /* Final step-- copying wrk to dtable with possible line ordering:*/
     for(j=1;!(j>int_lines);j++){
        from[j]=to[j]=0;
        order[j]=j;
     }
     for(j=1;!(j>int_lines);j++)
       for(i=1;!(i>vcount);i++)
        if(wrktable[i][j]){
           if(from[j]==0) from[j]=i; else to[j]=i;
        }
     for(i=1; i<int_lines; i++){
        mem[top=0]=i;
        for(j=i+1;!(j>int_lines);j++){
           if(from[j]==0)continue;
           if(   (from[j]==from[i])&&(to[j]==to[i])   ){
              if(!(++top<MAX_LINES_IN_VERTEX))halt(FOURLINE,NULL);
              mem[top]=j;
              from[j]=0;
           }
        }
        if(top){/*Coinciding  lines*/
           /* We whant to sort the lines so that the "less" line
            * will be upper. "less" means that: or "type" of line is less,
            * or, if types are coinciding, the number in the array
            * (order of declaration of the particle in the model)
            * is less.
            */
            /*130203: optionally, switch off sorting by type(according to g_sortByType)*/
           if(g_sortByType)for(k=0;!(k>top);k++){
             type[k]=get_type(mem[k],wrktable);
             number[k]=get_number(mem[k],wrktable);
           }else for(k=0;!(k>top);k++){
             type[k]=0;
             number[k]=get_number(mem[k],wrktable);
           }/*if(g_sortByType)for(k=0;!(k>top);k++) ... else ...*/
           /*Bubble sorting:*/
           for(k=0;k<top;k++)
              for(j=k+1;!(j>top);j++){
                if(  ( (type[j]<type[k]) )||
                     (    ( (type[j] == type[k]) )&&
                          (number[j]<number[k])
                     )
                ){/*"j" is less then "k".
                   * Exchange "j" and "k" elements:
                   */
                   swapch(order, mem[j],mem[k]);
                   swapch(mem,j,k);
                   swapw(number,j,k);
                   swapch(type,j,k);
                }
           }/*for(j=k+1;!(j>top);j++)*/
        }/*if(top)*/
     }/*for(i=1; i<int_lines; i++)*/
     for(i=1;!(i>vcount);i++)
       for(j=1;!(j>int_lines);j++){
          dtable[i][order[j]]=wrktable[i][j];
          wrktable[i][j]=0;
       }
}/*create_diagram_table*/

word set_prototype(char *prot)
{
 word *wptr;
   if(prototype_table==NULL)
      prototype_table=create_hash_table(prototype_hash_size,
                                        str_hash,str_cmp,int_destructor);
   if((wptr=lookup(prot,prototype_table))==NULL){
      if (!(top_prototype<max_top_prototype))
        if(
            (prototypes=realloc(prototypes,
               (max_top_prototype+=prototype_hash_size)*sizeof(char *)))==NULL
                )halt(NOTMEMORY,NULL);
      prototypes[top_prototype]=new_str(prot);
      *(wptr=get_mem(1,sizeof(word)))=top_prototype;
      install(prototypes[top_prototype],wptr,prototype_table);
      top_prototype++;
   }
   return(*wptr);
}/*set_prototype*/

void define_prototype(void)
{

  char prot[MAX_I_LINE+1],*ptr;
  char i;
     ptr=prot;
     for(i=1;!(i>int_lines);i++)
        *ptr++=get_type(topologies[cn_topol].l_subst[i],dtable);
     *ptr=0;
   cn_prototype=set_prototype(prot);
}/*define_prototype*/

void fill_up_diagram_array(word count)
{
  diagram[count].n_id=cn_topologyid;
  diagram[count].prototype=cn_prototype;
  diagram[count].topology=cn_topol;
  diagram[count].number=diagram_count;
  diagram[count].sort=count;
}/*fill_up_diagram_array*/

void output_common_protocol(word count)
{
  if(log_file==NULL)return;
  fprintf(log_file,COMMONPROTOKOLOUT,count,diagram_count,
          prototypes[cn_prototype],topologies[cn_topol].id[cn_topologyid]);
}/*output_common_protocol*/

void define_momenta(void)
{
  char i;
     /*Instead of using deep copy we will use just a pointers*/

     /*
     if(momenta!=NULL){
         for(i=1;!(i>momenta_top);i++)
              free_mem(&(momenta[i]));
     }
     */

     if(momenta_top<int_lines){/*Realloc!*/
        free_mem(&momenta);
        momenta=get_mem(int_lines+1,sizeof(int *));
        momenta_top=int_lines;
     }
     for(i=1; !(i>int_lines); i++)
       momenta[i]=topologies[cn_topol].momenta[cn_momentaset][i];

     /* Directions of momenta are checked in maccreateinput(macro.c).*/
}/*define_momenta*/

static char *new_text_str(char *s)
{
  char *ptr=s;
  int nind,nvec;
    if(s==NULL)return(NULL);
    /*Count maximal possible length of the string after all exchanging:*/
    ptr++;
    nind=nvec=0;
    while(*ptr){
        if(*ptr==2)/*Index*/
           nind++;
        if(*ptr==3){/*vector*/
           ptr+=2;/*Skip particle id and number of vectors*/
           while(*ptr++!='\n');/*Skip to the end of vectors*/
           nvec++;
        }
        ptr++;
    }
  return(s_let(s,get_mem(s_len(s)+3+(max_ind_length+1)*nind+max_vec_length*nvec,
          sizeof(char))));
}/*new_text_str*/

void create_indices_table(void)
{
  int i,j,k,l,m,mark;
  word prop,*wptr;
  int *info_ptr;
  char tmp[4];
  int is_tadpole;
  char *ptr,*gindex;
  char str[MAX_STR_LEN];
     *tmp=2;
     tmp[3]=0;

     for(i=1; !(i>int_lines); i++){
        int info[MAX_INDICES_INFO_LENGTH];
        /* Define up particle position:*/
        for(j=1; !(j>vcount); j++)
           if(dtable[j][i])break;
        if(*(id[ dtable[j][i]-1 ].id)==2){
           is_tadpole=1;
           dtable[0][i]=(prop=dtable[j][i]-1)+1;
        }else{
           is_tadpole=0;
           dtable[0][i]=(prop=id[ dtable[j][i]-1 ].link[1])+1;
        }
        /* Copying form id:*/
        ptr=id[prop].form_id;
        if(!(top_text<max_top_text))
           if(
             (text=realloc(text,
                (max_top_text+=DELTA_TEXT)*sizeof(char *)))==NULL
             )halt(NOTMEMORY,NULL);
        text[top_text]=new_text_str(ptr);
        wrktable[0][i]=++top_text;/* Because in wrktable is stored id+1*/
        /*Determine number of indices:*/
        if(id[ prop ].Nind =='\0')/*No indices in FORM id*/
           continue;/*for(i=1; !(i>int_lines); i++)*/
        /* About info see comments in utils.c at alloc_indices*/
        *info=0;/* Initialise the index group info*/
        info_ptr=info+1;/* 1 for the total number of groups*/
        ptr++;/* Skip number of the cycle id*/
        while(*ptr){
            if(*ptr==2){/*Index*/
               if(*(++ptr)=='0'){/*Interesting only the first particle here*/
                  *info_ptr++=*++ptr - '0';/*Push the group number*/
                  /* Number of indices to be allocated:*/
                  *info_ptr++=(is_tadpole)?2:1;
                  (*info)++;/* Increment the total number of groups*/
               }
            }
            if(*ptr==3){/*vector*/
               ptr+=2;/*Skip particle id and number of vectors*/
               while(*ptr++!='\n');/*Skip to the end of vectors*/
            }
            ptr++;
        }/*Now info[0] is equal to the number of indices belong
          to different groups.*/
        if(*info){
           if(!(top_text+1<max_top_text))
              if(
                (text=realloc(text,
                   (max_top_text+=DELTA_TEXT)*sizeof(char *)))==NULL
                )halt(NOTMEMORY,NULL);
           if(is_tadpole){
              char *secondIndex;
              text[top_text]=alloc_indices(info);
              wrktable[j][i]=top_text+1;/* Do not forget +1!*/
              gindex=text[top_text]+1;/* +1 to skip the number of the groups*/
              for(k=0; k<*info;k++){/*cycle for all groups*/
                 ptr=gindex;/* Point to the beginning of the index information*/
                 while(*ptr++);/*Now ptr points to second index*/
                 secondIndex=ptr;/*store it for future*/
                 /*'1' is the second particle number*/
                 tmp[1]=1+'0';/* Not just '1' to be independent on encoding*/
                 tmp[2]=*gindex;/*The current group*/
                 /* Replace first occurence by the index:*/
                 s_replaceall(tmp,ptr,text[wrktable[0][i]-1]);
                 /*Now process second particle:*/
                 /*Set ptr to first index:*/
                 ptr=gindex+2;/* +2==+1+1 -- skip the group number
                                 and the number of allocated indices*/
                 tmp[1]='0';/*'0' is the first particle number*/
                 /* tmp[2] is the same*/
                 /* Replace second occurence by the index:*/
                 s_replaceall(tmp,ptr,text[wrktable[0][i]-1]);
                 /* And go to the next group:*/
                 gindex=secondIndex;/* Now gindex points to the second index*/
                 while(*gindex++);/* Now gindex points to the next group*/
              }/*for(k=0; k<*info;k++)*/
              /*Ok. Now increment top - that's all with the text.*/
              top_text++;
           }else{/*if(is_tadpole)*/
              char n;
              /* Determine the particle number:*/
              if((n=id[ dtable[j][i]-1 ].kind)>0)
                 n--;/* due to counting from 0*/
              /* Allocate the index of the first particle:*/
              text[top_text]=alloc_indices(info);
              wrktable[j][i]=top_text+1;
              gindex=text[top_text]+1;/* +1 to skip the number of the groups*/
              for(k=0; k<*info;k++){/*cycle for all groups*/
                 /*Set ptr to the index:*/
                 ptr=gindex+2;/*+2==+1+1 -- skip the group number
                                and the number of allocated indices*/
                 tmp[1]=n+'0';/*'0' is the first particle number*/
                 tmp[2]=*gindex;/*The current group*/
                 /* Replace first occurence by the index:*/
                 s_replaceall(tmp,ptr,text[wrktable[0][i]-1]);
                 while(*gindex++);/* Now gindex points to the next group*/
              }/*for(k=0; k<*info;k++)*/
              /* Process next particle:*/
              top_text++;
              /*Looking for second particle:*/
              for(j++; !(j>vcount); j++)
                 if(dtable[j][i])break;
              text[top_text]=alloc_indices(info);
              wrktable[j][i]=top_text+1;
              gindex=text[top_text]+1;/* +1 to skip the number of the groups*/
              for(k=0; k<*info;k++){/*cycle for all groups*/
                 /*Set ptr to the index:*/
                 ptr=gindex+2;/*+2==+1+1 -- skip the group number
                                  and the number of allocated indices*/
                 tmp[1]=(!n)+'0';/* former was 'n' so now '!n'*/
                 tmp[2]=*gindex;/*The current group*/
                 /* Replace first occurence by the index:*/
                 s_replaceall(tmp,ptr,text[wrktable[0][i]-1]);
                 while(*gindex++);/* Now gindex points to the next group*/
              }/*for(k=0; k<*info;k++)*/
              top_text++;
           }/*if(is_tadpole)...else*/
        }/*if(*info)*/
     }/*for(i=1; !(i>int_lines); i++)*/

     /*All lines are ready.*/
     /*Process vertices:*/
     if(!(max_vertices>vcount)){
        free_mem(&vertices);
        max_vertices=vcount+1;
        vertices=get_mem(max_vertices,
           sizeof(VERTEX));
     }
     for(i=1; !(i>vcount); i++){
        /*Define internal identifier:*/
        *str=0;
        for(j=1; !(j>int_lines);j++)
            if(dtable[i][j])
              m_cat(str,str,id[ dtable[i][j]-1 ].id);
        for(j=-ext_lines; j<0;j++)
            if(dtable[i][j])
              m_cat(str,str,id[ dtable[i][j]-1 ].id);
        /*Looking for it in table:*/
        if( (wptr=lookup(str,main_id_table))==NULL )
           halt(UNDEFINEDID,m2s(str));
        dtable[i][0]=(*wptr)+1;/*Set it into dtable*/
        /*Allocate form_id:*/
        ptr=id[*wptr].form_id;
        if(!(top_text<max_top_text))
           if(
             (text=realloc(text,
                (max_top_text+=DELTA_TEXT)*sizeof(char *)))==NULL
             )halt(NOTMEMORY,NULL);
        text[top_text]=new_text_str(ptr);
        /*And put pointer to it into wrktable:*/
        wrktable[i][0]=++top_text;/* Because in wrktable is stored id+1*/
        /*Now define veritex[i]:*/
        mark=0;/* Use mark as bitmask for mark lines*/
        k=*(id[  dtable[i][0]-1 ].id);/* k is equal to the number
                                          of particles in vertex*/
        for(j=-ext_lines;!(j>int_lines);j++)if((j)&&(dtable[i][j])){
           if(*(id[ dtable[i][j]-1 ].id)==2){/*tadpole*/
              is_tadpole=1;/*+-1, if tadploe,or 0*/
              /*For tadpoles we keep id of a propagator instead of a particle.
                So first choose the begin of a propagator:*/
              prop=id[  dtable[i][j]-1 ].link[0];
              /*now prop is id of the first particle*/
           }else{
            is_tadpole=0;
            prop=dtable[i][j]-1;/*Now prop is id of the particle*/
           }/*if(*(id[ dtable[i][j]-1 ].id)==2)*/
           /*now we have is_tadpole (+-1, if tadploe,or 0) and prop -- id of
             an incoming vertex at line j. */
           do{/*Back enter for possible tadpoles:*/
              /*Maximum we can try is the number of particles in this vertex:*/
              for(l=0;l<k;l++){
                 if(prop == id[ dtable[i][0]-1  ].link[l])/*ok, this is our particle*/
                     if(!is_bit_set(&mark,l)){/*And it was not processed yet*/
                        set_bit(&mark,l);
                        vertices[i][l].n=j;
                        vertices[i][l].id=id[prop].id+1;
                        if(is_tadpole==1){
                          if( id[ prop ].kind==1 )/*begin of propagator -- outgoing. Store
                                                    this for the second end:*/
                              m=0;
                          else/* if 0, does not matter, really! If 2, then this is the end
                                  of propagator -- ingoing. Store this for the second end:*/
                              m=1;
                          vertices[i][l].is_ingoing=m;
                          is_tadpole=-1;/*To perform the second end.*/
                          /*And reset the particle according to the second end:*/
                          prop=id[  dtable[i][j]-1 ].link[1];
                        }else if(is_tadpole==-1){/*Second end of a tadpole*/
                          /*m was defined performing the begin of the tadpole:*/
                          vertices[i][l].is_ingoing=!m;
                          is_tadpole=0;/*Finish with this line, leave a cycle.*/
                        }else{/*is_tadpole==0*/
                            if(j<0){
                               vertices[i][l].is_ingoing=
                                 ext_particles[-j-1].is_ingoing;
                            }else{
                               /*Assume this is the down end:*/
                               vertices[i][l].is_ingoing=1;
                               /*And now check is this the case:*/
                               if(i<vcount) for(m=i+1;!(m>vcount);m++){
                                  if(dtable[m][j]){/*Hmm... My guess was wrong!*/
                                     vertices[i][l].is_ingoing=0;
                                     break;
                                  }/*if(dtable[m][j])*/
                               }/*if(i<vcount) for(m=i+1;!(m>vcount);m++)*/
                            }/*if(j<0)...else*/
                        }/*else /is_tadpole==0/ */
                        break;
                     }/*if(!is_bit_set(&mark,l))*/
              }/*for(l=0;l<k;l++)*/
           }while(is_tadpole);
        }/*for(j=-ext_lines;!(j>int_lines);j++)if((j)&&(dtable[i][j]))*/
        /*vertices[i] is determned.*/
        /*Now distribute the indices:*/
        for(j=1;!(j>ext_lines);j++){/*Check external particles*/
           if(dtable[i][-j]){/*Found external particle*/
              /*Allocate propagator of the external particles-need for utils:*/
              dtable[0][-j]= id[   dtable[i][-j]-1   ].link[1]  +1;
              if(  (ptr=ext_particles[j-1].ind)!=NULL ){/*Index*/
                 for(l=0;l<k;l++)if(vertices[i][l].n==-j){/*This is 'l' particle*/
                    int ii,m;
                    m=*ptr;/* m == number of groups*/
                    gindex=ptr+1;/*gindex points to the current index group*/
                    for(ii=0; ii<m; ii++){/*Cycle for all groups*/
                      ptr=gindex+2;/*ptr points to index*/
                      tmp[1]=l+'0';/* Set proper particle */
                      tmp[2]=*gindex;/* Set proper group */
                      /* Replace first occurence by the index:*/
                      s_replaceall(tmp,ptr,text[wrktable[i][0]-1]);
                      while(*gindex++);/* Go to the next group*/
                    }/*for(ii=0; ii<m; ii++)*/
                    break;
                 }/*for(l=0;l<k;l++)if(vertices[i][l].n==-j)*/
              }/*if(  (ptr=ext_particles[ext2my[j]].ind)!=NULL )*/
           }/*if(dtable[i][-j])*/
        }/*for(j=1;!(j>ext_lines);j++)*/

        if(id[*wptr].Nind!='\0')/*There are indices*/
        for(l=0;l<k;l++)if(wrktable[i][vertices[i][l].n]){/* Index found*/
           int ii,m;
           char cgroup;

           gindex=text[wrktable[i][vertices[i][l].n]-1];
           m=*gindex++;/* m == number of groups;
                          gindex points to the current index group*/
           for(ii=0; ii<m; ii++){/*Cycle for all groups*/
              ptr=gindex+1;/*ptr points to index which is an m-string*/
              cgroup=*gindex;/*cgroup == current group*/
              if(*ptr>1){/* Two indices allocated, tadpole*/
                 if(vertices[i][l].is_ingoing)while(*ptr++);/*Skip first index*/
                 else ptr++;
                 while(*gindex++);/* Skip first index in gindex*/
              }else{
                 ptr++;
              }/*if(*ptr>1)*/
              /*ptr points to the index*/
              tmp[1]=l+'0';
              tmp[2]=cgroup;
              /* Replace first occurence by the index:*/
              s_replaceall(tmp,ptr,text[wrktable[i][0]-1]);
              while(*gindex++);/* Go to the next group*/
           }/*for(ii=0; ii<m; ii++)*/
        }/*for(l=0;l<k;l++)if(wrktable[i][vertices[i][l].n])*/
     }/*for(i=1; !(i>vcount); i++)*/
     /*Now all form_id are allocated with indices, but without momenta.*/
}/*create_indices_table*/

/*This routine is used only from build_form_input. It expects the begin (i.e., the end!
  We process OPPOSITE the fermion number flow!) of a fermion line (line l and vertex v)
  initial Dirac direction fflow, and the return address lastVert for last processed
  vertex. Initial line is assumed to be processed, fermionlines_count is assumed to
  be set properly. It will move along the fermion line until it reach the initial line,
  or an external line. Then it returns it. Note, final external line is not processed!*/
static int processFLine(int l, int v, int fflow, int *lastVert)
{  /*see comment in pilot.c, seek  'MAJORANA fermions'*/
   int el;/*continuation of l*/
   int fflowEl;/*direction of the el*/
   int initLine=l;

   for(;;){

      /*Process the vertex v (we can't define majorana type here!):*/
      output[top_out].fcount=fermionlines_count;
      set_form_id( dtable[v][0]-1, output[top_out].i=wrktable[v][0]-1 );
      output[top_out].t='v';
      output[top_out].pos=v;
      v_outPos[v]=top_out;

      /*Define fermionic continuation:*/
      for(el=-ext_lines; !(el>int_lines) ;el++)
         if(   (el) /*Not 0! Index 0 is used for another information*/
             &&(dtable[v][el])
             &&(id[  dtable[0][el]-1  ].kind == -1)/*fermionic*/
             &&(el!=l)/*Found continuation. Note, tadpoles may NOT appear here!!!*/
         )break;
      /*Now el is a fermionic continuation.*/
      switch(id[ dtable[v][el]-1 ].kind){
         case 0:/*Majorana*/
            fflowEl=0;break;
         case 1:/*Begin of propagator, right Dirac direction*/
            fflowEl=1; break;
         case 2:/*End of propagator, opposite Dirac direction*/
            fflowEl=-1;break;
         /*debug:*/
         default:
            halt("Internal error 1",NULL);
      }/*switch(id[ dtable[v][el]-1 ].kind)*/
      /*Define type of vertex v:*/
      if(  (fflow<0)||(fflowEl<0)  )/*At least one of directions is opposite*/
         output[top_out].majorana=-1;
      else  if(  (fflow==0)&&(fflowEl==0)  )/* ----*---- */
         output[top_out].majorana='\0';
      else/*No wrong lines at the vertex:*/
         output[top_out].majorana='\1';
      addtoidentifiers(output[top_out++].i,'f','v');
      /*Vertex is ready!*/

      /*Is the line ended?*/
      if(
          (initLine==el)/*Closed line*/
          ||(el<0)/*External line is reached*/
        )
        break;/*Break for(;;)*/

      /*Now process the line el*/
      output[top_out].fcount=fermionlines_count;
      set_form_id( dtable[0][el]-1, output[top_out].i=wrktable[0][el]-1 );
      output[top_out].t='l';
      output[top_out].pos=el;
      l_outPos[el]=top_out;
      output[top_out].majorana=fflowEl;
      output[top_out].fflow=1;/*Default +1 must be set to all particles except majorana*/
      addtoidentifiers(output[top_out].i,'f','l');
      /*Both vertex and line are ready.*/

      /*Now shift line and vertex:*/
      {/*block*/
       int tmp=v;/*Store old value*/
         for(v=1; !(v>vcount) ;v++)if( (dtable[v][el])&&(tmp!=v) )break;
         /*Note, tadpoles may NOT appear here!!!*/
         /*The line we are processing is directed from tmp to v. We are moving opposite
         fflow, so fflow is directed from v to tmp.*/
         if (fflowEl==0){/*Majorana, we must detemine the sign*/
            if(tmp<v)/*Line is directed up, negative in 'red'*/
               output[top_out].fflow=-1;
         }/*if (fflowEl==0)*/
      }/*block*/

      top_out++;/*Increment output counter*/

      l=el;
      fflow=fflowEl;
      /*Now we may continue.*/
   }/*for(;;)*/
   *lastVert=v;/*Push back the index of the last processed vertex*/
   return el;
}/*processFLine*/

/*The idea of this routine: we create the array 'output' containig FORM expression
  for vertices/lines in correct order. External lines are NOT contained by this array.*/
void build_form_input(void)
{
 int notProcessedFerms=0,i,j,l,v;

struct  ext_part_struct *extP;
 fermionlines_count=0;

  for(i=1; i<=ext_lines;i++){
      extP=ext_particles+i-1;
      extP->fcount=0;
      /*1 -  fflow coincides with red; -1 - opposit. Assume first, it concides:*/
      extP->fflow=1;
      extP->majorana=-4;/*'0',-1,+1  depending on co-incide fermion number flow
        with the fermion flow (+1), opposit (-1), or undefined (0). Also we use it
       for marking external legs: unprocessed legs have the value -4.
       At the first pass we process only outgoing fermionic lines. Other legs we mark
       as 'majorana' (extP->majorana=-2) or 'begin of propagator' (extP->majorana=-3)
       and store information about coincident vertex in extP->fcount. At the second
       pass we use this information to simplify parsing. */
  }/*for(i=1; i<=ext_lines;i++)*/

  /* l_outPos, v_outPos :will contain the positions in the array
   * "output". Initially initialised by -1 to use them for marking.
   * Zero element nothing but the length: */
  *(l_outPos=get_mem((int_lines+2),sizeof(int)))=int_lines;
  *(v_outPos=get_mem(vcount+2,sizeof(int)))=vcount;
  memset(l_outPos+1,-1,int_lines*sizeof(int));
  memset(v_outPos+1,-1,vcount*sizeof(int));

  /*g_left_spinors[i] will contain index of external line (negative!) which is i left
    multiplier, g_right_spinors[i] will contain index of external line (negative!)
    which is i right multiplier. I starts from 1!!
    NOTE we assume get_mem initializes alocated memory by 0: */
  g_left_spinors=get_mem((ext_lines+2),sizeof(int));
  g_right_spinors=get_mem((ext_lines+2),sizeof(int));
  /*Note here, left corresponds the end of fflow, i.e., to the beginning of analyticla
   expression for the fermion chains.*/

  /*Reallocate an output array, if needed:*/
  if(max_top_out<int_lines*vcount+1){
     max_top_out=int_lines*vcount+1;
     free_mem(&output);
     output=get_mem(max_top_out,sizeof(struct output_struct));
  }
  top_out=0;

  /* Looking for external fermion lines. If majorana particles present,
   * there may be 2 passes. First pass, we are looking for outgoing
   * fermionic lines:*/
  for(i=-ext_lines;i<0;i++){
     extP=ext_particles-i-1;
     if (id[  dtable[0][i]-1  ].kind==-1){/*Fermion!*/

     if( extP->majorana == -4 ){/* Not marked*/
        notProcessedFerms++;/*Account it!*/
        /* So, now we can try to claw hold of a fermion line.*/

        /*Find the corresponding vertex, j:*/
        for(j=1;!(j>vcount); j++) if(dtable[j][i])break;
        /*Now id[  dtable[j][i]-1  ] is an id of a particle, see types.c internal_id_cell*/
        switch(id[  dtable[j][i]-1  ].kind){
           case 0:/*Majorana: fermion, and particle coincides with antiparticle.*/
              extP->majorana=-2;/*Postpone up to the second pass*/
              extP->fcount=j;/*Store the vertex number*/
              break;
           case 1:/*Begin of propagator*/
              extP->majorana=-3;/*May be will processed further or at the nest pass*/
              extP->fcount=j;/*Store the vertex number*/
              break;
           case 2:/*end of propagator -- claw!*/
              extP->fcount=++fermionlines_count;
              extP->majorana='\1';/*Extern particle is Dirac one with valid direction*/
              g_left_spinors[++*g_left_spinors]=i;/*account it as a left spinor*/
              /*Corresponding particle will be processed by the processFLine*/
              l=processFLine(i,j,1,&j);
              /*i-line,j-vert,+1-fflow direction,&j return last vertex*/
              /*Now l is equal to the second end of fermion line incidents to vertex j*/
              /*debug:*/
                 if(l>=0)halt("internal error 2",NULL);
              g_right_spinors[++*g_right_spinors]=l;/*account it as a right spinor*/
              extP=ext_particles-l-1;
              extP->fcount=fermionlines_count;
              /*The second end may be of 3 kinds: Majorana Dirak in or Dirac out:*/
              switch(id[  dtable[j][l]-1  ].kind){
                 case 0:/*Majorana*/
                    extP->majorana='\0';/*Extern particle is Majorana*/
                    /*Fermion number flow is INcoming into this leg: we have passed
                      through the diagram OPPOSIT the Fermion number flow !*/
                    if( extP->is_ingoing == '\0' )/*OUTgoing*/
                      extP->fflow=-1;/*fflow is opposit with red*/
                    break;
                 case 1:/*Begin of propagator*/
                    /*Extern particle is Dirac one with valid direction*/
                    extP->majorana='\1';
                    break;
                 case 2:
                    /*Extern particle is Dirac one with opposit direction*/
                    extP->majorana=-1;
                    break;
                 /*debug:*/
                 default:
                    halt("internal error 3",NULL);
              }/*switch(id[  dtable[j][i]-1  ].kind)*/
              notProcessedFerms--;/*Performed, do not take it into account*/
              break;
           /*debug:*/
           default:
               halt("internal error 4",NULL);
        }/*switch(id[  dtable[j][i]-1  ].kind==2*/
     }/*if( extP->majorana == -4 )*/
     }else/*if(id[  dtable[0][i]-1  ].kind==-1)*/
       extP->majorana='\0';
  }/*for(i=-ext_lines;i<0;i++)*/
  /*End of the first pass.*/

  if(notProcessedFerms)/*Majorana fields may violate fermion number flow!*/
     for(i=-ext_lines;i<0;i++){/* Second pass external fermion lines*/
        extP=ext_particles-i-1;
        switch((int)extP->majorana){
           case -2:/*Majorana*/
             /* We will claw hold of a line, so the fermion number flow is OUTgoing.*/
             if( extP->is_ingoing )/*INgoing*/
                extP->fflow=-1;/*fflow is opposit with red*/
           case -3:/*'wrong' direction, i.e., begin of propagator */
              v=-( (-extP->majorana)-2 );/* 0 -- Majorana, -1--begin of propagator*/
              j=extP->fcount;/*The corresponding vertex was stored in fcount*/
              extP->fcount=++fermionlines_count;/*claw hold of a line*/
              extP->majorana=v;/*  0 -- extern particle is Majorana;
                                 * -1 -- Dirac one with opposit direction*/
              g_left_spinors[++*g_left_spinors]=i;/*account it as a left spinor*/
              /*Corresponding particle will be processed by the processFLine*/
              l=processFLine(i,j,v,&j);
              /*i-line,j-vert,v= 0||-1 -- fflow direction, &j return last vertex*/
              /*Now l is equal to the second end of fermion line incidents to vertex j*/
              /*debug:*/
                 if(l>=0)halt("internal error 5",NULL);
              g_right_spinors[++*g_right_spinors]=l;/*account it as a right spinor*/

              extP=ext_particles-l-1;
              extP->fcount=fermionlines_count;

              switch(id[  dtable[j][l]-1  ].kind){
                 case 0:/*Majorana*/
                    extP->majorana='\0';/*Extern particle is Majorana*/
                    /*End of the fermion chain, so the fermion number flow is
                      INgoing. Check:*/
                    if( ! extP->is_ingoing )/*OUT going*/
                       extP->fflow=-1;/*fflow is opposit with red*/
                    break;
                 default:
                    /*Extern particle is Dirac one with valid direction.
                     * Indeed, kind canNOT be 2, since all such a particcles
                     * was processed at the first step. So we leave the
                     * diagram at kind 1 -- just conventional case*/
                    extP->majorana='\1';
                    break;
              }/*switch(id[  dtable[j][i]-1  ].kind)*/
           default:/*Bosonic or already processed*/
              break;
        }/*switch(extP->fcount)*/
     }/*for(i=-ext_lines;i<0;i++)*/
  /*external lines are ready!*/

  /*Internal lines:*/
  for(j=1; !(j>int_lines) ;j++)if( l_outPos[j]==-1  ){/* not marked*/
     /*fflow must be initialised for all verices!:*/
     output[top_out].fflow=1;
     if(id[  dtable[0][j]-1  ].kind==-1){ /*Fermion!*/
        output[top_out].fcount=++fermionlines_count;
        set_form_id( dtable[0][j]-1, output[top_out].i=wrktable[0][j]-1 );
        output[top_out].t='l';
        output[top_out].pos=j;
        l_outPos[j]=top_out;
        /*Define coincident vertex:*/
        for(i=1;!(i>vcount);i++) if(dtable[i][j])break;
        /*We need the END of a propagator. So clear the question:*/
        switch(id[  dtable[i][j]-1  ].kind){
           case 0:/*Majorana*/
              output[top_out].majorana='\0';/*Clear?*/
              break;
           case 1:/*Begin of propagator*/
              /*We have to pass to the routine the line attached to the end of propagator!*/
              /*Just continue -- we assume that there are no fermionic tadpoles!*/
              i++;
              for(;!(i>vcount);i++) if(dtable[i][j])break;
              /*So, this 'i' must correspond to the end of the propagator*/
              /*debug:*/
              /*Just in the off-chance-- to prevent crash at broken fermion line:*/
              if(!(dtable[i][j]))
                 halt("Internal error 6",NULL);

              /*No break!*/
           case 2:/*end of propagator -- claw!*/
              /* So now we have (j,i) line/vertex*/
              output[top_out].majorana='\1';/*Dirac fermion in 'right' direction*/
              break;
           default:/* -1 -- fermionic tadpole?*/
              /*dtable[i][j] must be a propagator!*/
              if( id[ *(id[  dtable[i][j]-1  ].link) ].kind==0)
                 /* Particle co-incide with antiparticle, majorana tadpole*/
                 output[top_out].majorana='\0';
              else
                 /*Dirac tadpole*/
                  output[top_out].majorana='\1';
              addtoidentifiers(output[top_out++].i,'f','l');
              /*And now we must process the vertex!*/
              output[top_out].fcount=fermionlines_count;
              set_form_id( dtable[i][0]-1, output[top_out].i=wrktable[i][0]-1 );
              output[top_out].t='v';
              output[top_out].pos=i;
              v_outPos[i]=top_out;
              output[top_out].majorana='\1';
              addtoidentifiers(output[top_out++].i,'f','v');
              i=0;/*Use i to prevent processing this line after the switch*/
        }/*switch(id[  dtable[i][j]-1  ].kind)*/
        /*Now if i == 0 then the line is a tadpole and already is ready:*/
        if(i){
           v=output[top_out].majorana;/*We need this parameter for processFLine, but we must
                                        increment top_out before invoke it*/

           /*Concerning fflow sign.
             If v == 0, we deal with majorana. By construction, we
             were seeking from the smallest numbers, so the line is directed
             (in red topology sense) FROM v, we are going to v, but our movement is
             OPPOSITE the fflow, so fflow coincides with red! Do nothing with
             output[top_out].fflow here.*/

           addtoidentifiers(output[top_out++].i,'f','l');
           /*j-line,i-vert,v= 0||+1 -- fflow direction, &i return last vertex*/
           if(processFLine(j,i,v,&i)!=j)/*This must be CLOSED fermion line!*/
              halt("Internal error 7",NULL);
        }/*if(i)*/
     }else{/*bosonic*/
        output[top_out].fcount=0;
        set_form_id( dtable[0][j]-1, output[top_out].i=wrktable[0][j]-1 );
        output[top_out].t='l';
        output[top_out].pos=j;
        l_outPos[j]=top_out;
        output[top_out].majorana=0;/*Not a fermion -- insensible agains flow*/
        addtoidentifiers(output[top_out++].i,'c','l');
     }/*if(id[  dtable[0][i]-1  ].kind==-1)...else*/
  }/*for(j=1; !(j>int_lines) ;j++)if( l_outPos[j]!=-1  )*/

  /* Only bosonic vertices remains*/
  for(j=1;!(j>vcount); j++)if( v_outPos[j]==-1  ){
     set_form_id( dtable[j][0]-1, output[top_out].i=wrktable[j][0]-1 );
     output[top_out].t='v';
     output[top_out].pos=j;
     output[top_out].fcount=0;
     v_outPos[j]=top_out;
     addtoidentifiers(output[top_out++].i,'c','v');
  }/*for(j=1;!(j>vcount); j++)if( v_outPos[j]!=-1  )*/
  /*That's all.*/
}/*build_form_input*/

