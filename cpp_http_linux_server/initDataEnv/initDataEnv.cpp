#include <iostream>
#include <fstream>
#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include "initDataEnv.h"
#include <string.h>



using namespace std;

int initDataEnv(char * fileName, char * listenPort)
{

	dataConnections dataConnect;
	fstream pl;

	printf("initDataEnv wykonano.........\n");

	memset(&dataConnect, 0, sizeof(dataConnections));

	dataConnect.port = atoi(listenPort),

	pl.open(fileName, ios_base::in | ios_base::out | ios_base::binary | ios_base::trunc);

	if (!pl.fail())
	{
		pl.write((char*)&dataConnect, sizeof(dataConnections));
		pl.close();
		printf("Dane zostały zapisane do pliku  ' %s '",fileName);
	}
	else 
		printf("Nie można utworzyć pliku z konfiguracja");

	return 0;
}