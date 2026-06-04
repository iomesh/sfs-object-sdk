/*
 * SFS 客户端示例：获取指定路径的文件/目录属性。
 *
 * 编译:
 *   make -C examples
 *
 * 运行:
 *   ./examples/getattr_example <ns_name> [path]
 *
 * path 为相对命名空间根目录的路径，省略或 "." 表示根目录。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ns_common.h"
#include "sfs_client.h"

#define ATTR_FMT_BUF 512
#define INO_FMT_BUF  128

#define CHECK(call, msg)                                                       \
    do {                                                                       \
        int32_t _rc = (call);                                                  \
        if (_rc != 0) {                                                        \
            fprintf(stderr, "%s: error %d\n", (msg), _rc);                     \
            goto cleanup;                                                      \
        }                                                                      \
    } while (0)

static const char *ftype_name(uint32_t ftype)
{
    switch (ftype) {
    case TypeFile:
        return "file";
    case TypeDir:
        return "dir";
    case TypeSymlink:
        return "symlink";
    case TypeSock:
        return "sock";
    case TypeFifo:
        return "fifo";
    case TypeChr:
        return "chr";
    case TypeBlk:
        return "blk";
    default:
        return "unknown";
    }
}

static void print_timespec(const char *label, const struct TimeSpec *ts)
{
    char buf[64];
    struct tm tm_buf;
    time_t sec = (time_t)ts->tv_sec;

    if (localtime_r(&sec, &tm_buf) != NULL) {
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
        printf("  %-8s %s.%09ld\n", label, buf, (long)ts->tv_nsec);
    } else {
        printf("  %-8s %lld.%09ld\n", label, (long long)ts->tv_sec, (long)ts->tv_nsec);
    }
}

static void print_attr_detail(const char *path, const struct Attr *attr)
{
    char attr_buf[ATTR_FMT_BUF];
    char ino_buf[INO_FMT_BUF];
    struct GIno ino = attr2ino(attr);
    uint64_t change = attr2change(attr);

    if (attr_format(attr, attr_buf, (int32_t)sizeof(attr_buf)) != NULL) {
        printf("attr_format: %s\n", attr_buf);
    }
    if (ino_format(&ino, ino_buf, (int32_t)sizeof(ino_buf)) != NULL) {
        printf("ino_format:  %s\n", ino_buf);
    }

    printf("\nPath:        %s\n", path);
    printf("  type        %s (%u)\n", ftype_name(attr->ftype), attr->ftype);
    printf("  version     %u\n", attr->version);
    printf("  shard_id    %u\n", attr->ishard_id);
    printf("  ino         %llu\n", (unsigned long long)attr->ino);
    printf("  igen        %llu\n", (unsigned long long)attr->igen);
    printf("  gen         %u:%u\n", attr->gen_hi, attr->gen_lo);
    printf("  mode        0%o\n", attr->mode & 07777);
    printf("  nlink       %u\n", attr->nlink);
    printf("  uid         %u\n", attr->uid);
    printf("  gid         %u\n", attr->gid);
    printf("  size        %llu\n", (unsigned long long)attr->size);
    printf("  blksize     %u\n", attr->blksize);
    printf("  blocks      %llu\n", (unsigned long long)attr->blocks);
    printf("  change      %llu\n", (unsigned long long)change);

    print_timespec("atime", &attr->atime);
    print_timespec("mtime", &attr->mtime);
    print_timespec("ctime", &attr->ctime);
    print_timespec("btime", &attr->btime);
}

static int32_t resolve_path_ino(const struct NameSpace *ns,
                                const char *path,
                                struct GIno *ino_out,
                                const struct Cred *cred)
{
    struct GIno cur = ns->root;
    char *path_copy = NULL;
    char *saveptr = NULL;
    char *component = NULL;

    if (path == NULL || path[0] == '\0' || strcmp(path, ".") == 0) {
        *ino_out = cur;
        return 0;
    }

    if (path[0] == '/') {
        fprintf(stderr, "Path must be relative to namespace root\n");
        return InvalidArgument;
    }

    path_copy = strdup(path);
    if (path_copy == NULL) {
        return IoErr;
    }

    component = strtok_r(path_copy, "/", &saveptr);
    while (component != NULL) {
        struct Attr attr = {0};
        int32_t rc;

        if (component[0] == '\0') {
            component = strtok_r(NULL, "/", &saveptr);
            continue;
        }

        rc = sfs_lookup(ns, &cur, component, cred, &attr);
        if (rc != 0) {
            fprintf(stderr, "sfs_lookup '%s': error %d\n", component, rc);
            free(path_copy);
            return rc;
        }
        cur = attr2ino(&attr);
        component = strtok_r(NULL, "/", &saveptr);
    }

    free(path_copy);
    *ino_out = cur;
    return 0;
}

int main(int argc, char *argv[])
{
    int ret = EXIT_FAILURE;
    struct NameSpace ns = {0};
    const char *ns_name = NULL;
    struct Cred cred = {.uid = 0, .gid = 0};
    struct GIno ino = {0};
    struct Attr attr = {0};
    const char *path = ".";

    CHECK(sfs_example_parse_ns_name(argc,
                                    argv,
                                    "<ns_name> [path]",
                                    &ns_name),
          "sfs_example_parse_ns_name");
    if (argc >= 3) {
        path = argv[2];
    }

    CHECK(sfs_export_ns(ns_name, &ns), "sfs_export_ns");
    CHECK(resolve_path_ino(&ns, path, &ino, &cred), "resolve_path_ino");
    CHECK(sfs_getattr(&ns, &ino, &cred, &attr, NULL), "sfs_getattr");

    print_attr_detail(path, &attr);

    ret = EXIT_SUCCESS;

cleanup:
    if (ns.nsid != 0) {
        sfs_unexport_ns(&ns);
    }
    return ret;
}
