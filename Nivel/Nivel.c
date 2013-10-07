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
	inicializarConexionPlataforma();
	log_info(logFile, "Esperando conexiones en ip: %s port: %s",
			configObj->localhostaddr, configObj->localhostport);

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
			log_warning(logFile, "No se pudo levantar la caja: %s", u);
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

	orquestadorSockfd = conectarOrquestador();
	hacerHandshake(orquestadorSockfd);
	enviarDatosConexion();
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

void enviarDatosConexion() {
	header_t h;
	ip_info_t i;
	i.addr = configObj->localhostaddr;
	i.port = configObj->localhostport;
	h.type = NOTIFICAR_DATOS_NIVEL;
	h.length = strlen(configObj->nombre) + 1;

	int16_t length;

	char* ipInfo = ipInfo_serializer(&i, &length);

	char* data = malloc(h.length + length);
	memcpy(data, configObj->nombre, h.length);
	memcpy(data + h.length, ipInfo, length);
	h.length += length;
	sockets_send(orquestadorSockfd, &h, data);
	free(data);
}

//-----------------------------------------------------------------------------------------------------------------------------

void recibirDatosPlanificador() {
	header_t header;
	char* data;
	if (recv(orquestadorSockfd, &header, sizeof(header), MSG_WAITALL) == 0) {
		log_error(logFile, "Conexion perdida con el planificador");
	}

	if (header.type == NOTIFICAR_DATOS_PLANIFICADOR) {
		data = malloc(header.length);
		recv(orquestadorSockfd, data, header.length, MSG_WAITALL);
		ip_info_t *infoPlanificador = ipInfo_deserializer(data);
		log_info(logFile, "Planificador IP: %s port: %s",
				infoPlanificador->addr, infoPlanificador->port);
	}
	free(data);
}

//-----------------------------------------------------------------------------------------------------------------------------
