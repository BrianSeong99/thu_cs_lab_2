#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>

#define MY_MMAP_LEN (256)
#define PAGE_SIZE 4096

static char *exec_name;

size_t libkdump_virt_to_phys(size_t virtual_address) {
    /*static int pagemap = -1;
    if (pagemap == -1) {
        pagemap = open("/proc/self/pagemap", O_RDONLY);
        if (pagemap < 0) {
            return 0;
        }
    }*/
    int pagemap = -1;
    pagemap = open("/proc/self/pagemap", O_RDONLY);
    if (pagemap < 0) {
        return 0;
    }
	printf("pagemap %x\n", pagemap);
    uint64_t value;
    int got = pread(pagemap, &value, 8, (virtual_address / 0x1000) * 8);
	printf("value %lx\n", value);
    if (got != 8) {
        return 0;
    }
    uint64_t page_frame_number = value & ((1ULL << 54) - 1);
	printf("page_frame_number %lx\n", page_frame_number);
    if (page_frame_number == 0) {
        return 0;
    }
	return page_frame_number * 0x1000 + virtual_address % 0x1000;
}

int main ( int argc, char **argv )
{
  int fd;
  char *address = NULL;
  char *sbuff;
  int i;

  exec_name = argv[0];
  sbuff = (char*) calloc(MY_MMAP_LEN,sizeof(char));

  fd = open("/dev/expdev", O_RDWR);
  if(fd < 0)
  {
    perror("Open call failed");
    return -1;
  }

  address = mmap( NULL, MY_MMAP_LEN * PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (address == MAP_FAILED)
  {
    perror("mmap operation failed");
    return -1;
  }

printf("%lx\n", libkdump_virt_to_phys((long unsigned int)address));

 for(i=0; i<80; i++)
  {
    printf("%16p: %d\n",address+i, (char)*(address+i));
  }

  memcpy(address, exec_name,80);
  for(i=0; i<80; i++)
  {
    printf("%16p: %c\n",address+i, (char)*(address+i));
  }

  if (munmap(address, MY_MMAP_LEN) == -1)
  {
	  perror("Error un-mmapping the file");
  }
  close(fd);
  return 0;
}
