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
#include <sys/types.h>
#include <sys/wait.h>

#include "tools.h"
#include "comdef.h"
#include "types.h"
#include "variabls.h"
#include "headers.h"
#include "texts.h"

#define stINIT 0
#define stL 1
#define stSD 2
#define stSP 3
#define stT 5
#define stP 6
#define stB 7
#define stE 8
#define stC 9
#define stX 10
#define stI 11
#define stF 12
#define stV 13
#define stR 14
#define stH 15
#define stQ 16
#define stO 17
#define stPRG 18
#define stM 19
#define stVT 20
#define stS 21
#define stD 22
#define stCC 23
#define stSMP 24
#define stEND 100

#define stMINUS 30
#define stCOMMA 31
#define stTHESAME 32

#define bitINTERACTIVE 0
#define bitCMD 1
#define bitEOW 2
#define bitNONULL 3

extern char *currentRevisionNumber;/*pilot.c*/

static int top_argc=1;
static char red_line[MAX_STR_LEN];
static char *ptr=red_line;
static int endoption_denied=0;
static char *(*read_line)(char *str);
static FILE *option_file=NULL;
static int read_mode=0;
static int is_first=1;
static char *prg_name=NULL;
static int state;
static int set_default;
static int is_new_t=1;
static int is_new_p=1;

/*Concatenates s1 and s2 with memory rellocation for s1, utils.c:*/
char *s_inc(char *s1, char *s2);

/*allocates new template:*/
static char *new_template(int is_new,struct template_struct *top, char *pattern)
{
   if(pattern==NULL) return(NULL);
   if(is_new){
      free(top->pattern);
      return(top->pattern=new_str(pattern));
   }
   while(top->next!=NULL)
     top=top->next;

   top->next=get_mem(1,sizeof(struct template_struct));
   top=top->next;
   top->next=NULL;
   return(top->pattern=new_str(pattern));
}/*new_template*/

static void template_done(struct template_struct *top)
{
struct template_struct *tmp=NULL;
  if(top==NULL)return;
  do{
     if(top->pattern!=NULL)
       free(top->pattern);
     tmp=top->next;
     free(top);
  }while((top=tmp)!=NULL);

}/*template_done*/

static void list2screen(struct list_struct *top)
{
/*PPP*/
   while(top!=NULL){
      printf("-%c ",top->xi);
      printf("%ld",top->from);
      if(top->from!=top->to)
         printf("-%ld",top->to);
      if (top->next!=NULL)
         printf("; ");
      else
         printf(";\n");
      top=top->next;
   }
}/*list2screen*/

static void show_options(void)
{
int i;
struct template_struct *tmp=NULL;
  printf(CURRENTOPTIONS);/*PPP*/
  if(xi_list_top!=NULL){
     list2screen(xi_list_top);
  }
  if(browser_out_name!=NULL) printf("-l %s; ",browser_out_name);
  printf("-sd %s; ",comp_d);
  printf("-sp %s; ",comp_p);

  printf("-t");
  tmp=template_t;
  do{
     printf(" %s",tmp->pattern);
  }while((tmp=tmp->next)!=NULL);
  printf(";\n");

  printf("-p");
  tmp=template_p;
  do{
     printf(" %s",tmp->pattern);
  }while((tmp=tmp->next)!=NULL);
  printf(";\n");

  printf("-b %u; ",start_diagram);
  printf("-e %u; ",finish_diagram);
  printf("-m %s; ",(message_enable)?(MSG_ENABLE_TXT):(MSG_DISABLE_TXT));
  printf("-c %s;\n",config_name);
  printf("-P %s;\n",programname);
  printf("-smp %d;\n",g_numberofprocessors);

  if(is_bit_set(&mode,bitVERIFY))printf("-v ;");
  for (i=0; i<run_argc; i++)
     printf("-r %s; ",run_argv[i]);
  printf("\n");
}/*show_options*/

static void help(void)
{
   printf(HELP1,prg_name);
   printf(HELP2);
   printf(HELP3);
   printf(HELP4);
   printf(HELP6);
   printf(HELP7);
   printf(HELP8);
   printf(HELP9);
   printf(HELP10);
   printf(HELP11);
   printf(HELP12);
   printf(HELP13);
   printf(HELP14);
   printf(HLP14A);
   printf(HELP15);
   printf(HELP16);
   printf(HELP18);
   printf(HELP17);
   printf(HELP19);
   printf(HELP20);
   printf(HELP21);
   fflush(stdout);
}/*help*/

static void add2list(long f,long t, char xi)
{
struct list_struct *top=xi_list_top;
struct list_struct *list=xi_list_top;
   if(list==NULL){
      xi_list_top=list=get_mem(1,sizeof(struct list_struct));
   }else{
     while(top!=NULL){list=top;top=top->next;}
     list->next=get_mem(1,sizeof(struct list_struct));
     list=list->next;
   }
   list->next=NULL;
   list->from=f;list->to=t;
   list->xi=xi;
}/*add2list*/

static int option2list(char *str,char xi)
{
  int state;
  long from,to;
  char *beg;
  set_of_char digit;
     set_str2set("0123456789",digit);
     for (state=stINIT;state!=stEND;)switch(state){
        case stINIT:
           if (!set_in(*str,digit))return(-1);
           beg=str;
           while(set_in(*str,digit))str++;
           if(*str=='-')state=stMINUS;
           else if(*str==comma_char)state=stCOMMA;
           else if(*str==0)state=stTHESAME;
           else return(-1);
           *str++=0;
           sscanf(beg,"%ld",&from);
           beg=str;
           break;
        case stMINUS:
           if (!set_in(*str,digit))return(-1);
           while(set_in(*str,digit))str++;
           if(*str==0)state=stEND;
           else if (*str==comma_char)state=stINIT;
           else return(-1);
           *str++=0;
           sscanf(beg,"%ld",&to);
           add2list(from,to,xi);
           break;
        case stCOMMA:
           add2list(from,from,xi);
           state=stINIT;
           break;
        case stTHESAME:
           add2list(from,from,xi);
           state=stEND;
           break;
     }
     return(0);
}/*option2list*/

static char *get_cmdline_option(char *str);
static char *get_file_option(char *str);

static char *get_interactive_option(char *str)
{
 char *tmp=str;
  if (is_first){
     help();
     is_first=0;
  }
  do{
     if (endoption_denied)
        printf(">");/*PPP*/
     else
        printf(PROMPT);/*PPP*/
     fflush(stdout);
     if((tmp=fgets(str,MAX_STR_LEN,stdin))!=NULL)/*PPP*/
        while(*tmp==' ')tmp++;
     else
        tmp=s_let("q",str);
     if(*tmp=='\n'){
        if (endoption_denied)
           message(ILLEGALOPTION,NULL);
        else{
           set_bit(&read_mode,bitEOW);
           return(NULL);
        }
     }
  }while(*tmp=='\n');
  str[s_len(str)-1]=0;
  return(str);
}/*get_interactive_option*/

static char *get_file_option(char *str)
{
 char *tmp;
     if((tmp=fgets(str,MAX_STR_LEN,option_file))!=NULL)
        while(*tmp==' ')tmp++;
     if((tmp==NULL)||(*tmp=='\n')){
        close_file(&option_file);
        if(is_bit_set(&read_mode,bitCMD))
           read_line=get_cmdline_option;
        else
           read_line=get_interactive_option;
        if (endoption_denied){
           message(ILLEGALOPTION,NULL);
           read_line=get_interactive_option;
           set_bit(&read_mode,bitINTERACTIVE);
           unset_bit(&read_mode,bitCMD);
        }
        return(read_line(str));
     }
     str[s_len(str)-1]=0;
     return(str);
}/*get_file_option*/

static char *get_cmdline_option(char *str)
{
   if (!(top_argc<g_argc)){
     if (endoption_denied){
         message(UNEXPECTEDENDOFOPTIONS,NULL);
         read_line=get_interactive_option;
         set_bit(&read_mode,bitINTERACTIVE);
         unset_bit(&read_mode,bitCMD);
     }else
        set_bit(&read_mode,bitEOW);
     return(NULL);
   }
   return(letnv(g_argv[top_argc++],str,MAX_STR_LEN));
}/*get_cmdline_option*/

static char *get_option(char *str)
{
 char *beg;
   if(ptr==red_line)/*String exhausted*/
      if (read_line(red_line)==NULL){
         if(is_bit_set(&read_mode,bitNONULL))
            return(s_let("",str));
         else
            return(NULL);
      }/*if (read_line(red_line)==NULL)*/
   while(*ptr==' ')ptr++;
   beg=ptr;
   while((*ptr!=' ')&&(*ptr))ptr++;
   if(*ptr==0)ptr=red_line;/*String exhausted*/
   else{
      (*ptr++)=0;
      while(*ptr==' ')ptr++;
      if(*ptr==0)ptr=red_line;/*String exhausted*/
   }
   return(letnv(beg,str,MAX_STR_LEN));
}/*get_option*/

void op_error(char *s1,char *s2)
{
  message(s1,s2);
  state=stINIT;
/*  help();*/ /* extra */
  ptr=red_line;
  read_line=get_interactive_option;
  set_bit(&read_mode,bitINTERACTIVE);
  unset_bit(&read_mode,bitCMD);
}/*op_error*/
static void xi_list_done(void)
{
  struct list_struct *top=xi_list_top;
     while(top!=NULL){
        xi_list_top=top->next;
        free_mem(&top);
        top=xi_list_top;
     }
}/*xi_list_done*/

void read_command_line(int argc, char *argv[])
{
  char str[MAX_STR_LEN+3];
  char *tmp;
  int rflag=0;
  prg_name=new_str(argv[0]);
  g_argc=argc;g_argv=argv;

  {int i,l=0;
    for (i=0; i<g_argc;i++){
       l+=(s_len(g_argv[i])+1);
       full_command_line=s_inc(full_command_line,g_argv[i]);
       full_command_line=s_inc(full_command_line," ");
       if(l>80){
          char *ptr=full_command_line+76;
          s_let("...",ptr);
          break;
       }/*if(l>80)*/
    }/*for (i=0; i<g_argc;i++)*/
  }/*{int i,l=0;*/

  if (argc>1){/*Read from command line*/
     read_line=get_cmdline_option;
     set_bit(&read_mode,bitCMD);
  }else{/*Begin interactive mode*/
     set_bit(&read_mode,bitINTERACTIVE);
     read_line=get_interactive_option;
  }
  for(state=stINIT;state!=stEND;)switch(state){
     case stINIT:
        endoption_denied=0;
        set_default=0;
        tmp=str;
        unset_bit(&read_mode,bitNONULL);
        if(get_option(tmp)==NULL){
           if (is_bit_set(&read_mode,bitEOW)){
              state=stEND;
              break;
           }
        }else if(*tmp!='-'){
           s_cat(tmp,"-r ",tmp);tmp[2]=0;
           rflag=1;
        }
        set_bit(&read_mode,bitNONULL);
        tmp++;
        if (*tmp=='-'){tmp++;set_default=1;}
        if ((!s_scmp(tmp,"s"))||(!s_scmp(tmp,"server")))
           state=stS;/*Start server*/
        else if ((!s_scmp(tmp,"d"))||(!s_scmp(tmp,"daemon")))
           state=stD;/*Start server as a daemon*/
        else if ((!s_scmp(tmp,"C"))||(!s_scmp(tmp,"client")))
           state=stCC;/*Fork out client at start*/
        else if (!s_scmp(tmp,"smp"))
           state=stSMP;/*Set the number of processors*/
        else if ((!s_scmp(tmp,"l"))||(!s_scmp(tmp,"look")))
           state=stL;/*Browser mode*/
        else if((!s_scmp(tmp,"sd"))||(!s_scmp(tmp,"sortd")))
           state=stSD;/*Sorting template for diagrams*/
        else if((!s_scmp(tmp,"sp"))||(!s_scmp(tmp,"sortp")))
           state=stSP;/*Sorting template for prototypes*/
        else if((!s_scmp(tmp,"t"))||(!s_scmp(tmp,"topology")))
           state=stT;/*Template for processed topologies*/
        else if((!s_scmp(tmp,"p"))||(!s_scmp(tmp,"prototype")))
           state=stP;/*Template for processed prototypes*/
        else if((!s_scmp(tmp,"b"))||(!s_scmp(tmp,"begin")))
           state=stB;/*Start diagram number*/
        else if((!s_scmp(tmp,"e"))||(!s_scmp(tmp,"end")))
           state=stE;/*Finish diagram number*/
        else if((!s_scmp(tmp,"c"))||(!s_scmp(tmp,"configuration")))
           state=stC; /*Configuration filename*/
        else if((!s_scmp(tmp,"x"))||(!s_scmp(tmp,"exclude")))
           state=stX;/*Exclude list*/
        else if((!s_scmp(tmp,"i"))||(!s_scmp(tmp,"include")))
           state=stI;/*Include list*/
        else if((!s_scmp(tmp,"f"))||(!s_scmp(tmp,"file")))
           state=stF;/*Read options from file*/
        else if((!s_scmp(tmp,"v"))||(!s_scmp(tmp,"verify")))
           state=stV;/*Set the verification option*/
        else if((!s_scmp(tmp,"vt"))||(!s_scmp(tmp,"verifyTopology")))
           state=stVT;/*Set the option to check momenta balance on topologies*/
        else if((!s_scmp(tmp,"r"))||(!s_scmp(tmp,"run")))
           state=stR;/*Option will be passed to run*/
        else if((!s_scmp(tmp,"m"))||(!s_scmp(tmp,"messages")))
           state=stM;/*messages enable/disable*/
        else if((!s_scmp(tmp,"h"))||(!s_scmp(tmp,"help")))
           state=stH;/*help*/
        else if((!s_scmp(tmp,"o"))||(!s_scmp(tmp,"options")))
           state=stO;/*options*/
        else if((!s_scmp(tmp,"P"))||(!s_scmp(tmp,"Program")))
           state=stPRG;/*program*/
        else if((!s_scmp(tmp,"q"))||(!s_scmp(tmp,"quit")))
           state=stQ;/*Immediate quit from program*/
        else if((!s_scmp(tmp,"V"))||(!s_scmp(tmp,"version"))){
           if ( ! is_bit_set(&read_mode,bitCMD))
              printf("%s\n",currentRevisionNumber);
           /* else -- The version information is alread outputted, so
            * just ignore it
            */
           state=stINIT;
        }else
           op_error(UNDEFOPT,tmp);

        endoption_denied=1;
        break;
     case stL:
        free_mem(&browser_out_name);
        if(set_default)
           unset_bit(&mode,bitBROWS);
        else{
           browser_out_name=new_str(get_option(str));
           set_bit(&mode,bitBROWS);
           unset_bit(&mode,bitVERIFY);
        }
        state=stINIT;
        break;
     case stCC:
        state=stINIT;
        if(g_toserver<0){
          startclient(&g_pipetoclient);
          /*Now on success g_pipetoclient is an opened pipe from non-activated client.
            To activate it, the function activateClient(g_pipetoclient) may be used*/
        }/*if(g_toserver<0)*/
        break;
     case stSMP:
        state=stINIT;
        if(set_default)
           g_numberofprocessors=1;
        else{
           char *b;
           long int n,i=strtol(get_option(str),&b,10);
           if( (*b) == ',' ){
              n=strtol(b+1,&tmp,10);
              if( (*tmp) !='\0' ){
                 op_error(NUMBEREXPECTED,b+1);
                 break;
              }/*if( (*tmp) !='\0' )*/
           }else{
               if ( (*b) !='\0' ){
                  op_error(NUMBEREXPECTED,str);
                  break;
               }/*if( (*b) !='\0' )*/
               n=0;
           }/*if( (*b) == ',' )...else*/
           if(i<0){
              op_error(TOOSMALLNUM,str);
              break;
           }/*if(i<0)*/
           if(i>MAX_HANDLERS_PER_HOST){
              op_error(TOOBIGNUM,str);
              break;
           }/*if(i>MAX_HANDLERS_PER_HOST)*/

           g_numberofprocessors=i;/*if i==0 then the local server will not be started!*/
           g_niceoflocalserver=n;
           if(i>0)
              runlocalservers();

        }/*if(set_default)...else*/
        break;
     case stD:
        g_daemonize=1;
        /*No break!*/
     case stS:
        state=stINIT;
        if(!set_default){
           char *b;
           long int n,i=strtol(get_option(str),&b,10);
           if( (*b) == ',' ){
              n=strtol(b+1,&tmp,10);
              if( (*tmp) !='\0' ){
                 op_error(NUMBEREXPECTED,b+1);
                 break;
              }/*if( (*tmp) !='\0' )*/

           }else{
               if ( (*b) !='\0' ){
                  op_error(NUMBEREXPECTED,str);
                  break;
               }/*if( (*b) !='\0' )*/
               n=1;
           }/*if( (*b) == ',' )...else*/
           if(i<0){
              op_error(TOOSMALLNUM,str);
              break;
           }/*if(i<0)*/
           if(i>MAX_HANDLERS_PER_HOST){
              op_error(TOOBIGNUM,str);
              break;
           }/*if(i>MAX_HANDLERS_PER_HOST)*/

           if (i==0)break;
           if( startserver(i,n)<1  )
              message(SERVERFAILS,NULL);
           break;
        }/*if(!set_default)*/
     case stSD:
         if(set_default)
           s_let(DIAGRAM_COMP_DEFAULT,comp_d);
         else{set_of_char valid_switches;
            get_option(tmp=str);
            set_str2set(DIAGRAM_COMP,valid_switches);
            while(*tmp)if(!set_in(*tmp++,valid_switches)){
                               op_error(INVALIDTEMPLATE,str);
                               s_let(DIAGRAM_COMP_DEFAULT,comp_d);
                               break;
            }
            if(state!=stINIT)
               letnv(str,comp_d,MAX_COMP_D);
         }
         state=stINIT;
         break;
     case stSP:
         if(set_default)
           s_let(PROTOTYPE_COMP_DEFAULT,comp_p);
         else{set_of_char valid_switches;
            get_option(tmp=str);
            set_str2set(PROTOTYPE_COMP,valid_switches);
            while(*tmp)if(!set_in(*tmp++,valid_switches)){
                               op_error(INVALIDTEMPLATE,str);
                               s_let(PROTOTYPE_COMP_DEFAULT,comp_p);
                               break;
            }
            if(state!=stINIT)
               letnv(str,comp_p,MAX_COMP_P);
         }
         state=stINIT;
         break;
     case stT:
        if(set_default){
           template_done(template_t);
           template_t=get_mem(1,sizeof(struct template_struct));
           template_t->next=NULL;
           template_t->pattern=new_str(DEFAULT_TEMPLATE_T);
           is_new_t=1;
        }else{
           new_template(is_new_t,template_t,get_option(str));
           is_new_t=0;
        }
        state=stINIT;
        break;
     case stP:
        if(set_default){
           template_done(template_p);
           template_p=get_mem(1,sizeof(struct template_struct));
           template_p->next=NULL;
           template_p->pattern=new_str(DEFAULT_TEMPLATE_P);
           is_new_p=1;
        }else{
           new_template(is_new_p,template_p,get_option(str));
           is_new_p=0;
        }
        state=stINIT;
        break;
     case stB:
        if(set_default)
           start_diagram=0;
        else{
          if(sscanf(get_option(str),"%u",&start_diagram)!=1)
            op_error(ILLEGALB,str);
        }
        state=stINIT;
        break;
     case stE:
        if(set_default)
           finish_diagram=0;
        else{
           if(sscanf(get_option(str),"%u",&finish_diagram)!=1)
             op_error(ILLEGALE,str);
        }
        state=stINIT;
        break;
     case stC:
        free_mem(&config_name);
        if(set_default)
           config_name=new_str(CONFIG);
        else
           config_name=new_str(get_option(str));
        state=stINIT;
        break;
     case stM:
        if(set_default)
           message_enable=MSG_ENABLE_INIT;
        else
           switch(*get_option(str)){
              case 'e':
              case 'E':
              case 'y':
              case 'Y':
                 message_enable=1;break;
              case 'd':
              case 'D':
              case 'n':
              case 'N':
                 message_enable=0;break;
              default:
                 op_error(ILLEGALVAL,str);break;
           }/*switch(*get_option(str))*/

        state=stINIT;
        break;
     case stX:
        if(set_default)
            xi_list_done();
        else{
           if(option2list(get_option(str),'x')){
              op_error(ILLEGALL,str);
              xi_list_done();
           }
        }
        state=stINIT;
        break;
     case stI:
        if(set_default)
            xi_list_done();
        else{
           if(option2list(get_option(str),'i')){
              op_error(ILLEGALL,str);
              xi_list_done();
           }
        }
        state=stINIT;
        break;
     case stF:
        if((option_file=fopen(get_option(str),"r"))==NULL)
           op_error(CANNOTOPEN,str);
        else
          read_line=get_file_option;
        state=stINIT;
        break;
     case stV:
        if(set_default)
          unset_bit(&mode,bitVERIFY);
        else{
           free_mem(&browser_out_name);
           unset_bit(&mode,bitBROWS);
           set_bit(&mode,bitVERIFY);
        }
        state=stINIT;
        break;
     case stVT:
        if(set_default){
          unset_bit(&mode,bitCHECKMOMENTABALANCE);
          unset_bit(&mode,bitFIRSTCHECKBALANCE);
          unset_bit(&mode,bitTHROUGHCHECKBALANCE);
        }else{
           set_bit(&mode,bitCHECKMOMENTABALANCE);
           switch(*get_option(str)){
              case 'a':/*all*/
                 /*nothig to set*/
                 break;
              case 'f':/*first ilegal and halt*/
                 set_bit(&mode,bitFIRSTCHECKBALANCE);
                 break;
              case 't':/*through -- only warnings, no halt*/
                 set_bit(&mode,bitTHROUGHCHECKBALANCE);
                 break;
              default:
                 op_error(ILLEGALVAL,str);
                 unset_bit(&mode,bitCHECKMOMENTABALANCE);
                 break;
           }/*switch(*get_option(str))*/

        }/*else*/
        state=stINIT;
        break;
     case stR:
        if(set_default)
           run_argv_done();
        else{
           if (!(run_argc < MAX_ARGV))
              op_error(TOOMANYROPT,NULL);
           else{
             if(rflag){
                run_argv[run_argc++]=new_str(tmp+2);
                rflag=0;
             }else
                run_argv[run_argc++]=new_str(get_option(str));
           }
        }
        state=stINIT;
        break;
     case stH:
        help();
        state=stINIT;
        break;
     case stO:
        show_options();
        state=stINIT;
        break;
     case stPRG:
        if(set_default)
           *programname=0;
        else
           get_option(programname);
        state=stINIT;
        break;
     case stQ:
        errorlevel=0;
        halt(USERBREAK,NULL);
  }/*for and switch*/
  free_mem(&prg_name);
}/*read_command_line*/

void run_argv_done(void)
{
int i;
  for(i=0; i<run_argc;i++)
    free_mem(&(run_argv[i]));
  run_argc=0;
}/*run_argv_done*/

void cmd_line_done(void)
{
  close_file(&option_file);
  free_mem(&browser_out_name);
  free_mem(&config_name);
  free_mem(&prg_name);
  xi_list_done();
  run_argv_done();
  template_done(template_t);
  template_t=NULL;
  template_done(template_p);
  template_p=NULL;
}/*cmd_line_done*/
