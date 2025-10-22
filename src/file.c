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
