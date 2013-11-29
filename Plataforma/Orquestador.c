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
	listaPlanificadores = list_create();
	listaEspera = list_create();
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
		atenderNuevoPersonaje(nuevoSockfd);
		break;
	case HANDSHAKE_NIVEL:
		enviarHandshakeOrquestador(nuevoSockfd);
		crearNuevoHiloPlanificador(nuevoSockfd);
		break;
	case FINALIZAR_PLAN: //un personaje finalizo su plan de niveles.
		logguearFinPlan(&header, nuevoSockfd);
		chequearUltimoPersonaje();
		break;
	}
}

void delegarAlPlanificador(header_t *header, int sockfd) {
	char *datos = malloc(header->length);
	recv(sockfd, datos, header->length, MSG_WAITALL);
	notificacion_datos_personaje_t *notificacion =
			notificacionDatosPersonaje_deserializer(datos);
	datos_personaje_t *datosPersonaje = crearDatosPersonaje(
			notificacion->simbolo, sockfd);
	agregarPersonajeAListos(datosPersonaje, notificacion->nombreNivel);
	free(datos);
	notificacionDatosPersonaje_destroy(notificacion);
}

void atenderNuevoPersonaje(int sockfd) {
	header_t header;
	recv(sockfd, &header, sizeof(header), MSG_WAITALL);

	switch (header.type) {
	case NOTIFICAR_DATOS_PERSONAJE:
		delegarAlPlanificador(&header, sockfd);
		break;
	}
}

void logguearFinPlan(header_t *header, int sockfd) {
	char *data = malloc(header->length);
	recv(sockfd, data, header->length, MSG_WAITALL);
	log_info(logFile, "Personaje %c finalizo plan de niveles.", data[0]);
	free(data);
}

void chequearUltimoPersonaje(void) {
	int i, hayPersonajesActivos = 1;
	char c;
	datos_planificador_t *unPlanificador;

	for (i = 0; i < list_size(listaPlanificadores); i++) {
		unPlanificador = list_get(listaPlanificadores, i);
		hayPersonajesActivos = tienePersonajesActivos(unPlanificador);

		if (hayPersonajesActivos)
			break;
	}

	if (!hayPersonajesActivos) {
		printf("Desea enfrentar a Koopa? y/n\n");

		if ((c = getchar()) == 'y') {
			printf("Aqui se llamaria a koopa.\n");
//			char *sols[] = { "koopa", configObj->solicitudesKoopa, (char *) 0 };
//			execv(configObj->binarioKoopa, sols);
		}
	}
}

int tienePersonajesActivos(datos_planificador_t *unPlanificador) {
	int personajesActivos = 1;
//TODO: aplicar mutexes.
	pthread_mutex_lock(unPlanificador->mutexColas);
	if (queue_is_empty(
			unPlanificador->personajesListos) && queue_is_empty(unPlanificador->personajesBloqueados)
			&& unPlanificador->personajeEnMovimiento==NULL) {
		personajesActivos = 0;
	}
	pthread_mutex_unlock(unPlanificador->mutexColas);

	return personajesActivos;
}

datos_planificador_t *buscarPlanificador(char *nombre) {
	int _is_planificador(datos_planificador_t *plan) {
		return strcmp(plan->nombre, nombre) == 0;
	}

	return list_find(listaPlanificadores, (void *) _is_planificador);
}

datos_planificador_t *removerPlanificador(char *nombre) {
	int _is_planificador(datos_planificador_t *plan) {
		return strcmp(plan->nombre, nombre) == 0;
	}

	return list_remove_by_condition(listaPlanificadores,
			(void *) _is_planificador);
}

void agregarPersonajeAListos(datos_personaje_t *datosPersonaje,
		char *nombreNivel) {
	datos_planificador_t *datosPlanificador = buscarPlanificador(nombreNivel);

	if (datosPlanificador != NULL ) {
		notificarNivel(datosPlanificador->sockfdNivel, datosPersonaje->simbolo);
		//datosPlanificador->mutexColas TODO:Implementar mutex
		FD_SET(datosPersonaje->sockfd, datosPlanificador->bagMaster);
		if (datosPersonaje->sockfd > datosPlanificador->sockfdMax)
			datosPlanificador->sockfdMax = datosPersonaje->sockfd;
		log_info(logFile, "Personaje %c delegado a %s", datosPersonaje->simbolo,
				datosPlanificador->nombre);
		pthread_mutex_lock(datosPlanificador->mutexColas);
		queue_push(datosPlanificador->personajesListos, datosPersonaje);
		pthread_mutex_unlock(datosPlanificador->mutexColas);
		//datosPlanificador->mutexColas TODO:Implementar mutex
	} else {
		agregarPersonajeAEspera(nombreNivel, datosPersonaje);
		log_info(logFile, "Nivel pedido por %c no conectado todavia.",
				datosPersonaje->simbolo);
	}
}

void agregarPersonajeAEspera(char *nombreNivel, datos_personaje_t *personaje) {
	personaje_espera_t *perEspera = malloc(sizeof(personaje_espera_t));
	perEspera->nombreNivel = malloc(strlen(nombreNivel) + 1);
	strcpy(perEspera->nombreNivel, nombreNivel);
	perEspera->personaje = personaje;
	list_add(listaEspera, perEspera);
}

int notificarNivel(int sockfdNivel, char simbolo) {
	header_t header;
	header.type = NOTIFICAR_DATOS_PERSONAJE;
	header.length = sizeof(char);

	return sockets_send(sockfdNivel, &header, &simbolo);
}

datos_personaje_t *crearDatosPersonaje(char simbolo, int sockfdPersonaje) {
	datos_personaje_t *datosPersonaje = malloc(sizeof(datos_personaje_t));
	coordenada_t *ubicacion = malloc(sizeof(coordenada_t));
	ubicacion->ejeX = 0;
	ubicacion->ejeY = 0;
	datosPersonaje->ubicacionActual = ubicacion;
	datosPersonaje->simbolo = simbolo;
	datosPersonaje->sockfd = sockfdPersonaje;
	datosPersonaje->objetivo = '\0';
	datosPersonaje->coordObjetivo = NULL;

	return datosPersonaje;
}

void crearNuevoHiloPlanificador(int sockfd) {
	header_t header;
	recv(sockfd, &header, sizeof(header), MSG_WAITALL);

	if (header.type == NOTIFICAR_ALGORITMO_PLANIFICACION) {
		char *dataNivel = malloc(header.length);
		recv(sockfd, dataNivel, header.length, MSG_WAITALL);
		informacion_planificacion_t *infoPlanificacion =
				informacionPlanificacion_deserializer(dataNivel);
		free(dataNivel);
		datos_planificador_t *datosPlanificador = crearDatosPlanificador(
				infoPlanificacion, sockfd);
		informacionPlanificacion_destroy(infoPlanificacion);
//		log_info(logFile, "Nivel %s conectado.", datosPlanificador->nombre);
		pthread_create(datosPlanificador->hilo, NULL, (void *) planificador,
				(void *) datosPlanificador);
		list_add(listaPlanificadores, datosPlanificador);
		informarPersonajesEspera(datosPlanificador);
	}
}

void informarPersonajesEspera(datos_planificador_t *datosPlanificador) {
	int _is_personaje(personaje_espera_t *per) {
		return strcmp(per->nombreNivel, datosPlanificador->nombre) == 0;
	}

	personaje_espera_t *perEspera;

	while ((perEspera = list_remove_by_condition(listaEspera,
			(void *) _is_personaje)) != NULL ) {
		notificarNivel(datosPlanificador->sockfdNivel,
				perEspera->personaje->simbolo);
		//datosPlanificador->mutexColas TODO:Implementar mutex
		FD_SET(perEspera->personaje->sockfd, datosPlanificador->bagMaster);
		if (perEspera->personaje->sockfd > datosPlanificador->sockfdMax)
			datosPlanificador->sockfdMax = perEspera->personaje->sockfd;
		pthread_mutex_lock(datosPlanificador->mutexColas);
		queue_push(datosPlanificador->personajesListos, perEspera->personaje);
		pthread_mutex_unlock(datosPlanificador->mutexColas);
		//datosPlanificador->mutexColas TODO:Implementar mutex
//		log_info(logFile, "Personaje %c salio de espera.",
//				perEspera->personaje->simbolo);
		personajeEspera_destroy(perEspera);
	}
}

datos_planificador_t *crearDatosPlanificador(
		informacion_planificacion_t *infoPlan, int sockfdNivel) {
	datos_planificador_t *datosPlanificador = malloc(
			sizeof(datos_planificador_t));
	datosPlanificador->nombre = malloc(strlen(infoPlan->nombreNivel) + 1);
	strcpy(datosPlanificador->nombre, infoPlan->nombreNivel);
	datosPlanificador->sockfdNivel = sockfdNivel;
	datosPlanificador->algoritmo = infoPlan->algoritmo;
	datosPlanificador->quatum = infoPlan->quantum;
	datosPlanificador->retardo = infoPlan->retardo;
	datosPlanificador->hilo = malloc(sizeof(pthread_t));
	datosPlanificador->personajesBloqueados = queue_create();
	datosPlanificador->personajesListos = queue_create();
	datosPlanificador->mutexColas = malloc(sizeof(pthread_mutex_t));
	datosPlanificador->bagMaster = malloc(sizeof(fd_set));
	datosPlanificador->personajeEnMovimiento = NULL;
	datosPlanificador->quantumCorriente = 0;
	FD_ZERO(datosPlanificador->bagMaster);
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

void personajeEspera_destroy(personaje_espera_t *self) {
	free(self->nombreNivel);
	free(self);
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

void datosPersonaje_destroy(datos_personaje_t *self) {
	if (self->coordObjetivo != NULL ) {
		coordenadas_destroy(self->coordObjetivo);
	}

	if (self->ubicacionActual != NULL ) {
		coordenadas_destroy(self->ubicacionActual);
	}

	free(self);
}
