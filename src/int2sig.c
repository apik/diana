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
#include <signal.h>
#include "comdef.h"

/*Converts system signals to diana's internal:*/
int sig2int(int thesignal)
{

/* Can't use switch here since signals are multiply!*/
#ifdef SIGHUP
       if(thesignal == SIGHUP) return mSIGHUP;
#endif
#ifdef SIGINT
       if(thesignal == SIGINT) return mSIGINT;
#endif
#ifdef SIGQUIT
       if(thesignal == SIGQUIT) return mSIGQUIT;
#endif
#ifdef SIGILL
       if(thesignal == SIGILL) return mSIGILL;
#endif
#ifdef SIGABRT
       if(thesignal == SIGABRT) return mSIGABRT;
#endif
#ifdef SIGFPE
       if(thesignal == SIGFPE) return mSIGFPE;
#endif
#ifdef SIGKILL
       if(thesignal == SIGKILL) return mSIGKILL;
#endif
#ifdef SIGSEGV
       if(thesignal == SIGSEGV) return mSIGSEGV;
#endif
#ifdef SIGPIPE
       if(thesignal == SIGPIPE) return mSIGPIPE;
#endif
#ifdef SIGALRM
       if(thesignal == SIGALRM) return mSIGALRM;
#endif
#ifdef SIGTERM
       if(thesignal == SIGTERM) return mSIGTERM;
#endif
#ifdef SIGUSR1
       if(thesignal == SIGUSR1) return mSIGUSR1;
#endif
#ifdef SIGUSR2
       if(thesignal == SIGUSR2) return mSIGUSR2;
#endif
#ifdef SIGCHLD
       if(thesignal == SIGCHLD) return mSIGCHLD;
#endif
#ifdef SIGCONT
       if(thesignal == SIGCONT) return mSIGCONT;
#endif
#ifdef SIGSTOP
       if(thesignal == SIGSTOP) return mSIGSTOP;
#endif
#ifdef SIGTSTP
       if(thesignal == SIGTSTP) return mSIGTSTP;
#endif
#ifdef SIGTTIN
       if(thesignal == SIGTTIN) return mSIGTTIN;
#endif
#ifdef SIGTTOU
       if(thesignal == SIGTTOU) return mSIGTTOU;
#endif
#ifdef SIGBUS
       if(thesignal == SIGBUS) return mSIGBUS;
#endif
#ifdef SIGPOLL
       if(thesignal == SIGPOLL) return mSIGPOLL;
#endif
#ifdef SIGPROF
       if(thesignal == SIGPROF) return mSIGPROF;
#endif
#ifdef SIGSYS
       if(thesignal == SIGSYS) return mSIGSYS;
#endif
#ifdef SIGTRAP
       if(thesignal == SIGTRAP) return mSIGTRAP;
#endif
#ifdef SIGURG
       if(thesignal == SIGURG) return mSIGURG;
#endif
#ifdef SIGVTALRM
       if(thesignal == SIGVTALRM) return mSIGVTALRM;
#endif
#ifdef SIGXCPU
       if(thesignal == SIGXCPU) return mSIGXCPU;
#endif
#ifdef SIGXFSZ
       if(thesignal == SIGXFSZ) return mSIGXFSZ;
#endif
#ifdef SIGIOT
       if(thesignal == SIGIOT) return mSIGIOT;
#endif
#ifdef SIGEMT
       if(thesignal == SIGEMT) return mSIGEMT;
#endif
#ifdef SIGSTKFLT
       if(thesignal == SIGSTKFLT) return mSIGSTKFLT;
#endif
#ifdef SIGIO
       if(thesignal == SIGIO) return mSIGIO;
#endif
#ifdef SIGCLD
       if(thesignal == SIGCLD) return mSIGCLD;
#endif
#ifdef SIGPWR
       if(thesignal == SIGPWR) return mSIGPWR;
#endif
#ifdef SIGINFO
       if(thesignal == SIGINFO) return mSIGINFO;
#endif
#ifdef SIGLOST
       if(thesignal == SIGLOST) return mSIGLOST;
#endif
#ifdef SIGWINCH
       if(thesignal == SIGWINCH) return mSIGWINCH;
#endif
#ifdef SIGUNUSED
       if(thesignal == SIGUNUSED) return mSIGUNUSED;
#endif

   return 0;
}/*sig2int*/

/*Converts diana's internal signals to system ones:*/
int int2sig(int thesignal)
{
   switch(thesignal){
#ifdef SIGHUP
       case 1: return SIGHUP;
#endif
#ifdef SIGINT
       case 2: return SIGINT;
#endif
#ifdef SIGQUIT
       case 3: return SIGQUIT;
#endif
#ifdef SIGILL
       case 4: return SIGILL;
#endif
#ifdef SIGABRT
       case 5: return SIGABRT;
#endif
#ifdef SIGFPE
       case 6: return SIGFPE;
#endif
#ifdef SIGKILL
       case 7: return SIGKILL;
#endif
#ifdef SIGSEGV
       case 8: return SIGSEGV;
#endif
#ifdef SIGPIPE
       case 9: return SIGPIPE;
#endif
#ifdef SIGALRM
       case 10: return SIGALRM;
#endif
#ifdef SIGTERM
       case 11: return SIGTERM;
#endif
#ifdef SIGUSR1
       case 12: return SIGUSR1;
#endif
#ifdef SIGUSR2
       case 13: return SIGUSR2;
#endif
#ifdef SIGCHLD
       case 14: return SIGCHLD;
#endif
#ifdef SIGCONT
       case 15: return SIGCONT;
#endif
#ifdef SIGSTOP
       case 16: return SIGSTOP;
#endif
#ifdef SIGTSTP
       case 17: return SIGTSTP;
#endif
#ifdef SIGTTIN
       case 18: return SIGTTIN;
#endif
#ifdef SIGTTOU
       case 19: return SIGTTOU;
#endif
#ifdef SIGBUS
       case 20: return SIGBUS;
#endif
#ifdef SIGPOLL
       case 21: return SIGPOLL;
#endif
#ifdef SIGPROF
       case 22: return SIGPROF;
#endif
#ifdef SIGSYS
       case 23: return SIGSYS;
#endif
#ifdef SIGTRAP
       case 24: return SIGTRAP;
#endif
#ifdef SIGURG
       case 25: return SIGURG;
#endif
#ifdef SIGVTALRM
       case 26: return SIGVTALRM;
#endif
#ifdef SIGXCPU
       case 27: return SIGXCPU;
#endif
#ifdef SIGXFSZ
       case 28: return SIGXFSZ;
#endif
#ifdef SIGIOT
       case 29: return SIGIOT;
#endif
#ifdef SIGEMT
       case 30: return SIGEMT;
#endif
#ifdef SIGSTKFLT
       case 31: return SIGSTKFLT;
#endif
#ifdef SIGIO
       case 32: return SIGIO;
#endif
#ifdef SIGCLD
       case 33: return SIGCLD;
#endif
#ifdef SIGPWR
       case 34: return SIGPWR;
#endif
#ifdef SIGINFO
       case 35: return SIGINFO;
#endif
#ifdef SIGLOST
       case 36: return SIGLOST;
#endif
#ifdef SIGWINCH
       case 37: return SIGWINCH;
#endif
#ifdef SIGUNUSED
       case 38: return SIGUNUSED;
#endif
      default: break;
   }/*switch(thesignal)*/
   return 0;
}/*int2sig*/
