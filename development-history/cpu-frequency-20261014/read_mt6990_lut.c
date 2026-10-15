/* Read-only inspection of the DT-declared MT6990 CPUFreq SRAM bank. */
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
int main(void)
{
    const off_t page = 0x11b000;
    int fd = open("/dev/mem", O_RDONLY | O_SYNC);
    if (fd < 0) { perror("open"); return 1; }
    void *p = mmap(NULL, 4096, PROT_READ, MAP_SHARED, fd, page);
    if (p == MAP_FAILED) { perror("mmap"); close(fd); return 1; }
    for (unsigned i = 0; i < 32; i++) {
        uint32_t v = *(volatile const uint32_t *)((const char *)p + 0xc10 + i * 4);
        printf("LUT %02u 0x%08x %u\n", i, v, (v & 0xfff) * 1000);
    }
    const unsigned offsets[] = { 0xc94, 0xc98, 0xc9c, 0xd20 };
    for (unsigned i = 0; i < sizeof(offsets)/sizeof(offsets[0]); i++)
        printf("REG 0x%lx 0x%08x\n", (unsigned long)page + offsets[i],
            *(volatile const uint32_t *)((const char *)p + offsets[i]));
    munmap(p, 4096);
    const off_t regaddr[] = { 0x100102f4, 0x0c53a2a0 };
    for (unsigned i=0; i<2; i++) {
        p=mmap(NULL,4096,PROT_READ,MAP_SHARED,fd,regaddr[i]&~(off_t)4095);
        if (p==MAP_FAILED) { perror("mmap PLL"); close(fd); return 1; }
        printf("PLL 0x%lx 0x%08x\n",(unsigned long)regaddr[i],
               *(volatile const uint32_t *)((const char *)p+(regaddr[i]&4095)));
        munmap(p,4096);
    }
    close(fd); return 0;
}
