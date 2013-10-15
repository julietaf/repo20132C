/*
 * Orquestador.c
 *
 *  Created on: 04/10/2013
 *      Author: utnso
 */

#include "Orquestador.h"
#include "Planificador.h"

void orquestador(void) {
	configuracion = getConfiguracion();
	logFile = log_create(LOG_PATH, "Orquestador", false,
			configuracion->detalleLog);
	int escuchasfd = sockets_createServer(configuracion->direccionIp,
			configuracion->puerto, configuracion->backlog);
	int sockfdMax = escuchasfd;
	planificadores = dictionary_create();
	int sockfd, nbytes;
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
					aceptarNuevaConexion(sockfd, &bagMaster, &sockfdMax);
				} else {
					nbytes = atenderPedido(sockfd);

					if (nbytes == 0) {
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

void aceptarNuevaConexion(int sockfd, fd_set *bagMaster, int *sockfdMax) {
	int nuevoSockfd = sockets_accept(sockfd);
	header_t header;

	recv(nuevoSockfd, &header, sizeof(header_t), MSG_WAITALL);

	switch (header.type) {
	case HANDSHAKE_PERSONAJE:
		enviarHandshakeOrquestador(nuevoSockfd);
		delegarAlHiloplanificador(nuevoSockfd);
		break;
	case HANDSHAKE_NIVEL:
		enviarHandshakeOrquestador(nuevoSockfd);
		crearNuevoHiloPlanificador(nuevoSockfd);
		agregarSockfd(bagMaster, sockfdMax, nuevoSockfd);
		break;
	}
}

void delegarAlHiloplanificador(int sockfd) {
	header_t header;
	recv(sockfd, &header, sizeof(header), MSG_WAITALL);

	if (header.type == NOTIFICAR_DATOS_PERSONAJE) {
		char *datos = malloc(header.length);
		recv(sockfd, datos, header.length, MSG_WAITALL);
		notificacion_datos_personaje_t *notificacion =
				notificacionDatosPersonaje_deserializer(datos);
		datos_personaje_t *datosPersonaje = malloc(sizeof(datos_personaje_t));
		datosPersonaje->simbolo = notificacion->simbolo;
		datosPersonaje->sockfd = sockfd;
		free(datos);
	}
}

void crearNuevoHiloPlanificador(int sockfd) {
	header_t header;
	recv(sockfd, &header, sizeof(header_t), MSG_WAITALL);

	if (header.type == NOTIFICAR_DATOS_NIVEL) {
		char *nombreNivel = malloc(header.length);
		recv(sockfd, nombreNivel, header.length, MSG_WAITALL);
		datos_planificador_t *datosPlanificador = crearDatosPlanificador(
				nombreNivel, sockfd);
		pthread_create(datosPlanificador->hilo, NULL, (void *) planificador,
				(void *) datosPlanificador);
		dictionary_put(planificadores, datosPlanificador->nombre,
				datosPlanificador);
		free(nombreNivel);
	}
}

datos_planificador_t *crearDatosPlanificador(char *nombre, int sockfdNivel) {
	datos_planificador_t *datosPlanificador = malloc(
			sizeof(datos_planificador_t));
	datosPlanificador->nombre = malloc(strlen(nombre) + 1);
	strcpy(datosPlanificador->nombre, nombre);
	datosPlanificador->sockfdNivel = sockfdNivel;
	datosPlanificador->hilo = malloc(sizeof(pthread_t));
	datosPlanificador->personajesBloqueados = queue_create();
	datosPlanificador->personajesListos = queue_create();
	datosPlanificador->mutexColas = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(datosPlanificador->mutexColas, NULL );

	return datosPlanificador;
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
