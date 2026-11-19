#pragma once


#ifndef ROOTER_FUNCS

	#define RUTER_FUNCS

#endif

#ifndef JSON_CONVERT_FUNCS

#include"../jsonConvert/jsonConv.h"

#endif

#ifndef DATA_CONECTIONS

#include"../initDataEnv/initDataEnv.h"

#endif

#ifndef SQL_MODULE_FUNCTIONS

#include "../sql_module/sql_module.h"

#endif

#ifndef HTTP_SERVER_CLSASS

#include "httpServer.h"

#endif


#include "../headers/mainApp.h"


int routerFunc_setDataPointers(dataConnections * dataC, http_server * server, sql_module * sqlm);


// Method GET

rooting_callback_status send_serverStatus(int s2, sockaddr_in sa2, request * req, pthread_t currentThread);

rooting_callback_status send_json(int s2, sockaddr_in sa2, request* req);


// Method POST



// Not found route endpoint all methods


rooting_callback_status send_serverError(int s2, sockaddr_in sa2, request * req, int code, char* message, int messageSize, char * allow_origin = "*", bool allow_credentials = false);


// Method Options


