#ifndef __SFS_CLIENT_H__
#define __SFS_CLIENT_H__

#pragma once

/* Generated with cbindgen:0.20.0 */

/*
 * SFS 对象存储客户端 C API。
 *
 * 本头文件由 cbindgen 从 Rust 绑定生成，结构体布局请勿随意修改。
 * 接口注释可随 SDK 文档一并维护。
 *
 * 典型用法：
 *   1. sfs_ns_name_to_nsid() 将命名空间名称解析为 nsid（可选）
 *   2. sfs_export_ns() 按名称挂载命名空间
 *   3. sfs_lookup() / sfs_create() 等元数据操作
 *   4. sfs_read() / sfs_write() 异步 I/O（通过 IoCbInfo 回调完成）
 *   5. sfs_unexport_ns() 释放命名空间
 *
 * 返回值：成功为 0，失败为 CError 错误码（与 errno 部分对应）。
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/** 读文件时可能触发预取的标志位 */
#define MAYBE_PREFETCH_FILE_BIT 1

/** Attr 结构体当前版本号 */
#define CURRENT_ATTR_VERSION 0

/**
 * API 错误码（CError）。
 * 0 表示成功，非 0 为具体错误类型。
 */
enum CError {
        PermissionDenied = 1,   /**< 权限不足 */
        NotFound = 2,           /**< 对象不存在 */
        IoErr = 5,              /**< I/O 错误 */
        BadFileDescriptor = 9,  /**< 无效描述符 */
        AccessDenied = 13,      /**< 访问被拒绝 */
        ResourceBusy = 16,      /**< 资源忙 */
        AlreadyExists = 17,     /**< 对象已存在 */
        NotADirectory = 20,     /**< 非目录 */
        IsADirectory = 21,      /**< 是目录（期望为文件） */
        InvalidArgument = 22,   /**< 无效参数 */
        TooManyOpenFiles = 24,  /**< 打开文件过多 */
        StorageFull = 28,       /**< 存储空间已满 */
        OutOfRange = 34,        /**< 越界 */
        DirectoryNotEmpty = 39, /**< 目录非空 */
        TimedOut = 110,         /**< 操作超时 */
        Stale = 116,            /**< 文件句柄/属性已过期 */
        Unavailable = 1002,     /**< 服务不可用 */
        Aborted = 1003,         /**< 操作已中止 */
        Unimplemented = 1004,   /**< 未实现 */
        NotAFile = 1005,        /**< 非普通文件 */
        IsAFile = 1006,         /**< 是普通文件（期望为目录） */
        RpcTransport = 1300,    /**< RPC 传输错误 */
        LockConflict = 1400,    /**< 文件锁冲突 */
        ReclaimNotMatch = 1402, /**< 锁回收不匹配 */
};
typedef int32_t CError;

/**
 * 创建文件/对象时的模式（对应 NFS CREATE 语义）。
 */
enum CreateMode {
        Unchecked,  /**< 存在则截断/覆盖，不存在则创建 */
        Guarded,    /**< 仅当不存在时创建，存在则失败 */
        Exclusive,  /**< 仅当不存在时创建，并保证独占 */
};
typedef uint32_t CreateMode;

/** 文件系统对象类型 */
enum FileType {
        TypeFile = 0,     /**< 普通文件 */
        TypeDir = 1,      /**< 目录 */
        TypeSymlink = 2,  /**< 符号链接 */
        TypeSock = 3,     /**< 套接字 */
        TypeFifo = 4,     /**< 管道 */
        TypeChr = 5,      /**< 字符设备 */
        TypeBlk = 6,      /**< 块设备 */
};
typedef int32_t FileType;

/**
 * setattr 时指定要更新的属性字段（位掩码，可 OR 组合）。
 */
enum RequestAttrMask {
        AttrNoop = 0,
        AttrSize = 1,          /**< 更新 size */
        AttrMode = 2,          /**< 更新 mode */
        AttrUid = 4,           /**< 更新 uid */
        AttrGid = 8,           /**< 更新 gid */
        AttrAtime = 16,        /**< 更新 atime */
        AttrMtime = 32,        /**< 更新 mtime */
        SetAtimeServer = 64,   /**< atime 由服务端设置 */
        SetMtimeServer = 128,  /**< mtime 由服务端设置 */
        AttrBlksize = 256,     /**< 更新 blksize */
        AttrBlocks = 512,      /**< 更新 blocks */
};
typedef int32_t RequestAttrMask;

/** 全局 inode 标识（shard + ino + igen） */
struct GIno {
        uint32_t shard_id; /**< 分片 ID */
        uint32_t padding;
        uint64_t ino;      /**< inode 号 */
        uint64_t igen;     /**< inode 世代号 */
};

/** 纳秒精度时间戳 */
struct TimeSpec {
        int64_t tv_sec;  /**< 秒 */
        int64_t tv_nsec; /**< 纳秒 */
};

/**
 * 对象完整属性（类似 stat 结果）。
 */
struct Attr {
        uint8_t version;       /**< 属性版本，见 CURRENT_ATTR_VERSION */
        uint8_t ftype;         /**< 对象类型，见 FileType */
        uint16_t gen_hi;
        uint32_t gen_lo;
        uint64_t ino;
        uint64_t igen;
        uint32_t ishard_id;    /**< 分片 ID */
        uint32_t mode;         /**< 权限模式 */
        uint32_t nlink;        /**< 硬链接计数 */
        uint32_t uid;
        uint32_t gid;
        uint32_t blksize;      /**< 首选 I/O 块大小 */
        uint64_t blocks;       /**< 占用块数 */
        uint64_t size;         /**< 文件字节大小 */
        struct TimeSpec atime; /**< 最后访问时间 */
        struct TimeSpec mtime; /**< 最后修改时间 */
        struct TimeSpec ctime; /**< 元数据变更时间 */
        struct TimeSpec btime; /**< 创建时间 */
};

/** 调用者身份凭证（类似 POSIX uid/gid） */
struct Cred {
        uint32_t uid;
        uint32_t gid;
};

/**
 * 目录或对象的变更世代信息。
 * 用于检测并发修改（如 rename、create 后父目录是否变化）。
 */
struct AttrGen {
        uint64_t change;
        uint64_t size;
        struct TimeSpec ctime;
        struct TimeSpec mtime;
};

/**
 * 已挂载的命名空间句柄。
 * export 成功后 root 为根目录 inode。
 */
struct NameSpace {
        uint32_t nsid;
        struct GIno root;
};

/** 命名空间容量与配额统计信息 */
struct NsInfo {
        uint32_t nsid;
        uint32_t block_size;
        uint64_t total_blocks;
        uint64_t free_blocks;
        uint64_t avail_blocks;
        uint64_t total_files;
        uint64_t free_files;
        uint64_t avail_files;
        struct TimeSpec birth_time;
};

/**
 * 创建/设置属性时的输入结构。
 * valid 为 RequestAttrMask 位掩码，仅更新掩码中置位的字段。
 */
struct IAttr {
        uint32_t valid;
        uint32_t mode;
        uint32_t uid;
        uint32_t gid;
        uint64_t size;
        struct TimeSpec atime;
        struct TimeSpec mtime;
        uint32_t blksize;
        uint64_t blocks;
};

/** 读写 I/O 向量（类似 POSIX iovec） */
struct SfsIoVec {
        uint64_t iov_len;    /**< 缓冲区长度 */
        const void *iov_base; /**< 写时为 const；读时在 sfs_read 中可写 */
};

/**
 * 异步 I/O 完成回调信息。
 * 调用 sfs_read/sfs_write 前设置 callback；
 * 完成后在回调中检查 rc（0 为成功）和 cookie。
 */
struct IoCbInfo {
        void (*callback)(struct IoCbInfo*);
        int32_t rc;      /**< 完成状态，0 成功，否则为 CError */
        uint64_t cookie; /**< 操作 cookie，可与 fsync 关联 */
};

/**
 * readdir 返回的目录项（变长，name 为柔性数组）。
 * 缓冲区中多项连续排列，下一项偏移为当前项的 size 字节。
 */
struct CDentry {
        size_t size;         /**< 本条目总字节数（含 name） */
        struct GIno ino;
        uint64_t whence;     /**< 目录遍历 cookie，下次 readdir 传入 */
        uint32_t ftype;      /**< FileType */
        struct Attr attr;
        char name[0];        /**< 以 '\0' 结尾的文件名 */
};

/**
 * 轻量目录项（不含完整 Attr，仅 file_id）。
 */
struct CDentryLight {
        size_t size;
        uint64_t file_id;
        uint64_t whence;
        uint32_t ftype;
        char name[0];
};

/* ---------- 辅助函数 ---------- */

/**
 * 将 GIno 格式化为可读字符串。
 * @param buf  输出缓冲区
 * @param len  缓冲区长度
 * @return buf，失败时可能为 NULL
 */
char *ino_format(const struct GIno *self, char *buf, int32_t len);

/** 从 Attr 提取 GIno */
struct GIno attr2ino(const struct Attr *self);

/** 从 Attr 计算 change 世代号 */
uint64_t attr2change(const struct Attr *self);

/**
 * 将 Attr 格式化为可读字符串。
 * @param buf  输出缓冲区
 * @param len  缓冲区长度
 * @return buf，失败时可能为 NULL
 */
char *attr_format(const struct Attr *self, char *buf, int32_t len);

/** 从 Attr 提取 uid/gid 作为 Cred */
struct Cred attr2cred(const struct Attr *self);

/** 判断 AttrGen 是否为无效/未初始化 */
bool attr_gen_is_invalid(const struct AttrGen *self);

/* ---------- 命名空间 ---------- */

/**
 * 根据命名空间名称查询 nsid。
 * @param ns_name   以 '\0' 结尾的命名空间名称
 * @param nsid_out  输出命名空间 ID
 * @return 0 成功；名称不存在返回 NotFound，否则返回 CError
 */
int32_t sfs_ns_name_to_nsid(const char *ns_name, uint32_t *nsid_out);

/**
 * 挂载（导出）命名空间。
 * @param ns_name  以 '\0' 结尾的命名空间名称
 * @param ns_out   输出已挂载的 NameSpace（含 root inode）
 * @return 0 成功，否则 CError
 */
int32_t sfs_export_ns(const char *ns_name, struct NameSpace *ns_out);

/**
 * 卸载（取消导出）命名空间，释放相关资源。
 */
void sfs_unexport_ns(const struct NameSpace *self);

/**
 * 获取命名空间统计信息（容量、文件数等）。
 * @param c_nsinfo 输出 NsInfo
 * @return 0 成功，否则 CError
 */
int32_t sfs_stat_ns(const struct NameSpace *self, struct NsInfo *c_nsinfo);

/* ---------- 元数据：查询 ---------- */

/**
 * 在父目录中按名称查找对象。
 * @param pino    父目录 inode
 * @param c_name  相对名称（单级，不含 '/'）
 * @param c_attr  输出对象属性
 * @return 0 成功，否则 CError（如 NotFound）
 */
int32_t sfs_lookup(const struct NameSpace *self,
                   const struct GIno *pino,
                   const char *c_name,
                   const struct Cred *cred,
                   struct Attr *c_attr);

/**
 * 按 inode 获取对象属性。
 * @param span_ctx  分布式追踪上下文，无则传 NULL
 * @return 0 成功，否则 CError
 */
int32_t sfs_getattr(const struct NameSpace *self,
                    const struct GIno *ino,
                    const struct Cred *cred,
                    struct Attr *c_attr,
                    const void *span_ctx);

/**
 * 读取目录内容（含完整 Attr）。
 * @param ino     目录 inode
 * @param whence  起始 cookie，首次传 0
 * @param c_dents 输出缓冲区，存放连续的 CDentry
 * @param size    缓冲区字节数
 * @param c_eof   输出是否已读完（非 0 表示 EOF）
 * @return 0 成功，否则 CError
 *
 * 遍历方式：解析缓冲区中各 CDentry，以 dent->size 步进；
 * 若未 EOF，将最后一项的 whence 作为下次调用的 whence。
 */
int32_t sfs_readdir(const struct NameSpace *self,
                    const struct GIno *ino,
                    uint64_t whence,
                    const struct Cred *cred,
                    void *c_dents,
                    uint32_t size,
                    int *c_eof);

/**
 * 轻量版读目录（返回 CDentryLight，不含完整 Attr）。
 * 参数语义同 sfs_readdir。
 */
int32_t sfs_readdir_light(const struct NameSpace *self,
                          const struct GIno *ino,
                          uint64_t whence,
                          const struct Cred *cred,
                          void *c_dents,
                          uint32_t size,
                          int *c_eof);

/* ---------- 元数据：创建与修改 ---------- */

/**
 * 在父目录下创建文件或特殊文件节点。
 * @param pino                  父目录 inode
 * @param name                  新对象名称
 * @param cm                    创建模式 CreateMode
 * @param filetype              对象类型 FileType
 * @param attr_in               初始属性（valid 指定有效字段）
 * @param prev_parent_gen_out   可选，输出父目录变更前世代
 * @param parent_out            可选，输出更新后的父目录属性
 * @param child_out             输出新对象属性
 * @return 0 成功，否则 CError
 */
int32_t sfs_create(const struct NameSpace *self,
                   const struct GIno *pino,
                   const char *name,
                   CreateMode cm,
                   FileType filetype,
                   const struct IAttr *attr_in,
                   const struct Cred *cred,
                   struct AttrGen *prev_parent_gen_out,
                   struct Attr *parent_out,
                   struct Attr *child_out,
                   const void *span_ctx);

/**
 * 设置对象属性（由 iattr->valid 掩码决定更新哪些字段）。
 * @param attr_out 输出更新后的完整属性
 * @return 0 成功，否则 CError
 */
int32_t sfs_setattr(const struct NameSpace *self,
                    const struct GIno *ino,
                    struct IAttr *iattr,
                    struct Attr *attr_out,
                    const struct Cred *cred,
                    const void *span_ctx);

/**
 * 删除目录项（硬链接减一，无链接时销毁对象）。
 * @param pino          父目录 inode
 * @param name          要删除的名称
 * @param prev_gen_out  可选，父目录变更前世代
 * @param attr_out      可选，输出父目录更新后属性
 * @return 0 成功，否则 CError
 */
int32_t sfs_unlink(const struct NameSpace *self,
                   const struct GIno *pino,
                   const char *name,
                   const struct Cred *cred,
                   struct AttrGen *prev_gen_out,
                   struct Attr *attr_out,
                   const void *span_ctx);

/**
 * 创建子目录。
 * @param pino  父目录 inode
 * @param name  新目录名
 * @param iattr 初始属性
 * @return 0 成功，否则 CError
 */
int32_t sfs_mkdir(const struct NameSpace *self,
                  const struct GIno *pino,
                  const char *name,
                  const struct IAttr *iattr,
                  const struct Cred *cred,
                  struct AttrGen *prev_parent_gen_out,
                  struct Attr *parent_out,
                  struct Attr *child_out,
                  const void *span_ctx);

/**
 * 删除空目录。
 * @return 0 成功；目录非空返回 DirectoryNotEmpty
 */
int32_t sfs_rmdir(const struct NameSpace *self,
                  const struct GIno *pino,
                  const char *name,
                  const struct Cred *cred,
                  struct AttrGen *prev_gen_out,
                  struct Attr *attr_out,
                  const void *span_ctx);

/**
 * 重命名或移动目录项（可在不同父目录间）。
 * @param oldino/newino  源/目标父目录 inode
 * @param oldname/newname 源/目标名称
 * @return 0 成功，否则 CError
 */
int32_t sfs_rename(const struct NameSpace *self,
                   const struct GIno *oldino,
                   const char *oldname,
                   const struct GIno *newino,
                   const char *newname,
                   const struct Cred *cred,
                   struct AttrGen *prev_old_dir_gen_out,
                   struct Attr *old_dir_attr_out,
                   struct AttrGen *prev_new_dir_gen_out,
                   struct Attr *new_dir_attr_out);

/**
 * 创建硬链接：在 pino 下为已有 inode 增加一个目录项 name。
 * @param ino   已有对象 inode
 * @param pino  父目录 inode
 * @return 0 成功，否则 CError
 */
int32_t sfs_link(const struct NameSpace *self,
                 const struct GIno *ino,
                 const struct GIno *pino,
                 const char *name,
                 const struct Cred *cred,
                 struct AttrGen *prev_parent_gen_out,
                 struct Attr *parent_out,
                 struct Attr *child_out);

/**
 * 在目录 ino 下创建符号链接 name，指向 link 路径字符串。
 * @param link  链接目标路径（非 inode）
 * @return 0 成功，否则 CError
 */
int32_t sfs_symlink(const struct NameSpace *self,
                    const struct GIno *ino,
                    const char *name,
                    const char *link,
                    const struct IAttr *attr_in,
                    const struct Cred *cred,
                    struct AttrGen *prev_parent_gen_out,
                    struct Attr *parent_out,
                    struct Attr *child_out);

/**
 * 读取符号链接目标路径。
 * @param c_link  输出缓冲区
 * @param size    缓冲区大小
 * @return 0 成功，否则 CError
 */
int32_t sfs_readlink(const struct NameSpace *self,
                     const struct GIno *ino,
                     const struct Cred *cred,
                     char *c_link,
                     uint32_t size);

/* ---------- 数据 I/O ---------- */

/**
 * 异步写入文件。
 * @param offset      文件内起始偏移
 * @param iovs        I/O 向量数组
 * @param count       向量个数
 * @param sync        true 表示同步落盘语义
 * @param io_cb_info  回调信息，需预先设置 callback
 * @param span_ctx    追踪上下文，无则 NULL
 *
 * 完成后在 io_cb_info->callback 中通知，检查 io_cb_info->rc。
 */
void sfs_write(const struct NameSpace *self,
               const struct GIno *ino,
               uint64_t offset,
               const struct SfsIoVec *iovs,
               int32_t count,
               bool sync,
               const struct Cred *cred,
               struct IoCbInfo *io_cb_info,
               const void *span_ctx);

/**
 * 将指定字节范围刷盘。
 * @param offset       起始偏移
 * @param size         字节长度
 * @param cookie_out   输出 fsync cookie
 * @return 0 成功，否则 CError
 */
int32_t sfs_fsync(const struct NameSpace *self,
                  const struct GIno *ino,
                  uint64_t offset,
                  uint64_t size,
                  const struct Cred *cred,
                  uint64_t *cookie_out);

/**
 * 异步读取文件。
 * @param iovs        可写 I/O 向量数组
 * @param io_cb_info  回调信息，需预先设置 callback
 *
 * 完成后在回调中检查 io_cb_info->rc。
 */
void sfs_read(const struct NameSpace *self,
              const struct GIno *ino,
              uint64_t offset,
              struct SfsIoVec *iovs,
              int32_t count,
              const struct Cred *cred,
              struct IoCbInfo *io_cb_info,
              const void *span_ctx);

#endif /* __SFS_CLIENT_H__ */
