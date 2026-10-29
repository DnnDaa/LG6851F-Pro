/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

static void fail(int fd)
{
 int error = errno ? errno : EIO;
 ssize_t n;
 do { n = write(fd, &error, sizeof(error)); } while (n < 0 && errno == EINTR);
 _exit(127);
}

int main(int argc, char **argv)
{
 int pipefd[2], status, error = 0;
 pid_t pid, waited;
 ssize_t bytes;
 if (argc < 2 || argv[1][0] != '/') return 2;
 if (pipe2(pipefd, O_CLOEXEC)) return 1;
 pid = fork();
 if (pid < 0) {close(pipefd[0]); close(pipefd[1]); return 1;}
 if (!pid) {
  DIR *fds;
  struct dirent *entry;
  close(pipefd[0]);
  if (setsid() < 0) fail(pipefd[1]);
  pid = fork();
  if (pid < 0) fail(pipefd[1]);
  if (pid) _exit(0);
  /* Do not keep a caller's configuration or driver flock alive after exec. */
  fds = opendir("/proc/self/fd");
  if (!fds) fail(pipefd[1]);
  while ((entry = readdir(fds))) {
   char *end;
   long fd = strtol(entry->d_name, &end, 10);
   if (*entry->d_name && !*end && fd >= 3 && fd <= INT_MAX &&
       fd != pipefd[1] && fd != dirfd(fds)) close((int)fd);
  }
  closedir(fds);
  execv(argv[1], &argv[1]);
  fail(pipefd[1]);
 }
 close(pipefd[1]);
 do {waited = waitpid(pid, &status, 0);} while (waited < 0 && errno == EINTR);
 do {bytes = read(pipefd[0], &error, sizeof(error));} while (bytes < 0 && errno == EINTR);
 close(pipefd[0]);
 if (waited < 0 || bytes < 0 || (bytes > 0 && bytes != (ssize_t)sizeof(error))) return 1;
 if (bytes == (ssize_t)sizeof(error)) {fprintf(stderr, "detach: %s\n", strerror(error)); return 1;}
 return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : 1;
}
