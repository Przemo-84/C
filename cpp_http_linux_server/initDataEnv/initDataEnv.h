#pragma once

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <cstdlib>

#ifndef DATA_CONECTIONS

#define DATA_CONECTIONS


struct dataConnections
{
	int port;
};

#endif

int initDataEnv(char * fileName, char * listenPort);