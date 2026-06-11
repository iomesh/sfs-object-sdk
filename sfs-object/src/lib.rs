use std::{io::Error, ptr::null_mut};

use libloading::{Library, Symbol};
use once_cell::sync::OnceCell;
use sfs_mod::{GET_PLUGIN_FN_NAME, GetPluginMod, ObjectEntry, PluginMod};

static SFS_LIB: OnceCell<PluginMod> = OnceCell::new();

/// Loads the SFS object storage plugin dynamic library and initializes the global plugin interface.
///
/// # Example
///
/// ```no_run
/// use sfs_object::load_sfs_library;
///
/// load_sfs_library("/path/to/libsfs_object_plugin.so")?;
/// # Ok::<(), Box<dyn std::error::Error>>(())
/// ```
pub fn load_sfs_library(lib_path: &str) -> Result<(), Box<dyn std::error::Error>> {
    let lib = unsafe { Library::new(lib_path)? };
    let plugin_mod: Symbol<GetPluginMod> = unsafe { lib.get(GET_PLUGIN_FN_NAME.as_bytes())? };

    if !SFS_LIB.set(plugin_mod()).is_ok() {
        return Err("SFS library already loaded".into());
    }
    Ok(())
}

pub struct SfsObjectClient(*mut ());

unsafe impl Send for SfsObjectClient {}
unsafe impl Sync for SfsObjectClient {}

impl SfsObjectClient {
    /// Creates an object storage client.
    ///
    /// `ns_name` is the namespace name, `config_path` is the kubeconfig file path,
    /// and `url_list` is the list of API server URLs.
    ///
    /// # Example
    ///
    /// ```no_run
    /// use sfs_object::{load_sfs_library, SfsObjectClient};
    ///
    /// # async fn example() -> Result<(), Box<dyn std::error::Error>> {
    /// load_sfs_library("/path/to/libsfs_object_plugin.so")?;
    /// let mut client = SfsObjectClient::init("default", "/path/to/kubeconfig", vec![]).await?;
    /// client.close().await;
    /// # Ok(()) }
    /// ```
    pub async fn init(
        ns_name: &str,
        config_path: &str,
        url_list: Vec<String>,
    ) -> Result<Self, Error> {
        let open_object_client = SFS_LIB.get().unwrap().open_object_client;
        let url_list = url_list.into_iter().map(|s| s.into()).collect();
        let client = open_object_client(ns_name.into(), config_path.into(), url_list)
            .await
            .into_result()?;

        Ok(Self(client))
    }

    /// Closes the object storage client and releases plugin-side resources.
    pub async fn close(&mut self) {
        let close_object_client = SFS_LIB.get().unwrap().close_object_client;
        close_object_client(self.0).await;
    }

    /// Lists objects under the specified path.
    ///
    /// `whence` is used as the pagination cursor, and `buff_size` controls
    /// the buffer size for a single request. The returned boolean is `true`
    /// when the listing has reached the end.
    ///
    /// # Example
    ///
    /// ```no_run
    /// use sfs_object::{load_sfs_library, SfsObjectClient};
    ///
    /// # async fn example() -> Result<(), Box<dyn std::error::Error>> {
    /// # load_sfs_library("/path/to/libsfs_object_plugin.so")?;
    /// # let mut client = SfsObjectClient::init("default", "/path/to/kubeconfig", vec![]).await?;
    /// let (objects, eof) = client.list_objects("/data", 0, 1024).await?;
    /// # client.close().await;
    /// # Ok(()) }
    /// ```
    pub async fn list_objects(
        &self,
        path: &str,
        whence: u64,
        buff_size: usize,
    ) -> Result<(Vec<ObjectEntry>, bool), Error> {
        let list_objects = SFS_LIB.get().unwrap().list_objects;
        let list = list_objects(self.0, path.into(), whence, buff_size)
            .await
            .into_result()?;
        Ok((list.objects.into(), list.eof))
    }

    /// Deletes the object at the specified path.
    pub async fn delete(&self, path: &str) -> Result<(), Error> {
        let delete = SFS_LIB.get().unwrap().delete;
        delete(self.0, path.into()).await.into_result()?;
        Ok(())
    }

    /// Gets the metadata of the object at the specified path.
    pub async fn stat(&self, path: &str) -> Result<ObjectEntry, Error> {
        let stat = SFS_LIB.get().unwrap().stat;
        let entry = stat(self.0, path.into()).await.into_result()?;
        Ok(entry)
    }

    /// Opens the object at the specified path for put.
    ///
    /// Callers should write data with [`ObjectWriter::write_at`] and close the writer with
    /// [`ObjectWriter::close`] to commit the written content.
    ///
    /// This fails if another writer is already putting the same object.
    ///
    /// # Example
    ///
    /// ```no_run
    /// use sfs_object::{load_sfs_library, SfsObjectClient};
    ///
    /// # async fn example() -> Result<(), Box<dyn std::error::Error>> {
    /// # load_sfs_library("/path/to/libsfs_object_plugin.so")?;
    /// # let mut client = SfsObjectClient::init("default", "/path/to/kubeconfig", vec![]).await?;
    /// let mut writer = client.open_for_put("/data/object.txt").await?;
    /// writer.write_at(0, b"hello").await?;
    /// writer.close().await?;
    /// # client.close().await;
    /// # Ok(()) }
    /// ```
    pub async fn open_for_put(&self, path: &str) -> Result<ObjectWriter, Error> {
        let open_for_put = SFS_LIB.get().unwrap().open_for_put;
        let stream = open_for_put(self.0, path.into()).await.into_result()?;
        Ok(ObjectWriter(stream))
    }

    /// Opens the object at the specified path for get.
    ///
    /// # Example
    ///
    /// ```no_run
    /// use sfs_object::{load_sfs_library, SfsObjectClient};
    ///
    /// # async fn example() -> Result<(), Box<dyn std::error::Error>> {
    /// # load_sfs_library("/path/to/libsfs_object_plugin.so")?;
    /// # let mut client = SfsObjectClient::init("default", "/path/to/kubeconfig", vec![]).await?;
    /// let reader = client.open_for_get("/data/object.txt").await?;
    /// let mut buff = [0; 1024];
    /// let nread = reader.read_at(0, &mut buff).await?;
    /// # client.close().await;
    /// # Ok(()) }
    /// ```
    pub async fn open_for_get(&self, path: &str) -> Result<ObjectReader, Error> {
        let open_for_get = SFS_LIB.get().unwrap().open_for_get;
        let stream = open_for_get(self.0, path.into()).await.into_result()?;
        Ok(ObjectReader(stream))
    }
}

pub struct ObjectWriter(*mut ());

unsafe impl Send for ObjectWriter {}
unsafe impl Sync for ObjectWriter {}

impl ObjectWriter {
    /// Writes a byte slice to the object at the specified offset.
    ///
    /// This operation is synchronous: once it completes successfully, the written
    /// bytes are guaranteed to be persisted.
    ///
    /// Best performance is achieved when `offset` is 1 MiB aligned and `content`
    /// is 1 MiB long.
    pub async fn write_at(&self, offset: u64, content: &[u8]) -> Result<(), Error> {
        let write_at = SFS_LIB.get().unwrap().write_at;
        write_at(self.0, offset, content.into())
            .await
            .into_result()?;
        Ok(())
    }

    /// Closes the writer and commits the written content.
    ///
    /// The written object becomes visible immediately after `close` completes,
    /// and remains invisible before `close` completes.
    pub async fn close(&mut self) -> Result<(), Error> {
        let close_write = SFS_LIB.get().unwrap().close_write;
        close_write(self.0).await.into_result()?;
        self.0 = null_mut();
        Ok(())
    }
}

pub struct ObjectReader(*mut ());

unsafe impl Send for ObjectReader {}
unsafe impl Sync for ObjectReader {}

impl ObjectReader {
    /// Reads data from the object at the specified offset into `buff`.
    ///
    /// Returns the number of bytes read.
    ///
    /// Best performance is achieved when `offset` is 1 MiB aligned and `buff`
    /// is 1 MiB long.
    ///
    /// # Example
    ///
    /// ```no_run
    /// use sfs_object::{load_sfs_library, SfsObjectClient};
    ///
    /// # async fn example() -> Result<(), Box<dyn std::error::Error>> {
    /// # load_sfs_library("/path/to/libsfs_object_plugin.so")?;
    /// # let mut client = SfsObjectClient::init("default", "/path/to/kubeconfig", vec![]).await?;
    /// # let reader = client.open_for_get("/data/object.txt").await?;
    /// let mut buff = [0; 1024];
    /// let nread = reader.read_at(0, &mut buff).await?;
    /// # client.close().await;
    /// # Ok(()) }
    /// ```
    pub async fn read_at(&self, offset: u64, buff: &mut [u8]) -> Result<usize, Error> {
        let read_at = SFS_LIB.get().unwrap().read_at;
        let nread = read_at(self.0, offset, buff.into()).await.into_result()?;
        Ok(nread)
    }
}

impl Drop for ObjectReader {
    /// Notifies the plugin to release read-side resources when the reader is dropped.
    fn drop(&mut self) {
        let close_read = SFS_LIB.get().unwrap().close_read;
        close_read(self.0);
    }
}
