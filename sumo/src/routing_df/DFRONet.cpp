#include "DFRONet.h"
#include <routing_df/DFDetector.h>

/* =========================================================================
 * compiler pragmas
 * ======================================================================= */
#pragma warning(disable: 4786)
#pragma warning(disable: 4503)


#include <iostream>
#include <routing_df/DFDetector.h>
#include <routing_df/DFRORouteDesc.h>

using namespace std;

DFRONet::DFRONet()
{
	ro = NULL;
}

DFRONet::DFRONet(RONet * Ro, bool amInHighwayMode)
	: myAmInHighwayMode(amInHighwayMode)
{
	ro = Ro;
}

DFRONet::~DFRONet()
{
    delete ro;
}

void
DFRONet::buildApproachList()
{
	std::vector<ROEdge *> r = ro->getMyEdgeCont()->getTempVector();
	std::vector<ROEdge *>::iterator rit = r.begin();
	for ( ; rit != r.end(); rit++ )
	{
		size_t i = 0;
        size_t length_size = (*rit)->getNoFollowing();
        for(i=0; i<length_size; i++)
		{
            ROEdge *help = (*rit)->getFollower(i);
			// add the connection help->*rit to myApproachingEdges
			if(myApproachingEdges.find(help)==myApproachingEdges.end()) {
                myApproachingEdges[help] = std::vector<ROEdge*>();
            }
			myApproachingEdges[help].push_back((*rit));
			// add the connection *rit->help to myApproachingEdges
            if(myApproachedEdges.find((*rit))==myApproachedEdges.end()) {
                myApproachedEdges[(*rit)] = std::vector<ROEdge*>();
            }
			myApproachedEdges[(*rit)].push_back(help);

		}
	}

	//debug
    /*
	std::cout << "approaching" << std::endl;
	std::map<std::string, std::vector<std::string> >::iterator it;
	for ( it = myApproachingEdges.begin(); it != myApproachingEdges.end(); it++ )
	{
		std::cout << it->first << std::endl;
		std::vector<std::string>::iterator ti;
		for ( ti = it->second.begin(); ti != it->second.end(); ti++ )
		{
			std::cout << "\t" << (*ti) << std::endl;
		}
	}
	std::cout << "approached" << std::endl;
	//std::map<std::string, std::vector<std::string> >::iterator it;
	for ( it = myApproachedEdges.begin(); it != myApproachedEdges.end(); it++ )
	{
		std::cout << it->first << std::endl;
		std::vector<std::string>::iterator ti;
		for ( ti = it->second.begin(); ti != it->second.end(); ti++ )
		{
			std::cout << "\t" << (*ti) << std::endl;
		}
	}
    */
}


size_t
DFRONet::numberOfEdgesBefore(ROEdge *edge) const
{
    if(myApproachingEdges.find(edge)==myApproachingEdges.end()) {
        return 0;
    }
    return myApproachingEdges.find(edge)->second.size();
}

const std::vector<ROEdge *> &
DFRONet::getEdgesBefore(ROEdge *edge) const
{
    assert(myApproachingEdges.find(edge)!=myApproachingEdges.end());
    return myApproachingEdges.find(edge)->second;
}


size_t
DFRONet::numberOfEdgesAfter(ROEdge *edge) const
{
    if(myApproachedEdges.find(edge)==myApproachedEdges.end()) {
        return 0;
    }
    return myApproachedEdges.find(edge)->second.size();
}


const std::vector<ROEdge *> &
DFRONet::getEdgesAfter(ROEdge *edge) const
{
    assert(myApproachedEdges.find(edge)!=myApproachedEdges.end());
    return myApproachedEdges.find(edge)->second;
}


void
DFRONet::buildDetectorEdgeDependencies(DFDetectorCon &detcont) const
{
    myDetectorsOnEdges.clear();
    myDetectorEdges.clear();
    const std::vector<DFDetector*> &dets = detcont.getDetectors();
    {
        for(std::vector<DFDetector*>::const_iterator i=dets.begin(); i!=dets.end(); ++i) {
            ROEdge *e = getDetectorEdge(**i);

            if(myDetectorsOnEdges.find(e)==myDetectorsOnEdges.end()) {
                myDetectorsOnEdges[e] = std::vector<std::string>();
            }
            myDetectorsOnEdges[e].push_back((*i)->getID());

            myDetectorEdges[(*i)->getID()] = e;
        }
    }
}


void
DFRONet::computeTypes(DFDetectorCon &detcont) const
{
    const std::vector< DFDetector*> &dets = detcont.getDetectors();
    // build needed information. first
    buildDetectorEdgeDependencies(detcont);
    // compute detector types then
    {
        for(std::vector< DFDetector*>::const_iterator i=dets.begin(); i!=dets.end(); ++i) {
			if((*i)->getType()==DISCARDED_DETECTOR) {
				continue;
			}
            if(isSource(**i, detcont)) {
                (*i)->setType(SOURCE_DETECTOR);
            }
            if(isDestination(**i, detcont)) {
                (*i)->setType(SINK_DETECTOR);
            }
            if((*i)->getType()==TYPE_NOT_DEFINED) {
                (*i)->setType(BETWEEN_DETECTOR);
            }
        }
    }
    // recheck sources
    {
        for(std::vector< DFDetector*>::const_iterator i=dets.begin(); i!=dets.end(); ++i) {
			if((*i)->getType()!=SOURCE_DETECTOR) {
				continue;
			}
			if(isFalseSource(**i, detcont)) {
                (*i)->setType(DISCARDED_DETECTOR);
            }
        }
    }
}


std::string
DFRONet::buildRouteID(const DFRORouteDesc &desc) const
{
	ROEdge *first = *(desc.edges2Pass.begin());
    ROEdge *last = *(desc.edges2Pass.end()-1);
	return first->getID() + "_to_" + last->getID();
}


bool
DFRONet::hasInBetweenDetectorsOnly(ROEdge *edge,
								   const DFDetectorCon &detectors) const
{
	assert(myDetectorsOnEdges.find(edge)!=myDetectorsOnEdges.end());
	const std::vector<std::string> &detIDs = myDetectorsOnEdges.find(edge)->second;
	std::vector<std::string>::const_iterator i;
	for(i=detIDs.begin(); i!=detIDs.end(); ++i) {
		const DFDetector &det = detectors.getDetector(*i);
		if(det.getType()!=BETWEEN_DETECTOR) {
			return false;
		}
	}
	return true;
}



void
DFRONet::computeRoutesFor(ROEdge *edge, DFRORouteDesc *base, int no,
                          std::vector<ROEdge*> &visited,
                          const DFDetector &det, DFRORouteCont &into,
						  const DFDetectorCon &detectors,
						  std::vector<ROEdge*> &seen) const
{
    priority_queue<DFRORouteDesc*, vector<DFRORouteDesc*>, DFRouteDescByTimeComperator> toSolve;
	base->passedNo = 0;
	toSolve.push(base);
	while(!toSolve.empty()) {
		DFRORouteDesc *current = toSolve.top();
		toSolve.pop();
		ROEdge *last = *(current->edges2Pass.end()-1);
		// do not process an edge twice
		if(find(seen.begin(), seen.end(), last)!=seen.end()) {
			// ... but keep the way to it
			current->routename = buildRouteID(*current);
			into.addRouteDesc(current);
			continue;
		}
		seen.push_back(last);
		// end if the edge has no further connections
		if(!hasApproached(last)) {
			// ok, no further connections to follow
			current->routename = buildRouteID(*current);
			into.addRouteDesc(current);
			continue;
		}
		// check for passing detectors:
		//  if the current last edge is not the one the detector is placed on ...
		bool addNextNoFurther = false;
		if(last!=getDetectorEdge(det)) {
			// ... if there is a detector ...
			if(hasDetector(last)) {
				if(!hasInBetweenDetectorsOnly(last, detectors)) {
					// ... and it's not an in-between-detector
					// -> let's add this edge and the following, but not any further
					addNextNoFurther = true;
					current->lastDetectorEdge = last;
                    current->duration2Last = current->duration;
                    current->distance2Last = current->distance;
					current->endDetectorEdge = last;
				} else {
					// ... if it's an in-between-detector
					// -> mark the current route as to be continued
					current->passedNo = 0;
                    current->duration2Last = current->duration;
                    current->distance2Last = current->distance;
					current->lastDetectorEdge = last;
				}
			}
		}
		// check for highway off-ramps
		if(myAmInHighwayMode) {
			// if it's beside the highway...
			if(last->getSpeed()<19.4&&last!=getDetectorEdge(det)) {
				/*
				// ... and has a detector which is not in-between (!!!)
				if(hasDetector(last)&&!hasInBetweenDetectorsOnly(last, detectors)) {
					// -> let's add this edge and the following, but not any further
					addNextNoFurther = true;
				}
				*/
				// ... and has more than one following edge
				if(myApproachedEdges.find(last)->second.size()>1) {
					// -> let's add this edge and the following, but not any further
					addNextNoFurther = true;
				}

			}
		}
		// check for missing end connections
		if(!addNextNoFurther) {
			// ... if this one would be processed, but already too many edge
			//  without a detector occured
			if(current->passedNo>15) { // !!!
				// mark not to process any further
				cout << "Could not find destinations for '" << det.getID() << "'" << endl;
				current->routename = buildRouteID(*current);
				into.addRouteDesc(current);
				continue; // !!!
			}
		}
		// ... else: loop over the next edges
		const std::vector<ROEdge*> &appr  = myApproachedEdges.find(last)->second;
		for(size_t i=0; i<appr.size(); i++) {
			DFRORouteDesc *t = new DFRORouteDesc(*current);
			t->duration += (last->getLength()/last->getSpeed());//!!!
            t->distance += last->getLength();
			t->edges2Pass.push_back(appr[i]);
			if(!addNextNoFurther) {
				t->passedNo = t->passedNo + 1;
				toSolve.push(t);
			} else {
				t->routename = buildRouteID(*t);
				into.addRouteDesc(t);
			}
		}
	}
}


void
DFRONet::buildRoutes(DFDetectorCon &detcont) const
{
    // build needed information first
    buildDetectorEdgeDependencies(detcont);
    // then build the routes
    std::map<ROEdge*, DFRORouteCont * > doneEdges;
    const std::vector< DFDetector*> &dets = detcont.getDetectors();
    for(std::vector< DFDetector*>::const_iterator i=dets.begin(); i!=dets.end(); ++i) {
        if((*i)->getType()!=SOURCE_DETECTOR) {
            // do not build routes for other than sources
            continue;
        }
/*
        // !!! maybe (optional) routes for between-detectors also should not be build
        if((*i)->getType()==BETWEEN_DETECTOR) {
            // do not build routes for sinks
            continue;
        }
		*/
        ROEdge *e = getDetectorEdge(**i);
        if(doneEdges.find(e)!=doneEdges.end()) {
            // use previously build routes
			(*i)->addRoutes(new DFRORouteCont(*doneEdges[e]));
            continue;
        }
		std::vector<ROEdge*> seen;
		DFRORouteCont *routes = new DFRORouteCont();
        doneEdges[e] = routes;
        DFRORouteDesc *rd = new DFRORouteDesc();
        rd->edges2Pass.push_back(e);
        rd->duration = 0;
		rd->endDetectorEdge = 0;
		rd->lastDetectorEdge = 0;
		rd->distance = 0;
		rd->distance2Last = 0;
		rd->duration2Last = 0;
        std::vector<ROEdge*> visited;
        visited.push_back(e);
        computeRoutesFor(e, rd, 0, visited, **i, *routes, detcont, seen);
		(*i)->addRoutes(routes);
		//cout << (*i)->getID() << " : " << routes->get().size() << endl;

    }
}


ROEdge *
DFRONet::getDetectorEdge(const DFDetector &det) const
{
    string edgeName = det.getLaneID();
    edgeName = edgeName.substr(0, edgeName.rfind('_'));
    return ro->getEdge(edgeName);
}


bool
DFRONet::hasApproaching(ROEdge *edge) const
{
	return
		myApproachingEdges.find(edge)!=myApproachingEdges.end()
		&&
		myApproachingEdges.find(edge)->second.size()!=0;
}


bool
DFRONet::hasApproached(ROEdge *edge) const
{
	return
		myApproachedEdges.find(edge)!=myApproachedEdges.end()
		&&
		myApproachedEdges.find(edge)->second.size()!=0;
}


bool
DFRONet::hasDetector(ROEdge *edge) const
{
	return
		myDetectorsOnEdges.find(edge)!=myDetectorsOnEdges.end()
		&&
		myDetectorsOnEdges.find(edge)->second.size()!=0;
}


SUMOReal
DFRONet::getAbsPos(const DFDetector &det) const
{
	if(det.getPos()>=0) {
		return det.getPos();
	}
	return getDetectorEdge(det)->getLength() + det.getPos();
}

bool
DFRONet::isSource(const DFDetector &det, const DFDetectorCon &detectors) const
{
    return
		isSource(det, getDetectorEdge(det), std::vector<ROEdge*>(), detectors);
}

bool
DFRONet::isFalseSource(const DFDetector &det, const DFDetectorCon &detectors) const
{
    return
		isFalseSource(det, getDetectorEdge(det), std::vector<ROEdge*>(), detectors);
}

bool
DFRONet::isDestination(const DFDetector &det, const DFDetectorCon &detectors) const
{
    return isDestination(det, getDetectorEdge(det), std::vector<ROEdge*>(),
		detectors);
}


bool
DFRONet::isSource(const DFDetector &det, ROEdge *edge,
					   std::vector<ROEdge*> seen,
					   const DFDetectorCon &detectors) const
{
	if(edge==getDetectorEdge(det)) {
		// maybe there is another detector at the same edge
		//  get the list of this/these detector(s)
		const std::vector<std::string> &detsOnEdge = myDetectorsOnEdges.find(edge)->second;
		for(std::vector<std::string>::const_iterator i=detsOnEdge.begin(); i!=detsOnEdge.end(); ++i) {
			if((*i)==det.getID()) {
				continue;
			}
			const DFDetector &sec = detectors.getDetector(*i);
			if(getAbsPos(sec)<getAbsPos(det)) {
				// ok, there is another detector on the same edge and it is
				//  before this one -> no source
				return false;
			}
		}
	}
    // it's a source if no edges are approaching the edge
    if(!hasApproaching(edge)) {
		if(edge!=getDetectorEdge(det)) {
			if(hasDetector(edge)) {
				return false;
			}
		}
        return true;
    }
	/*
	if(edge==getDetectorEdge(det)) {
		// maybe there is another detector at the same edge
		//  get the list of this/these detector(s)
		const std::vector<std::string> &detsOnEdge = myDetectorsOnEdges.find(edge)->second;
		for(std::vector<std::string>::const_iterator i=detsOnEdge.begin(); i!=detsOnEdge.end(); ++i) {
			if((*i)==det.getID()) {
				continue;
			}
			const DFDetector &sec = detectors.getDetector(*i);
			if(getAbsPos(sec)<getAbsPos(det)) {
				// ok, there is another detector on the same edge and it is
				//  before this one -> no source
				return false;
			}
		}
		*/
	if(edge!=getDetectorEdge(det)) {
		// ok, we are at one of the edges in front
		if(myAmInHighwayMode) {
			if(edge->getSpeed()>=19.4) {
				if(hasDetector(edge)) {
					// we are still on the highway and there is another detector
					return false;
				}
			}
		}
	}

	if(myAmInHighwayMode) {
		if(edge->getSpeed()<19.4&&edge!=getDetectorEdge(det)) {
			// we have left the highway already
			//  -> the detector will be a highway source
			//if(hasDetector(edge)) {
				/*myDetectorsOnEdges.find(edge)==myDetectorsOnEdges.end()
				||
			   myDetectorsOnEdges.find(edge)->second.size()!=0) {*/
				return true;
			//}
		}
	}
    if(myDetectorsOnEdges.find(edge)!=myDetectorsOnEdges.end()
        &&
        myDetectorEdges.find(det.getID())->second!=edge) {
        return false;
    }

	// let's check the edges in front
    const std::vector<ROEdge*> &appr = myApproachingEdges.find(edge)->second;
    bool isall = true;
    seen.push_back(edge);
    for(size_t i=0; i<appr.size()&&isall; i++) {
        bool had = std::find(seen.begin(), seen.end(), appr[i])!=seen.end();
        if(!had) {
            if(!isSource(det, appr[i], seen, detectors)) {
                isall = false;
            }
        }
    }
	return isall;
}


bool
DFRONet::isDestination(const DFDetector &det, ROEdge *edge, std::vector<ROEdge*> seen,
					   const DFDetectorCon &detectors) const
{
	if(det.getID()=="1607") {
		int bla = 0;
	}
	if(edge==getDetectorEdge(det)) {
		// maybe there is another detector at the same edge
		//  get the list of this/these detector(s)
		const std::vector<std::string> &detsOnEdge = myDetectorsOnEdges.find(edge)->second;
		for(std::vector<std::string>::const_iterator i=detsOnEdge.begin(); i!=detsOnEdge.end(); ++i) {
			if((*i)==det.getID()) {
				continue;
			}
			const DFDetector &sec = detectors.getDetector(*i);
			if(getAbsPos(sec)>getAbsPos(det)) {
				// ok, there is another detector on the same edge and it is
				//  after this one -> no destination
				return false;
			}
		}
	}
    if(!hasApproached(edge)) {
		if(edge!=getDetectorEdge(det)) {
			if(hasDetector(edge)) {
				return false;
			}
		}
        return true;
    }
	if(edge!=getDetectorEdge(det)) {
		// ok, we are at one of the edges coming behind
		if(myAmInHighwayMode) {
			if(edge->getSpeed()>=19.4) {
				if(hasDetector(edge)) {
					// we are still on the highway and there is another detector
					return false;
				}
			}
		}
	}

	if(myAmInHighwayMode) {
		if(edge->getSpeed()<19.4&&edge!=getDetectorEdge(det)) {
			if(hasDetector(edge)) {
				return true;
			}
			if(myApproachedEdges.find(edge)->second.size()>1) {
				return true;
			}

		}
	}

    if(myDetectorsOnEdges.find(edge)!=myDetectorsOnEdges.end()
        &&
        myDetectorEdges.find(det.getID())->second!=edge) {
        return false;
    }
    const std::vector<ROEdge*> &appr  = myApproachedEdges.find(edge)->second;
    bool isall = true;
    seen.push_back(edge);
    for(size_t i=0; i<appr.size()&&isall; i++) {
        //printf("checking %s->\n", appr[i].c_str());
        bool had = std::find(seen.begin(), seen.end(), appr[i])!=seen.end();
        if(!had) {
            if(!isDestination(det, appr[i], seen, detectors)) {
                isall = false;
            }
        }
    }
    return isall;
}

bool
DFRONet::isFalseSource(const DFDetector &det, ROEdge *edge, std::vector<ROEdge*> seen,
					   const DFDetectorCon &detectors) const
{
	if(det.getID()=="2868") {
		int bla = 0;
	}
	if(edge!=getDetectorEdge(det)) {
		// ok, we are at one of the edges coming behind
		if(myAmInHighwayMode) {
			if(hasDetector(edge)) {
				const std::vector<std::string> &dets = myDetectorsOnEdges.find(edge)->second;
				for(std::vector<std::string>::const_iterator i=dets.begin(); i!=dets.end(); ++i) {
					if(detectors.getDetector(*i).getType()==SINK_DETECTOR) {
						return false;
					}
					if(detectors.getDetector(*i).getType()==BETWEEN_DETECTOR) {
						return false;
					}
					const DFDetector &d = detectors.getDetector(*i);
					if(detectors.getDetector(*i).getType()==SOURCE_DETECTOR) {
						return true;
					}
				}
			}
		}
	}


    const std::vector<ROEdge*> &appr  = myApproachedEdges.find(edge)->second;
    bool isall = false;
    seen.push_back(edge);
    for(size_t i=0; i<appr.size()&&!isall; i++) {
        //printf("checking %s->\n", appr[i].c_str());
        bool had = std::find(seen.begin(), seen.end(), appr[i])!=seen.end();
        if(!had) {
            if(isFalseSource(det, appr[i], seen, detectors)) {
                isall = true;
			}
        }
	}
    return isall;
}
