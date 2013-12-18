/*
 * Nivel.c
 *
 *  Created on: 24/09/2013
 *      Author: utnso
 */

#include "Nivel.h"

int main(void) {
	fd_set bagMaster, bagEscucha;
	int sockfd, sockfdMax = plataformaSockfd, nbytes;
	int notifyfd;

	listaRecursos = list_create();
	listaPersonajes = list_create();
	listaEnemigos = list_create();

	mutexDibujables = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutexDibujables, NULL );

	configObj = inicializarCongiuracionNivel();
	inicializarLog();
	inicializarInterfazGrafica();
	inicializarConfiguracionCajas();
	dibujar();
	inicializarConexionPlataforma();
	crearHiloEnemigo();
	crearHiloDeadLock();

	notifyfd = getNotifyFileDescriptor();
	FD_ZERO(&bagMaster);
	FD_ZERO(&bagEscucha);
	FD_SET(plataformaSockfd, &bagMaster);
	FD_SET(notifyfd, &bagMaster);

	if (notifyfd > sockfdMax) {
		sockfdMax = notifyfd;
	}

	while (1) {

		bagEscucha = bagMaster;

		select(sockfdMax + 1, &bagEscucha, NULL, NULL, NULL );

		for (sockfd = 0; sockfd < sockfdMax + 1; sockfd++) {
			if (FD_ISSET(sockfd,&bagEscucha)) {
				if (sockfd == plataformaSockfd) {
					nbytes = atenderMensajePlanificador(plataformaSockfd);
				} else if (sockfd == notifyfd) {
					tratarModificacionAlgoritmo(notifyfd);
				}
			}
		}

		if (nbytes <= 0) {
			log_error(logFile,
					"Finalizacion por que se perdio la conexion con plataforma");
			break;
		}
	}

	nivel_gui_terminar();
	return EXIT_SUCCESS;
}

//-----------------------------------------------------------------------------------------------------------------------------

NIVEL_CONF* inicializarCongiuracionNivel() {
	t_config* conF = config_create(CONFIG_PATH);
	NIVEL_CONF *configObj = malloc(sizeof(NIVEL_CONF));

	if (config_has_property(conF, "Nombre")) {
		configObj->nombre = malloc(
				strlen(config_get_string_value(conF, "Nombre")) + 1);
		strcpy(configObj->nombre, config_get_string_value(conF, "Nombre"));
	}
	if (config_has_property(conF, "TiempoChequeoDeadlock")) {
		configObj->deadlockTime = config_get_int_value(conF,
				"TiempoChequeoDeadlock");
	}
	if (config_has_property(conF, "Recovery")) {
		configObj->recovery = config_get_int_value(conF, "Recovery");
	}
	char *tok = ":";
	if (config_has_property(conF, "orquestador")) {
		char *orqaddr = strdup(config_get_string_value(conF, "orquestador"));
		configObj->orquestadoraddr = strtok(orqaddr, tok);
		configObj->orquestadorport = strtok(NULL, tok);
	}
	if (config_has_property(conF, "localhost")) {
		char *locaddr = strdup(config_get_string_value(conF, "localhost"));
		configObj->localhostaddr = strtok(locaddr, tok);
		configObj->localhostport = strtok(NULL, tok);
	}
	if (config_has_property(conF, "Log")) {
		configObj->logLevel = malloc(
				strlen(config_get_string_value(conF, "Log")));
		strcpy(configObj->logLevel, config_get_string_value(conF, "Log"));
	}
	if (config_has_property(conF, "Enemigos")) {
		configObj->enemigos = config_get_int_value(conF, "Enemigos");
	}
	if (config_has_property(conF, "Sleep_Enemigos")) {
		configObj->sleepEnemigos = config_get_int_value(conF, "Sleep_Enemigos");
	}
	config_destroy(conF);
	return configObj;
}

//-----------------------------------------------------------------------------------------------------------------------------

void inicializarInterfazGrafica() {

	if (desactivarGUI) {
		return;
	}

	if (nivel_gui_inicializar() == -1) {
		log_error(logFile, "No se pudo inicializar la interfaz grafica");
	}
	if (nivel_gui_get_area_nivel(&fil, &col) == -1) {
		log_error(logFile, "No se pudo inicializar el area del nivel");
	}
	log_info(logFile, "Interfaz grafica inicializada, filas: %d, columnas: %d",
			fil, col);

}

//-----------------------------------------------------------------------------------------------------------------------------

void inicializarConfiguracionCajas() {
	t_config* configFile = config_create(CONFIG_PATH);
	char s[20], t[20];
	char*u = "Caja1";
	int i = 1;

	while (config_get_string_value(configFile, u) != NULL ) {

		strcpy(s, "Caja");
		sprintf(t, "%d", i);
		u = strcat(s, t);
		if (config_get_string_value(configFile, u) != NULL ) {
			char **array_values = string_split(
					config_get_string_value(configFile, u), ",");
			crearCajasConVector(array_values);
		} else {
			log_warning(logFile, "No se pudo inicializar la caja: %s", u);
		}
		i++;
	};
	config_destroy(configFile);
}

//-----------------------------------------------------------------------------------------------------------------------------

void crearCajasConVector(char** array) {
	char id = array[1][0];
	int instancia = atoi(array[2]);
	int pos_x = atoi(array[3]);
	int pos_y = atoi(array[4]);

	if (pos_x <= col && pos_y <= fil) {
		CrearCaja(listaRecursos, id, pos_x, pos_y, instancia);
		log_info(logFile, "Se creo la caja:%c, cantidad:%d, Pos:(%d,%d)", id,
				instancia, pos_x, pos_y);
	} else {
		log_warning(logFile,
				"No se creo la caja:%c, cantidad:%d, Pos:(%d,%d), sobrepasa los limites",
				id, instancia, pos_x, pos_y);
	}
}

//-----------------------------------------------------------------------------------------------------------------------------

void separador_log(char* head) {
	log_info(logFile,
			"------------------------------ %s -------------------------------",
			head);
}

//-----------------------------------------------------------------------------------------------------------------------------

void inicializarConexionPlataforma() {

	do {
		sleep(1);
		plataformaSockfd = conectarOrquestador();
	} while (plataformaSockfd == -1);

	hacerHandshake(plataformaSockfd);
	enviarDatosAlgoritmo();

}

//-----------------------------------------------------------------------------------------------------------------------------

int conectarOrquestador() {

	int orquestadorSockfd = sockets_createClient(configObj->orquestadoraddr,
			configObj->orquestadorport);

	if (orquestadorSockfd == -1) {
		log_error(logFile, "Conexion con orquestador fallo");
//		exit(EXIT_FAILURE);
	} else {
		log_info(logFile, "Conectado con orquestador, ip: %s, port: %s",
				configObj->orquestadoraddr, configObj->orquestadorport);
	}

	return orquestadorSockfd;
}

//-----------------------------------------------------------------------------------------------------------------------------

int hacerHandshake(int sockfdReceptor) {
	//Envio mi hanshake
	int r;
	header_t header;
	header.type = HANDSHAKE_NIVEL;
	header.length = 0;
	sockets_send(sockfdReceptor, &header, '\0');
	//Espero hanshake de vuelta
	r = recv(sockfdReceptor, &header, sizeof(header), MSG_WAITALL);
	if (r == 0) {
		log_error(logFile, "Conexion al tratar de hacer handshake.");
	}
	switch (header.type) {
	case HANDSHAKE_PLANIFICADOR:
		log_info(logFile, "Handshake Planificador.");
		break;
	case HANDSHAKE_ORQUESTADOR:
		log_info(logFile, "Handshake Orquestador.");
		break;
	default:
		log_error(logFile, "Handshake no reconocido.");
	}
	return r;
}

//-----------------------------------------------------------------------------------------------------------------------------

void enviarDatosAlgoritmo() {
	header_t h;
	int16_t lenght;
	informacion_planificacion_t* datosAlgoritmo = malloc(
			sizeof(informacion_planificacion_t));
	char* data;

	obtenerDatosAlgorimo(datosAlgoritmo);
	log_info(logFile, "nombre=%c algoritmo=%d retardo=%d quantum=%d",
			datosAlgoritmo->nombreNivel, datosAlgoritmo->algoritmo,
			datosAlgoritmo->retardo, datosAlgoritmo->quantum);
	data = informacionPlanificacion_serializer(datosAlgoritmo, &lenght);

	h.type = NOTIFICAR_ALGORITMO_PLANIFICACION;
	h.length = lenght;

	sockets_send(plataformaSockfd, &h, data);

	free(data);

}

//-----------------------------------------------------------------------------------------------------------------------------

void recibirDatosPlanificador() {
	header_t header;
	char* data;
	if (recv(plataformaSockfd, &header, sizeof(header), MSG_WAITALL) == 0) {
		log_error(logFile, "Conexion perdida con el planificador");
	}

	if (header.type == NOTIFICAR_DATOS_PLANIFICADOR) {
		data = malloc(header.length);
		recv(plataformaSockfd, data, header.length, MSG_WAITALL);
		ip_info_t *infoPlanificador = ipInfo_deserializer(data);
		log_info(logFile, "Planificador IP: %s port: %s",
				infoPlanificador->addr, infoPlanificador->port);
	}
	free(data);
}

//-----------------------------------------------------------------------------------------------------------------------------

int atenderMensajePlanificador(int sockfd) {
	header_t h;
	t_list* asignados;
	int nbytes = validarRecive(sockfd, &h);
	if (nbytes) {
		char* data = malloc(h.length);
		switch (h.type) {
		case NOTIFICAR_DATOS_PERSONAJE:
			nbytes = recv(sockfd, data, h.length, MSG_WAITALL);
			tratarNuevoPersonaje(data);
			break;
		case UBICACION_CAJA:
			nbytes = recv(sockfd, data, h.length, MSG_WAITALL);
			tratarSolicitudUbicacionCaja(data);
			break;
		case NOTIFICACION_MOVIMIENTO:
			nbytes = recv(sockfd, data, h.length, MSG_WAITALL);
			tratarMovimiento(data);
			break;
		case SOLICITAR_RECURSO:
			nbytes = recv(sockfd, data, h.length, MSG_WAITALL);
			tratarSolicitudRecurso(data);
			break;
		case PERSONAJE_FINALIZO:
			nbytes = recv(sockfd, data, h.length, MSG_WAITALL);
			tratarFinalizacionPersonaje(data);
			break;
		case NOTIFICACION_RECURSOS_ASIGNADOS:
//			nbytes = recv(sockfd, data, h.length, MSG_WAITALL);
			asignados = esperarRecursosAsignados(h);
			actualizarEstado(asignados);
			list_destroy(asignados);
			dibujar();
			break;
		default:
			log_error(logFile,
					"Protocolo invalido (%d) para comunicarse con el nivel",
					h.type);
			break;
		}
		free(data);
	}
	return nbytes;
}

//-----------------------------------------------------------------------------------------------------------------------------

int validarRecive(int sockfd, header_t* h) {
	int nbytes;
	if ((nbytes = recv(sockfd, h, sizeof(header_t), MSG_WAITALL)) <= 0) {
		if (nbytes == 0) { //conexion cerrada
			log_warning(logFile, "Socket: %d, desconectado inesperadamente",
					sockfd);
		} else
			log_error(logFile, "Error al receive de socket: %d", sockfd);
	}
	return nbytes;
}

//-----------------------------------------------------------------------------------------------------------------------------

void obtenerDatosAlgorimo(informacion_planificacion_t* datosAlgoritmo) {
	t_config* conF = config_create(CONFIG_PATH);

	if (config_has_property(conF, "Nombre")) {
		datosAlgoritmo->nombreNivel = malloc(
				strlen(config_get_string_value(conF, "Nombre")) + 1);
		strcpy(datosAlgoritmo->nombreNivel,
				config_get_string_value(conF, "Nombre"));
	}

	if (config_has_property(conF, "algoritmo")) {
		char* algoritmo = config_get_string_value(conF, "algoritmo");
		datosAlgoritmo->algoritmo =
				strcmp("RR", algoritmo) == 0 ? ROUND_ROBIN : SRDF;
	}

	if (config_has_property(conF, "quantum")) {
		datosAlgoritmo->quantum = config_get_int_value(conF, "quantum");
	}

	if (config_has_property(conF, "retardo")) {
		datosAlgoritmo->retardo = config_get_int_value(conF, "retardo");
	}

	config_destroy(conF);
}

//-----------------------------------------------------------------------------------------------------------------------------

void tratarNuevoPersonaje(char* data) {
	char pId;

	pId = data[0];
	log_info(logFile, "Creando Personaje %c", pId);
	CrearPersonaje(listaPersonajes, pId, 0, 0, 1, orden);
	orden++;
	dibujar();
}

//-----------------------------------------------------------------------------------------------------------------------------

void tratarSolicitudUbicacionCaja(char* data) {

	char rId = data[0];
	char pId = data[1];
	coordenada_t* coord;
	header_t h;
	int16_t length;
	char* coordSerialized;
	h.type = UBICACION_CAJA;
	h.length = sizeof(char);

	log_info(logFile, "Atendiendo pedido de posicion de caja %c para %c", rId,
			pId);
	coord = obtenerCoordenadas(listaRecursos, rId, logFile);
	coordSerialized = coordenadas_serializer(coord, &length);

	char* dataSend = malloc(h.length + length);
	memcpy(dataSend, &pId, h.length);
	memcpy(dataSend + h.length, coordSerialized, length);
	h.length += length;
	sockets_send(plataformaSockfd, &h, dataSend);

//	if (h.length == 9)
//		log_info(logFile, "type=%d length=%d idPersonaje=%c.", h.type, h.length,
//				pId);
	free(coordSerialized);
	coordenadas_destroy(coord);
}

//-----------------------------------------------------------------------------------------------------------------------------

void tratarMovimiento(char* data) {
	char pId;
	coordenada_t* pCoordenada;
	int offset = sizeof(char);

	pId = data[0];
	pCoordenada = coordenadas_deserializer(data + offset);

	MoverPersonaje(listaPersonajes, pId, pCoordenada->ejeX, pCoordenada->ejeY,
			logFile);

	dibujar();

	free(pCoordenada);
}

//-----------------------------------------------------------------------------------------------------------------------------

void tratarSolicitudRecurso(char* data) {

	personaje_recurso_t* personaje;
	int respuesta;

	personaje = personajeRecurso_deserializer(data);
	log_info(logFile, "Personaje %c solicito recurso %c.",
			personaje->idPersonaje, personaje->idRecurso);
	if (personajeEnCaja(personaje->idPersonaje, personaje->idRecurso)) {
		respuesta = darRecursoPersonaje(listaPersonajes, listaRecursos,
				personaje->idPersonaje, personaje->idRecurso, logFile);
	} else {
		log_error(logFile, "acceso incorrecto a caja de recurso: %s",
				personaje->idRecurso);
	}
	if (respuesta) {
		log_info(logFile, "Se le asigno el recurso: %c, al personaje: %c",
				personaje->idRecurso, personaje->idPersonaje);
		notificacionDarRecurso(personaje->idPersonaje);
		dibujar();
	} else {
		log_info(logFile, "Se bloqueo el personaje: %c, al pedir el recruso %c",
				personaje->idPersonaje, personaje->idRecurso);
		notificacionBloqueo(personaje->idPersonaje);
	}
}

//-----------------------------------------------------------------------------------------------------------------------------

void tratarFinalizacionPersonaje(char* data) {
//  TODO: REvisar con plataforma
	char pId;
	t_list* adquiridos = NULL;
//	t_list* asignados = NULL;

	pId = data[0];
	log_info(logFile, "Matando personaje..");
	adquiridos = getObjetosAdquiridosSerializable(listaPersonajes, pId);
	log_error(logFile, "Saco %c de listaPersonajes.", pId);
	matarPersonaje(listaPersonajes, listaRecursos, pId, logFile);
	mandarRecursosLiberados(adquiridos);
//	asignados = esperarRecursosAsignadosMain();
//	actualizarEstado(asignados);
//	listaRecursos_destroy(adquiridos);
//	listaRecursos_destroy(asignados);
//	log_info(logFile, "El personaje: %c, finalizo el nivel", pId);
//	dibujar();
	list_destroy_and_destroy_elements(adquiridos, (void *)free);
}

//-----------------------------------------------------------------------------------------------------------------------------

void notificarMuertePersonaje(char id, int causa) {
	header_t h;
	t_list* adquiridos = NULL;
//	t_list* asignados = NULL;
	int16_t length;

	h.type = causa;
	h.length = sizeof(char);

	adquiridos = getObjetosAdquiridosSerializable(listaPersonajes, id);
	log_error(logFile, "saco %c de listaPersonajes.", id);
	matarPersonaje(listaPersonajes, listaRecursos, id, logFile);
	char* adqData = listaRecursos_serializer(adquiridos, &length);
	char* data = malloc(h.length + length);
	memcpy(data, &id, h.length);
	memcpy(data + h.length, adqData, length);
	h.length += length;
	sockets_send(plataformaSockfd, &h, data);
//	asignados = esperarRecursosAsignados();
//	actualizarEstado(asignados);

	listaRecursos_destroy(adquiridos);
//	listaRecursos_destroy(asignados);
	free(data);
}

//-----------------------------------------------------------------------------------------------------------------------------

void notificacionDarRecurso(char id) {
	header_t header;
	header.type = OTORGAR_RECURSO;
	header.length = 0;
	sockets_send(plataformaSockfd, &header, '\0');
}

//-----------------------------------------------------------------------------------------------------------------------------

void notificacionBloqueo(char id) {
	header_t header;
	header.type = NEGAR_RECURSO;
	header.length = 0;
	sockets_send(plataformaSockfd, &header, '\0');
}

//-----------------------------------------------------------------------------------------------------------------------------

void dibujar() {

	if (desactivarGUI) {
		return;
	}
	log_trace(logFile, "Dibujando...");
	pthread_mutex_lock(mutexDibujables);

	t_list* tempI;

	tempI = list_create();

	list_add_all(tempI, listaRecursos);
	list_add_all(tempI, listaPersonajes);
	list_add_all(tempI, listaEnemigos);

	if (nivel_gui_dibujar(tempI, configObj->nombre) == -1) {
		log_info(logFile, "No se puedo dibujar");
		//      EXIT_FAILURE;
	} else {
		log_trace(logFile, "Dibujado...");
	}

	pthread_mutex_unlock(mutexDibujables);

}

//-----------------------------------------------------------------------------------------------------------------------------

void crearHiloEnemigo() {

	int i = 0;
	for (i = 0; i < configObj->enemigos; i++) {
		pthread_t hEnemigo;
		pthread_create(&hEnemigo, NULL, (void*) enemigo, (void*) i);
		sleep(1);
	}
}

//-----------------------------------------------------------------------------------------------------------------------------

void crearHiloDeadLock() {
	pthread_t hDeadLock;
	pthread_create(&hDeadLock, NULL, (void*) deadLock, NULL );
}

//-----------------------------------------------------------------------------------------------------------------------------

int personajeEnCaja(char pId, char rId) {

	int enCaja = 0;
	coordenada_t* posCaja = obtenerCoordenadas(listaRecursos, rId, logFile);
	coordenada_t* posPersonaje = obtenerCoordenadas(listaPersonajes, pId,
			logFile);

	if (posPersonaje != NULL )
		enCaja = coordenadasIguales(posCaja, posPersonaje);
	else
		enCaja = 1;

	coordenadas_destroy(posCaja);
	coordenadas_destroy(posPersonaje);

	return enCaja;
}

//-----------------------------------------------------------------------------------------------------------------------------

void mandarRecursosLiberados(t_list* recursosLiberados) {
	header_t header;
	header.type = NOTIFICACION_RECURSOS_LIBERADOS;
	int16_t length;
	char* data = listaRecursos_serializer(recursosLiberados, &length);
	header.length = length;

	if (sockets_send(plataformaSockfd, &header, data) == 0) {

		log_error(logFile, "Se perdio la conexion con orquestador");
	} else {
		log_info(logFile, "Enviando recuros liberados al planificador");
	}
	free(data);
}

//-----------------------------------------------------------------------------------------------------------------------------

t_list* esperarRecursosAsignados(header_t header) {
	log_info(logFile, "Recibiendo recursos asignados");
//	header_t header;
	t_list* asignados = NULL;
	char* data = NULL;
//	if (recv(plataformaSockfd, &header, sizeof(header), MSG_WAITALL) == 0) {
//		log_error(logFile, "Conexion perdida con el orqestador");
//	}
//
//	if (header.type == NOTIFICACION_RECURSOS_ASIGNADOS) {
	data = malloc(header.length);
	if (header.length > 0) {
		recv(plataformaSockfd, data, header.length, MSG_WAITALL);
		asignados = listaRecursos_deserializer(data, header.length);
	} else if (header.length == 0) {
		log_debug(logFile, "El orquestador no uso ningun recurso");
		return asignados = list_create();
	}

//	} else {
//		log_error(logFile, "Mensaje inesperado del Orquestador al esperar asignados");
//	}
	free(data);
	return asignados;
}

//-----------------------------------------------------------------------------------------------------------------------------

//t_list* esperarRecursosAsignadosMain() {
//	log_info(logFile, "Recibiendo recursos asignados");
//	header_t header;
//	t_list* asignados = NULL;
//	char* data;
//
//	if (recv(plataformaSockfd, &header, sizeof(header), MSG_WAITALL) == 0) {
//		log_error(logFile, "Conexion perdida con el orqestador");
//	}
//
//	if (header.type == NOTIFICACION_RECURSOS_ASIGNADOS) {
//		data = malloc(header.length);
//
//		if (header.length > 0) {
//			recv(plataformaSockfd, data, header.length, MSG_WAITALL);
//			asignados = listaRecursos_deserializer(data, header.length);
//		} else if (header.length == 0) {
//			log_debug(logFile, "El orquestador no uso ningun recurso");
//			return asignados = list_create();
//		}
//
//		free(data);
//	} else {
//		log_error(logFile,
//				"Mensaje inesperado del Orquestador al esperar asignados");
//		log_error(logFile, "Type=%d length=%d.", header.type, header.length);
//	}
//
//	return asignados;
//}

//-----------------------------------------------------------------------------------------------------------------------------

void actualizarEstado(t_list* asignados) {

	log_debug(logFile, "Actualizando estado de recurso y personajes...");
	personaje_recurso_t* personaje;
	int i;
	for (i = 0; i < asignados->elements_count; ++i) {
		personaje = list_get(asignados, i);
		log_debug(logFile, "Plataforma le dio el recurso: %c al personaje %c",
				personaje->idRecurso, personaje->idPersonaje);
		darRecursoPersonaje(listaPersonajes, listaRecursos,
				personaje->idPersonaje, personaje->idRecurso, logFile);
	}

}

//-----------------------------------------------------------------------------------------------------------------------------

int getNotifyFileDescriptor() {

	int file_descriptor = inotify_init();
	if (file_descriptor < 0) {
		log_error(logFile, "Al inicializar el inotify");
	} else {
		log_info(logFile, "Inotify inicializado");
	}

	int watch_descriptor = inotify_add_watch(file_descriptor, CONFIG_PATH,
			IN_MODIFY);
	if (watch_descriptor < 0) {
		log_error(logFile, "Al agregar el watch");
	} else {
		log_info(logFile, "Watch agregado");
	}

	return file_descriptor;

}

//-----------------------------------------------------------------------------------------------------------------------------

void tratarModificacionAlgoritmo(int file_descriptor) {

	char buffer[BUF_LEN];
	int length = read(file_descriptor, buffer, BUF_LEN);
	if (length < 0) {
		log_error(logFile, "Notify read");
	}
	int offset = 0;
	while (offset < length) {

		struct inotify_event *event = (struct inotify_event *) &buffer[offset];

		if (event->mask & IN_MODIFY) {
			if (!(event->mask & IN_ISDIR)) {
				enviarDatosAlgoritmo();
				log_info(logFile, "Cambio de Algoritmo de planificacion");
			}
		}
		offset += sizeof(struct inotify_event) + event->len;
	}
}

//-----------------------------------------------------------------------------------------------------------------------------

void inicializarLog() {

	remove(LOG_PATH);
	logFile = log_create(LOG_PATH, "ProcesoNivel", false,
			log_level_from_string(configObj->logLevel));
	separador_log(configObj->nombre);
}

//-----------------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------

void enemigo(int idEnemigo) {
	t_list* bufferMovimiento = list_create();
	coordenada_t* posicion = malloc(sizeof(coordenada_t));
	agregarEnemigo(idEnemigo, posicion);
	int usegundos = (configObj->sleepEnemigos * 1000);
	int sec = div(usegundos, 1000000).quot; // para sleep
	int usec = div(usegundos, 1000000).rem; // para usleep
	log_trace(logFile, "Retardo enemigo segundos: %d, micro: %d", sec, usec);

	while (1) {

		sleep(sec);
		usleep(usec);

		cazarPersonajes(bufferMovimiento, posicion, idEnemigo);
		moverEnemigo(listaEnemigos, idEnemigo, posicion->ejeX, posicion->ejeY);

		dibujar();

		log_trace(logFile, "Enemigo: %d,se movio a posicion (%d, %d) ",
				idEnemigo, posicion->ejeX, posicion->ejeY);
	}

}

//-----------------------------------------------------------------------------------------------------------------------------

void agregarEnemigo(int idEnemigo, coordenada_t* posicion) {
	do {
		coordenadaRandomEjes(posicion, col, fil);
	} while (!validarPosicionEnemigo(posicion)
			&& !coordenadasIgualesInt(posicion, 0, 0));

	CrearEnemigo(listaEnemigos, '*', posicion->ejeX, posicion->ejeY, idEnemigo);
	log_info(logFile, "Enemigo: %d, creado en posicion (%d, %d) ", idEnemigo,
			posicion->ejeX, posicion->ejeY);
}

//-----------------------------------------------------------------------------------------------------------------------------

void cazarPersonajes(t_list* bufferMovimiento, coordenada_t* posicion,
		int idEnemigo) {

	if (hayPersonajes()) {
		list_clean(bufferMovimiento);
		perseguirPersonaje(posicion, idEnemigo);
	} else {

		movimientoDeEspera(bufferMovimiento, posicion);
	}

}

//-----------------------------------------------------------------------------------------------------------------------------

int hayPersonajes() {
	int ret = !list_is_empty(listaPersonajes);
	return ret;
}

//-----------------------------------------------------------------------------------------------------------------------------

void perseguirPersonaje(coordenada_t* posicion, int idEnemigo) {
	ITEM_NIVEL * temp = NULL;
	int i;
	int distanciaMinima = 1000;
	int xActual, yActual;
	coordenada_t* cObjetivo = malloc(sizeof(coordenada_t));
	coordenada_t* obstaculo = malloc(sizeof(coordenada_t));
	coordenada_t* limites = malloc(sizeof(coordenada_t));
	modificarCoordenada(limites, col, fil);

	//Busco el personaje mas cercano
	for (i = 0; i < list_size(listaPersonajes); i++) {
		temp = list_get(listaPersonajes, i);
		coordenada_t* c = malloc(sizeof(coordenada_t));
		modificarCoordenada(c, temp->posx, temp->posy);
		int distancia = obtenerDistancia(c, posicion);
		if (distancia < distanciaMinima) {
			distanciaMinima = distancia;
			modificarCoordenada(cObjetivo, c->ejeX, c->ejeY);
		}
		free(c);

	}

	xActual = posicion->ejeX;
	yActual = posicion->ejeY;

	//Me muevo hacia el personaje mas cercano
	coordenadaMovimientoAlternado(posicion, cObjetivo);
	if (!validarPosicionEnemigo(posicion)) {

		modificarCoordenada(obstaculo, posicion->ejeX, posicion->ejeY);
		modificarCoordenada(posicion, xActual, yActual);
		coordenadaEvasion(obstaculo, cObjetivo, posicion, limites);
	}

	//Si toco un personaje lo mato
	temp = NULL;
	for (i = 0; i < list_size(listaPersonajes); i++) {
		temp = list_get(listaPersonajes, i);
		if (coordenadasIgualesInt(posicion, temp->posx, temp->posy)) {
			notificarMuertePersonaje(temp->id, VICTIMA_ENEMIGO);
			log_info(logFile, "Enemigo %d mato a %c.", idEnemigo, temp->id);
		}
	}

	//Frees
	free(limites);
	free(obstaculo);
	free(cObjetivo);
}

//-----------------------------------------------------------------------------------------------------------------------------

void movimientoDeEspera(t_list* bufferMovimiento, coordenada_t* posicion) {

	if (list_is_empty(bufferMovimiento)) {
		do {
			movimientoLRandom(posicion, bufferMovimiento);
		} while (!validarPosicionesEnemigo(bufferMovimiento));
	}

	coordenada_t* temp = list_remove(bufferMovimiento, 0);
	modificarCoordenada(posicion, temp->ejeX, temp->ejeY);
	free(temp);
}

//-----------------------------------------------------------------------------------------------------------------------------

int validarPosicionesEnemigo(t_list* bufferMovimiento) {
	int i, r;
	for (i = 0; i < list_size(bufferMovimiento); i++) {
		coordenada_t* coordenadaTemp; //= malloc(sizeof(coordenada_t));
		coordenadaTemp = list_get(bufferMovimiento, i);
		log_trace(logFile, "Validando buffer %d, posiicion (%d, %d)", i,
				coordenadaTemp->ejeX, coordenadaTemp->ejeY);
		r = validarPosicionEnemigo(coordenadaTemp);
//		free (coordenadaTemp);
		if (!r) {
			list_clean_and_destroy_elements(bufferMovimiento, (void*) free);
			return 0;
		}

	}

	return 1;
}

//-----------------------------------------------------------------------------------------------------------------------------

int validarPosicionEnemigo(coordenada_t* posicion) {
	int i;
	ITEM_NIVEL * temp = NULL;
//	coordenada_t* posCaja;
	for (i = 0; i < list_size(listaRecursos); i++) {
		temp = list_get(listaRecursos, i);
//		posCaja = obtenerCoordenadas(listaRecursos, temp->id);
		if (coordenadasIgualesInt(posicion, temp->posx, temp->posy)) {
			log_trace(logFile,
					"Posicion invalida (%d,%d), coincide con una caja",
					temp->posx, temp->posy);
			return 0;
		}
		if (posicion->ejeX < 0 || posicion->ejeX > col) {
			log_trace(logFile, "Posicion invalida (%d,%d), supera los limites",
					posicion->ejeX, posicion->ejeY);
			return 0;
		}
		if (posicion->ejeY < 0 || posicion->ejeY > fil) {
			log_trace(logFile, "Posicion invalida (%d,%d), supera los limites",
					posicion->ejeX, posicion->ejeY);
			return 0;
		}

//		free(posCaja);
//		temp = temp->next;
	}
	return 1;

}

//-----------------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------

void deadLock() {

//	int usegundos = (configObj->deadlockTime * 1000000);
//	int wait = div(usegundos, 1000000000).quot;// para sleep

	int usegundos = (configObj->deadlockTime * 1000);
	int wait = div(usegundos, 1000000).quot; // para sleep
	int uwait = div(usegundos, 1000000).rem; // para usleep
	log_info(logFile,
			"DeadLock se ejecutara cada: %d segundos, %d milisegundos", wait,
			uwait);

	while (1) {
		sleep(wait);
		usleep(uwait);
		log_info(logFile, "Ejecutando DeadLock");
		gestionarDeadLock();

	}

}

//-----------------------------------------------------------------------------------------------------------------------------

void gestionarDeadLock() {
	int i;
	t_list* bloqueados = obtenerPersonajesEnDL(listaRecursos, listaPersonajes);
	ITEM_NIVEL * temp = NULL;
	ITEM_NIVEL * menor = NULL;

	if (list_is_empty(bloqueados) || bloqueados->elements_count == 1) {
		list_destroy_and_destroy_elements(bloqueados, (void *) free);
		return;
	}

	logBloqueados(bloqueados);

	if (!configObj->recovery) {
		return;
	}

	menor = list_get(bloqueados, 0);
	for (i = 0; i < list_size(bloqueados); i++) {
		temp = list_get(bloqueados, i);
		if (temp->socket < menor->socket) {
			menor = temp;
		}
	}

	log_info(logFile, "Victima DeadLock: %c", menor->id);
	notificarMuertePersonaje(menor->id, VICTIMA_DEADLOCK);
	list_destroy_and_destroy_elements(bloqueados, (void *) free);
}

//-----------------------------------------------------------------------------------------------------------------------------

void logBloqueados(t_list* bloqueados) {

	int i;
	char *s = string_new();

	ITEM_NIVEL * temp = NULL;
	log_info(logFile, "Se detecto interbloque entre %d personajes",
			bloqueados->elements_count);
	for (i = 0; i < bloqueados->elements_count; i++) {
		temp = list_get(bloqueados, i);
		string_append(&s, "-");
		string_append(&s, string_repeat(temp->id, 1));

	}

	log_info(logFile, "Personajes involucharods: %s", s);
	free(s);
}

