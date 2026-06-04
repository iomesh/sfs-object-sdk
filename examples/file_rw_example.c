/*
 * SFS 客户端示例：创建文件、读写、同步并清理。
 *
 * 编译（需已安装 libsfs_client）:
 *   make -C examples
 *
 * 运行:
 *   ./examples/file_rw_example <ns_name> [filename]
 *
 * 说明：本 SDK 基于 inode 操作，无 POSIX open/close 接口；
 *       写完后通过 sfs_fsync 刷盘，结束时调用 sfs_unexport_ns 释放命名空间。
 */

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ns_common.h"
#include "sfs_client.h"

#define CHECK(call, msg)                                                       \
    do {                                                                       \
        int32_t _rc = (call);                                                  \
        if (_rc != 0) {                                                        \
            fprintf(stderr, "%s: error %d\n", (msg), _rc);                     \
            goto cleanup;                                                      \
        }                                                                      \
    } while (0)

struct IoWait {
    struct IoCbInfo cb;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int done;
};

static void io_done_callback(struct IoCbInfo *cb)
{
    struct IoWait *wait = (struct IoWait *)((char *)cb - offsetof(struct IoWait, cb));

    pthread_mutex_lock(&wait->mutex);
    wait->done = 1;
    pthread_cond_signal(&wait->cond);
    pthread_mutex_unlock(&wait->mutex);
}

static int32_t wait_io(struct IoWait *wait)
{
    pthread_mutex_lock(&wait->mutex);
    while (!wait->done) {
        pthread_cond_wait(&wait->cond, &wait->mutex);
    }
    pthread_mutex_unlock(&wait->mutex);
    return wait->cb.rc;
}

static void io_wait_init(struct IoWait *wait)
{
    memset(wait, 0, sizeof(*wait));
    wait->cb.callback = io_done_callback;
    pthread_mutex_init(&wait->mutex, NULL);
    pthread_cond_init(&wait->cond, NULL);
}

static void io_wait_destroy(struct IoWait *wait)
{
    pthread_mutex_destroy(&wait->mutex);
    pthread_cond_destroy(&wait->cond);
}

static int32_t sfs_write_sync(const struct NameSpace *ns,
                              const struct GIno *ino,
                              uint64_t offset,
                              const void *data,
                              size_t len,
                              const struct Cred *cred)
{
    struct IoWait wait;
    struct SfsIoVec iov = {
        .iov_len = len,
        .iov_base = data,
    };

    io_wait_init(&wait);
    sfs_write(ns, ino, offset, &iov, 1, false, cred, &wait.cb, NULL);
    int32_t rc = wait_io(&wait);
    io_wait_destroy(&wait);
    return rc;
}

static int32_t sfs_read_sync(const struct NameSpace *ns,
                             const struct GIno *ino,
                             uint64_t offset,
                             void *buf,
                             size_t len,
                             const struct Cred *cred)
{
    struct IoWait wait;
    struct SfsIoVec iov = {
        .iov_len = len,
        .iov_base = buf,
    };

    io_wait_init(&wait);
    sfs_read(ns, ino, offset, &iov, 1, cred, &wait.cb, NULL);
    int32_t rc = wait_io(&wait);
    io_wait_destroy(&wait);
    return rc;
}

int main(int argc, char *argv[])
{
    int ret = EXIT_FAILURE;
    struct NameSpace ns = {0};
    const char *ns_name = NULL;
    struct Cred cred = {.uid = 0, .gid = 0};
    struct IAttr iattr = {0};
    struct Attr child_attr = {0};
    struct Attr parent_attr = {0};
    struct AttrGen prev_gen = {0};
    struct GIno file_ino = {0};
    const char *filename = "example.txt";
    const char *write_msg = "Hello from SFS client example!\n";
    char read_buf[256] = {0};
    uint64_t fsync_cookie = 0;

    CHECK(sfs_example_parse_ns_name(argc,
                                    argv,
                                    "<ns_name> [filename]",
                                    &ns_name),
          "sfs_example_parse_ns_name");
    if (argc >= 3) {
        filename = argv[2];
    }

    /* 1. 挂载/导出命名空间 */
    CHECK(sfs_export_ns(ns_name, &ns), "sfs_export_ns");

    /* 2. 在根目录下创建普通文件 */
    iattr.valid = AttrMode;
    iattr.mode = 0644;

    CHECK(sfs_create(&ns,
                     &ns.root,
                     filename,
                     Exclusive,
                     TypeFile,
                     &iattr,
                     &cred,
                     &prev_gen,
                     &parent_attr,
                     &child_attr,
                     NULL),
          "sfs_create");

    file_ino = attr2ino(&child_attr);
    printf("Created file '%s' at offset 0\n", filename);

    /* 3. 写入数据 */
    CHECK(sfs_write_sync(&ns,
                         &file_ino,
                         0,
                         write_msg,
                         strlen(write_msg),
                         &cred),
          "sfs_write");

    printf("Wrote %zu bytes: %s", strlen(write_msg), write_msg);

    /* 4. 读取数据 */
    CHECK(sfs_read_sync(&ns, &file_ino, 0, read_buf, sizeof(read_buf) - 1, &cred),
          "sfs_read");

    printf("Read back: %s", read_buf);

    if (strcmp(write_msg, read_buf) != 0) {
        fprintf(stderr, "Data mismatch after read\n");
        goto cleanup;
    }

    /* 5. 关闭/收尾：刷盘并释放命名空间（SDK 无 fd close，fsync 相当于落盘确认） */
    CHECK(sfs_fsync(&ns, &file_ino, 0, strlen(write_msg), &cred, &fsync_cookie),
          "sfs_fsync");
    printf("File synced (cookie=%llu), closing namespace\n",
           (unsigned long long)fsync_cookie);

    ret = EXIT_SUCCESS;

cleanup:
    if (ns.nsid != 0) {
        sfs_unexport_ns(&ns);
    }
    return ret;
}
