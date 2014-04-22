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

/*
 $Id: pat_tcl_user.c,v 1.6 1995/03/21 23:28:17 gurin Exp $
*/

/* An example of user-defined part for tcldisp */

#include <stdio.h>
#include <string.h>

#include "tcldisp.h"

void userinit1(void)
  {
  /* Tcl script is not yet called now */
  }

void userinit2(void)
  {
  /* Tcl script is initialized already */
  /* We are ready to start processing  */
  /* of X-events/incoming data         */
  }

void userfin(void)
  {
  /* Tcl script is deactivated */
  }

void connection_lost(int socket)
  {
  /* connection is lost on line 'socket' */
  fprintf(stderr,"Lost connection with the dispatcher. Local line %d.\n",
    socket);
  exit(1);
  } 

void react(int socket, char *tag, char *data, int datalen)
  {
  int rc;
  char *result;
  static char varname[] = "dispatch___buffer";
  static char buffer[100+sizeof(varname)];

  /* Set the special variable */
  Tcl_SetVar(interp,varname,data,0);

  /* we prepare the command 'tag $<varname>' */
  /* So we suppose that Tcl script contains the routine with the name 'tag' */
  /* and this routine accepts exactly one parameter */

  strcpy(buffer,tag); strcat(buffer," $"); strcat(buffer,varname);

  /* Now we have the tcl command in buffer. Let's call the Tcl */
  rc = Tcl_Eval(interp,buffer);
  result = interp->result;
  }

/* 
 If you want to send the data to the dispatcher from your reaction
 you can use one of two ways:
  - if you have only one connection line or if you send the data
    always on the same line from wich you have recived something
    then it's easy - you have a variable 'socket' in 'react', and
    you can do something like
      int rc;
      char *data = "Hello boys";
      char *tag = "URGENT";
      rc = put_tagged_bwait(socket,tag,data,strlen(data));

  - otherwise you have to find somehow the socket number
    and then use put_tagged_bwait as before
    One of the ways to find the socket:
      suppose you have in your Tcl script the lines
         global line1 line2
         set line1 [dispconnect "rschqa"]
         set line2 [dispconnect "rschqb"]
      include before the routines 'userinit2' and 'react'
        the declaration
         static int socket_a, socket_b;
      put in the routine userinit2 the lines
         socket_a = atoi(Tcl_GetVar(interp,"line1",0));
         socket_b = atoi(Tcl_GetVar(interp,"line2",0));
      use socket_a,socket_b in your calls to put_tagged_bwait
        inside 'react' routine

  In both cases you need the line
    #include "displowl.h"

*/
