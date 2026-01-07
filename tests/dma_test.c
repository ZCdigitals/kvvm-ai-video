#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include <fcntl.h>
#include <unistd.h>

#include "rk_common.h"
#include "rk_mpi_cal.h"
#include "rk_mpi_mb.h"
#include "rk_mpi_mmz.h"
#include "rk_mpi_sys.h"
#include "rk_mpi_venc.h"
#include "rk_type.h"

#define BUFFER_COUNT 4
#define PLANE_LENGTH 1920 * 1088 * 3 / 2

uint32_t init_memory(uint32_t buffer_count)
{
    // init memory pool
    MB_POOL_CONFIG_S memory_pool_config;
    memset(&memory_pool_config, 0, sizeof(MB_POOL_CONFIG_S));
    memory_pool_config.u64MBSize = PLANE_LENGTH;
    memory_pool_config.u32MBCnt = buffer_count;
    memory_pool_config.enAllocType = MB_ALLOC_TYPE_DMA;
    memory_pool_config.bPreAlloc = RK_TRUE;

    uint32_t memory_pool_id = RK_MPI_MB_CreatePool(&memory_pool_config);
    if (memory_pool_id == MB_INVALID_POOLID)
    {
        errno = -1;
        perror("mpi memory pool create error");
        return MB_INVALID_POOLID;
    }
    printf("mpi memory pool %d %d create ok\n", memory_pool_id, buffer_count);

    return memory_pool_id;
}

int destroy_memory(uint32_t memory_pool_id)
{
    int32_t ret = RK_SUCCESS;

    // avoid invalid pool
    if (memory_pool_id != MB_INVALID_POOLID)
    {
        ret = RK_MPI_MB_DestroyPool(memory_pool_id);
        if (ret != RK_SUCCESS)
        {
            errno = ret;
            perror("mpi memory pool destory error");
        }
        printf("mpi memory pool %d destory ok\n", memory_pool_id);
    }
}

int allocate_buffers(uint32_t memory_pool_id, unsigned int buffer_count, void *blocks[])
{
    for (unsigned int i = 0; i < buffer_count; i += 1)
    {
        // get mpi block
        blocks[i] = RK_MPI_MB_GetMB(memory_pool_id, PLANE_LENGTH, RK_TRUE);
        if (blocks[i] == RK_NULL)
        {
            errno = -1;
            perror("mpi memory get block error");
            return -1;
        }
        printf("mpi memory get block %d ok\n", i);
    }

    return 0;
}

int free_buffers(unsigned int buffer_count, void *blocks[])
{
    for (unsigned int i = 0; i < buffer_count; i += 1)
    {
        int32_t ret = RK_MPI_MB_ReleaseMB(blocks[i]);
        if (ret != RK_SUCCESS)
        {
            errno = ret;
            perror("mpi memory release block error");
            continue;
        }
        printf("mpi memory release block ok\n");
    }

    return 0;
}

int main()
{
    uint32_t memory_pool_id = init_memory(BUFFER_COUNT);

    void *blocks[BUFFER_COUNT];
    allocate_buffers(memory_pool_id, BUFFER_COUNT, blocks);
    free_buffers(BUFFER_COUNT, blocks);

    destroy_memory(memory_pool_id);
}
