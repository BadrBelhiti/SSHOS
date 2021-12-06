	#
	# user-side system calls
	#
	# System calls use a special convention:
        #     %eax  -  system call number
        #
        #

	# void exit(int status)
	.global exit
exit:
	mov $0,%eax
	int $48
	ret

	# ssize_t write(int fd, void* buf, size_t nbyte)
	.global write
write:
	mov $1,%eax
	int $48
	ret

	# int fork()
	.global fork
fork:
	push %ebx
	push %esi
	push %edi
	push %ebp
	mov $2,%eax
	int $48
	pop %ebp
	pop %edi
	pop %esi
	pop %ebx
	ret

	# int sem(uint32_t init)
	.global sem
sem:
	mov $3,%eax
	int $48
	ret

	# int up(int s)
	.global up
up:
	mov $4,%eax
	int $48
	ret

	# int down(int s)
	.global down
down:
	mov $5,%eax
	int $48
	ret

	# int close(int id)
	.global close
close:
	mov $6,%eax
	int $48
	ret

	# int shutdown(void)
	.global shutdown
shutdown:
	mov $7,%eax
	int $48
	ret

	# int wait(int id, uint32_t *ptr)
	.global wait
wait:
	mov $8,%eax
	int $48
	ret

	# int execl(const char* path, const char* arg0, ....);
	.global execl
execl:
	mov $9,%eax
	int $48
	ret

	# int open(const char* fn)
	.global open
open:
	mov $10,%eax
	int $48
	ret


	# ssize_t len(int fd)
	.global len
len:
	mov $11,%eax
	int $48
	ret

	# ssize_t read(int fd, void* buffer, size_t n)
	.global read
read:
	mov $12,%eax
	int $48
	ret

	# off_t seek(int fd, off_t off)
	.global seek
seek:
	mov $13,%eax
	int $48
	ret

	# int shell_theme(int theme)
	.global shell_theme
shell_theme:
	mov $14,%eax
	int $48
	ret

	# int opendir(const char* fn)
	.global opendir
opendir:
	mov $15,%eax
	int $48
	ret

	# int readdir(int fd, char* buff_start, uint32_t max_size)
	.global readdir
readdir:
	mov $16,%eax
	int $48
	ret

	# int chdir(const char* fn)
	.global chdir
chdir:
	mov $17,%eax
	int $48
	ret

	# int touch(const char* fn)
	.global touch
touch:
	mov $18,%eax
	int $48
	ret

	# int readShellLine(char* buffer)
	.global readShellLine
readShellLine:
	mov $19,%eax
	int $48
	ret

	# int removeStructure(int fd)
	.global removeStructure
removeStructure:
	mov $20,%eax
	int $48
	ret

	# int getcwd(char* buff)
	.global getcwd
getcwd:
	mov $21,%eax
	int $48
	ret

	# int getcmd(char* buff)
	.global getcmd
getcmd:
	mov $22,%eax
	int $48
	ret

	# int copy(char* from, char* to)
	.global copy
copy:
	mov $23,%eax
	int $48
	ret

	.global mkdir
mkdir:
	mov $24,%eax
	int $48
	ret
