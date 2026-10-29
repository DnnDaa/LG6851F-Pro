#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
extern int lg63_change_type_noop(void*,void*,int,void*);
typedef union { uint64_t align; unsigned char b[2048]; } blob;
int main(void){blob d={0},w={0},p={0};void *wp=w.b;int kind=3;unsigned checks=0;
 memcpy(d.b+976,&wp,8);memcpy(w.b+8,&kind,4);
 assert(lg63_change_type_noop(0,0,3,0)==-22);checks++;
 for(unsigned t=0;t<16;t++)for(int addr=-1;addr<=1;addr++)for(unsigned flags=0;flags<2;flags++)for(unsigned mac=0;mac<2;mac++)for(unsigned group=0;group<2;group++)for(unsigned follow=0;follow<2;follow++){
  memset(p.b,0,sizeof(p.b));memcpy(p.b+4,&addr,4);memcpy(p.b,&flags,4);p.b[8]=mac;
  uint64_t g=group,f=follow;memcpy(p.b+16,&g,8);memcpy(p.b+24,&f,8);
  int want=(t==3&&addr<=0&&!flags&&!mac&&!group&&!follow)?0:-95;
  assert(lg63_change_type_noop(0,d.b,t,p.b)==want);checks++;
  assert(lg63_change_type_noop(0,d.b,t,0)==(t==3?0:-95));checks++;
 }
 printf("{\"passed\":true,\"checks\":%u,\"actual_kernel_object_executed\":true}\n",checks);return 0;}
