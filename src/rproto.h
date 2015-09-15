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

/*************** Peer to chat ************************/
/* 01xxxx - data of length xxxx for swallowed:*/
#define p2cDATASW "01"
#define p2cDATASW_i  1

/* 02xxxx - data of length xxxx for stdout:*/
#define p2cDATASO "02"
#define p2cDATASO_i 2

/* 03xxxx - data of length xxxx for stderr:*/
#define p2cDATASE "03"
#define p2cDATASE_i 3

/* dubbing swallowed to stdout:*/
/*+:*/
#define p2cSW2STDOUTstart "05"
#define p2cSW2STDOUTstart_i 5
/*-*/
#define p2cSW2STDOUTstop "06"
#define p2cSW2STDOUTstop_i 6

/* Pass stdin to peer:*/
/*+*/
#define p2cSTDIN2PEERstart "07"
#define p2cSTDIN2PEERstart_i 7
/*-*/
#define p2cSTDIN2PEERstop "08"
#define p2cSTDIN2PEERstop_i 8

/* Pass stdin to swallowed:*/
/*+*/
#define p2cSTDIN2SWstart "09"
#define p2cSTDIN2SWstart_i 9
/*-*/
#define p2cSTDIN2SWstop "0a"
#define p2cSTDIN2SWstop_i 10

/*obxx - send a signal xx to swallowed, xx - in Diana's notation:*/
#define p2cSIG "0b"
#define p2cSIG_i 11

/*Read data from swallowed:*/
/*+*/
#define p2cREADSWstart "00"
#define p2cREADSWstart_i 0
/*-*/
#define p2cREADSWstop "10"
#define p2cREADSWstop_i 16

/*Is it alive?*/
#define p2cISALIVE "0c"
#define p2cISALIVE_i 12

/*04xxxx<filename> - fork into background redirecting swallowed stdout into a
file. xxxx is a length of a file name:*/
#define p2cFORK "04"
#define p2cFORK_i 4

/*Hands up to swallowed will finish, and then die:*/
#define p2cRUNANDDIE "20"
#define p2cRUNANDDIE_i 32

/*Hands up to swallowed will finish, and then go to the next job:*/
#define p2cRUNANDNEXT "21"
#define p2cRUNANDNEXT_i 33

/*Die!:*/
#define p2cDIE "0d"
#define p2cDIE_i 13

/*Go to the next task:*/
#define p2cNEXT "1c"
#define p2cNEXT_i 28

/*ok:*/
#define p2cOK "0e"
#define p2cOK_i 14

/*Somethin is wrong:*/
#define p2cNOK "0f"
#define p2cNOK_i 15

/***************Chat to peer ************************/

/* 11xxxx - data of length xxxx from swallowed:*/
#define c2pDATASW "11"
#define c2pDATASW_i 17

/*12xxxx - data of length xxxx from stdin:*/
#define c2pDATASI "12"
#define c2pDATASI_i 18

/*exec error:*/ /*Not used!*/
#define c2pEXECERR "13"
#define c2pEXECERR_i 19

/*A swallowed closed its stdin:*/
#define c2pSTDINCLOSED "14"
#define c2pSTDINCLOSED_i 20

/*A swallowed closed its stdout:*/
#define c2pSTDOUTCLOSED "1b"
#define c2pSTDOUTCLOSED_i 27

/*select() error:*/
#define c2pSELERR "15"
#define c2pSELERR_i 21

/*timeout:*/
#define c2pTIMEOUT "16"
#define c2pTIMEOUT_i 22
/*Reply "is_alibe":*/
/*Working:*/
#define c2pWRK "17"
#define c2pWRK_i 23
/*Stopped:*/
#define c2pSTOPPED "18"
#define c2pSTOPPED_i 24
/*Died, status is unknown:*/
#define c2pDIED "19"
#define c2pDIED_i 25
/*1axxxxxxxx Finished, xxxxxxxx is a status:*/
#define c2pFIN "1a"
#define c2pFIN_i 26
/* HEX status is: 8 HEX digits:
   xx   : if normal exit, then the first two digits repersents exit()
          argument, othervice 00
   xx   : 00 - notmal exit, 01 - non-caught signal, 02 - the job was not run,
          03 - no such a job, 04 - job is finished, but status is lost (often
          occur after debugger detached), 1# (with any #) - the job was removed
          by the user; otherwise - ff.
 xxxx   : - signal number ( if previous was 01), otherwise - 0000. If signal
            is incompatible, then 0000
*/

/*Send to peer just before leving mkchat:*/
#define c2pNEXT "1d"
#define c2pNEXT_i 29

/*Ok:*/
#define c2pOK "1e"
#define c2pOK_i 30

/*Something is wrong:*/
#define c2pNOK "1f"
#define c2pNOK_i 31

/***************Client to server ************************/

/* Die:*/
#define c2sDIE "30"
#define c2sDIE_i 48

/*Start a new job:*/
#define c2sJOB "31"
#define c2sJOB_i 49

/*Continue listening*/
#define c2sBYE "32"
#define c2sBYE_i 50

/*send a signal to SOME process:*/
#define c2sSENDSIG "33"
#define c2sSENDSIG_i 51

/***************Server to client************************/

/*Ok:*/
#define s2cOK "2e"
#define s2cOK_i 46

/*Something is wrong:*/
#define s2cNOK "2f"
#define s2cNOK_i 47

/*************** client to diana ************************/
/*Job is performed:*/
#define c2dJOBOK "40"
#define c2dJOBOK_i 64
/*Then the client sends the length of the job name (two hex digits)
 and then the name*/

/*All job are finished:*/
#define c2dALLJOB "41"
#define c2dALLJOB_i 65

/*Job is accepted*/
#define c2dJOBRUNNING "42"
#define c2dJOBRUNNING_i 66

/*Double job name*/
#define c2dWRONGJOBNAME "43"
#define c2dWRONGJOBNAME_i 67

/*number of running jobs (after that it sends one digit
              (a <length>) and then <length> HEX digits - the number of running jobs):*/
#define c2dREMJOBS "44"
#define c2dREMJOBS_i 68

/*Sticky job raquires unknown name:*/
#define c2dWRONGSTICKYNAME "45"
#define c2dWRONGSTICKYNAME_i 69

/*Sticky job reqires to stick to a job which already failed:*/
#define c2dFAILDTOSTICK "46"
#define c2dFAILDTOSTICK_i 70

/*************** diana to client ************************/
/*Perform a job:*/
#define d2cNEWJOB "50"
#define d2cNEWJOB_i 80
/*The proto here is:
 50XXnameXX{XXxxx...XXxxx... ...}

*/

/*Report all jobs are finished:*/
#define d2cREPALLJOB "51"
#define d2cREPALLJOB_i 81

/*Give up all jobs and return:*/
#define d2cFINISH "52"
#define d2cFINISH_i 82

/*Report number of running jobs:*/
#define d2cGETNREM "53"
#define d2cGETNREM_i 83

/*Get IP of a running job:*/
#define d2cGETIP "54"
#define d2cGETIP_i 84

/*Get status of a job:*/
#define d2cGETST "55"
#define d2cGETST_i 85

#define d2cKILLALL "56"
#define d2cKILLALL_i 86

#define d2cGETTIME "57"
#define d2cGETTIME_i 87

/* Do NOT report all jobs are finished:*/
#define d2cNOREPALLJOB "58"
#define d2cNOREPALLJOB_i 88

#define d2cGETsTIME "59"
#define d2cGETsTIME_i 89

#define d2cGETfTIME "5a"
#define d2cGETfTIME_i 90

#define d2cGETHITS "5b"
#define d2cGETHITS_i 91

#define d2cSETSTART_TO "5c"
#define d2cSETSTART_TO_i 92

#define d2cSETCONNECT_TO "5d"
#define d2cSETCONNECT_TO_i 93

#define d2cGETFAILQN "5e"
#define d2cGETFAILQN_i 94

#define d2cGETPID "5f"
#define d2cGETPID_i 95

#define d2cSENDSIG "60"
#define d2cSENDSIG_i 96

#define d2cGETNAME2STICK "61"
#define d2cGETNAME2STICK_i 97

#define d2cSTARTTRACE "62"
#define d2cSTARTTRACE_i 98

#define d2cSTOPTRACE "63"
#define d2cSTOPTRACE_i 99

#define d2cGETJINFO "64"
#define d2cGETJINFO_i 100

#define d2cRMJOB "65"
#define d2cRMJOB_i 101
