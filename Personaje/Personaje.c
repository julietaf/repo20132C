/*
 * Personaje.c
 *
 *  Created on: 24/09/2013
 *      Author: utnso
 */

#include "Personaje.h"

int main(void) {
	getConfiguracion();
	int i;
	logFile = log_create(LOG_PATH, config->nombre, false, LOG_LEVEL_ERROR);
	t_list *hilos = list_create();
	hilo_personaje_t *dataHilo;

	do {
		flagReinicioPlan = 0;

		for (i = 0; config_get_array_value(configFile, "planDeNiveles")[i] != NULL ; i++) {

			dataHilo = crearDatosPersonaje(i);
			pthread_create(dataHilo->hilo, NULL, (void *) hiloPersonaje,
					(void *) dataHilo);
			list_add(hilos, dataHilo);
		}

		for (i = 0; list_get(hilos, i) != NULL ; i++) {
			dataHilo = list_get(hilos, i);
			pthread_join(*dataHilo->hilo, (void **) NULL );
			dataHiloDestroy(dataHilo);
		}

	} while (flagReinicioPlan == 1);

	return EXIT_SUCCESS;
}

hilo_personaje_t *crearDatosPersonaje(int index) {
	hilo_personaje_t *dataHilo = malloc(sizeof(hilo_personaje_t));
	dataHilo->ipOrquestador = config->ipOrquestador;
	dataHilo->puertoOrquestador = config->puertoOrquestador;
	dataHilo->nivel = malloc(
			strlen(config_get_array_value(configFile, "planDeNiveles")[index])
					+ 1);
	strcpy(dataHilo->nivel,
			config_get_array_value(configFile, "planDeNiveles")[index]);
	char *key = getObjetivoKey(dataHilo->nivel);
	dataHilo->objetivos = config_get_array_value(configFile, key);
	dataHilo->hilo = malloc(sizeof(pthread_t));
	dataHilo->simbolo = *config_get_string_value(configFile, "simbolo");
	free(key);

	return dataHilo;
}

char *getObjetivoKey(char *nombreNivel) {
	char *key = malloc(strlen("obj[") + strlen(nombreNivel) + strlen("]") + 1);
	strcpy(key, "obj[");
	strcat(key, nombreNivel);
	strcat(key, "]");

	return key;
}

void getConfiguracion(void) {
	configFile = config_create(CONFIG_PATH);
	config = malloc(sizeof(configuracion_personaje_t));
	config->nombre = malloc(
			strlen(config_get_string_value(configFile, "nombre")) + 1);
	strcpy(config->nombre, config_get_string_value(configFile, "nombre"));
	config->simbolo = *config_get_string_value(configFile, "simbolo");
	config->vidas = config_get_int_value(configFile, "vidas");
	char *tok = ":";
	char *orqaddr = strdup(config_get_string_value(configFile, "orquestador"));
	config->ipOrquestador = strtok(orqaddr, tok);
	config->puertoOrquestador = strtok(NULL, tok);
}

void hiloPersonaje(hilo_personaje_t *datos) {
//	fd_set bagMaster, bagEscucha;
	int sockfdOrquestador = sockets_createClient(datos->ipOrquestador,
			datos->puertoOrquestador);
	enviarHandshakePersonaje(sockfdOrquestador);
//	FD_ZERO(&bagMaster);
//	FD_ZERO(&bagEscucha);
//	FD_SET(sockfdOrquestador, &bagMaster);
//	int sockfd, sockfdMax = sockfdOrquestador, nbytes;
	datos->objetivoActual = 0;
	datos->coordObjetivo = NULL;
	datos->coordPosicion = malloc(sizeof(coordenada_t));
	datos->coordPosicion->ejeX = 0;
	datos->coordPosicion->ejeY = 0;

	while (1) {
//		bagEscucha = bagMaster;
//
//		select(sockfdMax + 1, &bagEscucha, NULL, NULL, NULL );
//
//		for (sockfd = 0; sockfd < sockfdMax + 1; sockfd++) {
//			if (FD_ISSET(sockfd,&bagEscucha)) {
//				if (sockfd == sockfdOrquestador) {
//					nbytes = atenderOrquestador(sockfdOrquestador, datos);
//
//					if (nbytes == 0) //TODO: que hacer cuando la plataforma se desconecta?
//						break;
//				}
//			}
//		}
		int nbytes = atenderOrquestador(sockfdOrquestador, datos);
		if (nbytes == 0) {
//			TODO:que hacer cuando la plataforma se desconecta?
		}
		if (flagReinicioPlan) {
			rutinaReinicioPlan(sockfdOrquestador, datos);
			break;
		}
	}
}

int atenderOrquestador(int sockfdOrquestador, hilo_personaje_t *datos) {
	header_t header;
	int nbytes = recv(sockfdOrquestador, &header, sizeof(header_t),
			MSG_WAITALL);

	switch (header.type) {
	case HANDSHAKE_ORQUESTADOR:
		nbytes = enviarDatosPersonaje(sockfdOrquestador, datos);
		break;
	case TURNO_CONCEDIDO:
		nbytes = realizarMovimiento(sockfdOrquestador, datos);
		break;
	case UBICACION_CAJA:
		nbytes = recibirCoordenadas(sockfdOrquestador, datos, header);
		break;
	case OTORGAR_RECURSO:
		recibirRecurso(sockfdOrquestador , datos);
		break;
	case NEGAR_RECURSO:
		nbytes = esperarDesbloqueo(sockfdOrquestador, datos);
		break;
	case NOTIFICAR_MUERTE:
		nbytes = rutinaMuerte(sockfdOrquestador, datos, "por Enemigo");
		break;
	}

	return nbytes;
}

int realizarMovimiento(int sockfdOrquestador, hilo_personaje_t *datos) {
	int nbytes = 0;

	if (datos->coordObjetivo == NULL ) { //No hay coordenadas del objetivo
		nbytes = solicitarCoordenadasObjetivo(sockfdOrquestador,
				datos->objetivos[datos->objetivoActual]);
	} else if (obtenerDistancia(datos->coordPosicion, datos->coordObjetivo)) { //el personaje debe moverse en busca del objetivo
		coordenadaMovimientoAlternado(datos->coordPosicion,
				datos->coordObjetivo);
		nbytes = enviarNotificacionMovimiento(sockfdOrquestador,
				datos->coordPosicion, datos->simbolo);
	} else { //el personaje ya llego al objetivo
		nbytes = enviarSolicitudObjetivo(sockfdOrquestador, datos);
	}

	return nbytes;
}

int enviarSolicitudObjetivo(int sockfdOrquestador, hilo_personaje_t *datos) {
	header_t header;
	header.type = SOLICITAR_RECURSO;
	header.length = sizeof(char);

	return sockets_send(sockfdOrquestador, &header,
			datos->objetivos[datos->objetivoActual]);
}

int enviarNotificacionMovimiento(int sockfdOrquestador,
		coordenada_t * coordenada, char id) {
	header_t header;
	int16_t coordLenght;
	int nbytes, offset = 0;
	header.type = NOTIFICACION_MOVIMIENTO;
	char *coordSerialized = coordenadas_serializer(coordenada, &coordLenght);
	char *serialized = malloc(sizeof(char) + coordLenght);
	memcpy(serialized, &id, offset = sizeof(char));
	memcpy(serialized + offset, coordSerialized, coordLenght);
	header.length = coordLenght + sizeof(char);
	nbytes = sockets_send(sockfdOrquestador, &header, serialized);
	free(serialized);
	free(coordSerialized);

	return nbytes;
}

int recibirCoordenadas(int sockfdOrquestador, hilo_personaje_t *datos, header_t header) {
	char *serialized = malloc(header.length);
	int nbytes = recv(sockfdOrquestador, serialized, header.length, MSG_WAITALL);
	datos->coordObjetivo = coordenadas_deserializer(serialized);
	free(serialized);

	return nbytes;
}

int solicitarCoordenadasObjetivo(int sockfdOrquestador, char *objetivo) {
	header_t header;
	header.type = UBICACION_CAJA;
	header.length = sizeof(char);
	return sockets_send(sockfdOrquestador, &header, objetivo);
}

int enviarDatosPersonaje(int sockfdOrquestador, hilo_personaje_t *datos) {
	header_t header;
	header.type = NOTIFICAR_DATOS_PERSONAJE;
	notificacion_datos_personaje_t *notificacion = malloc(
			sizeof(notificacion_datos_personaje_t));
	notificacion->nombreNivel = malloc(strlen(datos->nivel) + 1);
	strcpy(notificacion->nombreNivel, datos->nivel);
	notificacion->simbolo = datos->simbolo;
	char *serialized = notificacionDatosPersonaje_serializer(notificacion,
			&header.length);
	int nbytes = sockets_send(sockfdOrquestador, &header, serialized);
	free(serialized);
	notificacionDatosPersonaje_destroy(notificacion);

	return nbytes;
}

void enviarHandshakePersonaje(int sockfd) {
	header_t head;
	head.type = HANDSHAKE_PERSONAJE;
	head.length = 0;

	sockets_send(sockfd, &head, '\0');
}

void recibirRecurso(int sockfdOrquestador, hilo_personaje_t *datos ) {
	coordenadas_destroy(datos->coordObjetivo);
	datos->coordObjetivo = NULL;
	datos->objetivoActual++;

	if (datos->objetivos[datos->objetivoActual] == NULL){
		rutinaFinalizarNivel(sockfdOrquestador, datos );
	}
}

int esperarDesbloqueo(int sockfdOrquestador, hilo_personaje_t *datos) {
	header_t header;

	int nbytes = recv(sockfdOrquestador, &header, sizeof(header), MSG_WAITALL);

	switch (header.type) {
	case NOTIFICAR_MUERTE:
		nbytes = rutinaMuerte(sockfdOrquestador, datos, "victima Deadlock");
		break;
	case OTORGAR_RECURSO:
		recibirRecurso(sockfdOrquestador, datos);
		break;
	}

	return nbytes;
}

int rutinaMuerte(int sockfdOrquestador, hilo_personaje_t *datos, char* causa) {

	printf("Muerte por: %s \n", causa);

	if (datos->vidas == 0) {
		//reiniciar plan de niveles
//		datos->vidas = config_get_int_value(configFile, "vidas");
//		rutinaReinicioPlan(sockfdOrquestador, datos);
		flagReinicioPlan = 1;
	} else {
		//reiniciar el nivel actual
		datos->vidas--;
		rutinaReinicioNivel(sockfdOrquestador, datos);
	}
	return 1;
}

void rutinaReinicioNivel(int sockfdOrquestador, hilo_personaje_t *datos) {
	//TODO: Esto Anda?

	reiniciarDatosNivel(datos);
	close(sockfdOrquestador);
	sockfdOrquestador = sockets_createClient(datos->ipOrquestador,
			datos->puertoOrquestador);
	enviarHandshakePersonaje(sockfdOrquestador);

}

void rutinaReinicioPlan(int sockfdOrquestador, hilo_personaje_t *datos) {
	//TODO:
	close(sockfdOrquestador);
}

void reiniciarDatosNivel(hilo_personaje_t *datos) {
	datos->objetivoActual = 0;
	if (datos->coordPosicion == NULL ) {
		datos->coordPosicion = malloc(sizeof(coordenada_t));
	}
	modificarCoordenada(datos->coordPosicion, 0, 0);

}

void rutinaFinalizarNivel(int sockfdOrquestador, hilo_personaje_t *datos ){
//	header_t header;
//	header.type = FINALIZAR_NIVEL;

}

void dataHiloDestroy(hilo_personaje_t* datos){
	//TODO: HIlo????
	free(datos->nivel);
	free(datos->objetivos);
	free(datos->ipOrquestador);
	free(datos->puertoOrquestador);
	coordenadas_destroy(datos->coordPosicion);
	coordenadas_destroy(datos->coordObjetivo);

}
