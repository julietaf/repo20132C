/*
 * Nivel.c
 *
 *  Created on: 24/09/2013
 *      Author: utnso
 */

#include "Nivel.h"

int main(void) {

	configObj = inicializarCongiuracionNivel();
	logFile = log_create(LOG_PATH, "ProcesoNivel", false,
			log_level_from_string(configObj->logLevel));
	separador_log(configObj->nombre);
//	inicializarInterfazGrafica();
	inicializarConfiguracionCajas();
//	nivel_gui_dibujar(listaRecursos);
	inicializarSockEscucha();
//	inicializarConexionPlataforma();
	log_info(logFile, "Esperando conexiones en ip: %s port: %s", configObj->localhostaddr, configObj->localhostport);
//	crearHiloEnemigo();
	pthread_t hEnemigo;
	pthread_create(&hEnemigo, NULL, (void*) enemigo, NULL );
	//TODO: MUchas muy importantes cosa//
	pthread_join(hEnemigo, (void **)NULL);
	return EXIT_SUCCESS;
}

//-----------------------------------------------------------------------------------------------------------------------------

NIVEL_CONF* inicializarCongiuracionNivel() {
	t_config* conF = config_create(CONFIG_PATH);
	NIVEL_CONF *configObj = malloc(sizeof(NIVEL_CONF));

	if (config_has_property(conF, "Nombre")) {
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
	log_info(logFile, "Interfaz grafica inicializada, filas: %d, columnas: %d", fil, col);

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
		CrearCaja(&listaRecursos, id, pos_x, pos_y, instancia);
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
	recibirDatosPlanificador();

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
	informacion_planificacion_t* datosAlgoritmo =malloc(sizeof(informacion_planificacion_t));
	char* data;

	obtenerDatosAlgorimo(datosAlgoritmo);
	data = informacionPlanificacion_serializer(datosAlgoritmo, &lenght);

	h.type = NOTIFICAR_ALGORITMO_PLANIFICACION;
	h.length  = lenght;

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

void obtenerDatosAlgorimo(informacion_planificacion_t* datosAlgoritmo){
	t_config* conF = config_create(CONFIG_PATH);

	if(config_has_property(conF, "Nombre")){
		datosAlgoritmo->nombreNivel = malloc(strlen(config_get_string_value(conF, "Nombre")) + 1);
		strcpy(datosAlgoritmo->nombreNivel, config_get_string_value(conF, "Nombre"));
	}

	if(config_has_property(conF, "algoritmo")){
		char* algoritmo = config_get_string_value(conF, "algoritmo");
		datosAlgoritmo->algoritmo = strcmp("RR", algoritmo) == 0 ? ROUND_ROBIN : SRDF;
	}

	if(config_has_property(conF, "quantum")){
		datosAlgoritmo->quantum = config_get_int_value(conF, "quantum");
	}

	if(config_has_property(conF, "retardo")){
		datosAlgoritmo->retardo = config_get_int_value(conF, "retardo");
	}

}

//-----------------------------------------------------------------------------------------------------------------------------

void tratarNuevoPersonaje(char* data){
	char pId;

	pId = data[0];
	log_info(logFile, "Creando Personaje...");
	CrearPersonaje(&listaPersonajes, pId, 0, 0, 1, orden);
	orden++;
	dibujar();
}

//-----------------------------------------------------------------------------------------------------------------------------

void tratarSolicitudUbicacionCaja(char* data){
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

void tratarMovimiento(char* data){
	char pId;
	coordenada_t* pCoordenada;
	int offset = sizeof(char);

	pId = data[0];
	pCoordenada = coordenadas_deserializer(data+offset);

	MoverPersonaje(listaPersonajes, pId, pCoordenada->ejeX, pCoordenada->ejeY);

	free(pCoordenada);
}

//-----------------------------------------------------------------------------------------------------------------------------

void tratarSolicitudRecurso(char* data){
	//TODO:
	personaje_recurso_t* personaje;
	personaje = personajeRecurso_deserializer(data);
	int respuesta;
	if (personajeEnCaja(personaje->idPersonaje, personaje->idRecurso)) {
		respuesta = darRecursoPersonaje(&listaPersonajes, &listaRecursos, personaje->idPersonaje,
				personaje->idRecurso);
	} else {
		log_error(logFile, "acceso incorrecto a caja de recurso: %s", personaje->idRecurso);
	}
	if (respuesta) {
		log_info(logFile, "Se le asigno el recurso: %c, al personaje: %c", personaje->idRecurso,
				personaje->idPersonaje);
		notificacionDarRecurso(personaje->idPersonaje);
	} else {
		log_info(logFile, "Se bloqueo el personaje: %c, al pedir el recruso %c",
				personaje->idPersonaje, personaje->idRecurso);
		notificacionBloqueo(personaje->idPersonaje);
	}
}

//-----------------------------------------------------------------------------------------------------------------------------

void tratarFinalizacionPersonaje(char* data){
	//TODO:
}

//-----------------------------------------------------------------------------------------------------------------------------

void notificarMuertePersonaje(char id){
	//TODO:
}

//-----------------------------------------------------------------------------------------------------------------------------

void notificacionDarRecurso(char id){
	//TODO:
}

//-----------------------------------------------------------------------------------------------------------------------------

void notificacionBloqueo(char id){
	//TODO:
}

//-----------------------------------------------------------------------------------------------------------------------------

void dibujar() {
        log_info(logFile, "Dibujando...");
        ITEM_NIVEL* tempI = NULL;
        ITEM_NIVEL* tempR = NULL;
        ITEM_NIVEL* tempP = NULL;
        ITEM_NIVEL* tempE = NULL;

        tempR = listaRecursos;
        tempP = listaPersonajes;
        tempE = listaEnemigos;

        while (tempR != NULL ) {
                CrearCaja(&tempI, tempR->id, tempR->posx, tempR->posy, tempR->quantity);
                tempR = tempR->next;
        }

        while (tempP != NULL ) {
                CrearPersonaje(&tempI, tempP->id, tempP->posx, tempP->posy, 1,
                                tempP->socket);
                tempP = tempP->next;
        }

        while (tempE != NULL ) {
        	CrearEnemigo(&tempI, tempE->id, tempE->posx, tempE->posy, tempE->idEnemigo);
        	log_debug(logFile, "Enemigo: %d, dibujado en posicion (%d, %d) ", tempE->idEnemigo, tempE->posx, tempE->posy);
        	tempE = tempE->next;

        }

        if (nivel_gui_dibujar(tempI) == -1) {
                log_info(logFile, "No se puedo dibujar el personaje");
                //      EXIT_FAILURE;
        } else {
                log_info(logFile, "Dibujado...");
        }
        //TODO: Destroy
}

//-----------------------------------------------------------------------------------------------------------------------------

void crearHiloEnemigo (){

	int i = 0;
	for (i = 0; i < configObj->enemigos; i++) {
		pthread_t hEnemigo;
		pthread_create(&hEnemigo, NULL, (void*) enemigo,  (void*)i );
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
//-----------------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------

void enemigo (int idEnemigo){
//	t_list* bufferMovimiento = NULL;
	t_list* bufferMovimiento = list_create();
	coordenada_t* posicion = malloc(sizeof(coordenada_t));
	agregarEnemigo(idEnemigo, posicion);
	while (1){
		cazarPersonajes(bufferMovimiento, posicion);
		moverEnemigo(listaEnemigos, idEnemigo, posicion->ejeX, posicion->ejeY );
//		dibujar();
		log_info(logFile, "Enemigo: %d,se movio a posicion (%d, %d) ", idEnemigo, posicion->ejeX, posicion->ejeY);
	}

}

//-----------------------------------------------------------------------------------------------------------------------------

void agregarEnemigo(int idEnemigo, coordenada_t* posicion){
	do {
		coordenadaRandomEjes(posicion, col, fil);
	} while (!validarPosicionEnemigo(posicion) && !coordenadasIgualesInt(posicion, 0, 0));

	CrearEnemigo(&listaEnemigos, '*', posicion->ejeX, posicion->ejeY, idEnemigo);
	log_info(logFile, "Enemigo: %d, creado en posicion (%d, %d) ", idEnemigo, posicion->ejeX, posicion->ejeY);
}

//-----------------------------------------------------------------------------------------------------------------------------

void cazarPersonajes( t_list* bufferMovimiento, coordenada_t* posicion){
//	sleep(configObj->sleepEnemigos);
	sleep(1);
	if(hayPersonajes()){
		perseguirPersonaje(posicion);
	}else{

		movimientoDeEspera(bufferMovimiento, posicion);
	}

}

//-----------------------------------------------------------------------------------------------------------------------------

int hayPersonajes(){
	int ret = listaPersonajes != NULL ? 1 : 0;
	return ret;
}

//-----------------------------------------------------------------------------------------------------------------------------

void perseguirPersonaje(coordenada_t* posicion){
	ITEM_NIVEL * temp = listaPersonajes;
	int distanciaMinima = 1000;
	int xActual , yActual;
	coordenada_t* cObjetivo = malloc(sizeof(coordenada_t));
	coordenada_t* obstaculo = malloc(sizeof(coordenada_t));
	coordenada_t* limites = malloc(sizeof(coordenada_t));
	modificarCoordenada(limites, col, fil);

	//Busco el personaje mas cercano
	while(temp != NULL){
		coordenada_t* c = malloc(sizeof(coordenada_t));
		modificarCoordenada(c, temp->posx, temp->posy );
		int distancia = obtenerDistancia(c, posicion);
		if (distancia < distanciaMinima){
			distanciaMinima = distancia;
			modificarCoordenada(cObjetivo, c->ejeX, c->ejeY);
		}
		free(c);
		temp = temp->next;
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
	temp = listaPersonajes;
	while (temp != NULL){
		if (coordenadasIgualesInt(posicion, temp->posx, temp->posy)) {
			notificarMuertePersonaje(temp->id);
		}
	}


	//Frees
	free(limites);
	free(obstaculo);
	free(cObjetivo);
}

//-----------------------------------------------------------------------------------------------------------------------------

void movimientoDeEspera(t_list* bufferMovimiento, coordenada_t* posicion){

	if (list_is_empty(bufferMovimiento)){
		do{
			//TODO: aloca memoria
		movimientoLRandom(posicion, bufferMovimiento);
		}while(!validarPosicionesEnemigo(bufferMovimiento));
	}

	coordenada_t* temp = list_remove(bufferMovimiento, 0);
	modificarCoordenada(posicion, temp->ejeX, temp->ejeY);
	free(temp);
}

//-----------------------------------------------------------------------------------------------------------------------------

int validarPosicionesEnemigo(t_list* bufferMovimiento){
	int i, r;
	for (i = 0; i < list_size(bufferMovimiento); i++) {
		coordenada_t* coordenadaTemp ;//= malloc(sizeof(coordenada_t));
		coordenadaTemp = list_get(bufferMovimiento, i);
		r = validarPosicionEnemigo(coordenadaTemp);
//		free (coordenadaTemp);
		if (!r){
			return 0;
		}
	}

	return 1;
}

int validarPosicionEnemigo(coordenada_t* posicion){
	ITEM_NIVEL * temp = listaRecursos;
	coordenada_t* posCaja;
	while (temp != NULL ){
		posCaja = obtenerCoordenadas(listaRecursos, temp->id);
		if (coordenadasIguales(posCaja, posicion)) {
			return 0;
		}
		free(posCaja);
		temp = temp->next;
	}
	return 1;

}
//-----------------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------

void deadLock (){
	//TODO:
}

