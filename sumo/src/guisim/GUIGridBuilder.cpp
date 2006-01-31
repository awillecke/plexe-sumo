//---------------------------------------------------------------------------//
//                        GUIGridBuilder.h -
//  A class dividing the network in rectangular cells
//                           -------------------
//  project              : SUMO - Simulation of Urban MObility
//  begin                : Jul 2003
//  copyright            : (C) 2003 by Daniel Krajzewicz
//  organisation         : IVF/DLR http://ivf.dlr.de
//  email                : Daniel.Krajzewicz@dlr.de
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//---------------------------------------------------------------------------//
namespace
{
    const char rcsid[] =
    "$Id$";
}
// $Log$
// Revision 1.7  2006/01/31 10:56:35  dkrajzew
// debugging (unfinished)
//
// Revision 1.6  2005/11/09 06:35:34  dkrajzew
// debugging
//
// Revision 1.5  2005/10/07 11:37:17  dkrajzew
// THIRD LARGE CODE RECHECK: patched problems on Linux/Windows configs
//
// Revision 1.4  2005/09/22 13:39:35  dkrajzew
// SECOND LARGE CODE RECHECK: converted doubles and floats to SUMOReal
//
// Revision 1.3  2005/09/15 11:06:37  dkrajzew
// LARGE CODE RECHECK
//
// Revision 1.2  2005/05/04 07:59:59  dkrajzew
// level 3 warnings removed; a certain SUMOTime time description added
//
// Revision 1.1  2004/11/24 08:46:43  dkrajzew
// recent changes applied
//
// Revision 1.2  2004/10/29 06:01:54  dksumo
// renamed boundery to boundary
//
// Revision 1.1  2004/10/22 12:49:15  dksumo
// initial checkin into an internal, standalone SUMO CVS
//
// Revision 1.7  2004/07/02 08:41:40  dkrajzew
// detector drawer are now also responsible for other additional items
//
// Revision 1.6  2003/12/09 11:27:15  dkrajzew
// documentation added
//
/* =========================================================================
 * compiler pragmas
 * ======================================================================= */
#pragma warning(disable: 4786)


/* =========================================================================
 * included modules
 * ======================================================================= */
#ifdef HAVE_CONFIG_H
#ifdef WIN32
#include <windows_config.h>
#else
#include <config.h>
#endif
#endif // HAVE_CONFIG_H

#include <vector>
#include <algorithm>
#include "GUINet.h"
#include "GUIEdge.h"
#include <utils/geom/GeomHelper.h>
#include "GUIGridBuilder.h"
#include <utils/gui/globjects/GUIGlObject_AbstractAdd.h>
#include <guisim/GUILaneWrapper.h>
#include <guisim/GUIJunctionWrapper.h>

#ifdef _DEBUG
#include <utils/dev/debug_new.h>
#endif // _DEBUG


/* =========================================================================
 * used namespaces
 * ======================================================================= */
using namespace std;


/* =========================================================================
 * member method definitions
 * ======================================================================= */
GUIGridBuilder::GUIGridBuilder(GUINet &net, GUIGrid &grid)
    : myNet(net), myGrid(grid)
{
}


GUIGridBuilder::~GUIGridBuilder()
{
}



void
GUIGridBuilder::build()
{
    // allocate grid
    size_t size = myGrid.getNoXCells()*myGrid.getNoYCells();
    myGrid.myGrid = new GUIGrid::GridCell[size];
    // get the boundary
    myGrid.myBoundary = computeBoundary();
    // assert that the boundary is not zero in neither dimension
    if(myGrid.myBoundary.getHeight()==0||myGrid.myBoundary.getWidth()==0) {
        myGrid.myBoundary.add(myGrid.myBoundary.xmin()+1, myGrid.myBoundary.ymax()+1);
        myGrid.myBoundary.add(myGrid.myBoundary.xmin()-1, myGrid.myBoundary.ymax()-1);
    }
    // compute the cell size
    myGrid.myXCellSize =
        (myGrid.myBoundary.xmax()-myGrid.myBoundary.xmin()) / myGrid.getNoXCells();
    myGrid.myYCellSize =
        (myGrid.myBoundary.ymax()-myGrid.myBoundary.ymin()) / myGrid.getNoYCells();
    // divide Edges on grid
    divideOnGrid();
	myGrid.closeBuilding();
}


Boundary
GUIGridBuilder::computeBoundary()
{
    Boundary ret;
	{
		// use the junctions to compute the boundaries
		for(size_t index=0; index<myNet.myJunctionWrapper.size(); index++) {
			if(myNet.myJunctionWrapper[index]->getShape().size()>0) {
				ret.add(myNet.myJunctionWrapper[index]->getBoundary());
			} else {
				ret.add(myNet.myJunctionWrapper[index]->getJunction().getPosition());
			}
		}
	}
	{
		// use the lanes to compute the boundaries
		for(size_t index=0; index<myNet.myEdgeWrapper.size(); index++) {
			ret.add(myNet.myEdgeWrapper[index]->getBoundary());
		}
	}
    return ret;
}


void
GUIGridBuilder::divideOnGrid()
{
    size_t index;
    for(index=0; index<myNet.myEdgeWrapper.size(); index++) {
        computeEdgeCells(index, myNet.myEdgeWrapper[index]);
    }
    for(index=0; index<myNet.myJunctionWrapper.size(); index++) {
        setJunction(index, myNet.myJunctionWrapper[index]);
    }
    const std::vector<GUIGlObject_AbstractAdd*> &add =
        GUIGlObject_AbstractAdd::getObjectList();
    for(index=0; index<add.size(); index++) {
        setAdditional(index, add[index]);
    }
}


void
GUIGridBuilder::computeEdgeCells(size_t index, GUIEdge *edge)
{
    for(size_t i=0; i<edge->nLanes(); i++) {
        GUILaneWrapper &lane = edge->getLaneGeometry(i);
        computeLaneCells(index, lane);
    }
}


void
GUIGridBuilder::computeLaneCells(size_t index, GUILaneWrapper &lane)
{
    // compute the outer and inner positions of the edge
    //  (meaning the real edge position and the position yielding from
    //  adding the offset of lanes)
    const Position2D &beg = lane.getBegin();
    const Position2D &end = lane.getEnd();
    SUMOReal length = GeomHelper::distance(beg, end);
    std::pair<SUMOReal, SUMOReal> offsets(0, 0);
    if(length!=0) {
        offsets = GeomHelper::getNormal90D_CW(beg, end, length,
            3.5 / 2.0);
    }
    SUMOReal x11 = beg.x() - offsets.first;
    SUMOReal y11 = beg.y() + offsets.second;
    SUMOReal x12 = end.x() - offsets.first;
    SUMOReal y12 = end.y() + offsets.second;

    SUMOReal x21 = beg.x() + offsets.first;
    SUMOReal y21 = beg.y() - offsets.second;
    SUMOReal x22 = end.x() + offsets.first;
    SUMOReal y22 = end.y() - offsets.second;

    // compute the cells the lae is going through
    for(int y=0; y<myGrid.myYSize; y++) {
        SUMOReal ypos1 = SUMOReal(y) * myGrid.myYCellSize;
        for(int x=0; x<myGrid.myXSize; x++) {
            SUMOReal xpos1 = SUMOReal(x) * myGrid.myXCellSize;
            if(
                GeomHelper::intersects(x11, y11, x12, y12,
                    xpos1, ypos1, xpos1+myGrid.myXCellSize, ypos1) ||
                GeomHelper::intersects(x11, y11, x12, y12,
                    xpos1, ypos1, xpos1, ypos1+myGrid.myYCellSize) ||
                GeomHelper::intersects(x11, y11, x12, y12,
                    xpos1, ypos1+myGrid.myYCellSize, xpos1+myGrid.myXCellSize,
                    ypos1+myGrid.myYCellSize) ||
                GeomHelper::intersects(x11, y11, x12, y12,
                    xpos1+myGrid.myXCellSize, ypos1, xpos1+myGrid.myXCellSize,
                    ypos1+myGrid.myYCellSize) ||

                GeomHelper::intersects(x21, y21, x22, y22,
                    xpos1, ypos1, xpos1+myGrid.myXCellSize, ypos1) ||
                GeomHelper::intersects(x21, y21, x22, y22,
                    xpos1, ypos1, xpos1, ypos1+myGrid.myYCellSize) ||
                GeomHelper::intersects(x21, y21, x22, y22,
                    xpos1, ypos1+myGrid.myYCellSize, xpos1+myGrid.myXCellSize,
                    ypos1+myGrid.myYCellSize) ||
                GeomHelper::intersects(x21, y21, x22, y22,
                    xpos1+myGrid.myXCellSize, ypos1, xpos1+myGrid.myXCellSize,
                    ypos1+myGrid.myYCellSize) ||

                (x11>=xpos1&&x11<xpos1+myGrid.myXCellSize&&y11>=ypos1&&y11<ypos1+myGrid.myYCellSize) ||
                (x12>=xpos1&&x12<xpos1+myGrid.myXCellSize&&y12>=ypos1&&y12<ypos1+myGrid.myYCellSize) ||
                (x21>=xpos1&&x21<xpos1+myGrid.myXCellSize&&y21>=ypos1&&y21<ypos1+myGrid.myYCellSize) ||
                (x22>=xpos1&&x22<xpos1+myGrid.myXCellSize&&y22>=ypos1&&y22<ypos1+myGrid.myYCellSize)
                    )
            {
                size_t offset = myGrid.myXSize * y + x;
                myGrid.myGrid[offset].addEdge(index);
            }
        }
    }
}


void
GUIGridBuilder::setJunction(size_t index, GUIJunctionWrapper *junction)
{
    std::vector<size_t> cells = myGrid.getCellsContaining(junction->getBoundary());
    for(std::vector<size_t>::iterator i=cells.begin(); i!=cells.end(); i++) {
        myGrid.myGrid[*i].addJunction(index);
    }
}


void
GUIGridBuilder::setAdditional(size_t index, GUIGlObject_AbstractAdd *add)
{
    std::vector<size_t> cells = myGrid.getCellsContaining(add->getBoundary());
    for(std::vector<size_t>::iterator i=cells.begin(); i!=cells.end(); i++) {
        myGrid.myGrid[*i].addAdditional(index);
    }
}


/**************** DO NOT DEFINE ANYTHING AFTER THE INCLUDE *****************/

// Local Variables:
// mode:C++
// End:




