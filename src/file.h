#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <stdio.h>

/**
 * @brief 将数据保存到文件
 *
 * @param path 文件路径
 * @param data 要保存的数据指针
 * @param size 数据大小（字节）
 * @return int 成功返回0，失败返回-1
 */
int save_data(const char *path, void *data, size_t size);

int save_data_nv12(const char *path, void *data_y, size_t size_y, void *data_uv, size_t size_uv);

#endif
