/*
 * Planificador.c
 *
 *  Created on: 07/10/2013
 *      Author: utnso
 */

#include "Planificador.h"

void atenderPedidoPlanificador(fd_set *bagEscucha, int sockfdMax,
		datos_planificador_t *datos);
void seleccionarPersonaje(datos_planificador_t *datos);
void seleccionarPorRoundRobin(datos_planificador_t *datosPlan);
void seleccionarPorSRDF(datos_planificador_t *datos);
void enviarTurnoConcedido(datos_personaje_t *personaje);
void notificarNivel(datos_planificador_t *planificador, int type,
		int16_t length, char *data);
datos_personaje_t *buscarPersonajePorSockfd(t_queue *cola, int sockfd);
void atenderPedidoPersonaje(datos_planificador_t *datos, int sockfdPersonaje);
void atenderPedidoNivel(datos_planificador_t *datos);
void gestionarUbicacionCaja(datos_planificador_t *datosPlan,
		datos_personaje_t *personaje, char *objetivo);
void reenviarUbicacionCaja(datos_planificador_t *datosPlan,
		datos_personaje_t *personaje, header_t *header, char *data);

void planificador(datos_planificador_t *datos) {
	fd_set bagEscucha;
	FD_ZERO(&bagMaster);
	FD_SET(datos->sockfdNivel, datos->bagMaster);
	int retval, sockfdMax = datos->sockfdNivel;

	while (1) {
		bagEscucha = datos->bagMaster;

		retval = select(sockfdMax + 1, &bagEscucha, NULL, NULL, NULL );

		if (retval == -1) //Ocurrio un error
			log_error(logFile, "Error en select");
		else if (retval) //Hay un pedido de los procesos conectados
			atenderPedidoPlanificador(&bagEscucha, sockfdMax, datos);
		else if (!queue_is_empty(datos->personajesListos)) //Se produjo el timeout
			seleccionarPersonaje(datos);
	}
}

void atenderPedidoPlanificador(fd_set *bagEscucha, int sockfdMax,
		datos_planificador_t *datos) {
	int sockfd;

	for (sockfd = 0; sockfd <= sockfdMax; sockfd++) {
		if (sockfd == datos->sockfdNivel) {
			atenderPedidoNivel(datos, sockfd);
		} else {
			atenderPedidoPersonaje(datos, sockfd);
		}
	}
}

void seleccionarPersonaje(datos_planificador_t *datos) {
	switch (datos->algoritmo) {
	case ROUND_ROBIN:
		seleccionarPorRoundRobin(datos);
		break;
	case SRDF:
		seleccionarPorSRDF(datos);
		break;
	}
}

void seleccionarPorRoundRobin(datos_planificador_t *datosPlan) {
//TODO: implementar mutex
	int quantumPersonaje = datosPlan->quatum;
	datos_personaje_t *personaje = queue_pop(datosPlan->personajesListos);

	while (quantumPersonaje) {
		enviarTurnoConcedido(personaje);
		header_t header;
		recv(personaje->sockfd, &header, sizeof(header_t), MSG_WAITALL);
		char *data = malloc(header.length);
		recv(personaje->sockfd, data, header.length, MSG_WAITALL);

		switch (header.type) {
		case UBICACION_CAJA:
			reenviarUbicacionCaja(datosPlan, personaje, &header, data);
			gestionarUbicacionCaja(datosPlan, personaje, data);
			break;
		case NOTIFICACION_MOVIMIENTO:
			quantumPersonaje--;
			sockets_send(datosPlan->sockfdNivel, &header, data);
			break;
		case SOLICITAR_RECURSO:
			sockets_send(datosPlan->sockfdNivel, &header, data);
			break;
		}

		free(data);
	}

	if (quantumPersonaje == 0)
		queue_push(datosPlan->personajesListos, personaje);
}

void seleccionarPorSRDF(datos_planificador_t *datos) {

}

void enviarTurnoConcedido(datos_personaje_t *personaje) {
	header_t header;
	header.type = TURNO_CONCEDIDO;
	header.length = 0;
	sockets_send(personaje->sockfd, &header, '\0');
}

void notificarNivel(datos_planificador_t *planificador, int type,
		int16_t length, char *data) {

}

datos_personaje_t *buscarPersonajePorSockfd(t_queue *cola, int sockfd) {
	int _is_personaje(datos_personaje_t *datos) {
		return datos->sockfd == sockfd;
	}

	return list_find(cola->elements, (void *) _is_personaje);
}

void atenderPedidoPersonaje(datos_planificador_t *datos, int sockfdPersonaje) {
	header_t header;

	recv(sockfdPersonaje, &header, sizeof(header), MSG_WAITALL);

	switch (header.type) {
	case FINALIZAR_NIVEL:
		//TODO:Realizar tareas de finalización.
		break;
	}
}

void atenderPedidoNivel(datos_planificador_t *datos) {
	header_t header;

	recv(datos->sockfdNivel, &header, sizeof(header), MSG_WAITALL);

	switch (header.type) {
	case PERSONAJE_FINALIZO:
		//TODO:Realizar tareas de finalización.
		break;
	}
}

void reenviarUbicacionCaja(datos_planificador_t *datosPlan,
		datos_personaje_t *personaje, header_t *header, char *data) {
	memcpy(&personaje->objetivo, data, sizeof(char));
	sockets_send(datosPlan->sockfdNivel, header, data);
}

void gestionarUbicacionCaja(datos_planificador_t *datosPlan,
		datos_personaje_t *personaje, char *objetivo) {
	header_t header;

	recv(datosPlan->sockfdNivel, &header, sizeof(header), MSG_WAITALL);

	if (header.type == UBICACION_CAJA) {
		char *respuesta = malloc(header.length);
		recv(datosPlan->sockfdNivel, respuesta, header.length, MSG_WAITALL);
		personaje->coordObjetivo = coordenadas_deserializer(respuesta);
		sockets_send(personaje->sockfd, &header, respuesta);
		free(respuesta);
	}
}
