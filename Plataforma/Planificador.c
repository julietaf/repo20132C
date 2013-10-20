/*
 * Planificador.c
 *
 *  Created on: 07/10/2013
 *      Author: utnso
 */

#include "Planificador.h"

void atenderPedidoPlanificador(fd_set *bagEscucha, int sockfdMax,
		datos_planificador_t *datos);
void seleccionarPersonaje(datos_planificador_t *datos);

void planificador(datos_planificador_t *datos) {
	fd_set bagMaster, bagEscucha;
	FD_ZERO(&bagMaster);
	FD_ZERO(&bagEscucha);
	FD_SET(datos->sockfdNivel, &bagMaster);
	int retval, sockfdMax = datos->sockfdNivel;

	while (1) {
		bagEscucha = bagMaster;

		retval = select(sockfdMax + 1, &bagEscucha, NULL, NULL, NULL );

		if (retval == -1) //Ocurrio un error
			log_error(logFile, "Error en select");
		else if (retval) //Hay un pedido de los procesos conectados
			atenderPedidoPlanificador(&bagEscucha, sockfdMax, datos);
		else if (!queue_is_empty(datos->personajesListos)) //Se produjo el timeout
			seleccionarPersonaje(datos);
	}
}

void atenderPedidoPlanificador(fd_set *bagEscucha, int sockfdMax,
		datos_planificador_t *datos) {
	int sockfd;

	for (sockfd = 0; sockfd <= sockfdMax; sockfd++) {
		if (sockfd == datos->sockfdNivel) {

		}
	}
}

void seleccionarPersonaje(datos_planificador_t *datos) {
	switch (datos->criterioPlanificacion) {
	case ROUND_ROBIN:
		break;
	case SRDF:
		break;
	}
}
