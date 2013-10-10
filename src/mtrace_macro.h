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
/*mtrace_macro.h*/
/* This file must be included in case of compilation with mtarce.
   This file contains several macro definitions used to trace through
   get_mem, new_str, and new_int
 */

/*To use  mtracer, set the environment variable MALLOC_TRACE
  to point to the mtracer output file before start Diana.
  Then you can use perl script mtrace (Linux only) to examine
  the tracer output file
 */

/* Use macros instead of functions get_mem, new_str,new_int.*/
/* First, some helpful functions:*/
/*The following function will be used in a macro  new_int. It just copies the content
  of 'from' int 'to' and returns a pointer to 'to':*/
int *copy_int_vec(int *from, int*to);/*tools.c*/

/*Copies the argument to internal variable and returns its value*/
void *dup_ptr( void *s);/*tools.c*/
/*Returns the value stored by dup_ptr:*/
void *get_dup_ptr(void);/*tools.c*/
/*ATTENTION! Using of the above two functions is NOT re-enter-able!*/

/*Macro new_str() will be defined, and new_str_function() will stay as a function*/
char *new_str_function(char *s);

/*And, macros:*/
#define get_mem calloc
#define new_str(s) ((dup_ptr(s)==NULL)?(NULL):(s_let((char*)get_dup_ptr(),calloc((size_t)(s_len((char*)get_dup_ptr())+2),sizeof(char)))) )
#define new_int(s) ((dup_ptr(s)==NULL)?(NULL):(copy_int_vec((int*)get_dup_ptr(),calloc((size_t)( *( (int*)get_dup_ptr() )+1),sizeof(int)))))

