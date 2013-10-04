/*
 * Orquestador.c
 *
 *  Created on: 04/10/2013
 *      Author: utnso
 */

#include "Orquestador.h"

void orquestador(void) {
	configuracion = getConfiguracion();
	logFile = log_create(LOG_PATH, "Orquestador", false,
			configuracion->detalleLog);
	int escuchasfd = sockets_createServer(configuracion->direccionIp,
			configuracion->puerto, configuracion->backlog);
	int sockfdMax = escuchasfd;
	int sockfd, nuevoSockfd, nbytes;
	fd_set bagMaster, bagTemp;
	FD_ZERO(&bagMaster);
	FD_ZERO(&bagTemp);
	FD_SET(escuchasfd, &bagMaster);

	while (1) {
		bagTemp = bagMaster;

		select(sockfdMax + 1, &bagTemp, NULL, NULL, NULL );

		for (sockfd = 0; sockfd <= sockfdMax; sockfd++) {
			if (FD_ISSET(sockfd, &bagTemp)) {
				if (sockfd == escuchasfd) {
					nuevoSockfd = aceptarNuevaConexion(sockfd);
					agregarSockfd(&bagMaster, &sockfdMax, nuevoSockfd);
				} else {
					nbytes = atenderPedido(sockfd);

					if (!nbytes) {
						removerSockfd(&bagMaster, sockfd);
					}
				}
			}
		}
	}
}

int atenderPedido(int sockfd) {
	return 0;
}

int aceptarNuevaConexion(int sockfd) {
	int nuevoSockfd = sockets_accept(sockfd);
	header_t header;

	recv(nuevoSockfd, &header, sizeof(header_t), MSG_WAITALL);

	if (header.type == HANDSHAKE_PERSONAJE) {
		enviarHandshakeOrquestador(nuevoSockfd);
	}

	return nuevoSockfd;
}

int enviarHandshakeOrquestador(int sockfd) {
	header_t header;
	header.type = HANDSHAKE_ORQUESTADOR;
	header.length = 0;

	return sockets_send(sockfd, &header, '\0');
}

void agregarSockfd(fd_set *bagMaster, int *sockfdMax, int sockfd) {
	FD_SET(sockfd, bagMaster);

	if (*sockfdMax < sockfd)
		*sockfdMax = sockfd;
}

void removerSockfd(fd_set *bagMaster, int sockfd) {
	close(sockfd);
	FD_CLR(sockfd, bagMaster);
}

configuracion_plataforma_t *getConfiguracion(void) {
	t_config *configFile = config_create(CONFIG_PATH);
	configuracion_plataforma_t *config = malloc(
			sizeof(configuracion_plataforma_t));

	config->direccionIp = malloc(
			strlen(config_get_string_value(configFile, "IPORQUESTADOR")));
	strcpy(config->direccionIp,
			config_get_string_value(configFile, "IPORQUESTADOR"));
	config->puerto = malloc(
			strlen(config_get_string_value(configFile, "PUERTOORQUESTADOR")));
	strcpy(config->puerto,
			config_get_string_value(configFile, "PUERTOORQUESTADOR"));
	config->backlog = config_get_int_value(configFile, "BACKLOG");
	config->detalleLog = log_level_from_string(
			config_get_string_value(configFile, "DETALLELOG"));

	return config;
}
