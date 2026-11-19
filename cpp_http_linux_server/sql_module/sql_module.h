#pragma once

#include <queue>
#include <mutex>
#include <sql.h>
#include <sqlext.h>

#include <condition_variable>

#ifndef SQL_MODULE_FUNCTONS

#define	SQL_MODULE_FUNCTIONS

#endif

using namespace std;

const unsigned int OUT_CONNECTION_STRING_BUFFER_SIZE = 2000;

#ifndef JSON_CONVERT_FUNCS

	#include "../jsonConvert/jsonConv.h"

#endif		




struct ConnectionPoolStruct
{
	SQLHDBC hdbc;
};


class sql_module
{
	private:

		const unsigned int CONNECTION_POLL_COUNT = 8;

		SQLHENV henv = SQL_NULL_HENV;

		condition_variable cv;

		mutex poolMutex;
		
		queue <SQLHDBC> connectionPool;

		bool checkQueuePool() { return connectionPool.empty(); };
		
		SQLHSTMT hstmt = SQL_NULL_HSTMT ;

		char connStr_out[OUT_CONNECTION_STRING_BUFFER_SIZE];

		char server_ip[25];
		char dataBase[25];
		char user[25];
		char password[25];
		int port;

		char connectionString[250];

	protected:

	public:

		sql_module(char * server_ip_, char * dataBase_, char * user_, char * password_, int port_);

		~sql_module();

		int initConnection();

		int disconnect();

		//employer functions

		int exampleFunction();
	
};
