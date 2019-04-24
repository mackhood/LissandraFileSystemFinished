#ifndef LISSANDRA_H_
#define LISSANDRA_H_

#include "LFLExternals.h"
#include "FileSistem.h"

/* ESTRUCTURAS */

t_list * hilos;
t_list * compactadores;
t_list * memorias;
t_list * memtable;




typedef struct {
	t_keysetter keysetter;
	char* tabla;
}t_memtablekey;


/*VARIABLES GLOBALES*/

int socket_escucha;
int socketFD;
int retardo;
char* server_ip;
int server_puerto;

/* FUNCIONES */

void mainLissandra();
void iniciarServidor();
void escucharMemoria();
void setearValoresLissandra(t_config * archivoConfig);
void insertKeysetter(char* tabla, uint16_t key, char* value, time_t timestamp);
t_keysetter selectKey(char* tabla, uint16_t receivedKey);
int llamadoACrearTabla(char* nombre, char* consistencia, int particiones, int tiempoCompactacion);
int llamarEliminarTabla(char* tablaPorEliminar);

#endif /* LISSANDRA_H_ */
