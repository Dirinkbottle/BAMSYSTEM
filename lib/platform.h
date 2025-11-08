/**
 * @file platform.h
 * @brief 平台相关功能头文件
 * @author BAMSYSTEM团队
 * @date 2025-11-08
 */

#ifndef PLATFORM_H
#define PLATFORM_H

/**
 * @brief 初始化平台环境（Windows设置UTF-8，Linux无操作）
 * @return 成功返回0，失败返回-1
 */
int init_platform(void);

#endif /* PLATFORM_H */
