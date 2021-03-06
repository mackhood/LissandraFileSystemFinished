/*
 * LFLExternals.c
 *
 *  Created on: 6 may. 2019
 *      Author: utnso
 */

#include "LFLExternals.h"

int cantidadDeBloquesLibres()
{
	int i = 0;
	int contador = 0;
	for(i = 0; i < blocks; i++)
	{
		if(!bitarray_test_bit(bitarray, i))
			contador++;
	}
	return contador;
}

int cantidadDeBloquesAOcupar()
{
	int totalBlocks = 0;
	int counterTables = 0;
	while(NULL != list_get(tablasEnEjecucion, counterTables))
	{
		t_TablaEnEjecucion* tabla = list_get(tablasEnEjecucion, counterTables);
		bool esDeTabla(t_Memtablekeys* key)
		{
			return (key->tabla == tabla->tabla);
		}
		int counterKeys = 0;
		int blocksForTable = 0;
		int sizeOfKeys = 0;
		t_list* auxMemtable = list_filter(memtable, (void*)esDeTabla);
		while(NULL != list_get(auxMemtable, counterKeys))
		{
			t_Memtablekeys* keyAux = list_get(auxMemtable, counterKeys);
			char* auxbT = string_from_format("%lf", keyAux->data->timestamp);
			char* auxT = string_substring_until(auxbT, strlen(auxbT) - 7);
			char* auxK = string_itoa(keyAux->data->key);
			int sizeOfKey = strlen(auxT) + strlen(auxK) + strlen(keyAux->data->clave);
			sizeOfKeys += sizeOfKey;
			counterKeys++;
			free(auxbT);
			free(auxT);
			free(auxK);
			free(keyAux);
		}
		list_destroy(auxMemtable);
		blocksForTable = (sizeOfKeys/8) + 1;
		totalBlocks += blocksForTable;
		counterTables++;
	}
	return totalBlocks;
}

char* timeForLogs()
{
	time_t now;
	struct tm ts;
	char* buf = malloc(80);

	time(&now);

	ts = *localtime(&now);
	strftime(buf, 80, "%d-%m %H:%M:%S", &ts);
	return buf;
}

int chequearTimestamps(t_Memtablekeys* key1, t_Memtablekeys* key2)
{
	return (key1->data->timestamp > key2->data->timestamp);
}

int chequearTimeKey(t_keysetter* key1, t_keysetter* key2)
{
	return (key1->timestamp > key2->timestamp);
}

int obtenerTamanioArchivoConfig(char* direccionArchivo)
{
	t_config* archivo = config_create(direccionArchivo);
	int tamanio = config_get_int_value(archivo, "SIZE");
	config_destroy(archivo);
	return tamanio;
}

t_keysetter* construirKeysetter(char* timestamp, char* key, char* value)
{
	t_keysetter* theReturn = (t_keysetter *)malloc(sizeof(t_keysetter));
	theReturn->timestamp = (double)atoll(timestamp);
	theReturn->key = atoi(key);
	theReturn->clave = string_duplicate(value);
	return theReturn;
}

unsigned long obtenerTamanioArchivo(char* direccionArchivo)
{
	FILE* FP = fopen(direccionArchivo, "r+");
	fseek(FP, 0, SEEK_END);
	unsigned long tamanio = (unsigned long)ftell(FP);
	fclose(FP);
	return tamanio;
}

t_list* parsearKeys(t_list* clavesAParsear)
{
	t_list* clavesPostParseo = list_create();
	int parserListPointer = 0;
	char* keyHandler;
	char* key;
	char* value;
	char* timestamp;
	while((keyHandler = list_get(clavesAParsear, parserListPointer)) != NULL)
	{
		int parserPointer = 0;
		int handlerSize = strlen(keyHandler);
		key = malloc(7);
		value = malloc(tamanio_value + 1);
		timestamp = malloc(17);
		int status = 0;
		int k = 1;
		int v = 1;
		int t = 1;
		while(parserPointer < handlerSize)
		{
			switch(keyHandler[parserPointer])
			{
			case ';':
			{
				parserPointer++;
				status++;
				break;
			}
			case '\n':
			{
				t_keysetter* helpingHand = construirKeysetter(timestamp, key, value);
				list_add(clavesPostParseo, helpingHand);
				parserPointer++;
				status = 0;
				if(parserPointer != handlerSize)
				{
					free(key); free(value); free(timestamp);
					key = malloc(7);
					value = malloc(tamanio_value + 1);
					timestamp = malloc(17);
				}
				k = 1;
				v = 1;
				t = 1;
				break;
			}
			default:
			{
				char* aux = malloc(2);
				aux[0] = keyHandler[parserPointer];
				aux[1] = '\0';
				switch(status)
				{
				case 0:
				{
					if(t)
					{
						strcpy(timestamp, aux);
						t = 0;
					}
					else
						string_append(&timestamp, aux);
					break;
				}
				case 1:
				{
					if(k)
					{
						strcpy(key, aux);
						k = 0;
					}
					else
						string_append(&key, aux);
					break;
				}
				case 2:
				{
					if(v)
					{
						strcpy(value, aux);
						v = 0;
					}
					else
						string_append(&value, aux);
					break;
				}
				}
				free(aux);
				parserPointer++;
				break;
			}
			}
		}

		//		char** keys = string_split(keyHandler, "\n");
		//		int a = 0;
		//		while(keys[a] != NULL)
		//		{
		//			char** key = string_split(keys[a], ";");
		//			key[2][strlen(key[2])] = '\0';
		//			t_keysetter* helpingHand = construirKeysetter(key[0], key[1], key[2]);
		//			list_add(clavesPostParseo, helpingHand);
		//			liberadorDeArrays(key);
		//			a++;
		//		}
		//		liberadorDeArrays(keys);
		parserListPointer++;
		free(key); free(value); free(timestamp);
	}
	free(keyHandler);
	return clavesPostParseo;
}

t_list* inversaParsearKeys(t_list* clavesADesparsear)
{
	t_list* clavesParseadas = list_create();
	int a = 0;
	while(NULL != list_get(clavesADesparsear, a))
	{
		t_keysetter* keyAuxiliar = list_get(clavesADesparsear, a);
		char* aux = string_from_format("%lf", keyAuxiliar->timestamp);
		char* auxk = string_itoa(keyAuxiliar->key);
		char* keyConvertida = malloc(strlen(auxk) + strlen(aux) + strlen(keyAuxiliar->clave) + 3);
		char* auxb = string_substring_until(aux, strlen(aux) - 7);
		strcpy(keyConvertida, auxb);
		string_append(&keyConvertida, ";");
		string_append(&keyConvertida, auxk);
		string_append(&keyConvertida, ";");
		string_append(&keyConvertida, keyAuxiliar->clave);
		string_append(&keyConvertida, "\n");
		free(aux);
		free(auxb);
		free(auxk);
		list_add(clavesParseadas, keyConvertida);
		a++;
	}
	return clavesParseadas;
}

int cantBloquesFS(char* direccion)
{
	DIR* directorioDeBloques;
	struct dirent* blockdir;
	directorioDeBloques = opendir(direccion);
	int contadorDeBloques = 0;
	while(NULL != (blockdir = readdir(directorioDeBloques)))
	{
		if(!strcmp(blockdir->d_name, ".") || !strcmp(blockdir->d_name, "..")){}
		else
		{
			contadorDeBloques++;
		}
	}
	closedir(directorioDeBloques);
	return contadorDeBloques;
}

void liberadorDeKeys(t_keysetter* keysetter)
{
	free(keysetter->clave);
	free(keysetter);
}

void liberadorDeMemtableKeys(t_Memtablekeys* memtableKey)
{
	liberadorDeKeys(memtableKey->data);
	free(memtableKey);
}

void liberadorDeListasDeKeys(t_list* listaDeKeys)
{
	int a = 0;
	while(NULL != list_get(listaDeKeys, a))
	{
		t_keysetter* auxiliarium = list_remove(listaDeKeys, a);
		liberadorDeKeys(auxiliarium);
		a++;
	}
	list_destroy(listaDeKeys);
}


