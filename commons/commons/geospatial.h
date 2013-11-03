/*
 * geospatial.h
 *
 *  Created on: 30/09/2013
 *      Author: utnso
 */

#ifndef GEOSPATIAL_H_
#define GEOSPATIAL_H_

#include "sockets.h"
//#include "collections/list.c" TODO: Preguntar por herencia????
#include <stdlib.h>
#include <stdio.h>
#include <time.h>


int obtenerDistancia (coordenada_t* inicio, coordenada_t* fin);
void coordenadaMovimientoAlternado(coordenada_t* posicionActual, coordenada_t* poscionObjetivio);
int obtenerDistanciaEnX(coordenada_t* inicio, coordenada_t* fin) ;
int obtenerDistanciaEnY(coordenada_t* inicio, coordenada_t* fin);
void desplazarEnY( coordenada_t* posicionActual,coordenada_t* posicionObjetivio);
void desplazarEnX( coordenada_t* posicionActual, coordenada_t* posicionObjetivio);
indicacion_t* indicacionMovimientoAlternado(coordenada_t* posicionActual, coordenada_t* posicionObjetivio);
int coordenadasIguales(coordenada_t* c1, coordenada_t* c2);
int coordenadasIgualesInt(coordenada_t* c1, int x, int y);
coordenada_t* coordenadaRandom(coordenada_t* cMaxima);
void coordenadaRandomEjes(coordenada_t* ret, int xMax, int yMax);
int randomNumber(int min_num, int max_num);
void desplazarEnXPositivo(coordenada_t* coordenada);
void desplazarEnXNegativo(coordenada_t* coordenada);
void desplazarEnYPositivo(coordenada_t* coordenada);
void desplazarEnYNegativo(coordenada_t* coordenada);
void movimientoLRandom(coordenada_t* cActual, t_list* buffer);
void movimientoL(coordenada_t* cActual,t_list* buffer, int orden, int direccion, int sentido);
void modificarCoordenada(coordenada_t* coordenada, int x, int y);
void coordenadaEvasion(coordenada_t* obstaculo, coordenada_t* objetivo, coordenada_t* posActual, coordenada_t* cMaxima);
int esPar(int a);

#endif /* GEOSPATIAL_H_ */
