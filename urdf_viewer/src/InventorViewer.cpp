/**
 * <ORGANIZATION> = Jennifer Buehler 
 * <COPYRIGHT HOLDER> = Jennifer Buehler 
 * 
 * Copyright (c) 2016 Jennifer Buehler 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <ORGANIZATION> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ------------------------------------------------------------------------------
 **/
#include <urdf_viewer/InventorViewer.h>
#include <urdf2inventor/Helpers.h>

#include <Inventor/SoDB.h>  // for file reading
#include <Inventor/SoInput.h>   // for file reading
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/SoPickedPoint.h>

#include <Inventor/nodes/SoEventCallback.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedShape.h>
#include <Inventor/nodes/SoVertexProperty.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/details/SoFaceDetail.h>

#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>

#include <ros/ros.h>  // only needed for ROS prints, e.g. ROS_ERROR

using urdf_viewer::InventorViewer;

InventorViewer::InventorViewer(bool _faces_ccw):
    root(NULL), viewWindow(NULL), viewer(NULL),
    faces_ccw(_faces_ccw), initialized(false) {}


InventorViewer::InventorViewer(const InventorViewer& o):
    root(o.root), viewWindow(o.viewWindow), viewer(o.viewer),
    faces_ccw(o.faces_ccw) {}
InventorViewer::~InventorViewer()
{
//    SoQt::done();
    if (viewer)
    {
        delete viewer;
    }
//    root->unref();
}


void InventorViewer::init(const char * windowName, float bck_r, float bck_g, float bck_b)
{
    if (viewWindow)
    {
        ROS_ERROR("InventorViewer already initialized");
        return;
    }
    viewWindow = SoQt::init(windowName);
    viewer = new SoQtExaminerViewer(viewWindow);
    viewer->setBackgroundColor(SbColor(bck_r,bck_g,bck_b));
    root = new SoSelection();
    root->ref();

    SoEventCallback * ecb = new SoEventCallback();
    ecb->addEventCallback(SoMouseButtonEvent::getClassTypeId(), InventorViewer::mouseBtnCB, this);
    root->addChild(ecb);
    initialized = true;
}

void InventorViewer::loadModel(SoNode * model)
{
    if (!initialized)
    {
      ROS_ERROR("InventorViewer not initialized.");
      return;
    }
    if (model) root->addChild(model);
}

bool InventorViewer::loadModel(const std::string& filename)
{
    if (!initialized)
    {
      ROS_ERROR("InventorViewer not initialized.");
      return false;
    }
    SoInput in;
    SoNode  *model = NULL;
    if (!in.openFile(filename.c_str()))
        return false;
    if (!SoDB::read(&in, model) || model == NULL)
        /*model = SoDB::readAll(&in);
        if (!model)*/
        return false;

    root->addChild(model);
    in.closeFile();
    return true;
}

void InventorViewer::runViewer()
{
    if (!initialized)
    {
      ROS_ERROR("InventorViewer not initialized.");
      return;
    }
    viewer->setSceneGraph(root);
    viewer->show();

    SoQt::show(viewWindow);
    SoQt::mainLoop();
}


void printPath(const SoPath* p)
{
    for (int i = p->getLength() - 1; i >= 0;  --i)
    {
        SoNode * n = p->getNode(i);
        std::string name = n->getName().getString();
        ROS_INFO("Path[%i]: %s, type %s",i,name.c_str(),n->getTypeId().getName().getString());
    }
}

bool InventorViewer::computeCorrectFaceNormal(const SoPickedPoint * pick, bool ccw_face, Eigen::Vector3d& normal, int& shapeIdx)
{
    const SoDetail *pickDetail = pick->getDetail();
    if ((pickDetail != NULL) && (pickDetail->getTypeId() == SoFaceDetail::getClassTypeId()))
    {
        // Picked object is a face
        const SoFaceDetail * fd = dynamic_cast<const SoFaceDetail*>(pickDetail);
        if (!fd)
        {
            ROS_ERROR("Could not cast to face detail");
            return false;
        }

        // face index is always 0 with triangle strips
       // ROS_INFO_STREAM("Face index: "<<fd->getFaceIndex());

        if (fd->getNumPoints() < 3)
        {
            ROS_ERROR("Clicked on degenerate face, can't compute normal");
            return false;
        }
        /*else
        {
            ROS_INFO_STREAM("Clicked on face with "<<fd->getNumPoints()<<" points.");
        }*/       
        
        //ROS_INFO("Pick path:");
        //printPath(pick->getPath());

        /*SbVec3f pickNormal = pick->getNormal();
        //SbVec3f _normalObj=pick->getObjectNormal();
        float _x, _y, _z;
        pickNormal.getValue(_x, _y, _z);
        Eigen::Vector3d normalDef = Eigen::Vector3d(_x, _y, _z);
        normal = normalDef;*/

        // ROS_INFO_STREAM("Clicked on face with "<<fd->getNumPoints()<<" points.");

        int p1 = fd->getPoint(0)->getCoordinateIndex();
        int p2 = fd->getPoint(1)->getCoordinateIndex();
        int p3 = fd->getPoint(2)->getCoordinateIndex();

        // ROS_INFO_STREAM("Face part index: "<<fd->getPartIndex());

        // ROS_INFO_STREAM("First 3 coord indices: "<<p1<<", "<<p2<<", "<<p3);

        // Find the coordinate node that is used for the faces.
        // Assume that it's the last SoCoordinate3 node traversed
        // before the picked shape.
        SoSearchAction  searchCoords;
        searchCoords.setType(SoCoordinate3::getClassTypeId());
        searchCoords.setInterest(SoSearchAction::LAST);
        searchCoords.apply(pick->getPath());

        SbVec3f coord1, coord2, coord3;

        shapeIdx=pick->getPath()->getLength()-1;
        //ROS_INFO_STREAM("Len of pick path: "<<shapeIdx);

        if (searchCoords.getPath() == NULL)
        {
            // try to find SoIndexedShape instead
            // ROS_INFO("No SoCoordinate3 node found, looking for SoIndexedShape...");

            searchCoords.setType(SoIndexedShape::getClassTypeId());
            searchCoords.setInterest(SoSearchAction::LAST);
            searchCoords.apply(pick->getPath());

            if (searchCoords.getPath() == NULL)
            {
                ROS_ERROR("Failed to find coordinate node for the picked face. Returning default normal.");
                return false;
            }

            shapeIdx=searchCoords.getPath()->getLength()-1;
            // ROS_INFO_STREAM("Coords at Idx: "<<shapeIdx);

            // ROS_INFO("SearchCoords path:");
            // printPath(searchCoords.getPath());

            SoIndexedShape * vShapeNode = dynamic_cast<SoIndexedShape*>(searchCoords.getPath()->getTail());
            if (!vShapeNode)
            {
                ROS_ERROR("Could not cast SoIndexedShape");
                return false;
            }
            SoVertexProperty * vProp = dynamic_cast<SoVertexProperty*>(vShapeNode->vertexProperty.getValue());
            if (!vProp)
            {
                ROS_ERROR_STREAM("Could not cast SoVertexProperty.");
                return false;
            }
            coord1 = vProp->vertex[p1];
            coord2 = vProp->vertex[p2];
            coord3 = vProp->vertex[p3];
        }
        else
        {
            shapeIdx=searchCoords.getPath()->getLength()-1;
            
            SoCoordinate3 * coordNode = dynamic_cast<SoCoordinate3*>(searchCoords.getPath()->getTail());
            if (!coordNode)
            {
                ROS_ERROR("Could not cast SoCoordinate3");
                return false;
            }
            coord1 = coordNode->point[p1];
            coord2 = coordNode->point[p2];
            coord3 = coordNode->point[p3];
        }

        if (fd->getNumPoints() > 3)
        {
            ROS_WARN_STREAM("Face with " << fd->getNumPoints() <<
                            " points is not a triangle and may lead to wrong normal calculations.");
        }

        /*ROS_INFO_STREAM("Coords "<<p1<<", "<<p2<<", "<<p3);
        float _x, _y, _z;
        coord1.getValue(_x, _y, _z);
        ROS_INFO_STREAM("val1 "<<_x<<", "<<_y<<", "<<_z);
        coord2.getValue(_x, _y, _z);
        ROS_INFO_STREAM("val2 "<<_x<<", "<<_y<<", "<<_z);
        coord3.getValue(_x, _y, _z);
        ROS_INFO_STREAM("val3 "<<_x<<", "<<_y<<", "<<_z);*/

        SbVec3f diff1(coord2.getValue());
        diff1 -= coord1;
        SbVec3f diff2(coord3.getValue());
        diff2 -= coord1;
        SbVec3f cross = diff1.cross(diff2);
        if (!ccw_face) cross = -cross;

        float x, y, z;
        cross.getValue(x, y, z);
        double len = sqrt(x * x + y * y + z * z);
        x /= len;
        y /= len;
        z /= len;

        normal = Eigen::Vector3d(x, y, z);

        return true;
    }
    return false;
}


SoNode * InventorViewer::getIntStr(const std::string& sscanfStr, const SoPath * path, std::string& extStr, int& extNum, int& pathIdx)
{
    if (path->getLength()==0) return NULL;
    for (int i = path->getLength() - 1; i >= 0;  --i)
    {
        SoNode * n = path->getNode(i);
        std::string name = n->getName().getString();
        
        //ROS_INFO("Path[%i]: %s, type %s",i,name.c_str(),n->getTypeId().getName().getString());

        char ln[1000];
        int num;
        if (sscanf(name.c_str(), sscanfStr.c_str(), &num, ln) < 2) continue;
        // ROS_INFO("num: %i rest: %s\n",num,ln);
        extStr = ln; //urdf2inventor::helpers::getFilename(ln); // take only the name after the last '/'
        extNum = num;
        pathIdx = i;
        return n;
    }
    return NULL;
}


void InventorViewer::mouseBtnCB(void *userData, SoEventCallback *_pEvent)
{
    InventorViewer * obj = static_cast<InventorViewer*>(userData);
    if (!obj)
    {
        ROS_ERROR("Invalid UseData passed into mouseBtnCB");
        return;
    }

    const SoEvent  *pEvent  = _pEvent->getEvent();

    // general callback:
    obj->onMouseBtnClick(_pEvent);

    // also see whether part of the model was clicked:
    const SoQtViewer *pViewer = obj->viewer;

    if (SoMouseButtonEvent::isButtonPressEvent(pEvent, SoMouseButtonEvent::BUTTON1))
    {
        SoRayPickAction rayPick(pViewer->getViewportRegion());
        rayPick.setPoint(pEvent->getPosition());
        rayPick.setPickAll(false);
        // rayPick.setRadius(1.0);
        rayPick.apply(pViewer->getSceneManager()->getSceneGraph());
        const SoPickedPoint *pPickedPt = rayPick.getPickedPoint();
        if (pPickedPt != NULL)
        {
            obj->onClickModel(pPickedPt);

            // see if a URDF link was clicked:
            /*SoPath *pPickPath = pPickedPt->getPath();
            std::string linkName;
            int visualNum;
            SoNode * linkNode = getLinkDesc(pPickPath, linkName, visualNum);
            if (!linkNode)
            {
                ROS_INFO("Clicked on something other than a link");
                return;
            }
            float x, y, z;
            pPickedPt->getObjectPoint(linkNode).getValue(x, y, z);
            ROS_INFO_STREAM("Clicked on " << linkName << ", at pos " << x << ", " << y << ", " << z);*/
        }
    }
}
