#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint32_t width;
    uint32_t height;
    char *input_path;
    char *output_path;
    bool help_flag;
    bool version_flag;
} args_t;

/**
 * parse args
 *
 * @param argc argc
 * @param argv argv
 * @param args args pointer
 * @return 0 ok, -1 error
 */
int parse_args(int argc, char *argv[], args_t *args);

/**
 * destroy args
 *
 * @param args args pointer
 */
void destroy_args(args_t *args);

#endif
