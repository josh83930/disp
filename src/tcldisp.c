/*
############################################################################
#                                                                          #
# Copyright (c) 1993-1994 CASPUR Consortium                                # 
#                         c/o Universita' "La Sapienza", Rome, Italy       #
# All rights reserved.                                                     #
#                                                                          #
# Permission is hereby granted, without written agreement and without      #
# license or royalty fees, to use, copy, modify, and distribute this       #
# software and its documentation for any purpose, provided that the        #
# above copyright notice and the following two paragraphs and the team     #
# reference appear in all copies of this software.                         #
#                                                                          #
# IN NO EVENT SHALL THE CASPUR CONSORTIUM BE LIABLE TO ANY PARTY FOR       #
# DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING  #
# OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE       #
# CASPUR CONSORTIUM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    #
#                                                                          #
# THE CASPUR CONSORTIUM SPECIFICALLY DISCLAIMS ANY WARRANTIES,             #
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY #
# AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER   #
# IS ON AN "AS IS" BASIS, AND THE CASPUR CONSORTIUM HAS NO OBLIGATION TO   #
# PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.   #
#                                                                          #
#       +----------------------------------------------------------+       #
#       |   The ControlHost Team: Ruten Gurin, Andrei Maslennikov  |       #
#       |   Contact e-mail      : ControlHost@caspur.it            |       #
#       +----------------------------------------------------------+       #
#                                                                          #
############################################################################
*/
/***********************************************************************/
/* Tcl/Tk initialisation code is taken from the program by             */
/* Kees van der Poel (NIKHEF, Amsterdam)                               */
/***********************************************************************/

/*
 $Id: tcldisp.c,v 1.16 1996/05/15 09:44:14 ruten Exp $
*/

/* See tclsupp.c for the description of new Tcl commands */

/* Interfaces */
#include "tcldisp.h"
#if !defined(IRIX) && !defined(Linux)
#include <tclExtend.h>
#endif


/* Tcl interpreter */
Tcl_Interp  *interp;

/* id for perror */
static char *progname;

static void help(void)
{
  fprintf(stderr,
    "Format: %s [ -name WindowTitle ] [ -display Display ] tclfile [ tclfile_args ]\n",
     progname);
  exit(1);
}

/**********************************************************************/

int main(int argc, char **argv)
{
  Tk_Window   mainWindow;
  int                code;
  char        *tclfile;
  char        *title = NULL;
  char        *display = NULL;

  progname = argv[0];

  while( argc > 1 && argv[1][0] == '-' )
    {
    if( argc == 2 )
      help();
    if( strcmp(argv[1],"-name") == 0 )
      title = argv[2];
    else if( strcmp(argv[1],"-display") == 0 )
      display = argv[2];
    else
      help();
    argc -= 2;
    argv += 2;
    }
 
  if( argc < 2 )
    help();

  tclfile = argv[1]; 

  if( title == NULL )
    title = tclfile;

  progname = tclfile;

  /* Make a Tcl interpreter */
  interp = Tcl_CreateInterp();

  Tcl_SetVar(interp, "tcl_interactive", "0", TCL_GLOBAL_ONLY);
#if !defined(IRIX) && !defined(Linux)
  TclX_Init(interp);
#else
  Tcl_Init(interp);
#endif
  if (*interp->result != 0) {
    fprintf(stderr,"%s: Tcl_Init: %s.\n",progname,interp->result);
    exit(1);
  }

  mainWindow = Tk_CreateMainWindow(interp, display, title, "TK");
  if (mainWindow == NULL) {
    fprintf(stderr,"%s: Tk_CreateMainWindow: %s.\n",progname,interp->result);
    exit(1);
  }

  Tk_Init(interp);
  if (*interp->result != 0) {
    fprintf(stderr,"%s: Tk_Init: %s.\n",progname,interp->result);
    exit(1);
  }

  dispinit(interp);

  userinit1();
  atexit(userfin);

  argc--;
  argv++;
  {
  char buf[20];
  sprintf(buf,"%d",argc-1);
  Tcl_SetVar(interp,"argc",buf,TCL_GLOBAL_ONLY);
  }
  Tcl_SetVar(interp,"argv0",tclfile,TCL_GLOBAL_ONLY);
  if( argc == 1 )
    Tcl_SetVar(interp,"argv","",TCL_GLOBAL_ONLY);
  else
    Tcl_SetVar(interp,"argv",argv[1],TCL_GLOBAL_ONLY|TCL_LIST_ELEMENT);
  while( argc > 2 )
    {
    Tcl_SetVar(interp,"argv",argv[2],
      TCL_GLOBAL_ONLY|TCL_APPEND_VALUE|TCL_LIST_ELEMENT);
    argc--;
    argv++;
    }
  /* Invoke the script of the TK application */
  code = Tcl_VarEval(interp,"source ", tclfile, NULL); 
  if (code != TCL_OK) {
    fprintf(stderr,"%s: Error: %s.\n",progname,interp->result);
    exit(1);
  }

  userinit2();

  Tk_MainLoop();

  exit(0);
}
