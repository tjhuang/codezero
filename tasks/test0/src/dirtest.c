
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/types.h>
#include <linux/dirent.h>
#include <linux/unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <l4lib/os/posix/readdir.h>
#include <tests.h>

#define DENTS_TOTAL	50

void print_fsize(struct stat *s)
{
	printf("%d", s->st_size);
}

void print_flink(struct stat *s)
{
	printf("%d", s->st_nlink);
}

void print_fuser(struct stat *s)
{
	printf("%d", s->st_uid);
	printf("%c", ' ');
	printf("%c", ' ');
	printf("%d", s->st_gid);
}

void print_ftype(struct stat *s)
{
	unsigned int type = s->st_mode & S_IFMT;

	if (type == S_IFDIR)
		printf("%c", 'd');
	else if (type == S_IFSOCK)
		printf("%c", 's');
	else if (type == S_IFCHR)
		printf("%c", 'c');
	else if (type == S_IFLNK)
		printf("%c", 'l');
	else if (type == S_IFREG)
		printf("%c", '-');
}

void print_fperm(struct stat *s)
{
	if (s->st_mode & S_IRUSR)
		printf("%c", 'r');
	else
		printf("%c", '-');
	if (s->st_mode & S_IWUSR)
		printf("%c", 'w');
	else
		printf("%c", '-');
	if (s->st_mode & S_IXUSR)
		printf("%c", 'x');
	else
		printf("%c", '-');
}

void print_fstat(struct stat *s)
{
	print_ftype(s);
	print_fperm(s);
	printf("%c", ' ');
	printf("%c", ' ');
	print_fsize(s);
	printf("%c", ' ');
}

void print_dirents(char *path, void *buf, int cnt)
{
	int i = 0;
	struct dirent *dp = buf;
	// struct stat statbuf;
	char pathbuf[256];

	strncpy(pathbuf, path, 256);
	while (cnt > 0) {
		strcpy(pathbuf, path);
		strcpy(&pathbuf[strlen(pathbuf)],"/");
		strcpy(&pathbuf[strlen(pathbuf)],dp->d_name);
		//printf("Dirent %d:\n", i);
		//printf("Inode: %d\n", dp->d_ino);
		//printf("Offset: %d\n", dp->d_off);
		//printf("Reclen: %d\n", dp->d_reclen);
		//if (stat(pathbuf, &statbuf) < 0)
		//	perror("STAT");
		// print_fstat(&statbuf);
		printf("%s\n", dp->d_name);
		cnt -= dp->d_reclen;
		dp = (struct dirent *)((void *)dp + dp->d_reclen);
		i++;
	}
}

int lsdir(char *path)
{
	struct dirent dents[DENTS_TOTAL];
	int bytes;
	int fd;

	memset(dents, 0, sizeof(struct dirent) * DENTS_TOTAL);

	if ((fd = open(path, O_RDONLY)) < 0) {
		printf("OPEN failed.");
		return 0;
	} else
		printf("Got fd: %d for opening %s\n", fd, path);

	if ((bytes = os_readdir(fd, dents, sizeof(struct dirent) * DENTS_TOTAL)) < 0) {
		perror("GETDENTS\n");
		return 0;
	} else {
		print_dirents(path, dents, bytes);
	}

	return 0;
}


int dirtest(void)
{
	printf("\nlsdir current directory:\n");
	lsdir(".");
	printf("\nlsdir root directory:\n");
	lsdir("/");

	printf("\nCreating directories: usr, etc, tmp, var, home, opt, bin, boot, lib, dev\n");
	if (mkdir("/usr", 0) < 0)
		perror("MKDIR");
	if (mkdir("/etc", 0) < 0)
		perror("MKDIR");
	if (mkdir("/tmp", 0) < 0)
		perror("MKDIR");
	if (mkdir("/var", 0) < 0)
		perror("MKDIR");
	if (mkdir("/bin", 0) < 0)
		perror("MKDIR");
	if (mkdir("/boot", 0) < 0)
		perror("MKDIR");
	if (mkdir("/lib", 0) < 0)
		perror("MKDIR");
	if (mkdir("/dev", 0) < 0)
		perror("MKDIR");

	if (mkdir("/usr/bin", 0) < 0)
		perror("MKDIR");
	if (mkdir("/home/", 0) < 0)
		perror("MKDIR");
	if (mkdir("/home/bahadir", 0) < 0)
		perror("MKDIR");
	if (chdir("/home/bahadir") < 0)
		perror("CHDIR");
	printf("Changed curdir to /home/bahadir\n");

	printf("\nlsdir root directory:\n");
	lsdir("/");

	printf("\nlsdir /usr:\n");
	lsdir("/usr");

	printf("\nlsdir current directory:\n");
	lsdir(".");
	printf("\nlsdir /usr/./././bin//\n");
	lsdir("/usr/./././bin//");
	return 0;
}

