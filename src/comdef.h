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
#ifdef CHECKMEM
#include <alloc.h>
#endif
#define COMP_ARR_L 5

/*2147483646/4-1:*/
#define TOP_JOBS_ID 536870910
/*COMDEF.H*/
#define bitBROWS 0
#define bitTERROR 1
#define bitVERIFY 3
#define bitRUNSIGNAL 4
#define bitFORCEDTRUNSLATION 5
#define bitSKIP 6
#define bitEND 7
#define bitONLYINTERPRET 8
#define bitCHECKMOMENTABALANCE 9
#define bitFIRSTCHECKBALANCE 10
#define bitTHROUGHCHECKBALANCE 11

#define ZERO_VECTOR "z"
/*Initial bool value messages enable/disable (1/0):*/
#define MSG_ENABLE_INIT 1
/*Boolean, if 0 , lsubst() does not upset momenta:*/
#define FLIP_MOMENTA_ON_LSUBST 0

/*Used as macrocall flags:*/
#define bitDEBUG 0
#define bitLABELSUPDATE 1

/* Used in truns for if-then-else(elif) construction:*/
#define bitELIF 0
#define bitIF 1
#define bitELSE 2

#define ONLY_ELEMENTARY_LOOP_MOMENTA 1
/*default for toolc.c:*/
#define MAX_INCLUDE 200
/*utils.c:*/

/*Maximal numbers of attempts to read/write to a descriptor if 0 is returned:*/
#define MAX_FAILS_IO 2

#define FLINKS_HASH_SIZE 24
#define DELTA_TT_TBL 10

#define MIN_POSSIBLE_COORDINATE -1E+5
#define MAX_POSSIBLE_COORDINATE 5E+3
#define MAX_OUTPUT_LEN 70
#define MAX_DIAGRAM_NUMBER_S 65520
#define MAX_DIAGRAM_NUMBER_L 2100000000l
#define MAX_STR_LEN 102400

#define MAX_MOMENTA_SET 30
#define MAX_TOPOLOGY_ID_SET 30
#define MAX_LINES_IN_VERTEX 8
#define HALT -1
/* topology.c*/
/*Maximal numbers:*/
   /*Internal lines:*/
#define MAX_I_LINE 20
   /*External lines:*/
#define MAX_E_LINE 20
   /*Vetrices:*/
#define MAX_VERTEX 20
 /*max(lines,vertices)+1:*/
#define MAX_ITEM 21
/*truns.c:*/
#define DELTA_FOR_STACK 3
#define DELTA_FOR_ARGS 10
#define COMMAND_HASH_SIZE 107
#define DELTA_DEF_TOP 4
#define MAX_LABELS 32500
#define DELTA_LABELS 20
#define MAX_LOOP_DEPTH 20
#define MAX_MACRO_ARG 1048
#define DELTA_POLISH_LINE 50
#define LAST_RECURSION 20
#define LAST_M_RECURSION 20
#define DELTA_TEXT_SIZE 80
#define DELTA_TOKEN_SIZE 64
#define MAX_MACROS_DEPTH 10
#define DELTA_IF_DEPTH 5
#define NUM_STR_LEN 12
#define DELTA_MAX_OUT 20
#define DELTA_MACRO_ARG 50
#define DELTA_MAX_MEXPAND 10
#define MACRO_HASH_SIZE 359
#define MAX_MACROS_AVAIL 10000
#define DELTA_TEXT_BLOCK 10
#define PROC_HASH_SIZE 37
#define MAX_PROC_AVAIL  100
#define DELTA_MAX_PEXPAND 10
#define DEF_HASH_SIZE 41
#define SET_HASH_SIZE 41
#define ALLPIPES_HASH_SIZE 17
/*run.c:*/
#define DELTA_STACK_SIZE 20
/*macro.c:*/
#define PATHTOSHELL "/bin/sh"
#define MOMENTATABLE_RIGHT "P"
#define MOMENTATABLE_LEFT "Q"
#define MOMENTATABLE_EXT "Qe"
#define MOMENTA_TABLE_ROWS "NDEF"
#define MAX_ARGV 20
#define VAR_HASH_SIZE 23
#define EXPORT_HASH_SIZE 23
/*cmd_line.c:*/
/*For SMP computers set the following value to the number of processors:*/
#define MAX_HANDLERS_PER_HOST 32
#define CONFIG "config.cnf"
#define DIAGRAM_COMP_DEFAULT "tpn"
/*t-topology;p-prototype;n-number*/
#define DIAGRAM_COMP "tpn"
#define MAX_COMP_D 4
#define PROTOTYPE_COMP_DEFAULT "tpn"
/*t-topology;p-prototype;n-number*/
#define PROTOTYPE_COMP "tpn"
#define MAX_COMP_P 4
#define MAX_LINE_NUM 50
#define DEFAULT_TEMPLATE_T "*"
#define DEFAULT_TEMPLATE_P "*"
/*tables:*/
#define ALL_FUNCTIONS_HASH_SIZE 101
#define ALL_COMMUTING_HASH_SIZE 101
#define VECTORS_HASH_SIZE 11
#define TOPOLOGY_HASH_SIZE 101
#define GTOPOLOGY_HASH_SIZE 17
#define PROTOTYPE_HASH_SIZE 101
#define MAINID_HASH_SIZE 151
/* init.c:*/
#define CNF_COMMENT '*'
#define COMMA_CHAR ','
#define MAX_MARKS_LENGTH 4
#define SYSTEM_PATH_NAME "DIANATMLPATH"
/*cnf_read.c:*/
#define FSIZE "fsize"
#define MULTIPLIER "10"
#define SHOW "dblshow"
#define D_EPS 2.0
#define SMALL_RAD 10.0
#define MAX_IMG_LEN 256
/* Prime number, for all index/fnum/num/lmnum etc. */
#define ID_REPLACE_TABLE_SIZE 101
#define INIT_MAX_COUPLING 4
#define MAX_INDICES_GROPUS 32
/* MAX_INDICES_INFO_LENGTH must be (MAX_INDICES_GROPUS+1)*2:*/
#define MAX_INDICES_INFO_LENGTH 66
#define VALID_ESC_CHAR "\\~!@#%$^&*"
#define MAX_INDEXID_LEN 9
#define INDEX_ID "ind"
#define MOMENTUM_ID "vec"
#define VL_COUNTER_ID "num"
#define LM_COUNTER_ID "mnum"
#define F_COUNTER_ID "fnum"
#define FFLOW_COUNTER_ID "fflow"
#define FROM_COUNTER_ID "fromv"
#define TO_COUNTER_ID "tov"
#define MAX_MOMENTUM_LEN 9
#define VALIDID "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define DELTA_EXT_MOMENTA 5
#define VEC_GROUP_HASH_SIZE 13
#define MAX_VEC_GROUP 20
#define MAX_EXT_PART 6
#define DELTA_ID 10
#define DELTA_TEXT 20
#define DEFAULTTOPOLOGYID "t%u_"
#define DEFAULTgTOPOLOGYID "g%u_"
#define ADDITIONALTOPOLOGYID "t%uA%u_"
#define ADDITIONALgTOPOLOGYID "g%uA%u_"

/*read_inp.c:*/
#define UNDEFINEDTOPOLOGYID "u%u_"
#define EXTRA_UNDEFINEDTOPOLOGYID "u%uA%u_"
/*browser.c:*/
#define TOPOLOGY 't'
#define NUMBER 'n'
#define PROTOTYPE 'p'
#define DELTA_PROTOTYPES_LIST 20

/*int2sig.c:*/
#define mSIGHUP 1
#define mSIGINT 2
#define mSIGQUIT 3
#define mSIGILL 4
#define mSIGABRT 5
#define mSIGFPE 6
#define mSIGKILL 7
#define mSIGSEGV 8
#define mSIGPIPE 9
#define mSIGALRM 10
#define mSIGTERM 11
#define mSIGUSR1 12
#define mSIGUSR2 13
#define mSIGCHLD 14
#define mSIGCONT 15
#define mSIGSTOP 16
#define mSIGTSTP 17
#define mSIGTTIN 18
#define mSIGTTOU 19
#define mSIGBUS 20
#define mSIGPOLL 21
#define mSIGPROF 22
#define mSIGSYS 23
#define mSIGTRAP 24
#define mSIGURG 25
#define mSIGVTALRM 26
#define mSIGXCPU 27
#define mSIGXFSZ 28
#define mSIGIOT 29
#define mSIGEMT 30
#define mSIGSTKFLT 31
#define mSIGIO 32
#define mSIGCLD 33
#define mSIGPWR 34
#define mSIGINFO 35
#define mSIGLOST 36
#define mSIGWINCH 37
#define mSIGUNUSED 38

/*..._T are bitmasks, ..._I are index (log_2(..._T), ...D - default for corresp. params:*/
#define SYNC_JOB_T 1
#define SYNC_JOB_I 0

#define STICKY_JOB_T 2
#define STICKY_JOB_I 1

#define FAIL_STICKY_JOB_T 4
#define FAIL_STICKY_JOB_I 2

#define ERRORLEVEL_JOB_T 8
#define ERRORLEVEL_JOB_I 3
#define ERRORLEVEL_JOB_D "-1"
#define ERRORLEVEL_JOB_Di -1

#define RESTART_JOB_T 16
#define RESTART_JOB_I 4
#define RESTART_JOB_D "00"
#define RESTART_JOB_Di 0

#define JOBINFO_TEMPLATE "1 12345678 12345678 12345678 12345678 12 12 12 12 12345678\n"
/*                        1    2       3         4         5     6  7  8  9    10   */
