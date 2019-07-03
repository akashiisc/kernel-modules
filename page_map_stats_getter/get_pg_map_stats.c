#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>


#define PAGE_MAP_STATS _IOWR('a' , 'b' , int32_t*)


int main(int argc , char **argv)
{
        int fd;
        int32_t value, number;
        fd = open("/dev/page_map_stats_device", O_RDWR);
        if(fd < 0) {
                printf("Cannot open device file...\n");
                return 0;
        }
        char *x ;
        unsigned long pfn_value = strtoul(argv[1] , &x , 16);
        ioctl(fd, PAGE_MAP_STATS, (int32_t*) &pfn_value);
        close(fd);
}

