# sfs-object-sdk

SFS 对象存储客户端 SDK。

## 概述

本仓库提供 SFS 对象存储的 C 语言客户端头文件与示例程序。典型流程为：挂载命名空间 → 元数据操作（lookup / create 等）→ 异步读写 → 卸载命名空间。

## 示例

```bash
make -C examples

./examples/file_rw_example <ns_name> [filename]
./examples/list_dir_example <ns_name> [path]
./examples/getattr_example <ns_name> [path]
```

## 说明

### 1. 接口层级

当前接口比较底层，直接对 **inode** 进行操作（如 `sfs_lookup`、`sfs_create`、`sfs_read` / `sfs_write`）。

后续计划改为类似 **POSIX `open` / `close`** 的高层接口，对**文件路径**进行操作，降低使用门槛。

### 2. 语言与异步模型

当前为 **C 语言**风格 API（如 `include/sfs_client.h` 中的绑定）。

后续计划提供 **Rust** 版本，并预计提供适配 **tokio** 框架的异步接口。

### 3. 错误与重试

客户端内部已对**临时性错误**内置重试逻辑。

业务侧也可根据 API 返回的 **错误码**（`CError`）自行判断是否重试，例如网络超时（`TimedOut`）、服务不可用（`Unavailable`）等场景。

## 目录结构

```
include/sfs_client.h   # C API 头文件
examples/              # 示例程序
```
