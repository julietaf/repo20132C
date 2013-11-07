/*
 * Nivel.c
 *
 *  Created on: 24/09/2013
 *      Author: utnso
 */

#include "Nivel.h"

int main(void) {

	listaRecursos = list_create();
	listaPersonajes = list_create();
	listaEnemigos = list_create();

	configObj = inicializarCongiuracionNivel();
	logFile = log_create(LOG_PATH, "ProcesoNivel", false,
			log_level_from_string(configObj->logLevel));
	separador_log(configObj->nombre);
	inicializarInterfazGrafica();
	inicializarConfiguracionCajas();
	nivel_gui_dibujar(listaRecursos, configObj->nombre);
	inicializarSockEscucha();
	inicializarConexionPlataforma();
	log_info(logFile, "Esperando conexiones en ip: %s port: %s",
			configObj->localhostaddr, configObj->localhostport);
//	crearHiloEnemigo();
	pthread_t hEnemigo;
	pthread_create(&hEnemigo, NULL, (void*) enemigo, NULL );
	while (1){
		atenderMensajePlanificador(plataformaSockfd);
	}
	//TODO: MUchas muy importantes cosa//
	pthread_join(hEnemigo, (void **) NULL );
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

void inicializarSockEscucha() {
	sockEscucha = sockets_createServer(configObj->localhostaddr,
			configObj->localhostport, 1);
	if (sockEscucha == -1) {
		log_error(logFile, "No se pudo crear el socket escucha");
	} else {
		log_info(logFile, "Nivel escucnando en ip: %s, puerto: %s, fd:%d",
				configObj->localhostaddr, configObj->localhostport,
				sockEscucha);
	}
}

//-----------------------------------------------------------------------------------------------------------------------------

void inicializarConexionPlataforma() {

	plataformaSockfd = conectarOrquestador();
	hacerHandshake(plataformaSockfd);
	enviarDatosAlgoritmo();
//	recibirDatosPlanificador();

}

//-----------------------------------------------------------------------------------------------------------------------------

int conectarOrquestador() {

	int orquestadorSockfd = sockets_createClient(configObj->orquestadoraddr,
			configObj->orquestadorport);

	if (orquestadorSockfd == -1) {
		log_error(logFile, "Conexion con orquestador fallo");
		exit(EXIT_FAILURE);
	} else {
		log_info(logFile, "Conectado con orquestador, ip: %s, port: %s",
				configObj->orquestadoraddr, configObj->orquestadorport);
	}

	return orquestadorSockfd;
}

//-----------------------------------------------------------------------------------------------------------------------------

void hacerHandshake(int sockfdReceptor) {
	//Envio mi hanshake
	header_t header;
	header.type = HANDSHAKE_NIVEL;
	header.length = 0;
	sockets_send(sockfdReceptor, &header, '\0');
	//Espero hanshake de vuelta
	if (recv(sockfdReceptor, &header, sizeof(header), MSG_WAITALL) == 0) {
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
}

//-----------------------------------------------------------------------------------------------------------------------------

void enviarDatosAlgoritmo() {
	header_t h;
	int16_t lenght;
	informacion_planificacion_t* datosAlgoritmo = malloc(
			sizeof(informacion_planificacion_t));
	char* data;

	obtenerDatosAlgorimo(datosAlgoritmo);
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

void atenderMensajePlanificador(int sockfd) {
	header_t h;
	if (validarRecive(sockfd, &h)) {
		char* data = malloc(h.length);
		switch (h.type) {
		case NOTIFICAR_DATOS_PERSONAJE:
			recv(sockfd, data, h.length, MSG_WAITALL);
			tratarNuevoPersonaje(data);
			break;
		case UBICACION_CAJA:
			recv(sockfd, data, h.length, MSG_WAITALL);
			tratarSolicitudUbicacionCaja(data);
			break;
		case NOTIFICACION_MOVIMIENTO:
			recv(sockfd, data, h.length, MSG_WAITALL);
			tratarMovimiento(data);
			break;
		case SOLICITAR_RECURSO:
			recv(sockfd, data, h.length, MSG_WAITALL);
			tratarSolicitudRecurso(data);
			break;
		case PERSONAJE_FINALIZO:
			recv(sockfd, data, h.length, MSG_WAITALL);
			tratarFinalizacionPersonaje(data);
			break;
		default:
			log_error(logFile,
					"Protocolo invalido (%d) para comunicarse con el nivel",
					h.type);
			break;
		}
		free(data);
	}
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

}

//-----------------------------------------------------------------------------------------------------------------------------

void tratarNuevoPersonaje(char* data) {
	char pId;

	pId = data[0];
	log_info(logFile, "Creando Personaje...");
	CrearPersonaje(listaPersonajes, pId, 0, 0, 1, orden);
	orden++;
	dibujar();
}

//-----------------------------------------------------------------------------------------------------------------------------

void tratarSolicitudUbicacionCaja(char* data) {
	char rId = data[0];
	coordenada_t* coord;
	header_t h;
	int16_t length;
	char* coordSerialized;

	log_info(logFile, "Atendiendo pedido de posicion de caja %c", rId);
	coord = obtenerCoordenadas(listaRecursos, rId);
	coordSerialized = coordenadas_serializer(coord, &length);

	h.type = UBICACION_CAJA;
	h.length = length;

	sockets_send(plataformaSockfd, &h, coordSerialized);

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

	MoverPersonaje(listaPersonajes, pId, pCoordenada->ejeX, pCoordenada->ejeY);

	free(pCoordenada);
}

//-----------------------------------------------------------------------------------------------------------------------------

void tratarSolicitudRecurso(char* data) {

	personaje_recurso_t* personaje;
	int respuesta;

	personaje = personajeRecurso_deserializer(data);

	if (personajeEnCaja(personaje->idPersonaje, personaje->idRecurso)) {
		respuesta = darRecursoPersonaje(listaPersonajes, listaRecursos,
				personaje->idPersonaje, personaje->idRecurso);
	} else {
		log_error(logFile, "acceso incorrecto a caja de recurso: %s",
				personaje->idRecurso);
	}
	if (respuesta) {
		log_info(logFile, "Se le asigno el recurso: %c, al personaje: %c",
				personaje->idRecurso, personaje->idPersonaje);
		notificacionDarRecurso(personaje->idPersonaje);
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
	t_list* asignados = NULL;

	pId = data[0];
	log_info(logFile, "Matando personaje..");
	adquiridos = getObjetosAdquiridosSerializable(listaPersonajes, pId);
	matarPersonaje(listaPersonajes, listaRecursos, pId);
	mandarRecursosLiberados(adquiridos);
	asignados = esperarRecursosAsignados();
	actualizarEstado(asignados);
	listaRecursos_destroy(adquiridos);
	listaRecursos_destroy(asignados);
	log_info(logFile, "El personaje: %c, finalizo el nivel", pId);
	dibujar();
}

//-----------------------------------------------------------------------------------------------------------------------------

void notificarMuertePersonaje(char id, int causa) {
	header_t h;
	t_list* adquiridos = NULL;
	t_list* asignados = NULL;
	int16_t length;

	h.type = causa;
	h.length = sizeof(char) + 1;

	adquiridos = getObjetosAdquiridosSerializable(listaPersonajes, id);
	char* adqData = listaRecursos_serializer(adquiridos, &length);

	char* data = malloc(h.length + length);
	memcpy(data, &id, h.length);
	memcpy(data + h.length, adqData, length);
	h.length += length;
	sockets_send(plataformaSockfd, &h, data);
	asignados = esperarRecursosAsignados();
	actualizarEstado(asignados);

	listaRecursos_destroy(adquiridos);
	listaRecursos_destroy(asignados);
	free(data);
}

//-----------------------------------------------------------------------------------------------------------------------------

void notificacionDarRecurso(char id) {
	header_t header;
	header.type = OTORGAR_RECURSO;
	header.length = 0;
	sockets_send(plataformaSockfd, &header, "");
}

//-----------------------------------------------------------------------------------------------------------------------------

void notificacionBloqueo(char id) {
	header_t header;
	header.type = NEGAR_RECURSO;
	header.length = 0;
	sockets_send(plataformaSockfd, &header, "");
}

//-----------------------------------------------------------------------------------------------------------------------------

void dibujar() {
	t_list* tempI;

	tempI = list_create();

	list_add_all(tempI, listaRecursos);
	list_add_all(tempI, listaPersonajes);
	list_add_all(tempI, listaEnemigos);

	if (nivel_gui_dibujar(tempI, configObj->nombre) == -1) {
		log_info(logFile, "No se puedo dibujar el personaje");
		//      EXIT_FAILURE;
	} else {
		log_info(logFile, "Dibujado...");
	}
//        destroyItems(&tempI);
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

int personajeEnCaja(char pId, char rId) {

	int enCaja = 0;
	coordenada_t* posCaja = obtenerCoordenadas(listaRecursos, rId);
	coordenada_t* posPersonaje = obtenerCoordenadas(listaPersonajes, pId);

	enCaja = coordenadasIguales(posCaja, posPersonaje);

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

t_list* esperarRecursosAsignados() {
	log_info(logFile, "Reciviendo recursos asignados");
	header_t header;
	t_list* asignados = NULL;
	char* data;
	if (recv(plataformaSockfd, &header, sizeof(header), MSG_WAITALL) == 0) {
		log_error(logFile, "Conexion perdida con el orqestador");
	}

	if (header.type == NOTIFICACION_RECURSOS_ASIGNADOS) {
		data = malloc(header.length);
		if (header.length > 0) {
			recv(plataformaSockfd, data, header.length, MSG_WAITALL);
			asignados = listaRecursos_deserializer(data, header.length);
		} else if (header.length == 0) {
			log_debug(logFile, "El orquestador no uso ningun recurso");
			return asignados = list_create();
		}

	} else {
		log_error(logFile, "Mensaje inesperado del Orquestador");
	}
	free(data);
	return asignados;
}

//-----------------------------------------------------------------------------------------------------------------------------

void actualizarEstado(t_list* asignados) {

	log_debug(logFile, "Actualizando estado de recurso y personajes...");
	personaje_recurso_t* personaje;
	int i;
	for (i = 0; i < asignados->elements_count; ++i) {
		personaje = list_get(asignados, i);
		darRecursoPersonaje(listaPersonajes, listaRecursos,
				personaje->idPersonaje, personaje->idRecurso);
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

	int watch_descriptor = inotify_add_watch(file_descriptor, CONF_PATH,
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
//-----------------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------

void enemigo(int idEnemigo) {
//	t_list* bufferMovimiento = NULL;
	t_list* bufferMovimiento = list_create();
	coordenada_t* posicion = malloc(sizeof(coordenada_t));
	agregarEnemigo(idEnemigo, posicion);
	while (1) {
		cazarPersonajes(bufferMovimiento, posicion);
		moverEnemigo(listaEnemigos, idEnemigo, posicion->ejeX, posicion->ejeY);
		dibujar();
		log_info(logFile, "Enemigo: %d,se movio a posicion (%d, %d) ",
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

void cazarPersonajes(t_list* bufferMovimiento, coordenada_t* posicion) {
//	sleep(configObj->sleepEnemigos);
	sleep(1);
	if (hayPersonajes()) {
		perseguirPersonaje(posicion);
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

void perseguirPersonaje(coordenada_t* posicion) {
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
			//TODO: aloca memoria
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
		r = validarPosicionEnemigo(coordenadaTemp);
//		free (coordenadaTemp);
		if (!r) {
			return 0;
		}
		if (coordenadaTemp->ejeX < 0 && coordenadaTemp->ejeX > col) {
			log_warning(logFile,
					"Posicion invalida (%d,%d), supera los limites",
					coordenadaTemp->ejeX, coordenadaTemp->ejeY);
			return 0;
		}
		if (coordenadaTemp->ejeY < 0 && coordenadaTemp->ejeY > fil) {
			log_warning(logFile,
					"Posicion invalida (%d,%d), supera los limites",
					coordenadaTemp->ejeX, coordenadaTemp->ejeY);
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
			log_warning(logFile,
					"Posicion invalida (%d,%d), coincide con una caja",
					temp->posx, temp->posy);
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
	while (1){
		int wait = configObj->deadlockTime;
		sleep(wait);
		gestionarDeadLock();
		
	}
	
	
}

//-----------------------------------------------------------------------------------------------------------------------------

void gestionarDeadLock() {
	int = i;
	t_list* bloqueados = obtenerPersonajesEnDL(listaRecursos, listaPersonajes);
	ITEM_NIVEL * temp = NULL;
	ITEM_NIVEL * menor = NULL;
	
	if (list_is_empty(bloqueados)){
		return;
	}
	
	menor = list_get(bloqueados, 0)
	for (i = 0; i < list_size(bloqueados); i++){
		temp = list_get(bloqueados, i)
		if (temp->socket < menor->socket){
			menor = temp;
		}
	}
	
	notificarMuertePersonaje(menor->tempId, VICTIMA_DEADLOCK)
	
	

}

