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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <math.h>
#include <errno.h>
#include "tools.h"
#include "comdef.h"
#include "types.h"
#include "tt_type.h"
#include "variabls.h"
#include "headers.h"
#include "texts.h"

long int str2long(char *str)
{
char *b;
register long int i=strtol(str,&b,10);
   if( ((*str)=='\0')||( (*b) !='\0' ) )
      halt(NUMBEREXPECTED,str);
return i;
}/*str2long*/

int must_diagram_be_skipped(void)
{
register int i,ret=0;
     for(i=1; !(i>int_lines); i++){
       if ( is_bit_set(&(id[dtable[0][i]-1].skip),0) ){
          ret=1;break;
       }
       if ( is_bit_set(&(id[dtable[0][i]-1].skip),1) )
          unset_bit(&(id[dtable[0][i]-1].skip),1);
     }
     if (!ret)
        for(i=1; !(i>vcount); i++){
          if ( is_bit_set(&(id[dtable[i][0]-1].skip),0) ){
             ret=1;break;
          }
          if ( is_bit_set(&(id[dtable[i][0]-1].skip),1) )
             unset_bit(&(id[dtable[i][0]-1].skip),1);
        }
     for(i=0; i<includeparticletop;i++){
        if (is_bit_set(&(id[includeparticle[i]].skip),1))
           ret=1;
        else
           set_bit(&(id[includeparticle[i]].skip),1);
     }
 return(ret);
}/*must_diagram_be_skipped*/

int skip_prototype(void)
{
register struct template_struct *tmp=template_p;
register  int r=0;
  do{
      if(cmp_template(prototypes[cn_prototype],tmp->pattern)){
        r=1;break;
      }
  }while((tmp=tmp->next)!=NULL);
  return(!r);
}/*skip_prototype*/

static char *get_index(int n)
{
  if(iNdex[n]==NULL)
       halt(INDICESALL,index_id[n]);
  iNdex[n]=iNdex[n]->next;
  if(iNdex[n]==NULL)
       halt(INDICESALL,index_id[n]);
  return(iNdex[n]->id);
}/*char *get_index*/

/* Allocates indices in memory as follows:
 *<n, number of groups><group1><index as m-string> ... <group n><index as m-string>
 * Input array "info" is:
 * <n, number of groups><group1><number of indices> ....
 :*/
char *alloc_indices(int *info)
{
 char tmp[MAX_STR_LEN],*tmp_ptr=tmp,*mem,i;
 int n,j,group;
   *tmp_ptr++=*info;/* Set number of groups*/
   i=(*info)*3+1;/* 1+1+1 per group: 1 for m-string header,
                  * 1 for a group trailing '\0', 1 for a group
                  * number; and 1 for the number of groups*/
   for(j=*info++;j>0;j--){
      group=*info++;/* Set the group number*/
      *tmp_ptr++=group+'0';/* Set the TEXTUAL group number*/
      n=*tmp_ptr++=*info++;/*Set the number of indices allocated in this group*/
      if(n>1)
         i+=(n-1);
      /* Get the indices:*/
      while(n--){
         mem=get_index(group);
         while( (*tmp_ptr++=*mem++)!='\0')i++;
      }/*while(n--)*/
   }/*for(j=*info++;j>0;j--)*/
   mem=get_mem(i,sizeof(char));
   for(n=0;n<i;n++)mem[n]=tmp[n];
   return(mem);
}/*alloc_indices*/

void create_table(word ***buf,word ***table,int vertex,
                   int internal_lines, int external_lines)
{
  int i,number_of_lines=external_lines+1+internal_lines;
     (*buf)=get_mem(vertex+1,sizeof(word **));
     (*table)=get_mem(vertex+1,sizeof(word **));
     for(i=0;!(i>vertex);i++){
        (*buf)[i]=get_mem(number_of_lines,sizeof(word));
        (*table)[i]=(*buf)[i]+external_lines;
     }
}/*create_table*/

void clear_table(word ***buf,word ***table,int vertex)
{
  int i;
     for(i=0;!(i>vertex);i++)
        free((*buf)[i]);
     free(*table);*table=NULL;
     free(*buf);*buf=NULL;
}/*clear_table*/

int cmp_template(char *string,char *pattern)
{
  char *p = pattern, *n = string;
  char c;

  while ((c = *p++) != '\0')
    {
      switch (c)
	{
	case '?':
	  if (*n == '\0')
	    return (0);
	  break;

	case '*':
	  for (c = *p++; c == '?' || c == '*'; c = *p++, ++n)
	    if(c == '?' && *n == '\0')
	      return (0);

	  if (c == '\0')
	    return (1);

	  {
	    char c1 =  c;
	    for (--p; *n != '\0'; ++n)
	      if ((c == '[' || *n == c1) &&
		  cmp_template(n, p) == 1)
		return (1);
	    return (0);
	  }

	case '[':
	  {
	    /* Nonzero if the sense of the character class is inverted.  */
	    register int not;

	    if (*n == '\0')
	      return (0);

	    /* Make sure there is a closing `]'.  If there isn't, the `['
	       is just a character to be matched. */
	    {
	      register char *np;

	      for (np = p; np && *np && *np != ']'; np++);

	      if (np && !*np)
		{
		  if (*n != '[')
		    return (0);
		  goto next_char;
		}
	    }

	    not = (*p == '!' || *p == '^');
	    if (not)
	      ++p;

	    c = *p++;
	    for (;;)
	      {
		register char cstart = c, cend = c;

		if (c == '\0')
		  /* [ (unterminated) loses.  */
		  return (0);

		c = *p++;

		if (c == '-' && *p != ']')
		  {
		    cend = *p++;
		    if (cend == '\0')
		      return (0);
		    c = *p++;
		  }

		if (*n >= cstart && *n <= cend)
		  goto matched;

		if (c == ']')
		  break;
	      }
	    if (!not)
	      return (0);

	  next_char:
	    break;

	  matched:
	    /* Skip the rest of the [...] that already matched.  */
	    while (c != ']')
	      {
		if (c == '\0')
		  /* [... (unterminated) loses.  */
		  return (0);

		c = *p++;
	      }
	    if (not)
	      return (0);
	  }
	  break;

	default:
	  if (c != *n)
	    return (0);
	}

      ++n;
    }

  if (*n == '\0')
    return (1);

  return (0);
}/*cmp_template*/

/*Detect type of particle on line l in table table:*/
char get_type(char l,word **table)
{
  char i;
     for(i=1;!(i>vcount);i++)
       if(table[i][l])break;
     return(id[table[i][l]-1].type);
}/*get_type*/

/*Detect interna id number of particle on line l in table table:*/
char get_number(char l,word **table)
{
  char i;
     for(i=1;!(i>vcount);i++)
       if(table[i][l])break;
     return(id[table[i][l]-1].number);
}/*get_number*/

/* Exchnges a and b elements into arr:*/
void swapch(char* arr, char a, char b)
{
  char tmp;
     tmp=arr[a];arr[a]=arr[b];arr[b]=tmp;
}/*swapch*/

/* Exchnges a and b elements into arr:*/
void swapw(word* arr, int a, int b)
{
  word tmp;
     tmp=arr[a];arr[a]=arr[b];arr[b]=tmp;
}/*swapw*/

/*see comment in pilot.c, seek  'MAJORANA fermions'*/
/*
direction_noauto:
Returns 1, if direction of fermion coinside with direction of line l.
For Majorana particles it returns 1.
If it opposite, the function returns -1, If tadpole, return 1:*/
int direction_noauto(char l)
{
 register word i;

    if( (l<-ext_lines) || (l==0) ||(l>int_lines))
       return(0);

    if(islast)return(1);/*Invoked on "islast" condition, deals with a pure topology*/
    if (l<0){/*external*/
       for(i=1;!(i>vcount);i++)
          if(dtable[i][l]){
             int thePlus=farrow;

             i=(dtable[i][l]-1);
             if(id[i].kind==0)/*Majorana or boson*/
                return ext_particles[-l-1].fflow;
             /* If the particle is INgoing,then id[i] contains the end,
                not the beginning! So the sign must be upset:*/
             if(ext_particles[-l-1].is_ingoing)
                thePlus=-thePlus;

             if(id[i].kind==2)return(-thePlus);
             return(thePlus);
          }/*if(dtable[i][l])*/
    }/*if (l<0)*/

    /*Here l>0!*/
    for(i=1;!(i>vcount);i++)
       if(dtable[i][l]){
          if(*(id[ i=(dtable[i][l]-1) ].id)==2)/*Tadpole*/
             return(1);
          if(id[i].kind==2)return(-farrow);
          if(id[i].kind==0){/*Majorana or boson*/
             return output[l_outPos[l]].fflow;
          }/*if(id[i].kind==0)*/
          return(farrow);
       }
    return(0);
}/*direction_noauto*/

/*see comment in pilot.c, seek  'MAJORANA fermions'*/
/*Returns 1, if direction of fermion coinside with direction of line l AND
the fermion number flow is +1, i.e., positive dirac direction coincides with
topological one. Otherwise, returns -1, If tadpole, return 1.
For Majorana particles it returns 1:*/
int direction_auto(char l)
{
register  word i;
 int thePlus;
    if( (l<-ext_lines) || (l==0) ||(l>int_lines))
       return(0);

    if(islast)return(1);/*Invoked on "islast" condition, deals with a pure topology*/

    if (l<0){/*external*/
       switch((int)(ext_particles[-l-1].majorana)){
          case 1:
          case 0:
             thePlus=farrow;break;
          default:/*-1*/
             thePlus=-farrow;break;
       }/*switch((int)(output[l_outPos[l]].majorana))*/
       for(i=1;!(i>vcount);i++)
          if(dtable[i][l]){
             i=(dtable[i][l]-1);
             /*Now i is the index of id of external particle.
               BUT - ingoing, or outgoing??*/
             /*Majorana or boson - the sign is independent on in/out:*/
             if(id[i].kind==0)return(ext_particles[-l-1].fflow);
             /*If the particle is INgoing, then id[i] contains the end,
             not the beginning! So the sign must be upset:*/

             if(ext_particles[-l-1].is_ingoing)thePlus=-thePlus;

             if(id[i].kind==2)/*end of a propagator*/
                return(-thePlus);

             return(thePlus);/*begin of a propagator*/
          }/*if(dtable[i][l])*/
    }/*if (l<0)*/
    /*Here l>0!!!*/
    switch((int)(output[l_outPos[l]].majorana)){
       case 1:
       case 0:
          thePlus=farrow;break;
       default:/*-1*/
          thePlus=-farrow;break;
    }/*switch((int)(output[l_outPos[l]].majorana))*/
    for(i=1;!(i>vcount);i++)
       if(dtable[i][l]){
          if(*(id[ i=(dtable[i][l]-1) ].id)==2)/*Tadpole*/
             return(1);
          if(id[i].kind==2)return(-thePlus);
          if(id[i].kind==0)return(output[l_outPos[l]].fflow);
          return(thePlus);
       }
    return(0);
}/*direction_auto*/

/*Returns 1, if direction of propagator coinside with direction of line l.
If it opposite, the function returns -1, If tadpole, return 1.
It deals only with topologies and propagators, it does not know anything
about spinors.:*/
int pdirection(char l)
{
 word i;
 int dir;
    if((!(l>0))||(l>int_lines))
       return(0);
    if(islast)return(1);/*Invoked on "islast" condition, deals with a pure topology*/
    for(i=1;!(i>vcount);i++)
       if(dtable[i][l]){
          if(*(id[ i=(dtable[i][l]-1) ].id)==2)/*Tadpole*/
             return(1);
          dir=topologies[cn_topol].l_dir[lt_subst[l]]*l_dir[l];
          /*See "The substituions map" in "variables.h"*/
          if(id[i].kind==2)return(dir);
          return(-dir);
       }
    return(0);
}/*pdirection*/

/*Returns in arg the result. "momentum" is an array of integers of  a length
momentum[0]. Each momentum[i] corresponds to one vec_group[ abs(momentum[i]) ].text
The sign correspods to sgn(momentum[i]). The parameter "sign" is an additional sign
so the resulting sign is sgn(momentum[i])*sign.

31102002 - extended; if the group is started from '!', it assumes that
this group is NOT required:
*/
char *build_momentum(char *group, int *momentum, char *buf, int sign)
{
register int i;
register char *ptr,j;
char sch[2];/*"+" or "-"*/
int l_zeromomentum=0;
    if(   (momentum==NULL)||(*momentum==0)   )
       return s_let(g_zeroVec,buf);


    if(
        (g_zeromomentum!=NULL)&&
        (g_zeromomentum[0]==1)&&
        (g_zeroVec!=NULL)/*this function is used to build g_zeroVec!*/
      )l_zeromomentum=abs(g_zeromomentum[1]);

    sch[1]=0;*buf=0;
    for(i=1;!(i>*momentum);i++){

       if(l_zeromomentum == abs(momentum[i]))
          continue;

       *sch= (    (  ( (momentum[i]>0)?1:(-1) )*sign  )>0    )?('+'):('-');
       if(group[2]==1){/* Full momentum requared -- add current group*/
          s_cat(buf,buf,sch);
          s_cat(buf,buf,vec_group[abs(momentum[i])].text);
       }else{/* Defining is current group among 'group' */
          ptr=group+3;
          for(j=1; j<group[2]; j++){
             int corr=0;
             if(*ptr=='!'){
                ptr++;
                corr=1;
             }/*if(*ptr=='!')*/
             if(  s_gcmp(vec_group[abs(momentum[i])].vec,ptr,1)-corr   ){/*Need*/
                s_cat(buf,buf,sch);
                s_cat(buf,buf,vec_group[abs(momentum[i])].text);
                break;
             }
             /*Go to next group in 'group':*/
             while(!((*ptr==1)||(*ptr=='\n')))ptr++;
             ptr++;
          }
       }
    }

    if(  (*buf) == '\0' ) s_let(g_zeroVec,buf);

    return(buf);
}/*build_momentum*/

/*Set momenta of form input text in wrktable:*/
void create_vertex_text(char v)
{
   int i,j,k,m,dir;
   int *cmomentum;
   char *ptr,str[MAX_STR_LEN],tmp[MAX_STR_LEN],cmpp[3];

      if((!(v>0))||(v>vcount))
         halt(CURRUPTEDINPUT,input_name);
      if( id[ dtable[v][0]-1 ].Nvec == '\0')return;/*No vectors in vertex*/

      /*Replace all momenta according to vcall to text momenta:*/

      j=*(id[dtable[v][0]-1].id);/*id is m-string!*/
      /*Now j is equal to the number of particles in this vertex*/
      for(i=0;i<j;i++){/*Loop at all incoming particles*/
         cmpp[0]=3;cmpp[2]=0;
         cmpp[1]=i+'0';
         k=s_count(cmpp,text[wrktable[v][0]-1]);
         /*Now k equal to the number of momenta requested for particle i.
           There may be several momenta requests for single particle!*/
         dir=(vertices[v][i].is_ingoing)?1:(-1);
         /*Determine momenta array:*/
         if(vertices[v][i].n<0){/*external particle*/
              if(
                (topologies[cn_topol].ext_momenta!=NULL) &&
                (topologies[cn_topol].ext_momenta[cn_momentaset]!=NULL)
                )
                 cmomentum=topologies[cn_topol].ext_momenta
                                       [cn_momentaset][-vertices[v][i].n];
               else
                 /* cmomentum=ext_particles[ext2my[-vertices[v][i].n]].momentum;*/
                 cmomentum=ext_particles[-vertices[v][i].n-1].momentum;
         }else{/*internal line*/
               cmomentum=momenta[vertices[v][i].n];
               if(g_flip_momenta_on_lsubst)
                  dir*=l_dir[vertices[v][i].n];
         }/*if(vertices[v][i].n<0) ... else*/
         /* Now cmomentum is a proper momenta array, and dir (+-1) is a proper direction*/
         for(m=0;m<k;m++){/*loop on all momenta required for this particle:*/
            ptr=text[wrktable[v][0]-1]+s_pos(cmpp,text[wrktable[v][0]-1]);
            build_momentum(ptr,cmomentum,str,dir);            
            s_letc(ptr,tmp,'\n');
            s_replace(s_cat(tmp,tmp,"\n"),str,text[wrktable[v][0]-1]);
         }/*for(m=0;m<k;m++)*/
      }/*for(i=0;i<j;i++) -- Loop at all incoming particles*/
}/*create_vertex_text*/

/*Set momenta of form input text in wrktable:*/
void create_line_text(char l)
{
  char *ptr;
  int dir;
  char str[MAX_STR_LEN],tmp[MAX_STR_LEN];
      if((!(l>0))||(l>int_lines))
         halt(CURRUPTEDINPUT,input_name);
      if( id[ dtable[0][l]-1 ].Nvec == '\0')return;/*No vectors in propagator*/
      if(g_flip_momenta_on_lsubst)
         dir=l_dir[l]*direction(l);
      else
         dir=direction(l);
      ptr=text[ wrktable[0][l]-1 ]+1;
      while(*ptr){
          if(*ptr==3){/*vector*/
             /*Build momentum in str:*/
             build_momentum(ptr,momenta[l],str, dir);
             /*Replace vector calling by its value:*/
             *tmp=3;tmp[1]=127;tmp[2]=ptr[2];
             s_letc(ptr+3,tmp+3,'\n');
             s_replace(s_cat(tmp,tmp,"\n"),str,text[wrktable[0][l]-1]);
          }
          ptr++;
      }
}/*create_line_text*/

/*Returns index of fermion line continuing line l from vertex v.
If it is impossible, returns 0. Works as well with Majorana particles:*/
char fcont(char l, char v)
{
  char i;
  if((l<-ext_lines)||(l>int_lines)||(l==0))
     return(0);
   if( id[  dtable[0][l]-1  ].kind == 1 )return(0);/*Boson*/
   if( dtable[v][l] == 0 )return(0);/*line does not incendent to the vertex*/
   for(i=-ext_lines; !(i>int_lines) ;i++)
      if( (i)&&(dtable[v][i]) ){
         if(id[  dtable[0][i]-1  ].kind == -1){/*fermionic*/
           if(
               (i!=l) ||/*Found continuation*/
               ( *(id[dtable[v][i]-1].id)==2 )/*Tadpole - appropriate, too*/
           )
              return(i);
         }
      }
   return(0);
}/*fcont*/

/*Forms form_id into text[text_ind] according to current fermionlines_count:*/
void set_form_id(word curId,word text_ind)
{
  char i;
  char *beg=text[text_ind]+1;
  char str[MAX_STR_LEN],tmp[MAX_STR_LEN];
  long ferm_c;
      if(id[curId].kind == -1)
         ferm_c=fermionlines_count;
      else
         ferm_c=0;
      /*First approach: replace the keyword fnum:*/
      if(id[curId].Nfnum!='\0'){
         long2str(tmp,ferm_c);
         /*while(s_pos(p12,text[text_ind])!=-1)*/
         for(i=id[curId].Nfnum; i>0;i--)
          s_replace(p12,tmp,text[text_ind]);

      }else if(*(text[text_ind])!=1){
        /*Second approach: if there is no the keyword, we try to use different
        identifier:*/
             if(*(text[text_ind])<ferm_c){
                s_letc(text[text_ind]+1,str,1);
                halt(UNSUFFICIENTFERMID,str);
             }
          for(i=1;i<ferm_c;i++){
             while(*beg++!=1);
          }
      }/*else if(*(text[text_ind])!=1)*/
      s_letc(beg,str,1);
      *tmp=*(text[text_ind]);
      s_letc( text[text_ind]+1, tmp+1, '\n');
      s_replace(s_cat(tmp,tmp,"\n"),str,text[text_ind]);
}/*set_form_id*/

/*Returns vertex index of the second end of line l in vertex v:*/
char endline(char l, char v)
{
  char i;
   if((!(l>0))||(l>int_lines))
      return(0);
   if((!(v>0))||(v>vcount))
      return(0);
   if(dtable[v][l]==0)return(0);
   if( *(id[  dtable[v][l]-1  ].id) == 2 )return(v);/*tadpole*/
   for(i=1; !(i>vcount) ;i++)
      if( (dtable[i][l])&&(i!=v) )return(i);
   return(0);
}/*endline*/

/*Returns vertex index which is a source of internal line l:*/
char fromvertex(char l)
{
  char i;
  int d;
   if((l<-ext_lines)||(l>int_lines)||(l==0))
       return(0);
   if (l>0){/*See "The substituions map" in "variables.h"*/
      d=topologies[cn_topol].l_dir[lt_subst[l]]*l_dir[l];
      for(i=1;!(i>vcount);i++)
         if(dtable[i][l]){
            if( *(id[  dtable[i][l]-1  ].id) == 2 )return(i);/*tadpole*/
            if(d==1)return(i);
            d=1;
         }
      return(0);
   }else
     return(topologies[cn_topol].topology->e_line[-l].from);
}/*fromvertex*/

/*Returns vertex index which is end of internal line l:*/
char tovertex(char l)
{
  char i;
  int d;
   if((l<-ext_lines)||(l>int_lines)||(l==0))
       return(0);
   if (l>0){/*See "The substituions map" in "variables.h"*/
      d=topologies[cn_topol].l_dir[lt_subst[l]]*l_dir[l];
      for(i=1;!(i>vcount);i++)
         if(dtable[i][l]){
            if( *(id[  dtable[i][l]-1  ].id) == 2 )return(i);/*tadpole*/
            if(d==-1)return(i);
            d=-1;
         }
      return(0);
   }else
      return(topologies[cn_topol].topology->e_line[-l].to);
}/*tovertex*/

/*Returns index of fermion line outgoing from vertex v.
If it is impossible, returns 0:*/
char outfline(char v)
{
  char i;
   if((!(v>0))||(v>vcount))
      return(0);
   if( id[  dtable[v][0]-1  ].kind == 1 )return(0);/*Commutig*/
   for(i=-ext_lines; !(i>int_lines) ;i++)
      if( (dtable[v][i])&&(i) )
         if(
           (id[  dtable[0][i]-1  ].kind == -1)&&( /*fermion*/
               (id[  dtable[v][i]-1  ].kind == 2 )|| /*end of propagator*/
               ( *(id[  dtable[v][i]-1  ].id) == 2 ) /*tadpole*/
           )
           ) return(i);
   return(0);
}/*outfline*/

void addtoidentifiers (word textindex,char fc, char lv)
{
word i;
     for(i=0;i<top_identifiers;i++)
        if(cmp2b(text[textindex],text[identifiers[i].i]))return;
     if( !( top_identifiers<max_top_identifiers  )  ){
        if(
           (identifiers=realloc(identifiers,
             (++max_top_identifiers)*
                          sizeof(struct identifiers_structure )
                             )
           )==NULL
          )halt(NOTMEMORY,NULL);
     }
     identifiers[top_identifiers].i=textindex;
     identifiers[top_identifiers].fc=fc;
     identifiers[top_identifiers++].lv=lv;
}/*addtoidentifiers*/

void out_undefined_topology(void)
{
 char tmp[MAX_STR_LEN];

    sprintf(tmp,WARNINGDIAGRAMCOUNT,diagram_count);
    message(tmp,NULL);
    message(TOPOLOGYISUNDEFINED,top2str(topologies[cn_topol].topology,tmp));
}/*out_undefined_topology*/

/*Allocates and returns identical (123etc) or unit (1111etc) substitutions of a length |l|.
  If l<0, performs 1111etc, otherwise, performs 123etc:*/
char *alloc123Or111etc(int l)
{
   int i,buf[1]={1},*ptr;
   char *tmp;
      if(l<0){l=-l;ptr=buf;}
      else ptr=&i;

      tmp=get_mem(l+2,sizeof(char));

      for(*tmp=-1,i=1;i<=l;i++)tmp[i]=*ptr;
      tmp[i]='\0';
      return(tmp);
}/*alloc123etc*/

int substitutel(char *str, char *pattern)
{
 char buf[MAX_ITEM];
 int i,j;

    if((i=s_len(str+1))!=s_len(pattern))return(-1);
    s_let(str+1,buf+1);
    for(j=1;!(j>i);j++){
       if((str[j]=pattern[buf[j]-1])>i)return(-1);
       buf[j]=0;
    }
    *buf=0;
    for(j=0;j<i;j++){
       if(buf[pattern[j]-1])return(-1);
       buf[pattern[j]-1]=1;
    }
    return(0);
}/*substitutel*/

int  substituter(char *str, char *pattern)
{
 char buf[MAX_ITEM];
 int i,j;

    if((i=s_len(str+1))!=s_len(pattern))return(-1);
    s_let(str+1,buf);
    for(j=1;!(j>i);j++){
       if((str[j]=buf[pattern[j-1]-1])>i)return(-1);
    }
    for(j=0;j<i;j++)buf[j]=0;
    i++;
    for(j=1;j<i;j++){
       if(buf[str[j]-1])return(-1);
       buf[str[j]-1]=1;
    }
    return(0);
}/*substituter*/

char *invertsubstitution(char *str, char *substitution)
{
 register int i,j;
    *str=-1;
    for(i=1; substitution[i]!= '\0'; i++){
        if( (j=substitution[i])<0)j=-j;
        str[j]=i;
    }
    str[i]='\0';
    return(str);
}/*invertsubstitution*/

char *p_reoder(char *prototype, char *pattern)
{
 char buf[MAX_ITEM];
 int i;
    i=s_len(prototype)-1;
    s_let(prototype,buf);
    for(;!(i<0);i--)
       prototype[pattern[i]-1]=buf[i];
    return(prototype);
}/*p_reoder*/

char *let2b( char *from,char *to)
{
 word i;
    for(i=0;from[i] && (i < MAX_STR_LEN);i++)if(((to[i]=from[i])=='(')||
               (from[i]=='[')||(from[i]=='{'))break;
    to[i]=0;
    return(to);
}/*let2b*/

int cmp2b(char *a, char *b)
{

  while (*a&&*b){
     if(( (*a=='(')||(*a=='{')||(*a=='[') )&&( (*b=='(')||(*b=='{')||(*b=='[') ))
         return(1);
     if(*a!=*b)return(0);
     a++;b++;
  }
  return(((*a==0)||(*a=='(')||(*a=='{')||(*a=='[') )&&((*b==0)||(*b=='(')||(*b=='{')||(*b=='[') ));
}/*cmp2b*/

FILE *open_file_follow_system_path(char *name, char *mode)
{
FILE *tmp=NULL;
   if(   (tmp=fopen(name, mode))==NULL  ){
      char buf[MAX_STR_LEN];
      tmp=fopen(s_cat(buf,system_path,name),mode);
   }/*if(   (tmp=fopen(name, mode))==NULL  )*/
   return(tmp);
}/*open_file_follow_system_path*/

FILE *open_system_file(char *name)
{
FILE *tmp=open_file_follow_system_path(name, "rt");
   if(tmp==NULL)
      halt(CANNOTOPEN ,name);
   return(tmp);
}/*open_system_file*/

FILE *link_file(char *name, long pos)
{
struct flinks_struct *tmp=NULL;
     if (flinks_table==NULL)flinks_table=create_hash_table(
                      FLINKS_HASH_SIZE,str_hash,str_cmp,flinks_destructor);
     if ((tmp=lookup(name,flinks_table))==NULL){
        tmp=get_mem(1,sizeof(struct flinks_struct));
        tmp->stream=open_system_file(name);
        tmp->links=1;
        install(new_str(name),tmp,flinks_table);
     }else{
        if (tmp->links!= 0xFFFF)(tmp->links)++;
     }
     if(fseek(tmp->stream, pos, SEEK_SET))
        halt(CANNOTOPEN,name);
     return(tmp->stream);
}/*link_file*/

void keep_file(char *name)
{
struct flinks_struct *tmp=NULL;
     if ((tmp=lookup(name,flinks_table))!=NULL)
         tmp->links=0xFFFF;
}/*keep_file*/

FILE *link_stream(char *name, long pos)
{
struct flinks_struct *tmp=NULL;
     if ((tmp=lookup(name,flinks_table))==NULL)
        halt(CANTOPENFILE,name);
     if(fseek(tmp->stream, pos, SEEK_SET))
        halt(CANNOTOPEN,name);
     return(tmp->stream);
}/*link_stream*/

word unlink_file(char *name)
{
struct flinks_struct *tmp=NULL;
int ret;
     if (flinks_table==NULL)return(-1);
     if ((tmp=lookup(name,flinks_table))==NULL)return(-1);
     if (tmp->links!= 0xFFFF){
        if(tmp->links > 0)(tmp->links)--;
     }
     ret=tmp->links;
     if(ret==0)uninstall(name,flinks_table);
     return(ret);
}/*unlink_file*/

void unlink_all_files(void)
{
   if (flinks_table != NULL){
       hash_table_done(flinks_table);
       flinks_table=NULL;
    }
}/*unlink_all_files*/

char *letnv(char *from,char *to, word n)
{
 word i=1;
#ifdef DEBUG
     if ((from==NULL)||(to==NULL))halt("Argument letnv is NULL",NULL);
#endif
    for(i=0;i<n;i++)if((to[i]=from[i])==0) break;
    if((i==n)&&(i)){
       if (from[i-1])
          halt(TOOLONGSTRING,NULL);
       else
          to[i-1]=0;
    }
    return(to);
}/*letnv*/

void allocate_topology_coordinates( aTOPOL *theTopology )
{
double theMin=MIN_POSSIBLE_COORDINATE-10.0;
int i,
    max_l=(theTopology->topology)->i_n,/* Will be multiplied by 2 soon*/
    max_e=2*((theTopology->topology)->e_n),
    max_vertex=2*(theTopology->max_vertex);
    theTopology->rad=get_mem(max_l,sizeof(double));
    theTopology->ox=get_mem(max_l,sizeof(double));
    theTopology->oy=get_mem(max_l,sizeof(double));
    theTopology->start_angle=get_mem(max_l,sizeof(double));
    theTopology->end_angle=get_mem(max_l,sizeof(double));

    max_l*=2;
   /* external vertices:*/
   theTopology->ev=get_mem(max_e,sizeof(double));
   /* external vertices labels:*/
   theTopology->evl=get_mem(max_e,sizeof(double));
   /* internal vertices:*/
   theTopology->iv=get_mem(max_vertex,sizeof(double));
   /* internal vertices labels:*/
   theTopology->ivl=get_mem(max_vertex,sizeof(double));
   /* external lines:*/
   theTopology->el=get_mem(max_e,sizeof(double));
   /* external lines labels:*/
   theTopology->ell=get_mem(max_e,sizeof(double));
   /* internal lines:*/
   theTopology->il=get_mem(max_l,sizeof(double));
   /* internal lines labels */
   theTopology->ill=get_mem(max_l,sizeof(double));

   for(i=0;i<max_l;i++)
      (theTopology->il)[i]=
      (theTopology->ill)[i]=theMin;
   for(i=0;i<max_e;i++)
      (theTopology->ev)[i]=
      (theTopology->evl)[i]=
      (theTopology->el)[i]=
      (theTopology->ell)[i]=theMin;
   for(i=0;i<max_vertex;i++)
      (theTopology->iv)[i]=
      (theTopology->ivl)[i]=theMin;

   /* Mark that there are no coordinates:*/
   theTopology->coordinates_ok=0;
}/*allocate_topology_coordinates*/
#define stPSINIT 0
#define stPSEND 1
#define stPSCONT 2

static FILE *outPSfile=NULL;
static int outPSstate=stPSINIT;
static char psch='\0';
static int pslen=0;
void outPSinit(char *fname)
{
   outPSstate=stPSCONT;
   psch='\0';
   pslen=0;
   if(fname!=NULL){
      if (  (outPSfile=fopen( fname, "a+"))==NULL  )
         halt(CANTOPENFILE,fname);
   }/*if(fname!=NULL)*/

}/*outPSinit*/

void outPSend(void)
{
   if(outPSfile!=NULL){
      if(psch != '\n')
         fputc('\n',outPSfile);
      fclose(outPSfile);
   }
   outPSstate=stPSINIT;
   psch='\0';
   outPSfile=NULL;
}/*outPSend*/

static void l_mkPSutToFile(char ch)
{
   if(fputc(ch,outPSfile) == EOF )
      halt(CANNOTWRITETOFILE,"PS");
}/*l_mkPSutToFile*/

void outPS(char *ch,MKPSOUT mkPSout)
{
   if( !( (*ch) >' ') ){
      if( pslen> 70 ){
         pslen=0;
         (*ch) ='\n';
      }else{
         if(!  (psch > ' ')  )
            return;
         else if ( (*ch) == '\n')
            (*ch) = ' ';
      }
   }
   pslen++;
   if(mkPSout!=NULL)
      mkPSout(*ch);
   psch =*ch;
}/*outPS*/

#define stPSONECOM 3
#define stPSSKIP 4

int initPS(char *header, char *target)
{
   int state;
   char ch;
   FILE *infile=NULL;
   if(atleast_one_topology_has_coordinates == 0)return(-1);
   outPSinit(target);
   infile=open_system_file(header);
   for( state=stPSINIT;state!=stPSEND; )switch(state){
      case stPSINIT:
         if(  (ch=fgetc(infile))==EOF ){
            state=stPSEND;
         }else if( ch == '%' ){
            state=stPSONECOM;
         }else{
            outPS((char*)&ch,&l_mkPSutToFile);
         }
         break;
      case stPSONECOM:
         if(  (ch=fgetc(infile))==EOF ){
            state=stPSEND;
         }else if( ch == '%' ){
            outPS((char*)&ch,&l_mkPSutToFile);
            outPS((char*)&ch,&l_mkPSutToFile);
            state=stPSINIT;
         }else if( ch == '\n' ){
            outPS((char*)&ch,&l_mkPSutToFile);
            state=stPSINIT;
         } else {
            state=stPSSKIP;
         }
         break;
      case stPSSKIP:
         if(  (ch=fgetc(infile))==EOF ){
            state=stPSEND;
         } else if( ch == '\n' ){
            outPS((char*)&ch,&l_mkPSutToFile);
            state=stPSINIT;
         }
         break;
   }/*for( state=stPSINIT;state!=stPSEND; )switch(state)*/
   outPSend();
   fclose(infile);
   return(0);
}/*initPS*/

void allocate_indices_structures(void)
{
int i;
   index_id=get_mem(numberOfIndicesGroups,sizeof(char*));
   indices=get_mem(numberOfIndicesGroups,sizeof(struct indices_struct *));
   iNdex=get_mem(numberOfIndicesGroups,sizeof(struct indices_struct *));
   for(i=0; i<numberOfIndicesGroups;i++){
         index_id[i]=NULL;
         iNdex[i]=indices[i]=NULL;
   }
}/*allocate_indices_structures*/

void clear_lvmarks(void)
{
int i,j;
  if(linemarks != NULL){
     if(linemarks[0]!=NULL){
        j=*(linemarks[0]);
        for(i=0; !(i>j);i++)
           free_mem(&(linemarks[i]));
     }
     free_mem(&(linemarks));
  }/*if(linemarks != NULL)*/

  if(vertexmarks != NULL){
     if(vertexmarks[0]!=NULL){
        j=*(vertexmarks[0]);
        for(i=0; !(i>j);i++)
           free_mem(&(vertexmarks[i]));
     }
     free_mem(&(vertexmarks));
  }

}/*clear_lvmarks*/

/*This function performs execvp(cmd,argv) and swallows  stdin (*send) and stdout
  (*receive) of the executed command. If noblock!=0, then the reading will not be
  blocked:*/
pid_t swallow_cmd(FILE **send, FILE **receive, char *cmd,  char *argv[], int noblock)
{
int fdin[2], fdout[2];
pid_t childpid;
  /* Open two pipes:*/
  pipe(fdin);
  pipe(fdout);

  /* Fork to create the child process:*/
  if((childpid = fork()) == -1){
    return(-1);
		}

  if(childpid == 0){/*Child.*/
      close(fdin[1]);/*Close up parent's input channel*/
      close(fdout[0]);/* Close up parent's output channel*/
      /* Use fdin as stdin :*/
      close(0);
		    dup(fdin[0]);
      /* Use fdout as stdout:*/
      close(1);
      dup(fdout[1]);
      /*Execute external command:*/
      execvp(cmd, argv);
      /* Control newer  reach this point!*/
      exit(-1);
		  }else{/* The father*/
		    close(fdin[0]);/* Close up output side of fdin*/
      close(fdout[1]);/*Close up input side of fdout*/
      if(  /* Set non-blocking reading, if needed:*/
           ( noblock && (fcntl(fdout[0],F_SETFL,O_NONBLOCK) == -1) )
           /* Connect file stream with the descriptors:*/
         ||(  (*send=fdopen(fdin[1],"w"))==NULL  )
         ||(  (*receive=fdopen(fdout[0],"r"))==NULL )
      ){/* Failing in finishing of the setup.*/
         if (*receive!=NULL) fclose(*receive);
         if (*send!=NULL) fclose(*send);
         kill(childpid,SIGHUP); /* warn the child process */
         kill(childpid,SIGKILL);/* kill the child process*/
         return(-1);
      }
      return(childpid);
    }
}/*swallow_cmd*/

/* Appends c to the end of s1 with memory rellocation for s1:*/
char *s_addchar(char *s1, char c)
{
register int l1;

   if(s1 == NULL){
      if(   ( s1=(char *)calloc(2,sizeof(char)) ) ==NULL   )return NULL;
      *s1=c;
      return s1;
   }/*if(s1 == NULL)*/

   for(l1=0;s1[l1]!='\0';l1++);
   if( (s1=(char *)realloc(s1,l1+2))==NULL ) return NULL;
   s1[l1++]=c;s1[l1]='\0';
   return s1;
}/*s_addchar*/

/*Concatenates s1 and s2 with memory rellocation for s1:*/
char *s_inc(char *s1, char *s2)
{
register int l1,l2;

   if(s2== NULL) return new_str(s1);
   if(s1 == NULL)return new_str(s2);
   for(l1=0;s1[l1]!='\0';l1++);
   for(l2=0;s2[l2]!='\0';l2++);
   if( (s1=(char *)realloc(s1,l1+l2+1))==NULL ) return NULL;
   while( (s1[l1++]=*s2++)!='\0' );
   return s1;
}/*s_inc*/

/*Concatenates i1 and i2 with memory relocation for i1, if s==0 upset appended i2:*/
int *int_inc(int *i1, int *i2,int s)
{
register int i;

   if(i1 == NULL){
      if(i2== NULL)
         return(int*) get_mem(1,sizeof(int));
      else
         i1=get_mem(1,sizeof(int));
   }
   if(i2== NULL) return new_int(i1);

   if( (i1=(int *)realloc(i1,(*i1+*i2+1)*sizeof(int)))==NULL ) return NULL;
   if(s)
      for(i=1;i<=*i2;i++ )i1[++(*i1)]=i2[i];
   else
      for(i=1;i<=*i2;i++ )i1[++(*i1)]=-i2[i];
   return i1;
}/*int_inc*/

/*returns 0 if NOT coincide, or 1:*/
int int_eq(int *i1, int *i2)
{
int  *wm;
int i;
   if(i1==NULL) return(i2==NULL);
   if(i2==NULL) return 1;
   if( *i1 != *i2 ) return 0;
   wm=get_mem(top_vec_group,sizeof(int));
   for(i=1; i<= *i1; i++){
      /*Add +i1[i] into the ruler: */
      if(i1[i]>0)
         wm[i1[i]]++;
      else
         wm[-i1[i]]--;

      /*Add -i2[i] into the ruler: */
      if(i2[i]>0)
         wm[i2[i]]--;
      else
         wm[-i2[i]]++;
   }/*for(i=1; i<= *i1; i++)*/
   /*If i1 == i2, the ruler must be empty. Check it:*/
   for(i=1; i<top_vec_group; i++) if(wm[i]){
      free(wm); return 0;/*Not empty*/
   }/*for(i=1; i<top_vec_group; i++) if(wm[i])*/
   /*empty*/
   free(wm);
   return 1;
}/*int_eq*/

static char integers[16]={'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};

char *int2hex(char *buf, unsigned long int n, int w)
{
   if(w>0)
      buf[w]='\0';
   else/*if w<0, this is used to indicate NO trailing '\0':*/
      w=-w;

   for (w--;w > -1;w--){
      buf[w]=integers[n%16];
      n/=16;
   }
   return buf;
}/*int2hex*/

long int hex2int(char *buf,int w)
{
int i,n,p;
   if( *buf =='\0' )return -1;
   if(w<1){
      char *tmp=buf;
      for(w=0;*tmp!='\0';tmp++,w++);
   }/*if(w<1)*/
   for(p=1,n=0,i=w-1; i>-1;i--,p*=16)switch(buf[i]){
      case '0':break;
      case '1':n+=p;break;
      case '2':n+=p*2;break;
      case '3':n+=p*3;break;
      case '4':n+=p*4;break;
      case '5':n+=p*5;break;
      case '6':n+=p*6;break;
      case '7':n+=p*7;break;
      case '8':n+=p*8;break;
      case '9':n+=p*9;break;
      case 'a':case 'A':n+=p*10;break;
      case 'b':case 'B':n+=p*11;break;
      case 'c':case 'C':n+=p*12;break;
      case 'd':case 'D':n+=p*13;break;
      case 'e':case 'E':n+=p*14;break;
      case 'f':case 'F':n+=p*15;break;
      case '-':n=-n;
         /*No break!*/
      case '+':
         if(i==0)return n;
         /*No break!*/
      default:return -1;
   }/*for(p=1;n=0,i=w-1; i>-1;i--,p*=16)switch(buf[i])*/
   return n;
}/*hex2int*/

/*The following function transforms string representing SIGNED decimal number into
  the string representing SIGNED haexadecimal number. If w is >0, then it will be used
  as a width of produced HEX number, no error checkup, if w is not enough, only
  last w digits will be placed if w is too large, the leading 0 will be produced;
  if w<1, then the produced width will be defined automatically.
  buf will be used only to the first non-digit character; leading '+' will be ignored.
  If buf is not a number, the empty string will be produced.*/
char *dec2hex(char *decbuf,char *hexbuf,int w)
{
register int i=0,n=1,p;
char *ptr=decbuf,/*The beginning of incoming digits*/
     *b=hexbuf;/*The beginning of a buffer for outputted HEX digits*/

   *hexbuf='\0';

   /*Leading sign:*/
   switch(*ptr){
      case '\0':return hexbuf;
      case '-':
         *b++='-';/*No break!*/
         w--;
      case '+':
         ptr++;
      default:
         break;
   }/*switch(*ptr)*/

   /*Determine the width of a decimal number:*/
   do{
      switch(ptr[i]){
         case '0':
         case '1':
         case '2':
         case '3':
         case '4':
         case '5':
         case '6':
         case '7':
         case '8':
         case '9':
            i++;
            break;
         default:
            n=0;
      }/*switch(ptr[i])*/
   }while(n);

   /* Now i is a width of a decimal number*/

   if(i==0){/*Not a decimal number!*/
      *hexbuf='\0';/*hexbuf[0] may be '-'!*/
      return hexbuf;
   }/*if(i==0)*/

   /*Construct in n the integer:*/
   for(p=1,n=0,i--; i>-1;i--,p*=10)switch(ptr[i]){
      case '0':break;
      case '1':n+=p;break;
      case '2':n+=p*2;break;
      case '3':n+=p*3;break;
      case '4':n+=p*4;break;
      case '5':n+=p*5;break;
      case '6':n+=p*6;break;
      case '7':n+=p*7;break;
      case '8':n+=p*8;break;
      case '9':n+=p*9;break;
      default:break;
   }/*for(p=1;n=0,i--; i>-1;i--,p*=10)switch(b[i])*/

   if(w<1)
      w=hexwide(n);

   b[w]='\0';

   /*Convert the integer into a hexidecimal string representation:*/
   for (w--;w > -1;w--){
      b[w]=integers[n%16];
      n/=16;
   }/*for (w--;w > -1;w--)*/

   return hexbuf;
}/*dec2hex*/

/*converts long to a string representation and returns it:*/
char *long2str(char *buf, long n)
{
char tmp[22];/* This is a stack. 64/Log_2[10] = 19.3, so this is enough forever...*/
unsigned long u;
register char *bufptr=buf, *tmpptr=tmp+1;/*tmp[0] is a terminator ('\0')*/

   if(n<0){/*Swap the sign and store '-':*/
      /*INT_MIN workaround:*/
      u = ((unsigned long)(-(1+n))) + 1;
      *bufptr++='-';
   }else
       u=n;

   *tmp='\0';/*Set terminator*/

   /*Fill up the stack:*/
   do
     *tmpptr++=integers[u%10];
   while( (u=u/10)!=0 );

   /*Copy the stack to the output buffer:*/
   while( (*bufptr++ = *--tmpptr)!='\0' );
   return buf;
}/*long2str*/

/*converts long to a string representation, allocaltes a buffer and returns it:*/
char *new_long2str(long n)
{
char tmp[22];
   return new_str(long2str(tmp,n));
}/*new_long2str*/

int firstPrimes[]={
3,5,7,11,13,17,19,23,29,
31,37,41,43,47,53,59,61,67,71,
73,79,83,89,97,101,103,107,109,113,
127,131,137,139,149,151,157,163,167,173,
179,181,191,193,197,199,211,223,227,229,
233,239,241,251,257,263,269,271,277,281,
283,293,307,311,313,317,331,337,347,349,
353,359,367,373,379,383,389,397,401,409,
419,421,431,433,439,443,449,457,461,463,
467,479,487,491,499,503,509,521,523,541,
547,557,563,569,571,577,587,593,599,601,
607,613,617,619,631,641,643,647,653,659,
661,673,677,683,691,701,709,719,727,733,
739,743,751,757,761,769,773,787,797,809,
811,821,823,827,829,839,853,857,859,863,
877,881,883,887,907,911,919,929,937,941,
947,953,967,971,977,983,991,997,1009
};

int NumberOfFirstPrimes=sizeof (firstPrimes)/sizeof(int);

/*The following function returns the smallest prime number >= n:*/
long nextPrime(long n)
{
   long j,q;
   int LastFirstPrime=firstPrimes[sizeof (firstPrimes)/sizeof(int)-1];

   if(n<3) return 3;

   if( !(n%2) )n++;
   for(;;n+=2){
      q=sqrt((double)n);
      if( q >= LastFirstPrime){/*Too large number. Usually we are not interested in
                              such a huge primes so this inefficient algorithm is used: */
         for(j=3;!(j>q);j+=2)
            if(!(n%j))break;
         if(j>q)break;
      }else{
        for(j=0;firstPrimes[j]<=q; j++)
           if(!(n%firstPrimes[j]) )break;
        if(firstPrimes[j]>q) break;
      }
   }
   return n;
}/*nextPrime*/

/*Returns number of chars remained for buf, or -1 if overflow*/
int getindexes(char *buf, int *moment, int maxLen,int outlen)
{
int totN=0,
    k,
    i;

char *vptr,
     *bptr=buf,
     *l=buf;/*l will be used as an output length counter, bptr-l*/

   if(*buf!='\0'){
      totN++;
      for(;*bptr!='\0';bptr++){
         if(--maxLen<=0){*bptr='\0';return -1; }
         switch(*bptr){
            case ',' :totN++;break;
            case '\n':l=bptr+1;break;
         }/*switch(*bptr)*/
      }/*for(;*bptr!='\0';bptr++)*/
   }/*if(*buf!='\0')*/

   for (i=1;i<=(*moment);i++){/*Loop on all momenta*/
      vptr=vec_group[abs(moment[i])].vec;/*set pointer to the momentum*/
      for(bptr=buf,k=0; k<totN; k++){/*Check if this momentum is already present:*/
         if(s_gcmp(bptr, vptr,','))break;/*Councide! We need not this momentum*/
         /*Note, the following costruction requires '\0' is kept while ',' is skipped:*/
         for(;*bptr!='\0';bptr++)if(*bptr==','){
               bptr++; break;
         }/*for(;*bptr!='\0';bptr++)if(*bptr==',')*/

      }/*for(j=0;k=0; k<totN; k++)*/
      if(k!=totN)/*This momentum is present already.*/
         break;
      if(totN){/*Not first, add a comma:*/
         int dl=0;
         if(--maxLen<=0)return -1;/* */
         *bptr++=',';
         if(*vptr!='\0'){ for(; vptr[dl]!='\0';dl++);/*dl=len+1 -- just +1 for comma*/}

         if( (bptr-l)+dl>=outlen ){/*Output length is expiried -- add a new line*/
            if(--maxLen<=0)return -1;/* */
            *bptr++='\n';
            l=bptr;/*reset an output length counter*/
         }/*if( (bptr-l)>=outlen )*/
      }/*if(totN)*/

      while(--maxLen > 0)if( (*bptr++=*vptr++)=='\0' )break;

      if( (maxLen==0)&&(*vptr=='\0') ){/*Room exactly fits*/
         *bptr='\0';return 0;
      }/*if( (maxLen==0)&&(*vptr=='\0') )*/

      if(maxLen<=0){/*Overflow*/
         *bptr='\0';return -1;
      }/*if(maxLen<=0)*/

      totN++;

   }/*for (; i>0;i--)*/

   return maxLen;
}/*getindexes*/

extern tt_table_type **tt_table;

void read_topology_tables(void)
{
char *fn=g_ttnames;
char *term=g_ttnames;
char *msgHeader=NULL;
/*fn- begin term - end of current file name.
  If the current file name is started with '\1', this file required only for
  momenta, if '\2', only for shape, otherwise, both.*/
int thechoice=0;/*thechoice==1 - momenta table, thechoice==-1 -- shape table*/
   do switch(*term){
      case ':':
         *term++='\0';
         /*no break!*/
      case '\0':
         if(fn == term)
            break;/*Ignore empty strings*/
         switch(*fn){
            case '\1':thechoice=1;fn++;msgHeader=MOMENTA;break;/*momenta table*/
            case '\2':thechoice=-1;fn++;msgHeader=SHAPE;break;/*shape table*/
            default:thechoice=0;msgHeader=TABLE;break;/*default mode, both shape and momenta*/
         }/*switch(*fn)*/
         if(fn == term)
            break;/*Ignore empty strings*/
         {/*block*/
            int tbl=0;
            tt_table_type *t;
            g_tt_try_loaded++;

            {/*block*/
               /*Check is the file exist, since sc_init will halt the system, when fail*/
               FILE *tryopen=open_file_follow_system_path(fn, "rt");
               if(tryopen!=NULL){ /*Ok, may try to load a table*/
                  fclose(tryopen);
                  tbl=tt_loadTable(fn);
               }/*if(tryopen!=NULL)*/
               /*else -- the file is absent, tbl==0*/
            }/*block*/

            if(tbl<=0){/*The table was not loaded*/
               char *buf=get_mem(MAX_STR_LEN,sizeof(char));
               message(FAIL_LOADING_TABLE,fn,tt_error_txt(buf,tbl));
               free(buf);
               fn=term;
               break;
            }/*if(tbl<=0)*/
            /*Here tbl>0!*/

            t=tt_table[tbl-1];

            /* Table thechoice and t->momentaOrShape:
                 | -1 |  0 |  1
                -|----|----|---
               -1| -1 | -1 |  x
                -|----|----|---
                0| -1 |  0 |  1
                -|----|----|---
                1|  x |  1 |  1
             */
            /* unload case 'x' since momenta/shape types not compatible with required:*/
            if( thechoice*(t->momentaOrShape)<0 ){
               message(TABLE_NOT_LOADED,fn,NOTHINGTOLOAD);
               tt_deleteTable(tbl);
               fn=term;
               break;
            }/*if( thechoice*(t->momentaOrShape)<0 )*/

            /*Now the sign of thechoice+(t->momentaOrShape) must be assigned to
              t->momentaOrShape:*/
            if( (thechoice=thechoice+(t->momentaOrShape))!=0 )
               thechoice/=abs(thechoice);
            t->momentaOrShape=thechoice;

            if (t->totalN == t->extN){
               if(!(g_tt_top_full<g_tt_max_top_full))/*possible expansion*/
                  if((g_tt_full=(int *)realloc(g_tt_full,
                     (g_tt_max_top_full+=DELTA_TT_TBL)*sizeof(int*)))==NULL)
                        halt(NOTMEMORY,NULL);
               g_tt_full[g_tt_top_full++]=tbl;
            }else{/*if (t->totalN == t->extN)*/
               if(t->totalN != t->intN){/*Table of a mixed type*/
                  message(FAIL_LOADING_TABLE,fn,FAIL_LOADING_MIXED_TABLE);
                  tt_deleteTable(tbl);
                  fn=term;
                  break;
               }/*if(t->totalN != t->intN)*/
               if(!(g_tt_top_int<g_tt_max_top_int))/*possible expansion*/
                  if((g_tt_int=(int *)realloc(g_tt_int,
                     (g_tt_max_top_int+=DELTA_TT_TBL)*sizeof(int*)))==NULL)
                        halt(NOTMEMORY,NULL);
               g_tt_int[g_tt_top_int++]=tbl;
            }/*if (t->totalN == t->extN)...else...*/
            message(TBLLOADED,msgHeader,fn,tbl);
         }/*block*/
         fn=term;/*Reset the name*/
         g_tt_loaded++;/*Increase number of loaded tables*/
         break;
      case '\\':/* Just skip a charachter*/
         if(*++term == '\0'){/*Nonsense! The string is finished by bare '\'!*/
            *--term='\0';/*To avoid crash replace it by '\0'.*/
            if(term!=fn)term--;/*The string will be processed at the next step*/
            /*else -- ignore empty line.*/
            break;
         }/*if(*++term == '\0')*/
         /*No break!*/
      default:
         term++;
         break;
   }while(*term!='\0');/*do switch(*term)*/
}/*read_topology_tables*/

char *tt_error_txt(char *arg,int i)
{
  *arg='\0';
  switch(i){
     case 0:/*Fail open file, not TT code.*/
        return s_let(CANNOTOPENTT,arg);
     /*Error codes:*/
     case TT_NONFATAL:/*Unspecified non-fatal error:*/
        return s_let(TT_NONFATAL_TXT,arg);
     case TT_FATAL:/*Unspecified fatal error:*/
        return s_let(TT_FATAL_TXT,arg);
     case TT_NOTMEM:/*Not enought memory:*/
        return s_let(NOTMEMORY,arg);
     case TT_INVALID:/*Invalid topology*/
        return s_let(TT_INVALID_TXT,arg);
     case TT_NOTFOUND:/*Not found by lookup:*/
        return s_let(TT_NOTFOUND_TXT,arg);
     /*Removed:
       Attempt to change maximal number of topologies during loading a table:
      *case TT_DBL_MAXTOP:
      *  return s_let(TT_DBL_MAXTOP_TXT,arg);
      */

     case TT_U_EOF:/*Unexpected end of file:*/
        return s_let(UNEXPECTEDEOF,arg);
     case TT_TOOMANYTOPS:/*Too many topologies:*/
        return s_let(TT_TOOMANYTOPS_TXT,arg);
     case TT_DOUBLEDEFNAM:/*Double defined topology name*/
        return s_let(TT_DOUBLEDEFNAM_TXT,arg);
     case TT_FORMAT:/*Token conversion error (e.g. non-number instead of an expected  number) */
        return s_let(TT_FORMAT_TXT,arg);
     case TT_EMPTYTABLE:/*Empty tables not allowed:*/
        return s_let(TT_EMPTYTABLE_TXT,arg);
     case TT_CANTINITSCAN:/*Can't initialize a scanner:*/
        return s_let(TT_CANTINITSCAN_TXT,arg);
     case TT_SCANNER_NOTINIT:/*Call getToken without initializing of a scanner:*/
        return s_let(TT_SCANNER_NOTINIT_TXT,arg);
     case TT_DOUBLETOPS:/*Two topologies in the table are coincide after reduction:*/
        return s_let(TT_DOUBLETOPS_TXT,arg);
     case TT_TOOMANYMOMS:/*Too many momenta sets:*/
        return s_let(TOOMANYMOMENTASETS,arg);
     case TT_TOOLONGSTRING:/*Too long string:*/
        return s_let(TOOLONGSTRING,arg);
     case TT_DOUBLEDEFTOKEN:/*Double defined translation token:*/
        return s_let(TT_DOUBLEDEFTOKEN_TXT,arg);
     case TT_TOPUNDEF:/*Undefined topology name:*/
        return s_let(TT_TOPUNDEF_TXT,arg);
     case TT_INVALIDSUBS:/*Invalid substitution:*/
        return s_let(INVALIDREORDERINGSTRING,arg);
     /*Error codes returned by readTopolRemarksFromScanner:*/
     case TT_REM_DOUBLEDEF:/*Double defined name:*/
        return s_let(TT_REM_DOUBLEDEF_TXT,arg);
     case TT_REM_EMPTYNAME:/*Empty name is not allowed:*/
        return s_let(TT_REM_EMPTYNAME_TXT,arg);
     case TT_REM_UNEXPECTED_EQ:/*Unexpected '=':*/
        return s_let(TT_REM_UNEXPECTED_EQ_TXT,arg);
     case TT_CANNOT_WRITE:/*Can't write to disk:*/
        return s_let(TT_CANNOT_WRITE_TXT,arg);
  }/*switch(i)*/
  /*Not an error code*/
  return arg;
}/*tt_error_txt*/

void checkTT(int ret, char *msg)
{
   if(ret<0){
      char tmp[MAX_STR_LEN];
      tt_error_txt(tmp, ret);
      halt(msg,tmp);
   }
}/*checkTT*/

void make_canonical_topology(void)
{
int i;
     *vsubst=*lsubst=*vtsubst=*ltsubst=-1;
     *ldir=-1;
/*       s_let(l_dir,ldir);*/

     for(i=1;v_subst[i]!='\0';i++)vsubst[i]=i;vsubst[i]='\0';
     for(i=1;l_subst[i]!='\0';i++){lsubst[i]=i;ldir[i]='\1';}

     lsubst[i]=ldir[i]='\0';
     for(i=1;l_subst[i]!='\0';i++){lsubst[i]=i;}
     lsubst[i]='\0';

     canonical_topology = &buf_canonical_topology;

     reduce_internal_topology(topologies[cn_topol].topology,
                               canonical_topology,lsubst,vsubst, ldir);
     internal_canonical_topology=&buf_internal_canonical_topology;

     top_let(canonical_topology,internal_canonical_topology)->e_n=0;

     /*See "The substituions map" in "variables.h"*/
     invertsubstitution(ltsubst,lsubst);
     invertsubstitution(vtsubst,vsubst);
}/*make_canonical_topology*/

/*Saves translated wrk table (if present) into a named file.
  Returns number of saved topologies:*/
long save_wrk_table(char *fname)
{
long n, tN;
int wrkInd=g_tt_wrk-1;
FILE *outf=NULL;
REMARK *remarks;/*Array of remarks*/
word top_remarks;/*Number of allocated cells.*/

   if(
      (g_tt_wrk<=0)||/*No table*/
      ( (tN=tt_table[wrkInd]->totalN)<=0)/* Empty table*/
   )
    return 0;

   if(  (outf=fopen(fname,"w"))==NULL  )
      halt(CANNOTWRITETOFILE,fname);

/*May be useful in future:*/
#ifdef SKIP
   if(g_wrk_counter>0){/*Some of topologies in wrk table are used as momenta source*/
      tt_table_type *tbl=tt_table[g_tt_wrk-1];

      /*Allocate a bit array to mark used for momenta topologies:*/
      g_wrkmask_size=g_wrk_counter/250 + 1;
      if(g_wrkmask!=NULL)free(g_wrkmask);
      g_wrkmask=get_mem(g_wrkmask_size,sizeof(set_of_char));

      /*Mark used topologies:*/
      /*Note, everithing is 0 due to 'get_mem' is equivalent to 'calloc'*/
      for(n=(tbl->totalN)-1;n>=0;n--)if(  (tbl->topols)[n]->duty > 0 )/*It is used*/
         set_set((n % 250),g_wrkmask[n / 250]);/*Mark it*/
   }/*if(g_wrk_counter>0)*/
#endif

   /*Clear remarks-- we need not them anymore:*/
   for(n=0; n<tN; n++){
      remarks=(tt_table[wrkInd]->topols)[n]->remarks;
      top_remarks=(tt_table[wrkInd]->topols)[n]->top_remarks;
      deleteTopolRem(NULL,top_remarks,remarks,"WCtbl");
      deleteTopolRem(NULL,top_remarks,remarks,"WCind");
      deleteTopolRem(NULL,top_remarks,remarks,"WMtbl");
      deleteTopolRem(NULL,top_remarks,remarks,"WMind");
   }/*for(n=0; n<tN; n++)*/
   n=tt_saveTableToFile(outf,g_tt_wrk,0);/*last 0 means nusr translated*/
   fclose(outf);
   if (n<=0)remove(fname);
   return n;
}/*save_wrk_table*/

/*obsolete:*/
#ifdef SKIP
/*The following function will be invoked by tt_loadTable loading each token
 can be a vector:*/
static void l_newVec(long n, char *vec)/*n is the ttopology index, not used here*/
{
   if(!set_in(*vec,g_regchars))return;/*Not a vector*/
   if (!set_in(n % 250,g_wrkmask[n / 250]))
      return;/*This topology was not used as  a momenta source*/

   /*Now 'vec' is a vector id used in a topology used as a momenta source. Store it:*/
   vec=new_str(vec);
   install(vec,vec,g_wrk_vectors_table);
}/*l_newVec*/
#endif

/*Loads wrk table from a file. If wrk table exists, deletes it. All
  loaded vectors are stored into g_wrk_vectors_table. Returns number of
  loaded topologies:*/
int load_wrk_table(char *fname)
{
#ifdef SKIP
/*About tt_newVector -- see tt_loadTable*/
void (*mem)(long n, char *vec)=tt_newVector;/*Save old value*/
#endif
int old_tt_wrk=g_tt_wrk;/*save id of working table*/

   /*obsolete:*/
#ifdef SKIP
   if(g_wrk_vectors_table == NULL)
      g_wrk_vectors_table=create_hash_table(vectors_hash_size,
                                             str_hash,str_cmp,c_destructor);

   if(g_wrkmask!=NULL)
      tt_newVector=&l_newVec;/*Learn tt_loadTable saving vectors into g_wrk_vectors_table*/
#endif

   g_tt_wrk=tt_loadTable(fname);

#ifdef SKIP
   tt_newVector=mem;/*restore old value*/
#endif
   if(g_tt_wrk<0){/*Error*/
      int n=g_tt_wrk;
      g_tt_wrk=old_tt_wrk;/*Restore stored id*/
      return n;
   }/*if(g_tt_wrk<0)*/

   /*Now override all translations. IMPORTANT! Topology editor may destroy translations!*/
   {/*block*/
     HASH_TABLE
        newtrans=tt_table[g_tt_wrk-1]->htrans,
        oldtrans=tt_table[old_tt_wrk-1]->htrans;
        if(   (newtrans!=NULL) && (oldtrans!=NULL)   )
           binary_operations_under_hash_table(
              oldtrans,/*"from"*/
              newtrans,/*"to"*/
              -1);/*see "hash.c"; op>0 -> add enties to "to" from "from", only if they
                 are absent in "to", op<0 -> rewrite entries in "to" by the corresponding
                 "from", op==0 -> remove all entries in "to" matching "from"*/
   }/*block*/

   tt_deleteTable(old_tt_wrk);/*Destroy previous wrk table*/

#ifdef SKIP
   /*Grab remarks from old table -- this is a stub!:*/
   if( (old_tt_wrk>0)&&(tt_table[g_tt_wrk-1]->totalN==tt_table[old_tt_wrk-1]->totalN)  ){
      long i;
      tt_singletopol_type *tfrom,*tto;
      old_tt_wrk--;
      g_tt_wrk--;
      for(i=0; i<tt_table[g_tt_wrk]->totalN; i++){

         tfrom=tt_table[old_tt_wrk]->topols[i];
         tto=tt_table[g_tt_wrk]->topols[i];

         copyTopolRem(
             tfrom->top_remarks,
             tfrom->remarks,
             &(tto->top_remarks),
             &(tto->remarks)
         );
      }/*for(i=0; i<tt_table[g_tt_wrk]->totalN; i++)*/
      old_tt_wrk++;
      g_tt_wrk++;
      tt_deleteTable(old_tt_wrk);
   }/*if(old_tt_wrk>0)*/
#endif

#ifdef SKIP
   /*The table g_wrk_vectors_table is ready, so delete g_wrkmask:*/
   free_mem(&g_wrkmask);
   g_wrkmask_size=0;
#endif

   return tt_table[g_tt_wrk-1]->totalN;

}/*load_wrk_table*/

/*
   Parses a string. For each identifier (someting starting with regchars and consists of
   regchars or digits) it allocates a new string identical to this id and invokes
   int process_token(int n, char *thetoken).If the latter returns !=0, parse_tokens_id
   immediately returns the value returned by process_token. Arguments of process_token:
   n -- order number of token, thetoken -- the token. After all tokens are processed,
   parse_tokens_id returns the number of processed tokens.
*/
int parse_tokens_id(
                    set_of_char regchars,
                    set_of_char digits,
                    char *str,
                    PROCESS_TOKEN *process_token
                   )
{
char *b=str,*e=str,*t;
int n=0,l;
   if(str==NULL)return -1;
   while(*e!='\0'){
      /*Move 'b' to the begin of id:*/
      while(!set_in(*b,regchars))
         if( *b++=='\0' )/*No id's*/
            return n;
      /*Now b points to the begin of id*/
      n++;/*increase the counter*/
      e=b;
      /*Move 'e' after the end of id:*/
      while(set_in(*e,regchars)||set_in(*e,digits))/*body of id*/
         e++;
      /*Now e points just AFTER the id*/
      /*Note, we need NOT str anymore, so use it as a pointer to the token:*/
      str=t=get_mem(e-b+1,sizeof(char));/*Ok, sizeof(char)==1, so what?*/
      while(b!=e)*str++=*b++;/*Now b==e*/
      *str='\0';
      /*Now in 't' we have an allocated token. Invoke an iterator: */
      if( (l=process_token(n,t))!=0 )return l;
   }/*while(*e!='\0')*/
   return n;
}/*parse_tokens_id*/

word *newword( word w)
{
word *tmp=get_mem(1,sizeof(word));
   *tmp=w;
   return tmp;
}/*newword*/

char *checkTopMomentaBalance(char *returnedbuf,aTOPOL *topols,word cn_top,int cn_set)
{
int  **emom,i,isAlloc=0;
char *is_ingoing;

   if( (topols[cn_top].ext_momenta!=NULL)&&
          (topols[cn_top].ext_momenta[cn_set]!=NULL) )
         emom=topols[cn_top].ext_momenta[cn_set];
   else{
      isAlloc=1;
      emom=get_mem(ext_lines+1,sizeof(int*));
      for(i=1; i<=ext_lines; i++)
         /*emom[i]=ext_particles[ext2my[i]].momentum;*/
         emom[i]=ext_particles[i-1].momentum;
   }/*if*/

   *(is_ingoing=get_mem(ext_lines+2,sizeof(char)))=-1;
   for(i=1; i<=ext_lines; i++)
      is_ingoing[i]=ext_particles[i-1].is_ingoing;
   is_ingoing[i]='\0';

   /*See topology.c:*/
   check_momenta_balance(
      returnedbuf,
      (topols[cn_top].topology)->e_line,
      (topols[cn_top].topology)->i_line,
      (topols[cn_top].topology)->e_n,
      (topols[cn_top].topology)->i_n,
      topols[cn_top].max_vertex,
      is_ingoing,
      g_zeromomentum,
      emom,
      topols[cn_top].momenta[cn_set]
   );
   if(isAlloc) free(emom);
   free(is_ingoing);
   return returnedbuf;
}/*checkTopMomentaBalance*/

#define MAX_PARSE_DEEP 64
#define MAX_DOUBLE_LEN 64

#define NEG "neg"
#define DUP "dup"
#define SETRGBCOLOR "setrgbcolor"
#define CURRENTPOINT "currentpoint"
#define CURRENTRGBCOLOR "currentrgbcolor"
#define CURRENTFONT "currentfont"
#define SETFONT "setfont"
#define FINDFONT "findfont"
#define SCALEFONT "scalefont"
#define EXCH "exch"
#define POP "pop"
#define MOVETO "moveto"
#define RMOVETO "rmoveto"
#define DIV "div"
#define MUL "mul"

#define BEGIN_STR 1
#define END_STATE 2
#define END_STR 3
#define AFTER_OPEN_BRACE 4
#define ESC_ESC 5
#define ESC_X 6
#define ESC_Y 7
#define ESC_XY 8
#define ESC_C 9
#define ESC_F 10
#define ESC_S 11
#define CONTINUE_STR 12

/*Parses ?(param). Allocates and returns "param" and shifts *ptr to ')'
  Returns NULL at fail, *ptr is shifted at the problem place:*/
static char *l_getParam(char **ptr)
{
char *tmp;
  if( *(++(*ptr))!='('  )return NULL;

  if( *(++(*ptr)) == ')' )return NULL;
  tmp=new_str("");
  while( (**ptr)!=')' ){
     if((**ptr)=='\0'){ free(tmp);return NULL; }
     tmp=s_addchar(tmp,*(*ptr));
     (*ptr)++;
  }/*while( (*ptr)!=')' )*/
  return tmp;
}/*l_getParam*/

/* Invokes l_getParam and converts returned value to double. If fail, returns NULL.
   Input (*ptr) must NOT contain trailing non-digits!
 */
static char *l_getParamDig(char **ptr, double *dig)
{
char *mem=(*ptr)+2;
char *tmp=l_getParam(ptr);
char *buf;
   if(tmp==NULL)return NULL;
   if( s_len(tmp)>  MAX_DOUBLE_LEN-2 ){
      free(tmp);return NULL;
   }/*if( s_len(tmp)>  MAX_DOUBLE_LEN-2 )*/
   *dig=strtod(tmp,&buf);
   if( (*buf) !='\0' ){
      *ptr=mem;
      free(tmp);return NULL;
   }/*if( (*buf) !='\0' )*/
   return tmp;
}/*l_getParamDig*/

/*
   converts Diana-specific string into a PS form:
   e.g.:
   W{y(10){f(Symbol)(10)a}}  -> $W^\alpha$ ( in TeX notations!  Actually will be outputted
   in PS notations.)

   buf is input, ret is output.
   fonts is an array of non-standart fonts occured ( i entry is (*fonts)[i] ,
   accounted from 0, last entry is NULL)

   ATTENTION! fonts is allocated! On error it is cleared.

   ATTENTION! ret is allocated! On error, ret is cleared, and then it is set up
   to a static string with a diagnostic.

   ATTENTION! '(' and ')' are ALWAYS escaped!

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
   Returns >=0 ("complexity") - the number of non-trivial blocks.
   If the retirned value <0, then the  parsing error occured and -value is the number of
   a problem character (starting from 0).
 */
int parse_particle_image(char *fsize,char *multiplier,char *show, char *buf,char **ret,char ***fonts)
{
int state,
    st2,/*auxiliary state*/
    sp=0,/*wrk cell -  starting paramter index for parsing a digit*/
    np=0,/*wrk cell - number of parameters*/
    nspec=0;/*returned value*/
/*At present, not used actually, only to sheck conversion:*/
double tmp[3]={0.0,0.0,0.0};
char *ptr=buf, *p[3]={NULL,NULL,NULL};/*Parameters*/
/*Each time block is started, the proper small integer is added to the stack.
  When block is finished, this info is used to restore the state:*/
char stack[MAX_PARSE_DEEP];
int top_stack=0;
/*wrk table to determine uniqueness of non-standart fonts:*/
HASH_TABLE hfonts=NULL;
/*counter of non-standart fonts:*/
int nfonts=0;
   *ret=new_str("");
   for(state=BEGIN_STR;state!=END_STATE;ptr++)switch(state){
      case BEGIN_STR:
         switch(*ptr){
            case '\0':state=END_STATE; break;
            case '}':state=END_STR; break;
            case '{':state=AFTER_OPEN_BRACE; break;
            default:
               *ret=s_inc(*ret," (");
               switch(*ptr){
                  case '\\':state=ESC_ESC;break;
                  case '(':case ')':
                     /*Escape '(' ')':*/
                     *ret=s_addchar(*ret,'\\');/*No break*/
                  default:*ret=s_addchar(*ret,*ptr);state=CONTINUE_STR;break;
               }/*switch(*ptr)*/
               break;
         }/*switch(*ptr)*/
         break;
      case ESC_ESC:/*Escaped character - just add it as is*/
         if(*ptr == '\0'){/*Escaped end of parsed string!*/
            nspec=ptr-buf+1;
            buf=UNEXPECTED_EOL;
            goto fail_return;
         }/*if(*ptr == '\0')*/
         *ret=s_addchar(*ret,*ptr);state=CONTINUE_STR;break;
      case AFTER_OPEN_BRACE:
         st2=0;sp=0;
         switch(*ptr){
            case 'x':if(ptr[1]=='y'){ptr++;st2=ESC_XY;np=2;}else{st2=ESC_X;np=1;}break;
            case 'y':st2=ESC_Y;np=1;break;
            case 'c':st2=ESC_C;np=3;break;
            case 's':st2=ESC_S;np=1;break;
            case 'f':
               /*Get a font name - the only non-numerical parameter:*/
               if(  (p[0]=l_getParam(&ptr))==NULL){
                  nspec=ptr-buf+1;
                  buf=FONTNAMEEXPECTED;
                  goto fail_return;
               }/*if(  (p[0]=l_getParam(&ptr))==NULL)*/
               if(hfonts==NULL)
                  hfonts=create_hash_table(17,str_hash,str_cmp,c_destructor);
               {/*block*/
                  char *f=new_str(p[0]);
                  if(!install(f,f,hfonts)){ /*new font*/
                     if(*fonts == NULL)
                        *fonts = get_mem(2,sizeof(char*));
                     else{
                       if(
                           (*fonts = realloc(*fonts,(nfonts+2)*sizeof(char*)))
                           ==NULL
                       )halt(NOTMEMORY,NULL);
                     }
                     (*fonts)[nfonts]=new_str(p[0]);
                     nfonts++;
                     (*fonts)[nfonts]=NULL;
                  }/*if(!install(f,f,hfonts))*/
               }/*block*/
               st2=ESC_F;np=2;sp=1;
               break;
            default:
                  nspec=ptr-buf+1;
                  buf=INVALIDBRACE;
                  goto fail_return;
         }/*switch(*ptr)*/
         /*Now sp2 is set to a proper block type*/
         {/*block*/
            int i,j;
            if(top_stack>=MAX_PARSE_DEEP){
               nspec=ptr-buf+1;/*Stack overflow*/
               buf=STACK_OVERFLOW;
               goto fail_return;
            }/*if(top_stack>=MAX_PARSE_DEEP)*/
            nspec++;/*New block was found  so increment the block counter*/
            stack[top_stack++]=st2;/*And save its type to the stack*/
            /*Obtain parameters (starting from 'sp' since for  the 'f' type we already have
              the first parameter, a font name):*/
            for(i=sp; i<np;i++){
               if(  (p[i]=l_getParamDig(&ptr,tmp+i))==NULL ){
                  for(j=i-1;j>=0;j--)free(p[i]);
                  nspec=ptr-buf+1;
                  buf=NUMEXPECTED;
                  goto fail_return;
               }/*if(  (p[i]=l_getParam(&ptr))==NULL )*/
            }/*for(i=0; i<np;i++)*/
            state=st2;
            ptr--;/*since auto incrementing ptr*/
         }/*block*/
         break;
      case ESC_X:
         /*{x(#) .... }=> ' currentpoint pop fsize # mul multiplier div 0 rmoveto':*/
         *ret=s_inc(*ret," "CURRENTPOINT" "POP" ");
         *ret=s_inc(*ret,fsize);
         *ret=s_addchar(*ret,' ');
         *ret=s_inc(*ret,p[0]);
         *ret=s_inc(*ret," "MUL" ");
         *ret=s_inc(*ret,multiplier);
         *ret=s_inc(*ret," "DIV" 0 "RMOVETO);
         free_mem(&(p[0]));
         state=BEGIN_STR;
         break;
      case ESC_Y:
         /*{y(#) .... }=> ' currentpoint exch pop 0 fsize # mul multiplier div rmoveto':*/
         *ret=s_inc(*ret," "CURRENTPOINT" "EXCH" "POP" 0 ");
         *ret=s_inc(*ret,fsize);
         *ret=s_addchar(*ret,' ');
         *ret=s_inc(*ret,p[0]);
         *ret=s_inc(*ret," "NEG" "MUL" ");
         *ret=s_inc(*ret,multiplier);
         *ret=s_inc(*ret," "DIV" "RMOVETO);
         free_mem(&(p[0]));
         state=BEGIN_STR;
         break;
      case ESC_XY:/* {xy(#1)(#2) .... }=>
         ' currentpoint fsize #1 mul multiplier div fsize #2 mul multiplier div rmoveto':*/
         *ret=s_inc(*ret," "CURRENTPOINT" ");
         *ret=s_inc(*ret,fsize);
         *ret=s_addchar(*ret,' ');
         *ret=s_inc(*ret,p[0]);
         *ret=s_inc(*ret," "MUL" ");
         *ret=s_inc(*ret,multiplier);
         *ret=s_inc(*ret," "DIV" ");
         *ret=s_inc(*ret,fsize);
         *ret=s_addchar(*ret,' ');
         *ret=s_inc(*ret,p[1]);
         *ret=s_inc(*ret," "NEG" "MUL" ");
         *ret=s_inc(*ret,multiplier);
         *ret=s_inc(*ret," "DIV" "RMOVETO);
         free_mem(&(p[0]));
         free_mem(&(p[1]));
         state=BEGIN_STR;
         break;
      case ESC_C:
         /*{c(#1)(#2)(#3) ... }=> 'currentcolor #1 #2 #3 setrgbcolor'*/
         *ret=s_inc(*ret," "CURRENTRGBCOLOR" ");
         *ret=s_inc(*ret,p[0]);
         *ret=s_addchar(*ret,' ');
         *ret=s_inc(*ret,p[1]);
         *ret=s_addchar(*ret,' ');
         *ret=s_inc(*ret,p[2]);
         *ret=s_inc(*ret," "SETRGBCOLOR);
         free_mem(&(p[0]));
         free_mem(&(p[1]));
         free_mem(&(p[2]));
         state=BEGIN_STR;
         break;
      case ESC_F:
         /*{f(fontname)(#) ... }=>
           'currentfont /fontname findfont fsize # mul multiplier div scalefont setfont'*/
         *ret=s_inc(*ret," "CURRENTFONT" /");
         *ret=s_inc(*ret,p[0]);
         *ret=s_inc(*ret," "FINDFONT" ");
         *ret=s_inc(*ret,fsize);
         *ret=s_addchar(*ret,' ');
         *ret=s_inc(*ret,p[1]);
         *ret=s_inc(*ret," "MUL" ");
         *ret=s_inc(*ret,multiplier);
         *ret=s_inc(*ret," "DIV" "SCALEFONT" "SETFONT);
         free_mem(&(p[0]));
         free_mem(&(p[1]));
         sp=0;
         state=BEGIN_STR;
         break;
      case ESC_S:
         /* {s(#)...} =>'currentfont dup # multiplier div scalefont setfont'*/
         *ret=s_inc(*ret," "CURRENTFONT" "DUP" ");
         *ret=s_inc(*ret,p[0]);
         *ret=s_addchar(*ret,' ');
         *ret=s_inc(*ret,multiplier);
         *ret=s_inc(*ret," "DIV" "SCALEFONT" "SETFONT);
         free_mem(&(p[0]));
         state=BEGIN_STR;
         break;
      case CONTINUE_STR:
         switch(*ptr){
            case '\0':
               if(top_stack){
                  nspec=ptr-buf+1;
                  buf=UNCLOSEDBLOCK;
                  goto fail_return;
               }/*if(top_stack)*/
               *ret=s_inc(*ret,") ");
               *ret=s_inc(*ret,show);
               state=END_STATE;
               break;
            case '}':
               /*Finish the string:*/
               *ret=s_inc(*ret,") ");
               *ret=s_inc(*ret,show);
               state=END_STR;
               break;
            case '\\':state=ESC_ESC;break;
            case '{':
               *ret=s_inc(*ret,") ");
               *ret=s_inc(*ret,show);
               state=AFTER_OPEN_BRACE;
               break;
            case '(':case ')':*ret=s_addchar(*ret,'\\');/*No break*/
            default:*ret=s_addchar(*ret,*ptr);
         }/*switch(*ptr)*/
         break;
      case END_STR:
         if(!top_stack){
            nspec=ptr-buf;
            buf=STACK_UNDERFLOW;
            goto fail_return;
         }/*if(!top_stack)*/
         /*And now restore the saved status:*/
         switch(stack[--top_stack]){
            case ESC_X:
               *ret=s_inc(*ret," "CURRENTPOINT" "EXCH" "POP" "MOVETO);
               break;
            case ESC_Y:
               *ret=s_inc(*ret," "CURRENTPOINT" "POP" "EXCH" "MOVETO);
               break;
            case ESC_XY:
               *ret=s_inc(*ret," "MOVETO);
               break;
            case ESC_C:
               *ret=s_inc(*ret," "SETRGBCOLOR);
               break;
            case ESC_S:
            case ESC_F:
               *ret=s_inc(*ret," "SETFONT);
               break;
            default:
               nspec=ptr-buf+1;
               buf=INTERNALERROR;
               goto fail_return;
         }/*switch(stack[--top_stack])*/
         ptr--;/*since auto incrementing ptr*/
         state=BEGIN_STR;
         break;
   }/*for(state=INIT_STATE;state!=END_STATE;ptr++)switch(state)*/
   /*Stack must be empty!:*/
   if(top_stack){
      nspec=(ptr-buf);
      buf=UNCLOSEDBLOCK;
      goto fail_return;
   }/*if(top_stack)*/

   hash_table_done(hfonts);
   /*Return number of blocks found:*/
   return nspec;
   fail_return:
      free(*ret);/*It can't be NULL!*/
      (*ret)=buf;
      if((*fonts)!=NULL){
         for(state=0;(*fonts)[state]!=NULL;state++)
            free((*fonts)[state]);
         free_mem(fonts);
      }/*if((*fonts)!=NULL)*/
      hash_table_done(hfonts);
      return -nspec;
}/*parse_particle_image*/

/*Evaluates number of HEX digits needed to fit the argument:*/
int hexwide(unsigned long int j)
{
int i;
   for(i=0; j; j>>=1,i++);

   if(i==0)
      return 1;
   else if(i % 4 )
      return i/4+1;

   return i/4;
}/*hexwide*/

/*Wrapper to the read() syscall, to handle possible interrupts by unnblocked signals:*/
ssize_t read2b(int fd, char *buf, size_t count)
{
ssize_t res;

   if( (res=read(fd,buf,count)) <1 )/*EOF or read is interrupted by a signal?:*/
       while( (errno == EINTR)&&(res <1) )
          /*The call was interrupted by  a  signal  before  any data was read, try again:*/
          res=read(fd,buf,count);
   return res;
}/*read2b*/

/*Wrapper to the write()) syscall, to handle possible interrupts by unnblocked signals:*/
ssize_t writeFromb(int fd, char *buf, size_t count)
{
ssize_t res;

   if( (res=write(fd,buf,count)) <1 )/*Is write interrupted by a signal?:*/
       while( (errno == EINTR)&&(res <1) )
          /*The call was interrupted by a signal before any data was written, try again:*/
          res=write(fd,buf,count);
   return res;
}/*writeFromb*/

/*Reads exactly count bytes from the descriptor fd into buffer buf, independently on
  nonblocked signals and the MPU/buffer hits. Returns 0 or -1:
  */
int readexactly(int fd, char *buf, size_t count)
{
ssize_t i;
int j=0,n=0;

   for(;;){
      if(  (i=read2b(fd, buf+j, count-j)) < 0 ) return -1;
      j+=i;
      if(j==count) break;
/*!!!if(  (i==1) && (buf[j-1]< ' ')  )return -1;*/
      if(i==0)n++;
      else n=0;
      if(n>MAX_FAILS_IO)return -1;
   }/*for(;;)*/
   return 0;
}/*readexactly*/

/*Wtites exactly count bytes from the  buffer buf intop the descriptor fd, independently on
  nonblocked signals and the MPU/buffer hits. Returns 0 or -1:
*/
int writexactly(int fd, char *buf, size_t count)
{
ssize_t i;
int j=0,n=0;

   for(;;){
      if(  (i=writeFromb(fd, buf+j, count-j)) < 0 ) return -1;
      j+=i;
      if(j==count) break;
      if(i==0)n++;
      else n=0;
      if(n>MAX_FAILS_IO)return -1;
   }/*for(;;)*/
   return 0;
}/*writexactly*/

/*
   Reads from the file descriptor fd len bytes and tries to convert them into
   unsigned int (assuming they are in HEX format). In success, returns read value,
   on error, returns -1 and sends the message "cmd" of size cmdlen to the fd (if
   cmd!=NULL).

   ATTENTION!! Returns SIGNED int while converts from HEX to UNsigned! Take care
   possible overflows!
*/
long int readHex(int fd, char *buf, size_t len, char *cmd, size_t cmdlen)
{
   int ret;
      if(
            (   readexactly(fd,buf,len)<0   )
          ||(  (ret=hex2int(buf,len))<0  )
        ){
          if(cmd!=NULL)
             writexactly(fd,cmd,cmdlen);
         return -1;
      }/*if(   )*/
      return ret;
}/*readHex*/

/*The following function send the hexadecimal representation
  of an unsigned long integer n into a pipe fd atomically in such a form:
  XXxxxxxxx
  ^^the length
  i.e. dec. 15 will be sent as "01f".
  Problem with this function is that the operation must be atomic.
  For the size of the atomic IO operation, the  POSIX standard
  dictates 512 bytes. Linux PIPE_BUF is quite considerably, 4096.
  Anyway, in all systems we assume PIPE_BUF>20*/
int writeLong(int fd,unsigned long int n)
{
char buf[20];
int w=hexwide(n);
  int2hex(buf,w,2);
  int2hex(buf+2, n, w);
  return writexactly(fd, buf, w+2);
}/*writeLong*/

char *quote_char(char *buf, char ch)
{
   buf[0]='\\';
   if(ch<' ')
      sprintf(buf+1,"%2.2X",(unsigned char)ch);
   else{ 
      buf[1]=buf[2]='\0';
      switch(ch){
         case '\'':case '\\':case '<':case '>':
            buf[1]=ch;break;
         default:
           buf[0]=ch; 
      }/*switch(ch)*/
   }/*else*/
   return buf;
}/*quote_char*/
