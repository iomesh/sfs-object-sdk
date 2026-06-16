# SFS Object SDK

SFS Object SDK 是一个 Rust workspace，用于通过动态插件访问 SFS 对象存储能力。

项目主要包含两个 crate：

- `sfs-mod`：定义插件 ABI、FFI 类型和插件导出入口名称。
- `sfs-object`：面向使用方的对象存储客户端封装，负责加载插件并调用对象操作接口。

## 功能概览

- 动态加载对象存储插件库。
- 初始化和关闭对象存储客户端。
- 列举、删除对象。
- 打开对象进行 put 写入或 get 读取。
- 支持基于 offset 的随机读写。

## 基本用法

使用前需要先加载插件动态库。插件库必须导出 `get_sfs_object_plugin_mod` 符号。

```rust
use sfs_object::{load_sfs_library, SfsObjectClient};

async fn example() -> Result<(), Box<dyn std::error::Error>> {
    load_sfs_library("/path/to/libsfs_object_plugin.so")?;

    let mut client = SfsObjectClient::init(
        "default",
        "/path/to/kubeconfig",
        vec!["https://127.0.0.1:6443".to_string()],
    )
    .await?;

    client.close().await;
    Ok(())
}
```

## 列举对象

`list_objects` 返回对象列表和 `eof` 标记。`eof` 为 `true` 表示本次列举已经到达末尾。

```rust
let (objects, eof) = client.list_objects("/data", 0, 1024).await?;
```

参数说明：

- `path`：要列举的路径。
- `whence`：分页游标。
- `buff_size`：单次请求的缓冲区大小。

## 写入对象

使用 `open_for_put` 打开一个写入会话，然后通过 `ObjectWriter::write_at` 写入数据，最后调用 `ObjectWriter::close` 提交内容。

```rust
let mut writer = client.open_for_put("/data/object.txt").await?;
writer.write_at(0, b"hello").await?;
writer.close().await?;
```

注意事项：

- 如果同一个 object 正在被其他 writer 写入，`open_for_put` 会失败。
- `write_at` 是同步持久化操作，成功返回后写入的数据保证已经持久化。
- `close` 完成后对象会立即可见；`close` 完成前对象不可见。
- 当 `offset` 按 1 MiB 对齐且写入内容长度为 1 MiB 时，可以获得最佳性能。

## 读取对象

使用 `open_for_get` 打开一个读取会话，然后通过 `ObjectReader::read_at` 按 offset 读取数据。

```rust
let reader = client.open_for_get("/data/object.txt").await?;
let mut buff = [0; 1024];
let nread = reader.read_at(0, &mut buff).await?;
```

性能建议：

- 当 `offset` 按 1 MiB 对齐且 `buff` 长度为 1 MiB 时，可以获得最佳性能。

## 删除对象

```rust
client.delete("/data/object.txt").await?;
```

## 插件接口

插件侧需要实现 `sfs-mod` 中定义的 `PluginMod`，并通过下面的符号导出：

```rust
pub const GET_PLUGIN_FN_NAME: &str = "get_sfs_object_plugin_mod";
```

客户端通过 `load_sfs_library` 加载动态库后，会从该符号获取插件函数表。

## 开发

常用检查命令：

```bash
cargo check
cargo test
```
