#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include "httpServer.h"
#include "../headers/mainApp.h"


#ifndef JSON_CONVERT_FUNCS

    #include"../jsonConvert/jsonConv.h"

#endif


#ifndef ROOTER_FUNCS

    #include "routerFunc.h"

#endif

const int mimeTypesCount = 3;
const int httpProtocolsCount = 4;
const int methodsCount = 9;

char * methods_[methodsCount] = {"GET", "POST", "PUT", "OPTIONS", "HEAD", "TRACE", "DELETE", "PATH", "CONNECT"};
char * httpProtocols[httpProtocolsCount] = { "HTTP/1.0", "HTTP/1.1", "HTTP/2.0", "HTTP/3.0" };
char * mimeTypes[mimeTypesCount] = { "text/plain", "text/html", "application/json" };

rootingCallback rootingCallbackFunctions[] = {
	{"GET", "/",(rooting_functionPointer)&send_serverStatus},
    {"GET", "/getJson",(rooting_functionPointer)&send_json},
};

int rootingCallbackFunctions_size = sizeof(rootingCallbackFunctions) / sizeof(rootingCallback);

http_server::http_server()
{
    printf("\033[22;36m");

	printf("Http_server utworzono.....\n\n");

    printf("\033[22;0m");

}

http_server::~http_server()
{
    printf("\033[22;36m");

	printf("\n\nHttp_server dezaktywowano.....\n\n");

    printf("\033[22;0m");

}

int http_server::init_server(int port)
{
    memset(&sa1, 0, sizeof(sa1));

    s1 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sa1.sin_family = AF_INET;
    sa1.sin_port = htons(port);
    sa1.sin_addr.s_addr = inet_addr("127.0.0.1");
   /* sa1.sin_addr.s_addr = htonl(INADDR_ANY);*/

    bind(s1, (sockaddr*)&sa1, sizeof(sa1));
    listen(s1, SOMAXCONN);

    listen_thread = new thread(listenSocket, s1, this);

    is_init = true;
    return 0;
}

int listenSocket(int s1_, http_server * httpServer)
{
    sockaddr_in sa2;
    int s2=0;
    int recvLen = 0;
    unsigned int len = 0;
    pthread_t pt;
    //timespec sleepTime = { 0,100000000 }; // 10 request per 1 scond limit
    useconds_t usec = 1000;

    char buffer[MAX_REQ_RESP_BUFFER];

    while (1)
    {
        memset(&sa2, 0, sizeof(sa2));

        len = sizeof(sa2);

       //nanosleep(&sleepTime, NULL);
        usleep(usec);

        s2 = accept(s1_, (sockaddr*)&sa2, &len);
        if (s2 > 0)
        {
            if (!httpServer->is_init)
            {
                close(s2);
                break;
            }

            memset(buffer, 0, MAX_REQ_RESP_BUFFER);

            recvLen = recv(s2, buffer, MAX_REQ_RESP_BUFFER, 0);
            if (recvLen > 0)
            {
                requestThread * rT = (requestThread*)calloc(1,sizeof(requestThread));
                rT->sr.recvLen = recvLen;
                rT->sr.socket = s2;
                memcpy(&rT->sr.sAddr, &sa2, sizeof(sa2));
                memcpy(rT->sr.buffer, buffer, recvLen);
                rT->httpServer = httpServer;

                pthread_create(&pt, NULL, processRequest, (void*)rT);
                
            }
        }
        else break;
    }



    return 0;
}


void * processRequest(void * args)
{
    requestThread * rT = (requestThread*)args;
    http_server * httpS = (http_server*)rT->httpServer;
    pthread_t currentThread = pthread_self();
    request * req = NULL;
    int ret = 0;
    int i = 0;
    int licznik = 0;
    char * host = NULL;

    lock_guard<mutex> * processReqMutex = new lock_guard<mutex>(httpS->requestMutex);

   /* httpS->requestMutex.lock();*/
    httpS->processRequest_thread.push_back(currentThread);
    httpS->requestThreads_count++;

  /*  httpS->requestMutex.unlock();*/

    delete (processReqMutex);
 
    req = (request *) calloc(1,sizeof(request));
  
    req->bodyBuffer = NULL;
    req->headers = NULL;
    req->params = NULL;

    /*  printf("\033[22;033m");
    printf("\n\n");
    printf("%s", rT->sr.buffer);
    printf("\033[22;0m");
    printf("\n\n");
    cout.flush();*/

    req->max_requestHeaders_count = MAX_REQUEST_HEADER;

    req->max_requestParams_count = MAX_REQUEST_PARAMS;

    ret = requestParse(rT->sr.buffer, rT->sr.recvLen, req);

    if (ret != 0)
    {
        send_serverError(rT->sr.socket, rT->sr.sAddr, req, 500, "Bad Getway", 10);

        clearRequestPointers(req);

        /*delete(req);*/

        free(req);
        req = NULL;

        close(rT->sr.socket);

    }
    else 
    {
        host = inet_ntoa(rT->sr.sAddr.sin_addr);
        strcpy(req->host, host);

        requestRoute(rT->sr.socket, rT->sr.sAddr, req, currentThread);

    }
 
    free(args);
    args = NULL;

    processReqMutex = new lock_guard<mutex>(httpS->requestMutex);

    //httpS->requestMutex.lock();

    for (int i = 0; i < httpS->processRequest_thread.size(); i++)
    {
        if (httpS->processRequest_thread[i] == currentThread)
        {
            httpS->processRequest_thread.erase(httpS->processRequest_thread.begin() + i);
            httpS->requestThreads_count--;
            break;
        }
    }

    //httpS->requestMutex.unlock();

    delete(processReqMutex);

    pthread_detach(currentThread);

    return NULL;
}

int requestParse(char * buffer, int bufferLen, request * req)
{
    int pos = 0;
    int i,j;
    int is_badMethod = true;
    int is_badProtocol = true;
    int headersCount = req->max_requestHeaders_count;
    int paramsCount = req->max_requestParams_count;


    if (req->max_requestHeaders_count <= 0 || req->max_requestParams_count <= 0)
        return -1;

    if (req->headers != NULL)
        free(req->headers);

    if (req->params != NULL)
        free(req->params);

    memset(req, 0, sizeof(request));

    req->max_requestHeaders_count = headersCount;
    headersCount = 0;

    req->max_requestParams_count = paramsCount;
    paramsCount = 0;
    
    req->headers = (req_resp_header*)malloc(sizeof(req_resp_header) * req->max_requestHeaders_count);

    memset(req->headers, 0, sizeof(req_resp_header) * req->max_requestHeaders_count);

    req->params = (url_params*)malloc(sizeof(url_params) * req->max_requestParams_count);

    memset(req->params, 0, sizeof(url_params) * req->max_requestParams_count);

    //-----------------method  parsing-------------------

    for (i = 0; i < 50; i++)
    {
        if (buffer[i] == ' ')
            break;

        if (buffer[i] > 65 && buffer[i] <= 90)
        {
            req->method[i] = buffer[i];

            pos++;

            if (pos + 1 > bufferLen)
                return 1;
        }
        else
            return 2;
    }

    /*  if (buffer[pos] != ' ')
          return 2;*/

    for (i = 0; i < methodsCount; i++)
    {
        if (strcmp(methods_[i], req->method) == 0)
        {
            is_badMethod = false;
            break;
        }

    }

    if (is_badMethod)
        return 3;

    while (buffer[pos] == ' ')
    {
        pos++;

        if (pos + 1 > bufferLen)
            return 4;
    }

    if (buffer[pos] != '/')
        return 5;


    //-----------------url  parsing-------------------



    for (i = 0; i < 250; i++)
    {
        pos++;

        if (pos + 1 > bufferLen)
            return 6;

        if (buffer[pos] == ' ' || buffer[pos] == '?')
            break;

        if (buffer[pos] == '\r' || buffer[pos] == '\n')
            return 7;

        req->path[i + 1] = buffer[pos];
    }
    req->path[0] = '/';


    //-----------------url params  parsing-------------------

    if (buffer[pos] == '?')
    {
        do
        {
            for (i = 0; i < MAX_PARAM_NAME_SIZE; i++)
            {
                pos++;

                if (pos + 1 > bufferLen)
                    return 8;

                if (buffer[pos] == '=')
                {
                    if (i == 0)
                        return 9;
                    else
                        break;
                }

                if ((buffer[pos] >= 65 && buffer[pos] <= 90) || (buffer[pos] >= 97 && buffer[pos] <= 122) || (buffer[pos] >= 48 && buffer[pos] <= 57))
                    req->params[paramsCount].param_name[i] = buffer[pos];
                else if (buffer[pos] == '-' || buffer[pos] == '_' || buffer[pos] == '%')
                    req->params[paramsCount].param_name[i] = buffer[pos];
                else
                    return 10;
            }

            if (buffer[pos] != '=')
                return 11;

            for (i = 0; i < MAX_PARAM_VALUE_SIZE; i++)
            {
                pos++;

                if (pos + 1 > bufferLen)
                    return 12;

                if (buffer[pos] == 38 || buffer[pos] == 32)
                {
                    if (i == 0)
                        return 13;
                    else
                        break;
                }

                req->params[paramsCount].param_value[i] = buffer[pos];
            }

            paramsCount++;

            req->requestParams_count = paramsCount;

   

        }while (buffer[pos] != ' ');
    }

    while (buffer[pos] == ' ')
    {
        pos++;

        if (pos + 1 > bufferLen)
            return 9;
    }


    //-----------------protocol  parsing-------------------

    if (buffer[pos] != 'H')
        return 10;

    req->protocol[0] = 'H';



    for (i = 1; i < 50; i++)
    {
        pos++;

        if (pos + 1 > bufferLen)
            return 11;

        if (buffer[pos] == ' ')
            return 12;

        if (buffer[pos] == '\n')
        {
            if (pos + 1 > bufferLen)
                return 13;
            if (buffer[pos - 1] != '\r')
                return 14;
            else
                break;
        }
        else if (buffer[pos] != '\r')
            req->protocol[i] = buffer[pos];
    }

    for (i = 0; i < httpProtocolsCount; i++)
    {
        if (strcmp(req->protocol, httpProtocols[i]) == 0)
        {
            is_badProtocol = false;
            break;
        }
    }
    
    if(is_badProtocol)
        return 15;


    //-----------------headers  parsing-------------------

    pos++;

    while (pos + 1 <= bufferLen)
    {

        if (buffer[pos] == '\n')
        {
            
            if (buffer[pos - 1] != '\r')
                return 16;

            if (pos > 3)
            {
                if (buffer[pos - 3] == '\r' && buffer[pos - 2] == '\n')
                    break;
            }
            pos++;
        }
        else if (buffer[pos] == '\r')
            pos++;
        else
        {
            if (req->max_requestHeaders_count < headersCount + 1)
                return -2;

            for (i = 0; i < MAX_HEADER_KEY_SIZE; i++)
            {
                if ((buffer[pos] >= 65 && buffer[pos] <= 90) || (buffer[pos] >= 97 && buffer[pos] <= 122))
                    req->headers[headersCount].header_key[i] = buffer[pos];
                else if (buffer[pos] == '-')
                {
                    if (i > 0)
                    {
                        req->headers[headersCount].header_key[i] = '-';
                      
                        while (buffer[pos] == '-')
                            pos++;

                        if ((buffer[pos] >= 65 && buffer[pos] <= 90) || (buffer[pos] >= 97 && buffer[pos] <= 122))
                        {
                            if (i >= MAX_HEADER_KEY_SIZE)
                                return 17;

                            req->headers[headersCount].header_key[i+1] = buffer[pos];
                            pos++;
                            i++;
                            continue;
                        }
                        else
                            return 18;

                    }
                    else i--;
                }
                else if (buffer[pos] == ' ')
                {
                    j = 1;

                    while (buffer[pos + j] == ' ')
                        j++;

                    if (buffer[pos + j] != ':')
                        return 19;

                    pos += j;

                    break;
                }
                else if (buffer[pos] == ':')
                {
                    if (i == 0)
                        return 20;
                    
                    break;
                }

                pos++;

                if (pos + 1 > bufferLen)
                    return 21;
            }

            for (i = 0; i < MAX_HEADER_VALUE_SIZE; i++)
            {
                pos++;

                if (buffer[pos] == ' ' && i == 0)
                {
                    while (buffer[pos] == ' ')
                    {
                        pos++;

                        if (pos + 1 > bufferLen)
                            return 22;
                    }
                }

                if (pos + 1 > bufferLen)
                    return 23;

                if (buffer[pos] == '\n')
                {
                    if (buffer[pos - 1] != '\r')
                        return 24;

                    headersCount++;

                    req->requestHeaders_count = headersCount;
                    
                    break;
                }

                if (buffer[pos] != '\r')
                    req->headers[headersCount].header_value[i] = buffer[pos];
            }
        }
    }

    //-----------------body  parsing-------------------

    pos++;

    if (pos  < bufferLen)
    {
        req->bodyBuffer_size = bufferLen - pos;

        req->bodyBuffer = (char*)calloc(1, req->bodyBuffer_size + 2);

        memcpy((void*)req->bodyBuffer, (void*)&buffer[pos], req->bodyBuffer_size);
    }

    return 0;
}

int responseParse(char * buffer, int bufferLen, response * resp)
{
    int i = 0, j = 0, pos = 0;
    char statusCode[3];
    req_resp_header rh;

    char _buffer[2000];
    memset(_buffer, 0, 2000);
    memcpy(_buffer, buffer, bufferLen);

    int retVal = 0;

    if (buffer == NULL || resp ==  NULL || bufferLen <= 0)
        return 1;

    memset(statusCode, 0, 3);

    //protocol parse

    for (i = 0; i < 25; i++)
    {
        if (buffer[pos] == ' ')
            break;
        else if (buffer[pos] == '\r' || buffer[pos] == '\n')
            return 2;

        resp->protocol[i] = buffer[pos];

        pos++;
        if (pos + 1 > bufferLen)
            return 3;

    }

    if (buffer[pos] != ' ')
        return 4;

    for (i = 0; i < 4; i++)
    {
        retVal = 5;

        if (strcmp(resp->protocol, httpProtocols[i]) == 0)
        {
            retVal = 0;
            break; 
        }
    }

    if (retVal != 0)
        return retVal;

    while (buffer[pos] == ' ' || buffer[pos] == '\t')
    {
        pos++;

        if (pos + 1 > bufferLen)
            return 6;
    }
   

    //status code parse

    for (i = 0; i < 3; i++)
    {
       
        if (buffer[pos] < 48 || buffer[pos] > 57)
            return 7;

        statusCode[i] = buffer[pos];

        pos++;

        if (pos + 1 > bufferLen)
            return 8;
    }

    if (buffer[pos] != ' ')
        return 9;

    resp->status = atoi(statusCode);

    while (buffer[pos] == ' ' || buffer[pos] == '\t')
    {
        pos++;

        if (pos + 1 > bufferLen)
            return 10;
    }


    if (buffer[pos] == '\r' || buffer[pos] == '\n')
        return 11;

    i = 0;

    while (buffer[pos] != '\r')
    { 
        if (buffer[pos] >= 65 && buffer[pos] <= 90)
            resp->message[i] = buffer[pos];
        else if (buffer[pos] >= 97 && buffer[pos] <= 122)
            resp->message[i] = buffer[pos];
        else if (buffer[pos] == '_' || buffer[pos] == '-' || buffer[pos] == ' ')
            resp->message[i] = buffer[pos];
        else
            return 12;

        pos++;
        i++;

        if (pos + 1 > bufferLen)
            return 13;
    }

    if (pos + 2 > bufferLen)
        return 14;
    
    if (buffer[pos + 1] != '\n')
        return 15;

    pos += 2;

    if (pos > bufferLen)
        return 16;

    // headers parse

    while (pos + 1 <= bufferLen)
    {

        if (buffer[pos] == '\n')
        {

            if (buffer[pos - 1] != '\r')
                return 17;

            if (pos > 3)
            {
                if (buffer[pos - 3] == '\r' && buffer[pos - 2] == '\n')
                    break;
            }
            pos++;
        }
        else if (buffer[pos] == '\r')
            pos++;
        else
        {
            memset(&rh, 0, sizeof(req_resp_header));

            for (i = 0; i < MAX_HEADER_KEY_SIZE; i++)
            {
                if ((buffer[pos] >= 65 && buffer[pos] <= 90) || (buffer[pos] >= 97 && buffer[pos] <= 122))
                    rh.header_key[i] = buffer[pos];
                else if (buffer[pos] == '-')
                {
                    if (i > 0)
                    {
                        rh.header_key[i] = '-';

                        while (buffer[pos] == '-')
                            pos++;

                        if ((buffer[pos] >= 65 && buffer[pos] <= 90) || (buffer[pos] >= 97 && buffer[pos] <= 122))
                        {
                            if (i >= MAX_HEADER_KEY_SIZE)
                                return 18;

                            rh.header_key[i + 1] = buffer[pos];
                            pos++;
                            i++;
                            continue;
                        }
                        else
                            return 19;

                    }
                    else i--;
                }
                else if (buffer[pos] == ' ')
                {
                    j = 1;

                    while (buffer[pos + j] == ' ')
                        j++;

                    if (buffer[pos + j] != ':')
                        return 20;

                    pos += j;

                    break;
                }
                else if (buffer[pos] == ':')
                {
                    if (i == 0)
                        return 21;

                    break;
                }

                pos++;

                if (pos + 1 > bufferLen)
                    return 22;
            }

            for (i = 0; i < MAX_HEADER_VALUE_SIZE; i++)
            {
                pos++;

                if (buffer[pos] == ' ' && i == 0)
                {
                    while (buffer[pos] == ' ')
                    {
                        pos++;

                        if (pos + 1 > bufferLen)
                            return 23;
                    }
                }

                if (pos + 1 > bufferLen)
                    return 24;

                if (buffer[pos] == '\n')
                {
                    if (buffer[pos - 1] != '\r')
                        return 25;

                    break;
                }

                if (buffer[pos] != '\r')
                    rh.header_value[i] = buffer[pos];
            }

            if (strcmp(rh.header_key, "Content-Type") == 0)
            {
                for (i = 0; i < mimeTypesCount; i++)
                {
                    if (strncmp(rh.header_value, mimeTypes[i], strlen(mimeTypes[i])) == 0)
                    {
                        resp->contentType = i;
                        break;
                    }
                }
            }
            resp->responseHeaders.push_back(rh);
        }
    }

    //-----------------body  parsing-------------------

    pos++;

    if (pos < bufferLen)
    {
        resp->bodyBuffer_size = bufferLen - pos;

        resp->bodyBuffer = (char*)calloc(1, resp->bodyBuffer_size + 2);

        memcpy((void*)resp->bodyBuffer, (void*)&buffer[pos], resp->bodyBuffer_size);
    }
    
    return 0;
}

int clearRequestPointers(request * req) 
{
    if (req != NULL)
    {
        if (req->bodyBuffer != NULL)
        {
            free(req->bodyBuffer);
            req->bodyBuffer = NULL;
        }

        if (req->params != NULL)
        {
            free(req->params);
            req->params = NULL;
        }

        if (req->headers != NULL)
        {
            free(req->headers);
            req->headers = NULL;
        }
    }
    else
        return 1;

    return 0;
}

int clearResponsePointers(response * resp)
{
    if (resp != NULL)
    {
        while (!resp->jsonBody.empty())
            resp->jsonBody.pop();

        if (!resp->responseHeaders.empty())
            resp->responseHeaders.clear();

        if(resp->bodyBuffer != NULL)
        {
            free((void *)resp->bodyBuffer);
            resp->bodyBuffer_size = 0;
            resp->bodyBuffer = NULL;
        }
    }

    return 0;
}

bool getRequestHeader(req_resp_header* in_requestHeaders, int in_headersSize, req_resp_header* out_requestHeader, char* keyName, int keyName_size) {

    if (in_requestHeaders == NULL || out_requestHeader == NULL || keyName == NULL || keyName_size <= 0 || in_headersSize <= 0)
        return false;

    for (int i = 0; i < in_headersSize; i++)
    {
        if (strncmp(in_requestHeaders[i].header_key, keyName, keyName_size) == 0)
        {
            memset(out_requestHeader,0,  sizeof(req_resp_header));
            strncpy(out_requestHeader->header_key, keyName, keyName_size);
            strncpy(out_requestHeader->header_value, in_requestHeaders[i].header_value, strlen(in_requestHeaders[i].header_value));

            return true;
        }
    }

    return false;
}

bool getResponseHeader(vector<req_resp_header>* in_responseHeaders, req_resp_header * out_requestHeader, char* keyName, int keyName_size)
{
    if (in_responseHeaders == NULL || out_requestHeader == NULL || keyName == NULL || keyName_size <= 0)
        return false;

    for (int i = 0; i < in_responseHeaders->size(); i++)
    {
        if (strncmp(in_responseHeaders->at(i).header_key, keyName, keyName_size) == 0)
        {
            memset(out_requestHeader, 0, sizeof(req_resp_header));
            strncpy(out_requestHeader->header_key, keyName, keyName_size);
            strncpy(out_requestHeader->header_value, in_responseHeaders->at(i).header_value, strlen(in_responseHeaders->at(i).header_value));

            return true;
        }
    }

    return false;
}

int clearCookies(vector<request_cookie> * cookies)
{
    if (cookies == NULL)
        return 1;

    if(!cookies->empty())
        cookies->clear();

    return 0;
}

int parseCookie(char * buffer, int bufferLen, vector<request_cookie> * req_cookies)
{
    int retVal = 0;
    int pos = 0;
    int i = 0;
    request_cookie req_c;
    bool is_next_cookie = false;

    if (buffer == NULL || req_cookies == NULL)
        return 1;
    if (bufferLen <= 3)
        return 2;

    while(1)
    { 
        is_next_cookie = false;

        memset(&req_c, 0, sizeof(request_cookie));

        for (i = 0; i < MAX_COOKIE_NAME_SIZE; i++)
        {
            if (buffer[pos] >= 65 && buffer[pos] <= 90)
                req_c.cookie_name[i] = buffer[pos];
            else if (buffer[pos] >= 97 && buffer[pos] <= 122)
                req_c.cookie_name[i] = buffer[pos];
            else if (buffer[pos] >= 48 && buffer[pos] <= 57)
                req_c.cookie_name[i] = buffer[pos];
            else if (buffer[pos] == '-' || buffer[pos] == '_')
                req_c.cookie_name[i] = buffer[pos];
            else if (buffer[pos] == '=')
            {
                pos++;

                if (pos + 1 > bufferLen)            
                    retVal = 3;    

                break;
            }
            else
            {
                retVal = 4;
                break;
            }

            pos++;
            if (pos + 1 > bufferLen)
            {
                retVal = 5;
                break;
            }
        }

        if (retVal != 0)
            break;

        for (i = 0; i < MAX_COOKIE_VALUE_SIZE; i++)
        {
            if (buffer[pos] == ';')
            {
                if (i <= 1)
                {
                    retVal = 6;
                    break;
                }

                is_next_cookie = true;

                pos++;

                if (pos + 1 > bufferLen)
                    retVal = 7;

                break;
            }
            else
                req_c.cookie_value[i] = buffer[pos];

            pos++;

            if (pos >= bufferLen)
                break;
        }

        if (retVal == 0)
            req_cookies->push_back(req_c);

        if (is_next_cookie)
        {
            while (buffer[pos] == ' ' || buffer[pos] == '\t')
            {
                pos++;
                if (pos + 1 > bufferLen)
                {
                    retVal = 9;
                    break;
                }
            }

        }
        else
            break;
    }

    if (retVal != 0)
        clearCookies(req_cookies);

    return retVal;
}


bool getCookie(vector<request_cookie>* req_cookies, char * cookieName, char * returnValue, int returnValue_size)
{
    if (req_cookies == NULL || cookieName == NULL || returnValue == NULL || returnValue_size <= 0)
        return false;

    memset(returnValue, 0, returnValue_size);

    for (int i = 0; i < req_cookies->size(); i++)
    {
        if (strcmp(req_cookies->at(i).cookie_name, cookieName) == 0)
        {
            if (strlen(req_cookies->at(i).cookie_value) > returnValue_size)
                return false;
            strncpy(returnValue, req_cookies->at(i).cookie_value, strlen(req_cookies->at(i).cookie_value));

            return true;
        }
    }

    return false;
}

void http_server::claerThreads()
{
    printf("\033[22;36m");

    printf("Czakam na zakończenie obsługi rządań...\n\n");

    lock_guard<mutex> clearMutexG(mutex_);

  /*  mutex_.lock();*/

    is_init = false;
    listen_thread->detach();
    listen_thread = NULL;

   /* mutex_.unlock();*/

    clearMutexG.~lock_guard();

    while (processRequest_thread.size() > 0)
    {
        sleep(5);    
    }

    if (*mainApp_thread != 0)
        pthread_cancel(* mainApp_thread);


    printf("\033[22;36m");

    printf("Zakończono obługę rządań. Dezaktywacja programu...\n\n");
}

int setResponseStatus(response * resp, int statusCode)
{
    if (resp != NULL)
    {
        resp->status = statusCode;
    }
    else
        return -1;
    return 0;
}
int setResponseMessage(response * resp, char * message, int messageSize)
{
    if (resp != NULL)
    {
        if (message == NULL)
            return -2;
        if (messageSize > MAX_RESPONSE_MESSAGE_SIZE)
            return -3;

        memcpy((void*)resp->message, (void*)message, messageSize);
    }
    else return -1;
    return 0;
}
int setResponseContentType(response * resp, int contentType)
{
    if (resp != NULL)
    {
        if (contentType < 0 || contentType > 3)
            return -2;

        resp->contentType = contentType;
    }
    else return -1;

    return 0;
}
int addResponseBuffer(response * resp, char * buffer, int bufferSize)
{
    if (resp != NULL)
    {
        if (buffer == NULL || bufferSize <= 0)
            return -2;

        resp->bodyBuffer = buffer;
        resp->bodyBuffer_size = bufferSize;
    }
    else return -1;

    return 0;
}
int addResponseHeader_item(response * resp, char* name, int nameSize, char* value, int valueSize)
{
    req_resp_header rs;

    if (resp != NULL && name != NULL && value != NULL && nameSize > 0 && valueSize > 0)
    {
        if (nameSize > MAX_HEADER_KEY_SIZE || valueSize > MAX_HEADER_VALUE_SIZE)
            return -2;

        memset(&rs, 0, sizeof(rs));

        memcpy(rs.header_key, name, nameSize);
        memcpy(rs.header_value, value, valueSize);

        resp->responseHeaders.push_back(rs);
    }
    else return -1;

    return 0;
}
int addResponseJSON_item(response * resp, char * key, int keySize, char * value, int valueSize)
{
    json_body jb;

    if (resp != NULL && key != NULL && value != NULL && keySize > 0 && valueSize > 0)
    {
        if (keySize > MAX_HEADER_KEY_SIZE || valueSize > MAX_HEADER_VALUE_SIZE)
            return -2;

        memset(&jb, 0, sizeof(jb));

        memcpy(jb.json_key, key, keySize);
        memcpy(jb.json_value, value, valueSize);

        resp->jsonBody.push(jb);
    }
    else return -1;

    return 0;
}

int sendResponse(int s2, sockaddr_in sa2, response * resp)
{
    char buffer[MAX_REQ_RESP_BUFFER];
    req_resp_header  rs;
    json_body jb;
    char jsonBuffer[5000];
    
    memset(jsonBuffer, 0, 500);
    
    memset(buffer, 0, MAX_REQ_RESP_BUFFER);
    sprintf(buffer, "%s %d %s\r\n", resp->protocol, resp->status, resp->message);

    while (!resp->responseHeaders.empty())
    {
       rs = resp->responseHeaders.front();
       resp->responseHeaders.erase(resp->responseHeaders.begin());

        sprintf((char*)&buffer[strlen(buffer)], "%s: %s\r\n", rs.header_key, rs.header_value);
    }

    switch (resp->contentType)
    {
        case TEXT_HTML:
        case TEXT_PLAIN:
            if (resp->bodyBuffer != NULL && resp->bodyBuffer_size > 0)
            {
                sprintf((char*)&buffer[strlen(buffer)], "%s: %s\r\n", "Content-Type", resp->contentType==2? "text/html" : "text/plain");
                sprintf((char*)&buffer[strlen(buffer)], "%s: %d\r\n\r\n", "Content-Length", resp->bodyBuffer_size);
                memcpy((void*)&buffer[strlen(buffer)], resp->bodyBuffer, resp->bodyBuffer_size);
            }
            break;

        case APPLICATION_JSON:
            if (!resp->jsonBody.empty())
            {
                sprintf((char*)&jsonBuffer[strlen(jsonBuffer)], "{\r\n");
                
                while (!resp->jsonBody.empty())
                {
                    jb = resp->jsonBody.front();
                    resp->jsonBody.pop();

                    sprintf((char*)&jsonBuffer[strlen(jsonBuffer)], "\"%s\": \"%s\"",jb.json_key, jb.json_value);
                    if(!resp->jsonBody.empty())
                        sprintf((char*)&jsonBuffer[strlen(jsonBuffer)], ",\r\n");
                    else 
                        sprintf((char*)&jsonBuffer[strlen(jsonBuffer)], "\r\n");
                }

                sprintf((char*)&jsonBuffer[strlen(jsonBuffer)],"}\r\n");

                sprintf((char*)&buffer[strlen(buffer)], "Content-Type: application/json\r\nContent-Length: %d\r\n\r\n%s", strlen(jsonBuffer), jsonBuffer);
            }
            else if (resp->bodyBuffer != NULL && resp->bodyBuffer_size > 0)
            {
                sprintf((char*)&buffer[strlen(buffer)], "Content-Type: application/json\r\nContent-Length: %d\r\n\r\n%s", resp->bodyBuffer_size, resp->bodyBuffer);
            }
            break;

        default:
            break;
    }

    int ret = -1;

    try 
    {
        int len = strlen(buffer);
        len += 1;

        ret = send(s2, buffer, (size_t)len + 1, 0);
    
    }

    catch (...)
    {
        return ret;
    }
   
    return ret;
}


int sendRequest(int s1, sockaddr_in sa1, request * req) {
    
    char * buffer = NULL;
    unsigned int bufferLen = 0;
    int i = 0;
    int ret = 0;

    if (req == NULL)
        return 1;

    bufferLen = strlen(req->method) + strlen(req->protocol) + strlen(req->path) + strlen(req->host) + 12;

    for (i = 0; i < req->requestHeaders_count; i++)
        bufferLen += strlen(req->headers[i].header_key) + strlen(req->headers[i].header_value) + 4;

    for (i = 0; i < req->requestParams_count; i++)
        bufferLen += strlen(req->params[i].param_name) + strlen(req->params[i].param_value) + 2;
 
    bufferLen += req->bodyBuffer_size;
    bufferLen += 1000;

    buffer = (char *)malloc(bufferLen);

    memset(buffer, 0, bufferLen);

    sprintf(buffer, "%s %s", req->method, req->path, req->protocol);

    for (i = 0; i < req->requestParams_count; i++)
    {
        i == 0 ?
            sprintf(&buffer[strlen(buffer)], "?%s=%s", req->params[i].param_name, req->params[i].param_value) :
            sprintf(&buffer[strlen(buffer)], "&%s=%s", req->params[i].param_name, req->params[i].param_value);
    }

    sprintf(&buffer[strlen(buffer)], " %s\r\nHost: %s\r\n", req->protocol, req->host);

    for (i = 0; i < req->requestHeaders_count; i++)
        sprintf(&buffer[strlen(buffer)], "%s: %s\r\n", req->headers[i].header_key, req->headers[i].header_value);

    if (req->bodyBuffer != NULL && req->bodyBuffer_size > 0)
    {
        sprintf(&buffer[strlen(buffer)], "Content-Length: %d\r\n\r\n", req->bodyBuffer_size);
        memcpy(&buffer[strlen(buffer)], req->bodyBuffer, req->bodyBuffer_size);
    }

    sprintf(&buffer[strlen(buffer)], "\r\n\r\n\0");


    try
    {
       int len = strlen(buffer);
       ret = send(s1, buffer, (size_t)len, 0);

       free(buffer);

    }
    catch (...)
    {
        return ret;
    }

    if (ret > 0)
        return 0;
    else
        return 1;
}

int requestRoute(int s2, sockaddr_in sa2, request * req, pthread_t currentThread)
{
    bool is_root = false;
    bool is_assync = false;

    rooting_callback_status rcs;

    memset(&rcs, 0, sizeof(rooting_callback_status));

    rcs.status - REQUEST_STATUS_FAILED;
    rcs.returnValue = 0;

    for (int i = 0; i < rootingCallbackFunctions_size; i++)
    {
        if ((strcmp(rootingCallbackFunctions[i].method, req->method) == 0) && (strcmp(rootingCallbackFunctions[i].path, req->path) == 0) || rootingCallbackFunctions[i].path == "*")
        {
            rcs = (*rootingCallbackFunctions[i].rfp)(s2, sa2, req, currentThread);
            is_root = true; 
            break;
        }
    }

    if (!is_root)
        send_serverError(s2, sa2, req, 404, "Not Found", 9, rcs.allow_origin, rcs.allow_credentials);
    else if (rcs.status == REQUEST_STATUS_FAILED)
        send_serverError(s2, sa2, req, 500, "Bad Getway", 10, rcs.allow_origin, rcs.allow_credentials);
    else if(rcs.status == REQUEST_STATUS_FORBIDDEN)
        send_serverError(s2, sa2, req, 403, "Forbidden", 9, rcs.allow_origin, rcs.allow_credentials);
    
    if (rcs.status != REQUEST_STATUS_PENDING)
    {

       close(s2);

       clearRequestPointers(req);

       free((void*)req);
       req = NULL;
       /* delete req;*/
    }

    return 0;
}