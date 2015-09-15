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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "tools.h"
#include"comdef.h"
#define HALT_MODULE
#include "types.h"
#include "variabls.h"
#include "headers.h"
#include "texts.h"

#ifdef CHECKMEM
extern long membefore;
#endif

void halt(char *fmt, ...)
{
va_list arg_ptr;
 va_start (arg_ptr, fmt);
 fflush(stdout);
 if (errorlevel || message_enable ){
    fprintf(stderr,"\n");
    if(errorlevel)fprintf(stderr,ERRORMSG);

    vfprintf(stderr,fmt, arg_ptr);
    fprintf(stderr,"\n");
 }
 if(log_file!=NULL){
    va_end (arg_ptr);
    va_start (arg_ptr, fmt);
    fprintf(log_file,"\n");
    if(errorlevel)fprintf(log_file,ERRORMSG);
    vfprintf(log_file,fmt, arg_ptr);
    fprintf(log_file,"\n");
 }
 va_end (arg_ptr);
 tools_done();
 truns_done();
 run_done();
 cmd_line_done();
 cnf_read_done();
 browser_done();
 last_done();
 fflush(stderr);
 exit(errorlevel);
}/*halt*/

void message(char *fmt, ...)
{
va_list arg_ptr;
 va_start (arg_ptr, fmt);

 if(message_enable){
    fflush(stdout);
    vfprintf(stderr,fmt, arg_ptr);
    fprintf(stderr,"\n");
    fflush(stderr);
 }/*message_enable*/
 if(log_file!=NULL){
    va_end (arg_ptr);
    va_start (arg_ptr, fmt);
    vfprintf(log_file,fmt, arg_ptr);
    fprintf(log_file,"\n");
 }
 va_end (arg_ptr);

}/*message*/
