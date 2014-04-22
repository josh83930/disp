#include "displowl.h"
#include "dispvme.h"

#ifndef OS9
/* We simulate VME on shared memory */
#define host "local"
#define SZ (1024*1024)
#else
/* Using real VME */
#define host "rschv1"
#define SZ (16*1024*1024)
#endif

void main(int argc, char *argv[])
{
  int w;
  int s;
  int vme_addr;
  char *p;
  char *chunk; 
  char *prevp;
  int size;
  int counter;

  size = atoi(argv[1]);
  counter = atoi(argv[2]);

#ifndef OS9
  p = sh_mem_cre(argv[0],1,0666,SZ,&vme_addr,1);
  if( p == FAILPTR )
    {
    perror("sh_mem_cre");
    exit(1);
    } 
#else
  vme_addr = 0x10000000;
  p = (char *)0x80000000; 
#endif
  s = create_client_socket(host,DISPATCH_PORT);
  if( s < 0 )
    {
    perror("connect");
    exit(1);
    }
  if( vme_dsp_init(s,vme_addr,p,SZ) <= 0 )
    {
    perror("vme_dsp_init");
    exit(1);
    }
  printf("vme addr 0x%x, local addr 0x%p, socket %d, size %d, counter %d\n",
    vme_addr,p,s,size,counter);
  chunk = malloc(size);
  assert( chunk != NULL );
  prevp = NULL;
  for(w=0;w<counter;w++)
    {
    char *p;
    p = vme_dsp_getbuf("CHUNK",size);
    sprintf(chunk,"Data %d",w+1);
    memcpy(p,chunk,size);
    if( prevp > p )
       printf("wraparound %p %p\n",prevp,p);
    prevp = p;
    vme_dsp_notify(s,p);
    }
  printf("Sent %d\n",w*size);
  /* Wait a bit and destroy the memory */
#ifndef OS9
/*
  sleep(20);
  sh_mem_destroy(shared_memory_id);
*/
#endif
  exit(0); 
}
