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
#include "queue.h"
#include "tools.h"

/************************************************************************/

size_t add_base_queue(size_t len, void *msg, struct queue_base_struct *thequeue)
{
   struct queue_base_struct *tmp;

   if(len<1)return 0;

   tmp=get_mem(1,sizeof(struct queue_base_struct));
   tmp->next=thequeue->next;
   tmp->next->prev=tmp;
   thequeue->next=tmp;
   tmp->prev=thequeue;
   thequeue=tmp;
   thequeue->msg=get_mem(1,thequeue->len=len);
   memcpy(thequeue->msg,msg,len);
   return len;
}/*add_base_queue*/

size_t add_q1_queue(size_t len, char *msg, struct queue_q1_struct *thequeue)
{
   len=add_base_queue(len,(void *)msg,&thequeue->base_queue);
   if(len>0)
      thequeue->totlen+=len;
   return len;
}/*add_q1_queue*/

size_t get_base_queue(struct queue_base_struct *thequeue)
{
   struct queue_base_struct *tmp;

   if (thequeue->prev == thequeue){
      thequeue->len=0; thequeue->msg=NULL; return 0;
   }/*if (thequeue->prev == thequeue->next)*/

   tmp=thequeue->prev;
   thequeue->msg=tmp->msg;
   thequeue->len=tmp->len;
   thequeue->prev=tmp->prev;
   tmp->prev->next=tmp->next;

   free(tmp);
   return thequeue->len;

}/*get_base_queue*/

size_t get_q1_queue(char **buf, struct queue_q1_struct *thequeue)
{

   if(thequeue->alen == 0){
      free_mem(  &( (thequeue->base_queue).msg)  );
      if(get_base_queue(  &(thequeue->base_queue)  )==0)
         return 0;
      thequeue->msg=(char *)(thequeue->base_queue).msg;
      thequeue->alen=(thequeue->base_queue).len;
   }
   (*buf)=thequeue->msg;
   return thequeue->alen;
}/*get_q1_queue*/

/*Returns remaining length:*/
size_t accept_q1_queue(size_t len,struct queue_q1_struct *thequeue)
{
   if( (thequeue->alen < len)) return -1;
   (thequeue->msg)+=len;
   (thequeue->alen)-=len;
   (thequeue->totlen)-=len;
   if(thequeue->alen==0){
      free_mem(  &( (thequeue->base_queue).msg)  );
      (thequeue->base_queue).len=0;
   }
   return thequeue->alen;
}/*accept_q1_queue*/

void init_base_queue(struct queue_base_struct *thequeue)
{
   thequeue->len=0;
   thequeue->msg=NULL;
   thequeue->next=thequeue->prev=thequeue;
}

void init_q1_queue(struct queue_q1_struct *thequeue)
{
   init_base_queue(  &(thequeue->base_queue)  );
   thequeue->alen=thequeue->totlen=0;
   thequeue->msg=NULL;
}

int is_queue(struct queue_q1_struct *thequeue)
{
   return (
            (thequeue->alen!=0) ||
            (  (thequeue->base_queue).prev != &(thequeue->base_queue)  )
          );
}/*is_queue*/

void clear_base_queue(struct queue_base_struct *thequeue)
{
   while(get_base_queue(thequeue))
      free(thequeue->msg);
   thequeue->len=0;
   thequeue->msg=NULL;
}/*clear_base_queue*/

void clear_q1_queue(struct queue_q1_struct *thequeue)
{
   clear_base_queue(&(thequeue->base_queue));
   thequeue->msg=NULL;
   thequeue->alen=thequeue->totlen=0;
}/*clear_q1_queue*/

void move_base_queue(struct queue_base_struct *from,struct queue_base_struct *to)
{

   /*Remove cell from 'from':*/
   from->prev->next=from->next;
   from->next->prev=from->prev;

   /*Add cell to 'to':*/
   from->next=to->next;
   from->next->prev=from;
   to->next=from;
   from->prev=to;
}/*move_base_queue*/

struct queue_base_struct *unlink_cell(struct queue_base_struct *thecell)
{
   thecell->prev->next=thecell->next;
   thecell->next->prev=thecell->prev;
   return thecell;
}/*queue_base_struct*/

void link_cell(struct queue_base_struct *thecell,struct queue_base_struct *thequeue)
{
   thecell->next=thequeue->next;
   thecell->next->prev=thecell;
   thequeue->next=thecell;
   thecell->prev=thequeue;
}/*link_cell*/

void reverse_link_cell(struct queue_base_struct *thecell,struct queue_base_struct *thequeue)
{
   thecell->prev=thequeue->prev;
   thecell->prev->next=thecell;
   thequeue->prev=thecell;
   thecell->next=thequeue;
}/*reverse_link_cell*/

void init_ordered_queue( struct queue_base_struct *thequeue,
                         void (*putTheRoot)(struct queue_base_struct *))
{
   init_base_queue(thequeue);
   putTheRoot(thequeue);
}/*init_ordered_queue*/

int put2q(
            struct queue_base_struct *thecell,
            struct queue_base_struct *thequeue,
            /*ATTENTION!! theval must return 0 for the root!:*/
            int (*theval)(struct queue_base_struct *)
            )
{
int cn=theval(thecell);
   if( theval(thequeue->prev)+1 == cn )
      reverse_link_cell(thecell,thequeue);
   else{
      struct queue_base_struct *c=thequeue->next;
      int nn=theval(c);
         while( (nn<cn)&&(nn!=0) ){c=c->next;nn=theval(c);}
         reverse_link_cell(thecell,c);
   }
   while(   theval(thequeue->prev)+1==theval(thequeue->next) ){
      thecell=unlink_cell(thequeue->next);
      reverse_link_cell(thecell,thequeue);
   }/*while*/
   return theval(thequeue->prev);
}/*put2q*/

