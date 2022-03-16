/****************************************************************************/
// Eclipse SUMO, Simulation of Urban MObility; see https://eclipse.org/sumo
// Copyright (C) 2001-2022 German Aerospace Center (DLR) and others.
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// https://www.eclipse.org/legal/epl-2.0/
// This Source Code may also be made available under the following Secondary
// Licenses when the conditions for such availability set forth in the Eclipse
// Public License 2.0 are satisfied: GNU General Public License, version 2
// or later which is available at
// https://www.gnu.org/licenses/old-licenses/gpl-2.0-standalone.html
// SPDX-License-Identifier: EPL-2.0 OR GPL-2.0-or-later
/****************************************************************************/
/// @file    GNEFrameModules.cpp
/// @author  Pablo Alvarez Lopez
/// @date    Mar 2022
///
// Frame for create paths
/****************************************************************************/
#include <config.h>

#include <netedit/GNEApplicationWindow.h>
#include <netedit/GNENet.h>
#include <netedit/GNEUndoList.h>
#include <netedit/GNEViewNet.h>
#include <netedit/GNEViewParent.h>
#include <netedit/changes/GNEChange_Children.h>
#include <netedit/elements/additional/GNEAccess.h>
#include <netedit/elements/additional/GNEBusStop.h>
#include <netedit/elements/additional/GNECalibrator.h>
#include <netedit/elements/additional/GNECalibratorFlow.h>
#include <netedit/elements/additional/GNEChargingStation.h>
#include <netedit/elements/additional/GNEClosingLaneReroute.h>
#include <netedit/elements/additional/GNEClosingReroute.h>
#include <netedit/elements/additional/GNEContainerStop.h>
#include <netedit/elements/additional/GNEDestProbReroute.h>
#include <netedit/elements/additional/GNEDetectorE1.h>
#include <netedit/elements/additional/GNEDetectorE1Instant.h>
#include <netedit/elements/additional/GNEDetectorE2.h>
#include <netedit/elements/additional/GNEDetectorE3.h>
#include <netedit/elements/additional/GNEDetectorEntryExit.h>
#include <netedit/elements/additional/GNEPOI.h>
#include <netedit/elements/additional/GNEParkingArea.h>
#include <netedit/elements/additional/GNEParkingAreaReroute.h>
#include <netedit/elements/additional/GNEParkingSpace.h>
#include <netedit/elements/additional/GNEPoly.h>
#include <netedit/elements/additional/GNERerouter.h>
#include <netedit/elements/additional/GNERerouterInterval.h>
#include <netedit/elements/additional/GNERouteProbReroute.h>
#include <netedit/elements/additional/GNERouteProbe.h>
#include <netedit/elements/additional/GNETAZ.h>
#include <netedit/elements/additional/GNETAZSourceSink.h>
#include <netedit/elements/additional/GNEVaporizer.h>
#include <netedit/elements/additional/GNEVariableSpeedSign.h>
#include <netedit/elements/additional/GNEVariableSpeedSignStep.h>
#include <netedit/elements/additional/GNETractionSubstation.h>
#include <netedit/elements/additional/GNEOverheadWire.h>
#include <netedit/elements/data/GNEDataInterval.h>
#include <netedit/elements/demand/GNEContainer.h>
#include <netedit/elements/demand/GNEPerson.h>
#include <netedit/elements/demand/GNEPersonTrip.h>
#include <netedit/elements/demand/GNERide.h>
#include <netedit/elements/demand/GNERoute.h>
#include <netedit/elements/demand/GNEStop.h>
#include <netedit/elements/demand/GNETranship.h>
#include <netedit/elements/demand/GNETransport.h>
#include <netedit/elements/demand/GNEVehicle.h>
#include <netedit/elements/demand/GNEVType.h>
#include <netedit/elements/demand/GNEVTypeDistribution.h>
#include <netedit/elements/demand/GNEWalk.h>
#include <netedit/elements/network/GNEConnection.h>
#include <netedit/elements/network/GNECrossing.h>
#include <netedit/frames/common/GNEInspectorFrame.h>
#include <utils/foxtools/MFXMenuHeader.h>
#include <utils/gui/div/GLHelper.h>
#include <utils/gui/div/GUIDesigns.h>
#include <utils/gui/globjects/GLIncludes.h>
#include <utils/gui/windows/GUIAppEnum.h>

#include "GNEM_PathCreator.h"
#include "GNEFrame.h"


// ===========================================================================
// FOX callback mapping
// ===========================================================================

FXDEFMAP(GNEM_PathCreator) PathCreatorMap[] = {
    FXMAPFUNC(SEL_COMMAND, MID_GNE_EDGEPATH_ABORT,          GNEM_PathCreator::onCmdAbortPathCreation),
    FXMAPFUNC(SEL_COMMAND, MID_GNE_EDGEPATH_FINISH,         GNEM_PathCreator::onCmdCreatePath),
    FXMAPFUNC(SEL_COMMAND, MID_GNE_EDGEPATH_REMOVELAST,     GNEM_PathCreator::onCmdRemoveLastElement),
    FXMAPFUNC(SEL_COMMAND, MID_GNE_EDGEPATH_SHOWCANDIDATES, GNEM_PathCreator::onCmdShowCandidateEdges)
};

// Object implementation
FXIMPLEMENT(GNEM_PathCreator,                FXGroupBoxModule,     PathCreatorMap,                 ARRAYNUMBER(PathCreatorMap))


// ===========================================================================
// method definitions
// ===========================================================================

GNEM_PathCreator::Path::Path(const SUMOVehicleClass vClass, GNEEdge* edge) :
    mySubPath({edge}),
          myFromBusStop(nullptr),
          myToBusStop(nullptr),
          myConflictVClass(false),
myConflictDisconnected(false) {
    // check if we have to change vClass flag
    if (edge->getNBEdge()->getNumLanesThatAllow(vClass) == 0) {
        myConflictVClass = true;
    }
}


GNEM_PathCreator::Path::Path(GNEViewNet* viewNet, const SUMOVehicleClass vClass, GNEEdge* edgeFrom, GNEEdge* edgeTo) :
    myFromBusStop(nullptr),
    myToBusStop(nullptr),
    myConflictVClass(false),
    myConflictDisconnected(false) {
    // calculate subpath
    mySubPath = viewNet->getNet()->getPathManager()->getPathCalculator()->calculateDijkstraPath(vClass, {edgeFrom, edgeTo});
    // if subPath is empty, try it with pedestrian (i.e. ignoring vCass)
    if (mySubPath.empty()) {
        mySubPath = viewNet->getNet()->getPathManager()->getPathCalculator()->calculateDijkstraPath(SVC_PEDESTRIAN, {edgeFrom, edgeTo});
        if (mySubPath.empty()) {
            mySubPath = { edgeFrom, edgeTo };
            myConflictDisconnected = true;
        } else {
            myConflictVClass = true;
        }
    }
}


const std::vector<GNEEdge*>&
GNEM_PathCreator::Path::getSubPath() const {
    return mySubPath;
}


GNEAdditional* GNEM_PathCreator::Path::getFromBusStop() const {
    return myFromBusStop;
}


GNEAdditional* GNEM_PathCreator::Path::getToBusStop() const {
    return myToBusStop;
}


bool
GNEM_PathCreator::Path::isConflictVClass() const {
    return myConflictVClass;
}


bool
GNEM_PathCreator::Path::isConflictDisconnected() const {
    return myConflictDisconnected;
}


GNEM_PathCreator::Path::Path() :
    myFromBusStop(nullptr),
    myToBusStop(nullptr),
    myConflictVClass(false),
    myConflictDisconnected(false) {
}


GNEM_PathCreator::GNEM_PathCreator(GNEFrame* frameParent) :
    FXGroupBoxModule(frameParent->getContentFrame(), "Route creator"),
    myFrameParent(frameParent),
    myVClass(SVC_PASSENGER),
    myCreationMode(0),
    myToStoppingPlace(nullptr),
    myRoute(nullptr) {
    // create label for route info
    myInfoRouteLabel = new FXLabel(getCollapsableFrame(), "No edges selected", 0, GUIDesignLabelFrameThicked);
    // create button for finish route creation
    myFinishCreationButton = new FXButton(getCollapsableFrame(), "Finish route creation", nullptr, this, MID_GNE_EDGEPATH_FINISH, GUIDesignButton);
    myFinishCreationButton->disable();
    // create button for abort route creation
    myAbortCreationButton = new FXButton(getCollapsableFrame(), "Abort route creation", nullptr, this, MID_GNE_EDGEPATH_ABORT, GUIDesignButton);
    myAbortCreationButton->disable();
    // create button for remove last inserted edge
    myRemoveLastInsertedElement = new FXButton(getCollapsableFrame(), "Remove last inserted edge", nullptr, this, MID_GNE_EDGEPATH_REMOVELAST, GUIDesignButton);
    myRemoveLastInsertedElement->disable();
    // create check button
    myShowCandidateEdges = new FXCheckButton(getCollapsableFrame(), "Show candidate edges", this, MID_GNE_EDGEPATH_SHOWCANDIDATES, GUIDesignCheckButton);
    myShowCandidateEdges->setCheck(TRUE);
    // create shift label
    myShiftLabel = new FXLabel(this,
                               "SHIFT-click: ignore vClass",
                               0, GUIDesignLabelFrameInformation);
    // create control label
    myControlLabel = new FXLabel(this,
                                 "CTRL-click: add disconnected",
                                 0, GUIDesignLabelFrameInformation);
    // create backspace label (always shown)
    new FXLabel(this,
                "BACKSPACE: undo click",
                0, GUIDesignLabelFrameInformation);
}


GNEM_PathCreator::~GNEM_PathCreator() {}


void
GNEM_PathCreator::showPathCreatorModule(SumoXMLTag element, const bool firstElement, const bool consecutives) {
    // declare flag
    bool showPathCreator = true;
    // first abort creation
    abortPathCreation();
    // disable buttons
    myFinishCreationButton->disable();
    myAbortCreationButton->disable();
    myRemoveLastInsertedElement->disable();
    // reset creation mode
    myCreationMode = 0;
    // set first element
    if (firstElement) {
        myCreationMode |= REQUIRE_FIRSTELEMENT;
    }
    // set consecutive or non consecuives
    if (consecutives) {
        myCreationMode |= CONSECUTIVE_EDGES;
    } else {
        myCreationMode |= NONCONSECUTIVE_EDGES;
    }
    // set specific mode depending of tag
    switch (element) {
        // routes
        case SUMO_TAG_ROUTE:
        case GNE_TAG_ROUTE_EMBEDDED:
            myCreationMode |= SHOW_CANDIDATE_EDGES;
            myCreationMode |= START_EDGE;
            myCreationMode |= END_EDGE;
            break;
        // vehicles
        case SUMO_TAG_VEHICLE:
        case GNE_TAG_FLOW_ROUTE:
        case GNE_TAG_WALK_ROUTE:
            myCreationMode |= SINGLE_ELEMENT;
            myCreationMode |= ROUTE;
            break;
        case SUMO_TAG_TRIP:
        case SUMO_TAG_FLOW:
        case GNE_TAG_VEHICLE_WITHROUTE:
        case GNE_TAG_FLOW_WITHROUTE:
            myCreationMode |= SHOW_CANDIDATE_EDGES;
            myCreationMode |= START_EDGE;
            myCreationMode |= END_EDGE;
            break;
        case GNE_TAG_TRIP_JUNCTIONS:
        case GNE_TAG_FLOW_JUNCTIONS:
            myCreationMode |= SHOW_CANDIDATE_JUNCTIONS;
            myCreationMode |= START_JUNCTION;
            myCreationMode |= END_JUNCTION;
            myCreationMode |= ONLY_FROMTO;
            break;
        // walk edges
        case GNE_TAG_WALK_EDGES:
            myCreationMode |= SHOW_CANDIDATE_EDGES;
            myCreationMode |= START_EDGE;
            myCreationMode |= END_EDGE;
            break;
        // edge->edge
        case GNE_TAG_PERSONTRIP_EDGE:
        case GNE_TAG_RIDE_EDGE:
        case GNE_TAG_WALK_EDGE:
            myCreationMode |= SHOW_CANDIDATE_EDGES;
            myCreationMode |= ONLY_FROMTO;
            myCreationMode |= START_EDGE;
            myCreationMode |= END_EDGE;
            break;
        // edge->busStop
        case GNE_TAG_PERSONTRIP_BUSSTOP:
        case GNE_TAG_RIDE_BUSSTOP:
        case GNE_TAG_WALK_BUSSTOP:
            myCreationMode |= SHOW_CANDIDATE_EDGES;
            myCreationMode |= ONLY_FROMTO;
            myCreationMode |= END_BUSSTOP;
            break;
        // junction->junction
        case GNE_TAG_PERSONTRIP_JUNCTIONS:
        case GNE_TAG_WALK_JUNCTIONS:
            myCreationMode |= SHOW_CANDIDATE_JUNCTIONS;
            myCreationMode |= START_JUNCTION;
            myCreationMode |= END_JUNCTION;
            myCreationMode |= ONLY_FROMTO;
            break;
        // stops
        case GNE_TAG_STOPPERSON_BUSSTOP:
            myCreationMode |= SINGLE_ELEMENT;
            myCreationMode |= END_BUSSTOP;
            break;
        case GNE_TAG_STOPPERSON_EDGE:
            myCreationMode |= SINGLE_ELEMENT;
            myCreationMode |= START_EDGE;
            break;
        // generic datas
        case SUMO_TAG_EDGEREL:
            myCreationMode |= ONLY_FROMTO;
            myCreationMode |= START_EDGE;
            myCreationMode |= END_EDGE;
            break;
        default:
            showPathCreator = false;
            break;
    }
    // update colors
    updateEdgeColors();
    updateJunctionColors();
    // check if show path creator
    if (showPathCreator) {
        // recalc before show (to avoid graphic problems)
        recalc();
        // show modul
        show();
    } else {
        // hide modul
        hide();
    }
}


void
GNEM_PathCreator::hidePathCreatorModule() {
    // clear path
    clearPath();
    // hide modul
    hide();
}


SUMOVehicleClass
GNEM_PathCreator::getVClass() const {
    return myVClass;
}


void
GNEM_PathCreator::setVClass(SUMOVehicleClass vClass) {
    myVClass = vClass;
    // update edge colors
    updateEdgeColors();
}


bool
GNEM_PathCreator::addJunction(GNEJunction* junction, const bool /* shiftKeyPressed */, const bool /* controlKeyPressed */) {
    // check if junctions are allowed
    if (((myCreationMode & START_JUNCTION) + (myCreationMode & END_JUNCTION)) == 0) {
        return false;
    }
    // check if only an junction is allowed
    if ((myCreationMode & SINGLE_ELEMENT) && (mySelectedJunctions.size() == 1)) {
        return false;
    }
    // continue depending of number of selected edge
    if (mySelectedJunctions.size() > 0) {
        // check double junctions
        if (mySelectedJunctions.back() == junction) {
            // Write warning
            WRITE_WARNING("Double junctions aren't allowed");
            // abort add junction
            return false;
        }
    }
    // check number of junctions
    if (mySelectedJunctions.size() == 2 && (myCreationMode & Mode::ONLY_FROMTO)) {
        // Write warning
        WRITE_WARNING("Only two junctions are allowed");
        // abort add junction
        return false;
    }
    // All checks ok, then add it in selected elements
    mySelectedJunctions.push_back(junction);
    // enable abort route button
    myAbortCreationButton->enable();
    // enable finish button
    myFinishCreationButton->enable();
    // disable undo/redo
    myFrameParent->getViewNet()->getViewParent()->getGNEAppWindows()->disableUndoRedo("route creation");
    // enable or disable remove last junction button
    if (mySelectedJunctions.size() > 1) {
        myRemoveLastInsertedElement->enable();
    } else {
        myRemoveLastInsertedElement->disable();
    }
    // recalculate path
    recalculatePath();
    // update info route label
    updateInfoRouteLabel();
    // update junction colors
    updateJunctionColors();
    return true;
}


bool
GNEM_PathCreator::addEdge(GNEEdge* edge, const bool shiftKeyPressed, const bool controlKeyPressed) {
    // check if edges are allowed
    if (((myCreationMode & CONSECUTIVE_EDGES) + (myCreationMode & NONCONSECUTIVE_EDGES) +
            (myCreationMode & START_EDGE) + (myCreationMode & END_EDGE)) == 0) {
        return false;
    }
    // check if only an edge is allowed
    if ((myCreationMode & SINGLE_ELEMENT) && (mySelectedEdges.size() == 1)) {
        return false;
    }
    // continue depending of number of selected eges
    if (mySelectedEdges.size() > 0) {
        // check double edges
        if (mySelectedEdges.back() == edge) {
            // Write warning
            WRITE_WARNING("Double edges aren't allowed");
            // abort add edge
            return false;
        }
        // check consecutive edges
        if (myCreationMode & Mode::CONSECUTIVE_EDGES) {
            // check that new edge is consecutive
            const auto& outgoingEdges = mySelectedEdges.back()->getToJunction()->getGNEOutgoingEdges();
            if (std::find(outgoingEdges.begin(), outgoingEdges.end(), edge) == outgoingEdges.end()) {
                // Write warning
                WRITE_WARNING("Only consecutives edges are allowed");
                // abort add edge
                return false;
            }
        }
    }
    // check number of edges
    if (mySelectedEdges.size() == 2 && (myCreationMode & Mode::ONLY_FROMTO)) {
        // Write warning
        WRITE_WARNING("Only two edges are allowed");
        // abort add edge
        return false;
    }
    // check candidate edge
    if ((myShowCandidateEdges->getCheck() == TRUE) && !edge->isPossibleCandidate()) {
        if (edge->isSpecialCandidate()) {
            if (!shiftKeyPressed) {
                // Write warning
                WRITE_WARNING("Invalid edge (SHIFT + click to add an invalid vClass edge)");
                // abort add edge
                return false;
            }
        } else if (edge->isConflictedCandidate()) {
            if (!controlKeyPressed) {
                // Write warning
                WRITE_WARNING("Invalid edge (CONTROL + click to add a disconnected edge)");
                // abort add edge
                return false;
            }
        }
    }
    // All checks ok, then add it in selected elements
    mySelectedEdges.push_back(edge);
    // enable abort route button
    myAbortCreationButton->enable();
    // enable finish button
    myFinishCreationButton->enable();
    // disable undo/redo
    myFrameParent->getViewNet()->getViewParent()->getGNEAppWindows()->disableUndoRedo("route creation");
    // enable or disable remove last edge button
    if (mySelectedEdges.size() > 1) {
        myRemoveLastInsertedElement->enable();
    } else {
        myRemoveLastInsertedElement->disable();
    }
    // recalculate path
    recalculatePath();
    // update info route label
    updateInfoRouteLabel();
    // update edge colors
    updateEdgeColors();
    return true;
}


const std::vector<GNEEdge*>&
GNEM_PathCreator::getSelectedEdges() const {
    return mySelectedEdges;
}


const std::vector<GNEJunction*>&
GNEM_PathCreator::getSelectedJunctions() const {
    return mySelectedJunctions;
}


bool
GNEM_PathCreator::addStoppingPlace(GNEAdditional* stoppingPlace, const bool /*shiftKeyPressed*/, const bool /*controlKeyPressed*/) {
    // check if stoppingPlaces aren allowed
    if ((myCreationMode & END_BUSSTOP) == 0) {
        return false;
    }
    // check if previously stopping place from was set
    if (myToStoppingPlace) {
        return false;
    } else {
        myToStoppingPlace = stoppingPlace;
    }
    // enable abort route button
    myAbortCreationButton->enable();
    // enable finish button
    myFinishCreationButton->enable();
    // disable undo/redo
    myFrameParent->getViewNet()->getViewParent()->getGNEAppWindows()->disableUndoRedo("route creation");
    // enable or disable remove last stoppingPlace button
    if (myToStoppingPlace) {
        myRemoveLastInsertedElement->enable();
    } else {
        myRemoveLastInsertedElement->disable();
    }
    // recalculate path
    recalculatePath();
    // update info route label
    updateInfoRouteLabel();
    // update stoppingPlace colors
    updateEdgeColors();
    return true;
}


GNEAdditional*
GNEM_PathCreator::getToStoppingPlace(SumoXMLTag expectedTag) const {
    if (myToStoppingPlace && (myToStoppingPlace->getTagProperty().getTag() == expectedTag)) {
        return myToStoppingPlace;
    } else {
        return nullptr;
    }
}


bool
GNEM_PathCreator::addRoute(GNEDemandElement* route, const bool /*shiftKeyPressed*/, const bool /*controlKeyPressed*/) {
    // check if routes aren allowed
    if ((myCreationMode & ROUTE) == 0) {
        return false;
    }
    // check if previously a route was added
    if (myRoute) {
        return false;
    }
    // set route
    myRoute = route;
    // recalculate path
    recalculatePath();
    updateInfoRouteLabel();
    updateEdgeColors();
    return true;
}


void
GNEM_PathCreator::removeRoute() {
    // set route
    myRoute = nullptr;
    // recalculate path
    recalculatePath();
    updateInfoRouteLabel();
    updateEdgeColors();
}


GNEDemandElement*
GNEM_PathCreator::getRoute() const {
    return myRoute;
}


const std::vector<GNEM_PathCreator::Path>&
GNEM_PathCreator::getPath() const {
    return myPath;
}


bool
GNEM_PathCreator::drawCandidateEdgesWithSpecialColor() const {
    return (myShowCandidateEdges->getCheck() == TRUE);
}


void
GNEM_PathCreator::updateJunctionColors() {
    // clear junction colors
    clearJunctionColors();
    // check if show possible candidates
    if (myCreationMode & SHOW_CANDIDATE_JUNCTIONS) {
        // set candidate flags
        for (const auto& junction : myFrameParent->getViewNet()->getNet()->getAttributeCarriers()->getJunctions()) {
            junction.second->resetCandidateFlags();
            junction.second->setPossibleCandidate(true);
        }
    }
    // set selected junctions
    if (mySelectedJunctions.size() > 0) {
        // mark selected eges
        for (const auto& junction : mySelectedJunctions) {
            junction->resetCandidateFlags();
            junction->setSourceCandidate(true);
        }
        // finally mark last selected element as target
        mySelectedJunctions.back()->resetCandidateFlags();
        mySelectedJunctions.back()->setTargetCandidate(true);
    }
    // update view net
    myFrameParent->getViewNet()->updateViewNet();
}


void
GNEM_PathCreator::updateEdgeColors() {
    // clear edge colors
    clearEdgeColors();
    // first check if show candidate edges
    if (myShowCandidateEdges->getCheck() == TRUE && (myCreationMode & SHOW_CANDIDATE_EDGES)) {
        // mark all edges that have at least one lane that allow given vClass
        for (const auto& edge : myFrameParent->getViewNet()->getNet()->getAttributeCarriers()->getEdges()) {
            if (edge.second->getNBEdge()->getNumLanesThatAllow(myVClass) > 0) {
                edge.second->setPossibleCandidate(true);
            } else {
                edge.second->setSpecialCandidate(true);
            }
        }
    }
    // set reachability
    if (mySelectedEdges.size() > 0) {
        // only coloring edges if checkbox "show candidate edges" is enabled
        if ((myShowCandidateEdges->getCheck() == TRUE) && (myCreationMode & SHOW_CANDIDATE_EDGES)) {
            // mark all edges as conflicted (to mark special candidates)
            for (const auto& edge : myFrameParent->getViewNet()->getNet()->getAttributeCarriers()->getEdges()) {
                edge.second->resetCandidateFlags();
                edge.second->setConflictedCandidate(true);
            }
            // set special candidates (Edges that are connected but aren't compatibles with current vClass
            setSpecialCandidates(mySelectedEdges.back());
            // mark again all edges as conflicted (to mark possible candidates)
            for (const auto& edge : myFrameParent->getViewNet()->getNet()->getAttributeCarriers()->getEdges()) {
                edge.second->setConflictedCandidate(true);
            }
            // set possible candidates (Edges that are connected AND are compatibles with current vClass
            setPossibleCandidates(mySelectedEdges.back(), myVClass);
        }
        // now mark selected eges
        for (const auto& edge : mySelectedEdges) {
            edge->resetCandidateFlags();
            edge->setSourceCandidate(true);
        }
        // finally mark last selected element as target
        mySelectedEdges.back()->resetCandidateFlags();
        mySelectedEdges.back()->setTargetCandidate(true);
    }
    // update view net
    myFrameParent->getViewNet()->updateViewNet();
}


void
GNEM_PathCreator::clearJunctionColors() {
    // reset all junction flags
    for (const auto& junction : myFrameParent->getViewNet()->getNet()->getAttributeCarriers()->getJunctions()) {
        junction.second->resetCandidateFlags();
    }
}


void
GNEM_PathCreator::clearEdgeColors() {
    // reset all junction flags
    for (const auto& edge : myFrameParent->getViewNet()->getNet()->getAttributeCarriers()->getEdges()) {
        edge.second->resetCandidateFlags();
    }
}


void
GNEM_PathCreator::drawTemporalRoute(const GUIVisualizationSettings& s) const {
    const double lineWidth = 0.35;
    const double lineWidthin = 0.25;
    // Add a draw matrix
    GLHelper::pushMatrix();
    // Start with the drawing of the area traslating matrix to origin
    glTranslated(0, 0, GLO_MAX - 0.1);
    // check if draw bewteen junction or edges
    if (myPath.size() > 0) {
        // set first color
        GLHelper::setColor(RGBColor::GREY);
        // iterate over path
        for (int i = 0; i < (int)myPath.size(); i++) {
            // get path
            const GNEM_PathCreator::Path& path = myPath.at(i);
            // draw line over
            for (int j = 0; j < (int)path.getSubPath().size(); j++) {
                const GNELane* lane = path.getSubPath().at(j)->getLanes().back();
                if (((i == 0) && (j == 0)) || (j > 0)) {
                    GLHelper::drawBoxLines(lane->getLaneShape(), lineWidth);
                }
                // draw connection between lanes
                if ((j + 1) < (int)path.getSubPath().size()) {
                    const GNELane* nextLane = path.getSubPath().at(j + 1)->getLanes().back();
                    if (lane->getLane2laneConnections().exist(nextLane)) {
                        GLHelper::drawBoxLines(lane->getLane2laneConnections().getLane2laneGeometry(nextLane).getShape(), lineWidth);
                    } else {
                        GLHelper::drawBoxLines({lane->getLaneShape().back(), nextLane->getLaneShape().front()}, lineWidth);
                    }
                }
            }
        }
        glTranslated(0, 0, 0.1);
        // iterate over path again
        for (int i = 0; i < (int)myPath.size(); i++) {
            // get path
            const GNEM_PathCreator::Path& path = myPath.at(i);
            // set path color color
            if ((myCreationMode & SHOW_CANDIDATE_EDGES) == 0) {
                GLHelper::setColor(RGBColor::ORANGE);
            } else if (path.isConflictDisconnected()) {
                GLHelper::setColor(s.candidateColorSettings.conflict);
            } else if (path.isConflictVClass()) {
                GLHelper::setColor(s.candidateColorSettings.special);
            } else {
                GLHelper::setColor(RGBColor::ORANGE);
            }
            // draw line over
            for (int j = 0; j < (int)path.getSubPath().size(); j++) {
                const GNELane* lane = path.getSubPath().at(j)->getLanes().back();
                if (((i == 0) && (j == 0)) || (j > 0)) {
                    GLHelper::drawBoxLines(lane->getLaneShape(), lineWidthin);
                }
                // draw connection between lanes
                if ((j + 1) < (int)path.getSubPath().size()) {
                    const GNELane* nextLane = path.getSubPath().at(j + 1)->getLanes().back();
                    if (lane->getLane2laneConnections().exist(nextLane)) {
                        GLHelper::drawBoxLines(lane->getLane2laneConnections().getLane2laneGeometry(nextLane).getShape(), lineWidthin);
                    } else {
                        GLHelper::drawBoxLines({ lane->getLaneShape().back(), nextLane->getLaneShape().front() }, lineWidthin);
                    }
                }
            }
        }
    } else if (mySelectedJunctions.size() > 0) {
        // set color
        GLHelper::setColor(RGBColor::ORANGE);
        // draw line between junctions
        for (int i = 0; i < (int)mySelectedJunctions.size() - 1; i++) {
            // get two points
            const Position posA = mySelectedJunctions.at(i)->getPositionInView();
            const Position posB = mySelectedJunctions.at(i + 1)->getPositionInView();
            const double rot = ((double)atan2((posB.x() - posA.x()), (posA.y() - posB.y())) * (double) 180.0 / (double)M_PI);
            const double len = posA.distanceTo2D(posB);
            // draw line
            GLHelper::drawBoxLine(posA, rot, len, 0.25);
        }
    }
    // Pop last matrix
    GLHelper::popMatrix();
}


void
GNEM_PathCreator::createPath() {
    // call create path implemented in frame parent
    myFrameParent->createPath();
}


void
GNEM_PathCreator::abortPathCreation() {
    // first check that there is elements
    if ((mySelectedJunctions.size() > 0) || (mySelectedEdges.size() > 0) || myToStoppingPlace || myRoute) {
        // unblock undo/redo
        myFrameParent->getViewNet()->getViewParent()->getGNEAppWindows()->enableUndoRedo();
        // clear edges
        clearPath();
        // disable buttons
        myFinishCreationButton->disable();
        myAbortCreationButton->disable();
        myRemoveLastInsertedElement->disable();
        // update info route label
        updateInfoRouteLabel();
        // update junction colors
        updateJunctionColors();
        // update edge colors
        updateEdgeColors();
        // update view (to see the new route)
        myFrameParent->getViewNet()->updateViewNet();
    }
}


void
GNEM_PathCreator::removeLastElement() {
    if (mySelectedEdges.size() > 1) {
        // remove special color of last selected edge
        mySelectedEdges.back()->resetCandidateFlags();
        // remove last edge
        mySelectedEdges.pop_back();
        // change last edge flag
        if ((mySelectedEdges.size() > 0) && mySelectedEdges.back()->isSourceCandidate()) {
            mySelectedEdges.back()->setSourceCandidate(false);
            mySelectedEdges.back()->setTargetCandidate(true);
        }
        // enable or disable remove last edge button
        if (mySelectedEdges.size() > 1) {
            myRemoveLastInsertedElement->enable();
        } else {
            myRemoveLastInsertedElement->disable();
        }
        // recalculate path
        recalculatePath();
        // update info route label
        updateInfoRouteLabel();
        // update junction colors
        updateJunctionColors();
        // update edge colors
        updateEdgeColors();
        // update view
        myFrameParent->getViewNet()->updateViewNet();
    }
}


long
GNEM_PathCreator::onCmdCreatePath(FXObject*, FXSelector, void*) {
    // just call create path
    createPath();
    return 1;
}


long
GNEM_PathCreator::onCmdAbortPathCreation(FXObject*, FXSelector, void*) {
    // just call abort path creation
    abortPathCreation();
    return 1;
}


long
GNEM_PathCreator::onCmdRemoveLastElement(FXObject*, FXSelector, void*) {
    // just call remove last element
    removeLastElement();
    return 1;
}


long
GNEM_PathCreator::onCmdShowCandidateEdges(FXObject*, FXSelector, void*) {
    // update labels
    if (myShowCandidateEdges->getCheck() == TRUE) {
        myShiftLabel->show();
        myControlLabel->show();
    } else {
        myShiftLabel->hide();
        myControlLabel->hide();
    }
    // recalc frame
    recalc();
    // update edge colors (view will be updated within function)
    updateEdgeColors();
    return 1;
}


void
GNEM_PathCreator::updateInfoRouteLabel() {
    if (myPath.size() > 0) {
        // declare variables for route info
        double length = 0;
        double speed = 0;
        int pathSize = 0;
        for (const auto& path : myPath) {
            for (const auto& edge : path.getSubPath()) {
                length += edge->getNBEdge()->getLength();
                speed += edge->getNBEdge()->getSpeed();
            }
            pathSize += (int)path.getSubPath().size();
        }
        // declare ostringstream for label and fill it
        std::ostringstream information;
        information
                << "- Selected edges: " << toString(mySelectedEdges.size()) << "\n"
                << "- Path edges: " << toString(pathSize) << "\n"
                << "- Length: " << toString(length) << "\n"
                << "- Average speed: " << toString(speed / pathSize);
        // set new label
        myInfoRouteLabel->setText(information.str().c_str());
    } else {
        myInfoRouteLabel->setText("No edges selected");
    }
}


void
GNEM_PathCreator::clearPath() {
    /// reset flags
    clearJunctionColors();
    clearEdgeColors();
    // clear junction, edges, additionals and route
    mySelectedJunctions.clear();
    mySelectedEdges.clear();
    myToStoppingPlace = nullptr;
    myRoute = nullptr;
    // clear path
    myPath.clear();
    // update info route label
    updateInfoRouteLabel();
}


void
GNEM_PathCreator::recalculatePath() {
    // first clear path
    myPath.clear();
    // set edges
    std::vector<GNEEdge*> edges;
    // add route edges
    if (myRoute) {
        edges = myRoute->getParentEdges();
    } else {
        // add selected edges
        for (const auto& edge : mySelectedEdges) {
            edges.push_back(edge);
        }
        // add to stopping place edge
        if (myToStoppingPlace) {
            edges.push_back(myToStoppingPlace->getParentLanes().front()->getParentEdge());
        }
    }
    // fill paths
    if (edges.size() == 1) {
        myPath.push_back(Path(myVClass, edges.front()));
    } else {
        // add every segment
        for (int i = 1; i < (int)edges.size(); i++) {
            myPath.push_back(Path(myFrameParent->getViewNet(), myVClass, edges.at(i - 1), edges.at(i)));
        }
    }
}


void
GNEM_PathCreator::setSpecialCandidates(GNEEdge* originEdge) {
    // first calculate reachability for pedestrians (we use it, because pedestran can walk in almost all edges)
    myFrameParent->getViewNet()->getNet()->getPathManager()->getPathCalculator()->calculateReachability(SVC_PEDESTRIAN, originEdge);
    // change flags
    for (const auto& edge : myFrameParent->getViewNet()->getNet()->getAttributeCarriers()->getEdges()) {
        for (const auto& lane : edge.second->getLanes()) {
            if (lane->getReachability() > 0) {
                lane->getParentEdge()->resetCandidateFlags();
                lane->getParentEdge()->setSpecialCandidate(true);
            }
        }
    }
}

void
GNEM_PathCreator::setPossibleCandidates(GNEEdge* originEdge, const SUMOVehicleClass vClass) {
    // first calculate reachability for pedestrians
    myFrameParent->getViewNet()->getNet()->getPathManager()->getPathCalculator()->calculateReachability(vClass, originEdge);
    // change flags
    for (const auto& edge : myFrameParent->getViewNet()->getNet()->getAttributeCarriers()->getEdges()) {
        for (const auto& lane : edge.second->getLanes()) {
            if (lane->getReachability() > 0) {
                lane->getParentEdge()->resetCandidateFlags();
                lane->getParentEdge()->setPossibleCandidate(true);
            }
        }
    }
}

/****************************************************************************/
