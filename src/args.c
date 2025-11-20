#include "args.h"

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "version.h"

#define DEFAULT_BIT_RATE 10 * 1024
#define DEFAULT_GOP 60

void print_help()
{
    printf("Usage: -w <width> -h <height> -i <input path> -o <output path> -b <bit_rate> -g <gop>\n");
    printf("Required args:\n");
    printf("    -w <pixel>   Must be greater than 0\n");
    printf("    -h <pixel>   Must be greater than 0\n");
    printf("    -i <path>    Input device path, eg. `/dev/video0`\n");
    printf("    -o <path>    Output socket path, eg. `/var/run/capture.sock`\n");
    printf("    -b <number>  Encoder bit rate, default is `10240`\n");
    printf("    -g <number>  Encode group of pictures, default is `60`\n");
}

void print_version()
{
    printf("Version: %s\n", VERSION);
    printf("Git commit: %s\n", COMMIT);
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

    // init values
    memset(args, 0, sizeof(args_t));

    // set default values
    args->bit_rate = DEFAULT_BIT_RATE;
    args->gop = DEFAULT_GOP;

    // 使用getopt_long支持长选项
    static struct option long_options[] = {
        {"width", required_argument, 0, 'w'},
        {"height", required_argument, 0, 'h'},
        {"input", required_argument, 0, 'i'},
        {"output", required_argument, 0, 'o'},
        {"bit-rate", required_argument, 0, 'b'},
        {"gop", required_argument, 0, 'g'},
        {"help", no_argument, 0, 0},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}};

    while ((opt = getopt_long(argc, argv, "w:h:i:o:b:g:", long_options, NULL)) != -1)
    {
        switch (opt)
        {
        case 'w':
            args->width = (uint32_t)strtoul(optarg, NULL, 10);
            break;
        case 'h':
            args->height = (uint32_t)strtoul(optarg, NULL, 10);
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
        case 'b':
            args->bit_rate = (uint32_t)strtoul(optarg, NULL, 10);
            break;
        case 'g':
            args->gop = (uint32_t)strtoul(optarg, NULL, 10);
            break;
        case 0:
            print_help();
            args->help_flag = true;
            break;
        case 'v':
            print_version();
            args->version_flag = true;
            break;
        default:
            break;
        }
    }

    // valid args
    if (args->help_flag || args->version_flag)
    {
        return 0;
    }
    else if (args->width == 0 || args->width > 8192)
    {
        printf("width must be in (0,8192], get %d\n", args->width);
        return -1;
    }
    else if (args->height == 0 || args->height > 8192)
    {
        printf("height must be in (0,8192], get %d\n", args->height);
        return -1;
    }
    else if (args->input_path == NULL)
    {
        printf("input path is required\n");
        return -1;
    }
    else if (args->output_path == NULL)
    {
        printf("output path is required\n");
        return -1;
    }
    else if (args->bit_rate == 0)
    {
        printf("bit rate must be greater than 0\n");
        return -1;
    }
    else if (args->gop == 0)
    {
        printf("gop must be greater than 0\n");
        return -1;
    }

    return 0;
}

void destroy_args(args_t *args)
{
    if (args->input_path != NULL)
    {
        free(args->input_path);
    }
    if (args->output_path != NULL)
    {
        free(args->output_path);
    }
}
