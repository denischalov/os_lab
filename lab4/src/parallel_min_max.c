#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

#define MAX_P 64

static pid_t children[MAX_P];
static int nchildren = 0;

static void alrm_handler(int sig)
{
    (void)sig;
    for (int i = 0; i < nchildren; i++) {
        if (children[i] > 0) {
            kill(children[i], SIGKILL);
        }
    }
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = -1;
  bool with_files = false;

  static struct option options[] = {{"seed", required_argument, 0, 's'},
                                      {"array_size", required_argument, 0, 'n'},
                                      {"pnum", required_argument, 0, 'p'},
                                      {"timeout", optional_argument, 0, 't'},
                                      {"by_files", no_argument, 0, 'f'},
                                      {0, 0, 0, 0}};

  int c;
  while ((c = getopt_long(argc, argv, "s:n:p:t::f", options, NULL)) != -1) {
      switch (c) {
        case 's':
          seed = atoi(optarg);
          if (seed <= 0) {
              fprintf(stderr, "--seed must be a positive number\n");
              return EXIT_FAILURE;
          }
          break;

        case 'n':
          array_size = atoi(optarg);
          if (array_size <= 0) {
              fprintf(stderr, "--array_size must be a positive number\n");
              return EXIT_FAILURE;
          }
          break;

        case 'p':
          pnum = atoi(optarg);
          if (pnum <= 0 || pnum > MAX_P) {
              fprintf(stderr, "--pnum must be a positive number no more than %d\n", MAX_P);
              return EXIT_FAILURE;
          }
          break;

        case 't':
          if (optarg) {
            timeout = atoi(optarg);
            if (timeout < 0) {
                fprintf(stderr, "--timeout must be a positive number\n");
                return EXIT_FAILURE;
            }
          }
          break;

        case 'f':
          with_files = true;
          break;

        case '?':
          return EXIT_FAILURE;

        default:
          fprintf(stderr, "getopt_long returned unexpected code 0%o\n", c);
          return EXIT_FAILURE;
      }
  }

  if (optind < argc) {
      fprintf(stderr, "Unexpected positional argument: %s\n", argv[optind]);
      return EXIT_FAILURE;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
      fprintf(stderr,
              "Usage: %s --seed NUM --array_size NUM --pnum NUM "
              "[--timeout=NUM] [--by_files]\n",
              argv[0]);
      return EXIT_FAILURE;
  }

  int *array = malloc(sizeof(int) * array_size);
  if (!array) {
    perror("malloc");
    return EXIT_FAILURE;
  }

  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  mkdir("tmp", 0777);

  int mypipe[2];
  if (pipe (mypipe)) {
    perror("pipe");
    return EXIT_FAILURE;
  }

  if (signal(SIGALRM, alrm_handler) == SIG_ERR) {
    perror("signal");
    return 1;
  }

  for (int i = 0; i < pnum; i++) {
    pid_t pid = fork();
    if (pid >= 0) {
      if (pid == 0) {
        close(mypipe[0]);

        int begin = (array_size / pnum) * i;
        int end = (i == pnum - 1) ? array_size : (array_size / pnum) * (i + 1);

        struct MinMax min_max = GetMinMax(array, begin, end);

        if (with_files) {
          FILE *file;
          char file_name[50];  
          sprintf(file_name, "tmp/file%d", i);

          file = fopen(file_name, "wb");
          if (!file) {
            perror("fopen");
            _exit(EXIT_FAILURE);
          }

          if (fwrite(&min_max.min, sizeof(min_max.min), 1, file) != 1 || fwrite(&min_max.max, sizeof(min_max.max), 1, file) != 1) {
            perror("fwrite");
            fclose(file);
            _exit(EXIT_FAILURE);
          }
          fclose(file);
        } else {
          if (write(mypipe[1], &min_max.min, sizeof(min_max.min)) != sizeof(min_max.min) || write(mypipe[1], &min_max.max, sizeof(min_max.max)) != sizeof(min_max.max)) {
            perror("write");
            close(mypipe[1]);
            _exit(EXIT_FAILURE);
          }
          close(mypipe[1]);
        }

        _exit(EXIT_SUCCESS);
      }

      children[nchildren++] = pid;
    } else {
      perror("fork");
      for (int j = 0; j < nchildren; j++) {
        kill(children[j], SIGKILL);
      }
      while (wait(NULL) > 0);
      close(mypipe[0]);
      close(mypipe[1]);
      return EXIT_FAILURE;
    }
  }

  close(mypipe[1]);

  ualarm(timeout * 1000, 0);

  bool killed[MAX_P] = { false };

  int reaped = 0;
  while (reaped < pnum) {
    int status;
    pid_t r = waitpid(-1, &status, WNOHANG);

    if (r > 0) {
      reaped++;
      for (int i = 0; i < nchildren; i++) {
        if (children[i] == r) {
          if (WIFSIGNALED(status) || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            killed[i] = true;
          }
          children[i] = 0;
          break;
        }
      }
    } else if (r == 0) {
      usleep(1000); 

    } else {
      if (errno == EINTR) continue;
      if (errno == ECHILD) break;
      perror("waitpid");
      break;
    }
  }

  ualarm(0, 0);

  bool any_killed = false;
  for (int i = 0; i < pnum; i++) {
      if (killed[i]) { 
        any_killed = true;
        break;
      }
  }

  if (any_killed) {
    fprintf(stderr, "Timeout: some children were killed by SIGKILL\n");
    close(mypipe[0]);
    for (int i = 0; i < pnum; i++) {
      char file_name[50];
      snprintf(file_name, sizeof(file_name), "tmp/file%d", i);
      remove(file_name);
    }
    rmdir("tmp");
    free(array);
    return EXIT_FAILURE;
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      FILE *file;
      char file_name[50];  
      sprintf(file_name, "tmp/file%d", i); 

      file = fopen(file_name, "rb");
      if (!file) {
        perror("fopen");
        return EXIT_FAILURE;
      }

      if (fread(&min, sizeof(min), 1, file) != 1 || fread(&max, sizeof(max), 1, file) != 1) {
        perror("fread");
        fclose(file);
        return EXIT_FAILURE;
      }
      fclose(file);
      remove(file_name);
    } else {
      if (read(mypipe[0], &min, sizeof(min)) != sizeof(min) || read(mypipe[0], &max, sizeof(max)) != sizeof(max)) {
        perror("read");
        return EXIT_FAILURE;
      }
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  close(mypipe[0]);

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  rmdir("tmp");

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}
