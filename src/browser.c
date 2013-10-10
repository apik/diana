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
struct list_prototypes_struct{
   word diagram_number;
   word sort;
};

static struct list_prototypes_struct *list_prototypes=NULL;
static word top_list_prototype=0;
static word max_top_list_prototype=0;

static int cmpd_t(word a, word b)
{
  return(s_cmp(topologies[diagram[a].topology].id[diagram[a].n_id],
               topologies[diagram[b].topology].id[diagram[b].n_id]));
}/*cmpd_t*/

static int cmpd_n(word a, word b)
{
  return(diagram[a].number-diagram[b].number);
}/*cmpd_n*/

static int cmpd_p(word a, word b)
{
  return(s_cmp(prototypes[diagram[a].prototype],
               prototypes[diagram[b].prototype]));
}/*cmpd_p*/

static int all_compd(char k,word a,word b)
{
  switch(k){
     case TOPOLOGY:return(cmpd_t(a,b));
     case NUMBER:  return(cmpd_n(a,b));
     case PROTOTYPE:   return(cmpd_p(a,b));
  }
  return(0);
}/*all_compd*/

static int comparatord(word a, word b)
{
 char *cmp;
 int c;
   for(cmp=comp_arrange;*cmp;cmp++){
      if((c=all_compd(*cmp,a,b))!=0)return(c);
   }
 return(0);
}/*comparatord*/

static void sortd(word count)
{
 long gap,i,j;
 word tmp;

    for (gap=count/2;gap>0;gap/=2)
       for(i=gap;i<count;i++)
          for(j=i-gap;!(j<0);j-=gap){
             if(
                !(
                   comparatord(diagram[j].sort,diagram[j+gap].sort)
                 >0)
               )break;
             tmp=diagram[j].sort;
             diagram[j].sort=diagram[j+gap].sort;
             diagram[j+gap].sort=tmp;
          }
}/*sortd*/

static int cmpp_t(word a, word b)
{
return(s_cmp(
     topologies[diagram[list_prototypes[a].diagram_number].topology].id[
              diagram[list_prototypes[a].diagram_number].n_id
     ],
     topologies[diagram[list_prototypes[b].diagram_number].topology].id[
             diagram[list_prototypes[b].diagram_number].n_id
     ]));
}/*cmpp_t*/

static int cmpp_n(word a, word b)
{
  return(list_prototypes[a].diagram_number-list_prototypes[b].diagram_number);
}/*cmpp_n*/

static int cmpp_p(word a, word b)
{
  return(s_cmp(prototypes[diagram[list_prototypes[a].diagram_number
                                                               ].prototype],
               prototypes[diagram[list_prototypes[b].diagram_number
                                                               ].prototype]));
}/*cmpp_p*/

static int all_compp(char k,word a,word b)
{
  switch(k){
     case TOPOLOGY: return(cmpp_t(a,b));
     case NUMBER:   return(cmpp_n(a,b));
     case PROTOTYPE:return(cmpp_p(a,b));
  }
  return(0);
}/*all_compp*/

static int comparatorp(word a, word b)
{
 char *cmp;
 int c;
   for(cmp=comp_arrange;*cmp;cmp++){
      if((c=all_compp(*cmp,a,b))!=0)return(c);
   }
 return(0);
}/*comparatorp*/

static void sortp(void)
{
 long gap,i,j;
 word tmp;
    for (gap=top_list_prototype/2;gap>0;gap/=2)
       for(i=gap;i<top_list_prototype;i++)
          for(j=i-gap;!(j<0);j-=gap){
             if(
                !(
                   comparatorp(list_prototypes[j].sort,
                                                  list_prototypes[j+gap].sort)
                 >0)
               )break;
             tmp=list_prototypes[j].sort;
             list_prototypes[j].sort=list_prototypes[j+gap].sort;
             list_prototypes[j+gap].sort=tmp;
          }
}/*sortp*/

void out_browse_head(word count)
{
struct template_struct *tmp=NULL;

  if(browser_out_file==NULL)
     browser_out_file=open_file(browser_out_name, "w+");
  fprintf(browser_out_file,BROWSERHEAD1,
                 config_name,input_name,start_diagram,finish_diagram,count);

  fprintf(browser_out_file,BROWSERHEAD2a);
  tmp=template_t;
  do{
     fprintf(browser_out_file," %s",tmp->pattern);
  }while((tmp=tmp->next)!=NULL);
  fprintf(browser_out_file,";\n");

  fprintf(browser_out_file,BROWSERHEAD2b);
  tmp=template_p;
  do{
     fprintf(browser_out_file," %s",tmp->pattern);
  }while((tmp=tmp->next)!=NULL);
  fprintf(browser_out_file,".\n");

  fprintf(browser_out_file,BROWSERHEAD3,comp_d,comp_p);

}/*out_browse_head*/

void browser_done(void)
{
  close_file(&browser_out_file);
  free_mem(&list_prototypes);
  max_top_list_prototype=top_list_prototype=0;
}/*browser_done*/

void sort_diargams(word count)
{
   letnv(comp_d,comp_arrange,COMP_ARR_L);
   sortd(count);
}/*sort_diargams*/

void print_diagrams(word count)
{
word i;
char tmp[MAX_STR_LEN],*p;
   /*Use prototype_table to select unique prototype:*/
   if(prototype_table!=NULL)
      hash_table_done(prototype_table);

   prototype_table=create_hash_table(prototype_hash_size,
                                        str_hash,str_cmp,c_destructor);
   fprintf(browser_out_file,"\n\n");

   for(i=0;i<count;i++){
      fprintf(browser_out_file,OUTPUTDIAGRAMINFO,
        i+1,
        diagram[diagram[i].sort].number,
        topologies[diagram[diagram[i].sort].topology].id[
           diagram[diagram[i].sort].n_id
        ],
        prototypes[diagram[diagram[i].sort].prototype]
      );
      /* Store full prototype, if need:*/

      if(lookup(s_cat(tmp,
                          topologies[diagram[i].topology].id[
                            diagram[i].n_id
                          ],
                          prototypes[diagram[i].prototype]
                          ),
                prototype_table
        )==NULL){
/*!!! Why +1?:*/
            if (!(top_list_prototype+1<max_top_list_prototype))
              if(
                 (list_prototypes=realloc(list_prototypes,
                  (max_top_list_prototype+=DELTA_PROTOTYPES_LIST)
                                    *sizeof(struct list_prototypes_struct)))
                                                                       ==NULL
                   )halt(NOTMEMORY,NULL);
            list_prototypes[top_list_prototype].diagram_number=i;
            list_prototypes[top_list_prototype].sort=top_list_prototype;
            top_list_prototype++;
            p=new_str(tmp);
            install(p,p,prototype_table);
      }

   }

}/*print_diagrams*/

void sort_prototypes(void)
{
   letnv(comp_p,comp_arrange,COMP_ARR_L);
   sortp();
}/*sort_prototypes*/

void print_prototypes(void)
{
word i,j;
   fprintf(browser_out_file,PROTOTYPESFOUND,top_list_prototype);
   for(i=0;i<top_list_prototype;i++){
      j=list_prototypes[list_prototypes[i].sort].diagram_number;
      fprintf(browser_out_file,OUTPUTPROTOTYPESINFO,
        i+1,
        topologies[diagram[j].topology].id[
           diagram[j].n_id
        ],
        prototypes[diagram[j].prototype]
      );
   }
}/*print_prototypes*/

void print_all_found_topologies(void)
{
word i,j=0;
char tmp[MAX_STR_LEN];
char tt[4];
   fprintf(browser_out_file,FOUNDTOPOLOGIES);
   for(i=0;i<top_topol;i++){
      if(is_bit_set(&(topologies[i].label),0)){
         j++;
         if(topologies[i].orig!=NULL)
            top2str(topologies[i].orig, tmp);
         else
            top2str(topologies[i].topology, tmp);
         if(topologies[i].n_id>1)
           s_let("...",tt);
         else
           *tt=0;
         fprintf(browser_out_file,OUTPUTFOUNDTOPOLOGY,
         j,topologies[i].id[0],tt,tmp);
      }
   }
}/*print_all_found_topologies*/

void print_not_found_topologies(void)
{
word i,j=0;
char tmp[MAX_STR_LEN];
char tt[4];
   fprintf(browser_out_file,NOTFOUNDTOPOLOGIES);
   for(i=0;i<top_topol;i++){
      if(!is_bit_set(&(topologies[i].label),0)){
         j++;
         if(topologies[i].orig!=NULL)
            top2str(topologies[i].orig, tmp);
         else
            top2str(topologies[i].topology, tmp);
         if(topologies[i].n_id>1)
           s_let("...",tt);
         else
           *tt=0;
         fprintf(browser_out_file,OUTPUTFOUNDTOPOLOGY,
         j,topologies[i].id[0],tt,tmp);
      }
   }
   if(!j)fprintf(browser_out_file,ABSENT);
}/*print_not_found_topologies*/

void print_undefined_topologies(void)
{
word i,j=0;
char tmp[MAX_STR_LEN];
   fprintf(browser_out_file,UNDEFINEDTOPOLOGIES);
   for(i=0;i<top_topol;i++){
      if(topologies[i].orig==NULL){
         j++;
         fprintf(browser_out_file,OUTPUTFOUNDTOPOLOGY,
         j,topologies[i].id[0],"", top2str(topologies[i].topology, tmp));
      }
   }
   if(!j)fprintf(browser_out_file,ABSENT);
}/*print_undefined_topologies*/
