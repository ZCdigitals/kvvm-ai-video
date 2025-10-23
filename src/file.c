#include <stdio.h>
#include "file.h"

int save_data(const char *path, void *data, size_t size)
{
    FILE *fp = fopen(path, "wb");
    if (!fp)
    {
        perror("无法创建文件");
        return -1;
    }

    if (fwrite(data, 1, size, fp) != size)
    {
        perror("写入文件失败");
        fclose(fp);
        return -1;
    }

    fclose(fp);
    printf("图像已保存到: %s (大小: %zu 字节)\n", path, size);
    return 0;
}

int save_data_nv12(const char *path, void *data_y, size_t size_y, void *data_uv, size_t size_uv)
{
    FILE *fp = fopen(path, "wb");
    if (!fp)
    {
        perror("无法创建文件");
        return -1;
    }

    fwrite(data_y, 1, size_y, fp);
    fwrite(data_uv, 1, size_uv, fp);

    fclose(fp);
    printf("图像已保存到: %s\n", path);
    return 0;
}
