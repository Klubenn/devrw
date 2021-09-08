#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <zconf.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/stat.h>

typedef struct		s_data
{
	int				operation;		// op
	char			*device;		// dev
	unsigned int	blocksize;		// bs
	unsigned int	count;			// count
	unsigned int	startsector; 	// sts
	unsigned int 	pause;			// p
	unsigned int 	verbose;		// v
	unsigned int 	sync;			// sync
}					t_data;

// devrw  op=wr dev=/dev/sda bs=64k count=1 sts=5 pause=5

void usage(char *name)
{
	printf("Usage:\t%s op=r|w dev=str bs=int count=int [sts=int] [p=int] [v=0|1] [sync=0|1]\n"
  		"\top - операция чтения (r) или записи (w)\n\tdev - путь до устройства\n"
		"\tbs - размер блока для чтения/записи в кб\n\tcount - количество блоков для чтения/записи\n"
  		"\tsts - начальный сектор (по умолчанию 0)\n\tp - пауза между операциями чтения/записи (по умолчанию 0)\n"
		"\tv - вывод данных о текущей операции: 0 или 1 (по умолчанию 0)\n"
  		"\tsync - синхронизация при выполнении r/w операций (по умолчанию 1)\n", name);
	exit(0);
}

int my_exit(char *str, t_data *data, int res, int fd)
{
	printf("An error occured: %s. Exiting.\n", str);
	close(fd);
	if (data && data->device)
		free(data->device);
	exit(res);
}

void parse_args(int ac, char **av, t_data *data)
{
	char *position;
	long op_len;
	char flag = 0;
	char sync = 1;

	if (ac == 1 || !strcmp(av[1], "-h"))
		usage(av[0]);
	while (--ac > 0)
	{
		position = strchr(av[ac], '=');
		if (position)
		{
			op_len = position - av[ac];
			if (strncmp(av[ac], "op", op_len) == 0)
			{
				if (position[1] == 'w')
					data->operation = O_WRONLY;
				else if (position[1] == 'r')
					data->operation = O_RDONLY;
				else
					usage(av[0]);
				flag = 1;
			}
			else if (strncmp(av[ac], "dev", op_len) == 0)
				data->device = strdup(position + 1);
			else if (strncmp(av[ac], "bs", op_len) == 0)
				data->blocksize = atoi(position + 1);
			else if (strncmp(av[ac], "count", op_len) == 0)
				data->count = atoi(position + 1);
			else if (strncmp(av[ac], "sts", op_len) == 0)
				data->startsector= atoi(position + 1);
			else if (strncmp(av[ac], "pause", op_len) == 0)
				data->pause = atoi(position + 1);
			else if (strncmp(av[ac], "verbose", op_len) == 0)
				data->verbose = atoi(position + 1);
			else if (strncmp(av[ac], "sync", op_len) == 0 && 0 == atoi(position + 1))
				sync = 0;
			else
				usage(av[0]);
		}
		else
			usage(av[0]);
	}
	if (!data->blocksize || !flag || !data->count || !data->device || data->blocksize < 0 || data->count < 0 ||
		data->startsector < 0 || data->pause < 0)
		usage(av[0]);
	if (sync)
		data->sync = 1;
}

int fstat64_blk(int fd, struct stat *st)
{
	if (fstat(fd, st) == -1)
		return (-1);

	if (S_ISBLK(st->st_mode)) {
		/* get the size of a block device */
		if (ioctl(fd, BLKGETSIZE64, &st->st_size) != 0)
			return (-1);
		/* Get block device sector size.  */
		if (ioctl(fd, BLKSSZGET, &st->st_blksize) != 0)
			return (-1);
	}

	return (0);
}

int process_device(t_data *data, int fd, struct stat *statbuf)
{
	ssize_t rw_size;
	ssize_t buf_size;
	void *buf;
	unsigned int i = 0;

	buf_size = data->blocksize * 1024;
	if (statbuf->st_size < (buf_size * data->count + data->startsector * statbuf->st_blksize))
		return my_exit("Operating area exceeds device size", data, -1, fd);

	buf = (void *)malloc(buf_size);
	if (!buf)
		return my_exit(strerror(errno), data, errno, fd);

	lseek(fd, statbuf->st_blksize * data->startsector, SEEK_SET);

	if (data->verbose)
		printf("Processed data block equals %ld logical units\n", buf_size/statbuf->st_blksize);

	while(i < data->count)
	{

		if(data->operation == O_RDONLY)
		{
			if (data->verbose)
				printf("Reading sector %u\n", data->startsector + i);
			rw_size = read(fd, buf, buf_size);
		}

		else if(data->operation == O_WRONLY)
		{
			if (data->verbose)
				printf("Writing to sector %u\n", data->startsector + i);
			memset(buf, 'a', buf_size);
			rw_size = write(fd, buf, buf_size);
		}

		if (data->sync)
			fsync(fd);

		if (rw_size != buf_size)
		{
			free(buf);
			printf("rw_size - %ld; buf_size - %ld\n", rw_size, buf_size);
			return my_exit("processed data differs from buffer size: ", data, -1, fd);
		}

		i++;
		if (i != data->count)
			sleep(data->pause);
	}
	return 0;
}

int main(int ac, char **av)
{
	t_data data = {0};
	int fd;

	parse_args(ac, av, &data);
	printf("PID : %d\n", getpid());
	fd = open(data.device, data.operation);
	if (fd < 0)
		return (my_exit(strerror(errno), &data, errno, 0));

	struct stat statbuf;
	if (fstat64_blk( fd, &statbuf) != 0) {
		return (my_exit( strerror(errno), &data, -errno, fd ));
	}
	process_device(&data, fd, &statbuf);
	close(fd);
	return 0;
}
