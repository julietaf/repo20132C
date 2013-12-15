/*
 * Planificador.c
 *
 *  Created on: 07/10/2013
 *      Author: utnso
 */

#include "Planificador.h"

void planificador(datos_planificador_t *datos) {
	fd_set bagEscucha;
	FD_ZERO(&bagEscucha);
	FD_SET(datos->sockfdNivel, datos->bagMaster);
	int retval;
	datos->sockfdMax = datos->sockfdNivel;
	log_info(logFile, "Planificador %s iniciado.", datos->nombre);

	while (1) {
		retval = llamadaSelect(datos, &bagEscucha);

		if (retval == -1) //Ocurrio un error
			log_error(logFile, "Error en select");
		else if (retval) //Hay un pedido de los procesos conectados
			atenderPedidoPlanificador(&bagEscucha, datos->sockfdMax, datos);
		else if (!queue_is_empty(
				datos->personajesListos) || datos->personajeEnMovimiento != NULL) //Se produjo el timeout
			moverPersonaje(datos);
	}
}

int llamadaSelect(datos_planificador_t *datosPlan, fd_set *bagEscucha) {
	struct timeval timeout;
	*bagEscucha = *datosPlan->bagMaster;
	timeout.tv_sec = div(datosPlan->retardo, 1000).quot;
	timeout.tv_usec = div(datosPlan->retardo, 1000).rem * 1000;

	return select(datosPlan->sockfdMax + 1, bagEscucha, NULL, NULL, &timeout);
}

int atenderPedidoPersonaje(datos_planificador_t *datosPlan, int sockfdPersonaje) {
	header_t header;
	int nbytes = recv(sockfdPersonaje, &header, sizeof(header), MSG_WAITALL);

	switch (header.type) {
	case FINALIZAR_NIVEL:
		informarPersonajeFinalizado(datosPlan, sockfdPersonaje);
		break;
	case UBICACION_CAJA:
		reenviarUbicacionCaja(datosPlan, sockfdPersonaje, &header);
		break;
	case NOTIFICACION_MOVIMIENTO:
		reenviarNotificacionMovimiento(datosPlan, sockfdPersonaje, &header);
		break;
	case SOLICITAR_RECURSO:
		reenviarSolicitudRecurso(datosPlan, sockfdPersonaje, &header);
		break;
	case NOTIFICAR_REINICIO_PLAN:
		atenderReinicioPlan(datosPlan, sockfdPersonaje);
		break;
	default:
		log_warning(logFile,
				"Mensaje inesperado. type=%d length=%d. (atender pedido personaje).",
				header.type, header.length);
	}

	if (nbytes == 0) {
		datos_personaje_t *unPersonaje = removerPersonajePorSockfd(datosPlan,
				sockfdPersonaje);
		enviarPersonajeFinalizo(datosPlan, unPersonaje->simbolo);
		FD_CLR(sockfdPersonaje, datosPlan->bagMaster);
		close(sockfdPersonaje);
		log_error(logFile,
				"Personaje %c cerro la conexion inesperadamente con %s.",
				unPersonaje->simbolo, datosPlan->nombre);
		datosPersonaje_destroy(unPersonaje);
	}

	return nbytes;
}

int atenderPedidoNivel(datos_planificador_t *datosPlan) {
	header_t header;

	int nbytes = recv(datosPlan->sockfdNivel, &header, sizeof(header),
			MSG_WAITALL);

	switch (header.type) {
	case NOTIFICAR_ALGORITMO_PLANIFICACION:
		actualizarAlgoritmo(&header, datosPlan);
		break;
	case VICTIMA_DEADLOCK:
		removerPersonaje(&header, datosPlan, "deadlock");
		break;
	case VICTIMA_ENEMIGO:
		removerPersonaje(&header, datosPlan, "enemigo");
		break;
	case UBICACION_CAJA:
		gestionarUbicacionCaja(datosPlan, &header);
		break;
	case NOTIFICACION_RECURSOS_LIBERADOS:
		gestionarDesbloqueoPersonajes(&header, datosPlan);
		break;
	default:
		log_warning(logFile,
				"Mensaje inesperado. type=%d length=%d. (atender pedido nivel).",
				header.type, header.length);
	}

	if (nbytes == 0) {
		log_error(logFile, "Nivel %s se desconecto inesperadamente.",
				datosPlan->nombre);
		self_destroy(datosPlan->nombre);
	}

	return nbytes;
}

void self_destroy(char *nombre) {
	int i;
	datos_personaje_t *unPersonaje;
	datos_planificador_t *self = removerPlanificador(nombre);

	close(self->sockfdNivel);
	free(self->bagMaster);
	free(self->nombre);

	for (i = 0; i < list_size(self->personajesListos->elements); i++) {
		unPersonaje = list_get(self->personajesListos->elements, i);
		close(unPersonaje->sockfd);
		datosPersonaje_destroy(unPersonaje);
	}

	for (i = 0; i < list_size(self->personajesBloqueados->elements); i++) {
		unPersonaje = list_get(self->personajesListos->elements, i);
		close(unPersonaje->sockfd);
		datosPersonaje_destroy(unPersonaje);
	}

	if (self->personajeEnMovimiento != NULL ) {
		close(self->personajeEnMovimiento->sockfd);
		datosPersonaje_destroy(self->personajeEnMovimiento);
	}

	pthread_mutex_destroy(self->mutexColas);
	free(self->mutexColas);
	free(self);
	pthread_exit((void *) NULL );
}

int removerPersonaje(header_t *header, datos_planificador_t *datosPlan,
		char *motivo) {
	int nbytes = 0;
	char *data = malloc(header->length);
	nbytes = recv(datosPlan->sockfdNivel, data, header->length, MSG_WAITALL);
	char idPersonaje;
	memcpy(&idPersonaje, data, sizeof(char));
	t_list *recursosLiberados = listaRecursos_deserializer(data + sizeof(char),
			header->length - sizeof(char));
	free(data);
	datos_personaje_t *personajeMuerto = removerPersonajePorSimbolo(datosPlan,
			idPersonaje);
	t_list *recursosUsados = desbloquearPersonajes(recursosLiberados,
			datosPlan);
	nbytes = informarRecursosUsados(recursosUsados, datosPlan);
	list_destroy_and_destroy_elements(recursosUsados, (void *) free);

	if (personajeMuerto == NULL )
		log_warning(logFile, "Personaje %c muerto no encontrado.", idPersonaje);
	else {
		nbytes = notificarMuertePersonaje(personajeMuerto, datosPlan);
		log_info(logFile, "Personaje %c muerto por %s.",
				personajeMuerto->simbolo, motivo);
		FD_CLR(personajeMuerto->sockfd, datosPlan->bagMaster);
		close(personajeMuerto->sockfd);
		datosPersonaje_destroy(personajeMuerto);
	}

	return nbytes;
}

int gestionarDesbloqueoPersonajes(header_t *header,
		datos_planificador_t *datosPlan) {
	int nbytes = 0;

	if (header->length == 0) {
		nbytes = informarRecursosUsados(NULL, datosPlan);
	} else {
		char *data = malloc(header->length);
		nbytes = recv(datosPlan->sockfdNivel, data, header->length,
				MSG_WAITALL);
		t_list *recursosLiberados = listaRecursos_deserializer(data,
				header->length);
		free(data);
		t_list *recursosUsados = desbloquearPersonajes(recursosLiberados,
				datosPlan);
		nbytes = informarRecursosUsados(recursosUsados, datosPlan);
		list_destroy_and_destroy_elements(recursosUsados, (void *) free);
		list_destroy_and_destroy_elements(recursosLiberados, (void *) free);
	}

	return nbytes;
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

	return nbytes;
}

void moverPersonaje(datos_planificador_t *datosPlan) {
	switch (datosPlan->algoritmo) {
	case ROUND_ROBIN:
		seleccionarPorRoundRobin(datosPlan);
		break;
	case SRDF:
		seleccionarPorSRDF(datosPlan);
		break;
	}
}

void seleccionarPorRoundRobin(datos_planificador_t *datosPlan) {
	if (datosPlan->personajeEnMovimiento == NULL ) {
		datosPlan->quantumCorriente = datosPlan->quatum;
		pthread_mutex_lock(datosPlan->mutexColas);
		datosPlan->personajeEnMovimiento = queue_pop(
				datosPlan->personajesListos);
		pthread_mutex_unlock(datosPlan->mutexColas);
		log_info(logFile, "Personaje %c seleccionado por RR.",
				datosPlan->personajeEnMovimiento->simbolo);
	} else if (datosPlan->quantumCorriente == 0) {
		log_info(logFile, "Personaje %c finalizo quantum.",
				datosPlan->personajeEnMovimiento->simbolo);
		pthread_mutex_lock(datosPlan->mutexColas);
		queue_push(datosPlan->personajesListos,
				datosPlan->personajeEnMovimiento);
		datosPlan->quantumCorriente = datosPlan->quatum;
		datosPlan->personajeEnMovimiento = queue_pop(
				datosPlan->personajesListos);
		pthread_mutex_unlock(datosPlan->mutexColas);
		log_info(logFile, "Personaje %c seleccionado por RR.",
				datosPlan->personajeEnMovimiento->simbolo);
	}

	enviarTurnoConcedido(datosPlan->personajeEnMovimiento);
	atenderPedidoPersonaje(datosPlan, datosPlan->personajeEnMovimiento->sockfd);
}

void seleccionarPorSRDF(datos_planificador_t *datosPlan) {
	int recursosSolicitados = 0;

	if (datosPlan->personajeEnMovimiento == NULL )
		recursosSolicitados = solicitarUbicacionRecursos(datosPlan);

	if (recursosSolicitados) {
		log_info(logFile, "Informacion de objetivos de personajes incompleta.");
	} else {
		if (datosPlan->personajeEnMovimiento == NULL ) {
			datosPlan->personajeEnMovimiento = seleccionarCaminoMasCorto(
					datosPlan);
			log_info(logFile, "Personaje %c seleccionado por SRDF.",
					datosPlan->personajeEnMovimiento->simbolo);
		}

		enviarTurnoConcedido(datosPlan->personajeEnMovimiento);
		atenderPedidoPersonaje(datosPlan,
				datosPlan->personajeEnMovimiento->sockfd);
	}
}

int enviarTurnoConcedido(datos_personaje_t *personaje) {
	header_t header;
	header.type = TURNO_CONCEDIDO;
	header.length = 0;

	return sockets_send(personaje->sockfd, &header, '\0');
}

int atenderReinicioPlan(datos_planificador_t *datosPlan, int sockfdPersonaje) {
	notificarReinicioPlan(sockfdPersonaje);
	return informarPersonajeReinicio(datosPlan, sockfdPersonaje);
}

int informarPersonajeReinicio(datos_planificador_t *datosPlan,
		int sockfdPersonaje) {
	datos_personaje_t *personaje = removerPersonajePorSockfd(datosPlan,
			sockfdPersonaje);
	int nbytes = enviarPersonajeFinalizo(datosPlan, sockfdPersonaje);
	log_info(logFile, "Personaje %c reinicio el nivel %s", personaje->simbolo,
			datosPlan->nombre);
	datosPersonaje_destroy(personaje);

	return nbytes;
}

int informarPersonajeFinalizado(datos_planificador_t *datosPlan,
		int sockfdPersonaje) {
	datos_personaje_t *personaje = removerPersonajePorSockfd(datosPlan,
			sockfdPersonaje);
	int nbytes = enviarPersonajeFinalizo(datosPlan, personaje->simbolo);
	log_info(logFile, "Personaje %c finalizo el nivel %s", personaje->simbolo,
			datosPlan->nombre);
	FD_CLR(personaje->sockfd, datosPlan->bagMaster);
	close(personaje->sockfd);
	datosPersonaje_destroy(personaje);

	return nbytes;
}

int reenviarNotificacionMovimiento(datos_planificador_t *datosPlan,
		int sockfdPersonaje, header_t *header) {
	char *data = malloc(header->length);
	recv(sockfdPersonaje, data, header->length, MSG_WAITALL);
	datosPlan->personajeEnMovimiento->ubicacionActual =
			coordenadas_deserializer(data + sizeof(char));
	log_trace(logFile, "Personaje %c se movio a X: %d Y: %d.",
			datosPlan->personajeEnMovimiento->simbolo,
			datosPlan->personajeEnMovimiento->ubicacionActual->ejeX,
			datosPlan->personajeEnMovimiento->ubicacionActual->ejeY);

	int nbytes = sockets_send(datosPlan->sockfdNivel, header, data);
	free(data);

	if (datosPlan->algoritmo == ROUND_ROBIN) {
		datosPlan->quantumCorriente--;
	}

	return nbytes;
}

int reenviarSolicitudRecurso(datos_planificador_t *datosPlan,
		int sockfdPersonaje, header_t *header) {
	char *data = malloc(header->length);
	recv(sockfdPersonaje, data, header->length, MSG_WAITALL);

	personaje_recurso_t *personajeRecurso = malloc(sizeof(personaje_recurso_t));
	personajeRecurso->idPersonaje = datosPlan->personajeEnMovimiento->simbolo;
	personajeRecurso->idRecurso = data[0];
	char *serialized = personajeRecurso_serializer(personajeRecurso,
			&header->length);
	log_info(logFile, "Personaje %c solicito recurso %c.",
			personajeRecurso->idPersonaje, personajeRecurso->idRecurso);
	sockets_send(datosPlan->sockfdNivel, header, serialized);
	free(serialized);
	free(personajeRecurso);
	free(data);

	return esperarSolicitudRecurso(datosPlan, NULL );
}

int esperarSolicitudRecurso(datos_planificador_t *datosPlan,
		datos_personaje_t *personaje) {
	header_t header;
	int nbytes, desincronizacion = 0;

	if (personaje == NULL )
		personaje = datosPlan->personajeEnMovimiento;

	recv(datosPlan->sockfdNivel, &header, sizeof(header_t), MSG_WAITALL);

	switch (header.type) {
	case NEGAR_RECURSO: //TODO: chequear logica de personaje bloqueado.
		log_info(logFile, "Recurso %c negado a Personaje %c.",
				personaje->objetivo, personaje->simbolo);
		datosPlan->personajeEnMovimiento = NULL;
		datosPlan->quantumCorriente = 0;
		pthread_mutex_lock(datosPlan->mutexColas);
		queue_push(datosPlan->personajesBloqueados, personaje);
		pthread_mutex_unlock(datosPlan->mutexColas);
		log_info(logFile, "Personaje %c entro en cola bloqueados.",
				personaje->simbolo);
		break;
	case OTORGAR_RECURSO:
		log_info(logFile, "Recurso %c otorgado a Personaje %c.",
				personaje->objetivo, personaje->simbolo);
		coordenadas_destroy(personaje->coordObjetivo);
		personaje->coordObjetivo = NULL;
		personaje->objetivo = '\0';
		pthread_mutex_lock(datosPlan->mutexColas);
		queue_push(datosPlan->personajesListos, personaje);
		pthread_mutex_unlock(datosPlan->mutexColas);
		log_info(logFile, "Personaje %c entro en cola listos.",
				personaje->simbolo);
		break;
	case NOTIFICAR_ALGORITMO_PLANIFICACION:
		actualizarAlgoritmo(&header, datosPlan);
		desincronizacion = 1;
		esperarSolicitudRecurso(datosPlan, personaje);
		break;
	case VICTIMA_ENEMIGO:
		removerPersonaje(&header, datosPlan, "enemigo");
		desincronizacion = 1;
		esperarSolicitudRecurso(datosPlan, personaje);
		break;
	default:
		log_warning(logFile,
				"Mensaje inesperado. type=%d length=%d.(esperando solicitud recurso)",
				header.type, header.length);
		break;
	}

	if (!desincronizacion) {
		nbytes = informarSolicitudRecurso(personaje, header.type);
		datosPlan->personajeEnMovimiento = NULL;
		datosPlan->quantumCorriente = 0;
	}

	return nbytes;
}

int reenviarUbicacionCaja(datos_planificador_t *datosPlan, int sockfdPersonaje,
		header_t *header) {
	char *data = malloc(header->length);
	recv(sockfdPersonaje, data, header->length, MSG_WAITALL);
	datos_personaje_t *unPersonaje = buscarPersonajePorSockfd(datosPlan,
			sockfdPersonaje);
	unPersonaje->objetivo = data[0];
	char *dataSend = malloc(2 * sizeof(char));
	memcpy(dataSend, data, sizeof(char));
	memcpy(dataSend + sizeof(char), &unPersonaje->simbolo, sizeof(char));
	header->length = 2 * sizeof(char);
	sockets_send(datosPlan->sockfdNivel, header, dataSend);
	log_info(logFile, "Personaje %c con nuevo objetivo: %c",
			unPersonaje->simbolo, unPersonaje->objetivo);
	free(dataSend);
	free(data);

	return esperarUbicacionCaja(datosPlan, unPersonaje);
}

int esperarUbicacionCaja(datos_planificador_t *datosPlan,
		datos_personaje_t *unPersonaje) {
	header_t header;
	recv(datosPlan->sockfdNivel, &header, sizeof(header_t), MSG_WAITALL);
	char *respuesta = malloc(header.length);
	int nbytes = 0;

	switch (header.type) {
	case UBICACION_CAJA:
		nbytes = recv(datosPlan->sockfdNivel, respuesta, header.length,
				MSG_WAITALL);
		unPersonaje->coordObjetivo = coordenadas_deserializer(
				respuesta + sizeof(char));
		header.length = header.length - sizeof(char);
		nbytes = sockets_send(unPersonaje->sockfd, &header,
				respuesta + sizeof(char));
		break;
	case NOTIFICAR_ALGORITMO_PLANIFICACION:
		actualizarAlgoritmo(&header, datosPlan);
		esperarUbicacionCaja(datosPlan, unPersonaje);
		break;
	case VICTIMA_ENEMIGO:
		removerPersonaje(&header, datosPlan, "enemigo");
		break;
	default:
		log_warning(logFile, "Mensaje inesperado. type=%d length=%d.",
				header.type, header.length);
		break;
	}

	free(respuesta);

	return nbytes;
}

int gestionarReinicioPlan(datos_planificador_t *datosPlan, int sockfdPersonaje) {
	datos_personaje_t *unPersonaje = removerPersonajePorSockfd(datosPlan,
			sockfdPersonaje);
	int nbytes = notificarReinicioPlan(sockfdPersonaje);
	enviarPersonajeFinalizo(datosPlan, unPersonaje->simbolo);
	close(unPersonaje->sockfd);
	datosPersonaje_destroy(unPersonaje);

	return nbytes;
}

int notificarReinicioPlan(int sockfd) {
	header_t header;
	header.type = NOTIFICAR_REINICIO_PLAN;
	header.length = 0;

	return sockets_send(sockfd, &header, '\0');
}

int enviarPersonajeFinalizo(datos_planificador_t *datosPlan, char simbolo) {
	header_t header;
	header.type = PERSONAJE_FINALIZO;
	header.length = sizeof(char);

	return sockets_send(datosPlan->sockfdNivel, &header, &simbolo);
}

datos_personaje_t *removerPersonajePorSockfd(datos_planificador_t *datosPlan,
		int sockfd) {
	int _is_personaje(datos_personaje_t *unPersonaje) {
		return unPersonaje->sockfd == sockfd;
	}

	datos_personaje_t *unPersonaje = NULL;

	pthread_mutex_lock(datosPlan->mutexColas);
	if (datosPlan->personajeEnMovimiento != NULL ) {
		if (datosPlan->personajeEnMovimiento->sockfd == sockfd) {
			unPersonaje = datosPlan->personajeEnMovimiento;
			datosPlan->personajeEnMovimiento = NULL;
			datosPlan->quantumCorriente = 0;
		}
	}

	if (unPersonaje == NULL ) {
		unPersonaje = list_remove_by_condition(
				datosPlan->personajesListos->elements, (void *) _is_personaje);
	}

	if (unPersonaje == NULL ) {
		unPersonaje = list_remove_by_condition(
				datosPlan->personajesBloqueados->elements,
				(void *) _is_personaje);
	}
	pthread_mutex_unlock(datosPlan->mutexColas);

	return unPersonaje;
}

datos_personaje_t *buscarPersonajePorSockfd(datos_planificador_t *datosPlan,
		int sockfdPersonaje) {
	int _is_personaje(datos_personaje_t *unPersonaje) {
		return unPersonaje->sockfd == sockfdPersonaje;
	}

	datos_personaje_t *unPersonaje = NULL;

	pthread_mutex_lock(datosPlan->mutexColas);
	if (datosPlan->personajeEnMovimiento != NULL ) {
		if (datosPlan->personajeEnMovimiento->sockfd == sockfdPersonaje) {
			unPersonaje = datosPlan->personajeEnMovimiento;
		}
	}

	if (unPersonaje == NULL ) {
		unPersonaje = list_find(datosPlan->personajesListos->elements,
				(void *) _is_personaje);
	}

	if (unPersonaje == NULL ) {
		unPersonaje = list_find(datosPlan->personajesBloqueados->elements,
				(void *) _is_personaje);
	}
	pthread_mutex_unlock(datosPlan->mutexColas);

	return unPersonaje;
}

int gestionarUbicacionCaja(datos_planificador_t *datosPlan, header_t *header) {
	char *respuesta = malloc(header->length);
	int nbytes = recv(datosPlan->sockfdNivel, respuesta, header->length,
			MSG_WAITALL);
	datos_personaje_t *unPersonaje = buscarPersonajePorSimbolo(datosPlan,
			respuesta[0]);

	if (unPersonaje == NULL ) {
		log_warning(logFile,
				"Personaje no encontrado. type=%d length=%d idPersonaje=%c. %s (ubicacion caja).",
				header->type, header->length, respuesta[0], datosPlan->nombre);
	} else {
		unPersonaje->coordObjetivo = coordenadas_deserializer(
				respuesta + sizeof(char));
		header->length = header->length - sizeof(char);
		nbytes = sockets_send(unPersonaje->sockfd, header,
				respuesta + sizeof(char));
	}

	free(respuesta);

	return nbytes;
}

int informarRecursosUsados(t_list *recursosUsados, datos_planificador_t *datos) {
	header_t header;
	header.type = NOTIFICACION_RECURSOS_ASIGNADOS;
	int nbytes = 0;

	if (recursosUsados == NULL ) {
		header.length = 0;
		nbytes = sockets_send(datos->sockfdNivel, &header, '\0');
	} else {
		char *serialized = listaRecursos_serializer(recursosUsados,
				&header.length);
		nbytes = sockets_send(datos->sockfdNivel, &header, serialized);
		free(serialized);
	}

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
		datos_planificador_t *datosPlan) {
	int _is_personaje(datos_personaje_t *personaje) {
		return personaje->simbolo == perDesbloqueado->simbolo;
	}

	t_list *personajesBloqueados = datosPlan->personajesBloqueados->elements;
	t_queue *personajesListos = datosPlan->personajesListos;
	datos_personaje_t *personajeDesbloqueado = list_remove_by_condition(
			personajesBloqueados, (void *) _is_personaje);
	coordenadas_destroy(perDesbloqueado->coordObjetivo);
	perDesbloqueado->coordObjetivo = NULL;
	perDesbloqueado->objetivo = '\0';
	pthread_mutex_lock(datosPlan->mutexColas);
	queue_push(personajesListos, personajeDesbloqueado);
	pthread_mutex_unlock(datosPlan->mutexColas);
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

int notificarMuertePersonaje(datos_personaje_t *personajeMuerto,
		datos_planificador_t *datos) {
	header_t header;
	header.type = NOTIFICAR_MUERTE;
	header.length = 0;
	int nbytes = sockets_send(personajeMuerto->sockfd, &header, '\0');

	return nbytes;
}

int actualizarAlgoritmo(header_t *header, datos_planificador_t *datosPlan) {
	char *data = malloc(header->length);
	int nbytes = recv(datosPlan->sockfdNivel, data, header->length,
			MSG_WAITALL);
	informacion_planificacion_t *info = informacionPlanificacion_deserializer(
			data);
	free(data);
	datosPlan->algoritmo = info->algoritmo;
	datosPlan->retardo = info->retardo;
	datosPlan->quatum = info->quantum;
	informacionPlanificacion_destroy(info);

	if (datosPlan->personajeEnMovimiento != NULL ) {
		pthread_mutex_lock(datosPlan->mutexColas);
		queue_push(datosPlan->personajesListos,
				datosPlan->personajeEnMovimiento);
		datosPlan->personajeEnMovimiento = NULL;
		pthread_mutex_unlock(datosPlan->mutexColas);
	}

	switch (datosPlan->algoritmo) {
	case ROUND_ROBIN:
		log_info(logFile, "Cambio de algoritmo a Round Robin.");
		break;
	case SRDF:
		log_info(logFile,
				"Cambio de algoritmo a Shortest Resource Distance First.");
		break;
	}

	return nbytes;
}

datos_personaje_t *buscarPersonajePorSimbolo(datos_planificador_t *datosPlan,
		char simbolo) {
	int _is_personaje(datos_personaje_t *unPersonaje) {
		return unPersonaje->simbolo == simbolo;
	}

	datos_personaje_t *unPersonaje = NULL;

	pthread_mutex_lock(datosPlan->mutexColas);
	if (datosPlan->personajeEnMovimiento != NULL ) {
		if (datosPlan->personajeEnMovimiento->simbolo == simbolo) {
			unPersonaje = datosPlan->personajeEnMovimiento;
		}
	}

	if (unPersonaje == NULL ) {
		unPersonaje = list_find(datosPlan->personajesListos->elements,
				(void *) _is_personaje);
	}

	if (unPersonaje == NULL ) {
		unPersonaje = list_find(datosPlan->personajesBloqueados->elements,
				(void *) _is_personaje);
	}
	pthread_mutex_unlock(datosPlan->mutexColas);

	return unPersonaje;
}

datos_personaje_t *removerPersonajePorSimbolo(datos_planificador_t *datosPlan,
		char simbolo) {
	int _is_personaje(datos_personaje_t *unPersonaje) {
		return unPersonaje->simbolo == simbolo;
	}

	datos_personaje_t *unPersonaje = NULL;

	pthread_mutex_lock(datosPlan->mutexColas);
	if (datosPlan->personajeEnMovimiento != NULL ) {
		if (datosPlan->personajeEnMovimiento->simbolo == simbolo) {
			unPersonaje = datosPlan->personajeEnMovimiento;
			datosPlan->personajeEnMovimiento = NULL;
			datosPlan->quantumCorriente = 0;
		}
	}

	if (unPersonaje == NULL ) {
		unPersonaje = list_remove_by_condition(
				datosPlan->personajesListos->elements, (void *) _is_personaje);
	}

	if (unPersonaje == NULL ) {
		unPersonaje = list_remove_by_condition(
				datosPlan->personajesBloqueados->elements,
				(void *) _is_personaje);
	}
	pthread_mutex_unlock(datosPlan->mutexColas);

	return unPersonaje;
}

int informarSolicitudRecurso(datos_personaje_t *personaje, int type) {
	header_t header;
	header.type = type;
	header.length = 0;
	return sockets_send(personaje->sockfd, &header, '\0');
}

datos_personaje_t *seleccionarCaminoMasCorto(datos_planificador_t *datosPlan) {
	t_list *listaPersonajes = datosPlan->personajesListos->elements;
	int i, flag = 0, index = 0;
	datos_personaje_t *personaje, *personajeElegido;

	for (i = 0; i < list_size(listaPersonajes); i++) {
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

	pthread_mutex_lock(datosPlan->mutexColas);
	list_remove(listaPersonajes, index);
	pthread_mutex_unlock(datosPlan->mutexColas);

	return personajeElegido;
}

int solicitarUbicacionRecursos(datos_planificador_t *datosPlan) {
	int i, recursosSolicitados = 0;
	datos_personaje_t *unPersonaje;

	pthread_mutex_lock(datosPlan->mutexColas);
	for (i = 0; i < queue_size(datosPlan->personajesListos); i++) {
		unPersonaje = list_get(datosPlan->personajesListos->elements, i);

		if (unPersonaje->coordObjetivo == NULL ) {
			enviarTurnoConcedido(unPersonaje);
			recursosSolicitados = 1;
		}
	}
	pthread_mutex_unlock(datosPlan->mutexColas);

	return recursosSolicitados;
}
