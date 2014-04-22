/*##########################################################################*/
/*                                                                          */
/* Copyright (c) 1993-1994 CASPUR Consortium                                */
/*                         c/o Universita' "La Sapienza", Rome, Italy       */
/* All rights reserved.                                                     */
/*                                                                          */
/* Permission is hereby granted, without written agreement and without      */
/* license or royalty fees, to use, copy, modify, and distribute this       */
/* software and its documentation for any purpose, provided that the        */
/* above copyright notice and the following two paragraphs and the team     */
/* reference appear in all copies of this software.                         */
/*                                                                          */
/* IN NO EVENT SHALL THE CASPUR CONSORTIUM BE LIABLE TO ANY PARTY FOR       */
/* DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING  */
/* OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE       */
/* CASPUR CONSORTIUM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    */
/*                                                                          */
/* THE CASPUR CONSORTIUM SPECIFICALLY DISCLAIMS ANY WARRANTIES,             */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY */
/* AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER   */
/* IS ON AN "AS IS" BASIS, AND THE CASPUR CONSORTIUM HAS NO OBLIGATION TO   */
/* PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.   */
/*                                                                          */
/*       +----------------------------------------------------------+       */
/*       |   The ControlHost Team: Ruten Gurin, Andrei Maslennikov  |       */
/*       |   Contact e-mail      : ControlHost@caspur.it            |       */
/*       +----------------------------------------------------------+       */
/*                                                                          */
/*##########################################################################*/

/*
 $Id: findclient.c,v 1.3 1995/04/25 18:13:38 ruten Exp $ 
*/

/*
  This program connects to the dispatcher on a given host
  and prints the list of hosts from which the clients
  with the given nickname are connected to that dispatcher.
  There are three possibilities:
    1) Connection to dispatcher failed:
       exit code 1, empty output
    2) No clients with the given nickname:
       exit code 0, empty output
    3) Otherwise:
       exit code 0, list of hostnames on output
*/  

#include <stdlib.h>
#include <stdio.h>

#include "dispatch.h"

int main(int argc, char *argv[])
  {
  char host[2000];
  int rc;
  if( argc != 3 )
    {
    fprintf(stderr,"Format: %s disp_host nickname\n",argv[0]);
    exit(1);;
    }
  rc = whereis(argv[1],argv[2],host,sizeof(host));
  if( rc < 0 )
    exit(1);
  if( rc > 0 )
    puts(host);
  return 0;
  }
