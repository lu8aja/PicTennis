/* 
 * File:   sintable.h
 * Author: Javier
 *
 * Created on June 29, 2016, 9:49 PM
 */

#ifndef SINTABLE_H
#define	SINTABLE_H

#pragma udata
extern rom const float sintable[];

float simplesin(unsigned char a);
float simplecos(unsigned char a);

#endif	/* SINTABLE_H */

