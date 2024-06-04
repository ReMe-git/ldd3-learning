#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int
main(int argc, char **argv)
{
	int res, fd = open("/dev/scull0", O_RDWR);
	char str[10];
	if (fd < 0) {
		printf("failed open device\n");
		return -1;
	} //failed open device
	printf("write something:\n");
	scanf("%s", str);
	res = write(fd, str, strlen(str));
	printf("write length is %d\n", res);
	close(fd);
	return 0;
}
