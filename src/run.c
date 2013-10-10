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
/*RUN.C*/
#include <stdlib.h>
#include <stdio.h>
#include "tools.h"
#include "comdef.h"
#include "types.h"
#include "variabls.h"
#include "headers.h"
#include "texts.h"
#define RUN
#include "trunsrun.h"

static char *datastack=NULL;
static word stacktop=0;
static word max_stacktop=0;

int ifdef(char *id)
{
   return(lookup(id,export_table)!=NULL);
}/*ifdef*/

static char outbuffer[MAXOUTBUFFERSIZE];
static char *head=outbuffer;
static int fromputstr=0;
static int wasleftflush=0;

static void closesuspendedfiles(void)
{
  FILE *outf;
  struct mem_outfile_struct *tmp;
     while(suspendedfiles!=NULL){
        outf=(tmp=suspendedfiles)->outfile;
        suspendedfiles=suspendedfiles->next;
        free_mem(&tmp);
        if((outf!=NULL)&&(outf!=stdout))close_file(&outf);
     }
}/*closesuspendedfiles*/

void flush(void)
{
  char *buf;
    if(outfile==NULL)
       head=outbuffer;
    if(head==outbuffer)/*buffer is empty*/
        return;
    *head=0;
    if((offleadingspaces)&&(wasleftflush==0)){
       head=buf=outbuffer;
       while((*head==' ')||(*head=='\t'))head++;
       while((*buf++=*head++)!=0);
    }
    if((offtailspaces)&&(fromputstr)){
       int i;char c;
       head=buf=outbuffer;
       while(!((*head=='\n')||(*head==0)))head++;
       if((i=head-buf)!=0){
         c=*head;
         for(i--;(!(buf[i]>' '))&&(!(i<0));i--){
             buf[i]=c;buf[i+1]=0;
         }
       }
    }
    if((offblanklines)&&(wasleftflush==0)){
      head=outbuffer;
      while(*head)
        if(*head>' ')break;
        else head++;
      if(!(*head)){
         head=outbuffer;
         return;
      }
    }
    if (fputs(outbuffer,outfile)<0)
         halt(CANNOTWRITETOFILE,outputfile_name);
    head=outbuffer;
    if (fromputstr)
       wasleftflush=0;
    else
       wasleftflush=1;

}/*flush*/

void outputstr(char *str)
{
  if(offout)return;
  if((outfile==NULL)||(*str==0))return;
  {
   char *tmp;
   int i;
     tmp=str;
     i=head-outbuffer+2;
     do{
         do{
           if(*tmp==chEOF)
              tmp++;
           if(*tmp==0)return;
           if(++i>MAXOUTBUFFERSIZE)
              halt(TOOLONGSTRING,NULL);
           *head++=*tmp;
         }while(*tmp++!='\n');
         fromputstr=1;
         flush();
         fromputstr=0;
     }while(*tmp);
  }
}/*outputstr*/

static void stackexpand(void)
{
 if((datastack=realloc(datastack,
                        (max_stacktop+=DELTA_STACK_SIZE)*sizeof(char)
                      ))==NULL) halt(NOTMEMORY,NULL);
}/*stackexpand*/

#ifdef SKIP
static int put_data(char *str)/*returns length of placed string*/
{
   if ((*str==tpVAL)||(*str==tpBOOLEAN)){
     if (!(stacktop<max_stacktop))stackexpand();
     datastack[stacktop++]=*(str+1);
     return(2);
   } else if(*str==tpSTRING){
     int tmp=stacktop;
        str++;
        if (!(stacktop<max_stacktop))stackexpand();
        datastack[stacktop++]=TERM;
        while(*str!=TERM){
           if (!(stacktop<max_stacktop))stackexpand();
           datastack[stacktop++]=*str++;
        }
        return(stacktop-tmp+1);
   }
#ifdef DEBUG
   else halt("run:unknown type!",NULL);
#endif
   return(0);
}/*put_data*/
#endif

static int put_data(char *str)/*returns length of placed string*/
{
int tmp;
   switch(*str){
      case tpVAL:
      case tpBOOLEAN:
         if (!(stacktop<max_stacktop))stackexpand();
         datastack[stacktop++]=*(str+1);
         return 2;
      case tpSTRING:
         tmp=stacktop;
         str++;
         if (!(stacktop<max_stacktop))stackexpand();
         datastack[stacktop++]=TERM;
         while(*str!=TERM){
            if (!(stacktop<max_stacktop))stackexpand();
            datastack[stacktop++]=*str++;
         }
         return(stacktop-tmp+1);
#ifdef DEBUG
      default:
         halt("run:unknown type!",NULL);
#endif
   }/*switch(*str)*/
   return(0);
}/*put_data*/

void push_string(char *str)
{
  if (!(stacktop<max_stacktop))stackexpand();
  datastack[stacktop++]=TERM;
  while(*str!=0){
     if (!(stacktop<max_stacktop))stackexpand();
     datastack[stacktop++]=*str++;
  }
}/*push_string*/

static void put_val(char val)
{
  if (!(stacktop<max_stacktop))stackexpand();
  datastack[stacktop++]=val;
}/*put_val*/

static char get_val(void)
{
  return(datastack[--stacktop]);
}/*get_val*/

int total_length=0;

char *get_string( char *str)
{
  int tmp=stacktop,i;
  char *mem=str;
    while(datastack[--stacktop]!=TERM);

  if(!( (total_length=tmp-stacktop)<MAX_STR_LEN))
      halt(TOOLONGSTRING,NULL);

    for(i=stacktop+1;i<tmp;i++)
                 *str++ =datastack[i];
    *str=0;
    return(mem);
}/*get_string*/

long get_num(char *name,char *buf)
{
  int i,tmp=stacktop;
  long num=0;
    for(i=0; !(i<0);){

       switch( datastack[--stacktop] ){
          case TERM:
             i=-1;/*Quit main cycle*/
             break;
          case '0':
             i++;
             break;
          case '1':
             num+=tenToThe[(i++) % g_maxlonglength];
             break;
          case '2':
             num+=(2l)*tenToThe[(i++) % g_maxlonglength];
             break;
          case '3':
             num+=(3l)*tenToThe[(i++) % g_maxlonglength];
             break;
          case '4':
             num+=(4l)*tenToThe[(i++) % g_maxlonglength];
             break;
          case '5':
             num+=(5l)*tenToThe[(i++) % g_maxlonglength];
             break;
          case '6':
             num+=(6l)*tenToThe[(i++) % g_maxlonglength];
             break;
          case '7':
             num+=(7l)*tenToThe[(i++) % g_maxlonglength];
             break;
          case '8':
             num+=(8l)*tenToThe[(i++) % g_maxlonglength];
             break;
          case '9':
             num+=(9l)*tenToThe[(i++) % g_maxlonglength];
             break;
          case '-':
             num = - num;
             /* no break!*/
          case '+':/*Nothing to do for '+'*/
             if(i == 0 ){/*Error! Empty number!*/
                i=-2;break;
             }
             /* No break, i==0 already performed*/
          case ' ':
             if(i == 0 ){/*Trailing spaces?*/
                int q=1;/*Quit flag for 'while'*/
                while(q)switch( datastack[--stacktop] ){
                    case ' '  :/*skip spaces*/
                       break;/*continue*/
                    case TERM :/*Empty string!*/
                       i=-3;/*An error!*/
                       q=0;/*Quit 'while'*/
                       break;
                    default:/*Something interesting:*/
                       stacktop++;/*Push it back*/
                       q=0;/*Quit 'while'*/
                       break;
                }/*while(1)switch( datastack[--stacktop] )*/
                /*i is untouched*/
                break;
             }/*if(i == 0 )*/
             /*else: leading spaces?*/
             while(i>0)switch( datastack[--stacktop] ){
                case ' '  :/*skip spaces*/
                   break;/*continue*/
                case TERM :/*That's all*/
                    i=-1;/*Quit main cycle*/
                    break;
                default:/*Something not allowed!:*/
                   i=-4;/*An error!*/
                   break;
             }/*while(1)switch( datastack[--stacktop] )*/
             break;
          default:
             /*Something not allowed!:*/
             i=-5;/*An error!*/
             break;
       }/*switch( datastack[--stacktop] )*/
    }/*for(i=0; !(i<0);)*/
    if(i!=-1){/*An error!*/
       char arg[MAX_STR_LEN];
       stacktop=tmp;/*Restore the stack*/
       get_string(buf);
       sprintf(arg,NUMERROR,name,buf);
       halt(arg,NULL);
    }/*if(i!=-1)*/
    return num;
}/*get_num*/

void stack2stack(int num)
{
  int d=callstacktop,i=0,c=0;
    for(i=callstacktop-1,c=0;i>-1;i--){
       if(callstack[i]==TERM)c++;
       if(c==num)break;
    }
    /* Now i points to the begin of copying,
       (callstacktop-i)=?*/
    d-=i;
    if (!(stacktop+d<max_stacktop))
       if((datastack=realloc(datastack,
                        (max_stacktop=stacktop+d+1)*sizeof(char)
                      ))==NULL) halt(NOTMEMORY,NULL);
    callstacktop-=d;
    for(;d>0;d--)
       datastack[stacktop++]=callstack[i++];
}/*stack2stack*/

static long current_line=0;
static int current_col=0;
static char *input_f_name=NULL;
static char *mexec(char seg,char offset, char *arg)
{
 word num;
 num=(seg-1)*127;
 num+=(offset-1);
 if (get_val()-1){/* Need debug info*/
   input_f_name=macros_f_name[get_val()-1];
   sscanf(get_string(arg),"%d",&current_col);
   sscanf(get_string(arg),"%ld",&current_line);
   mexpand[num](arg);
   input_f_name=NULL;
   current_line=current_col=0;
 }else
   mexpand[num](arg);
 return(arg);
}/*mexec*/

char *pexec(char seg,char offset,char *arg)
{
 word num;
 int i;
 if(top_save_run>LAST_M_RECURSION)
    halt(PROCSTACKOVERFLOW,NULL);
 num=(seg-1)*127;
 num+=(offset-1);
 save_run[top_save_run].variables_table=variables_table;
 variables_table=NULL;
 save_run[top_save_run].labels_tables=labels_tables;
 labels_tables=pexpand[num].labels_tables;
 for(i=pexpand[num].arg_numb;i;i--)
    set_run_var(pexpand[num].var[i-1],get_string(arg));
 save_run[top_save_run++].stacktop=stacktop;

 *returnedvalue=0;
 run(pexpand[num].top_out,pexpand[num].out_str);
 s_let(returnedvalue,arg);
 *returnedvalue=0;
 stacktop=save_run[--top_save_run].stacktop;
 labels_tables=save_run[top_save_run].labels_tables;
 variables_table=save_run[top_save_run].variables_table;
 return(arg);
}/*pexec*/

struct args_struct{
   char arg1[MAX_STR_LEN];
   char arg2[MAX_STR_LEN];
} *args=NULL;
int top_args=0,max_top_args=0;

int run(long out_len,char *inp_array[])
{
 char *str,*ptr;
 char *arg1=NULL,*arg2=NULL;
 long i;
    wasrun=1;
    runexitcode=0;
    isrunexit=0;
    current_instruction_address=&i;
    if (!(top_args<max_top_args))
       if((args=realloc(args,
                        (max_top_args+=DELTA_ARGS_SIZE)
                           *sizeof(struct args_struct)
                      ))==NULL) halt(NOTMEMORY,NULL);

    arg1=args[top_args].arg1;
    arg2=args[top_args].arg2;
    top_args++;
    for(i=0;i<out_len;i++){
#ifndef NO_DEBUGGER
      if(g_trace_on)
         while(doTrace(i,inp_array));/**/
#endif
      str=inp_array[i]+g_debug_offset;
      if(*str!=PLSIGNAL){
        outputstr(str);/* Regular string*/
      }else{/* Polish line*/
           /* Load stack until command and do the command */
           ptr=str+1;
           do{
              while(*ptr!=tpCOMMAND)
                    ptr+=put_data(ptr);/*Put all data */
              ptr++;/* skip 'tpCOMMAND'*/
              switch(*ptr){
                 case opNOT:
                    *arg1=get_val()-1;
                    put_val(!(*arg1)+1);
                    break;
                 case opLT:
                    get_string(arg2);
                    get_string(arg1);
                    put_val((s_cmp(arg1,arg2)<0)+1);
                    break;
                 case opLE:
                    get_string(arg2);
                    get_string(arg1);
                    put_val((!(s_cmp(arg1,arg2)>0))+1);
                    break;
                 case opGT:
                    get_string(arg2);
                    get_string(arg1);
                    put_val((s_cmp(arg1,arg2)>0)+1);
                    break;
                 case opGE:
                    get_string(arg2);
                    get_string(arg1);
                    put_val((!(s_cmp(arg1,arg2)<0))+1);
                    break;
                 case opEQ:
                    get_string(arg2);
                    get_string(arg1);
                    put_val((s_scmp(arg1,arg2)==0)+1);
                    break;
                 case opNE:
                    get_string(arg2);
                    get_string(arg1);
                    put_val((s_scmp(arg1,arg2)!=0)+1);
                    break;
                 case opAND:
                    *arg2=get_val()-1;
                    *arg1=get_val()-1;
                    put_val(((*arg2)&&(*arg1))+1);
                    break;
                 case opXOR:
                    *arg2=get_val()-1;
                    *arg1=get_val()-1;
                    put_val((((*arg2)||(*arg1))&&(!((*arg2)&&(*arg1))))+1);
                    break;
                 case opOR:
                    *arg2=get_val()-1;
                    *arg1=get_val()-1;
                    put_val(((*arg2)||(*arg1))+1);
                    break;
                 case opISDEF:
                    put_val(ifdef(get_string(arg1))+1);
                    break;
                 case opCAT:
                    get_string(arg2);
                    { int mem=total_length;
                      get_string(arg1);
                      if( mem+total_length>MAX_STR_LEN )
                         halt(TOOLONGSTRING,NULL);
                    }
                    push_string(s_cat(arg1,arg1,arg2));
                    break;
                 case cmIF:
                    get_string(arg1);
                    if(!(get_val()-1)){
                       i=decode(arg1, BASE);
                       i--;/* 1 due to 'for' autoincrement*/
                    }
                    break;
                 case cmELSE:
                    get_string(arg1);
                    i=decode(arg1, BASE);
                    i--;/* 1 due to 'for' autoincrement*/
                    break;
                 case cmLOOP:
                    get_string(arg1);/*number of string*/
                    if(get_val()-1){
                       i=decode(arg1, BASE);
                       i--;/* 1 due to 'for' autoincrement*/
                    }
                    break;
                 case cmWLOOP:
                    get_string(arg1);
                    i=decode(arg1, BASE);
                    i--;/* 1 due to 'for' autoincrement*/
                    break;
                 case cmOUTLINE:
                    outputstr(get_string(arg1));
                    break;
                 case cmMACRO:
                    push_string(mexec(*(ptr+1),*(ptr+2),arg2));
                    ptr+=2;
                    current_instruction_address=&i;
                    if(isrunexit){/*Exit from run:*/
                        i=out_len;/*End exterior cycle*/
                        while(*(ptr+1))ptr++;/*End do-while sysle*/
                    }
                    if(isreturn){/*Exit from run:*/
                        i=out_len;/*End exterior cycle*/
                        while(*(ptr+1))ptr++;/*End do-while sysle*/
                        isreturn=0;
                    }
                    break;
                 case cmPROC:
                    push_string(pexec(*(ptr+1),*(ptr+2),arg2));
                    ptr+=2;
                    current_instruction_address=&i;
                    if(isrunexit){/*Exit from run:*/
                        i=out_len;/*End exterior cycle*/
                        while(*(ptr+1))ptr++;/*End do-while sysle*/
                    }
                    break;
                 case cmOFFLS:
                    flush();
                    offleadingspaces=1;
                    break;
                 case cmONLS:
                    flush();
                    offleadingspaces=0;
                    break;
                 case cmOFFTS:
                    flush();
                    offtailspaces=1;
                    break;
                 case cmONTS:
                    flush();
                    offtailspaces=0;
                    break;
                 case cmOFFBL:
                    flush();
                    offblanklines=1;
                    break;
                 case cmONBL:
                    flush();
                    offblanklines=0;
                    break;
                 case cmOFFOUT:
                    offout=1;
                    break;
                 case cmONOUT:
                    offout=0;
                    break;
                 default:
                    halt(INTERNALERROR,NULL);
                    break;
              }/*switch*/

           }while(*++ptr);/* Repeat until string exhausted*/
      }/*if(*str!=PLSIGNAL) ... else */
    }/*for(i=0;i<out_len;i++)*/
    flush();
    top_args--;
    if (variables_table!=NULL){
       hash_table_done(variables_table);
       variables_table=NULL;
    }
    if((outfile!=stdout)&&(top_save_run==0)){
        close_file(&outfile);
        closesuspendedfiles();
    }
    if((is_scanner_init)&&(top_save_run==0)){
       if(is_scanner_double_init)
          double_done();
       else
          sc_done();
    }
    current_instruction_address=NULL;
    return(runexitcode);
}/*run*/

void clear_modestack(void)
{
  if(max_modestack){
     free_mem(&modestack);
     max_modestack=top_modestack=0;
  }
  if(p_max_modestack){
     free_mem(&p_modestack);
     p_max_modestack=p_top_modestack=0;
  }

}/*clear_modestack*/

static void done(void)
{
  int i;
  long j;
  
  for(i=0;i<top_fname;i++)
     free_mem(&(macros_f_name[i]));
  clear_modestack();
  free_mem(&datastack);
  stacktop=max_stacktop=0;

  free_mem(&callstack);
  max_callstacktop=callstacktop=0;
  free_mem(&args);
  close_file(&outfile);
  closesuspendedfiles();
  for(i=0;i<top_txt;i++)
     free_mem(&(text_block[i]));
  free_mem(&text_block);
  top_txt=max_top_txt=0;

  if (proc_table!=NULL){
     hash_table_done(proc_table);
     proc_table=NULL;
  }
  for(i=0;i<top_pexpand;i++){
     for(j=0;j<pexpand[i].arg_numb;j++)
        free_mem(&(pexpand[i].var[j]));
     free_mem(&(pexpand[i].var));
     for(j=0; j<pexpand[i].top_out; j++)
        free_mem(&((pexpand[i].out_str)[j]));
     free_mem(&(pexpand[i].out_str));
     hash_table_done(pexpand[i].labels_tables);
  }
/*  cleanproc();*/
  free_mem(&pexpand);
  top_pexpand=max_pexpand=0;
  if (variables_table!=NULL){
     hash_table_done(variables_table);
     variables_table=NULL;
  }
  if (export_table!=NULL){
     hash_table_done(export_table);
     export_table=NULL;
  }
  if (ext_file!=stdin){
    close_file(&ext_file);
  }
}/*done*/

void run_done(void)
{
  if (input_f_name!=NULL){
     fprintf(stderr,RUNERROR);
     if(log_file!=NULL)
       fprintf(log_file,RUNERROR);
     pointposition(input_f_name,current_line,current_col);
  }
  done();
#ifndef NO_DEBUGGER
  debugger_done();
#endif
}/*run_done*/

int set_run_var(char *varname,char *varval)
{
  if (variables_table==NULL)variables_table=create_hash_table(
                                var_hash_size,str_hash,str_cmp,c_destructor);
  return(install(new_str(varname),new_str(varval),variables_table));
}/*set_run_var*/

int set_export_var(char *varname,char *varval)
{
  if (export_table==NULL)export_table=create_hash_table(
                      export_hash_size,str_hash,str_cmp,c_destructor);
  return(install(new_str(varname),new_str(varval),export_table));
}/*set_export_var*/

