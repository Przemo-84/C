#include "sql_module.h"
#include "string.h"
#include "stdio.h"
#include <chrono>


using namespace std::chrono;

sql_module::sql_module(char* server_ip_, char* dataBase_, char* user_, char* password_, int port_)
{
	strcpy(server_ip, server_ip_);
	strcpy(dataBase, dataBase_);
	strcpy(user, user_);
	strcpy(password, password_);
	port = port_;

}

sql_module::~sql_module()
{

}

int sql_module::initConnection()
{
	SQLRETURN retcode;


	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	if (retcode != SQL_SUCCESS)
		return 1;	

	retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);
	if (retcode != SQL_SUCCESS)
		return 1;

	/*retcode = SQLSetEnvAttr(henv, SQL_ATTR_CONNECTION_POOLING, (SQLPOINTER)SQL_CP_ONE_PER_DRIVER, SQL_IS_INTEGER);
	if (retcode != SQL_SUCCESS)
		return 1;*/
	
	memset(connectionString, 0, sizeof(connectionString));

	memset(connStr_out, 0, OUT_CONNECTION_STRING_BUFFER_SIZE);


	sprintf(connectionString, "Driver={PostgreSQL UNICODE};Server=%s;Port=%d;Database=%s;Uid=%s;Pwd=%s;", server_ip, port, dataBase, user, password);

	for (int i = 0; i < CONNECTION_POLL_COUNT; i++)
	{
		SQLHDBC hdbc = SQL_NULL_HDBC;

		retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

		if (retcode != SQL_SUCCESS)
		{
			disconnect();

			return 2;
		}

		retcode = SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)10, 0);

		retcode = SQLDriverConnect(hdbc, NULL, (SQLCHAR*)connectionString, strlen(connectionString), (SQLCHAR*)connStr_out, OUT_CONNECTION_STRING_BUFFER_SIZE, NULL, SQL_DRIVER_COMPLETE);

		if (retcode != SQL_SUCCESS)
		{
			disconnect();

			return 3;
		}
		connectionPool.push(hdbc);
	}

	return 0;

}

int sql_module::disconnect()
{
	if (hstmt != SQL_NULL_HSTMT)
	{
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		hstmt = SQL_NULL_HSTMT;
	}

	while (!connectionPool.empty())
	{
		SQLHDBC hdbc = connectionPool.front();
		if (hdbc != SQL_NULL_HDBC)
		{
			SQLDisconnect(hdbc);
			SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			hdbc = SQL_NULL_HDBC;
		}
		connectionPool.pop();
	}

	if (henv != SQL_NULL_HENV)
	{
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
		henv = SQL_NULL_HENV;
	}

	printf("\033[22;36m");

	printf("\n\nPołączenie z bazą danych workin7_db zostało zakończone....\n\n");

	printf("\033[22;0m");

	return 0;
}

int sql_module::exampleFunction()
{
	SQLHSTMT hstmt = SQL_NULL_HDBC;
	SQLRETURN retcode;
	int retVal = 0;

	

	unique_lock<mutex> ul(poolMutex);

	while (connectionPool.empty())
		cv.wait(ul);

	SQLHDBC hdbc_ = connectionPool.front();
	connectionPool.pop();

	ul.unlock();

	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc_, &hstmt);
	if (retcode != SQL_SUCCESS)
		retVal = 1;
	else
	{
		// execute sql functions like (SQLPraepare, SQLExecute, SQLExecDirect....)using hstmt handle.......


		retVal = 0;

		if (hstmt != SQL_NULL_HDBC)
			SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	}

	connectionPool.push(hdbc_);

	cv.notify_one();

}
