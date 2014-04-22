/*

Header: utils.c[1.6] Sun Aug  9 03:48:00 1992 nickel@cs.tu-berlin.de proposed
This file is part of socket(1).
Copyright (C) 1992 by Juergen Nickelsen <nickel@cs.tu-berlin.de>
except the file siglist.c, which is Copyright (C) 1989 Free Software
Foundation, Inc.

This applies to the Socket program, release Socket-1.0.

Socket is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 1, or (at your option) any
later version.

It is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You may have received a copy of the GNU General Public License along
with GNU Emacs or another GNU program; see the file COPYING.  If not,
write to me at the electronic mail address given above or to 
Juergen Nickelsen, Hertzbergstr. 28, 1000 Berlin 44, Germany.
*/

/*
Modified by Ruten Gurin, <ruten@caspur.it>
*/

/*
 *  Modified by PEWG for Solaris. (12-12-97).  It appears that signals are
 * not ignored by calling a "do-nothing" routine (e.g. ignorsig here).  But
 * the system call 'sigignore' seems to behave as advertised.
 */

#if defined(VM) || defined(MVS) || defined(VMS) || defined(OS9) || defined(Darwin)
void init_signals(void){}
#else

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#ifdef AIX
extern sigvec(int signal, struct sigvec *in, struct sigvec *out);
#endif

extern void initialize_siglist(void);

#ifndef Linux
extern char *sys_siglist[];
#endif

/* Signal handler, print message and exit */
static void sayandexit(int sig)
{
    time_t t;
    time(&t);
    fprintf(stderr, "\nsignal %s caught, exiting. %s", 
	sys_siglist[sig],ctime(&t)) ;
    exit(-sig) ;
}

/* Signal handler, print message */
static void sayandignore(int sig)
{
    time_t t;
    time(&t);
    fprintf(stderr, "\nsignal %s caught, ignored. %s", 
	sys_siglist[sig],ctime(&t)) ;
}

/* Signal handler, exit */
static void quietexit(int sig)
{
    exit(-sig) ;
}

/* Signal handler, just ignore the signal */
static void ignorsig(int sig)
{
}

typedef void sigproc_t(int signal);

/* set up handling of a specific signal */
#if defined(SIG_SETMASK) && (!defined(Linux) && !defined(Solaris))              /* only with BSD signals */
static void init_signal(int sig, sigproc_t *proc)
{
    static struct sigvec svec = { NULL, ~0, 0 } ;
    svec.sv_handler = proc;    
    sigvec(sig, &svec, NULL) ;
}
#else
#define init_signal(sig,proc)  signal(sig, proc)
#endif

/* set up signal handling. */
void init_signals(void)
{
    int i ;

    initialize_siglist() ;      /* shamelessly stolen from BASH */
    for (i = 1; i < NSIG; i++)  /* signal 0 doesn't exist */
      switch(i)
        {
#ifdef SIGTSTP
        case SIGTSTP:
        case SIGCONT:
           break;
#endif
        case SIGKILL:
        case SIGSTOP:
           break;     /* non-interceptable */
        case SIGHUP:
	   init_signal(i,sayandignore); 
           break;
        case SIGCLD:
        case SIGQUIT:
        case SIGWINCH:
           break;
        case SIGUSR1:
	   init_signal(i,quietexit); 
           break;
        case SIGPIPE:
/* PWG
 * 'ignorsig' appears not to work reliably with Solaris.
 * but the system call to ignore the signal does.
 */
#ifdef Solaris
	  sigignore (i);
#else
	  init_signal(i,ignorsig);
#endif
           break;
        default:
           init_signal(i,sayandexit);
           break;
    }
}
#endif

