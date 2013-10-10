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
/* THRUSRUN.H*/
/*Base for enconing. BASE+' ' is maximal character:*/
#define BASE 95
#define WRONGNAME_HASH_SIZE 20
#define DELTA_ARGS_SIZE 5
#define MAXOUTBUFFERSIZE 1024
#define MAXMODESTACK 1000
#define DELTAMODESTACK 10
#define MAX_LABELS_GROUP (long)81450620
/*types:less then 0*/
#define tpCOMMAND -1
#define tpSTRING -2
#define tpVAL -3
#define tpBOOLEAN -4
/*boolean const: */
#define blFALSE 1
#define blTRUE 2
/*string terminator:*/
#define TERM 3
/*Polish line flag:*/
#define PLSIGNAL 4
/*EOF character:*/
#define chEOF 26
/*Separator for labels group id:*/
#define chLABELSEP 31
/*operations:from 10 to 30*/
#define opCAT 10
#define opISDEF 11
#define opLT 12
#define opLE 13
#define opGT 14
#define opGE 15
#define opEQ 16
#define opNE 17
#define opNOT 18
#define opAND 19
#define opXOR 20
#define opOR 21

#define opNOP 30

/*commands:starting from 30*/
#define cmIF 30
#define cmELSE 31
#define cmENDIF 32
#define cmWLOOP 33
#define cmOUTLINE 34
#define cmMACRO 35
#define cmLOOP 36
#define cmOFFLS 37
#define cmONLS 38
#define cmOFFBL 39
#define cmONBL 40
#define cmOFFTS 41
#define cmONTS  42
#define cmPROC 43
#define cmOFFOUT 44
#define cmONOUT 45
#define cmLABEL 46
#define cmELIF 47
#define cmDO 48
#define cmBEGINLABELS 49
#define cmENDLABELS 50
#define cm_ENDFOR 51
#define cm_FOR 52
#define cm_ENDDEF 53
#define cmINCLUDE 54
#define cm_IFDEF 55
#define cm_IFNDEF 56
#define cm_IFSET 57
#define cm_IFNSET 58
#define cm_ELSE 59
#define cm_ENDIF 60
#define cm_SET 61
#define cm_UNSET 62
#define cmEND 63
#define cm_DEF 64
#define cmFUNCTION 65
#define cmPROGRAM 66
#define cmKEEPFILE 67
#define cm_IFEQ 68
#define cm_IFNEQ 69
#define cm_POP 70
#define cm_PUSH 71
#define cm_ERROR 72
#define cm_GET 73
#define cm_PUSHQ 74
#define cm_ADD 75
#define cm_SCAN 76
#define cm_MESSAGE 77
#define cm_UNDEF 78
#define cm_READ 79
#define cm_CHANGEESC 80
#define cm_NEWCOMMENT 81
#define cm_CMDLINE 82
#define cm_REMOVE 83
#define cm_REPLACE 84
#define cm_REPLACEALL 85
#define cm_NEWCOMMA 86
#define cmMODESAVE 87
#define cmMODERESTORE 88
#define cm_ADDSPACES 89
#define cm_RMSPACES 90
#define cm_ADDDELIMITERS 91
#define cm_RMDELIMITERS 92
#define cm_ARGC 93
#define cm_REALLOCINCLUDE 94
#define cm_VERSION 95
#define cm_CHECKMINVERSION 96
#define cmBEGINHASH 97
#define cmENDHASH 98
#define cm_RMARG 99
#define cmBEGINKEEPSPACES 100
#define cmENDKEEPSPACES 101
#define cm_GETENV 102

#define DELTA_FORMOUT 5

struct macro_hash_cell{
  char arg_numb;/*Number of requered arguments*/
  int flags;/*0 -- no flags,Is possible run-time error - bit 0,
              current labels level -1, etc(reserved)*/
  char *name;/*Name of the macro*/
  int num_mexpand;/*Numbed of expand function in mexpand array
                    ( or number of procerdure)*/
};

struct mem_outfile_struct{/*Used by the operators suspendout and restoreout*/
   FILE *outfile;
   struct mem_outfile_struct *next;
};

void flush(void);
char *pexec(char seg,char offset,char *arg);
void push_string(char *str);
void stack2stack(int num);

void outputstr(char *str);

/*module RUN.C retuns argument from datastack:*/
char *get_string( char *str);
/*converts stack into long number. On error, halts complaining of "name":*/
long get_num(char *name,char *buf);
#ifdef TRUNS
char *macros_f_name[MAX_INCLUDE];
int top_fname=0;
MEXPAND **mexpand=NULL;
PEXPAND *pexpand=NULL;
HASH_TABLE macro_table=NULL;
HASH_TABLE proc_table=NULL;
int top_pexpand=0,max_pexpand=0;
#else
extern char *macros_f_name[MAX_INCLUDE];
extern int top_fname;
extern MEXPAND **mexpand;
extern PEXPAND *pexpand;
extern HASH_TABLE macro_table;
extern HASH_TABLE proc_table;
extern int top_pexpand,max_pexpand;
#endif
#ifdef MACROS
int *modestack=NULL;
int top_modestack=0;
int max_modestack=0;
int *p_modestack=NULL;
int p_top_modestack=0;
int p_max_modestack=0;

int is_scanner_init=0;
int is_scanner_double_init=0;
char *callstack=NULL;
word max_callstacktop=0;
word callstacktop=0;
HASH_TABLE variables_table=NULL;
HASH_TABLE labels_tables=NULL;
HASH_TABLE export_table=NULL;
FILE *ext_file=NULL;
char **text_block=NULL;
long top_txt=0,max_top_txt=0;
char from_expr[MAX_STR_LEN],end_expr[MAX_STR_LEN],to_expr[MAX_STR_LEN];
long *current_instruction_address=NULL;
#else
extern int *modestack;
extern int top_modestack;
extern int max_modestack;
extern int *p_modestack;
extern int p_top_modestack;
extern int p_max_modestack;
extern int is_scanner_init;
extern int is_scanner_double_init;
extern char *callstack;
extern word max_callstacktop;
extern word callstacktop;
extern HASH_TABLE variables_table;
extern HASH_TABLE labels_tables;
extern HASH_TABLE export_table;
extern FILE *ext_file;
extern char **text_block;
extern long top_txt,max_top_txt;
extern char from_expr[MAX_STR_LEN],end_expr[MAX_STR_LEN],
           to_expr[MAX_STR_LEN];
extern long *current_instruction_address;
#endif
#ifdef RUN
int doTrace(long n, char *inp_array[]);
void debugger_done(void);
struct mem_outfile_struct *suspendedfiles=NULL;
int offleadingspaces=0;
int offtailspaces=0;
int offblanklines=0;
int offout=0;
/*FILE *outfile=NULL;*/
char outputfile_name[MAX_STR_LEN];
int runexitcode=0;
int isrunexit=0;
int isreturn=0;
struct save_run_struct save_run[LAST_M_RECURSION+1];
int top_save_run=0;
char returnedvalue[MAX_STR_LEN];
#else
extern struct mem_outfile_struct *suspendedfiles;
extern int offleadingspaces;
extern int offtailspaces;
extern int offblanklines;
extern int offout;
/*extern FILE *outfile;*/
extern char outputfile_name[MAX_STR_LEN];
extern int runexitcode;
extern int isrunexit;
extern int isreturn;
extern struct save_run_struct save_run[LAST_M_RECURSION+1];
extern int top_save_run;
extern char returnedvalue[MAX_STR_LEN];
#endif
