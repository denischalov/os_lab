#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <inttypes.h>
#include <getopt.h>
#include <sys/time.h>

#include <pthread.h>

#include "utils.h"

void *ThreadSum(void *args) {
  struct SumArgs *sum_args = (struct SumArgs *)args;
  return (void *)(intptr_t)Sum(sum_args);
}

int main(int argc, char **argv) {
  uint32_t threads_num = 0;
  uint32_t array_size = 0;
  uint32_t seed = 0;

  static struct option options[] = {{"threads_num", required_argument, 0, 'p'},
                                    {"array_size", required_argument, 0, 'n'},
                                    {"seed", required_argument, 0, 's'},
                                    {0, 0, 0, 0}};

  int c, tmp;
  while ((c = getopt_long(argc, argv, "p:n:s:", options, NULL)) != -1) {
      switch (c) {
        case 's':
          tmp = atoi(optarg);
          if (tmp <= 0) {
              fprintf(stderr, "--seed must be a positive number\n");
              return EXIT_FAILURE;
          }
          seed = (uint32_t)tmp;
          break;

        case 'n':
          tmp = atoi(optarg);
          if (tmp <= 0) {
              fprintf(stderr, "--array_size must be a positive number\n");
              return EXIT_FAILURE;
          }
          array_size = (uint32_t)tmp;
          break;

        case 'p':
          tmp = atoi(optarg);
          if (tmp <= 0) {
              fprintf(stderr, "--threads_num must be a positive number\n");
              return EXIT_FAILURE;
          }
          threads_num = (uint32_t)tmp;
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

  if (seed == 0 || array_size == 0 || threads_num == 0) {
      fprintf(stderr, "Usage: %s --threads_num NUM --seed NUM --array_size NUM\n", argv[0]);
      return EXIT_FAILURE;
  }

  pthread_t threads[threads_num];

  int *array = malloc(sizeof(int) * array_size);

  GenerateArray(array, array_size, seed);

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  struct SumArgs args[threads_num];
  for (uint32_t i = 0; i < threads_num; i++) {
    args[i].array = array;
    args[i].begin = (int)((uint64_t)i * array_size / threads_num);
    args[i].end = (int)((uint64_t)(i + 1) * array_size / threads_num);

    if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i])) {
      printf("Error: pthread_create failed!\n");
      return 1;
    }
  }

  long int total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
    void *retval = NULL;
  pthread_join(threads[i], &retval);
  total_sum += (long int)(intptr_t)retval;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  printf("Total: %ld\n", total_sum);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);

  return 0;
}
