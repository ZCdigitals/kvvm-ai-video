#include "args.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_help()
{
    printf("Usage: -w <width> -h <height>\n");
}

int parse_args(int argc, char *argv[], args_t *args)
{
    int opt;

    // 使用getopt_long支持长选项
    static struct option long_options[] = {
        {"width", required_argument, 0, 'w'},
        {"height", required_argument, 0, 'h'},
        {"help", no_argument, 0, 0},
        {0, 0, 0, 0}};

    while ((opt = getopt_long(argc, argv, "w:h:i:o:", long_options, NULL)) != -1)
    {
        switch (opt)
        {
        case 'w':
            args->width = (unsigned int)strtoul(optarg, NULL, 10);
            break;
        case 'h':
            args->height = (unsigned int)strtoul(optarg, NULL, 10);
            break;
        case 0:
            print_help();
            break;
        default:
            return -1;
        }
    }

    return 0;
}
