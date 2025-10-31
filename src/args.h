#ifndef ARGS_H
#define ARGS_H

typedef struct
{
    unsigned int width;
    unsigned int height;
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

#endif
