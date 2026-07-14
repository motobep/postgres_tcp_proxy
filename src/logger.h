#ifndef LOGGER
#define LOGGER

#include <arpa/inet.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

FILE *logger_init(const char *filename);
void log_query(FILE *logger, const char *query);
int parse_simple_query(const unsigned char *buffer, size_t buffer_size,
                       char *query_out, size_t query_out_size);
int parse_prepared_query(const unsigned char *buffer, size_t buffer_size,
                         char *query_out, size_t query_out_size);
int process_message(const unsigned char *buffer, size_t buffer_size,
                    char *query_out, size_t query_out_size);
#endif


#ifdef LOGGER_IMPLEMENTATION

FILE *logger_init(const char *filename) {
  FILE *fp = (FILE *)malloc(sizeof(FILE));
  if (!fp)
    return NULL;

  fp = fopen(filename, "a");
  if (!fp) {
    perror("Failed to open log file");
    return NULL;
  }
  return fp;
}

int logger_close(FILE *fp) {
  if (fclose(fp) != 0)
  {
      perror("Error closing file");
      return 1;
  }
  return 0;
}

void log_query(FILE *fp, const char *query) {
  time_t now = time(NULL);
  struct tm *timeinfo = localtime(&now);
  char timestamp[32];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

  fprintf(fp, "[%s] Query: %s\n", timestamp, query);
  fflush(fp);
}

int process_message(const unsigned char *buffer, size_t buffer_size,
                    char *query_out, size_t query_out_size) {
  if (buffer_size < 5)
    return -1;

  int result = 0;

  if (buffer[0] == 'Q') {
    printf("Q query\n");
    result = parse_simple_query(buffer, buffer_size, query_out, query_out_size);
  } else if (buffer[0] == 'P') {
    printf("P query\n");
    result =
        parse_prepared_query(buffer, buffer_size, query_out, query_out_size);
  }

  return result;
}

int parse_simple_query(const unsigned char *buffer, size_t buffer_size,
                       char *query_out, size_t query_out_size) {
  uint32_t length = ntohl(*(uint32_t *)(buffer + 1));
  if (length < 5 || length > buffer_size - 1)
    return -1;

  uint32_t data_length = length - 4;
  if (data_length >= query_out_size) {
    data_length = query_out_size - 1;
  }

  memcpy(query_out, buffer + 5, data_length);
  query_out[data_length] = '\0';

  return 1;
}

int parse_prepared_query(const unsigned char *buffer, size_t buffer_size,
                         char *query_out, size_t query_out_size) {
  uint32_t length = ntohl(*(uint32_t *)(buffer + 1));

  if (length < 5 || length > buffer_size - 1)
    return -1;

  const unsigned char *ptr = buffer + 5;
  const unsigned char *end = buffer + 1 + length;

  // Skip name
  ptr += strlen((const char *)ptr) + 1;
  if (ptr >= end)
    return -1;

  // Getting query
  const char *query = (const char *)ptr;
  size_t query_length = strlen(query);

  if (query_length >= query_out_size) {
    query_length = query_out_size - 1;
  }

  memcpy(query_out, query, query_length);
  query_out[query_length] = '\0';

  return 1;
}
#endif
