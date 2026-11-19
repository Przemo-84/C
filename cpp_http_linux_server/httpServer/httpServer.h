#pragma once

#include <stdlib.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstdlib>
#include <bits/stdc++.h>
#include <queue>
#include <thread>
#include <algorithm>
#include <mutex>
#include <pthread.h>
#include <cstddef>



#ifndef HTTP_SERVER_CLSASS

	#define HTTP_SERVER_CLSASS

#endif


#define MAX_REQ_RESP_BUFFER 15000

#define MAX_HEADER_KEY_SIZE 250

#define MAX_HEADER_VALUE_SIZE 1000

#define MAX_REQUEST_HEADER 200

#define MAX_PARAM_NAME_SIZE 50

#define MAX_PARAM_VALUE_SIZE 1000

#define MAX_REQUEST_PARAMS 200

#define MAX_COOKIE_NAME_SIZE 50

#define MAX_COOKIE_VALUE_SIZE 200

#define MAX_RESPONSE_MESSAGE_SIZE 150


#define NULL_BODY 0
#define TEXT_PLAIN 1
#define TEXT_HTML 2
#define APPLICATION_JSON 3



using namespace std;

struct socket_request
{
	int socket;
	int recvLen;
	char buffer[MAX_REQ_RESP_BUFFER];
	sockaddr_in sAddr;
};

struct request_cookie
{
	char cookie_name[MAX_COOKIE_NAME_SIZE];
	char cookie_value[MAX_COOKIE_VALUE_SIZE];
};

struct req_resp_header
{
	char header_key[MAX_HEADER_KEY_SIZE];
	char header_value[MAX_HEADER_VALUE_SIZE];
};

struct url_params
{
	char param_name[MAX_HEADER_KEY_SIZE];
	char param_value[MAX_HEADER_VALUE_SIZE];
};


struct request
{
	char method[50];
	char path[250];
	char protocol[50];
	char host[50];
	int requestHeaders_count;
	int requestParams_count;
	int max_requestHeaders_count;
	int max_requestParams_count;
	req_resp_header * headers;
	url_params * params;
	int bodyBuffer_size;
	char * bodyBuffer;
};

struct json_body
{
	char json_key[MAX_HEADER_KEY_SIZE];
	char json_value[MAX_HEADER_VALUE_SIZE];
};

struct response
{
	char protocol[25]="HTTP/1.1\0";
	int status = 200;
	char message[MAX_RESPONSE_MESSAGE_SIZE] = "OK\0";
	int contentType = 0; // 0 null body; 1 - text/plain; 2 - text/html; 3 - applicatiuon/json
	vector<req_resp_header> responseHeaders;
	queue<json_body> jsonBody;
	int bodyBuffer_size = 0;
	char * bodyBuffer = NULL;
};

enum status_codes { REQUEST_STATUS_SUCCESS, REQUEST_STATUS_FAILED, REQUEST_STATUS_PENDING, REQUEST_STATUS_FORBIDDEN};

struct rooting_callback_status
{
	status_codes status;
	int returnValue;
	char allow_origin[MAX_HEADER_VALUE_SIZE];
	bool allow_credentials = false;
};

typedef void * PENDING_PROC(void* args);

struct processRequest_pendingArgs
{
	sockaddr_in sa2;
	void * httpServer_;
	void * sqlmodule;
	int s2;
	void * jsonOb;
	void * cookies;

	PENDING_PROC * pendingProc;
};

struct requestThread
{
	socket_request sr;
	void * httpServer;
};

struct pendingRequestThread
{
	pthread_t thread_id;
	int s2;
	sockaddr_in sa2;
};

typedef  rooting_callback_status (*rooting_functionPointer)(int s2, sockaddr_in sa2, request * req, pthread_t currentThread);

struct rootingCallback
{
	char method[50];
	char path[250];
	rooting_functionPointer rfp;
};

class http_server
{

private:

public:

	queue<socket_request> socketRequests;

	bool is_init = false;

	thread * listen_thread = NULL;
	pthread_t  * mainApp_thread;

	int requestThreads_count = 0;

	vector <pthread_t> processRequest_thread;

	mutex mutex_, requestMutex;

	sockaddr_in sa1;
	int s1 = 0;
	
	void claerThreads();

	http_server();

	~http_server();

	int init_server(int port);


};


int listenSocket(int s1, http_server * httpServer);

void * processRequest(void * args);


int requestRoute(int s2, sockaddr_in sa2, request* req, pthread_t currentThread);


int requestParse(char * buffer, int bufferLen, request * req);
int responseParse(char* buffer, int bufferLen, response* req);

int clearRequestPointers(request * req);
int clearResponsePointers(response * resp);

bool getRequestHeader(req_resp_header * in_requestHeaders, int in_headersSize, req_resp_header * out_requestHeader, char * keyName, int keyName_size);
bool getResponseHeader(vector<req_resp_header> * in_requestHeaders, req_resp_header * out_requestHeader, char* keyName, int keyName_size);

int parseCookie(char * buffer, int bufferLen, vector<request_cookie> * req_cookies);
bool getCookie(vector<request_cookie>* req_cookies, char* cookieName, char* returnValue, int returnValue_size);
int clearCookies(vector<request_cookie>* cookies);


int setResponseStatus(response * resp, int statusCode);
int setResponseMessage(response * resp, char * message, int messageSize);
int setResponseContentType(response * resp, int contentType);
int addResponseBuffer(response * resp, char * buffer, int bufferSize);
int addResponseHeader_item(response* resp, char * name , int nameSize, char * value, int valueSize);
int addResponseJSON_item(response * resp, char * key, int keySize, char * value, int valueSize);

int sendResponse(int s2, sockaddr_in sa2, response * resp);

int sendRequest(int s1, sockaddr_in sa1, request* req);

