

#include "routerFunc.h"

#include <sys/types.h>
#include <sys/stat.h>
//#include <stdio.h>
//#include <stdlib.h>
#include <dirent.h>
#include <filesystem>
#include <curl/curl.h>

//#ifndef JSON_CONVERT_FUNCS
//
//#include"../jsonConvert/jsonConv.h"
//
//#endif


using namespace std;

//#ifndef HTTP_SERVER_CLSASS
//
//#include "httpServer.h"
//
//#endif

dataConnections * dataConn = NULL;

http_server * httpServer_ = NULL;

sql_module * sqlmodule = NULL;


// Method Options End oints6


int routerFunc_setDataPointers(dataConnections * dataC, http_server * server, sql_module * sqlm)
{
	if (dataC != NULL)
		dataConn = dataC;


	if (server != NULL)
		httpServer_ = server;

	if (sqlm != NULL)
		sqlmodule = sqlm;
	
	return 0;
}


rooting_callback_status send_json(int s2, sockaddr_in sa2, request * req)
{
	response resp;

	rooting_callback_status rcs;

	req_resp_header rh[] = {
		{"Access-Control-Allow-Origin", "*"},
		{"Access-Control-Allow-Methods", "*"},
		{"Access-Control-Allow-Headers", "*"},
		{"Connection", "keep-alive"},
		{"Keep-Alive", "timeout=5"},
	};

	memset(&rcs, 0, sizeof(rooting_callback_status));

	rcs.status = REQUEST_STATUS_SUCCESS;
	rcs.returnValue = 0;

	int rhSize = sizeof(rh) / sizeof(req_resp_header);
	//memset(&resp, 0, sizeof(resp));

	for (int i = 0; i < rhSize; i++)
		addResponseHeader_item(&resp, rh[i].header_key, strlen(rh[i].header_key), rh[i].header_value, strlen(rh[i].header_value));

	setResponseContentType(&resp, APPLICATION_JSON);
	addResponseJSON_item(&resp, "key1", 4, "value1", 6);
	addResponseJSON_item(&resp, "key2", 4, "value2", 6);
	addResponseJSON_item(&resp, "key3", 4, "value3", 6);

	int ret = sendResponse(s2, sa2, &resp);

	rcs.returnValue = ret;

	if (ret <= 0)
		rcs.status = REQUEST_STATUS_FAILED;

	return rcs;
}

rooting_callback_status send_serverStatus(int s2, sockaddr_in sa2, request * req, pthread_t currentThread)
{
	response resp;

	rooting_callback_status rcs;

	memset(&rcs, 0, sizeof(rooting_callback_status));

	rcs.status = REQUEST_STATUS_SUCCESS;
	rcs.returnValue = 0;

	req_resp_header rh[] = {
		{"Access-Control-Allow-Origin", "*"},
		{"Access-Control-Allow-Methods", "*"},
		{"Access-Control-Allow-Headers", "*"},
		{"Connection", "keep-alive"},
		{"Keep-Alive", "timeout=5"},
	};
	
	char htmlBody[1500];

	memset(htmlBody, 0, 1500);

	sprintf(htmlBody, "<HTML>\r\n\<HEAD>\r\n<TITLE>\r\nHttp Linux Server\r\n</TITLE><meta name=\"viewport\" content=\"width=device-width, height=device-height, initial-scale=1.0\">\r\n</HEAD>\r\n<BODY>\r\n<br/><span style=\"position : relative; display: block; left: 20%; width: 70%; text-align:left; font-family: \'Tahoma\'; font-weight: bold; font-size:24px; letter-spacing:1px; word-spacing: 2px\">\r\nWelcome to Http linux Server!\r\n</span><br/><br/><span style=\" position: relative; display: block; left: 20%; width: 70%; text-align:left; font-family: \'Tahoma\'; font-size: 18px; letter-spacing:1px; word-spacing: 3px\">If you see this page, the Http Linux server is successfully installed and working. Further configuration is required.<br/><br/>\r\n</span></BODY>\r\n\</HTML>\r\n");

	int rhSize = sizeof(rh) / sizeof(req_resp_header);
	//memset(&resp, 0, sizeof(resp));

	for(int i = 0; i < rhSize; i++)
		addResponseHeader_item(&resp, rh[i].header_key, strlen(rh[i].header_key), rh[i].header_value, strlen(rh[i].header_value));

	setResponseContentType(&resp, TEXT_HTML);
	addResponseBuffer(&resp, htmlBody, strlen(htmlBody));

	int ret = sendResponse(s2, sa2, &resp);

	rcs.returnValue = ret;
	
	if (ret <= 0)
		rcs.status = REQUEST_STATUS_FAILED;

	return rcs;
}

rooting_callback_status send_serverError(int s2, sockaddr_in sa2, request* req, int code, char* message, int messageSize, char * allow_origin, bool allow_credentials)
{

	response resp;

	rooting_callback_status rcs;

	memset(&rcs, 0, sizeof(rooting_callback_status));

	rcs.status = REQUEST_STATUS_SUCCESS;
	rcs.returnValue = 0;

	req_resp_header rh[] = {
		{"Access-Control-Allow-Origin", ""},
		{"Access-Control-Allow-Credentials", ""},
		{"Access-Control-Allow-Methods", "*"},
		{"Access-Control-Allow-Headers", "*"},
		{"Connection", "keep-alive"},
		{"Keep-Alive", "timeout=5"},
	};

	int rhSize = sizeof(rh) / sizeof(req_resp_header);
	//memset(&resp, 0, sizeof(resp));

	if (allow_origin != NULL && strlen(allow_origin) < MAX_HEADER_VALUE_SIZE && strlen(allow_origin) > 0)
		strcpy(rh[0].header_value, allow_origin);
	else
		strcpy(rh[0].header_value, "*");

	if (allow_credentials)
		strcpy(rh[1].header_value, "true");
	else 
		strcpy(rh[1].header_value, "false");

	for (int i = 0; i < rhSize; i++)
		addResponseHeader_item(&resp, rh[i].header_key, strlen(rh[i].header_key), rh[i].header_value, strlen(rh[i].header_value));

	setResponseStatus(&resp, code);
	setResponseMessage(&resp, message, messageSize);
	setResponseContentType(&resp, TEXT_HTML);

	char htmlBody[1500];

	memset(htmlBody, 0, 1500);

	sprintf(htmlBody, "<HTML>\r\n\<HEAD>\r\n<TITLE>\r\Http linux server\r\n</TITLE><meta name=\"viewport\" content=\"width=device-width, height=device-height, initial-scale=1.0\">\r\n</HEAD>\r\n<BODY>\r\n<br/><span style=\"position : relative; display: block; left: 20%; width: 60%; text-align:center; font-family: \'Tahoma\'; font-weight: bold; font-size:24px; letter-spacing:1px; word-spacing: 2px\">\r\nWelcome to Http Linux Server!\r\n</span><br/><br/><span style=\" position: relative; display: block; left: 20%; width: 60%; text-align:center; font-family: \'Tahoma\'; font-size: 18px; letter-spacing:1px; word-spacing: 3px\">Error Code: %d %s<br/><br/>\r\n</span></BODY>\r\n\</HTML>\r\n", code, message);

	addResponseBuffer(&resp, htmlBody, strlen(htmlBody));

	int ret = sendResponse(s2, sa2, &resp);

	rcs.returnValue = ret;

	if (ret <= 0)
		rcs.status = REQUEST_STATUS_FAILED;

	return rcs;
}






