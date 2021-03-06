#include "Lissandra.h"

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024* (EVENT_SIZE + 16))

void inicializar()
{
	memtable = list_create();
	killthreads = false;
	initNotifier();
	iniciarServidor();
}

void initNotifier()
{
	pthread_t notifierHandler;
	logInfo("[Lissandra]: Se inicio un hilo para manejar el notifier.");
	pthread_create(&notifierHandler, NULL, (void *) notifier, NULL);
	pthread_detach(notifierHandler);
}

void setearValoresLissandra(t_config * archivoConfig)
{
	retardo = config_get_int_value(archivoConfig, "RETARDO");
	server_puerto = config_get_int_value(archivoConfig, "PUERTO_ESCUCHA");
	server_ip = string_duplicate(config_get_string_value(archivoConfig,"IP_FILE_SYSTEM"));
	tamanio_value = config_get_int_value(archivoConfig, "TAMANIO_VALUE");
}

void mainLissandra ()
{
	inicializar();
}

void iniciarServidor()
{
	socket_escucha = levantar_servidor(server_puerto);

	int socket_memoria;
	struct sockaddr_in direccion_cliente;
	unsigned int tamanio_direccion = sizeof(direccion_cliente);

	//Se aceptan clientes cuando los haya
	// accept es una funcion bloqueante, si no hay ningun cliente esperando ser atendido, se queda esperando a que venga uno.
	while(  (socket_memoria = accept(socket_escucha, (void*) &direccion_cliente, &tamanio_direccion)) > 0)
	{
		puts("Se ha conectado una memoria");
		logInfo( "[Lissandra]: Se conecto una Memoria");
		pthread_t RecibirMensajesMemoria;
		/*Duplico la variable que tiene el valor del socket del cliente*/
		int* memoria = (int*) malloc (sizeof(int));
		*memoria = socket_memoria;
		pthread_create(&RecibirMensajesMemoria,NULL, (void*)escucharMemoria, memoria);
	}
}

void escucharMemoria(int* socket_memoria)
{
	int socket = *socket_memoria;
	bool memoriaDesconectada = false;
	while(!killthreads)
	{
		t_prot_mensaje* mensaje_memoria = prot_recibir_mensaje(socket);
		switch(mensaje_memoria->head)
		{
			case HANDSHAKE:
			{
				logInfo( "[Lissandra]: me llegó un handshake de una Memoria, procedo a enviar los datos necesarios");
//				bool solicitadoPorMemoria = true;
//				char* buffer = string_new();
//				if(0 == describirTablas("", solicitadoPorMemoria, buffer))
//				{
//					size_t tamanioBuffer = strlen(buffer);
//					void* messageBuffer = malloc(tamanioBuffer + 1);
//					memcpy(messageBuffer, buffer, strlen(buffer));
//					prot_enviar_mensaje(socket, FULL_DESCRIBE, tamanioBuffer, messageBuffer);
//					logInfo("Lissandra: Se ha enviado la metadata de todas las tablas a Memoria.");
//				}
//				else
//				{
//					prot_enviar_mensaje(socket, FAILED_DESCRIBE, 0, NULL);
//					logError("Lissandra: falló al leer todas las tablas");
//				}
//				free(buffer);

				size_t tamanioBuffer = sizeof(int);
				void* messageBuffer = malloc(tamanioBuffer + 1);
				memcpy(messageBuffer, &tamanio_value, sizeof(int));
				prot_enviar_mensaje(socket, HANDSHAKE, tamanioBuffer, messageBuffer);
				logInfo("[Lissandra]: Se ha enviado la información de saludo a la Memoria.");
				free(messageBuffer);
				break;
			}
			case SOLICITUD_TABLA:
			{
				logInfo( "[Lissandra]: Nos llega un pedido de Select de parte de la Memoria");
				puts("Llegó un select desde memoria.");
				uint16_t auxkey;
				int tamanioNombre;
				memcpy(&auxkey, mensaje_memoria->payload, sizeof(uint16_t));
				memcpy(&tamanioNombre, mensaje_memoria->payload + sizeof(uint16_t), sizeof(int));
				char* tabla = malloc(tamanioNombre + 1);
				memcpy(tabla, mensaje_memoria->payload + sizeof(uint16_t) + sizeof(int), tamanioNombre);
				tabla[tamanioNombre] = '\0';

				 t_keysetter* helpinghand = selectKey(tabla, auxkey);
				 if(helpinghand != NULL)
				 {
					double tiempo_pag = helpinghand->timestamp;
					char* value = string_duplicate(helpinghand->clave);
					int tamanio_value = strlen(value);
					size_t tamanio_buffer = (sizeof(double)+tamanio_value+sizeof(int));
					void* buffer = malloc(tamanio_buffer);

					memcpy(buffer, &tiempo_pag, sizeof(double));
					memcpy(buffer+sizeof(double), &tamanio_value, sizeof(int));
					memcpy(buffer+sizeof(double)+sizeof(int), value, tamanio_value);

					prot_enviar_mensaje(socket, VALUE_SOLICITADO_OK, tamanio_buffer, buffer);
					logInfo("[Lissandra]: se envía a Memoria el value de la key %i", helpinghand->key);
					free(buffer);
				 }
				 else
				 {
				 	 prot_enviar_mensaje(socket, VALUE_FAILURE, 0, NULL);
				 }

				break;
			}
			case CREATE_TABLA:
			{
				logInfo( "[Lissandra]: Llega un pedido de Create de parte de Memoria");
				puts("Llegó un Create desde memoria.");
				int cantParticionesRecibida;
				int tiempoEntreCompactacionesRecibido;
				int tamanioNombreTabla;
				int tamanioConsistencia;
				memcpy(&tamanioNombreTabla, mensaje_memoria->payload, sizeof(int));
				char* tablaRecibida = malloc(tamanioNombreTabla + 1);
				memcpy(tablaRecibida, mensaje_memoria->payload + sizeof(int), tamanioNombreTabla);
				tablaRecibida[tamanioNombreTabla] = '\0';
				memcpy(&tamanioConsistencia, mensaje_memoria->payload + sizeof(int) + tamanioNombreTabla, sizeof(int));
				char* consistenciaRecibida = malloc(tamanioConsistencia + 1);
				memcpy(consistenciaRecibida, mensaje_memoria->payload + sizeof(int) + tamanioNombreTabla +sizeof(int),
						tamanioConsistencia);
				consistenciaRecibida[tamanioConsistencia] = '\0';
				memcpy(&cantParticionesRecibida, mensaje_memoria->payload + sizeof(int) + tamanioNombreTabla +sizeof(int)
						+ tamanioConsistencia, sizeof(int));
				memcpy(&tiempoEntreCompactacionesRecibido, mensaje_memoria->payload + sizeof(int) + tamanioNombreTabla
						+ sizeof(int) + tamanioConsistencia + sizeof(int), sizeof(int));
				switch(llamadoACrearTabla(tablaRecibida, consistenciaRecibida, cantParticionesRecibida, tiempoEntreCompactacionesRecibido))
				{
					case 0:
					{
						logInfo( "[Lissandra]: Tabla creada satisfactoriamente a pedido de Memoria");
						prot_enviar_mensaje(socket, TABLA_CREADA_OK, 0, NULL);
						break;
					}
					case 2:
					{
						logInfo( "[Lissandra]: Tabla ya existía, lamentablemente para Memoria");
						prot_enviar_mensaje(socket, TABLA_CREADA_YA_EXISTENTE, 0, NULL);
						break;
					}
					case 5:
					{
						logInfo("[Lissandra]: se informa a memoria de que el FS no tiene más espacio");
						prot_enviar_mensaje(socket, FILE_SYSTEM_FULL, 0, NULL);
					}
					default:
					{
						logError( "[Lissandra]: La tabla o alguna de sus partes no pudo ser creada, informo a Memoria");
						prot_enviar_mensaje(socket, TABLA_CREADA_FALLO, 0, NULL);
						break;
					}
				}
				break;
			}
			case TABLE_DROP:
			{
				puts("Llegó un Drop desde memoria.");
				char* tablaRecibida;
				int tamanioNombreTabla;
				memcpy(&tamanioNombreTabla, mensaje_memoria->payload, sizeof(int));
				tablaRecibida = malloc(tamanioNombreTabla + 1);
				memcpy(tablaRecibida, mensaje_memoria->payload + sizeof(int), tamanioNombreTabla);
				tablaRecibida[tamanioNombreTabla] = '\0';
				int results = llamarEliminarTabla(tablaRecibida);
				switch(results)
				{
					case 1:
					{
						prot_enviar_mensaje(socket, TABLE_DROP_NO_EXISTE, 0, NULL);
						logError( "[Lissandra]: La %s no existe y no se puede eliminar.", tablaRecibida);
						break;
					}
					case 0:
					{
						prot_enviar_mensaje(socket, TABLE_DROP_OK, 0, NULL);
						logInfo( "[Lissandra]: La %s fue eliminada correctamente", tablaRecibida);
						break;
					}
					default:
					{
						prot_enviar_mensaje(socket, TABLE_DROP_FALLO, 0, NULL);
						logError( "[Lissandra]: la operacion no fue terminada por un fallo en acceder a la %s", tablaRecibida);
						break;
					}
				}
				free(tablaRecibida);
				break;
			}
			case DESCRIBE:
			{
				puts("Llegó un Describe desde memoria.");
				bool solicitadoPorMemoria = true;
				if(mensaje_memoria->tamanio_total > 8)
				{
					char* tablaRecibida;
					int tamanioNombreTabla;
					memcpy(&tamanioNombreTabla, mensaje_memoria->payload, sizeof(int));
					tablaRecibida = malloc(tamanioNombreTabla + 1);
					memcpy(tablaRecibida, mensaje_memoria->payload + sizeof(int), tamanioNombreTabla);
					tablaRecibida[tamanioNombreTabla] = '\0';
					char* buffer;
					if(!strcmp((buffer = describirTablas(tablaRecibida, solicitadoPorMemoria)), "1"))
					{
						prot_enviar_mensaje(socket, FAILED_DESCRIBE, 0, NULL);
						logError( "[Lissandra]: fallo al leer la %s", tablaRecibida);
					}
					else
					{
						int tamanio_buffer = strlen(buffer);
						size_t tamanioBuffer = sizeof(int) + tamanio_buffer;
						void* messageBuffer = malloc(tamanioBuffer + 1);
						memcpy(messageBuffer, &tamanio_buffer, sizeof(int));
						memcpy(messageBuffer + sizeof(int), buffer, strlen(buffer));
						prot_enviar_mensaje(socket, POINT_DESCRIBE, tamanioBuffer, messageBuffer);
						logInfo( "[Lissandra]: Se ha enviado la metadata de la %s a Memoria.", tablaRecibida);
						free(messageBuffer);
					}
					free(tablaRecibida);
					free(buffer);
				}
				else
				{
					char* buffer;
					if(!strcmp((buffer = describirTablas("", solicitadoPorMemoria)), "1"))
					{
						prot_enviar_mensaje(socket, FAILED_DESCRIBE, 0, NULL);
						logError( "[Lissandra]: falló al leer todas las tablas");
					}
					else
					{
						size_t tamanioBuffer = sizeof(int) + strlen(buffer);
						void* messageBuffer = malloc(tamanioBuffer + 1);
						int tamanio_buffer = strlen(buffer);
						memcpy(messageBuffer, &tamanio_buffer, sizeof(int));
						memcpy(messageBuffer + sizeof(int), buffer, tamanio_buffer);
						prot_enviar_mensaje(socket, FULL_DESCRIBE, tamanioBuffer, messageBuffer);
						logInfo( "[Lissandra]: Se ha enviado la metadata de todas las tablas a Memoria.");
						free(messageBuffer);
						free(buffer);
					}
				}
				break;
			}
			case JOURNALING_INSERT:
			{
				puts("Llegó un Insert desde memoria.");
				uint16_t keyRecibida;
				double timestampRecibido;
				int tamanioNombreTabla;
				int tamanioValue;
				memcpy(&timestampRecibido, mensaje_memoria->payload, sizeof(double));
				memcpy(&tamanioNombreTabla, mensaje_memoria->payload + sizeof(double), sizeof(int));
				char* tablaRecibida = malloc(tamanioNombreTabla + 1);
				memcpy(tablaRecibida, mensaje_memoria->payload + sizeof(double) + sizeof(int), tamanioNombreTabla);
				tablaRecibida[tamanioNombreTabla] = '\0';
				memcpy(&keyRecibida, mensaje_memoria->payload + sizeof(double) + sizeof(int) + tamanioNombreTabla
						, sizeof(uint16_t));
				memcpy(&tamanioValue, mensaje_memoria->payload + sizeof(double) + sizeof(int) + tamanioNombreTabla + sizeof(uint16_t)
						, sizeof(int));
				char* valueRecibido = malloc(tamanioValue + 1);
				memcpy(valueRecibido, mensaje_memoria->payload + sizeof(double) + sizeof(int) + tamanioNombreTabla + sizeof(uint16_t)
						+ sizeof(int), tamanioValue);
				valueRecibido[tamanioValue] = '\0';

				switch(insertKeysetter(tablaRecibida, keyRecibida, valueRecibido, timestampRecibido))
				{
					case 0:
					{
						prot_enviar_mensaje(socket, INSERT_SUCCESSFUL, 0, NULL);
						break;
					}
					case 2:
					{
						prot_enviar_mensaje(socket, INSERT_FAILED_ON_MEMTABLE, 0, NULL);
						break;
					}
					default:
					{
						prot_enviar_mensaje(socket, INSERT_FAILURE, 0, NULL);
						break;
					}
				}
				break;
			}
			case DESCONEXION:
			{
				puts("Una memoria ha sido desconectada.");
				memoriaDesconectada = true;
				break;
			}
			default:
			{
				break;
			}
		}
		prot_destruir_mensaje(mensaje_memoria);
		if(memoriaDesconectada)
		{
			logInfo("[Lissandra]: Una memoria se ha desconectado");
			break;
		}
		usleep(retardo * 1000);
	}
	if(!memoriaDesconectada)
	{
		prot_enviar_mensaje(socket, GOODBYE, 0, NULL);
		close(socket);
	}
}

int insertKeysetter(char* tablaRecibida, uint16_t keyRecibida, char* valueRecibido, double timestampRecibido)
{
	t_Memtablekeys* auxiliar = (t_Memtablekeys *)malloc(sizeof(t_Memtablekeys));
	t_keysetter* auxiliarprima = (t_keysetter *)malloc(sizeof(t_keysetter));
	auxiliarprima->key = keyRecibida;
	auxiliarprima->clave = string_duplicate(valueRecibido);
	auxiliarprima->timestamp = timestampRecibido;
	auxiliar->tabla = string_duplicate(tablaRecibida);
	auxiliar->data = auxiliarprima;

	printf("\033[1;34m");
	printf("%i, %s,", auxiliar->data->key, auxiliar->tabla);
	printf(" %s, %lf\n", auxiliar->data->clave, auxiliar->data->timestamp);
	if(0 == existeTabla(tablaRecibida))
	{
		logError( "[Lissandra]: La tabla no existe, por lo que no puede insertarse una clave.");
		printf("Tabla no existente.\n");
		printf("\033[1;36m");
		return 1;
	}
	else
	{
		if(strlen(valueRecibido) > tamanio_value)
		{
			logError("[Lissandra]: el value a agregar era demasiado grande");
			printf("El value ingresado era demasiado grande, por favor, ingrese uno más pequeño.\n");
			printf("\033[1;36m");
			return 3;
		}
		pthread_mutex_lock(&dumpEnCurso);
		tamanio_memtable = memtable->elements_count;
		logInfo( "[Lissandra]: Se procede a insertar la clave recibida en la Memtable.");
		list_add(memtable, auxiliar);
		if(tamanio_memtable == memtable->elements_count)
		{
			pthread_mutex_unlock(&dumpEnCurso);
			logError( "[Lissandra]: La clave fracasó en su intento de insertarse correctamente.");
			printf("Fallo al agregar a memtable.\n");
			printf("\033[1;36m");
			return 2;
		}
		else
		{
			pthread_mutex_unlock(&dumpEnCurso);
			logInfo( "[Lissandra]: La clave fue insertada correctamente.");
			printf("Agregado correctamente.\n");
			printf("\033[1;36m");
			return 0;
		}
	}
	tamanio_memtable = 0;
}

t_keysetter* selectKey(char* tabla, uint16_t receivedKey)
{
	int perteneceATabla(t_Memtablekeys* key)
	{
		return !strcmp(key->tabla, tabla);
	}

	int esDeTalKey(t_Memtablekeys* chequeada)
	{
		return chequeada->data->key == receivedKey;
	}

		if(existeTabla(tabla))
		{
			t_list* keysDeTablaPedida;
			t_list* keyEspecifica;
			t_Memtablekeys* auxMemtable;
			t_keysetter* key;
			pthread_mutex_lock(&dumpEnCurso);
			keysDeTablaPedida = list_filter(memtable, (void*)perteneceATabla);
			keyEspecifica = list_filter(keysDeTablaPedida, (void*)esDeTalKey);
			if(!list_is_empty(keyEspecifica))
			{
				list_sort(keyEspecifica, (void*)chequearTimestamps);
				auxMemtable = list_get(keyEspecifica, 0);
				t_keysetter* keyTemps = selectKeyFS(tabla, receivedKey);
				if(keyTemps != NULL)
				{
					if(chequearTimeKey(keyTemps, auxMemtable->data))
					{
						key = keyTemps;
					}
					else
					{
						key = auxMemtable->data;
						liberadorDeKeys(keyTemps);
					}
				}
				else
				{
					key = auxMemtable->data;
					free(keyTemps);
				}
			}
			else
			{
				t_keysetter* keyTemps = selectKeyFS(tabla, receivedKey);
				if(keyTemps != NULL)
				{
					key = keyTemps;
				}
				else
				{
					puts("La key que usted solicitó no existe en el File System.");
					logError("[Lissandra]: Clave inexistente en el FS.");
					key = NULL;
					pthread_mutex_unlock(&dumpEnCurso);
					return key;
				}
			}
			pthread_mutex_unlock(&dumpEnCurso);
			list_destroy(keysDeTablaPedida);
			list_destroy(keyEspecifica);
			logInfo("[Lissandra]: se ha obtenido la clave más actualizada en el proceso.");
			return key;
		}
		else
		{

			printf("La tabla que usted quiso acceder no existe dentro del File System.\n");
			logError("[Lissandra]: Tabla inexistente en el FS.");
			t_keysetter* key = NULL;
			return key;
		}

}

int llamadoACrearTabla(char* nombre, char* consistencia, int particiones, int tiempoCompactacion)
{
	switch (crearTabla(nombre, consistencia, particiones, tiempoCompactacion))
	{
		case 0:
		{
			logInfo("[Lissandra]: se ha pedido al Compactador que inicie un hilo para gestionar la %s", nombre);
			gestionarTabla(nombre);
			return 0;
			break;
		}
		case 1:
		{
			return 1;
			break;
		}
		case 2:
		{
			return 2;
			break;
		}
		case 5:
		{
			logError("[Lissandra]: Se solicitaron más bloques de los disponibles actualmente en el FS.");
			return 5;
			break;
		}
		default:
			return 1;
		break;
	}
}

int llamarEliminarTabla(char* tablaPorEliminar)
{
	bool estaTabla(t_TablaEnEjecucion* tablaDeLista)
	{
		char* tablaAux = malloc(strlen(tablaPorEliminar) + 1);
		strcpy(tablaAux, tablaPorEliminar);
		int cantCarac = strlen(tablaDeLista->tabla);
		//char* tablaDeListaAux = string_new();
		char* tablaDeListaAux = malloc(cantCarac + 1);
		strcpy(tablaDeListaAux, tablaDeLista->tabla);
		bool result = (0 == strcmp(tablaDeListaAux, tablaAux));
		free(tablaDeListaAux);
		free(tablaAux);
		return result;
	}
	int result = dropTable(tablaPorEliminar);
	switch(result)
	{
	case 0:
	{
		t_TablaEnEjecucion * tabla = list_find(tablasEnEjecucion, (void*) estaTabla);
		pthread_mutex_lock(&tabla->dropPendiente);
		pthread_mutex_lock(&modifierTablasEnCurso);
		list_remove_by_condition(tablasEnEjecucion, (void*) estaTabla);
		pthread_mutex_unlock(&modifierTablasEnCurso);
		logInfo("[Lissandra]: Se ha removido a la %s de la lista de tablas en ejecución", tablaPorEliminar);
		pthread_mutex_unlock(&tabla->dropPendiente);
		break;
	}
	default:
		break;
	}
	return result;
}

char* describirTablas(char* tablaSolicitada, bool solicitadoPorMemoria)
{
	char* buffer;
	if(0 == strcmp(tablaSolicitada, ""))
	{
		logInfo( "[Lissandra]: Me llega un pedido de describir todas las tablas");
		int tablasExistentes = contarTablasExistentes();
		if(tablasExistentes == 0)
		{
			logError( "[Lissandra]: No existe ningún directorio en el FileSystem");
			printf("No existe ninguna tabla.");
			char* errormarker = "error";
			buffer = malloc(6);
			memcpy(buffer, errormarker, strlen(errormarker));
			return "1";
		}
		else
		{
			if(!solicitadoPorMemoria)
			{
				t_list* ignoredList = mostrarTodosLosMetadatas(solicitadoPorMemoria);
				list_destroy_and_destroy_elements(ignoredList, &free);
				return "0";
			}
			else
			{
				t_list* listedTables = mostrarTodosLosMetadatas(solicitadoPorMemoria);
				int parserList = 0;
				int totalSize = 0;
				if(list_is_empty(listedTables))
				{
					return "1";
				}
				while(NULL != list_get(listedTables, parserList))
				{
					char* table = list_get(listedTables, parserList);
					totalSize += strlen(table);
					parserList++;
				}
				parserList = 0;
				buffer = malloc(totalSize + 1);
				while(NULL != list_get(listedTables, parserList))
				{
					char* table = list_get(listedTables, parserList);
					if(parserList == 0)
						strcpy(buffer,table);
					else
						string_append(&buffer, table);
					parserList++;
				}
				list_destroy_and_destroy_elements(listedTables, &free);
				return buffer;
			}
		}
	}
	else
	{
		char* auxbuffer = malloc(strlen(tablaSolicitada) + 1);
		logInfo("[Lissandra]: Me llega un pedido de describir la tabla %s", tablaSolicitada);
		if(solicitadoPorMemoria)
		{
			strcpy(auxbuffer, tablaSolicitada);
			string_append(&auxbuffer, ",");
			auxbuffer = mostrarMetadataEspecificada(tablaSolicitada, solicitadoPorMemoria);
			int sizebuffer = strlen(auxbuffer);
			buffer = malloc(sizebuffer + 1);
			strcpy(buffer, auxbuffer);
			free(auxbuffer);
			return buffer;
		}
		else
		{
			auxbuffer = mostrarMetadataEspecificada(tablaSolicitada, solicitadoPorMemoria);
			free(auxbuffer);
			return "0";
		}
	}
}

void notifier()
{
	int length;
	int i = 0;
	char buffer[BUF_LEN];

	fileToWatch = inotify_init();
	if( fileToWatch < 0)
	{
		logError("[Lissandra]: Error al iniciar el notifier.");
		pthread_mutex_unlock(&deathProtocol);
		return;
	}

	watchDescriptor = inotify_add_watch(fileToWatch, lissandraFL_config_ruta, IN_CREATE | IN_MODIFY | IN_DELETE | IN_DELETE_SELF);
	length = read(fileToWatch, buffer, BUF_LEN);

	if(length < 0)
	{
		logError("[Lissandra]: Error al tratar de acceder al archivo a vigilar.");
		pthread_mutex_unlock(&deathProtocol);
		return;
	}

	while(i < length)
	{
		struct inotify_event* event = (struct inotify_event*) &buffer[i];
		if(event->mask & IN_MODIFY)
		{
			sleep(2);
			logInfo("[Lissandra]: se ha modificado el archivo de configuración, actualizando valores.");
			t_config* ConfigMain = config_create(lissandraFL_config_ruta);
			tiempoDump = config_get_int_value(ConfigMain, "TIEMPO_DUMP");
			retardo = config_get_int_value(ConfigMain, "RETARDO");
			if(tiempoDump > slowestCompactationInterval)
			{
				slowestCompactationInterval = tiempoDump;
			}
			config_destroy(ConfigMain);
			logInfo("[Lissandra]: valores actualizados.");
			printf("\033[1;34m");
			puts("Al detectarse un cambio en el archivo de configuración, se actualizaron los valores del FS.");
			printf("\033[1;36m");
		}
		else if(event->mask & IN_DELETE_SELF)
		{
			logInfo("[Lissandra]: Se ha detectado que el archivo de config ha sido eliminado. Terminando sistema.");
			printf("\033[1;34m");
			puts("El FileSystem está siendo desconectado ya que el archivo de configuración fue destruido.");
			printf("\033[1;36m");
			pthread_mutex_unlock(&deathProtocol);
			break;
		}
		else if(event->mask & IN_CREATE)
		{
			logInfo("[Lissandra]: Se ha detectado el archivo de configuración");
		}
		length = read(fileToWatch, buffer, BUF_LEN);
	}
	(void) inotify_rm_watch(fileToWatch, watchDescriptor);
	(void) close(fileToWatch);
}

void killProtocolLissandra()
{
	killthreads = true;
	free(server_ip);
	(void) inotify_rm_watch(fileToWatch, watchDescriptor);
	(void) close(fileToWatch);
	logInfo("[Lissandra]: Todas las memorias han sido desalojadas.");
}

