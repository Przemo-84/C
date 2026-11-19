#include <iostream>
#include <fstream>
#include <stdio.h>
#include <cstdlib>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include<exception>



#define NITEMS(arr)  (sizeof((arr)) / sizeof((arr)[0]))


#ifndef DATA_CONECTIONS

    #include "headers/mainApp.h"

#endif

#ifndef HTTP_SERVER_CLSASS

    #include "httpServer/httpServer.h"

#endif


#ifndef SQL_MODULE_FUNCTIONS

    #include "sql_module/sql_module.h"

#endif



#include "initDataEnv/initDataEnv.h"


#ifndef ROOTER_FUNCS

#include "./httpServer/routerFunc.h"

#endif

#ifndef JSON_CONVERT_FUNCS

#include "./jsonConvert/jsonConv.h"

#endif


#define printRetCodeMessage(retCode)\
{                                                                                                                               \
    switch(retCode)                                                                                                             \
    {                                                                                                                           \
    case 1: printf("\033[22;32mNie udało się połączyć z bazą danych kod 1 - henv allloc handle error\n\n\033[22;0m"); break;    \
    case 2: printf("\033[22;32mNie udało się połączyć z bazą danych kod 2 - hdbc allloc handle error\n\n\033[22;0m"); break;    \
    case 3: printf("\033[22;32mNie udało się połączyć z bazą danych kod 3 - SqlDriverConnect error\n\n\033[22;0m"); break;   \
    case 0: printf("\033[22;36mPołączono się z bazą danych workin7_db\n\n\033[22;0m"); break;                                   \
    }                                                                                                                           \
};

using namespace std;


mutex mainApp_mutex;

http_server * httpServer = NULL;

sql_module * sqlModule = NULL;

bool mainApp_close = false;

pthread_t mainApp_thread;


void sendError_callback(int signal_num)
{
    printf("\033[22;0m");
    printf("Send error exception...\n");
    cout.flush();

}

void handle_signal_terminate(int signal_num) 
{
    printf("\033[22;32m");

    printf("\n\nZatrzymanie programu 'Ctrl-C'\n\n\033[0m");

    if (httpServer != NULL)
    {
        lock_guard<mutex> httpS_mut(httpServer->mutex_);
        lock_guard<mutex> mainApp_mut(mainApp_mutex);

       
        httpServer->is_init = false;
        close(httpServer->s1);
        httpServer->s1 = 0;

        mainApp_close = true;


        httpS_mut.~lock_guard();
        mainApp_mut.~lock_guard();

        httpServer->claerThreads();

        sqlModule->disconnect();

        delete(sqlModule);

        delete(httpServer);

        printf("\033[22;31m");
        printf("\n\nlinux_http_server został zatrzymany............\n\n\033[0m");

        exit(0);
    }

}


int main(int argc, char* argv[])
{

    fstream pl;
    dataConnections dataConnect;
    int retCode, argIndex = 1, init_args_count = 2, run_args_count = 2;
    int dataConnect_argIndex = -1;

    char dataEnv[50];

    struct sigaction sigIntHandler;
    struct sigaction sigActionPipe;

    memset(&dataConnect, 0, sizeof(dataConnections));
    memset(&sigIntHandler, 0, sizeof(sigIntHandler));
    memset(&sigActionPipe, 0, sizeof(sigIntHandler));

    memset(dataEnv, 0, 50);

    sigIntHandler.sa_handler = handle_signal_terminate;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigActionPipe.sa_handler = &sendError_callback;
    sigemptyset(&sigActionPipe.sa_mask);
    sigActionPipe.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);
    sigaction(SIGPIPE, &sigActionPipe, NULL);

    printf("\033[22;31m");


    printf("\nlinux_http_server został uruchomiony............\n\n\033[0m");


    printf("\033[22;0m");


    if (argc < 2)
    {
        printf("Nie podano żadnego argumentu\n\n");
        printf("Możliwe polecienia to :\n\n");
        printf("  W celu ustawienia nowych parametrów dla programu\n");
        printf("      --initDataEnv [nazwa_pliku_do_zapisu] [port_nasluchiwania] \n\n");
        printf("  Uruchomienia programu\n");
        printf("      --dataEnv [nazwa_zapisanego_pliku]\n\n");

        printf("\033[22;31m");
        printf("\n\nlinux_http_server został zatrzymany............\n\n\033[0m");

        return -1;
    }

    if (strcmp(argv[1], "--dataEnv") != 0)
    {
       
        if (strcmp(argv[argIndex], "--initDataEnv") == 0)
        {
            if (argc - 1 < argIndex + init_args_count)
            {
                printf("Podano za mało argumentów\nPoprawne wywołanie to:\n\n");
                printf("      --initDataEnv [nazwa_pliku_do_zapisu] [port_nasluchiwania]\n\n");
                cout.flush();
                return -1;
            }
            
            dataConnect_argIndex = argIndex + 1;
            argIndex += init_args_count;
        }
        else
        {
            printf("\033[22;0m");
            printf("\n\nNie podano odpowiedniego argumentu....\n\n");

            printf("\033[22;31m");
            printf("\n\nlinux_http_server został zatrzymany............\n\n\033[0m");

            cout.flush();

            return -1;
        }

        if (dataConnect_argIndex > 0)
            initDataEnv(argv[dataConnect_argIndex], argv[dataConnect_argIndex + 1]);
        
        printf("\033[22;31m");
        printf("\n\nlinux_http_server został zatrzymany............\n\n\033[0m");

        cout.flush();

        return 0;
    }
    else
    {
        if (argc != run_args_count + 1)
        {
            printf("\nPodano nieprawidłową ilość argumentów\nPoprawne wywołanie to:\n--dataEnv [nazwa_pliku]\n\n");

            printf("\033[22;31m");
            printf("\n\nlinux_http_server został zatrzymany............\n\n\033[0m");
            return -1;
        }

        if (strcmp(argv[1], "--dataEnv") != 0)
        {
            printf("Podano nieprawidłowy argument\n\n");
            printf("  Uruchomienia programu\n");
            printf("      --dataEnv [nazwa_zapisanego_pliku]\n\n");
            return -1;
        }

        if (strlen(argv[2]) > 50)
        {
            printf("\nPodano zbyd długą nazwę pliku\nMaksymalna długość nazwy pliku to 50 znaków\n\n");

            printf("\033[22;31m");
            printf("\n\nlinux_http_server został zatrzymany............\n\n\033[0m");
            return -1;
        }

        argIndex = 1;

        while (argIndex < argc)
        {
            if (strcmp(argv[argIndex], "--dataEnv") == 0)
                strcpy(dataEnv, argv[argIndex + 1]);

            argIndex += run_args_count;
        }

        pl.open(dataEnv, ios_base::in | ios_base::binary);

        if (pl.fail())
        {
            printf("\nNie można otworzyć pliku z konfiguracją.\n\n");

            printf("\033[22;31m");
            printf("\n\nlinux_http_server został zatrzymany............\n\n\033[0m");

            return -2;
        }


        pl.read((char*)&dataConnect, sizeof(dataConnections));
        pl.close();


        httpServer = new http_server();

        //sqlModule = new sql_module(dataConnect.workin7_dataBase_host, dataConnect.workin7_dataBase_name, dataConnect.workin7_dataBase_user, dataConnect.workin7_dataBase_password, dataConnect.workin7_dataBase_port);

        //retCode = sqlModule->initConnection();
        //printRetCodeMessage(retCode);

        //if (retCode != 0)
        //{
        //    sqlModule->disconnect();
        //    delete(sqlModule);

        //    delete(httpServer);

        //    printf("\033[22;31m");
        //    printf("\n\nlinux_http_server został zatrzymany............\n\n\033[0m");

        //    return 1;
        //}

        routerFunc_setDataPointers(&dataConnect, httpServer, sqlModule);

        httpServer->mainApp_thread = &mainApp_thread; 

        httpServer->init_server(dataConnect.port);

        pthread_create(&mainApp_thread, NULL, mainApp_commands, (void*)httpServer);


        while (1)
        {

            sleep(5);        

            mainApp_mutex.lock();

            if (mainApp_close == true)
            {
                printf("\n\n");

                if (mainApp_thread != 0)
                {
                    pthread_cancel(mainApp_thread);
                    pthread_kill(mainApp_thread, 1);
                    mainApp_thread = 0;
                }
                mainApp_mutex.unlock();

                break;
            }

            mainApp_mutex.unlock();
        }

    }

    httpServer->claerThreads();

    //sqlModule->disconnect();

    //delete(sqlModule);

    delete(httpServer);


    printf("\033[22;31m");
    printf("\n\nlinux_http_server został zatrzymany............\n\n\033[0m");
 
    return 0;
}


void * mainApp_commands( void * args)
{
    char komenda[250];
    int readLen = 0;    

    sleep(2);

    while(1)
    {
        memset(komenda, 0, 250);

        printf("\033[22;0m");

        printf("Poprawne komendy :\nclose - zamyka program\nPodaj komende : \0");

        try 
        {
            int ret = scanf("%s", &komenda[0]);

            if (!ret)
                throw("error");
        }
        catch(char * error)
        {
            lock_guard<mutex> httpS_mut(httpServer->mutex_);
            lock_guard<mutex> mainApp_mut(mainApp_mutex);

            /*mainApp_mutex.lock();
            httpServer->mutex_.lock();*/

            httpServer->is_init = false;

            close(httpServer->s1);

           /* httpServer->mutex_.unlock();*/

            httpS_mut.~lock_guard();

            mainApp_close = true;

           /* mainApp_mutex.unlock();*/

            mainApp_mut.~lock_guard();

            return NULL;

        }

        if (strcmp(komenda, "close") == 0)
        {
            lock_guard<mutex> httpS_mut(httpServer->mutex_);
            lock_guard<mutex> mainApp_mut(mainApp_mutex);
            
            /*httpServer->mutex_.lock();
            mainApp_mutex.lock();*/
          
            httpServer->is_init = false;

            close(httpServer->s1);
            httpServer->s1 = 0;

            pthread_detach(mainApp_thread);

            mainApp_thread = 0;

            mainApp_close = true;

           /* httpServer->mutex_.unlock();
            mainApp_mutex.unlock();*/

            httpS_mut.~lock_guard();
            mainApp_mut.~lock_guard();

            return NULL;
        }
        
    }
    return NULL;
}
