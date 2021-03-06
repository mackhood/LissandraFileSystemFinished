#ifndef CONFIG_MEMORIA_H_
#define CONFIG_MEMORIA_H_

#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include "../../SharedLibrary/loggers.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/config.h>

//Tamaño de la ip
#define IP_SIZE (sizeof(char) * (strlen("123.000.000.001") + 1))


typedef struct{

	char* ip_memoria;
	int puerto;
	char* ip_fs;
	int puerto_fs;
	char** ip_seeds;
	char** puerto_seeds;
	int retardo_mp;
	int retardo_fs;
	int tamanio_mem;
	int tiempo_jour;
	int tiempo_goss;
	int numero_memoria;

}t_info;


t_config *conf_mapeada;
t_info info_memoria;

void levantar_config(char* PATH_CONFIG);

#endif /* CONFIG_MEMORIA_H_ */
