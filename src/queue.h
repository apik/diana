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
#ifndef QUEUE_H
#define QUEUE_H

struct queue_base_struct
{
   struct queue_base_struct *next,*prev;
   size_t len;
   void *msg;
};

struct queue_q1_struct
{
   struct queue_base_struct base_queue;
   size_t alen;
   size_t totlen;
   char *msg;
};

struct queue_base_struct *unlink_cell(struct queue_base_struct *thecell);

void reverse_link_cell(struct queue_base_struct *thecell,struct queue_base_struct *thequeue);
void link_cell(struct queue_base_struct *thecell,struct queue_base_struct *thequeue);
size_t add_base_queue(size_t len, void *msg, struct queue_base_struct *thequeue);
size_t add_q1_queue(size_t len, char *msg, struct queue_q1_struct *thequeue);
size_t get_base_queue(struct queue_base_struct *thequeue);
size_t get_q1_queue(char **buf, struct queue_q1_struct *thequeue);
size_t accept_q1_queue(size_t len,struct queue_q1_struct *thequeue);
void init_base_queue(struct queue_base_struct *thequeue);
void init_q1_queue(struct queue_q1_struct *thequeue);
int is_queue(struct queue_q1_struct *thequeue);
void clear_base_queue(struct queue_base_struct *thequeue);
void clear_q1_queue(struct queue_q1_struct *thequeue);
void move_base_queue(struct queue_base_struct *from,struct queue_base_struct *to);
void init_ordered_queue( struct queue_base_struct *thequeue,
                         void (*putTheRoot)(struct queue_base_struct *));
int put2q(
            struct queue_base_struct *thecell,
            struct queue_base_struct *thequeue,
            /*ATTENTION!! theval must return 0 for the root!:*/
            int (*theval)(struct queue_base_struct *)
            );

#endif
