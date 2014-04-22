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
 $Id: tclsupp.c,v 1.2 1995/05/10 14:01:29 ruten Exp $
*/

/* New Tcl commands are defined here:

 set line [dispconnect host] 
 dispmyid      $line nickname 
 dispsubscribe $line subscription 
 dispsend      $line tag data  
 dispdrop      $line 
 dispwhereis   disphost nickname      

e.g.:

 set line [dispconnect "rschqa.cern.ch"] 
 dispmyid      $line "Daddy" 
 dispsubscribe $line "a DAQERROR a DAQMAIN" 
 dispsend      $line "URGENT" "Hello, boys!"  
 dispdrop      $line 
 set clients_host [dispwhereis "rschqa" "ERRLOG"]

*/

/* Tcl interfaces, user defined routines */
#include "tclsupp.h"

/* dispatcher communications */
#include "displowl.h"

/* procedure to communicate with the dispatcher */

static Tcl_CmdProc DispConnect;
static Tcl_CmdProc DispMyId;
static Tcl_CmdProc DispSubscribe;
static Tcl_CmdProc DispSend;
static Tcl_CmdProc DispDrop;
static Tcl_CmdProc DispWhereIs;

/* reaction on the data from dispatcher */
static void SocketReaction(ClientData clientData, int mask);

/* final cleanup */
static void shutsockets(void);

/* close one line */
static void sockoff(int socket);

/* used sockets */
static fd_set used_sockets;
static int used_width;

/**********************************************************************/

void dispinit(Tcl_Interp *interp)
{
  /* create Tcl commands for the dispatcher buisness */
  Tcl_CreateCommand(interp, "dispconnect", DispConnect, (ClientData*) NULL,
                    (Tcl_CmdDeleteProc*) NULL);
  Tcl_CreateCommand(interp, "dispmyid", DispMyId, (ClientData*) NULL,
                    (Tcl_CmdDeleteProc*) NULL);
  Tcl_CreateCommand(interp, "dispsubscribe", DispSubscribe, (ClientData*) NULL,
                    (Tcl_CmdDeleteProc*) NULL);
  Tcl_CreateCommand(interp, "dispsend", DispSend, (ClientData*) NULL,
                    (Tcl_CmdDeleteProc*) NULL);
  Tcl_CreateCommand(interp, "dispdrop", DispDrop, (ClientData*) NULL,
                    (Tcl_CmdDeleteProc*) NULL);
  Tcl_CreateCommand(interp, "dispwhereis", DispWhereIs, (ClientData*) NULL,
                    (Tcl_CmdDeleteProc*) NULL);

  FD_ZERO(&used_sockets);
  used_width = 0; 
  atexit(shutsockets);
}

/**********************************************************************/

void shutsockets(void)
  {
  int i;
  for(i=0; i<used_width; i++)
    if( FD_ISSET(i,&used_sockets) )
      sockoff(i);
  }

void sockoff(int sock)
  {
  shut_line(sock);
  FD_CLR(sock,&used_sockets);
  }

static char wrongnargs[] = "wrong # of arguments";
static char wrongsock[] = "wrong socket argument";
static char noconnection[] = "no connection to dispatcher";

/********************************************************************/
/* set line [dispconnect host] */
int DispConnect(ClientData z, Tcl_Interp* interp, int argc, char** argv)
{
  int sock;
  if( argc != 2 )
    {
    interp->result = wrongnargs;
    return TCL_ERROR;
    }
  sock = create_client_socket(argv[1],DISPATCH_PORT);
  if( sock < 0 ||
      put_tagged_bwait(sock,DISPTAG_Always,NULL,0) < 0 )
    {
    shut_line(sock);
    interp->result = noconnection;
    return TCL_ERROR;
    }
  FD_SET(sock,&used_sockets);
  if( sock >= used_width )
    used_width = sock+1;
  Tk_CreateFileHandler(sock,TK_READABLE,SocketReaction,(ClientData)sock);  
  sprintf(interp->result,"%d%c",sock,0);
  return TCL_OK;
}

/* dispmyid line nickname */
int DispMyId(ClientData z, Tcl_Interp* interp, int argc, char** argv)
{
  int sock;
  if( argc != 3 )
    {
    interp->result = wrongnargs;
    return TCL_ERROR;
    }
  sock = atoi(argv[1]);
  if( !FD_ISSET(sock,&used_sockets) )
    {
    interp->result = wrongsock;
    return TCL_ERROR;
    }
  if( put_tagged_bwait(sock,DISPTAG_MyId,argv[2],strlen(argv[2])) < 0 )
    {
    sockoff(sock);
    connection_lost(sock);
    interp->result = noconnection;
    return TCL_ERROR;
    }
  return TCL_OK;
}

/* dispsubscribe line subscription */
int DispSubscribe(ClientData z, Tcl_Interp* interp, int argc, char** argv)
{
  int sock;
  if( argc != 3 )
    {
    interp->result = wrongnargs;
    return TCL_ERROR;
    }
  sock = atoi(argv[1]);
  if( !FD_ISSET(sock,&used_sockets) )
    {
    interp->result = wrongsock;
    return TCL_ERROR;
    }
  if( put_tagged_bwait(sock,DISPTAG_Subscribe,argv[2],strlen(argv[2])) < 0 )
    {
    sockoff(sock);
    connection_lost(sock);
    interp->result = noconnection;
    return TCL_ERROR;
    }
  return TCL_OK;
}

/* dispsend line tag data  */
int DispSend(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int sock;
  if( argc != 4 )
    {
    interp->result = wrongnargs;
    return TCL_ERROR;
    }
  sock = atoi(argv[1]);
  if( !FD_ISSET(sock,&used_sockets) )
    {
    interp->result = wrongsock;
    return TCL_ERROR;
    }
  if( put_tagged_bwait(sock, argv[2], argv[3], strlen(argv[3])) < 0 )
    {
    sockoff(sock);
    connection_lost(sock);
    interp->result = noconnection;
    return TCL_ERROR;
    }
  return TCL_OK;
}

/* dispdrop line */
int DispDrop(ClientData z, Tcl_Interp* interp, int argc, char** argv)
{
  int sock;

  if( argc != 2 )
    {
    interp->result = wrongnargs;
    return TCL_ERROR;
    }
  sock = atoi(argv[1]);
  if( !FD_ISSET(sock,&used_sockets) )
    {
    interp->result = wrongsock;
    return TCL_ERROR;
    }
  Tk_DeleteFileHandler(sock);
  sockoff(sock);
  return TCL_OK;
}

/* dispwhereis   disphost nickname */
int DispWhereIs(ClientData z, Tcl_Interp* interp, int argc, char** argv)
{
  int rc;
  char buf[1000];

  if( argc != 3 )
    {
    interp->result = wrongnargs;
    return TCL_ERROR;
    }
  rc = whereis(argv[1],argv[2],buf,sizeof(buf));
  if( rc < 0 )
    Tcl_AppendElement(interp,"-1");
  else if( rc == 0 )
    ;
  else
    {
    char *p;
    for(p = strtok(buf," "); p != NULL; p = strtok(NULL," "))
      Tcl_AppendElement(interp,p);
    }
  return TCL_OK; 
}
 

/********************************************************************/

static char buffer[1000];

/********************************************************************/
static void SocketReaction(ClientData clientData, int mask)
  {
  int rc;
  prefix_t header;
  char tag[TAGSIZE+1];
  int size;
  int sock = (int)clientData;
  static int maxlen = sizeof(buffer)-1;

  rc = getbwait(sock,&header,sizeof(prefix_t));
  fromprefix(&header,tag,&size);
  if( size < 0 )
    rc = -1;
  if( rc > 0 )
    {
    if( size > maxlen )
      {
      rc = getbwait(sock,buffer,maxlen);
      if( rc > 0 )
        rc = skipbwait(sock,size-maxlen);
      size = maxlen;
      }
    else 
      rc = getbwait(sock,buffer,size);
    }
  if( rc <= 0 )
    {
    sockoff(sock);
    connection_lost(sock);
    return;
    }
  buffer[size] = 0;
  react(sock,tag,buffer,size);
  }
