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
 $Id: sendcmd.c,v 1.6 1996/10/16 12:41:35 chorusdq Exp $
*/


/* send the command to the dispatcher */

#include "displowl.h"

static void help(void)
    {
    printf("Format: sendcmd { host | -env } tag [ cmd ]\n");
    exit(1);
    }

int main(int argc, char *argv[])
  {
  int i;
  char cmd[5000];
  char *host;
  char bufhost[100];
  if( argc < 3 )
    help();

  host = argv[1];
  if( host[0] == '-' )
   if( strcmp(host,"-env") == 0 )
    {
    host = getenv("disphost");
    if( !host )
      {
      printf("Environment variable \"disphost\" is not defined\n");
      exit(1);
      }
    strcpy(bufhost,host);
    host = bufhost;
    }
   else
    help();

  if( strcmp(host,"none") == 0 )
    exit(0);

  cmd[0] = 0;
  for(i=3; i<argc; i++)
    {
    if( i != 3 )
      strcat(cmd," ");
    strcat(cmd,argv[i]);
    } 

  if( init_disp_link(host,"") < 0 )
    {
    socket_perror(argv[0]);
    exit(1);
    }

  if( put_zfullstring(argv[2],cmd) <= 0 )
    {
    socket_perror(argv[0]);
    exit(1);
    }
  drop_connection();
/*fprintf(stderr,"sendcmd %s %s\n",argv[2],cmd);
*/
  return 0;
  }
