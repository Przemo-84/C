
#ifndef JSON_CONVERT_FUNCS

	#include "jsonConv.h"

	#include <regex>

#endif


int clearJsonObjectsPointers(json_object * jsonObject)
{
	key_item ki;

	while (!jsonObject->key_items.empty())
	{
		ki = jsonObject->key_items.front();

		if (ki.next != NULL)
		{
			clearJsonObjectsPointers((json_object*)ki.next);

			delete(ki.next); 
		}

		if (!ki.arrayValues.empty())
			clearJsonObjectsArrays(&ki.arrayValues);

		

		jsonObject->key_items.erase(jsonObject->key_items.begin());
	}


	return 0;
}

int clearJsonObjectsArrays(vector<array_value> * arrayValues)
{
	array_value av;

	while (!arrayValues->empty())
	{
		av = arrayValues->front();

		if (av.next != NULL && av.item_type == ARRAY)
		{
			clearJsonObjectsArrays((vector<array_value> *)av.next);

			delete(av.next);
		}
		else if (av.jsonObject != NULL && av.item_type == OBJECT)
		{
			clearJsonObjectsPointers((json_object*)av.jsonObject);

			delete(av.jsonObject);
		}

		arrayValues->erase(arrayValues->begin());
	}


	return 0;
}

int parse_json_array(char* buffer, int bufferLen, int* start_position, vector<array_value> * array_values)
{
	int pos = 0;
	int i = 0;
	bool is_digit = false;
	int retVal = 0;
	array_value av;
	bool is_value_first_slash = false;


	if (buffer == NULL || array_values == NULL)
		return 1;


	if (start_position != NULL)
		pos = *start_position;

	while (1)
	{
		memset(&av, 0, sizeof(array_value));

		while (buffer[pos] == ' ' || buffer[pos] == '\t')
		{
			pos++;
			if (pos + 1 > bufferLen)
			{
				retVal = 2; break;
			}
			
		}

		if (retVal != 0)
			break;


		if (buffer[pos] == '"')
		{
			pos++;

			if (pos >= bufferLen)
			{
				retVal = 3; break;
			}
			av.item_type = STRING;
		}
		else if (buffer[pos] >= 48 && buffer[pos] <= 57)
			av.item_type = NUMBER;
		else if (buffer[pos] == 't')
		{
			if (pos + 5 > bufferLen)
			{
				retVal = 4; break;
			}

			if (strncmp(&buffer[pos], "true", 4) != 0)
			{
				retVal = 5; break;
			}
			strncpy(av.array_value, &buffer[pos], 4);
			av.item_type = BOOLEAN;
			pos += 4;
		}
		else if (buffer[pos] == 'f')
		{
			if (pos + 6 > bufferLen)
			{
				retVal = 6; break;
			}

			if (strncmp(&buffer[pos], "false", 5) != 0)
			{
				retVal = 7; break;
			}
			strncpy(av.array_value, &buffer[pos], 5);
			av.item_type = BOOLEAN;
			pos += 5;
		}
		else if (buffer[pos] == '[')
		{
			if (pos + 3 > bufferLen) { retVal = 8; break; }

			pos++;

			av.next = (vector<array_value> *) new vector<array_value>;

			av.item_type = ARRAY;

			array_values->push_back(av);

			retVal = parse_json_array(buffer, bufferLen, &pos, (vector<array_value> *)av.next);


			pos++;

			if (pos + 1 > bufferLen)
			{
				retVal = 9; break;
			}

		}
		else if (buffer[pos] == '{')
		{
			av.jsonObject = (json_object*) new json_object;

			av.item_type = OBJECT;

			array_values->push_back(av);

			retVal = parse_json(buffer, bufferLen, &pos, (json_object*)av.jsonObject);

			pos++;

			if (pos + 1 > bufferLen)
			{
				retVal = 10; break;
			}
		}
		else if (buffer[pos] == ']')
			break;
		else
		{
			retVal = 11; break;
		}

		if (retVal != 0)
			break;

		if (av.item_type == STRING || av.item_type == NUMBER)
		{
			bool  is_space = false;

			for (i = 0; i < MAX_ARRAY_VALUE_SIZE; i++)
			{
				if (av.item_type == NUMBER)
				{
					if (buffer[pos] >= 48 && buffer[pos] <= 57 && !is_space)
						av.array_value[i] = buffer[pos];
					else if (buffer[pos] == ',' || buffer[pos] == ']')
					{
						break;
					}
					else if (buffer[pos] == ' ' || buffer[pos] == '\t')
						is_space = true;
					else
					{
						retVal = 12; break;
					}

				}
				else
				{
					if (buffer[pos] == '\r' || buffer[pos] == '\n')
					{
						retVal = 13; break;
					}
					else if (buffer[pos] == '\\' && !is_value_first_slash)
					{
						is_value_first_slash = true;
						i--;
					}
					else if (buffer[pos] == '"')
					{
						if (is_value_first_slash)
						{
							av.array_value[i] = buffer[pos];
							is_value_first_slash = false;
						}
						else
						{
							pos++;

							if (pos >= bufferLen)
								retVal = 14;

							break;
						}
					}
					else
					{
						av.array_value[i] = buffer[pos];
						is_value_first_slash = false;
					}
				}

				pos++;

				if (pos + 1 > bufferLen)
				{
					retVal = 15; break;
				}

			}

		}

		if (av.item_type != OBJECT && av.item_type != ARRAY)
			array_values->push_back(av);

		if (retVal != 0)
			break;

		while (buffer[pos] == ' ' || buffer[pos] == '\t')
		{
			pos++;		

			if (pos + 1 > bufferLen)
			{
				retVal = 16; break;
			}
		}

		if (buffer[pos] == ']' || buffer[pos] == ',')
		{
			if (buffer[pos] == ']')
			{
				if (start_position != NULL) *start_position = pos;
					break;
			}
			else
			{
				pos++;

				if (pos + 1 > bufferLen)
				{
					retVal = 17; break;
				}
			}
		}
		else
		{
			retVal = 18; break;
		}
	}

	return retVal;

}

int parse_json(char * buffer, int bufferLen, int * start_position, json_object* jsonObject)
{
	int i = 0;
	int pos = 0;
	int retVal = 0;
	bool is_digit = false;
	json_object * joP = NULL;
	key_item keyIt;
	bool is_anyKey = false;
	bool is_nextKey = false;
	bool is_end = false;
	bool is_value_first_slash = false;

	if (start_position != NULL)
		pos = *start_position;


	if (buffer == NULL || bufferLen < 0 || jsonObject == NULL)
		return 1;

	while (buffer[pos] != '{')
	{
		if (buffer[pos] != ' ' && buffer[pos] != '\r' && buffer[pos] != '\n' && buffer[pos] != '\t')
			return 2;

		if (buffer[pos] == '\r')
		{
			if (pos + 1 >= bufferLen)
				return 3;
			if (buffer[pos + 1] != '\n')
				return 4;
		}
		if (buffer[pos] == '\n')
		{
			if (pos == 0)
				return 5;
			if (buffer[pos - 1] != '\r')
				return 6;
		}

		pos++;

		if (pos >= bufferLen)
			break;
	}

	pos++;

	if (pos >= bufferLen)
		return 7;

	joP = jsonObject;

	while (1)
	{
		while (buffer[pos] != '"')
		{
			if (buffer[pos] != ' ' && buffer[pos] != '\r' && buffer[pos] != '\n' && buffer[pos] != '\t')
			{
				retVal = 8; break;
			}

			if (buffer[pos] == '\r')
			{
				if (pos + 1 > bufferLen)
				{
					retVal = 9; break;
				}
				if (buffer[pos + 1] != '\n')
				{
					retVal = 10; break;
				}
			}
			else if (buffer[pos] == '\n')
			{
				if (pos == 0)
				{
					retVal = 11; break;
				}
				if (buffer[pos - 1] != '\r')
				{
					retVal = 12; break;
				}
			}

			pos++;

			if (pos >= bufferLen)
			{
				retVal = 13; break;
			}

		}

		if (joP == NULL)
		{
			retVal = 14; break;
		}

		if (retVal != 0)
			break;

		memset(&keyIt, 0, sizeof(key_item));
		is_nextKey = false;

		for (i = 0; i < MAX_KEY_NAME_SIZE; i++)
		{
			pos++;

			if (pos >= bufferLen)
			{
				retVal = 15; break;
			}

			if (buffer[pos] >= 65 && buffer[pos] <= 90)
				keyIt.keyName[i] = buffer[pos];
			else if (buffer[pos] >= 97 && buffer[pos] <= 122)
				keyIt.keyName[i] = buffer[pos];
			else if (buffer[pos] >= 48 && buffer[pos] <= 57)
				keyIt.keyName[i] = buffer[pos];
			else if (buffer[pos] == '-')
			{
				if (i == 0)
				{
					retVal = 16; break;
				}
				if (keyIt.keyName[i - 1] == '-')
				{
					retVal = 17; break;
				}

				keyIt.keyName[i] = '-';
			}
			else if(buffer[pos] == '_')
				keyIt.keyName[i] = buffer[pos];
			else if (buffer[pos] == '"')
			{
				if (i == 0)
					retVal = 18;
				break;
			}
			else
			{
				retVal = 19;
				break;
			}
		}

		if (retVal != 0)
			break;

		pos++;

		while (buffer[pos] != ':')
		{
			if (buffer[pos] != ' ' && buffer[pos] != '\t')
			{
				retVal = 20; break;
			}

			pos++;

			if (pos >= bufferLen)
			{
				retVal = 21; break;
			}
		}

		if (retVal != 0)
			break;

		pos++;

		if (pos + 1 > bufferLen)
		{
			retVal = 20; break;
		}

		while (buffer[pos] == ' ' || buffer[pos] == '\t')
		{
			pos++;

			if (pos + 1 > bufferLen)
			{
				retVal = 21; break;
			}
		}

		if (retVal != 0)
			break;

		if (buffer[pos] == '"')
		{
			pos++;

			if (pos >= bufferLen)
			{
				retVal = 22; break;
			}

			is_digit = false;
		}
		else if (buffer[pos] >= 48 && buffer[pos] <= 57)
		{
			is_digit = true;
			keyIt.is_digit_value = true;
		}
		else if (buffer[pos] == 't')
		{
			if (pos + 5 > bufferLen)
			{
				retVal = 23; break;
			}

			if (strncmp(&buffer[pos], "true", 4) != 0)
			{
				retVal = 24; break;
			}
			strncpy(keyIt.value, &buffer[pos], 4);
			keyIt.is_boolean_value = true;
			pos += 4;
		}
		else if (buffer[pos] == 'f')
		{
			if (pos + 6 > bufferLen)
			{
				retVal = 25; break;
			}

			if (strncmp(&buffer[pos], "false", 5) != 0)
			{
				retVal = 26; break;
			}
			strncpy(keyIt.value, &buffer[pos], 5);
			keyIt.is_boolean_value = true;
			pos += 5;
		}
		else if (buffer[pos] == '[')
		{
			if (pos + 3 > bufferLen) 
			{ 
				retVal = 27; break; 
			}

			pos++;

			/*keyIt.arrayValues = (vector<array_value> *) new vector<array_value>;*/

			keyIt.is_array_value = true;

			retVal = parse_json_array(buffer, bufferLen, &pos, &keyIt.arrayValues);

			joP->key_items.push_back(keyIt);

			if (retVal != 0)
			{
				retVal = 28; break;
					
			}

			pos++;

			if (pos >= bufferLen)
			{
				retVal = 29; break;
			}

		}
		else if (buffer[pos] == '{')
		{
			keyIt.next = (json_object*) new json_object;

			joP->key_items.push_back(keyIt);

			retVal = parse_json(buffer, bufferLen, &pos, (json_object*)keyIt.next);

			pos++;

			if (pos >= bufferLen)
			{
				retVal = 30; break;
			}
		}
		else
		{
			retVal = 31; break;
		}

		if (retVal != 0)
			break;

		if (!keyIt.is_boolean_value && !keyIt.is_array_value && keyIt.next == NULL)
		{
			for (i = 0; i < MAX_VALUE_SIZE; i++)
			{
				if (is_digit)
				{
					if (buffer[pos] >= 48 && buffer[pos] <= 57)
						keyIt.value[i] = buffer[pos];
					else if (buffer[pos] == ',' || buffer[pos] == '}')
					{
						break;
					}
					else if (buffer[pos] == '\r')
					{
						if (pos + 1 > bufferLen)
						{
							retVal = 32; break;
						}
						if (buffer[pos + 1] != '\n')
						{
							retVal = 33; break;
						}

						pos++;

						break;
					}
					else if (buffer[pos] == '\n')
					{
						if (buffer[pos - 1] != '\r')
						{
							retVal = 34; break;
						}

						break;
					}
					else
					{
						retVal = 35; break;
					}
				}
				else
				{
					if (buffer[pos] == '\r' || buffer[pos] == '\n')
					{
						retVal = 36; break;
					}
					else if (buffer[pos] == '\\' && !is_value_first_slash)
					{
						is_value_first_slash = true;
						i--;
					}
					else if (buffer[pos] == '"')
					{
						if (is_value_first_slash)
						{
							keyIt.value[i] = buffer[pos];
							is_value_first_slash = false;
						}
						else
						{
							pos++;

							if (pos >= bufferLen)
								retVal = 37;

							break;
						}
					}
					else
					{
						keyIt.value[i] = buffer[pos];
						is_value_first_slash = false;
					}
				}

				pos++;

				if (pos >= bufferLen)
				{
					retVal >= 38; break;
				}

			}

			if (retVal != 0)
				break;
		}

		if (buffer[pos] == ',')
		{
			is_nextKey = true;
			pos++;
			if (pos >= bufferLen)
			{
				retVal = 39; break;
			}
		}

		if(keyIt.next == NULL && !keyIt.is_array_value)
			joP->key_items.push_back(keyIt);

		if (!is_anyKey)
			is_anyKey = true;

		if (!is_nextKey)
		{
			while (buffer[pos] != '}' && pos < bufferLen)
			{
				if (buffer[pos] != ' ' && buffer[pos] != '\r' && buffer[pos] != '\n' && buffer[pos] != '\t')
				{
					retVal = 40; break;
				
				}
				if (buffer[pos] == '\r')
				{
					if (pos + 1 > bufferLen)
					{
						retVal = 40; break;
					}
					if (buffer[pos + 1] != '\n')
					{
						retVal = 41; break;
					}

				}
				else if (buffer[pos] == '\n')
				{
					if (buffer[pos - 1] != '\r')
					{
						retVal = 42; break;
					}

				}

				pos++;

				if (pos >= bufferLen)
				{
					retVal = 44; break;
				}
			}
			if (start_position != NULL)
				*start_position = pos;
			break;
		}

	}

	if (retVal == 0 && start_position == NULL)
	{
		if (!is_anyKey)
			retVal = 45;
		else
		{
			pos++;

			while (pos < bufferLen)
			{
				if (buffer[pos] != ' ' && buffer[pos] != '\r' && buffer[pos] != '\n' && buffer[pos] != '\t')
				{
					retVal = 46; break;
				}

				if (buffer[pos] == '\r')
				{
					if (pos + 1 > bufferLen)
					{
						retVal = 47; break;
					}
					if (buffer[pos + 1] != '\n')
					{
						retVal = 48; break;
					}
				}
				else if (buffer[pos] == '\n')
				{
					if (pos == 0)
					{
						retVal = 49; break;
					}
					if (buffer[pos - 1] != '\r')
					{
						retVal = 50; break;
					}
				}

				pos++;

			}
		}
	}

	if (retVal != 0 && start_position == NULL)
	{
		clearJsonObjectsPointers(jsonObject);
	}
	
	return retVal;
}

int stringfy_json_array(vector<array_value> * arrayValues, char * buffer, int bufferLen, int * start_position)
{
	int retVal = 0;
	array_value av;
	int pos = 0;
	int arrayValue_buffer_size = 0;

	if (start_position != NULL)
		pos = *start_position;

	if (pos + 2 > bufferLen)
	{
		return 1;
	}

	buffer[pos] = '[';
	pos++;

	for(int i = 0; i < arrayValues->size(); i++)
	{
		av = arrayValues->at(i);

		if (i != 0)
		{
			if (pos + 2 > bufferLen)
			{
				retVal = 2;
				break;
			}

			buffer[pos] = ',';
			pos++;

		}

		if (av.next != NULL && av.item_type == ARRAY)
			retVal= stringfy_json_array((vector<array_value> *)av.next, buffer, bufferLen, &pos);
		else if (av.jsonObject != NULL && av.item_type == OBJECT)
			retVal = stringify_json((json_object*)av.jsonObject, buffer, bufferLen, &pos);
		else
		{
			arrayValue_buffer_size = strlen(av.array_value);

			if (av.item_type == STRING)
			{
				arrayValue_buffer_size += 2;

				if (pos + arrayValue_buffer_size > bufferLen)
				{
					retVal = 3;
					break;
				}

				buffer[pos] = '"';
				pos++;

				for (int i = 0; i < sizeof(av.array_value); i++)
				{
					if (av.array_value[i] == 0)
					{
						if (pos + 1 > bufferLen)
						{
							retVal = 4;
							break;
						}

						buffer[pos] = '"';
						pos++;

						break;
					}

					if (av.array_value[i] == '"')
					{
						buffer[pos] = '\\';

						pos++;

						if (pos + 1 > bufferLen)
						{
							retVal = 5;
							break;
						}
					}
					buffer[pos] = av.array_value[i];

					pos++;

					if (pos + 1 > bufferLen)
					{
						retVal = 6;
						break;
					}
				}
				
			}
			else
			{
				if (pos + arrayValue_buffer_size > bufferLen)
				{
					retVal = 4;
					break;
				}

				strncpy(&buffer[pos],av.array_value, strlen(av.array_value));
				pos += arrayValue_buffer_size;
			}
		}
	
	}

	if (retVal == 0)
	{

		if (pos + 1 > bufferLen)
			retVal = 5;
		else
		{

			buffer[pos] = ']';
			pos++;
		}
	}

	if (start_position != NULL)
		*start_position = pos;

	return retVal;
}

int stringify_json(json_object * jsonObject, char * buffer, int bufferLen, int * start_position)
{
	int retVal = 0;
	key_item ki;
	int pos = 0;
	int keyItem_buffer_size = 0;

	if (start_position != NULL)
		pos = *start_position;

	if (pos + 2 > bufferLen)
	{
		return 1;
	}

	buffer[pos] = '{';
	pos++;

	for(int i = 0; i < jsonObject->key_items.size(); i++)
	{
		ki = jsonObject->key_items[i];

		if (i != 0)
		{
			if (pos + 2 > bufferLen)
			{
				retVal = 2;
				break;
			}

			buffer[pos] = ',';
			pos++;

		}

		keyItem_buffer_size = strlen(ki.keyName) + 3;

		if (ki.arrayValues.empty() && ki.next == NULL)
		{
			if (!ki.is_boolean_value && !ki.is_digit_value)
			{
				keyItem_buffer_size++;

				if (pos + keyItem_buffer_size > bufferLen)
				{
					retVal = 3;
					break;
				}

				sprintf(&buffer[pos], "\"%s\":\"", ki.keyName);

				pos += keyItem_buffer_size;

				for (int i = 0; i < sizeof(ki.value); i++)
				{
					if (ki.value[i] == 0)
					{
						if (pos + 1 > bufferLen)
						{
							retVal = 4;
							break;
						}

						buffer[pos] = '"';
						pos++;

						break;
					}
					
					if (ki.value[i] == '"')
					{
						buffer[pos] = '\\';

						pos++;

						if (pos + 1 > bufferLen)
						{
							retVal = 5;
							break;
						}
					}
					buffer[pos] = ki.value[i];

					pos++;

					if (pos + 1 > bufferLen)
					{
						retVal = 6;
						break;
					}

				}
			}
			else
			{
				keyItem_buffer_size += strlen(ki.value);

				if (pos + keyItem_buffer_size > bufferLen)
				{
					retVal = 7;
					break;
				}
				sprintf(&buffer[pos], "\"%s\":%s", ki.keyName, ki.value);

				pos += keyItem_buffer_size;
			}

		}
		else if (ki.next != NULL)
		{
			if (pos + keyItem_buffer_size > bufferLen)
			{
				retVal = 8;
				break;
			}

			sprintf(&buffer[pos], "\"%s\":", ki.keyName);
			pos += keyItem_buffer_size;

			retVal = stringify_json((json_object*)ki.next, buffer, bufferLen, &pos);
		}
		else if (!ki.arrayValues.empty())
		{
			if (pos + keyItem_buffer_size > bufferLen)
			{
				retVal = 9;
				break;
			}

			sprintf(&buffer[pos], "\"%s\":", ki.keyName);
			pos += keyItem_buffer_size;

			retVal = stringfy_json_array(&ki.arrayValues, buffer, bufferLen, &pos);
		}

		if (retVal != 0)
			break;

	}

	if (retVal == 0)
	{

		if (pos + 1 > bufferLen)
			retVal = 10;
		else
		{

			buffer[pos] = '}';

			pos++;
		}
	}

	if (start_position != NULL)
		*start_position = pos;

	return retVal;
}


int get_json_value(json_object * jsonObject, char * keyName, value_item * out_valueItem)
{
	int retVal = 0;
	int pos = 0;
	int i = 0, j = 0;
	key_item * ki = NULL;
	value_item  vi;
	array_value * av = NULL;
	vector<array_value> * av_v = NULL;
	int keyNameSize = strlen(keyName);
	char keyN[MAX_KEY_NAME_SIZE];
	char key_array_index[MAX_KEY_NAME_SIZE];
	json_object * jo;

	const char jsonKey_exp[] = "^([a-zA-Z0-9]+(([_]+[a-zA-Z0-9]+)*([-][a-zA-Z0-9]+)*([.][a-zA-Z0-9]+)*(\\[[0-9]\\])*((\\[[1-9]\\d*\])+\\.[a-zA-Z0-9]+)*)*(\\[[1-9]\\d*\\])*)$";
	/*const char jsonKey_exp[] = "^([a-zA-Z0-9]+(([_]+[a-zA-Z0-9]+)*([-][a-zA-Z0-9]+)*([.][a-zA-Z0-9]+)*((\\[[1-9]\\d*\\])+\\.[a-zA-Z0-9]+)*)*(\\[[1-9]\\d*\\])*)$";*/
	regex reqExp(jsonKey_exp, regex_constants::ECMAScript);


	if (jsonObject == NULL || keyName == NULL || out_valueItem == NULL)
		return 1;

	if (!regex_match(keyName, reqExp))
		return 2;

	jo = jsonObject;

	memset(&vi, 0, sizeof(value_item));

	while (pos + 1 <= keyNameSize)
	{
		memset(keyN, 0, MAX_KEY_NAME_SIZE);
		i = 0;

		while (keyName[pos] != '.' && keyName[pos] != '[' && pos + 1 <= keyNameSize)
		{
			keyN[i] = keyName[pos];
			i++;
			pos++;
		}

		ki = NULL;

		for (j = 0; j < jo->key_items.size(); j++)
		{
			if (strcmp(keyN, jo->key_items.at(j).keyName) == 0)
			{
				ki = (key_item * )&jo->key_items.at(j);
				break;
			}
		}

		if (ki == NULL)
			return 3;

		if (pos + 1 > keyNameSize)
			break;

		if (keyName[pos] == '.')
		{
			if (ki->next == NULL)
				return 4;
			else
			{
				if (pos + 2 > keyNameSize)
					return 5;

				jo = (json_object*)ki->next;
			}
		}
		else if(keyName[pos] == '[')
		{
			if (!ki->is_array_value || ki->arrayValues.empty())
				return 5;

			av_v = (vector<array_value> *) &ki->arrayValues;

			while (keyName[pos] != '.' && pos + 1 < keyNameSize)
			{
				pos++;
				
				memset(key_array_index, 0, MAX_KEY_NAME_SIZE);
				j = 0;

				while (keyName[pos] != ']')
				{
					if (j >= MAX_KEY_NAME_SIZE)
						return 6;

					key_array_index[j] = keyName[pos];

					pos++;
					j++;
				}

				j = (int)atoi(key_array_index);

				if (j > av_v->size() - 1)
					return 6;

				av = (array_value*)&av_v->at(j);
			
				pos++;

				if (pos + 1 < keyNameSize)
				{
					if (keyName[pos] == '.')
					{
						if (av->item_type != OBJECT || av->jsonObject == NULL)
							return 7;

						jo = (json_object*)av->jsonObject;
					}
					else if (keyName[pos] == '[')
					{
						if (av->item_type != ARRAY || av->next == NULL)
							return 8;

						av_v = (vector<array_value> *)av->next;
					}
					
				}
				else
				{
					if (av->item_type == NUMBER)
					{
						out_valueItem->numberValue = atol(av->array_value);
						out_valueItem->value_type = NUMBER;
					}
					else if (av->item_type == BOOLEAN)
					{
						if (strcmp(av->array_value, "true") == 0)
						{
							out_valueItem->booleanValue = true;
							out_valueItem->value_type = BOOLEAN;
						}
						else if (strcmp(av->array_value, "false") == 0)
						{
							out_valueItem->booleanValue = false;
							out_valueItem->value_type = BOOLEAN;
						}
						else
							out_valueItem->value_type = UNDEFINED;
					}
					else if (av->item_type == STRING)
					{
						out_valueItem->value = (char*)av->array_value;
						out_valueItem->value_type = STRING;
					}
					else if (av->item_type == ARRAY)
					{
						out_valueItem->arrayValues = (vector<array_value>*) av->next;
						out_valueItem->value_type = ARRAY;
					}
					else if (av->item_type == OBJECT)
					{
						out_valueItem->jsonObject = (json_object*)av->jsonObject;
						out_valueItem->value_type = OBJECT;
					}

					return 0;
				}
			}

		}
		
		pos++;
	}

	if (ki != NULL)
	{
		if (ki->is_digit_value)
		{
			out_valueItem->numberValue = atol(ki->value);
			out_valueItem->value_type = NUMBER;
		}
		else if (ki->is_boolean_value)
		{
			if (strcmp(ki->value, "true") == 0)
			{
				out_valueItem->booleanValue = true;
				out_valueItem->value_type = BOOLEAN;
			}
			else if (strcmp(ki->value, "false") == 0)
			{
				out_valueItem->booleanValue = false;
				out_valueItem->value_type = BOOLEAN;
			}
			else
				out_valueItem->value_type = UNDEFINED;
		}
		else if (ki->is_array_value)
		{
			out_valueItem->arrayValues = (vector<array_value>*) &ki->arrayValues;
			out_valueItem->value_type = ARRAY;
		}
		else if (ki->next != NULL)
		{
			out_valueItem->jsonObject = (json_object*)ki->next;
			out_valueItem->value_type = OBJECT;
		}
		else
		{
			out_valueItem->value = (char*)ki->value;
			out_valueItem->value_type = STRING;
		}
	}
	else
		return 7;

	return 0;
}