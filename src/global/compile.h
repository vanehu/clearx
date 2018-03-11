/*
* Copyright (c) 2017-2018 the ClearX authors
* All rights reserved.
*
* The project sponsor and lead author is Xu Rendong.
* E-mail: xrd@ustc.edu, QQ: 277195007, WeChat: ustc_xrd
* You can get more information at https://xurendong.github.io
* For names of other contributors see the contributors file.
*
* Commercial use of this code in source and binary forms is
* governed by a LGPL v3 license. You may get a copy from the
* root directory. Or else you should get a specific written
* permission from the project author.
*
* Individual and educational use of this code in source and
* binary forms is governed by a 3-clause BSD license. You may
* get a copy from the root directory. Certainly welcome you
* to contribute code of all sorts.
*
* Be sure to retain the above copyright notice and conditions.
*/

#ifndef CLEARX_GLOBAL_COMPILE_H
#define CLEARX_GLOBAL_COMPILE_H

//---------- 基础组件 ----------//

#define CLEARX_GLOBAL_EXP
//#define CLEARX_GLOBAL_IMP

#define CLEARX_SHARES_EXP
//#define CLEARX_SHARES_IMP

//---------- 设置结束 ----------//

#ifdef CLEARX_GLOBAL_EXP
    #define CLEARX_GLOBAL_EXPIMP __declspec(dllexport)
#endif

#ifdef CLEARX_GLOBAL_IMP
    #define CLEARX_GLOBAL_EXPIMP __declspec(dllimport)
#endif

//------------------------------//

#ifdef CLEARX_SHARES_EXP
    #define CLEARX_SHARES_EXPIMP __declspec(dllexport)
#endif

#ifdef CLEARX_SHARES_IMP
    #define CLEARX_SHARES_EXPIMP __declspec(dllimport)
#endif

//------------------------------//

#endif // CLEARX_GLOBAL_COMPILE_H
