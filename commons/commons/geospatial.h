/*
 * geospatial.h
 *
 *  Created on: 30/09/2013
 *      Author: utnso
 */

#ifndef GEOSPATIAL_H_
#define GEOSPATIAL_H_

#include "sockets.h"

int obtenerDistancia (coordenada_t* inicio, coordenada_t* fin);
coordenada_t* coordenadaMovimientoAlternado(coordenada_t* posicionActual, coordenada_t* poscionObjetivio);
int obtenerDistanciaEnX(coordenada_t* inicio, coordenada_t* fin) ;
int obtenerDistanciaEnY(coordenada_t* inicio, coordenada_t* fin);
coordenada_t* desplazarEnY( coordenada_t* posicionActual,coordenada_t* posicionObjetivio);
coordenada_t* desplazarEnX( coordenada_t* posicionActual, coordenada_t* posicionObjetivio);
indicacion_t* indicacionMovimientoAlternado(coordenada_t* posicionActual, coordenada_t* posicionObjetivio);
int coordenadasIguales(coordenada_t* c1, coordenada_t* c2);
int randomNumber(int min_num, int max_num);
coordenada_t* desplazarEnXPositivo(coordenada_t* coordenada);
coordenada_t* desplazarEnXNegativo(coordenada_t* coordenada);
coordenada_t* desplazarEnYPositivo(coordenada_t* coordenada);
coordenada_t* desplazarEnYNegativo(coordenada_t* coordenada);
t_list* movimientoL(coordenada_t* cActual);

#endif /* GEOSPATIAL_H_ */
