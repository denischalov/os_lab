#include <ctype.h>
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

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
              fprintf(stderr, "seed is a positive number");
              return EXIT_FAILURE;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              fprintf(stderr, "array_size is a positive number");
              return EXIT_FAILURE;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
              fprintf(stderr, "pnum is a positive number");
              return EXIT_FAILURE;
            }
            break;
          case 3:
            with_files = true;
            break;

          defalut:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    fprintf(stderr, "Has at least one no option argument\n");
    return EXIT_FAILURE;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    fprintf(stderr, "Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" \n",
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

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      active_child_processes += 1;

      if (child_pid == 0) {
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
    } else {
      perror("fork");
      close(mypipe[0]);
      close(mypipe[1]);
      return EXIT_FAILURE;
    }
  }

  close(mypipe[1]);

  while (active_child_processes > 0) {
    wait(NULL);
    active_child_processes -= 1;
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
