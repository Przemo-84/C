#pragma once

#include <stdio.h>
#include <cstdlib>
#include <unistd.h>
#include <string.h>
#include <queue>
#include <vector>

#include<stack>

using namespace std;

#ifndef JSON_CONVERT_FUNCS

	#define JSON_CONVERT_FUNCS
	
	#define MAX_KEY_NAME_SIZE 250
	
	#define MAX_VALUE_SIZE 1000

	#define MAX_ARRAY_VALUE_SIZE 1000

#endif

enum VALUE_TYPE { UNDEFINED, STRING, NUMBER, BOOLEAN, OBJECT, ARRAY };

struct array_value
{
	VALUE_TYPE item_type;
	char array_value[MAX_ARRAY_VALUE_SIZE];
	void * jsonObject;
	void * next;
};

struct key_item
{
	char keyName[MAX_KEY_NAME_SIZE];
	char value[MAX_VALUE_SIZE];
	vector<array_value> arrayValues;
	bool is_boolean_value;
	bool is_array_value;
	bool is_digit_value;
	void * next;
};

struct json_object
{
	vector<key_item> key_items;
};

struct value_item
{
	VALUE_TYPE value_type;
	char* value;
	vector<array_value> * arrayValues;
	bool booleanValue;
	long numberValue;
	json_object * jsonObject;
};

int clearJsonObjectsPointers(json_object* jsonObject);

int clearJsonObjectsArrays(vector<array_value> * arrayValues);

int parse_json_array(char * buffer, int bufferLen, int * start_position, vector<array_value> * array_values);

int parse_json(char * buffer, int bufferLen, int * start_position, json_object * jsonObject);

int stringfy_json_array(vector<array_value>* arrayValues, char * buffer, int bufferLen, int * start_position);

int stringify_json(json_object * jsonObject, char * buffer, int bufferLen, int * start_position);

int get_json_value(json_object * jsonObject, char * keyName, value_item * out_valueItem);


