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
/*TRUNC.C*/

#include <stdlib.h>
#include <stdio.h>
#include "tools.h"
#include "comdef.h"
#include "types.h"
#include "variabls.h"
#include "headers.h"
#include "texts.h"
#define TRUNS
#include "trunsrun.h"

/* Constants for type definition:*/
/*Operation:*/
#define tpOP 1
/* Open bracket:*/
#define tpOPENB 2
/* Close bracket:*/
#define tpCLOSEB 3
/* String type:*/
#define tpLINE 4
/* Boolean type:*/
#define tpBOOL 5

static long current_labels_free;
static long current_labels_level;
static long *current_labels_stack=NULL;
static long current_labels_top=0;
static long current_labels_max=0;

static int wascall=0;
static char *polish_line=NULL;
static word ptr=0;/*Counter for polish_line*/
static word max_ptr=0;/*Top allocated polish_line*/
static int mrec=0;/*nested macros counter*/
static int rec=0;/*nested compile counter*/

/*Infor for debug :*/
static long current_line=0;
static int current_col=0;
static char inp_f_name[MAX_STR_LEN];

extern char *currentRevisionNumber;
struct control_struct{
   int sign;
   long line;
   word offset;
};

static char *IF_stack=NULL;
static word IF_depth=0;
static word max_IF_depth=0;

static struct control_struct *ifstack=NULL;
static int ifdepth=0;/*ifstack index*/
static int max_ifdepth=0;/*top allocated ifstack*/

static struct control_struct *wstack=NULL;/*while stack*/
static int wdepth=0;/*wstack index*/
static int max_wdepth=0;/*top allocated wstack*/

static long *dostack=NULL;
static int dodepth=0;
static int max_dodepth=0;/*top allocated wstack*/

static char **defnames=NULL;
static long deftop=-1;
static long maxdeftop=0;

static void proc_IF(int bool)
{
  IF_depth++;
  if(!(IF_depth < max_IF_depth))
     if((IF_stack=realloc(IF_stack,
            (max_IF_depth+=DELTA_IF_DEPTH)*
              sizeof(char *)))==NULL)
              halt(NOTMEMORY,NULL);
  IF_stack[IF_depth]=(bool)?1:2;
}/*proc_IF*/

static void register_command(char *command, int numeric)
{
int *tmp;
   *(tmp=get_mem(1,sizeof(int)))=numeric;
   install(new_str(command),tmp, command_table);
}/*register_command*/

/* Registering of all commands in the command hash table:*/
void command_init(void)
{
char esc_line[2];
   if(command_table==NULL)
      command_table=
        create_hash_table(COMMAND_HASH_SIZE,str_hash,str_cmp,c_destructor);
   *esc_line=esc_char;
   esc_line[1]=0;
   register_command("",0);
   register_command(esc_line,cm_CHANGEESC);
   register_command("(",0);
   register_command("#",0);
   register_command("eq",opEQ);
   register_command("ne",opNE);
   register_command("and",opAND);
   register_command("or",opOR);
   register_command("lt",opLT);
   register_command("gt",opGT);
   register_command("le",opLE);
   register_command("ge",opGE);
   register_command("xor",opXOR);
   register_command("not",opNOT);
   register_command("cat",opCAT);
   register_command("exist",opISDEF);
   register_command("while",cmWLOOP);
   register_command("[",cmBEGINHASH);
   register_command("]",cmENDHASH);
   register_command("{",cmBEGINKEEPSPACES);
   register_command("}",cmENDKEEPSPACES);
   register_command("label",cmLABEL);
   register_command("if",cmIF);
   register_command("else",cmELSE);
   register_command("elif",cmELIF);
   register_command("endif",cmENDIF);
   register_command("loop",cmLOOP);
   register_command("do",cmDO);
   register_command("-",cmOFFOUT);
   register_command("+",cmONOUT);
   register_command("offleadingspaces",cmOFFLS);
   register_command("onleadingspaces",cmONLS);
   register_command("offtailspaces",cmOFFTS);
   register_command("ontailspaces",cmONTS);
   register_command("offblanklines",cmOFFBL);
   register_command("onblanklines",cmONBL);
   register_command("beginlabels",cmBEGINLABELS);
   register_command("endlabels",cmENDLABELS);
   register_command("ENDFOR",cm_ENDFOR);
   register_command("ENDDEF",cm_ENDDEF);
   register_command("FOR",cm_FOR);
   register_command("include",cmINCLUDE);
   register_command("IFDEF",cm_IFDEF);
   register_command("IFNDEF",cm_IFNDEF);
   register_command("IFSET",cm_IFSET);
   register_command("IFNSET",cm_IFNSET);
   register_command("IFEQ",cm_IFEQ);
   register_command("IFNEQ",cm_IFNEQ);
   register_command("ELSE",cm_ELSE);
   register_command("ENDIF",cm_ENDIF);
   register_command("SET",cm_SET);
   register_command("GET",cm_GET);
   register_command("GETENV",cm_GETENV);
   register_command("VERSION",cm_VERSION);
   register_command("ADDSPACES",cm_ADDSPACES);
   register_command("RMSPACES",cm_RMSPACES);
   register_command("ADDDELIMITERS",cm_ADDDELIMITERS);
   register_command("RMDELIMITERS",cm_RMDELIMITERS);
   register_command("CMDLINE",cm_CMDLINE);
   register_command("RMARG",cm_RMARG);
   register_command("ARGC",cm_ARGC);
   register_command("UNSET",cm_UNSET);
   register_command("POP",cm_POP);
   register_command("REMOVE",cm_REMOVE);
   register_command("REPLACE",cm_REPLACE);
   register_command("REPLACEALL",cm_REPLACEALL);
   register_command("ADD",cm_ADD);
   register_command("PUSH",cm_PUSH);
   register_command("PUSHQ",cm_PUSHQ);
   register_command("DEF",cm_DEF);
   register_command("UNDEF",cm_UNDEF);
   register_command("function",cmFUNCTION);
   register_command("program",cmPROGRAM);
   register_command("keepfile",cmKEEPFILE);
   register_command("SCAN",cm_SCAN);
   register_command("MESSAGE",cm_MESSAGE);
   register_command("READ",cm_READ);
   register_command("MINVERSION",cm_CHECKMINVERSION);
   register_command("ERROR",cm_ERROR);
   register_command("NEWCOMMENT",cm_NEWCOMMENT);
   register_command("NEWCOMMA",cm_NEWCOMMA);
   register_command("REALLOCINCLUDE",cm_REALLOCINCLUDE);
   register_command("_modesave",cmMODESAVE);
   register_command("_moderestore",cmMODERESTORE);
   register_command("end",cmEND);
}/*command_init*/

static void put_cmd(char cmd)/*puts command into polish_line*/
{
  if(ptr+2>max_ptr)/*possible expansion polish_line*/
     if((polish_line=(char *)realloc(polish_line,
                           (max_ptr+=DELTA_POLISH_LINE)*sizeof(char)))==NULL)
        halt(NOTMEMORY,NULL);
  polish_line[ptr++]=tpCOMMAND;/* Put type id*/
  polish_line[ptr++]=cmd;/* Put command*/
}/*put_cmd*/

static void put_bool(char bool)/*puts boolean value into polish_line*/
{
  if(ptr+2>max_ptr)
     if((polish_line=(char *)realloc(polish_line,
                    (max_ptr+=DELTA_POLISH_LINE)*sizeof(char)))==NULL)
        halt(NOTMEMORY,NULL);
  polish_line[ptr++]=tpBOOLEAN;
  polish_line[ptr++]=bool;
}/*put_bool*/

static void put_val(char val)/*puts value into polish_line*/
{
  if(ptr+2>max_ptr)
     if((polish_line=(char *)realloc(polish_line,
               (max_ptr+=DELTA_POLISH_LINE)*sizeof(char)))==NULL)
        halt(NOTMEMORY,NULL);
  polish_line[ptr++]=tpVAL;
  polish_line[ptr++]=val;
}/*put_val*/

static int get_fname(void)/*For possible error. This function looking for
                            current file name in the array 'macros_f_name'.
                            If it not found, it stores current filename in
                            this array. Returns index of this array
                            corresponding current filename.*/
{
  int i;
  char *fname=get_input_file_name();
     for(i=0;i<top_fname;i++)
        if(!s_scmp(macros_f_name[i],fname))
           return(i);
     if(!(top_fname<MAX_INCLUDE))
        halt(INTERNALERROR,NULL);
     macros_f_name[top_fname++]=new_str(fname);
     return(top_fname-1);
}/*get_fname*/

static void l_put_linenumberandfilename_into_polishline(void)
{
int c=get_current_col();
   if(max_ptr<9)
      if((polish_line=(char *)realloc(polish_line,
        (max_ptr+=DELTA_POLISH_LINE)*sizeof(char)))==NULL)
           halt(NOTMEMORY,NULL);

   encode(get_current_line(),polish_line,BASE);
   polish_line[4]=(c/127)+1;/*segment*/
   polish_line[5]=(c%127)+1;/*offset*/
   polish_line[6]=get_fname()+1;
}/*l_put_linenumberandfilename_into_polishline*/

static void put_string(char *str)/*puts string into polish_line*/
{
  while(ptr+s_len(str)+3>max_ptr)
     if((polish_line=(char *)realloc(polish_line,
     (max_ptr+=DELTA_POLISH_LINE)*sizeof(char)))==NULL)
        halt(NOTMEMORY,NULL);
  polish_line[ptr++]=tpSTRING;
  while(*str)polish_line[ptr++]=*str++;
  polish_line[ptr++]=TERM;/* We can't use 0 as a string terminator.*/
}/*put_string*/

static int expand_str( char *str);/*Forward definition: put_macros()
                                    and expand_str() may call one another.*/

static char macro_error[MAX_STR_LEN];
static int active_token=0;

static char *text_token=NULL;/*The result of 'pre_scan' work. It consists of
                               successively allocated ASCII-Z strings.*/
static int text_token_len=0,max_text_token_len=0;

struct token_struct{
   int n;/*Number of begin of a token in text_token*/
   char type; /*tpOP operation,tpOPENB (,tpCLOSEB ),tpLINE string*/
   long current_line;/*Debug info for possible error*/
   int current_col;/*Debug info for possible error*/
};
static struct token_struct *token=NULL;/*Array of tokens for trunslating
                                        input text into polish line*/
static int token_top=0;/*top of token array*/
static int max_token=0;/*Top of allocated tokens*/

static struct {/* Used for store some information from proc_macro*/
   int is_proc;
   struct macro_hash_cell *cellptr;
   int nargs;
} macro_control;

static int put_macros( char **str)/*Puts macro calling into 'polish_line'.
                                  In success, returns 0, if error, returns 1*/
{
  char *beg=*str,*fin=*str;
  char balance=0;/*bracket balance*/
  if(++mrec>LAST_M_RECURSION)
     halt(TOOMANNESTEDMACRO,NULL);
  /*Pick out macro name:*/
  for(;*fin;fin++){
     if(*fin==esc_char){
        *fin=0;
        break;
     }
     if(*fin=='(')/*Begin of arguments is found*/
        break;
  }
  if(*fin == 0){
     sprintf(macro_error,WHEREARGS,beg);
     return(1);
  }
  *fin=0;

  if(*beg==0){/* Quotation macro -- expanded during translation*/
     beg=fin=fin+1;/*Now beg points to the begin of arguments*/
     for(;*fin;fin++){
        if (*fin=='(')balance++;
        else if (*fin==')'){
           if (balance--==0){
              *fin=0;
              *str=fin+1;
              put_string(beg);
              mrec--;
              return(0);
           }
        }
     }
     halt(INTERNALERROR,NULL);/*The control can't reach this point!*/
  }

  {/*block begin*/
   /* Here beg is the macro name.*/
  struct macro_hash_cell *cellptr;
  char i=0;
  char is_proc=0;
      if(macro_control.cellptr!=NULL){
          cellptr=macro_control.cellptr;
          macro_control.cellptr=NULL;
          is_proc=macro_control.is_proc;
      }else if((cellptr=lookup(beg,macro_table))==NULL){
        if((cellptr=lookup(beg,proc_table))==NULL){
           sprintf(macro_error,NAMENOTFOUND,beg);
           return(1);
        }else is_proc=1;
      }
      if(is_proc==0){
        if ((!s_scmp(beg, "call"))||
           (!s_scmp(beg, "compile")))
              wascall=1;
      }
      sprintf(macro_error,WRONGARGNUM,cellptr->name);
      if(cellptr->arg_numb==0){/*Macro without args.*/
        if(*(fin+1)!=')'){
           sprintf(macro_error,WRONGEMPTYARGS,cellptr->name,fin+1);
           return(1);
        }
        fin++;/* Go after ')'.*/
        *fin=0;
        *str=fin+1;/*Shift incoming pointer*/
      }else{
         beg=fin=fin+1;/*Now beg points to the begin of arguments*/
         /* Begin determining arguments:*/
         balance=0;

         for(i=0;i<cellptr->arg_numb;i++){
            for(;*fin;fin++){
               if (*fin=='(')balance++;
               else if (*fin==')'){
                  if (balance==0){
                     if(i!=cellptr->arg_numb-1)
                        return(1);/*Wrong number of args.*/

                     balance=-1;/*Use 'balance' as a flag for further checking.*/
                     break;
                  }else balance--;
               }else if((*fin==comma_char)&&(balance==0)){/*Found argument*/
                  *fin=0;
                  /*Call to expand_str() to expand argument:*/
                  if(expand_str(beg))
                     return(1); /*expand_str() returns nonzero when error*/
                  else/*Reset name:*/
                    sprintf(macro_error,WRONGARGNUM,cellptr->name);
                  beg=fin=fin+1;
                  break;/* Build follow argument*/
               }
            }
         }
         if((i!=cellptr->arg_numb)||(*fin!=')')||(balance!=-1))
             return(1);/*Wrong number of args.*/
         *fin=0;
         *str=fin+1;/*Shift incoming pointer*/
         /*Call to expand_str() to expand lastargument:*/
         if(expand_str(beg))
            return(1); /*expand_str() returns nonzero when error*/
         else/* Reset name:*/
            sprintf(macro_error,WRONGARGNUM,cellptr->name);
      }
      /*Put info about labels update, if need. This info is used by
        operator goto.*/
      if(is_bit_set(&(cellptr->flags),bitLABELSUPDATE)){
         char tmp[6];
         encode(current_labels_level,tmp,BASE);
         put_string(tmp);
      }

      if (is_bit_set(&(cellptr->flags),bitDEBUG)){/* Put debug info into polish_line:*/
       char num[NUM_STR_LEN];
         if (active_token){
            long2str(num,token[active_token].current_line);
            put_string(num);
            long2str(num,token[active_token].current_col);
            put_string(num);
         }else{
            long2str(num,get_current_line());
            put_string(num);
            long2str(num,get_current_col());
            put_string(num);
         }
         put_val(get_fname()+1);
      }
      if(is_proc){
         /*Put command 'proc':*/
         put_cmd(cmPROC);
         /*Mark as used:*/
         (pexpand[cellptr->num_mexpand]).wascall=1;
      }else{
         /*Put info about need debug:*/
         if(is_bit_set(&(cellptr->flags),bitDEBUG))
            put_bool(2);
         else
            put_bool(1);
         /*Put commanr 'macro':*/
         put_cmd(cmMACRO);
      }
      /*Put number of mexpand function:*/
      if(ptr+2>max_ptr)/*expand polish_line, if need:*/
         if((polish_line=(char *)realloc(polish_line,
           (max_ptr+=DELTA_POLISH_LINE)*sizeof(char)))==NULL)
                halt(NOTMEMORY,NULL);
      /*Put segment:*/
      polish_line[ptr++]=(cellptr->num_mexpand/127)+1;
      /*Put offset:*/
      polish_line[ptr++]=(cellptr->num_mexpand % 127)+1;

  }/*block end*/
  mrec--;
  return(0);
}/*put_macros*/

static int expand_str( char *str)/*Puts argument into polish_line.
                                  If the argument contains a macro,
                                  the function will call to 'put_macros()'.*/
{
  int is_cat=0;/*Flag for possible concatenatios*/
  char *beg=str,*fin=str;
    if (*fin==0){/*Empty string*/
       put_string(beg);
       return(0);
    }
    do{
       while(*fin==esc_char){/*Macro found*/
          *fin=0;
          if (fin!=beg){/*if fin==beg, then the macro is at the BEGIN of the
                                string.*/
             put_string(beg);
             if(is_cat) put_cmd(opCAT);
             is_cat=1;
          }
          beg=fin+1;/*Now beg points to the begin of a macros*/
          if(put_macros(&beg))return(1);
          /*Now beg points just after macros*/
          if(is_cat)put_cmd(opCAT);
          fin=beg;
          is_cat=(*fin);/*If macros ends the string, *fin is zero,
                        no need to cat.*/
       }
    }while(*fin++);
    if(*beg)put_string(beg);
    if(is_cat) put_cmd(opCAT);
    return(0);
}/*expand_str*/

static long max_out=0;/*Top of allocated output array.*/
static long *ttop_out=NULL;/*Top of output array.*/
static char ***out_str=NULL;/*Reference to output array.*/

static void putstr(char *str)/*Output routine*/
{
     if(str[g_debug_offset]=='\0')return;/*Do not output empty strings.*/
    if(!(*ttop_out<max_out))
      if((*out_str=
         (char**)realloc(*out_str,
              (max_out+=DELTA_MAX_OUT)*sizeof(char *)))==NULL)
           halt(NOTMEMORY,NULL);
    (*out_str)[*ttop_out]=new_str(str);
    (*ttop_out)++;
}/*putstr*/

static int *group=NULL;/*Array af integer numbers corresponding to
                        numbers of tokens. 0 element is a length of a group.*/
static int put_text(char *str)/*Puts text into text_token and returns
                                old length of the text_token, because it is
                                just a pointer to the begin of allocated
                                text.*/
{
  int len=s_len(str);
  int mem=text_token_len;
  while(text_token_len+len+2>max_text_token_len){
     if((
        text_token=(char *)realloc(text_token,
           (max_text_token_len+=DELTA_TEXT_SIZE)*sizeof(char))
        )==NULL)halt(NOTMEMORY,NULL);
  }
  s_let(str,text_token+text_token_len);
  text_token_len+=(len+1);
  return(mem);
}/*put_text*/

static void put_token(int n, int type)/*Creates token correponding to the
                                      text_token[n] of type 'type'.*/
{
   if(!(token_top<max_token)){
     if((
        token=(struct token_struct *)realloc(token,
           (max_token+=DELTA_TOKEN_SIZE)*sizeof(struct token_struct))
        )==NULL)halt(NOTMEMORY,NULL);
   }
   token[token_top].n=n;
   token[token_top].type=type;
   token[token_top].current_line=get_current_line();
   token[token_top++].current_col=get_current_col();
}/*put_token*/

static int pre_scan(char *endscan1,char *endscan2)/*Converts input stream
   into set of tokens and creates primary group. It scans until the word
   endscan1 or endscan2 occur. If endscan1, the procedure returns 1;
   if endscan2, it returns 2.*/
{

  char str[MAX_STR_LEN];
  char bracked_balance=0;
  int i,retval=0;

      token_top=0;
      text_token=get_mem(max_text_token_len=DELTA_TEXT_SIZE,sizeof(char));
      text_token_len=0;
      token=get_mem(max_token=DELTA_TOKEN_SIZE,sizeof(struct token_struct));

      /* Set in 0 cell the fictitious operation with minimal priority:*/
      *str=opNOP;*(str+1)=0;
      put_token(put_text(str),tpOP);

      do{
         if(*sc_get_token(str)=='('){
           put_token(put_text(str),tpOPENB);
           bracked_balance++;
         }else if(*str==')'){
           put_token(put_text(str),tpCLOSEB);
           bracked_balance--;
           if(bracked_balance<0)halt(SUPERFLUOUSrBRACKET,NULL);
         }else if(*str=='"'){
            char old_esc=esc_char;
            esc_char='"';
            hash_enable=0;
            put_token(put_text(sc_get_token(str)),tpLINE);
            if(str[s_len(str)-1]=='\n')halt(UNTERMINATEDSTR,NULL);
            sc_get_token(str);
            esc_char=old_esc;
         }else if(!s_scmp(str,endscan1)){
            retval=1;
            break;
         }else if(!s_scmp(str,endscan2)){
            retval=2;
            break;
         }else{
             int *tmp;
             if(  (tmp=lookup(str,command_table))==NULL )
                halt(UNEXPECTED,str);
             switch(*tmp){
                case opEQ:
                  *str=opEQ;*(str+1)=0;
                  put_token(put_text(str),tpOP);
                  break;
                case opNE:
                  *str=opNE;*(str+1)=0;
                  put_token(put_text(str),tpOP);
                  break;
                case opAND:
                  *str=opAND;*(str+1)=0;
                  put_token(put_text(str),tpOP);
                  break;
                case opOR:
                  *str=opOR;*(str+1)=0;
                  put_token(put_text(str),tpOP);
                  break;
                case opLT:
                  *str=opLT;*(str+1)=0;
                  put_token(put_text(str),tpOP);
                  break;
                case opGT:
                  *str=opGT;*(str+1)=0;
                  put_token(put_text(str),tpOP);
                  break;
                case opLE:
                  *str=opLE;*(str+1)=0;
                  put_token(put_text(str),tpOP);
                  break;
                case opGE:
                  *str=opGE;*(str+1)=0;
                  put_token(put_text(str),tpOP);
                  break;
                case opXOR:
                  *str=opXOR;*(str+1)=0;
                  put_token(put_text(str),tpOP);
                  break;
                case opNOT:
                  *str=opNOT;*(str+1)=0;
                  put_token(put_text(str),tpOP);
                  break;
                case opCAT:
                  *str=opCAT;*(str+1)=0;
                  put_token(put_text(str),tpOP);
                  break;
                case opISDEF:
                  *str=opISDEF;*(str+1)=0;
                  put_token(put_text(str),tpOP);
                  break;
                default:
                  halt(UNEXPECTED,str);
             }/*switch*/
         }/*else*/
      }while(1);

      if(bracked_balance)halt(SUPERFLUOUSlBRACKET,NULL);

      *(group=get_mem(token_top+1,sizeof(int)))=token_top-1;/*-1 :skip [0] */

      for(i=1;i<token_top;i++)group[i]=i;
      return(retval);
}/*pre_scan*/

static int closebracket(int startpos,int *group)/*Returns position of
                                                  nearest balanced ')'.*/
{
 int balance=0;
 int i;
 for (i=startpos; !(i>*group);i++){
    if(token[group[i]].type==tpOPENB)balance++;
    else if(token[group[i]].type==tpCLOSEB)balance--;
    if (balance<0)return(0);
    else if (balance==0)return(i);
 }
 return(0);
}/*closebracket*/

void typeFilePosition(char *fname, long line, int col, FILE *outstream, FILE *logfile)
{
  long i;
  FILE *f;
  char tmp[MAX_STR_LEN];

    if (*fname==0)
      return;

    if((f=open_file_follow_system_path(fname,"rt"))==NULL){
          fprintf(stderr,"Internal error\n");
          exit(20);
    }/*if((f=open_file_follow_system_path(fname,"rt"))==NULL)*/
    for(i=0;i!=line;i++)
      if(fgets(tmp, MAX_STR_LEN,f)==NULL){
         fprintf(stderr,DISKPROBLEM);
         return;
      }

    fclose(f);

    if(tmp[s_len(tmp)-1]=='\n')
       tmp[s_len(tmp)-1]=0;

    if(outstream!=NULL){
        fprintf(outstream,"%s\n",tmp);
        for(i=1;i<col;i++)
          fprintf(outstream," ");    
        fprintf(outstream,"^\n");
        fflush(outstream);
    }/*if(outstream!=NULL)*/

    if(logfile!=NULL){
        fprintf(logfile,"%s\n",tmp);
        for(i=1;i<col;i++)
          fprintf(logfile," ");    
        fprintf(logfile,"^\n");
        fflush(logfile);
    }/*if(logfile!=NULL)*/

}/*typeFilePosition*/ 

/*Types 'line' string from file 'fname' and marks position 'col':*/
void pointposition(char *fname, long line, int col)
{
    fprintf(stderr,TOKENERROR, fname,line, col);
    if (*fname==0)/* Point position is unavailable*/
       fprintf(stderr,POSITIONINFOUNAVAIL);
    if(log_file!=NULL){
      fprintf(log_file,TOKENERROR, fname, line, col);
      if (*fname==0)/* Point position is unavailable*/
         fprintf(log_file,POSITIONINFOUNAVAIL);
    }
    typeFilePosition(fname, line, col, stderr, log_file);
}/*pointposition*/

static void t_error(int token_n, char *message)/*If an error occure during
                                                 token expansing...*/
{
  current_line=token[token_n].current_line;
  current_col=token[token_n].current_col;
  s_let(get_input_file_name(),inp_f_name);
  sc_done();
  halt(message,NULL);
}/*t_error*/

static void token2line(int n)/*Puts token into polish_line*/
{
  active_token=n;
  switch(token[n].type){
    case tpOP:
       put_cmd(text_token[token[n].n]);
       break;
    case tpBOOL:
       put_bool(text_token[token[n].n]);
       break;
    case tpLINE:
       if(expand_str(text_token+token[n].n))
          t_error(n,macro_error);
       break;
    default:
       halt(INTERNALERROR,NULL);
       break;
  }
  active_token=0;
}/*token2line*/

static char trunslate(int *group)/*Recursive compiler.*/
{
  int i,j;
  int **g_line=NULL;/*Array of all groups*/
  int top_g_line=0;

  /* Remove possible extra brackets:*/
  if (*group>2) while(token[*(group+1)].type==tpOPENB){
     if ((i=closebracket(1,group))==0)t_error(*(group+1),NORIGHTBRACKET);
     if(i==*group){
        if(*group < 3)t_error(*(group+1),SYNTAXERROR);
        (*group)-=2;
        for(i=1; !(i>(*group));i++)group[i]=group[i+1];
     }else break;
  }
  /* Allocates memory for g_line:*/
  g_line=get_mem(*group+1,sizeof(int *));
  {/*block begin*/
     /* Creating all arguments and operations:*/
     int k;
     for(i=1;!(i>(*group));){
       if(token[group[i]].type==tpOPENB){
          if ((j=closebracket(i,group))==0)t_error(group[i],NORIGHTBRACKET);
          g_line[top_g_line]=get_mem(j-i, sizeof (int));
          g_line[top_g_line][0]=j-i-1;
          for(i++,k=1;i<j;i++,k++)
            g_line[top_g_line][k]=group[i];
          top_g_line++;i++;
       }else if(token[group[i]].type==tpCLOSEB)t_error(group[i],UNEXPrBRACK);
       else{/* Only single token may remains*/
          g_line[top_g_line]=get_mem(2, sizeof (int));
          g_line[top_g_line][0]=1;
          g_line[top_g_line][1]=group[i];
          top_g_line++;i++;
       }
     }
  }/*block end*/
  g_line[top_g_line]=get_mem(2, sizeof (int));
  g_line[top_g_line][0]=1;
  g_line[top_g_line][1]=0;/*This is the minimal operation*/

  {/*block bedin*/
      /* Reset all operations after its right argument:*/
      int *ptr;
      for (i=10;i<30;i++)/* Cycle for all operation */
        for(j=0;j<top_g_line;j++){/*Cycle for all groups*/
          ptr=g_line[j];
          if( ((*ptr)==1)&&
              (token[ptr[1]].type==tpOP)&&
              (text_token[token[ptr[1]].n]==i)
            ){
                /* Right argument can NOT be an operation:*/
                if(
                    ((g_line[j+1][0])==1)&&
                    (token[g_line[j+1][1]].type==tpOP)
                )t_error(g_line[j][1],SYNTAXERROR);

                for(j++;
                        !(((g_line[j][0])==1)&&
                          (token[g_line[j][1]].type==tpOP)&&
                          (!(text_token[token[g_line[j][1]].n]<i))
                         ) ;j++)
                            g_line[j-1]=g_line[j];
                g_line[j-1]=ptr;
                j--;
          }
      }
  }/*block end*/
  {/*block bedin*/
       /*Build polish line and check stack:*/
      char *stack,tmp;
      j=0;
      stack=get_mem(top_g_line+1,sizeof(char));

      for(i=0;i<top_g_line;i++){/*Cycle for all groups*/
        if(g_line[i][0]==1){
           token2line(g_line[i][1]);
           if (token[g_line[i][1]].type==tpOP){
             switch(text_token[token[g_line[i][1]].n]){
                case opISDEF:
                   if ((j==0)||(stack[j-1]!=tpLINE))
                      t_error(g_line[i][1],SYNTAXERROR);
                   stack[j-1]=tpBOOL;
                   break;
                case opNOT:
                   if ((j==0)||(stack[j-1]!=tpBOOL))
                      t_error(g_line[i][1],NONBOOL);
                   break;
                case opLT:
                case opLE:
                case opGT:
                case opGE:
                case opEQ:
                case opNE:
                   if ((j<2)||(stack[j-1]!=tpLINE)||(stack[j-2]!=tpLINE))
                      t_error(g_line[i][1],SYNTAXERROR);
                   stack[--j - 1]=tpBOOL;
                   break;
                case opAND:
                case opXOR:
                case opOR:
                   if ((j<2)||(stack[j-1]!=tpBOOL)||(stack[j-2]!=tpBOOL))
                      t_error(g_line[i][1],SYNTAXERROR);
                   j--;
                   break;
                case opCAT:
                   if ((j<2)||(stack[j-1]!=tpLINE)||(stack[j-2]!=tpLINE))
                      t_error(g_line[i][1],NONSTRING);
                   j--;
                   break;
             }
           }else{
             stack[j++]=token[g_line[i][1]].type;
           }
           free_mem(&(g_line[i]));
        }else {
           /* Verify deep of the recursion:*/
           if (++rec>LAST_RECURSION)t_error(g_line[i][1],TOOMANYRECS);
           stack[j++]=trunslate(g_line[i]);
           rec--;
           free_mem(&(g_line[i]));
        }
      }
      if(j!=1)halt(SYNTAXERROR,NULL);
      tmp=*stack;
      free_mem(&stack);
      free_mem(&(g_line[top_g_line]));
      free_mem(&g_line);
      return(tmp);
  }/*block end*/
}/*trunslate*/

static void proc_if(void)
{
  pre_scan("then","then");
    rec=0;
  trunslate(group);
  free_mem(&group);
  free_mem(&text_token);
  free_mem(&token);
  /*Create new info cell:*/
  ifdepth++;
  if(!(ifdepth < max_ifdepth))
     if((ifstack=realloc(ifstack,
            (max_ifdepth+=DELTA_IF_DEPTH)*
              sizeof(struct control_struct)))==NULL)
              halt(NOTMEMORY,NULL);
  ifstack[ifdepth].sign=0;/*Clear all bits*/
  set_bit(&(ifstack[ifdepth].sign),bitIF);/* Mark current
                                             cell*/
  ifstack[ifdepth].line=*ttop_out;/*Store current
                                          line number*/
  ifstack[ifdepth].offset=ptr+1;/*Store offset.
                                        +1 due to tpSTRING*/
  put_string("    ");/*Reserve 4 bytes*/
  put_cmd(cmIF);

}/*proc_if*/

static int proc_loop(void)
{
int retval;
  retval=pre_scan("do","loop");
  rec=0;
  trunslate(group);
  free_mem(&group);
  free_mem(&text_token);
  free_mem(&token);
  return(retval);
}/*proc_loop*/

static void marg_add(char *str)
{
  while(m_ptr+s_len(str)+2>max_m_ptr)
     if((macro_arg=(char *)realloc(macro_arg,
       (max_m_ptr+=DELTA_MACRO_ARG)*sizeof(char)))==NULL)
        halt(NOTMEMORY,NULL);
  if (m_ptr>MAX_MACRO_ARG){
      macro_arg[max_m_ptr-1]='\0';
      halt(TOOLONGMACROARG,macro_arg);
  }
  while((macro_arg[m_ptr]=*str++)!=0)m_ptr++;/*This should copy terminating
                                   zero, but do not move m_ptr through it.*/
  /*while(*str)macro_arg[m_ptr++]=*str++;*/
}/*marg_add*/

static void set_def_arg(struct def_struct *tmp)
{
  if(!(tmp->top<tmp->maxtop))
     if( ( tmp->str = realloc(tmp->str,
           (tmp->maxtop+=2)*sizeof(char *) ) )==NULL)
              halt(NOTMEMORY,NULL);
     (tmp->str)[tmp->top]=new_str(macro_arg+m_c_ptr);
  tmp->top++;
}/*set_def_arg*/

static struct for_struct{
   char *iterator;
   word currentiteration;
   word nargs;
   word maxargs;
   char **args;
} *for_stack=NULL;
static word top_for_stack=0;
static word max_for_stack=0;

static void set_list_arg(struct for_struct *tmp)
{
char esc_line[3];
  if(!(tmp->nargs<tmp->maxargs))
     if( ( tmp->args = realloc(tmp->args,
           (tmp->maxargs+=DELTA_FOR_ARGS)*sizeof(char *) ) )==NULL)
              halt(NOTMEMORY,NULL);
  *esc_line=esc_char;esc_line[1]='*';esc_line[2]=0;
  if(!s_scmp(macro_arg+m_c_ptr,esc_line)){struct def_struct *cdef=NULL;word i;
     if(deftop<0)
        halt(UNEXPECTED,macro_arg+m_c_ptr);
     if (  (cdef=lookup(defnames[deftop],def_table)) == NULL  )
         halt(INTERNALERROR,NULL);
     for(i=1;i<cdef->top;i++){
        if(!(tmp->nargs<tmp->maxargs))
           if( ( tmp->args = realloc(tmp->args,
                 (tmp->maxargs+=DELTA_FOR_ARGS)*sizeof(char *) ) )==NULL)
                    halt(NOTMEMORY,NULL);
        (tmp->args)[tmp->nargs++]=new_str(cdef->str[i]);
     }
     return;
  }
  (tmp->args)[tmp->nargs++]=new_str(macro_arg+m_c_ptr);
}/*set_list_arg*/

static void read_def_arg(char *str)
{
int n;
struct def_struct *tmp=NULL;

  if(deftop<0)
     halt(UNEXPECTED,str);
  if(*sc_get_token(str)!='(')
     halt(OBRACKEDEXPECTED,str);
  if(sscanf(sc_get_token(str),"%d",&n)!=1)
     halt(INVALIDNUMBER,str);
  if(*sc_get_token(str)!=')')
     halt(CBRACKEDEXPECTED,str);

  if (  (tmp=lookup(defnames[deftop],def_table)) == NULL  )
      halt(INTERNALERROR,NULL);
   if((n<0)||(!(n<tmp->top)))
     return;
   gotomacro(tmp->str[n],-1,0,0);
}/*read_def_arg*/

static int proc_def(char *str, int cmd);

void read_list(void (*setarg)(void *),void *tmp,char *str,int fullexp)
{
int isrepeat,balance;
         if(g_keep_spaces)
        /*!!!*/rm_spaces(" ");
        if(*sc_get_token(str)!='(')halt(OBRACKEDEXPECTED,str);
        balance=0;
        m_c_ptr=m_ptr;
        while((*sc_get_token(str)!=')')||(balance)){
          do{
             isrepeat=0;
             if(*str=='(')balance++;
             else if(*str==')')balance--;

             if (balance<0)halt(SUPERFLUOUSrBRACKET,NULL);
             if((*str==comma_char)&&(balance==0)){/*Argument found*/
                 if(m_ptr==m_c_ptr)
                    marg_add("");
                 setarg(tmp);
                 m_ptr=m_c_ptr;
             }else{
                if((*str==esc_char)&&(str[1]==0)){/* Check is this argument*/
                  sc_get_token(str);
                  if ((fullexp)&&(*str=='(' )){/*Quotation*/
                     marg_add(sc_get_token(str));
                     if(*sc_get_token(str)!=')' )
                        halt(CBRACKEDEXPECTED,str);
                  }else{int tag,mem_m_c_ptr;
                     mem_m_c_ptr=m_c_ptr;
                     tag=!proc_def(str,-1);
                     m_c_ptr=mem_m_c_ptr;
                     if(tag){char esc_line[2];
                        esc_line[1]=0;*esc_line=esc_char;
                        marg_add(esc_line);
                        isrepeat=1;
                     }
                  }
                }else
                   marg_add(str);
             }
          }while(isrepeat);
        }
   if(m_ptr!=m_c_ptr)
      setarg(tmp);
   else{
         marg_add("");
         setarg(tmp);
   }
   if(g_keep_spaces)
   /*!!!*/add_spaces(" ");
   m_ptr=m_c_ptr;
}/*read_list*/

static void set_set_arg(char **tmp)
{

   if(*tmp==NULL)
      *tmp=new_str(macro_arg+m_c_ptr);
   else if(**tmp == 1)
      letnv(macro_arg+m_c_ptr,*tmp,MAX_STR_LEN);
   else
      halt(UNEXPECTED,",");
}/*set_set_arg*/

static int eq_set(char *str)
{
 char nam[MAX_STR_LEN], val[MAX_STR_LEN],*tmp;
   *(tmp=nam)=1;
   read_list((void (*)(void *))set_set_arg,&tmp,str,1);
   *(tmp=val)=1;
   read_list((void (*)(void *))set_set_arg,&tmp,str,1);
   if(  (str=lookup(nam,set_table))==NULL)return(!s_scmp(val,""));
   return(!s_scmp(val,str));
}/*eq_set*/

static int if_def(char *str, HASH_TABLE table)
{
 char nam[MAX_STR_LEN],*tmp;
   *(tmp=nam)=1;
   read_list((void (*)(void *))set_set_arg,&tmp,str,1);
   return(lookup(nam,table)!=NULL);
}/*if_def*/

/*ATTENTION! l_parseVersion does NOT preserve the contents of theVersion!!*/
void l_parseVersion(char *theVersion, int *maxN, int *minN, char **additional)
{
char *ptr,*minC,c='\0';
   *additional=NULL;
   for(ptr=theVersion; (*ptr!='\0')&&(*ptr!='.'); ptr++);
   if(ptr!='\0'){/*ptr=.*/
      *ptr++='\0';/*treminate so that theVersion is a major number*/
      for(minC=ptr; (*ptr!='\0')&&set_in(*ptr,digits); ptr++);
      if(*ptr!='\0'){/*appendix present*/
         /*A problem: e.g. 1.11a: ptr->a,minC->11a So terminate here minC but store
           a char at terminator:*/
         c=*(*additional=ptr);
         *ptr='\0';
      }/*if(ptr!='\0')*/
      if(*minC=='\0')
         *minN=-1;/*Minor number is absent*/
      else{
         if(sscanf(minC,"%d",minN)!=1)
            halt(CANNOTCONVERTTONUMBER,minC);
      }
      if(c!='\0')**additional=c;/*minC was terminated, restore the *additional*/
   }else/*if(ptr!='\0')*/
      *minN=-1;/*Minor number is absent*/
   if(sscanf(theVersion,"%d",maxN)!=1)
      halt(CANNOTCONVERTTONUMBER,theVersion);
}/*l_parseVersion*/
/*Returns -1 if one<two, +1 if one>two, or 0 if coincide
  ATTENTION! l_cmpRevision does NOT preserve the contents of both one and two!!*/
int l_cmpRevision(char *one, char *two)
{
int maxNone,minNone,maxNtwo,minNtwo;
char *additionalOne, *additionalTwo;

   l_parseVersion(one,&maxNone,&minNone,&additionalOne);
   l_parseVersion(two,&maxNtwo,&minNtwo,&additionalTwo);
   /*Compare major numbers:*/
   if(maxNone<maxNtwo)return -1;
   if(maxNone>maxNtwo)return 1;
   /*Compare minor numbers:*/
   if( (minNone<0)&&(minNtwo<0) )return 0;/*No minor numbers*/
   if(minNone<0)return -1;/*one minor absent while two present, so two>one*/
   if(minNtwo<0)return 1;/* two minor absent while one present, so one<two*/
   if(minNone<minNtwo)return -1;
   if(minNone>minNtwo)return 1;

   if( (additionalOne==NULL)&&(additionalTwo==NULL)  )return 0;/*No additional*/
   if(additionalOne==NULL)return -1;/*one additional absent while two present*/
   if(additionalTwo==NULL)return 1;/* two additionalabsent while two present*/
   maxNone=s_cmp(additionalOne,additionalTwo);
   if(maxNone<0)return -1;
   if(maxNone>0)return 1;
   return 0;
}/*l_cmpRevision*/

static int proc_def(char *str, int cmd)
{
int i;
   if(cmd==-1){int *cmd_ptr;
      if(  (cmd_ptr=lookup(str,command_table))==NULL)
        cmd=0;
      else
        cmd=*cmd_ptr;
   }
   if(top_for_stack){/*FOR is active*/
      /*Verify is it FOR parameter:*/
      for(i=top_for_stack-1;!(i<0);i--)
         if(!s_scmp(str,for_stack[i].iterator)){
             if(*sc_get_token(str)!='(') halt(OBRACKEDEXPECTED,str);
             if(*sc_get_token(str)!=')') halt(CBRACKEDEXPECTED,str);
             gotomacro(for_stack[i].args[for_stack[i].currentiteration],
                                                              -1,0,0);
             return(1);
         }
      /*Verify is it ENDFOR:*/
      if(cmd==cm_ENDFOR){struct for_struct *forstack=
                                                  for_stack+top_for_stack-1;
         if(++(forstack->currentiteration) < forstack->nargs)
            break_include();
         else{
            for(i=0; i<forstack->nargs; i++)
                free_mem(forstack->args +i);
            free_mem(&(forstack->iterator));
            forstack->nargs=0;
            top_for_stack--;
         }
         return(1);
      }
   }/*if(top_for_stack)*/

   {/*Verify is it a macro:*/
    struct def_struct *tmp=NULL;
      if (  (tmp=lookup(str,def_table)) != NULL  ){
         if(isdebug)
           message(GOTOMACRO,str);
         for(i=1; i<tmp->top; i++){
            free_mem(&((tmp->str)[i]));
         }

         tmp->top=1;
         read_list((void (*)(void *))set_def_arg,tmp,str,0);
         if(!(++deftop<maxdeftop))
            if(  (defnames=realloc(defnames,
               (maxdeftop+=DELTA_DEF_TOP)*sizeof(char *) ) )==NULL)
                  halt(NOTMEMORY,NULL);
         defnames[deftop]=tmp->str[0];
         gotomacro(tmp->filename,tmp->offset, tmp->line, tmp->col);
         return(1);
      }/*if*/
   }/*End macro operation*/
   if (deftop>-1){/*DEF is active*/
      /*Verify is it ENDDEF:*/
      if(cmd==cm_ENDDEF){
         break_include();
         deftop--;
         return(1);
      }
      /*Verify is it a macro argument:*/
      if(*str=='#'){
          read_def_arg(str);
          return(1);
      }
   }/*if (deftop>-1)*/
   switch(cmd){
      case cm_FOR:{char *fn;int ccol; long clin,cpos;
         if(!(top_for_stack<max_for_stack)){
            if ( (for_stack=realloc(for_stack,
               (max_for_stack+=DELTA_FOR_STACK)*sizeof(struct for_struct)))==
                   NULL) halt(NOTMEMORY,NULL);
            for(i=top_for_stack; i<max_for_stack; i++){
               for_stack[i].iterator=NULL;
               for_stack[i].args=NULL;
               for_stack[i].maxargs=for_stack[i].nargs=0;
            }
         }
         /* iterator variable:*/
         fn=NULL;
         read_list((void (*)(void *))set_set_arg,&fn,str,1);
         for_stack[top_for_stack].iterator=fn;
         for_stack[top_for_stack].nargs=0;
         read_list((void (*)(void *))set_list_arg,
             for_stack+top_for_stack,str,1);
         for_stack[top_for_stack].currentiteration = 0;
         if (for_stack[top_for_stack].nargs==0)
            halt(ARGSEXPECTED,NULL);
         if((fn=get_input_file_name())==NULL)
            halt(FORNATALOWEDHERE,NULL);
         cpos=get_current_pos();
         clin=get_current_line();
         ccol=get_current_col();
         if(for_stack[top_for_stack].nargs >1){
            gotomacro(fn,cpos,clin,ccol);
            multiplylastinclude(for_stack[top_for_stack].nargs-2);
         }
         top_for_stack++;
         return(1);
      }/*case cm_FOR*/
      case cm_SET:{char *nam=NULL,*val=NULL;
         read_list((void (*)(void *))set_set_arg,&nam,str,1);
         read_list((void (*)(void *))set_set_arg,&val,str,1);
         install(nam,val,set_table);
         return(1);
      }
      case cm_GET:{ char tmp[MAX_STR_LEN],*ptr=tmp;
         *ptr=1;
         read_list((void (*)(void *))set_set_arg,&ptr,str,1);
         if(  ( ptr=lookup(tmp,set_table) )==NULL  ){
            macro_arg[m_c_ptr]='\0';
            return(1);
         }else if(*ptr=='\0')
            macro_arg[m_c_ptr]='\0';
         gotomacro(ptr,-1,0,0);
         return(1);
      }
      case cm_GETENV:{ char tmp[MAX_STR_LEN],*ptr=tmp;
         *ptr=1;
         read_list((void (*)(void *))set_set_arg,&ptr,str,1);
         if(  (ptr=getenv(tmp))==NULL )
            *(ptr=tmp)='\0';
         
         gotomacro(ptr,-1,0,0);
         return(1);
      }
      case cm_ADDSPACES:{ char tmp[MAX_STR_LEN],*ptr=tmp;
         *ptr=1;
         read_list((void (*)(void *))set_set_arg,&ptr,str,1);
         add_spaces(tmp);
         return(1);
      }
      case cm_VERSION:{
         gotomacro(currentRevisionNumber,-1,0,0);
         return(1);
      }
      case cm_RMSPACES:{ char tmp[MAX_STR_LEN],*ptr=tmp;
         *ptr=1;
         read_list((void (*)(void *))set_set_arg,&ptr,str,1);
         rm_spaces(tmp);
         return(1);
      }
      case cm_ADDDELIMITERS:{ char tmp[MAX_STR_LEN],*ptr=tmp;
         *ptr=1;
         read_list((void (*)(void *))set_set_arg,&ptr,str,1);
         add_delimiters(tmp);
         return(1);
      }
      case cm_RMDELIMITERS:{ char tmp[MAX_STR_LEN],*ptr=tmp;
         *ptr=1;
         read_list((void (*)(void *))set_set_arg,&ptr,str,1);
         rm_delimiters(tmp);
         return(1);
      }
      case cm_UNSET:{ char tmp[MAX_STR_LEN],*ptr=tmp;
         *ptr=1;
         read_list((void (*)(void *))set_set_arg,&ptr,str,1);
         uninstall(tmp,set_table);
         return(1);
      }
      case cm_CMDLINE:{ char tmp[MAX_STR_LEN],*ptr=tmp;int n=0;
         *ptr=1;
         read_list((void (*)(void *))set_set_arg,&ptr,str,1);
         if (sscanf(tmp,"%d",&n)!=1)
            halt(NUMBEREXPECTED,tmp);
         if( (n<1)||(n>run_argc) ){
            macro_arg[m_c_ptr]='\0';
            return(1);
         }
         gotomacro(run_argv[n-1],-1,0,0);
         return(1);
      }
      case cm_RMARG:{ char tmp[MAX_STR_LEN],*ptr=tmp;int n=0;
         *ptr=1;
         read_list((void (*)(void *))set_set_arg,&ptr,str,1);
         if (sscanf(tmp,"%d",&n)!=1)
            halt(NUMBEREXPECTED,tmp);
         macro_arg[m_c_ptr]='\0';

         if( (n<1)||(n>run_argc) )
            return(1);
         free(run_argv[n-1]);
         for(; n<run_argc;n++)
            run_argv[n-1]=run_argv[n];
         run_argc--;
         return(1);
      }
      case cm_ARGC:{ char tmp1[MAX_STR_LEN],tmp2[NUM_STR_LEN];int i=0;
         *tmp1='\0';
         for(i=1;i<run_argc;i++){
            sprintf(tmp2,"%d,",i);
            s_cat(tmp1,tmp1,tmp2);
         }
         if(run_argc){
            long2str(tmp2,i);
            s_cat(tmp1,tmp1,tmp2);
         }
         gotomacro(tmp1,-1,0,0);
         return(1);
      }/*case cm_ARGC:*/

      case cm_POP:{char tmp[MAX_STR_LEN],*name=tmp,*ptr,*val,*newval;
         *name=1;
         read_list((void (*)(void *))set_set_arg,&name,str,1);
         if( (val=lookup(name,set_table) )== NULL){
            *name=1;
            read_list((void (*)(void *))set_set_arg,&name,str,1);
            return(1);
         }
         ptr=val;
         while((*ptr!=0)&&(*ptr!=comma_char))ptr++;
         if (*ptr==0){
            name=NULL;
            newval=new_str(val);
         }else{
            *ptr=0;
            name=new_str(name);
            newval=new_str(val);
            val=new_str(ptr+1);
         }
         uninstall(tmp,set_table);
         if(name!=NULL)install(name,val,set_table);
         /*Now only newval is necessary*/
         name=NULL;
         read_list((void (*)(void *))set_set_arg,&name,str,1);
         install(name,newval,set_table);
         return(1);
      }
      case cm_PUSH:
      case cm_PUSHQ:{char *name=NULL,*val=NULL,*newval;
         read_list((void (*)(void *))set_set_arg,&name,str,1);
         read_list((void (*)(void *))set_set_arg,&val,str,1);
         if( (newval=lookup(name,set_table) )== NULL){
            install(name,val,set_table);
         }else{
             if( (val=realloc(val,
                  (s_len(val)+s_len(newval)+2)*(sizeof(char*)))
                 )==NULL)halt(NOTMEMORY,NULL);
             if (cmd==cm_PUSH){
                s_cat(val,val,",");
                s_cat(val,val,newval);
             }else{/*cmd==cm_PUSHQ*/
                s_cat(val,",",val);
                s_cat(val,newval,val);
             }
             uninstall(name,set_table);
             install(name,val,set_table);
         }
         return(1);
      }
      case cm_ADD:{char *name=NULL,tmp[MAX_STR_LEN],*val=tmp;long orig,delt;
         read_list((void (*)(void *))set_set_arg,&name,str,1);
         *val=1;
         read_list((void (*)(void *))set_set_arg,&val,str,1);
         if (sscanf(val,"%ld",&delt)!=1)
            halt(NUMBEREXPECTED,val);
         if( (val=lookup(name,set_table) )== NULL){
            install(name,new_str(tmp),set_table);
         }else{
             if (sscanf(val,"%ld",&orig)!=1)
                halt(CANNOTCONVERTTONUMBER,val);
             long2str(tmp,orig+delt);
             uninstall(name,set_table);
             install(name,new_str(tmp),set_table);
         }
         return(1);
      }
      case cm_REMOVE:{char tmp[MAX_STR_LEN],*name=tmp,*val=tmp, *ptr;
                      set_of_char patterns;int wasch=0;
         *name=1;
         read_list((void (*)(void *))set_set_arg,&name,str,1);
         name=lookup(name,set_table);
         *val=1;
         read_list((void (*)(void *))set_set_arg,&val,str,1);
         if (name == NULL) return(1);
         set_str2set(val,patterns);
         /*Now name points to value */
         for(ptr=name,val=tmp;*ptr!='\0';ptr++)
            if(!set_in(*ptr,patterns))*val++=*ptr;
            else wasch=1;
         *val='\0';
         if(wasch)
            s_let(tmp,name);/* No danger, tmp is always shorter then name*/
         return(1);
      }
      case cm_REPLACE:{char olds[MAX_STR_LEN],news[MAX_STR_LEN],
                            tmp[MAX_STR_LEN],*name=NULL,*ptr;

         read_list((void (*)(void *))set_set_arg,&name,str,1);
         *(ptr=olds)='\1';
         read_list((void (*)(void *))set_set_arg,&ptr,str,1);
         *(ptr=news)='\1';
         read_list((void (*)(void *))set_set_arg,&ptr,str,1);
         if((ptr=lookup(name,set_table))==NULL){
            free_mem(&name);
            return(1);
         }
         install(
            name,new_str(s_replace(olds,news,s_let(ptr,tmp))),
            set_table);
         return(1);
      }
      case cm_REPLACEALL:{char olds[MAX_STR_LEN],news[MAX_STR_LEN],
                            tmp[MAX_STR_LEN],*name=NULL,*ptr;int i;

         read_list((void (*)(void *))set_set_arg,&name,str,1);
         *(ptr=olds)='\1';
         read_list((void (*)(void *))set_set_arg,&ptr,str,1);
         *(ptr=news)='\1';
         read_list((void (*)(void *))set_set_arg,&ptr,str,1);
         if((ptr=lookup(name,set_table))==NULL){
            free_mem(&name);
            return(1);
         }
         s_let(ptr,tmp);ptr=NULL;
         while(      ( i=s_pos(olds,tmp) ) != -1   )
              ptr=s_insert(news,s_del(tmp,tmp,i,s_len(olds)),i);
         if (ptr!=NULL)
            install(name,new_str(tmp), set_table);
         else
            free(name);
         return(1);
      }

      case cm_READ:{char *name=NULL,tmp[MAX_STR_LEN],*mes=tmp;
         read_list((void (*)(void *))set_set_arg,&name,str,1);
         *mes=1;
         read_list((void (*)(void *))set_set_arg,&mes,str,1);
         printf("%s",tmp);/*PPP*/
         fgets(tmp, MAX_STR_LEN,stdin);/*PPP*/
         tmp[s_len(tmp)-1]=0;/*Avoid '\n' at the end of line*/
         install(name,new_str(tmp),set_table);
         return(1);
      }
      case cm_SCAN:{ char tmp[MAX_STR_LEN],*ptr=tmp;
         *ptr=1;
         read_list((void (*)(void *))set_set_arg,&ptr,str,1);
         gotomacro(ptr,-1,0,0);
         return(1);
      }
      case cm_MESSAGE:{char tmp[MAX_STR_LEN],*ptr=tmp;
         *ptr=1;
         read_list((void (*)(void *))set_set_arg,&ptr,str,1);
         message(tmp,NULL);
         return(1);
      }
      case cm_CHANGEESC:{char esc_line[2];set_of_char valid_esc;
         set_str2set(VALID_ESC_CHAR,valid_esc);
         *esc_line=esc_char;esc_line[1]=0;
         if(!set_in(*sc_get_token(str),valid_esc))
             halt(INVALDESCCHAR,str);
         uninstall(esc_line,command_table);
         if(q_char==esc_char)
            q_char=*str;
         *esc_line=esc_char=*str;
         register_command(esc_line,cm_CHANGEESC);
         if(isdebug)
           message(NEWESCAPE,esc_line);
         return(1);
      }
      case cm_CHECKMINVERSION:{char *required,*current,*ptr; int res;
         ptr=required=get_mem(MAX_STR_LEN,sizeof(char));
         *ptr=1;
         current=new_str(currentRevisionNumber);
         read_list((void (*)(void *))set_set_arg,&ptr,str,1);
         /*Now maxN - major rev. num., minN - minor (or -1), additional- the rest (or NULL)*/
         res=l_cmpRevision(required,current);
         free(current);free(required);
         if(res>0)
            halt(INCORRECTVERSION,currentRevisionNumber);
         return(1);
      }
      case cm_ERROR:{char tmp[MAX_STR_LEN],*ptr=tmp;
         *ptr=1;
         read_list((void (*)(void *))set_set_arg,&ptr,str,1);
         halt(ERRORDIRECTIVE,tmp);
         return(1);
      }
      case cm_NEWCOMMENT:{char tmp[MAX_STR_LEN],*ptr=tmp;
         *ptr=1;
         read_list((void (*)(void *))set_set_arg,&ptr,str,1);
         if(s_len(tmp)!=1)
            halt(WRONGNEWCOMMENT,tmp);
         new_comment(*tmp);
         if(isdebug)
            message(NEWCOMMENTSET,tmp);
         return(1);
      }
      case cm_NEWCOMMA:{char tmp[MAX_STR_LEN],*ptr=tmp;
         *ptr=1;
         read_list((void (*)(void *))set_set_arg,&ptr,str,1);
         if(s_len(tmp)!=1)
            halt(WRONGNEWCOMMA,tmp);
         comma_char=*tmp;
         if(isdebug)
            message(NEWCOMMASET,tmp);
         return(1);
      }
      case cm_REALLOCINCLUDE:{char tmp[MAX_STR_LEN],*ptr=tmp;int n;
         *ptr=1;
         read_list((void (*)(void *))set_set_arg,&ptr,str,1);
         if(sscanf(tmp,"%d",&n)!=1)
            halt(CANNOTCONVERTTONUMBER,tmp);
         tryexpand_include(n);
         return(1);
      }

      case cm_DEF:{struct def_struct *tmp=NULL;char *ptr=NULL;
         read_list((void (*)(void *))set_set_arg,&ptr,str,1);
         if(lookup(ptr,command_table)!=NULL)
                           halt(USINGKEYWORD,str);
         if (def_table==NULL)def_table=create_hash_table(
                       def_hash_size,str_hash,str_cmp,def_destructor);
         tmp=get_mem(1,sizeof(struct def_struct));
         tmp->line=get_current_line()-1;
         tmp->col=get_current_col();
         tmp->offset=get_current_pos();/*This offset to the
                                          begin of the line*/
         if( *(tmp->filename=new_str(get_input_file_name()))==0)
            halt(FORNATALOWEDHERE,NULL);
         tmp->top=tmp->maxtop=1;
         tmp->str=get_mem(1,sizeof(char *) );
         tmp->str[0]=new_str(ptr);
         if(install(ptr,tmp,def_table))
           message(WARNINGREDEFINITION, ptr);
         i=1;
         do{
            while(*sc_get_token(str)!=esc_char);
            while(*sc_get_token(str)==esc_char);
            if(!s_scmp(str,"ENDDEF"))
               i--;
            else
              if(!s_scmp(str,"DEF"))
                i++;
         }while(i);
         return(1);
      }
      case cm_UNDEF:{ char tmp[MAX_STR_LEN],*ptr=tmp;
         *ptr=1;
         read_list((void (*)(void *))set_set_arg,&ptr,str,1);
         uninstall(tmp,def_table);
         return(1);
      }

   }
   return(0);
}/*proc_def*/

static void build_macro_arg(char *str)
{
 char balance=0;
 int isrepeat;
   m_ptr=0;
   proc_def(str,-1);/*If it is macrodefinition, this call will process it.*/
   marg_add(str);
   if(g_keep_spaces)
   /*!!!*/rm_spaces(" ");
   if(*str!='('){
      if(*sc_get_token(str)!='(')halt(OBRACKEDEXPECTED,str);
      marg_add("(");
   }
   while((*sc_get_token(str)!=')')||(balance)){
      do{
         isrepeat=0;
         if(*str=='(')balance++;
         else if(*str==')')balance--;
         if (balance<0)halt(SUPERFLUOUSrBRACKET,NULL);
         if((*str==esc_char)&&(str[1]==0)){
             if (*sc_get_token(str)==')')
                halt(UNEXPECTED,str);
             else if(!proc_def(str,-1)){
               char esc_line[2];
                  esc_line[1]=0;*esc_line=esc_char;
                  marg_add(esc_line);
                  isrepeat=1;
             }
         }else
            marg_add(str);
      }while(isrepeat);
   }
   if(g_keep_spaces)
   /*!!!*/add_spaces(" ");
   marg_add(")");
   str=macro_arg;
   if(put_macros(&str))halt(macro_error,NULL);
}/*build_macro_arg*/

struct one_labels_table_struct{
  char *label;
  long *position;
};

static struct one_labels_table_struct *current_labels=NULL;
static int top_labels_tables=0;
static int number_labels_tables=0;

static HASH_TABLE get_labels_tables(void)
{
  HASH_TABLE tmp_labels_tables=NULL;
  int i;
     if(number_labels_tables==0)return(NULL);
     tmp_labels_tables=create_hash_table(number_labels_tables+10,
                      str_hash,str_cmp,c_destructor);
     for(i=0; i<number_labels_tables; i++){
        install(current_labels[i].label,
                current_labels[i].position,
                tmp_labels_tables);
     }
     free_mem(&current_labels);
     top_labels_tables=number_labels_tables=0;
     return(tmp_labels_tables);
}/*get_labels_tables*/

static void proc_label(char *str)
{
 char balance=0;
 int l=0;
 char label_name[MAX_STR_LEN];
  if(*sc_get_token(str)!='(')halt(OBRACKEDEXPECTED,str);
  encode(current_labels_level,label_name,BASE);
  label_name[4]=chLABELSEP;
  label_name[5]=0;
  while((*sc_get_token(str)!=')')||(balance)){
        if(*str=='(')balance++;
        else if(*str==')')balance--;
        if (balance<0)halt(SUPERFLUOUSrBRACKET,NULL);
        if((*str==esc_char)&&(str[1]==0)){
             if(*sc_get_token(str)=='('){/*Quotation macro*/
                if(!((l+=s_len(sc_get_token(str)))<MAX_STR_LEN-2)){
                   label_name[MAX_STR_LEN-1]='\0';
                   halt(TOOLONGMACROARG,label_name);
                }
                s_cat(label_name,label_name,str);
                if(*sc_get_token(str)!=')')halt(CBRACKEDEXPECTED,str);
             }else if(!proc_def(str,-1))
                halt(INVCHARINLABEL,label_name+5);
             continue;
        }
        if(!((l+=s_len(str))<MAX_STR_LEN-5)){
           label_name[MAX_STR_LEN-1]='\0';
           halt(TOOLONGMACROARG,label_name);
        }
        s_cat(label_name,label_name,str);
        if (*str==comma_char)
           halt(INVCHARINLABEL,label_name+5);
  }
  for(l=0;l<number_labels_tables;l++)
     if(!s_scmp(label_name,current_labels[l].label))
        halt(DOUBLELABEL,label_name+5);
  if(!(number_labels_tables<top_labels_tables)){
    if (!(top_labels_tables<MAX_LABELS))
       halt(TOOMANYLABELS,NULL);
    if((current_labels=(struct one_labels_table_struct *)
                        realloc(current_labels,
                        (top_labels_tables+=DELTA_LABELS)*
                           sizeof(struct one_labels_table_struct)))==NULL)
                                                        halt(NOTMEMORY,NULL);
  }
  current_labels[number_labels_tables].label=NULL;
  current_labels[number_labels_tables].position=NULL;
  current_labels[number_labels_tables].label=new_str(label_name);
  if(
     (current_labels[number_labels_tables].position=get_mem(1,sizeof(long)))
     ==NULL
            )
              halt(NOTMEMORY,NULL);
  *(current_labels[number_labels_tables++].position)=*ttop_out-1;
}/*proc_label*/

static int proc_macros(char *str)
{
 macro_control.cellptr=NULL;
 if(*str!='('){
    if((macro_control.cellptr=lookup(str,macro_table))!=NULL){
       macro_control.is_proc=0;
    }else if((macro_control.cellptr=lookup(str,proc_table))!=NULL){
       macro_control.is_proc=1;
    }else
       return(0);
    macro_control.nargs=macro_control.cellptr->arg_numb;
 }else
    macro_control.nargs=1;
 mrec=0;
 build_macro_arg(str);
#ifdef DEBUG
 if(mrec)halt(INTERNALERROR,NULL);
#endif

 return(1);
}/*proc_macros*/

static void proc_table_init(void)
{
 proc_table=create_hash_table(PROC_HASH_SIZE,str_hash,str_cmp,c_destructor);
}/*proc_table_init*/

static int new_proc_cell(char arg_numb,char *name)
{
 struct macro_hash_cell *tmp;
 int i;
    /* Is this redeclaration?*/
    if((tmp=lookup(name,proc_table))==NULL){/*New cell:*/
       tmp=get_mem(1,sizeof(struct macro_hash_cell));
       tmp->arg_numb=arg_numb;
       tmp->flags=0;
       tmp->name=new_str(name);
       if(!(top_pexpand<max_pexpand)){
         if((max_pexpand+=DELTA_MAX_PEXPAND)>MAX_PROC_AVAIL)
            halt(TOOMANYPROCS,NULL);
         if((pexpand=
            (PEXPAND*)realloc(pexpand,max_pexpand*sizeof(PEXPAND)))==NULL)
              halt(NOTMEMORY,NULL);
       }
       (pexpand[tmp->num_mexpand=top_pexpand]).arg_numb=arg_numb;
       (pexpand[top_pexpand]).out_str=NULL;
       (pexpand[top_pexpand]).top_out=0;
       (pexpand[top_pexpand]).wascall=0;
       (pexpand[top_pexpand]).var=(arg_numb)?
                              get_mem(arg_numb,sizeof(char*)):
                              NULL;
       (pexpand[top_pexpand]).labels_tables=NULL;
       for(i=0;i<arg_numb;i++)
         (pexpand[top_pexpand]).var[i]=NULL;
       install(tmp->name,tmp,proc_table);
       top_pexpand++;
    }else{/* Reset old cell:*/
       if(tmp->arg_numb!=arg_numb)
         halt(WRONGARSINREDECLAR,name);
       if(pexpand[tmp->num_mexpand].top_out){/* Delete old code:*/
          message(WARNINGREDECLARATION, name);
          for(i=0; i<pexpand[tmp->num_mexpand].top_out; i++)
             free_mem(&((pexpand[tmp->num_mexpand].out_str)[i]));
          free_mem(&(pexpand[tmp->num_mexpand].out_str));
          pexpand[tmp->num_mexpand].top_out=0;
          hash_table_done(pexpand[tmp->num_mexpand].labels_tables);
          pexpand[tmp->num_mexpand].labels_tables=NULL;
       }
    }
    return(tmp->num_mexpand);
}/*new_proc_cell*/

void reject_proc(void)
{
int k;
char tmp[NUM_STR_LEN];
  if(wascall==0){
     if((k=cleanproc())!=0){
        if (isdebug){
           long2str(tmp,k);
           message(PROCCLEANED,tmp);
        }
     }
     proc_done();
  }
}/*reject_proc*/

void command_done(void)
{
   if(wascall==0){
     hash_table_done(proc_table);
     proc_table=NULL;
   }
}/*command_done*/

void for_done(void)
{
word i,j;
   if(for_stack!=NULL){
      for(i=0; i<max_for_stack; i++){
         for(j=0; j<for_stack[i].nargs; j++)
            free_mem(for_stack[i].args +j);
         free_mem(&(for_stack[i].args));
         free_mem(&(for_stack[i].iterator));
      }
      free_mem(&for_stack);
   }
   top_for_stack=max_for_stack=0;
}/*for_done*/

void IF_done(void)
{
   free_mem(&IF_stack);
   IF_depth=max_IF_depth=0;
}/*IF_done*/

int cleanproc(void)
{
int i,k=0;
long j;
  for(i=0;i<top_pexpand;i++)if (pexpand[i].wascall==0){
     k++;
     for(j=0;j<pexpand[i].arg_numb;j++)
        free_mem(&(pexpand[i].var[j]));
     free_mem(&(pexpand[i].var));
     pexpand[i].arg_numb=0;
     for(j=0; j<pexpand[i].top_out; j++)
        free_mem(&((pexpand[i].out_str)[j]));
     free_mem(&(pexpand[i].out_str));
     pexpand[i].top_out=0;
     hash_table_done(pexpand[i].labels_tables);
     pexpand[i].labels_tables=NULL;
  }
  return(k);
}/*cleanproc*/

static int register_proc(char *name,char arg_numb)
{
  if(lookup(name,macro_table)!=NULL)
     halt(USINGMACRONAMEASPRC,name);
  if(proc_table==NULL)proc_table_init();
  return(new_proc_cell(arg_numb,name));
}/*register_proc*/

void proc_done(void)
{
  if (proc_table!=NULL){
     hash_table_done(proc_table);
     proc_table=NULL;
  }
}/*proc_done*/

static char *scan_inkl_name(char *str)
{
 char name[MAX_STR_LEN];
   *str=1;
   read_list((void (*)(void *))set_set_arg,&str,name,1);
   return(str);
}/*scan_inkl_name*/

static int l_forced_hash=0;
/*Translator. Input from standart scaner, output into output array:*/
static int compile(void)
{
 char str[MAX_STR_LEN];
 char skipcommand=0;
 int is_prg=1;
 int num_pexpand=-1;
 int rept;
 int cmd;
   if(ifstack==NULL)
     ifstack=get_mem(max_ifdepth=DELTA_IF_DEPTH,
                          sizeof(struct control_struct));
   (ifstack[ifdepth=0]).sign=0;

   if(IF_stack==NULL){
     IF_stack=get_mem(max_IF_depth=DELTA_IF_DEPTH,sizeof(char *));
     (IF_stack[IF_depth=0])=0;
   }
   if(wstack==NULL)
     wstack=get_mem(max_wdepth=DELTA_IF_DEPTH,
                          sizeof(struct control_struct));
   (wstack[wdepth=0]).sign=-1;

   if(dostack==NULL)
     dostack=get_mem(max_dodepth=DELTA_IF_DEPTH,sizeof(long));
   dostack[dodepth=0]=-1;

   if (set_table==NULL)set_table=create_hash_table(
       set_hash_size,str_hash,str_cmp,c_destructor);

   l_forced_hash=0;
   hash_enable=1;
   rept=1;
   do{
     /*do{*/
       if((*sc_get_token(str)==esc_char)&&(str[1]==0))
         sc_get_token(str);
       else{
          if (IF_stack[IF_depth]<2)/*Check preprocessor IF*/
             halt(UNEXPECTED,str);
       }
/*     }while((*str==esc_char)&&(str[1]==0));*/
     {int *cmd_ptr;
       if( (cmd_ptr=lookup(str,command_table))==NULL)
          cmd=0;
       else
          cmd=*cmd_ptr;
     }
     if(IF_stack[IF_depth]>1)switch(cmd){
        case cm_IFDEF:
        case cm_IFNDEF:
        case cm_IFSET:
        case cm_IFNSET:
        case cm_IFEQ:
        case cm_IFNEQ:
           IF_stack[IF_depth]++;
           break;
        case cm_ENDIF:
           if(--(IF_stack[IF_depth]) == 1)
           IF_depth--;
           break;
        case cm_ELSE:
           if(IF_stack[IF_depth]==2)
           IF_stack[IF_depth]=1;
           break;
     }else switch(cmd){/*switch 0*/
        case cmINCLUDE:
           include(scan_inkl_name(str));break;
        case cmKEEPFILE:
           keep_file(get_input_file_name());break;
        case cm_IFDEF:
           proc_IF(if_def(str,def_table));
           break;
        case cm_IFNDEF:
           proc_IF(!if_def(str,def_table));
           break;
        case cm_IFSET:
           proc_IF(if_def(str,set_table));
           break;
        case cm_IFNSET:
           proc_IF(!if_def(str,set_table));
           break;
        case cm_IFEQ:
           proc_IF(eq_set(str));
           break;
        case cm_IFNEQ:
           proc_IF(!eq_set(str));
           break;
        case cm_ELSE:
           if(IF_stack[IF_depth]==0) halt(ELSEWITHOUT_IF,NULL);
           IF_stack[IF_depth]=2;
           break;
        case cm_ENDIF:
           if(IF_depth==0) halt(ENDIFWITHOUT_IF,NULL);
           IF_depth--;
           break;
        case cmFUNCTION:{int i=0;is_prg=0;/* Use it as a params counter*/
           sc_mark();
           if(*sc_get_token(str)==';')
              halt(UNEXPECTED,str);
           if(lookup(str,command_table)!=NULL)
                               halt(USINGKEYWORD,str);
           message(FUNCTION,str);
           if(*sc_get_token(str)!=';')do{
              if((*(str)==';')||((*str)==comma_char))
                  halt(UNEXPECTED,str);
              is_prg++;
              if(*sc_get_token(str)==';')
                 break;
              if((*str)!=comma_char)
                  halt(UNEXPECTED,str);
              sc_get_token(str);
           }while(1);
           sc_repeat();
           num_pexpand=register_proc(sc_get_token(str),is_prg);
           if(is_prg)for(;is_prg;is_prg--){
             if((pexpand[num_pexpand]).var[i]!=NULL){
               if(s_scmp((pexpand[num_pexpand]).var[i],sc_get_token(str))){
                  {char old[MAX_STR_LEN];
                    sprintf(old, RESETATOB,(pexpand[num_pexpand]).var[i],str);
                    message(old,NULL);
                  }
                  free((pexpand[num_pexpand]).var[i]);
                  (pexpand[num_pexpand]).var[i]=new_str(str);
               }
             }else
               (pexpand[num_pexpand]).var[i]=new_str(sc_get_token(str));
             i++;
             sc_get_token(str);
           }else sc_get_token(str);
           install(new_str("_body"),new_str("function"),set_table);
           rept=0;
           break;}
        case cmPROGRAM:
           rept=0;
           install(new_str("_body"),new_str("program"),set_table);
           message(PROGRAM,NULL);
           break;
        case cmMODESAVE:
        case cmMODERESTORE:
            /* Operators _modesave() and _moderestore()
                    may appear outside of a body*/
              sc_get_token(str);/*(*/
              sc_get_token(str);/*)*/
        case cmOFFOUT:
        case cmONOUT:
        case cmOFFLS:
        case cmONLS:
        case cmOFFTS:
        case cmONTS:
        case cmOFFBL:
        case cmONBL:
             /* Just ignore these commands outside of a body!*/
             break;
        default:
           if (proc_def(str,cmd)==0)
              halt(PROCORPROGEXPECTED,NULL);
           break;
     }/*switch 0*/
   }while(rept);
   /* Now we are in body of a function or a program.*/
   hash_enable=0;
   /*install(new_str("_body"),new_str("function"),set_table);*/
   while(1){
      if(*sc_get_token(str+g_debug_offset)!=esc_char){/*regular string*/
           if (IF_stack[IF_depth]<2){/*Check preprocessor IF*/
#ifndef NO_DEBUGGER
               if(g_debug_offset){
                  int c=get_current_col();
                  encode(get_current_line(),str,BASE);
                  str[4]=(c/127)+1;/*segment*/
                  str[5]=(c%127)+1;/*offset*/
                  str[6]=get_fname()+1;
               }/*if(g_debug_offset)*/
#endif
               putstr(str);
           }/*if (IF_stack[IF_depth]<2)*/
      }else{
         {int *cmd_ptr;
            if( (cmd_ptr=lookup(sc_get_token(str),command_table))==NULL)
               cmd=0;
            else
               cmd=*cmd_ptr;
         }
         if(IF_stack[IF_depth]>1)switch(cmd){
            case cm_IFDEF:
            case cm_IFNDEF:
            case cm_IFSET:
            case cm_IFNSET:
            case cm_IFEQ:
            case cm_IFNEQ:
               IF_stack[IF_depth]++;
               break;
            case cm_ENDIF:
               if(--(IF_stack[IF_depth]) == 1)
                  IF_depth--;
               break;
            case cm_ELSE:
               if(IF_stack[IF_depth]==2)
                   IF_stack[IF_depth]=1;
               break;
         }else switch(cmd){/*switch 1*/
            case cmINCLUDE:
               include(scan_inkl_name(str));break;
            case cm_IFDEF:
               proc_IF(if_def(str,def_table));
               break;
            case cm_IFNDEF:
               proc_IF(!if_def(str,def_table));
               break;
            case cm_IFSET:
               proc_IF(if_def(str,set_table));
               break;
            case cm_IFNSET:
               proc_IF(!if_def(str,set_table));
               break;
            case cm_IFEQ:
               proc_IF(eq_set(str));
               break;
            case cm_IFNEQ:
               proc_IF(!eq_set(str));
               break;
            case cm_ELSE:
               if(IF_stack[IF_depth]==0) halt(ELSEWITHOUT_IF,NULL);
               IF_stack[IF_depth]=2;
               break;
            case cm_ENDIF:
               if(IF_depth==0) halt(ENDIFWITHOUT_IF,NULL);
               IF_depth--;
               break;
            case cmEND:
               if(ifdepth)halt(UNTERMINATEDIF,NULL);
               if(wdepth)halt(UNTERMINATEDWHILE,NULL);
               if(dodepth)halt(UNTERMINATEDLOOP,NULL);
               if(is_prg==0){
                  (pexpand[num_pexpand]).out_str=*out_str;
                  (pexpand[num_pexpand]).top_out=*ttop_out;
                  (pexpand[num_pexpand]).labels_tables=get_labels_tables();
               }else{
                  if(top_for_stack)halt(UNTERMINATED_FOR,NULL);
                  labels_tables=get_labels_tables();
               }
               uninstall("_body",set_table);
               return(is_prg);
            default:{/*Must be a command*/
               int repeatcmd=0;
               do{
                  if(g_debug_offset){
                    ptr=1+g_debug_offset;
                    l_put_linenumberandfilename_into_polishline();
                  }else
                    ptr=1;
                  if(repeatcmd){
                     if(is_bit_set(&repeatcmd,bitELIF)){
                        set_bit(&(ifstack[ifdepth].sign),bitELIF);/*Mark
                                                              current cell*/
                        proc_if();
                        {int *cmd_ptr;
                           if(  (cmd_ptr=lookup(str,command_table))==NULL)
                               cmd=0;
                           else
                              cmd=*cmd_ptr;
                        }
                     }
                     repeatcmd=0;
                  }else if(proc_def(str,cmd)){
                     skipcommand=1;
                  }else if(proc_macros(str)){
                     put_cmd(cmOUTLINE);
                  }else{
                     if(cmd==0)
                        halt(UNEXPECTED,str);
                     switch(cmd){/*switch 2*/
                        case cmBEGINKEEPSPACES:
                           g_keep_spaces=1;
                           skipcommand=1;
                           break;
                        case cmENDKEEPSPACES:
                           g_keep_spaces=0;
                           skipcommand=1;
                           break;
                        case cmBEGINHASH:
                           l_forced_hash=1;
                           skipcommand=1;
                           break;
                        case cmENDHASH:
                           l_forced_hash=0;
                           skipcommand=1;
                           break;
                        case cmWLOOP:
                           if(proc_loop()==1){/*pre-condition*/
                              /*Create new info cell:*/
                              wdepth++;
                              if(!(wdepth < max_wdepth))
                                 if((wstack=realloc(wstack,
                                        (max_wdepth+=DELTA_IF_DEPTH)*
                                          sizeof(struct control_struct)))
                                              ==NULL)halt(NOTMEMORY,NULL);
                              wstack[wdepth].sign=1;/* Mark current cell*/
                              wstack[wdepth].line=*ttop_out;/*Store current
                                                                line number*/
                              wstack[wdepth].offset=ptr+1;/*Store offset.
                                                          +1 due to tpSTRING*/
                              put_string("    ");/*Reserve 4 bytes*/
                              put_cmd(cmIF);
                           }else{/*proc_loop()==2, postcondition*/
                              if(dodepth==0)halt(UNDEFLOOP,NULL);
                              put_string(encode(dostack[dodepth],str, BASE));
                              put_cmd(cmLOOP);
                              dodepth--;
                           }
                           break;
                        case cmLABEL:
                           proc_label(str);
                           skipcommand=1;
                           break;
                        case cmIF:
                           proc_if();
                           break;
                        case cmELSE:
                        case cmELIF:
                           if(
                              (!is_bit_set(&(ifstack[ifdepth].sign),bitIF))
                                  /*Verify "if"*/
                              ||
                              (is_bit_set(&(ifstack[ifdepth].sign),bitELSE))
                                 /*Verify "else"*/
                           )
                              halt(ELSEWITHOUTIF,NULL);
                           /*Put address in "if": */
                           encode(*ttop_out+1,
                            (*out_str)[ifstack[ifdepth].line]+
                            ifstack[ifdepth].offset,BASE);
                           /*Set TERM instead of 0 as string terminator:*/
                           *( (*out_str)[ifstack[ifdepth].line]+
                                 ifstack[ifdepth].offset+4) = TERM;
                           set_bit(&(ifstack[ifdepth].sign),bitELSE);/* Mark
                                                               current  cell*/
                           ifstack[ifdepth].line=*ttop_out;/*Store
                                                         current line number*/
                           ifstack[ifdepth].offset=ptr+1;/*Store offset.
                                                          +1 due to tpSTRING*/
                           put_string("    ");/*Reserve 4 bytes*/
                           put_cmd(cmELSE);
                           if(cmd==cmELIF)
                               set_bit(&repeatcmd,bitELIF);
                           break;
                        case cmENDIF:
                            /*Verify "if":*/
                           if(!is_bit_set(&(ifstack[ifdepth].sign),bitIF))
                                halt(ENDIFWITHOUTIF,NULL);
                           do{
                              /*Put address in "if" or "else": */
                              encode(*ttop_out,
                                  (*out_str)[ifstack[ifdepth].line]+
                                     ifstack[ifdepth].offset,BASE);
                              /*Set TERM instead of 0 as string terminator:*/
                              *( (*out_str)[ifstack[ifdepth].line]+
                                  ifstack[ifdepth].offset+4) =TERM;
                              ifstack[ifdepth].sign=0;/*Reset all flags in
                                                               current cell*/
                              ifdepth--;
                           }while(is_bit_set(&(ifstack[ifdepth].sign),
                                                                   bitELIF));
                           skipcommand=1;
                           break;
                        case cmLOOP:
                           if(wstack[wdepth].sign!=1)/*Verify "while"*/
                              halt(LOOPWITHOUTWHILE,NULL);
                           /*Put address in "while": */
                           encode(*ttop_out+1,(*out_str)[wstack[wdepth].line]
                                                +wstack[wdepth].offset, BASE);
                           /*Set TERM instead of 0 as string terminator:*/
                           *( (*out_str)[wstack[wdepth].line]+
                           wstack[wdepth].offset+4)=TERM;
                           /*Set "while" address: */
                           put_string(encode(wstack[wdepth].line,str,BASE));
                           put_cmd(cmWLOOP);
                           wstack[wdepth].sign=0;/*Unmark current cell*/
                           wdepth--;
                           break;
                        case cmDO:
                           /*Create new info cell:*/
                           dodepth++;
                           if(!(dodepth < max_dodepth))
                              if((dostack=realloc(dostack,
                                     (max_dodepth+=DELTA_IF_DEPTH)*
                                       sizeof(long)))==NULL)
                                       halt(NOTMEMORY,NULL);
                           dostack[dodepth]=*ttop_out;
                           skipcommand=1;
                           break;
                        case cmOFFOUT:
                           put_cmd(cmOFFOUT);
                           break;
                        case cmONOUT:
                           put_cmd(cmONOUT);
                           break;
                        case cmOFFLS:
                           put_cmd(cmOFFLS);
                           break;
                        case cmONLS:
                           put_cmd(cmONLS);
                           break;
                        case cmOFFTS:
                           put_cmd(cmOFFTS);
                           break;
                        case cmONTS:
                           put_cmd(cmONTS);
                           break;
                        case cmOFFBL:
                           put_cmd(cmOFFBL);
                           break;
                        case cmONBL:
                           put_cmd(cmONBL);
                           break;
                        case cmBEGINLABELS:
                           if(!(++current_labels_free<MAX_LABELS_GROUP ))
                              halt(TOOMANYLABELSGROUP,NULL);
                           if(!(current_labels_top<current_labels_max))
                              if(
                                 (current_labels_stack=
                                    realloc(current_labels_stack,
                                  (current_labels_max+=DELTA_LABELS)
                                         *sizeof(long *)))
                                                                 ==NULL)
                                                         halt(NOTMEMORY,NULL);
                           current_labels_stack[current_labels_top++]=
                                     current_labels_level;
                           current_labels_level=current_labels_free;
                           skipcommand=1;
                           break;
                        case cmENDLABELS:
                           if(--current_labels_top<0)
                              halt(NOLABELSGROUP,NULL);
                           current_labels_level=
                              current_labels_stack[current_labels_top];
                           skipcommand=1;
                           break;
                        default:
                           halt(UNDEFINEDCONTROLSEQUENCE,str);
                     }/*switch 2*/
                  }/*else*/
                  if(skipcommand==0){
                     if(!(ptr<max_ptr))
                       if((polish_line=(char *)realloc(polish_line,
                              (max_ptr=ptr+1)*sizeof(char)))==NULL)
                           halt(NOTMEMORY,NULL);
                     polish_line[ptr]=0;
                     putstr(polish_line);
                  }else skipcommand=0;
               }while(repeatcmd);
            }/*enddefault (switch 1)*/
         }/*switch 1*/

         if(!l_forced_hash)
            hash_enable=0;
      }
      /*Here polish line already in the array*/
   }

}/*compile*/

extern long number_of_trunslated_lines;
extern char **array_of_trunslated_lines;

void clear_program(void)
{
  long i;
     /*Clear translated MAIN program:*/
     for(i=0;i<number_of_trunslated_lines;i++){
       free( array_of_trunslated_lines[i]);
     }
     number_of_trunslated_lines=0;
     free_mem(&array_of_trunslated_lines);

     /*If the fatal error occurs during translation of the main program,
       number_of_trunslated_lines and array_of_trunslated_lines are not defined yet.
       So clear them again:*/
     if((wasrun==0)&&(ttop_out!=NULL)){
        /*Note, (*ttop_out)==0 if number_of_trunslated_lines was defuned!*/
        for(i=0;i < (*ttop_out);i++){
          free( (*out_str)[i]);
        }
        free_mem(out_str);
        ttop_out=NULL;
     }
     if(top_save_run){/* halt comes from some functon!
                       * True labels_tabel must be in : */
       labels_tables=save_run[0].labels_tables;
     }
     hash_table_done(labels_tables);
     labels_tables=NULL;
}/*clear_program*/

static void clean_def_names(void)
{
   if(defnames!=NULL)
      free_mem(&defnames);
   maxdeftop=0;
}/*clean_def_names*/

void clear_defs(void)
{
  if(wascall==0){
    clean_def_names();
    hash_table_done(def_table);
    def_table=NULL;
  }
}/*clear_defs*/

void clear_sets(void)
{
 hash_table_done(set_table);
 set_table=NULL;
}/*clear_sets*/

void clear_label_stack(void)
{
  free_mem(&current_labels_stack);
  current_labels_free=current_labels_level=current_labels_top=
  current_labels_max=0;
}/*clear_label_stack*/

static void done(void)
{
int i;
  free_mem(&polish_line);
  free_mem(&text_token);
  free_mem(&token);
  free_mem(&group);
  free_mem(&ifstack);
  free_mem(&wstack);
  free_mem(&dostack);
  free_mem(&mexpand);
  clear_label_stack();
  macro_done();
  proc_done();
  free_mem(&macro_arg);
  clear_program();
  hash_table_done(def_table);
  def_table=NULL;
  clear_sets();
  clean_def_names();
  if(number_labels_tables){
     for(i=0; i<number_labels_tables; i++){
        free_mem(&(current_labels[i].label));
        free_mem(&(current_labels[i].position));
     }
     free_mem(&current_labels);
     top_labels_tables=number_labels_tables=0;
  }
  hash_table_done(command_table);
  for_done();
  IF_done();
}/*done*/

void truns_done(void)
{
  if(*inp_f_name) pointposition(inp_f_name,current_line,current_col);
  done();
}/*truns_done*/

static int top_mexpand=0,max_mexpand=0;

static struct macro_hash_cell *new_macro_cell(char arg_numb, int flags,
                                              char *name,
                                                          MEXPAND *mexp)
{
 struct macro_hash_cell *tmp;
    tmp=get_mem(1,sizeof(struct macro_hash_cell));
    tmp->arg_numb=arg_numb;
    tmp->flags=flags;
    tmp->name=name;
    if(!(top_mexpand<max_mexpand)){
      if((max_mexpand+=DELTA_MAX_MEXPAND)>MAX_MACROS_AVAIL)
         halt(TOOMANYMACRO,NULL);
      if((mexpand=
         (MEXPAND**)realloc(mexpand,max_mexpand*sizeof(MEXPAND *)))==NULL)
           halt(NOTMEMORY,NULL);
    }
    mexpand[tmp->num_mexpand=top_mexpand++]=mexp;
    return(tmp);
}/*new_macro_cell*/

static void macro_table_init(void)
{
 macro_table=create_hash_table(MACRO_HASH_SIZE,str_hash,str_cmp,c_destructor);
}/*macro_table_init*/

void register_macro(char *name,char arg_numb,
                       int flags,MEXPAND *mexp)
{
  void *tmp;
  tmp=new_str(name);
  if(macro_table==NULL)macro_table_init();
  if(install(tmp,new_macro_cell(arg_numb,flags,tmp,mexp),
                          macro_table)
    )halt(DOUBLEDEF,name);
}/*register_macro*/

int truns(long *top_output_array, char ***output_array)
{
int ec;

   ttop_out=top_output_array;
   out_str=output_array;
   *ttop_out=max_out=0;
   *out_str=NULL;
   *inp_f_name=0;
   current_labels_free=0;
   current_labels_level=0;
   macro_control.cellptr=NULL;
   macro_init();
   if(proc_table==NULL)proc_table_init();
   *( (polish_line=get_mem(max_ptr+=DELTA_POLISH_LINE,sizeof(char)))+g_debug_offset )=PLSIGNAL;
   ec=compile();
   message(TRUNSEND,NULL);
   /*Clear temporary allocations:*/
   free_mem(&polish_line);
   free_mem(&macro_arg);
   m_ptr=0;
   max_m_ptr=0;
   ptr=0;
   max_ptr=0;
   return(ec);
}/*truns*/
