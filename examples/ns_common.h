#ifndef NS_COMMON_H
#define NS_COMMON_H

#include <stdint.h>

/**
 * 从命令行解析命名空间名称：argv[1] 为 ns_name。
 *
 * @param usage        用法说明字符串（用于参数不足时打印）
 * @param ns_name_out  输出指向 argv[1] 的指针（调用方勿释放）
 * @return 0 成功；参数不足返回 InvalidArgument
 */
int32_t sfs_example_parse_ns_name(int argc,
                                  char *argv[],
                                  const char *usage,
                                  const char **ns_name_out);

#endif /* NS_COMMON_H */
