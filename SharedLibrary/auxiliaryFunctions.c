/*
 * auxiliaryFunctions.c
 *
 *  Created on: 26 abr. 2019
 *      Author: utnso
 */

#include "auxiliaryFunctions.h"

double getCurrentTime()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
}

