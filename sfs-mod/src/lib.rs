use abi_stable::{
    StableAbi,
    std_types::{RIoError, RResult, RSlice, RSliceMut, RStr, RString, RVec},
};
use async_ffi::FfiFuture;

type Result<T> = RResult<T, RIoError>;

#[derive(StableAbi)]
#[repr(C)]
pub struct ObjectEntry {
    pub size: u64,
    pub path: RString,
    pub isdir: bool,
    pub mtime: u64,
    pub whence: u64,
}

#[derive(StableAbi)]
#[repr(C)]
pub struct ListedObjects {
    pub objects: RVec<ObjectEntry>,
    pub eof: bool,
}

#[derive(StableAbi)]
#[sabi(kind(Prefix))]
#[repr(C)]
pub struct PluginMod {
    pub open_object_client: extern "C" fn(
        ns_name: RStr,
        config_path: RStr,
        url_list: RVec<RString>,
    ) -> FfiFuture<Result<*mut ()>>,
    pub close_object_client: extern "C" fn(client: *mut ()) -> FfiFuture<()>,

    pub list_objects: extern "C" fn(
        client: *const (),
        path: RStr,
        whence: u64,
        buff_size: usize,
    ) -> FfiFuture<Result<ListedObjects>>,

    pub delete: extern "C" fn(client: *const (), path: RStr) -> FfiFuture<Result<()>>,

    pub open_for_put: extern "C" fn(client: *const (), path: RStr) -> FfiFuture<Result<*mut ()>>,
    pub write_at: extern "C" fn(
        fh: *const (),
        offfset: u64,
        content: RSlice<'_, u8>,
    ) -> FfiFuture<Result<()>>,
    pub close_write: extern "C" fn(fh: *mut ()) -> FfiFuture<Result<()>>,

    pub open_for_get: extern "C" fn(client: *const (), path: RStr) -> FfiFuture<Result<*mut ()>>,
    pub read_at: extern "C" fn(
        fh: *const (),
        offset: u64,
        buff: RSliceMut<'_, u8>,
    ) -> FfiFuture<Result<usize>>,
    pub close_read: extern "C" fn(fh: *mut ()) -> (),
}

pub type GetPluginMod = extern "C" fn() -> PluginMod;
pub const GET_PLUGIN_FN_NAME: &str = "get_sfs_object_plugin_mod";
