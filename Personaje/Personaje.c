/*
 * Personaje.c
 *
 *  Created on: 24/09/2013
 *      Author: utnso
 */

#include "Personaje.h"

int main(void) {

	int i, terminar = 0;
	hilo_personaje_t *dataHilo;

	signal(SIGUSR1, (funcPtr) signalRutinaVidas);
	signal(SIGTERM, (funcPtr) signalRutinaMuerte);

	mutexContadorVidas = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutexContadorVidas, NULL );

	getConfiguracion();
	inicializarLog();
	hilos = list_create();
	sem_init(&sHiloTermino, 1, 0);
	estado = malloc(sizeof(estado_t));

	do {
		vidasPlan = config->vidas;

		for (i = 0;
				config_get_array_value(configFile, "planDeNiveles")[i] != NULL ;
				i++) {

			dataHilo = crearDatosPersonaje(i);
			log_debug(logFile, "Creando hilo personaje para nivel: %s", dataHilo->nivel);
			pthread_create(&dataHilo->hilo, NULL, (void *) hiloPersonaje,
					(void *) dataHilo);
			list_add(hilos, dataHilo);
		}

		while (1) {
			sem_wait(&sHiloTermino);
			terminar = gestionarFinHilo();
			if (terminar != ESPERAR_HILO) {
				break;
			}

		}

	} while (terminar == REINICIAR);

	enviarSuccessPersonaje();

	log_debug(logFile, "Termino \n");
	return EXIT_SUCCESS;
}

//-----------------------------------------------------------------------------------------------------------------------------

hilo_personaje_t *crearDatosPersonaje(int index) {

	log_debug(logFile, "Creando datos personaje");



	hilo_personaje_t *dataHilo = malloc(sizeof(hilo_personaje_t));


	dataHilo->ipOrquestador = config->ipOrquestador;
	dataHilo->puertoOrquestador = config->puertoOrquestador;
	dataHilo->nivel = string_duplicate(config_get_array_value(configFile, "planDeNiveles")[index]);
	string_trim(&dataHilo->nivel);
//	dataHilo->nivel = malloc( strlen(config_get_array_value(configFile, "planDeNiveles")[index]) + 1);
//	strcpy(dataHilo->nivel, config_get_array_value(configFile, "planDeNiveles")[index]);
	char *key = getObjetivoKey(dataHilo->nivel);
	dataHilo->objetivos = config_get_array_value(configFile, key);
//	dataHilo->hilo = malloc(sizeof(pthread_t));
	dataHilo->simbolo = *config_get_string_value(configFile, "simbolo");

	free(key);


	return dataHilo;
}

//-----------------------------------------------------------------------------------------------------------------------------

char *getObjetivoKey(char *nombreNivel) {
	char *key = malloc(strlen("obj[") + strlen(nombreNivel) + strlen("]") + 1);
	strcpy(key, "obj[");
	strcat(key, nombreNivel);
	strcat(key, "]");

	return key;
}

//-----------------------------------------------------------------------------------------------------------------------------

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

void rutinaReinicioPlan() {

	log_info(logFile, "Reinicio Plan de niveles");
	estado->id = '\0';
	estado->motivo = FIN_REINICIO_PLAN;
	sem_post(&sHiloTermino);

}

//-----------------------------------------------------------------------------------------------------------------------------

void dataHiloDestroy(hilo_personaje_t* datos) {

	free(datos->nivel);
	free(datos->objetivos);
	free(datos->ipOrquestador);
//	free(datos->puertoOrquestador);
	coordenadas_destroy(datos->coordPosicion);
	coordenadas_destroy(datos->coordObjetivo);

}

//-----------------------------------------------------------------------------------------------------------------------------

void signalRutinaVidas() {

	if (vidasPlan > 0) {

//	pthread_mutex_lock(mutexContadorVidas);
		vidasPlan++;
//	pthread_mutex_unlock(mutexContadorVidas);
		log_info(logFile, "El personaje recibio una vida por señal, Vidas: %d",
				vidasPlan);
		printf("El personaje recibio una vida por señal, Vidas: %d \n",
				vidasPlan);

	}
}

//-----------------------------------------------------------------------------------------------------------------------------

void signalRutinaMuerte() {

	if (vidasPlan <= 0){
		return;
	}
	int hayVidas = rutinaMuerte();
	printf("El personaje perdio una vida por señal, Vidas: %d \n",
			vidasPlan);
	log_info(logFile, "El personaje perdio una vida por señal, Vidas: %d",
			vidasPlan);

	if (!hayVidas) {
		rutinaReinicioPlan();
	}

}

//-----------------------------------------------------------------------------------------------------------------------------

int rutinaMuerte() {

	int r;
	log_info(logFile, "Rutina Muerte, descontando vidas...");
//	pthread_mutex_lock(mutexContadorVidas);

//	config->vidas--;
//	r = (config->vidas > 0);
	vidasPlan--;
	r = (vidasPlan > 0);

//	pthread_mutex_unlock(mutexContadorVidas);

	return r;

}

//-----------------------------------------------------------------------------------------------------------------------------

int mostrarContinue() {

	log_info(logFile, "Lanzar continue");
	char r = 'i';
	while (r != 'S' && r != 's' && r != 'N' && r != 'n') {

		printf("El personaje ha perdido todas sus vidas, desea continuar? \n");
		printf("S/N \n");
		scanf(" %c", &r);

		if (r == 'S' || r == 's') {
			contItentos++;
			printf("Intentos: %d\n", contItentos);
			return 1;
		} else if (r == 'N' || r == 'n') {
			return 0;
		}

	}
	return r == 'S';
}

//-----------------------------------------------------------------------------------------------------------------------------

void enviarSuccessPersonaje() {
	log_info(logFile, "Envio mensaje de plan finalizado");
	header_t header;
	header.type = FINALIZAR_PLAN;
	header.length = sizeof(char);

	int sockfdPlatafoma = sockets_createClient(config->ipOrquestador,
			config->puertoOrquestador);

	sockets_send(sockfdPlatafoma, &header, &config->simbolo);
}

//-----------------------------------------------------------------------------------------------------------------------------

void inicializarLog() {
	remove(LOG_PATH);
	logFile = log_create(LOG_PATH, config->nombre, false, LOG_LEVEL_DEBUG);
	log_info(logFile,
			"----------------------------- %s --------------------------------",
			config->nombre);

}

//-----------------------------------------------------------------------------------------------------------------------------

void matarHilos() {
	log_info(logFile, "Matando Hilos");

	while (hilos->elements_count != 0) {
		hilo_personaje_t* datahilo = list_get(hilos, 0);
		if (pthread_cancel(datahilo->hilo) != 0) {
			log_error(logFile, "no se pudo matar el hilo");
		} else {
			pthread_join(datahilo->hilo, (void **) NULL );
			enviarFinNivel(datahilo);
			sacarHiloLista(datahilo);
		}
	}

}

//-----------------------------------------------------------------------------------------------------------------------------

int gestionarFinNivel(char id) {
	log_info(logFile, "Gestion del hilo por fin de nivel");
	int i;

	for (i = 0; i < hilos->elements_count; i++) {
		hilo_personaje_t* hilo = list_get(hilos, i);
		if (hilo->estadoPersonaje == FIN_NIVEL) {
			sacarHiloLista(hilo); //TODO: Testiar refactoring aca
//			dataHiloDestroy(hilo);
			break;
		}
	}

	return list_is_empty(hilos);
}

//-----------------------------------------------------------------------------------------------------------------------------

int gestionarFinHilo() {
	int ret;

	log_debug(logFile, "Semaforo main habilito, joineando hilos");
	if (estado->motivo == FIN_REINICIO_PLAN) { //Reinicia el plan
		matarHilos();
		ret = mostrarContinue() ? REINICIAR : FINALIZAR;

	} else { // Termino un hilo
		ret = gestionarFinNivel(estado->id) ? FINALIZAR : ESPERAR_HILO; //Si terminaron todos

	}

	return ret;
}

//-----------------------------------------------------------------------------------------------------------------------------

void sacarHiloLista(hilo_personaje_t* hiloPersonaje) {

	int _is_personaje(hilo_personaje_t *hPersonaje) {
		return hPersonaje->hilo == hiloPersonaje->hilo;
	}

	log_info(logFile, "Sacando personaje: %c del nivel: %s",
			hiloPersonaje->simbolo, hiloPersonaje->nivel);
	list_remove_by_condition(hilos, (void *) _is_personaje);
//	dataHiloDestroy(personaje); TODO: Ver por que rompe si quiero volver a lanzar el hilo

}

//-----------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------   HILO PERSONAJE   -----------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------

void hiloPersonaje(hilo_personaje_t *datos) {
//	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
//	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	crearClientePlanificador(datos);

	enviarHandshakePersonaje(datos->sockfdPlanificador);

	datos->objetivoActual = 0;
	datos->coordObjetivo = NULL;
	datos->coordPosicion = malloc(sizeof(coordenada_t));
	datos->coordPosicion->ejeX = 0;
	datos->coordPosicion->ejeY = 0;

	while (1) {

//		log_debug(logFile, "Testeo si me cancelaron");
//		pthread_testcancel();
		int nbytes = atenderOrquestador(datos);
		if (nbytes == 0) {
			log_warning(logFile, "Se perdio la conexion con plataforma");
//			TODO:que hacer cuando la plataforma se desconecta?
			break;
		}
		if (datos->estadoPersonaje == FIN_NIVEL) {
			log_info(logFile, "Saliendo del loop por fin de nivel");
			estado->id = datos->simbolo;
			estado->motivo = FIN_NIVEL;
			sem_post(&sHiloTermino);
			break;
		}
	}

	return;
}

//-----------------------------------------------------------------------------------------------------------------------------

void crearClientePlanificador(hilo_personaje_t* datos) {

	do {
		datos->sockfdPlanificador = sockets_createClient(datos->ipOrquestador,
				datos->puertoOrquestador);
		if (datos->sockfdPlanificador < 0) {
			sleep(1);
		}
	} while (datos->sockfdPlanificador < 0);
}

//-----------------------------------------------------------------------------------------------------------------------------

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
	case NOTIFICAR_REINICIO_PLAN: //Deprecated
		log_info(logFile, "Algun hilo perdio todas las vidas");
		break;
	default:
		log_warning(logFile, "Protocolo inesperado");
		break;
	}

	return nbytes;
}

//-----------------------------------------------------------------------------------------------------------------------------

int enviarDatosPersonaje(hilo_personaje_t *datos) {
	log_info(logFile, "Enviando datos de personaje para el nivel: %s", datos->nivel);
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

//-----------------------------------------------------------------------------------------------------------------------------

int realizarMovimiento(hilo_personaje_t *datos) {
	int nbytes = 0;

	if (datos->coordObjetivo == NULL ) { //No hay coordenadas del objetivo
		log_debug(logFile, "Solicitando coordenadas del objetivo: %s en el nivel: %s", datos->objetivos[datos->objetivoActual], datos->nivel);
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

//-----------------------------------------------------------------------------------------------------------------------------

int solicitarCoordenadasObjetivo(int sockfdOrquestador, char *objetivo) {
	header_t header;
	header.type = UBICACION_CAJA;
	header.length = sizeof(char);
	return sockets_send(sockfdOrquestador, &header, objetivo);
}

//-----------------------------------------------------------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------------------------------------------------------

void recibirRecurso(hilo_personaje_t *datos) {
	log_info(logFile, "Recurso Otorgado");
	coordenadas_destroy(datos->coordObjetivo);
	datos->coordObjetivo = NULL;
	datos->objetivoActual++;

	if (datos->objetivos[datos->objetivoActual] == NULL ) {
		log_info(logFile, "Ultimo recurso otorgado del Nivel: %s", datos->nivel);
		rutinaFinalizarNivel(datos);
	}
}

//-----------------------------------------------------------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------------------------------------------------------

int hiloRutinaMuerte(hilo_personaje_t *datos, char* causa) {

	printf("Muerte por: %s \n", causa);
	log_info(logFile, "Muerte por: %s", causa);

	int r;

	pthread_mutex_lock(mutexContadorVidas);
	r = rutinaMuerte();
	pthread_mutex_unlock(mutexContadorVidas);

	if (r) { //reiniciar el nivel actual

		rutinaReinicioNivel(datos);

	} else { //reiniciar plan de niveles

		if (flagReinicioPlan == 0) {
			rutinaReinicioPlan();
		}
	}

	return 1;
}

//-----------------------------------------------------------------------------------------------------------------------------

void rutinaReinicioNivel(hilo_personaje_t *datos) {
	//TODO: Esto Anda?
	log_info(logFile, "Reinicio de nivel: %s", datos->nivel);
	reiniciarDatosNivel(datos);
	close(datos->sockfdPlanificador);
	sleep(1);
	datos->sockfdPlanificador = sockets_createClient(datos->ipOrquestador,
			datos->puertoOrquestador);
	enviarHandshakePersonaje(datos->sockfdPlanificador);

}

//-----------------------------------------------------------------------------------------------------------------------------

int enviarSolicitudObjetivo(hilo_personaje_t *datos) {
	char* obj= datos->objetivos[datos->objetivoActual];
	log_debug(logFile, "Solicitando recurso: %s, del nivel: %s", obj, datos->nivel);
	header_t header;
	header.type = SOLICITAR_RECURSO;
	header.length = sizeof(char);

	return sockets_send(datos->sockfdPlanificador, &header,
			obj);
}

//-----------------------------------------------------------------------------------------------------------------------------

int recibirCoordenadas(hilo_personaje_t *datos, header_t header) {
	char *serialized = malloc(header.length);
	int nbytes = recv(datos->sockfdPlanificador, serialized, header.length,
			MSG_WAITALL);
	datos->coordObjetivo = coordenadas_deserializer(serialized);
	free(serialized);

	return nbytes;
}

//-----------------------------------------------------------------------------------------------------------------------------

void enviarHandshakePersonaje(int sockfd) {
	log_info(logFile, "Enviando handshake personaje");
	header_t head;
	head.type = HANDSHAKE_PERSONAJE;
	head.length = 0;

	sockets_send(sockfd, &head, '\0');
}

//-----------------------------------------------------------------------------------------------------------------------------

void enviarFinNivel(hilo_personaje_t *datos) {

	header_t header;
	header.type = FINALIZAR_NIVEL;
	header.length = 0;

	sockets_send(datos->sockfdPlanificador, &header, '\0');
}

//-----------------------------------------------------------------------------------------------------------------------------

void reiniciarDatosNivel(hilo_personaje_t *datos) {
	datos->objetivoActual = 0;
	if (datos->coordPosicion == NULL ) {
		datos->coordPosicion = malloc(sizeof(coordenada_t));
	}

	modificarCoordenada(datos->coordPosicion, 0, 0);

	if (datos->coordObjetivo != NULL ) {
		free(datos->coordObjetivo);
		datos->coordObjetivo = NULL;
	}
}

//-----------------------------------------------------------------------------------------------------------------------------

void rutinaFinalizarNivel(hilo_personaje_t *datos) {
	log_info(logFile, "Finalizar Nivel: %s", datos->nivel);
	enviarFinNivel(datos);
	close(datos->sockfdPlanificador);
	datos->estadoPersonaje = FIN_NIVEL;

}

