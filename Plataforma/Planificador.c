/*
 * Planificador.c
 *
 *  Created on: 07/10/2013
 *      Author: utnso
 */

#include "Planificador.h"

int atenderPedidoPlanificador(fd_set *bagEscucha, int sockfdMax,
		datos_planificador_t *datos);
void moverPersonaje(datos_planificador_t *datos);
void seleccionarPorRoundRobin(datos_planificador_t *datosPlan);
void seleccionarPorSRDF(datos_planificador_t *datos);
void enviarTurnoConcedido(datos_personaje_t *personaje);
void atenderPedidoPersonaje(datos_planificador_t *datos, int sockfdPersonaje);
int atenderPedidoNivel(datos_planificador_t *datos);
void gestionarUbicacionCaja(datos_planificador_t *datosPlan,
		datos_personaje_t *personaje);
void reenviarUbicacionCaja(datos_planificador_t *datosPlan,
		datos_personaje_t *personaje, header_t *header, char *data);
void reenviarNotificacionMovimiento(datos_planificador_t *datosPlan,
		datos_personaje_t *personaje, header_t *header, char *data);
void gestionarNotificacionMovimiento(datos_planificador_t *datosPlan,
		datos_personaje_t *personaje);
void gestionarSolicitudRecurso(datos_planificador_t *datosPlan,
		datos_personaje_t *personaje);
datos_personaje_t *seleccionarCaminoMasCorto(t_queue *personajes);
int actualizarAlgoritmo(header_t *header, datos_planificador_t *datos);
int removerPersonaje(header_t *header, datos_planificador_t *datos);
int notificarMuerteDeadlock(char idPersonaje, datos_planificador_t *datos);
t_list *desbloquearPersonajes(t_list *recursosLiberados,
		datos_planificador_t *datos);
int usarRecurso(char idObjetivo, t_list *recursosLiberados, t_list *recUsados,
		char simboloPersonaje);

void planificador(datos_planificador_t *datos) {
	fd_set bagEscucha;
	FD_ZERO(&bagEscucha);
	FD_SET(datos->sockfdNivel, datos->bagMaster);
	int retval, sockfdMax = datos->sockfdNivel;
	struct timeval timeout;
	int usegundos = (datos->retardo * 1000000);
	int sec = div(usegundos, 1000000).quot;
	int usec = div(usegundos, 1000000).rem;

	while (1) {
		bagEscucha = *datos->bagMaster;
		timeout.tv_sec = sec;
		timeout.tv_usec = usec;
		retval = select(sockfdMax + 1, &bagEscucha, NULL, NULL, &timeout);

		if (retval == -1) //Ocurrio un error
			log_error(logFile, "Error en select");
		else if (retval) //Hay un pedido de los procesos conectados
			atenderPedidoPlanificador(&bagEscucha, sockfdMax, datos);
		else if (!queue_is_empty(datos->personajesListos)) //Se produjo el timeout
			moverPersonaje(datos);
	}
}

int atenderPedidoPlanificador(fd_set *bagEscucha, int sockfdMax,
		datos_planificador_t *datos) {
	int nbytes, sockfd;

	for (sockfd = 0; sockfd <= sockfdMax; sockfd++) {
		if (sockfd == datos->sockfdNivel) {
			nbytes = atenderPedidoNivel(datos);
		} else {
			atenderPedidoPersonaje(datos, sockfd);
		}
	}

	return nbytes;
}

void moverPersonaje(datos_planificador_t *datos) {
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
			gestionarUbicacionCaja(datosPlan, personaje);
			break;
		case NOTIFICACION_MOVIMIENTO:
			quantumPersonaje--;
			reenviarNotificacionMovimiento(datosPlan, personaje, &header, data);
			break;
		case SOLICITAR_RECURSO:
			quantumPersonaje--;
			sockets_send(datosPlan->sockfdNivel, &header, data);
			gestionarSolicitudRecurso(datosPlan, personaje);
			break;
		}

		free(data);
	}

	if (quantumPersonaje == 0)
		queue_push(datosPlan->personajesListos, personaje);
}

void seleccionarPorSRDF(datos_planificador_t *datosPlan) {
	datos_personaje_t *personaje = seleccionarCaminoMasCorto(
			datosPlan->personajesListos);
	header_t header;

	enviarTurnoConcedido(personaje);
	recv(personaje->sockfd, &header, sizeof(header_t), MSG_WAITALL);
	char *data = malloc(header.length);
	recv(personaje->sockfd, data, header.length, MSG_WAITALL);

	while (header.type != SOLICITAR_RECURSO) {
		switch (header.type) {
		case UBICACION_CAJA:
			reenviarUbicacionCaja(datosPlan, personaje, &header, data);
			gestionarUbicacionCaja(datosPlan, personaje);
			break;
		case NOTIFICACION_MOVIMIENTO:
			reenviarNotificacionMovimiento(datosPlan, personaje, &header, data);
			break;
		}
	}

	sockets_send(datosPlan->sockfdNivel, &header, data);
	gestionarSolicitudRecurso(datosPlan, personaje);
}

void enviarTurnoConcedido(datos_personaje_t *personaje) {
	header_t header;
	header.type = TURNO_CONCEDIDO;
	header.length = 0;
	sockets_send(personaje->sockfd, &header, '\0');
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

int atenderPedidoNivel(datos_planificador_t *datos) {
	header_t header;

	int nbytes = recv(datos->sockfdNivel, &header, sizeof(header), MSG_WAITALL);

	switch (header.type) {
	case PERSONAJE_FINALIZO:
		//TODO:Realizar tareas de finalización.
		break;
	case NOTIFICAR_ALGORITMO_PLANIFICACION:
		nbytes = actualizarAlgoritmo(&header, datos);
		break;
	case VICTIMA_DEADLOCK:
		nbytes = removerPersonaje(&header, datos);
		break;
		//TODO: implementar mensajes: VICTIMA_ENEMIGO, NOTIFICACION_RECURSOS_LIBERADOS
	}

	return nbytes;
}

int removerPersonaje(header_t *header, datos_planificador_t *datos) {
	char *data = malloc(header->length);
	int nbytes = recv(datos->sockfdNivel, data, header->length, MSG_WAITALL);
	char idPersonaje;
	memcpy(&idPersonaje, data, sizeof(char));
	t_list *recursosLiberados = listaRecursos_deserializer(data + sizeof(char),
			header->length - sizeof(char));
	free(data);
	nbytes = notificarMuerteDeadlock(idPersonaje, datos);
	desbloquearPersonajes(recursosLiberados, datos);

	return nbytes;
}

t_list *desbloquearPersonajes(t_list *recursosLiberados,
		datos_planificador_t *datos) {
	t_list *recUsados = list_create();

	if (!queue_is_empty(datos->personajesBloqueados)) {
		t_list *perDesbloqueados = list_create();
		datos_personaje_t *perBloqueado, *perDesbloqueado;
		int i;

		for (i = 0; i < datos->personajesBloqueados->elements; i++) {
			perBloqueado = list_get(datos->personajesBloqueados->elements, i);
			int fueUsado = usarRecurso(perBloqueado->objetivo,
					recursosLiberados, recUsados, perBloqueado->simbolo);

			if (fueUsado) {
				list_add(perDesbloqueados, perBloqueado);
//				informarDesbloqueo(perBloqueado);

				if (datos->personajesBloqueados->elements == 0)
					break;
			}
		}

		for (i = 0; i < perDesbloqueados->elements_count; i++) {
			perDesbloqueado = list_get(perDesbloqueados, i);
//			desbloquearPersonaje(perDesbloqueado, datos);
		}

		list_destroy(perDesbloqueados);
	}

	return recUsados;
}

int usarRecurso(char simboloRecurso, t_list *recursosLiberados, t_list *recursosUsados,
		char simboloPersonaje) {
		int _is_recurso(recurso_t *recurso) {
			return recurso->id == simboloRecurso;
		}

		recurso_t *recursoLiberado = list_find(recursosLiberados,
				(void *) _is_recurso);

		if (recursoLiberado != NULL ) {
			recursoLiberado->quantity--;
//			datos_personaje_t *personajeDesbloqueado = malloc(
//					sizeof(datos_personaje_t));
//			personajeDesbloqueado->idRecurso = recursoLiberado->id;
//			personajeDesbloqueado->idPersonaje = simboloPersonaje;
//			list_add(recursosUsados, personajeDesbloqueado);

			if (recursoLiberado->quantity == 0) {
				list_remove_by_condition(recursosLiberados, (void *) _is_recurso);
				recurso_destroy(recursoLiberado);
			}
		}

		return recursoLiberado != NULL ;
}

int notificarMuerteDeadlock(char idPersonaje, datos_planificador_t *datos) {
	int _isPersonaje(datos_personaje_t *personaje) {
		return personaje->simbolo == idPersonaje;
	}
	//TODO: implementar mutex
	datos_personaje_t *personajeMuerto = list_remove_by_condition(
			datos->personajesBloqueados->elements, (void *) _isPersonaje);
	header_t header;
	header.type = NOTIFICAR_MUERTE;
	header.length = 0;
	int nbytes = sockets_send(personajeMuerto->sockfd, &header, '\0');
	close(personajeMuerto->sockfd);
	datosPersonaje_destroy(personajeMuerto);

	return nbytes;
}

int actualizarAlgoritmo(header_t *header, datos_planificador_t *datos) {
	char *data = malloc(header->length);
	int nbytes = recv(datos->sockfdNivel, data, header->length, MSG_WAITALL);
	informacion_planificacion_t *info = informacionPlanificacion_deserializer(
			data);
	free(data);
	datos->algoritmo = info->algoritmo;
	datos->retardo = info->retardo;
	datos->quatum = info->quantum;
	informacionPlanificacion_destroy(info);

	return nbytes;
}

void reenviarUbicacionCaja(datos_planificador_t *datosPlan,
		datos_personaje_t *personaje, header_t *header, char *data) {
	memcpy(&personaje->objetivo, data, sizeof(char));
	sockets_send(datosPlan->sockfdNivel, header, data);
}

void gestionarUbicacionCaja(datos_planificador_t *datosPlan,
		datos_personaje_t *personaje) {
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

void reenviarNotificacionMovimiento(datos_planificador_t *datosPlan,
		datos_personaje_t *personaje, header_t *header, char *data) {
	personaje->ubicacionActual = coordenadas_deserializer(data + sizeof(char));
	sockets_send(datosPlan->sockfdNivel, header, data);
}

void gestionarNotificacionMovimiento(datos_planificador_t *datosPlan,
		datos_personaje_t *personaje) {
	header_t header;

	recv(datosPlan->sockfdNivel, &header, sizeof(header), MSG_WAITALL);

	if (header.type == NOTIFICACION_MOVIMIENTO) {
		//TODO:Confirmación de movimiento realizado.
	}
}

void gestionarSolicitudRecurso(datos_planificador_t *datosPlan,
		datos_personaje_t *personaje) {
	header_t header;

	recv(datosPlan->sockfdNivel, &header, sizeof(header), MSG_WAITALL);

	switch (header.type) {
	case OTORGAR_RECURSO:
		coordenadas_destroy(personaje->coordObjetivo);
		personaje->coordObjetivo = NULL;
		break;
	case NEGAR_RECURSO:
//		pthread_mutex_lock(datosPlan->mutexColas);
		queue_push(datosPlan->personajesBloqueados, personaje);
//		pthread_mutex_unlock(datosPlan->mutexColas);
		break;
	}
}

datos_personaje_t *seleccionarCaminoMasCorto(t_queue *colaPersonajes) {
	t_list *listaPersonajes = colaPersonajes->elements;
	int i, flag = 0, index = 0;
	datos_personaje_t *personaje, *personajeElegido;

	for (i = 0; list_size(listaPersonajes) < i; i++) {
		personaje = list_get(listaPersonajes, i);

		if (personaje->coordObjetivo != NULL ) {
			if (flag) {
				if (obtenerDistancia(personaje->ubicacionActual,
						personaje->coordObjetivo)
						< obtenerDistancia(personajeElegido->ubicacionActual,
								personajeElegido->coordObjetivo)) {
					personajeElegido = personaje;
					index = i;
				}
			} else {
				personajeElegido = personaje;
				flag = 1;
				index = i;
			}
		}
	}

	list_remove(listaPersonajes, index);

	return personajeElegido;
}
