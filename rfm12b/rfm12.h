/**** RFM 12 library for Atmel AVR Microcontrollers *******
 *
 * This software is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * @author Peter Fuhrmann, Hans-Gert Dahmen, Soeren Heisrath
 */

/** \file rfm12.h
 * \brief rfm12 library main header
 * \author Hans-Gert Dahmen
 * \author Peter Fuhrmann
 * \author Soeren Heisrath
 * \version 0.9.0
 * \date 08.09.09
 *
 * This header represents the library's core API.
 */

/******************************************************
 *                                                    *
 *    NO  C O N F I G U R A T I O N  IN THIS FILE     *
 *                                                    *
 *      ( thou shalt not change lines below )         *
 *                                                    *
 ******************************************************/
 
#ifndef _RFM12_H
#define _RFM12_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//this was missing, but is very important to set the config options for structs and such
#include "include/rfm12_core.h"
#include <ctype.h>
#include <stdio.h>

/************************
 * function protoypes
*/

/************************
 * include headers for all the optional stuff in here
 * this way the user only needs to include rfm12.h
*/
#include "include/rfm12_extra.h"
#include "include/rfm12_livectrl.h"

#ifdef __cplusplus
}
#endif

#endif /* _RFM12_H */
