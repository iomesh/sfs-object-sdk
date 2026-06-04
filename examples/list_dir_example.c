/*
 * SFS 客户端示例：列出指定目录下的所有条目。
 *
 * 编译:
 *   make -C examples
 *
 * 运行:
 *   ./examples/list_dir_example <ns_name> [path]
 *
 * path 为相对命名空间根目录的路径，省略或 "." 表示根目录。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ns_common.h"
#include "sfs_client.h"

#define READDIR_BUF_SIZE (256 * 1024)

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

static int32_t resolve_path(const struct NameSpace *ns,
                            const char *path,
                            struct GIno *ino_out,
                            struct Attr *attr_out,
                            const struct Cred *cred)
{
    struct GIno cur = ns->root;
    char *path_copy = NULL;
    char *saveptr = NULL;
    char *component = NULL;

    if (path == NULL || path[0] == '\0' || strcmp(path, ".") == 0) {
        *ino_out = cur;
        return sfs_getattr(ns, &cur, cred, attr_out, NULL);
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
    return sfs_getattr(ns, &cur, cred, attr_out, NULL);
}

static int32_t list_directory(const struct NameSpace *ns,
                              const struct GIno *dir_ino,
                              const struct Cred *cred)
{
    void *buf = NULL;
    uint64_t whence = 0;
    int eof = 0;
    int entry_no = 0;

    buf = malloc(READDIR_BUF_SIZE);
    if (buf == NULL) {
        fprintf(stderr, "malloc failed\n");
        return IoErr;
    }

    printf("%-6s %-10s %-12s %s\n", "TYPE", "SIZE", "WHENCE", "NAME");
    printf("------ ---------- ------------ ----\n");

    do {
        int32_t rc;
        uint8_t *cur;
        uint8_t *end;

        memset(buf, 0, READDIR_BUF_SIZE);
        eof = 0;

        rc = sfs_readdir(ns, dir_ino, whence, cred, buf, READDIR_BUF_SIZE, &eof);
        if (rc != 0) {
            free(buf);
            return rc;
        }

        cur = buf;
        end = cur + READDIR_BUF_SIZE;

        while (cur + sizeof(struct CDentry) <= end) {
            const struct CDentry *dent = (const struct CDentry *)cur;

            if (dent->size == 0) {
                break;
            }
            if (dent->size < sizeof(struct CDentry) || cur + dent->size > end) {
                break;
            }

            printf("%-6s %-10llu %-12llu %s\n",
                   ftype_name(dent->ftype),
                   (unsigned long long)dent->attr.size,
                   (unsigned long long)dent->whence,
                   dent->name);

            whence = dent->whence;
            entry_no++;
            cur += dent->size;
        }
    } while (!eof);

    printf("\nTotal entries: %d\n", entry_no);
    free(buf);
    return 0;
}

int main(int argc, char *argv[])
{
    int ret = EXIT_FAILURE;
    struct NameSpace ns = {0};
    const char *ns_name = NULL;
    struct Cred cred = {.uid = 0, .gid = 0};
    struct GIno dir_ino = {0};
    struct Attr dir_attr = {0};
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

    CHECK(resolve_path(&ns, path, &dir_ino, &dir_attr, &cred), "resolve_path");

    if (dir_attr.ftype != TypeDir) {
        fprintf(stderr, "'%s' is not a directory (ftype=%u)\n", path, dir_attr.ftype);
        goto cleanup;
    }

    printf("Directory: %s\n\n", path);
    CHECK(list_directory(&ns, &dir_ino, &cred), "list_directory");

    ret = EXIT_SUCCESS;

cleanup:
    if (ns.nsid != 0) {
        sfs_unexport_ns(&ns);
    }
    return ret;
}
