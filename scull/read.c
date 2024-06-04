#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int
main(int argc, char **argv)
{
	int len, fd = open("/dev/scull0", O_RDWR);
	char str[10];
	if (fd < 0) {
		printf("failed open device\n");
		return -1;
	} //failed open device
	printf("read length:\n");
	scanf("%d", len);
	len = read(fd, str, len);
	printf("read length is %d\n", len);
	close(fd);
	return 0;
}
