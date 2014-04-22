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
  $Id: dispreact.c,v 1.2 1995/12/09 15:45:40 ruten Exp $
*/

#include "displowl.h"

#ifdef UNIX
static shdata_descr_t shdescr;
static int unlock_pending;
#endif

static void * buffer;
static int bufsize = 0;

typedef struct r_s {
  struct r_s *next;
  disp_reaction_t *reaction;
  char tag[TAGSIZE+1];
} r_t;

static r_t root = { NULL,NULL,""};

#define findit(p,tg) for(p=root.next; p!=NULL && strcmp(p->tag,tg) != 0; p=p->next)

void dispsetreact(const char *tag, disp_reaction_t *reaction)
  {
  r_t *p;
  if( tag == NULL || tag[0] == 0 )
    {
    root.reaction = reaction;
    return;
    }
  findit(p,tag);
  if( p == NULL )
    {
    if( reaction == NULL )  
      return; /* an attemt to drop non-existent reaction */
    p = (r_t *)malloc(sizeof(r_t));
    assert( p != NULL );
    p->next = root.next;
    root.next = p;
    p->reaction = reaction;
    strncpy(p->tag,tag,TAGSIZE);
    p->tag[TAGSIZE] = 0;
    }
   else
    {
    if( reaction == NULL )
      {
      r_t *prev;
      for(prev = &root; prev->next != p; prev = prev->next);
      prev->next = p->next;
      free(p);
      }
    else
      p->reaction = reaction;
    }
  }

void disploop(void)
  {
  int s;
  char tag[TAGSIZE+1];
  int size; 
  r_t *p;
  void *data;
  disp_reaction_t *reaction;

  for(;;)
    {
    dispallnext();
    s = dispselect(-1,-1);
    if( s < 0 )
      {
      if( bufsize )
        {
        free(buffer);
        bufsize = 0;
        }
      return; 
      }
    if( dispcheck(s,tag,&size,1) <= 0 )
      {
      dispdrop(s);
      continue;
      }
#ifdef UNIX
    unlock_pending = 0;
    if( size < 0 )
      {
      data = getshared(s,&shdescr);
      if( data == NULL )
         {
         dispdrop(s);
         continue;
         }
      size = -size;
      unlock_pending = 1;
      }
    else
#endif
    {
    assert( size >= 0 );
    if( size > bufsize )
      {
      free(buffer);
      bufsize = 0;
      buffer = malloc(size);
      if( buffer == NULL )
        {
        dispdrop(s);
        continue;
        }
      bufsize = size;
      data = buffer;
      if( getbwait(s,data,size) <= 0 )
        {
        dispdrop(s);
        continue;
        }
      }
    } 

    findit(p,tag);
    if( p == NULL )
      reaction = root.reaction;
    else
      reaction = p->reaction;
    if( reaction == NULL )
      {
      fprintf(stderr,"No reaction for tag %s\n",tag);
      dispdrop(s);
      continue;
      }
    if( size < 0 )
      size = -size; 
    (*reaction)(tag,data,size);
#ifdef UNIX
    if( unlock_pending )
      unlockshared(&shdescr);
#endif
    }
  }
