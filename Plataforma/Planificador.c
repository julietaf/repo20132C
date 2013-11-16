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
int enviarTurnoConcedido(datos_personaje_t *personaje);
int atenderPedidoPersonaje(datos_planificador_t *datos, int sockfdPersonaje);
int atenderPedidoNivel(datos_planificador_t *datos);
int gestionarUbicacionCaja(datos_planificador_t *datosPlan, header_t *header);
void reenviarUbicacionCaja(datos_planificador_t *datosPlan,
		datos_personaje_t *personaje, header_t *header, char *data);
int reenviarNotificacionMovimiento(datos_planificador_t *datosPlan,
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
int informarDesbloqueo(datos_personaje_t *perBloqueado);
void desbloquearPersonaje(datos_personaje_t *perDesbloqueado,
		datos_planificador_t *datos);
int informarRecursosUsados(t_list *recursosUsados, datos_planificador_t *datos);
int reenviarSolicitudRecurso(datos_planificador_t *datosPlan,
		datos_personaje_t *personaje, char *data);
int informarSolicitudRecurso(datos_personaje_t *personaje, int type);

void planificador(datos_planificador_t *datos) {
	fd_set bagEscucha;
	FD_ZERO(&bagEscucha);
	FD_SET(datos->sockfdNivel, datos->bagMaster);
	int retval;
	datos->sockfdMax = datos->sockfdNivel;
	struct timeval timeout;
//	int usegundos = (datos->retardo * 1000000);
//	int sec = div(usegundos, 1000000).quot;
//	int usec = div(usegundos, 1000000).rem;

	while (1) {
		bagEscucha = *datos->bagMaster;
		timeout.tv_sec = 1;
		timeout.tv_usec = datos->retardo;

		retval = select(datos->sockfdMax + 1, &bagEscucha, NULL, NULL,
				&timeout);

		if (retval == -1) //Ocurrio un error
			log_error(logFile, "Error en select");
		else if (retval) //Hay un pedido de los procesos conectados
			atenderPedidoPlanificador(&bagEscucha, datos->sockfdMax, datos);
		else if (!queue_is_empty(
				datos->personajesListos) || datos->personajeEnMovimiento!=NULL) //Se produjo el timeout
			moverPersonaje(datos);
	}
}

int atenderPedidoPlanificador(fd_set *bagEscucha, int sockfdMax,
		datos_planificador_t *datos) {
	int nbytes, sockfd;

	for (sockfd = 0; sockfd <= sockfdMax; sockfd++) {
		if (FD_ISSET(sockfd,bagEscucha)) {
			if (sockfd == datos->sockfdNivel) {
				nbytes = atenderPedidoNivel(datos);
			} else {
				nbytes = atenderPedidoPersonaje(datos, sockfd);
			}
		}
	}

	if (sockfd == 0) {
		close(sockfd);
		FD_CLR(sockfd, datos->bagMaster);
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
	if (datosPlan->personajeEnMovimiento == NULL ) {
		datosPlan->quantumCorriente = datosPlan->quatum;
		datosPlan->personajeEnMovimiento = queue_pop(
				datosPlan->personajesListos);
	} else if (datosPlan->quantumCorriente == 0) {
		queue_push(datosPlan->personajesListos,
				datosPlan->personajeEnMovimiento);
		datosPlan->quantumCorriente = datosPlan->quatum;
		datosPlan->personajeEnMovimiento = queue_pop(
				datosPlan->personajesListos);
	}

	enviarTurnoConcedido(datosPlan->personajeEnMovimiento);
}
//while (quantumPersonaje) {
//	enviarTurnoConcedido(personaje);
//	header_t header;
//	recv(personaje->sockfd, &header, sizeof(header_t), MSG_WAITALL);
//	char *data = malloc(header.length);
//	recv(personaje->sockfd, data, header.length, MSG_WAITALL);
//
//	switch (header.type) {
//		case UBICACION_CAJA:
//		reenviarUbicacionCaja(datosPlan, personaje, &header, data);
//		gestionarUbicacionCaja(datosPlan, personaje);
//		break;
//		case NOTIFICACION_MOVIMIENTO:
//		quantumPersonaje--;
//		reenviarNotificacionMovimiento(datosPlan, personaje, &header, data);
//		break;
//		case SOLICITAR_RECURSO:
//		quantumPersonaje--;
//		reenviarSolicitudRecurso(datosPlan, personaje, data);
//		gestionarSolicitudRecurso(datosPlan, personaje);
//		break;
//		case PERSONAJE_FINALIZO:
//		//TODO:implementar finalizacion de nivel
//		break;
//	}
//
//	free(data);
//
//	if (quantumPersonaje == 0)
//	queue_push(datosPlan->personajesListos, personaje);
//	else
//	sleep(1); //TODO: implementar el retardo
//}

int reenviarSolicitudRecurso(datos_planificador_t *datosPlan,
		datos_personaje_t *personaje, char *data) {
	header_t header;
	header.type = SOLICITAR_RECURSO;
	personaje_recurso_t *personajeRecurso = malloc(sizeof(personaje_recurso_t));
	personajeRecurso->idPersonaje = personaje->simbolo;
	personajeRecurso->idRecurso = data[0];
	char *serialized = personajeRecurso_serializer(personajeRecurso,
			&header.length);
	int nbytes = sockets_send(datosPlan->sockfdNivel, &header, serialized);
	free(serialized);
	free(personajeRecurso);

	return nbytes;
}

void seleccionarPorSRDF(datos_planificador_t *datosPlan) {
//	datos_personaje_t *personaje = seleccionarCaminoMasCorto(
//			datosPlan->personajesListos);
//	header_t header;
//	char *data;
//
//	do {
//		enviarTurnoConcedido(personaje);
//		recv(personaje->sockfd, &header, sizeof(header_t), MSG_WAITALL);
//		data = malloc(header.length);
//		recv(personaje->sockfd, data, header.length, MSG_WAITALL);
//
//		switch (header.type) {
//		case UBICACION_CAJA:
//			reenviarUbicacionCaja(datosPlan, personaje, &header, data);
//			gestionarUbicacionCaja(datosPlan, personaje);
//			sleep(1); //TODO:implementar el retardo
//			break;
//		case NOTIFICACION_MOVIMIENTO:
//			reenviarNotificacionMovimiento(datosPlan, personaje, &header, data);
//			sleep(1); //TODO:implementar el retardo
//			break;
//		case SOLICITAR_RECURSO:
//			reenviarSolicitudRecurso(datosPlan, personaje, data);
//			gestionarSolicitudRecurso(datosPlan, personaje);
//			break;
//		}
//
//		free(data);
//	} while (header.type != SOLICITAR_RECURSO);
}

int enviarTurnoConcedido(datos_personaje_t *personaje) {
	header_t header;
	header.type = TURNO_CONCEDIDO;
	header.length = 0;

	return sockets_send(personaje->sockfd, &header, '\0');
}

int atenderPedidoPersonaje(datos_planificador_t *datosPlan, int sockfdPersonaje) {
	header_t header;
	int nbytes = recv(sockfdPersonaje, &header, sizeof(header), MSG_WAITALL);
	char *data;

	switch (header.type) {
	case FINALIZAR_NIVEL:
		//TODO:Realizar tareas de finalización.
		break;
	case UBICACION_CAJA:
		data = malloc(header.length);
		recv(sockfdPersonaje, data, header.length, MSG_WAITALL);
		reenviarUbicacionCaja(datosPlan, datosPlan->personajeEnMovimiento,
				&header, data);
		free(data);
		break;
	case NOTIFICACION_MOVIMIENTO:
		data = malloc(header.length);
		recv(sockfdPersonaje, data, header.length, MSG_WAITALL);
		reenviarNotificacionMovimiento(datosPlan,
				datosPlan->personajeEnMovimiento, &header, data);
		free(data);
		datosPlan->quantumCorriente--;
		break;
	case SOLICITAR_RECURSO:
		data = malloc(header.length);
		recv(sockfdPersonaje, data, header.length, MSG_WAITALL);
		reenviarSolicitudRecurso(datosPlan, datosPlan->personajeEnMovimiento,
				data);
		free(data);
		datosPlan->quantumCorriente--;
		break;
	}

	return nbytes;
}

int atenderPedidoNivel(datos_planificador_t *datosPlan) {
	header_t header;

	int nbytes = recv(datosPlan->sockfdNivel, &header, sizeof(header),
			MSG_WAITALL);

	switch (header.type) {
	case PERSONAJE_FINALIZO:
		//TODO:Realizar tareas de finalización.
		break;
	case NOTIFICAR_ALGORITMO_PLANIFICACION:
		nbytes = actualizarAlgoritmo(&header, datosPlan);
		break;
	case VICTIMA_DEADLOCK:
		nbytes = removerPersonaje(&header, datosPlan);
		break;
	case VICTIMA_ENEMIGO:
		nbytes = removerPersonaje(&header, datosPlan);
		break;
	case UBICACION_CAJA:
		gestionarUbicacionCaja(datosPlan, &header);
		break;
	case OTORGAR_RECURSO:
		coordenadas_destroy(datosPlan->personajeEnMovimiento->coordObjetivo);
		datosPlan->personajeEnMovimiento->coordObjetivo = NULL;
		informarSolicitudRecurso(datosPlan->personajeEnMovimiento, header.type);
		break;
	case NEGAR_RECURSO:
		//TODO: implementar mutex
		queue_push(datosPlan->personajesBloqueados,
				datosPlan->personajeEnMovimiento);
		datosPlan->personajeEnMovimiento = NULL;
		informarSolicitudRecurso(datosPlan->personajeEnMovimiento, header.type);
		break;
		//TODO: implementar mensajes: NOTIFICACION_RECURSOS_LIBERADOS
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
	t_list *recursosUsados = desbloquearPersonajes(recursosLiberados, datos);
	nbytes = informarRecursosUsados(recursosUsados, datos);
	list_destroy_and_destroy_elements(recursosUsados, (void *) free);

	return nbytes;
}

int informarRecursosUsados(t_list *recursosUsados, datos_planificador_t *datos) {
	header_t header;
	header.type = NOTIFICACION_RECURSOS_ASIGNADOS;
	char *serialized = listaRecursos_serializer(recursosUsados, &header.length);
	int nbytes = sockets_send(datos->sockfdNivel, &header, serialized);
	free(serialized);

	return nbytes;
}

t_list *desbloquearPersonajes(t_list *recursosLiberados,
		datos_planificador_t *datos) {
	t_list *recUsados = list_create();

	if (!queue_is_empty(datos->personajesBloqueados)) {
		t_list *perDesbloqueados = list_create();
		datos_personaje_t *perBloqueado, *perDesbloqueado;
		int i;

		for (i = 0; i < datos->personajesBloqueados->elements->elements_count;
				i++) {
			perBloqueado = list_get(datos->personajesBloqueados->elements, i);
			int fueUsado = usarRecurso(perBloqueado->objetivo,
					recursosLiberados, recUsados, perBloqueado->simbolo);

			if (fueUsado) {
				list_add(perDesbloqueados, perBloqueado);
				informarDesbloqueo(perBloqueado);

				if (recursosLiberados->elements_count == 0)
					break;
			}
		}

		for (i = 0; i < perDesbloqueados->elements_count; i++) {
			perDesbloqueado = list_get(perDesbloqueados, i);
			desbloquearPersonaje(perDesbloqueado, datos);
		}

		list_destroy(perDesbloqueados);
	}

	return recUsados;
}

void desbloquearPersonaje(datos_personaje_t *perDesbloqueado,
		datos_planificador_t *datos) {
	int _is_personaje(datos_personaje_t *personaje) {
		return personaje->simbolo == perDesbloqueado->simbolo;
	}

	t_list *personajesBloqueados = datos->personajesBloqueados->elements;
	t_queue *personajesListos = datos->personajesListos;
	datos_personaje_t *personajeDesbloqueado = list_remove_by_condition(
			personajesBloqueados, (void *) _is_personaje);
	coordenadas_destroy(perDesbloqueado->coordObjetivo);
	perDesbloqueado->coordObjetivo = NULL;
	queue_push(personajesListos, personajeDesbloqueado);
}

int usarRecurso(char simboloRecurso, t_list *recursosLiberados,
		t_list *recursosUsados, char simboloPersonaje) {
	int _is_recurso(recurso_t *recurso) {
		return recurso->id == simboloRecurso;
	}

	recurso_t *recursoLiberado = list_find(recursosLiberados,
			(void *) _is_recurso);

	if (recursoLiberado != NULL ) {
		recursoLiberado->quantity--;
		personaje_recurso_t *personajeDesbloqueado = malloc(
				sizeof(personaje_recurso_t));
		personajeDesbloqueado->idRecurso = recursoLiberado->id;
		personajeDesbloqueado->idPersonaje = simboloPersonaje;
		list_add(recursosUsados, personajeDesbloqueado);

		if (recursoLiberado->quantity == 0) {
			list_remove_by_condition(recursosLiberados, (void *) _is_recurso);
			recurso_destroy(recursoLiberado);
		}
	}

	return recursoLiberado != NULL ;
}

int informarDesbloqueo(datos_personaje_t *perDesbloqueado) {
	header_t header;
	header.type = OTORGAR_RECURSO;
	header.length = 0;

	return sockets_send(perDesbloqueado->sockfd, &header, '\0');
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

int gestionarUbicacionCaja(datos_planificador_t *datosPlan, header_t *header) {
	datos_personaje_t *personaje = datosPlan->personajeEnMovimiento;
	char *respuesta = malloc(header->length);
	int nbytes = recv(datosPlan->sockfdNivel, respuesta, header->length,
			MSG_WAITALL);
	personaje->coordObjetivo = coordenadas_deserializer(respuesta);
	nbytes = sockets_send(personaje->sockfd, header, respuesta);

	free(respuesta);

	return nbytes;
}

int reenviarNotificacionMovimiento(datos_planificador_t *datosPlan,
		datos_personaje_t *personaje, header_t *header, char *data) {
	personaje->ubicacionActual = coordenadas_deserializer(data + sizeof(char));

	return sockets_send(datosPlan->sockfdNivel, header, data);
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

	informarSolicitudRecurso(personaje, header.type);
}

int informarSolicitudRecurso(datos_personaje_t *personaje, int type) {
	header_t header;
	header.type = type;
	header.length = 0;
	return sockets_send(personaje->sockfd, &header, '\0');
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
