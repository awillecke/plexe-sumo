/***************************************************************************
                          Bresenham.cpp
			  A class to realise a uniform n:m - relationship using the
              bresenham - algorithm
                             -------------------
    project              : SUMO
    begin                : Fri, 07 Jun 2002
    copyright            : (C) 2001 by DLR/IVF http://ivf.dlr.de/
    author               : Daniel Krajzewicz
    email                : Daniel.Krajzewicz@dlr.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
namespace
{
    const char rcsid[] =
    "$Id$";
}
// $Log$
// Revision 1.1  2002/10/16 15:01:11  dkrajzew
// initial commit for utility classes for geometry handling
//
// Revision 1.5  2002/07/25 08:47:16  dkrajzew
// cvs-id patched
//
// Revision 1.4  2002/06/21 13:16:21  dkrajzew
// some minor improvements
//
// Revision 1.3  2002/06/11 15:58:25  dkrajzew
// windows eol removed
//
// Revision 1.2  2002/06/07 14:58:45  dkrajzew
// Bugs on dead ends and junctions with too few outgoing roads fixed; Comments improved
//
//
/* =========================================================================
 * included modules
 * ======================================================================= */
#include <iostream>
#include "Bresenham.h"

using namespace std;

/* =========================================================================
 * method definitions
 * ======================================================================= */
void Bresenham::compute(BresenhamCallBack *callBack, double val1, double val2)
{
    // case1: both numbers are equal
    if(val1==val2) {
        for(double step=0; step<val1; step++) {
            callBack->execute(step, step);
        }
	return;
    }
    // case2: the first value is higher
    if(val1>val2) {
        double pos = 0;
        double prop = val2 / val1;
        double cnt = prop / 2;
        for(double i=0; i<val1; i++) {
            callBack->execute(i, pos);
            cnt += prop;
            if(cnt>=1.0) {
                pos++;
                cnt -= 1.0;
            }
        }
	return;
    }
    // case3: the first value is smaller than the second
    if(val1<val2) {
        double pos = 0;
        double prop = val1 / val2;
        double cnt = prop / 2;
        for(double i=0; i<val2; i++) {
            callBack->execute(pos, i);
            cnt += prop;
            if(cnt>=1.0) {
                pos++;
                cnt -= 1.0;
            }
        }
	return;
    }
}

/**************** DO NOT DEFINE ANYTHING AFTER THE INCLUDE *****************/

//#ifdef DISABLE_INLINE
//#include "Bresenham.icc"
//#endif

// Local Variables:
// mode:C++
// End:
