/*
 * Personaje.c
 *
 *  Created on: 24/09/2013
 *      Author: utnso
 */

#include "Personaje.h"

int main(void) {

	signal(SIGUSR1, (funcPtr) signalRutinaVidas);
	signal(SIGTERM, (funcPtr) signalRutinaMuerte);

	getConfiguracion();
	int i;

	mutexContadorVidas = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutexContadorVidas, NULL );
	logFile = log_create(LOG_PATH, config->nombre, false, LOG_LEVEL_ERROR);
	hilos = list_create();
	hilo_personaje_t *dataHilo;

	do {
		flagReinicioPlan = 0;

		for (i = 0;
				config_get_array_value(configFile, "planDeNiveles")[i] != NULL ;
				i++) {

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

		if (mostrarContinue() == 0) {
			break;
		}

	} while (flagReinicioPlan == 1);

	//TODO: enviar mensaje personaje finalizo correctamente... (si no continuo se debe tomar de forma positiva)
	enviarSuccessPersonaje();


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
	crearClientePlanificador(datos);

	enviarHandshakePersonaje(datos->sockfdPlanificador);
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
//					if (nbytes == 0)
//						break;
//				}
//			}
//		}
		int nbytes = atenderOrquestador(datos);
		if (nbytes == 0) {
//			TODO:que hacer cuando la plataforma se desconecta?
		}
		if (flagReinicioPlan) {
//			rutinaReinicioPlan(datos);
			break;
		}
	}
}

int atenderOrquestador(hilo_personaje_t *datos) {
	header_t header;
	int nbytes = recv(datos->sockfdPlanificador, &header, sizeof(header_t),
			MSG_WAITALL);

	switch (header.type) {
	case HANDSHAKE_ORQUESTADOR:
		nbytes = enviarDatosPersonaje(datos);
		break;
	case TURNO_CONCEDIDO:
		nbytes = realizarMovimiento(datos);
		break;
	case UBICACION_CAJA:
		nbytes = recibirCoordenadas(datos, header);
		break;
	case OTORGAR_RECURSO:
		recibirRecurso(datos);
		break;
	case NEGAR_RECURSO:
		nbytes = esperarDesbloqueo(datos);
		break;
	case NOTIFICAR_MUERTE:
		nbytes = hiloRutinaMuerte(datos, "por Enemigo");
		break;
	}

	return nbytes;
}

int realizarMovimiento(hilo_personaje_t *datos) {
	int nbytes = 0;

	if (datos->coordObjetivo == NULL ) { //No hay coordenadas del objetivo
		nbytes = solicitarCoordenadasObjetivo(datos->sockfdPlanificador,
				datos->objetivos[datos->objetivoActual]);
	} else if (obtenerDistancia(datos->coordPosicion, datos->coordObjetivo)) { //el personaje debe moverse en busca del objetivo
		coordenadaMovimientoAlternado(datos->coordPosicion,
				datos->coordObjetivo);
		nbytes = enviarNotificacionMovimiento(datos->sockfdPlanificador,
				datos->coordPosicion, datos->simbolo);
	} else { //el personaje ya llego al objetivo
		nbytes = enviarSolicitudObjetivo(datos);
	}

	return nbytes;
}

int enviarSolicitudObjetivo(hilo_personaje_t *datos) {
	header_t header;
	header.type = SOLICITAR_RECURSO;
	header.length = sizeof(char);

	return sockets_send(datos->sockfdPlanificador, &header,
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

int recibirCoordenadas(hilo_personaje_t *datos, header_t header) {
	char *serialized = malloc(header.length);
	int nbytes = recv(datos->sockfdPlanificador, serialized, header.length,
			MSG_WAITALL);
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

int enviarDatosPersonaje(hilo_personaje_t *datos) {
	header_t header;
	header.type = NOTIFICAR_DATOS_PERSONAJE;
	notificacion_datos_personaje_t *notificacion = malloc(
			sizeof(notificacion_datos_personaje_t));
	notificacion->nombreNivel = malloc(strlen(datos->nivel) + 1);
	strcpy(notificacion->nombreNivel, datos->nivel);
	notificacion->simbolo = datos->simbolo;
	char *serialized = notificacionDatosPersonaje_serializer(notificacion,
			&header.length);
	int nbytes = sockets_send(datos->sockfdPlanificador, &header, serialized);
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

void recibirRecurso(hilo_personaje_t *datos) {
	coordenadas_destroy(datos->coordObjetivo);
	datos->coordObjetivo = NULL;
	datos->objetivoActual++;

	if (datos->objetivos[datos->objetivoActual] == NULL ) {
		rutinaFinalizarNivel(datos);
	}
}

int esperarDesbloqueo(hilo_personaje_t *datos) {
	header_t header;

	int nbytes = recv(datos->sockfdPlanificador, &header, sizeof(header),
			MSG_WAITALL);

	switch (header.type) {
	case NOTIFICAR_MUERTE:
		nbytes = hiloRutinaMuerte(datos, "victima Deadlock");
		break;
	case OTORGAR_RECURSO:
		recibirRecurso(datos);
		break;
	}

	return nbytes;
}

int hiloRutinaMuerte(hilo_personaje_t *datos, char* causa) {

	printf("Muerte por: %s \n", causa);

	if (rutinaMuerte()) { //reiniciar el nivel actual

		rutinaReinicioNivel(datos);

	} else { //reiniciar plan de niveles

		if (flagReinicioPlan == 0) {
			rutinaReinicioPlan();
		}
	}

	return 1;
}

void rutinaReinicioNivel(hilo_personaje_t *datos) {
	//TODO: Esto Anda?

	reiniciarDatosNivel(datos);
	close(datos->sockfdPlanificador);
	datos->sockfdPlanificador = sockets_createClient(datos->ipOrquestador,
			datos->puertoOrquestador);
	enviarHandshakePersonaje(datos->sockfdPlanificador);

}

void rutinaReinicioPlan() {

	flagReinicioPlan = 1;
	int i;
	for (i = 0; i < hilos->elements_count; i++) {
		hilo_personaje_t* hilo = list_get(hilos, i);

		header_t head;
		head.type = NOTIFICAR_REINICIO_PLAN;

		sockets_send(hilo->sockfdPlanificador, &head, '\0');

	}

}

void reiniciarDatosNivel(hilo_personaje_t *datos) {
	datos->objetivoActual = 0;
	if (datos->coordPosicion == NULL ) {
		datos->coordPosicion = malloc(sizeof(coordenada_t));
	}
	modificarCoordenada(datos->coordPosicion, 0, 0);

}

void rutinaFinalizarNivel(hilo_personaje_t *datos) {
	header_t header;
	header.type = FINALIZAR_NIVEL;
	header.length = 0;

	sockets_send(datos->sockfdPlanificador, &header, '\0');

	close(datos->sockfdPlanificador);

}

void dataHiloDestroy(hilo_personaje_t* datos) {
	//TODO: HIlo????
	free(datos->nivel);
	free(datos->objetivos);
	free(datos->ipOrquestador);
	free(datos->puertoOrquestador);
	coordenadas_destroy(datos->coordPosicion);
	coordenadas_destroy(datos->coordObjetivo);

}

void signalRutinaVidas() {

	pthread_mutex_lock(mutexContadorVidas);
	config->vidas++;
	pthread_mutex_unlock(mutexContadorVidas);
}

void signalRutinaMuerte() {

	int hayVidas = rutinaMuerte();

	if (!hayVidas) {
		rutinaReinicioPlan();
	}

}

int rutinaMuerte() {

	int r;

	pthread_mutex_lock(mutexContadorVidas);

	config->vidas--;
	r = (config->vidas > 0);

	pthread_mutex_unlock(mutexContadorVidas);

	return r;

}

int mostrarContinue() {

	char r = 'i';
	while (1) {

		printf("El personaje ha perdido todas sus vidas, desea continuar \n");
		printf("S/N ");
		scanf("%c", &r);

		if (r == 'S') {
			contItentos++;
			return 1;
		} else if (r == 'N') {
			return 0;
		}

	}
	return r == 'S';
}

void enviarSuccessPersonaje(){
	header_t header;
	header.type = SOLICITAR_RECURSO;
	header.length = sizeof(char);

	int sockfdPlatafoma = sockets_createClient(config->ipOrquestador, config->puertoOrquestador);

	sockets_send(sockfdPlatafoma, &header, &config->simbolo);
}

void crearClientePlanificador(hilo_personaje_t* datos){

	do{
		datos->sockfdPlanificador = sockets_createClient(datos->ipOrquestador, datos->puertoOrquestador);
		if (!datos->sockfdPlanificador < 0){
			sleep(1);
		}
	}while(datos->sockfdPlanificador < 0);
}
