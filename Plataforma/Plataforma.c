/*
 * Plataforma.c
 *
 *  Created on: 24/09/2013
 *      Author: utnso
 */

#include "Plataforma.h"
#include "Orquestador.h"
#include <stdlib.h>
#include <pthread.h>

int main(void) {
//	pthread_t hOrquestador;
//
//	pthread_create(&hOrquestador, NULL, (void *) orquestador, NULL );
//	pthread_join(hOrquestador, (void **) NULL );
	orquestador();
	return EXIT_SUCCESS;
}
