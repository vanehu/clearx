/*
* Copyright (c) 2018-2018 the ClearX authors
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

#include <chrono>
#include <thread>
#include <stdlib.h>
#include <iostream>

#include <common/sysdef.h>

#ifdef __OS_LINUX__
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#endif

#ifdef __OS_WINDOWS__
#include <windows.h>
#include <shellapi.h> // NOTIFYICONDATA
#pragma comment( lib, "shell32.lib" ) // Shell_NotifyIcon
#endif

#include <common/common.h>
#include <syslog/syslog.h>
#include <sysdbi_m/sysdbi_m.h>

#include "global/define.h"

#include "clearx.h"

bool g_command_model = false; // 标记进入命令行模式
bool g_block_auto_info = false; // 阻止自动显示信息
void* g_single_mutex = NULL; // 单例限制

void SystemUninitialize() { // 在控制台事件和单例限制退出时调用会异常
	try {
		basicx::SysDBI_M* sysdbi_m = basicx::SysDBI_M::GetInstance(); // 02
		if( sysdbi_m != nullptr ) {
			sysdbi_m->~SysDBI_M();
		}

		basicx::SysLog_S* syslog = basicx::SysLog_S::GetInstance(); // 01
		if( syslog != nullptr ) {
			syslog->~SysLog_S();
		}
	} // try
	catch( ... ) {}
}

int __stdcall ConsoleHandler( unsigned long event ) { // 控制台事件检测
	std::string log_cate = "<SYSTEM_EVENT>";
	basicx::SysLog_S* syslog = basicx::SysLog_S::GetInstance();

#ifdef __OS_LINUX__
	switch( event ) {
	case SIGINT: // 用户按下 "Ctrl + C" 键
		g_command_model = true;
		syslog->LogWrite( basicx::syslog_level::c_info, log_cate, std::string( "用户按下 Ctrl + C 键。" ) );
		break;
	case SIGQUIT: // 关闭控制台窗口
		syslog->LogWrite( basicx::syslog_level::c_warn, log_cate, std::string( "控制台窗口被强制关闭，系统结束运行！\r\n" ) );
		SystemUninitialize(); // 这里一般来不及清理
		break;
	}
#endif

#ifdef __OS_WINDOWS__
	switch( event ) {
	case CTRL_C_EVENT: // 用户按下 "Ctrl + C" 键
		g_command_model = true;
		syslog->LogWrite( basicx::syslog_level::c_info, log_cate, std::string( "用户按下 Ctrl + C 键。" ) );
		break;
	case CTRL_BREAK_EVENT: // 用户按下 "Ctrl + Break" 键
		syslog->LogWrite( basicx::syslog_level::c_info, log_cate, std::string( "用户按下 Ctrl + Break 键。" ) );
		break;
	case CTRL_CLOSE_EVENT: // 关闭控制台窗口 // 可清理延时时间 5 秒
		syslog->LogWrite( basicx::syslog_level::c_warn, log_cate, std::string( "控制台窗口被强制关闭，系统结束运行！\r\n" ) );
		SystemUninitialize(); // 这里一般来不及清理
		break;
	case CTRL_LOGOFF_EVENT: // 用户注销 // 可清理延时时间 20 秒
		syslog->LogWrite( basicx::syslog_level::c_warn, log_cate, std::string( "用户注销，系统结束运行！\r\n" ) );
		break;
	case CTRL_SHUTDOWN_EVENT: // 用户关机 // 可清理延时时间 20 秒
		syslog->LogWrite( basicx::syslog_level::c_warn, log_cate, std::string( "用户关机，系统结束运行！\r\n" ) );
		break;
	}
#endif

	return 1;
}

void ConsoleEventsSet() {
#ifdef __OS_LINUX__
	struct sigaction sig_action;
	sig_action.sa_flags = 0;
	sig_action.sa_handler = ConsoleHandler; // 信号处理函数
	sigemptyset( &sig_action.sa_mask ); // 将 sa_mask 初始化为 0
	sigaction( SIGINT, &sig_action, NULL );
	sigaction( SIGQUIT, &sig_action, NULL );
#endif

#ifdef __OS_WINDOWS__
	if( !SetConsoleCtrlHandler( (PHANDLER_ROUTINE)ConsoleHandler, TRUE ) ) { // 安装事件处理，用于检测控制台关闭及按键等
		std::string log_cate = "<SYSTEM_MAIN>";
		basicx::SysLog_S* syslog = basicx::SysLog_S::GetInstance();
		syslog->LogWrite( basicx::syslog_level::c_error, log_cate, std::string( "无法安装控制台事件检测句柄！" ) );
	}
#endif
}

void CheckSingleMutex() { // 单例限制检测
#ifdef __OS_LINUX__
	sem_t* sem_item = sem_open( DEF_APP_NAME, O_RDWR | O_CREAT, 0644, 1 );
	int32_t result = sem_wait( sem_item );
	if( result != 0 ) {
		basicx::SysLog_S* syslog = basicx::SysLog_S::GetInstance();
		syslog->LogWrite( basicx::syslog_level::c_warn, std::string( "<SYSTEM_MAIN>" ), std::string( "已有另外一个实例在运行！" ) );
		std::this_thread::sleep_for( std::chrono::seconds( 1 ) ); // 留点时间输出日志
		sem_close( sem_item );
		SystemUninitialize(); // 这里能完整清理
		exit( 0 );
	}
#endif

#ifdef __OS_WINDOWS__
	g_single_mutex = ::CreateMutexA( NULL, FALSE, DEF_APP_NAME );
	if( GetLastError() == ERROR_ALREADY_EXISTS ) {
		basicx::SysLog_S* syslog = basicx::SysLog_S::GetInstance();
		syslog->LogWrite( basicx::syslog_level::c_warn, std::string( "<SYSTEM_MAIN>" ), std::string( "已有另外一个实例在运行！" ) );
		std::this_thread::sleep_for( std::chrono::seconds( 1 ) ); // 留点时间输出日志
		CloseHandle( g_single_mutex );
		SystemUninitialize(); // 这里能完整清理
		exit( 0 );
	}
#endif
}

void SetConsoleWindow() {
#ifdef __OS_WINDOWS__
	wchar_t app_exec_path[MAX_PATH] = { 0 };
	GetModuleFileName( NULL, app_exec_path, MAX_PATH );
	std::wstring string_path( app_exec_path );
	size_t slash_index = string_path.rfind( '\\' );
	std::wstring app_icon_path = string_path.substr( 0, slash_index ) + LOGO_APP_IMAGE;

	// 获取窗口句柄
	wchar_t title[255];
	GetConsoleTitle( title, 255 );
	HWND h_wnd = FindWindow( L"ConsoleWindowClass", title );
	void* h_out = GetStdHandle( STD_OUTPUT_HANDLE );

	// 设置大小位置
	// 如果行数超过屏幕高度而出现纵向滚动条，则宽度必须 <= 80 才不会出现横向滚动条
	// 也就是一旦窗口宽度或高度超过屏幕宽度或高度，窗口就会被自动调整为 { 80, 25 } 且带滚动条的形式
	COORD wnd_size = { 80, 100 };
	SetConsoleScreenBufferSize( h_out, wnd_size );
	SMALL_RECT wnd_rect = { 0, 0, 80 - 1, 100 - 1 };
	SetConsoleWindowInfo( h_out, TRUE, &wnd_rect );
	// ShowWindow( h_wnd, g_wm_taskbar_created );

	// 更改窗体图标
	HICON h_icon = (HICON)LoadImage( NULL, app_icon_path.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE ); // 从 ico 文件读取图标
	SendMessage( h_wnd, WM_SETICON, ICON_SMALL, (LPARAM)h_icon ); // 设置窗体图标

	// 更改窗体标题
	std::string console_title;
	FormatLibrary::StandardLibrary::FormatTo( console_title, "{0} {1}", DEF_APP_NAME, DEF_APP_VERSION );
	int32_t number = MultiByteToWideChar( 0, 0, console_title.c_str(), -1, NULL, 0 );
	wchar_t* temp_console_title = new wchar_t[number];
	MultiByteToWideChar( 0, 0, console_title.c_str(), -1, temp_console_title, number );
	SetConsoleTitle( temp_console_title ); // 修改 Console 窗口标题
	delete[] temp_console_title;
#endif
}

bool SystemInitialize() {
	std::string log_info;
	std::string log_cate = "<SYSTEM_INIT>";
	basicx::SysLog_S* syslog = basicx::SysLog_S::GetInstance();

	syslog->LogWrite( basicx::syslog_level::c_info, log_cate, std::string( "开始系统初始化 ...\r\n" ) );
	syslog->LogPrint( basicx::syslog_level::c_info, log_cate, std::string( "LOG>: 开始系统初始化 ...." ) );

	try {
		// TODO：添加更多初始化任务
	} // try
	catch( ... ) {
		syslog->LogWrite( basicx::syslog_level::c_fatal, log_cate, std::string( "系统初始化时发生未知错误！\r\n" ) );
	}

	syslog->LogWrite( basicx::syslog_level::c_info, log_cate, std::string( "系统初始化完成。\r\n" ) );
	syslog->LogPrint( basicx::syslog_level::c_info, log_cate, std::string( "LOG>: 系统初始化完成。" ) );

	return true;
}

int main( int argc, char* argv[] ) {
	std::string log_info;
	std::string log_cate = "<SYSTEM_MAIN>";
	basicx::SysLog_S syslog_s( DEF_APP_NAME ); // 唯一实例 // 01
	basicx::SysLog_S* syslog = basicx::SysLog_S::GetInstance();
	syslog->SetThreadSafe( false );
	syslog->SetLocalCache( true );
	syslog->SetActiveFlush( false );
	syslog->SetActiveSync( false );
	syslog->SetWorkThreads( 1 );
	syslog->SetFileStreamBuffer( DEF_SYSLOG_FSBM_NONE );
	syslog->InitSysLog( DEF_APP_NAME, DEF_APP_VERSION, DEF_APP_COMPANY, DEF_APP_COPYRIGHT );
	syslog->PrintSysInfo();
	syslog->WriteSysInfo();

	// syslog->ClearScreen( 0, 0, true, 3000 ); // 等待 3 秒清屏

	try {
		ConsoleEventsSet();  // 01
		CheckSingleMutex();  // 02
		SetConsoleWindow();  // 03
		SystemInitialize();  // 04

		while( 1 ) {
			std::this_thread::sleep_for( std::chrono::seconds( 1 ) );

			if( true == g_command_model ) {
				if( false == g_block_auto_info ) {
					g_block_auto_info = true;
					syslog->LogWrite( basicx::syslog_level::c_info, log_cate, std::string( "用户 进入 命令行模式。" ) );
					char user_input[CFG_MAX_PATH_LEN] = { 0 }; // 缓存输入
					syslog->LogPrint( basicx::syslog_level::c_info, std::string( "<USER_COMMAND>" ), std::string( "CMD>: " ) ); // 最好不换行
					gets_s( user_input, CFG_MAX_PATH_LEN );
					if( strlen( user_input ) == 0 ) {
						syslog->LogWrite( basicx::syslog_level::c_info, std::string( "<USER_COMMAND>" ), std::string( "用户输入命令为空。" ) );
						syslog->LogPrint( basicx::syslog_level::c_error, std::string( "<USER_COMMAND>" ), std::string( "ERR>: 命令不能为空！" ) ); // 最好不换行  // 输出 "\nCMD>: " 时换行
					}
					size_t length = strlen( user_input );
					for( size_t i = 0; i < length; i++ ) { // 全部转换为小写字母
						if( user_input[i] >= 'A' && user_input[i] <= 'Z' ) {
							user_input[i] += 'a' - 'A';
						}
					}
					std::string command = user_input;
					FormatLibrary::StandardLibrary::FormatTo( log_info, "用户输入命令：{0}", user_input );
					syslog->LogWrite( basicx::syslog_level::c_info, std::string( "<USER_COMMAND>" ), log_info );

					while( command != "work;" && command != "01" ) { //
						bool need_cout_cmd = true;
						if( command == "info;" || command == "02" ) { // 查看系统信息
							// syskit->SystemInfo();
							syslog->LogPrint( basicx::syslog_level::c_info, std::string( "<USER_COMMAND>" ), std::string( "SystemInfo" ) );
						}
						else if( command == "help;" || command == "03" ) { // 查看系统信息
							// syskit->SystemHelp();
							syslog->LogPrint( basicx::syslog_level::c_info, std::string( "<USER_COMMAND>" ), std::string( "SystemHelp" ) );
						}
						else if( command == "exit;" || command == "04" ) { // 用户退出系统
							// if( syskit->SystemExit() ) {
							if( true ) {
								SystemUninitialize(); // 这里交给主线程退出时清理
								exit( 0 );
							}
						}
						else {
							if( command != "" ) {
								FormatLibrary::StandardLibrary::FormatTo( log_info, "ERR>: 未知命令：{0}", command );
								syslog->LogPrint( basicx::syslog_level::c_error, std::string( "<USER_COMMAND>" ), log_info ); // 最好不换行 // 输出 "\nCMD>: " 时换行
							}
						}

						// user_input[CFG_MAX_PATH_LEN] = { 0 }; // 清空
						if( true == need_cout_cmd ) { // 为 false 的均由线程执行结束后输出 "\nCMD>: "
							syslog->LogPrint( basicx::syslog_level::c_info, std::string( "<USER_COMMAND>" ), std::string( "\nCMD>: " ) ); // 最好不换行
						}
						gets_s( user_input, CFG_MAX_PATH_LEN );
						if( strlen( user_input ) == 0 ) {
							syslog->LogWrite( basicx::syslog_level::c_info, std::string( "<USER_COMMAND>" ), std::string( "用户输入命令为空。" ) );
							syslog->LogPrint( basicx::syslog_level::c_error, std::string( "<USER_COMMAND>" ), std::string( "ERR>: 命令不能为空！" ) ); // 最好不换行 // 输出 "\nCMD>: " 时换行
						}
						size_t length = strlen( user_input );
						for( size_t i = 0; i < length; i++ ) { // 全部转换为小写字母
							if( user_input[i] >= 'A' && user_input[i] <= 'Z' ) {
								user_input[i] += 'a' - 'A';
							}
						}
						command = user_input;
						FormatLibrary::StandardLibrary::FormatTo( log_info, "用户输入命令：{0}", user_input );
						syslog->LogWrite( basicx::syslog_level::c_info, std::string( "<USER_COMMAND>" ), log_info );
					}
				}

				if( true == g_block_auto_info ) {
					g_block_auto_info = false;
					syslog->LogWrite( basicx::syslog_level::c_info, log_cate, std::string( "用户 退出 命令行模式。" ) );
					g_command_model = false;
					syslog->ClearScreen( 0, 0, true ); // 清屏
				}
			}
		} // while( 1 )
	} // try
	catch( ... ) {
		syslog->LogWrite( basicx::syslog_level::c_fatal, log_cate, std::string( "系统主线程发生未知错误！" ) );
	}

	syslog->LogWrite( basicx::syslog_level::c_warn, log_cate, std::string( "系统主线程退出！" ) );

	return 0;
}

namespace clearx {

} // namespace clearx
