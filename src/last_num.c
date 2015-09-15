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
#include "tools.h"
#include "comdef.h"
#include "types.h"
#include "variabls.h"
#include "headers.h"
#include "texts.h"

void detect_last_diagram_number(void)
{
   long l=0;
   FILE *input_file=NULL;
   char tmp[MAX_STR_LEN], *ptr;
   int i;
   long max_diagram_number;
      max_diagram_number = (sizeof(int) < 4 )?MAX_DIAGRAM_NUMBER_S:MAX_DIAGRAM_NUMBER_L;
      input_file=open_system_file(input_name);

      /* Go to the <end of input file>-100 -- we want to determine
      the number of diagram: */

       /*go to end of file:*/
      if(fseek(input_file, 0L, SEEK_END)) halt(DISKPROBLEM,NULL);
       /*get position:*/
      if((l=ftell(input_file))==-1) halt(DISKPROBLEM,NULL);
       /*go back for 100 bytes:*/
      if(fseek(input_file, (l>100)?(l-100):0L, SEEK_SET))
         halt(DISKPROBLEM,NULL);
      *tmp=0;l=0;
      do{
        if((i=s_pos("*--#] d",tmp))!=-1){
           ptr=tmp+i+7;
           while((*ptr!=':')&&(*ptr))ptr++;
           *ptr=0;
           sscanf(tmp+i+7,"%ld",&l);
        }
      }while(fgets(tmp, MAX_STR_LEN, input_file)!=NULL);
      close_file(&input_file);
      if(l==0)halt(CURRUPTEDINPUT,input_name);
      if(l>max_diagram_number)halt(MAXDIAGRAM,NULL);
      /* Now l == last diagran number.*/
      if(start_diagram==0)
         start_diagram=1;
      if((finish_diagram==0)||(finish_diagram>l))
         finish_diagram=l;
      if (start_diagram>finish_diagram){
         sprintf(tmp, WRRONGSTARTNUMBER,start_diagram,finish_diagram);
         halt(tmp,NULL);
      }
      /* Now we know size of the dmask*/
      dmask_size=l/250 + 1;
      dmask=get_mem(dmask_size,sizeof(set_of_char));
      if(xi_list_top!=NULL){
         struct list_struct *top=xi_list_top;
            while(top!=NULL){
               if(top->from < start_diagram)
                  top->from=start_diagram;
               if(top->to > finish_diagram)
                  top->to=finish_diagram;
               for(l=top->from;!(l>top->to);l++){
                 if(top->xi=='x')/*ATTENTION! We use inverse order!*/
                   set_set((l % 250),dmask[l / 250]);
                 else/*ATTENTION! We use inverse order!*/
                   set_del((l % 250),dmask[l / 250]);
               }
               xi_list_top=top->next;
               free_mem(&top);
               top=xi_list_top;
            }
      }
      if (is_bit_set(&mode,bitBROWS))
        diagram=get_mem(finish_diagram-start_diagram+1,
           sizeof(struct diagram_struct));
}/*detect_last_diagram_number*/
