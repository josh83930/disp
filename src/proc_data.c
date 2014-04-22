#include <stdio.h>
#include <stdlib.h>
#include "dispatch.h"

void proc_remote_data_(int *size, int *eods, int *irc)
{
  *irc = get_data((int *)data, *size); */
  if (*irc < 0) {
    fprintf(stderr," get_data failed\n");
    return;
  }
  int idx = 0;
  int nw = *size / sizeof(int);
  if( !data )
    {
      fprintf(stderr,"get_data returned no data\n");
      exit(1);
    }
  *eods = data[0] == 0;
  if( !*eods )
    while( idx < nw )
      {
	bank_(data+idx);
	idx += data[idx];
      }
}

void proc_data_(int *size, int *eods)
{
  int *data = (int *)get_data_addr(); 
  int idx = 0;
  int nw = *size / sizeof(int);
  if( !data )
    {
      fprintf(stderr,"get_data_addr() failed\n");
      exit(1);
    }
  *eods = data[0] == 0;
  if( !*eods )
    while( idx < nw )
      {
	bank_(data+idx);
	idx += data[idx];
      }
  unlock_data();
}

