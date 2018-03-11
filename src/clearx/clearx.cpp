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

#include <stdlib.h>
#include <iostream>

#include <syslog/syslog.h>
#include <sysdbi_m/sysdbi_m.h>

#include "global/define.h"
#include "clearx.h"

int main( int argc, char* argv[] ) {
	basicx::SysLog_S g_syslog_s( "ClearX" );
	basicx::SysLog_S* syslog_s = basicx::SysLog_S::GetInstance();
	syslog_s->SetThreadSafe( false );
	syslog_s->SetLocalCache( true );
	syslog_s->SetActiveFlush( false );
	syslog_s->SetActiveSync( false );
	syslog_s->SetWorkThreads( 1 );
	//syslog_s->SetFileStreamBuffer( DEF_SYSLOG_FSBM_NONE ); // 会影响性能，但在静态链接 MySQL、MariaDB 时需要设为 无缓冲 不然写入文件的日志会被缓存
	syslog_s->InitSysLog( DEF_APP_NAME, DEF_APP_VERSION, DEF_APP_COMPANY, DEF_APP_COPYRIGHT ); // 设置好参数后
	syslog_s->PrintSysInfo();
	syslog_s->WriteSysInfo();
	
	basicx::SysDBI_M g_sysdbi_m;
	basicx::SysDBI_M* sysdbi_m = basicx::SysDBI_M::GetInstance();
	int32_t connect_count = sysdbi_m->AddConnect( 1, "10.0.7.53", 3306, "user", "user", "financial", "utf8" );
	if( connect_count > 0 ) {
		MYSQL* connection = nullptr;
		connection = sysdbi_m->GetConnect( "10.0.7.53", "financial", "utf8" );
		if( connection != nullptr ) {
			std::string query_error;
			std::string sql_query = "SELECT* FROM trading_day WHERE natural_date >= '2018-1-1' AND natural_date <= '2018-1-31'";
			if( MYSQL_RES* result = sysdbi_m->Query_R( connection, sql_query, query_error ) ) {
				uint32_t num_fields = mysql_num_fields( result );
				MYSQL_FIELD* fields = mysql_fetch_fields( result );
				for( size_t i = 0; i < num_fields; i++ ) {
					std::cout << fields[i].name << "\t";
				}
				std::cout << std::endl;
				uint64_t num_rows = mysql_num_rows( result );
				while( MYSQL_ROW row = mysql_fetch_row( result ) ) {
					unsigned long* lengths = mysql_fetch_lengths( result );
					for( size_t i = 0; i < num_fields; i++ ) {
						std::cout << row[i] << "\t";
					}
					std::cout << std::endl;
				}
				std::cout << "共计 " << num_rows << " 条。" << std::endl;
				mysql_free_result( result );
			}
			else {
				std::cout << "查询错误：" << query_error << std::endl;
			}
			sysdbi_m->ReturnConnect( "10.0.7.53", connection );
		}
		//connection = sysdbi_m->GetConnect( "10.0.7.53", "financial", "utf8" );
		//if( connection != nullptr ) {
		//	std::string query_error;
		//	std::string sql_query = "DELETE FROM trading_day WHERE week_end = 1";
		//	if( bool result = sysdbi_m->Query_E( connection, sql_query, query_error ) ) {
		//		std::cout << "删除完成。" << std::endl;
		//	}
		//	else {
		//		std::cout << "执行错误：" << query_error << std::endl;
		//	}
		//	sysdbi_m->ReturnConnect( "10.0.7.53", connection );
		//}
	}
	
	system( "pause" );
	return 0;
}

namespace clearx {

} // namespace clearx
