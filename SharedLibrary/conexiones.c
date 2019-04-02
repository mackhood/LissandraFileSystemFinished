/*
 * conexiones.c
 *
 *  Created on: 1 abr. 2019
 *      Author: utnso
 */

#include "conexiones.h"

int levantar_servidor(int puerto){
	int socket_escucha;
	crear_socket(&socket_escucha);

	//Codigo que permite hacer pruebas sin que se bloqueen los puertos
	int activado = 1;
	setsockopt(socket_escucha, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado));
    //me permite que los puertos sean reutilizados sin importar si se cerraron o no los sockets anteriormente

	//RESERVO PUERTO

	//Burocracia inicial
	struct sockaddr_in direccion_servidor;
	direccion_servidor.sin_family = AF_INET;
	//necesita un hueco en el sistema operativo, por eso no pasamos ninguna ip en particular
	direccion_servidor.sin_addr.s_addr = INADDR_ANY;
	direccion_servidor.sin_port = htons(puerto);
	//Fin de la burocracia

		//Bind: <CREO> que le pide un puerto al SO, lo reserva para él y le asocia un puerto
	if( bind(socket_escucha, (void*) &direccion_servidor, sizeof(direccion_servidor)) <0){
		close(socket_escucha);
		print_error(ERROR_RESEVAR_PUERTO);
	}


	//ESCUCHO PUERTO

	//Habilita a que los clientes es conecten
	if(listen(socket_escucha, MAX_CLIENTES_ENCOLADOS) <0){
		close(socket_escucha);
		print_error(ERROR_ESCUCHAR_PUERTO);
		exit(VALOR_EXIT);
	}
	return socket_escucha;
}


void print_error(char *mensaje){
	printf("ERROR:\t%s\n", mensaje);
	sleep(TIEMPO_SLEEP);
}


void crear_socket(int *socket_destino){
	*socket_destino = socket(AF_INET, SOCK_STREAM, 0);
	//Si no se puede crear socket retorna -
	if(socket_destino <0){
		close(*socket_destino);
		print_error(ERROR_CREAR_SOCKET);
		exit(VALOR_EXIT); //si no se puede crear se cierra el proceso
	}
}


int conectar_a_servidor(char* ip, int puerto){
	int socket_cliente;
	crear_socket(&socket_cliente);

	//conectando el socket

	//Burocracia inicial
	struct sockaddr_in direccion_servidor;
	direccion_servidor.sin_family = AF_INET;
	direccion_servidor.sin_addr.s_addr = inet_addr(ip);
	direccion_servidor.sin_port = htons(puerto);
	//Fin de la burocracia

	//Conectarse con el IP - Puerto del servidor
	if(connect(socket_cliente, (void*) &direccion_servidor, sizeof(direccion_servidor)) < 0){
		close(socket_cliente);
		print_error(ERROR_CONECTAR_SERVIDOR);
		exit(1);
	}

	//enviamos un mensaje de que se conecto
	/*int numero_a_mandar = 1;

	printf("mando el %d (me conecto)\n", numero_a_mandar);

	prot_enviar_mensaje(socket_cliente, 0, sizeof(int), &numero_a_mandar);
	return socket_cliente;*/
}
