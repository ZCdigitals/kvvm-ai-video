#include "args.h"

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "version.h"

void print_help()
{
    printf("Usage: -w <width> -h <height> -i <input path> -o <output path>\n");
    printf("Required args:\n");
    printf("    -w <pixel>  Must be greater than 0\n");
    printf("    -h <pixel>  Must be greater than 0\n");
    printf("    -i <path>   Input device path, eg. `/dev/video0`\n");
    printf("    -o <path>   Output socket path, eg. `/var/run/capture.sock`\n");
}

void print_version()
{
    printf("Version: %s\n", VERSION_STRING);
    printf("Build: %s\n", BUILD_TIME);
}

char *safe_strdup(const char *src)
{
    if (src == NULL)
    {
        return NULL;
    }

    size_t len = strlen(src) + 1;
    char *dst = malloc(len);
    if (dst != NULL)
    {
        memcpy(dst, src, len);
    }

    return dst;
}

int parse_args(int argc, char *argv[], args_t *args)
{
    int opt;

    // 使用getopt_long支持长选项
    static struct option long_options[] = {
        {"width", required_argument, 0, 'w'},
        {"height", required_argument, 0, 'h'},
        {"input", required_argument, 0, 'i'},
        {"output", required_argument, 0, 'o'},
        {"help", no_argument, 0, 0},
        {"version", no_argument, 0, -2},
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
        case 'i':
        {
            char *ip = safe_strdup(optarg);
            if (ip == NULL)
            {
                errno = -1;
                perror("dup input error");
                return -1;
            }

            args->input_path = ip;
            break;
        }
        case 'o':
        {
            char *op = safe_strdup(optarg);
            if (op == NULL)
            {
                errno = -1;
                perror("dup output error");
                return -1;
            }

            args->output_path = op;
            break;
        }
        case 0:
            print_help();
            args->help_flag = true;
            break;
        case -2:
            print_version();
            args->version_flag = true;
            break;
        default:
            return -1;
        }
    }

    return 0;
}

void destroy_args(args_t *args)
{
    free(args->input_path);
    free(args->output_path);
}
