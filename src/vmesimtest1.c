#include "displowl.h"
#include "dispvme.h"

#ifndef OS9
/* We simulate VME on shared memory */
#define host "local"
#define SZ (28*230)
#else
/* Using real VME */
#define host "rschv1"
#define SZ (16*1024*1024)
#endif

static int t[] = {
23,
23,
23,
23,
23,
23,
23,
23,
23,
23,
23,
23,
23,
23,
230,
23,
23,
23,
23,
23,
23,
23,
23,
23,
23,
23,
0
};

void main(int argc, char *argv[])
{
  int w;
  int s;
  int vme_addr;
  char *p;

#ifndef OS9
  p = sh_mem_cre(argv[0],1,0666,SZ,&vme_addr,1);
  if( p == FAILPTR )
    {
    perror("sh_mem_cre");
    exit(1);
    } 
#else
  vme_addr = 0x10000000;
  p = 0x80000000; 
#endif
  printf("vme addr %x, local addr %x\n",vme_addr,p);
  s = create_client_socket(host,DISPATCH_PORT);
  printf("socket %d\n",s);
  if( s < 0 )
    {
    perror("coonect");
    exit(1);
    }
  printf("sending init\n"); 
  if( vme_dsp_init(s,vme_addr,p,SZ) <= 0 )
    {
    perror("vme_dsp_init");
    exit(1);
    }
  printf("init done\n");
  for(w=0;w<sizeof(t)/sizeof(t[0]);w++)
    {
    char *p;
    p = vme_dsp_getbuf("ZAG-5",10*t[w]);
    if( t[w] >= 10 )
      sprintf(p,"Data %d",w+1);
    else
      memset(p,'Z',t[w]);
    printf("notifying %p\n",p);
    vme_dsp_notify(s,p);
    }
  /* Wait a bit and destroy the memory */
/*
  sleep(20);
  sh_mem_destroy(shared_memory_id);
*/
  exit(0); 
}
