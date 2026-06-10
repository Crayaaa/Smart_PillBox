/*
 * Copyright (c) 2024-2025, Qi Yaolong.
 * dragon@hbu.edu.cn
 * HeBei University, China.
 * 
 * 保留所有权利。除非遵守以下条款，否则您不得使用此文件：
 * 本文件及其内容仅用于个人学习用途，在此范围之外的使用请先获得版权持有人的授权。
 *
 * 本程序由版权持有人和贡献人“按原样”提供，不提供任何明示或暗示担保，包括但不局限于对适销性和特定
 * 用途适合性的暗示担保。在任何情况下，版权持有人或贡献人对因使用本程序而导致的任何直接、间接、附带、
 * 特殊、典型或后果性损害（包括但不限于购买替代商品或服务；使用损失、数据丢失或利润损失；业务中断）
 * 不承担任何责任，无论其如何产生，也不论其责任理论为何，无论是合同、严格责任还是侵权（包括疏忽或
 * 其他），即使已告知此类损害的可能性。
 */

// 标准输入输出
#include <stdio.h>

// ohos_init.h是OpenHarmony的特有头文件
// 位置：utils\native\lite\include\ohos_init.h
// Provides the entries for initializing services and features during service development
// 在开发中，它提供了一系列入口，用于初始化服务(services)和功能(features)。
//
// 在系统启动过程中，服务和功能按以下顺序初始化：
// 阶段1. core
// 阶段2. core system service
// 阶段3. core system feature
// 阶段4. system startup
// 阶段5. system service
// 阶段6. system feature
// 阶段7. application-layer service
// 阶段8. application-layer feature
//
#include "ohos_init.h"

// 定义一个函数，输出hello world
void hello(void)
{
    printf("Hello world!\r\n");
}

// ohos_init.h中定义了以下8个宏，用于让一个函数以“优先级2”在系统启动过程的1-8阶段执行。
// 即：函数会被标记为入口，在系统启动过程的1-8阶段，以“优先级2”被调用。
// 优先级范围：0-4。
// 优先级顺序：0, 1, 2, 3, 4
// CORE_INIT()：          阶段1. core
// SYS_SERVICE_INIT()：   阶段2. core system service
// SYS_FEATURE_INIT()：   阶段3. core system feature
// SYS_RUN()：            阶段4. system startup
// SYSEX_SERVICE_INIT()： 阶段5. system service
// SYSEX_FEATURE_INIT()： 阶段6. system feature
// APP_SERVICE_INIT()：   阶段7. application-layer service
// APP_FEATURE_INIT()：   阶段8. application-layer feature

// SYS_RUN() 是ohos_init.h中定义的宏，让函数在系统启动时执行。
// 让hello函数以“优先级2”在系统启动过程的“阶段4. system startup”阶段执行。
SYS_RUN(hello);
