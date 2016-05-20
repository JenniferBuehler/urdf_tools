/**
    Copyright (C) 2015 Jennifer Buehler

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**/

#include <urdf_traverser/Helpers.h>
#include <urdf_traverser/Functions.h>
#include <urdf_traverser/UrdfTraverser.h>
#include <urdf_traverser/PrintModel.h>
#include <urdf_traverser/JointNames.h>
#include <urdf_traverser/DependencyOrderedJoints.h>

#include <string>
#include <ros/ros.h>
#include <ros/package.h>

#include <map>
#include <vector>
#include <set>
#include <fstream>
#include <algorithm>

#define RAD_TO_DEG 180/M_PI

using urdf_traverser::UrdfTraverser;

std::string UrdfTraverser::getRootLinkName() const
{
    LinkConstPtr root = this->model->getRoot();
    if (!root)
    {
        ROS_ERROR("Loaded model has no root");
        return "";
    }
    return root->name;
}

bool UrdfTraverser::printModel(const std::string& fromLink, bool verbose)
{
    return urdf_traverser::printModel(*this,fromLink,verbose);
}

bool UrdfTraverser::printModel(bool verbose)
{
    return urdf_traverser::printModel(*this, verbose);
}


bool UrdfTraverser::getDependencyOrderedJoints(std::vector<JointPtr>& result,
        const JointPtr& from_joint, bool allowSplits, bool onlyActive)
{
    return urdf_traverser::getDependencyOrderedJoints(*this, result, from_joint, allowSplits, onlyActive);
}

bool UrdfTraverser::getDependencyOrderedJoints(std::vector<JointPtr>& result, const LinkPtr& from_link,
        bool allowSplits, bool onlyActive)
{
    return urdf_traverser::getDependencyOrderedJoints(*this, result, from_link->name, allowSplits, onlyActive);
}

    
bool UrdfTraverser::getJointNames(const std::string& fromLink,
            const bool skipFixed, std::vector<std::string>& result)
{
    std::string rootLink=fromLink;
    if (rootLink.empty()){
        rootLink = getRootLinkName();
    }

    // get root link
    LinkPtr root_link;
    this->model->getLink(rootLink, root_link);
    if (!root_link)
    {
        ROS_ERROR("no root link %s", fromLink.c_str());
        return false;
    }

    return urdf_traverser::getJointNames(*this, rootLink, skipFixed, result);
}

int UrdfTraverser::getChildJoint(const JointPtr& joint, JointPtr& child)
{
    LinkPtr childLink = getChildLink(joint);
    if (!childLink)
    {
        ROS_ERROR("Consistency: all joints must have child links");
        return -2;
    }
    if (childLink->child_joints.size() > 1)
    {
        return -1;
    }
    if (childLink->child_joints.empty())
    {   // this is the end link, and we've defined the end
        // frame to be at the same location as the last joint,
        // so no rotation should be needed?
        return 0;
    }
    // there must be only one joint
    child = childLink->child_joints.front();
    return 1;
}

urdf_traverser::LinkPtr UrdfTraverser::getChildLink(const JointPtr& joint)
{
    LinkPtr childLink;
    model->getLink(joint->child_link_name, childLink);
    return childLink;
}

urdf_traverser::LinkConstPtr UrdfTraverser::readChildLink(const JointPtr& joint) const
{
    LinkPtr childLink;
    model->getLink(joint->child_link_name, childLink);
    return childLink;
}

urdf_traverser::JointPtr UrdfTraverser::getParentJoint(const JointPtr& joint)
{
    LinkConstPtr parentLink = model->getLink(joint->parent_link_name);
    if (!parentLink) return JointPtr();
    return parentLink->parent_joint;
}

urdf_traverser::JointConstPtr UrdfTraverser::readParentJoint(const JointPtr& joint) const
{
    LinkConstPtr parentLink = model->getLink(joint->parent_link_name);
    if (!parentLink) return JointPtr();
    return parentLink->parent_joint;
}



bool UrdfTraverser::hasChildLink(const LinkConstPtr& link, const std::string& childName) const
{
    for (unsigned int i = 0; i < link->child_links.size(); ++i)
    {
        LinkPtr childLink = link->child_links[i];
        if (childLink->name == childName) return true;
    }
    return false;
}


int UrdfTraverser::traverseTreeTopDown(const std::string& linkName, boost::function< int(RecursionParamsPtr&)> link_cb,
                                      RecursionParamsPtr& params, bool includeLink)
{
    LinkPtr link=getLink(linkName);
    if (!link)
    {
        ROS_ERROR_STREAM("Could not get Link "<<linkName);
        return -1;
    }
    return traverseTreeTopDown(link, link_cb, params, includeLink, 0);
}

int UrdfTraverser::traverseTreeTopDown(const LinkPtr& link, boost::function< int(RecursionParamsPtr&)> link_cb,
                                      RecursionParamsPtr& params, bool includeLink, unsigned int level)
{
    if (includeLink)
    {
        params->setParams(link, level);
        int link_ret = link_cb(params);
        if (link_ret <= 0)
        {
            // stopping traversal
            return link_ret;
        }
    }

    level += 1;
    for (std::vector<LinkPtr>::const_iterator child = link->child_links.begin();
            child != link->child_links.end(); child++)
    {
        LinkPtr childLink = *child;
        if (childLink)
        {
            params->setParams(childLink, level);
            int link_ret = link_cb(params);
            if (link_ret <= 0)
            {
                // stopping traversal
                return link_ret;
            }

            // recurse down the tree
            int ret = traverseTreeTopDown(childLink, link_cb, params, false, level);
            if (ret < 0)
            {
                ROS_ERROR("Error parsing branch of %s", childLink->name.c_str());
                return -1;
            }
        }
        else
        {
            ROS_ERROR("root link: %s has a null child!", link->name.c_str());
            return false;
        }
    }
    return 1;
};





int UrdfTraverser::traverseTreeBottomUp(const std::string& linkName, boost::function< int(RecursionParamsPtr&)> link_cb,
                                      RecursionParamsPtr& params, bool includeLink)
{
    LinkPtr link=getLink(linkName);
    if (!link)
    {
        ROS_ERROR_STREAM("Could not get Link "<<linkName);
        return -1;
    }
    return traverseTreeBottomUp(link, link_cb, params, includeLink, 0);
}



int UrdfTraverser::traverseTreeBottomUp(const LinkPtr& link, boost::function<int(RecursionParamsPtr&)> link_cb,
                         RecursionParamsPtr& params, bool includeLink, unsigned int level)
{
    std::set<std::string> toTraverse;
    for (unsigned int i = 0; i < link->child_links.size(); ++i)
    {
        LinkPtr childLink = link->child_links[i];
        toTraverse.insert(childLink->name);
    }

    for (std::set<std::string>::iterator it = toTraverse.begin(); it != toTraverse.end(); ++it)
    {
        if (!hasChildLink(link,*it))
        {
            ROS_ERROR_STREAM("Consistency: Link "<<link->name<<" does not have child "<<*it<<" any more.");
            return -1;
        }
        
        LinkPtr childLink;
        this->model->getLink(*it, childLink);

        if (childLink)
        {
            // ROS_INFO("Traversal into child %s",childLink->name.c_str());
            // recurse down the tree
            int travRes=traverseTreeBottomUp(childLink, link_cb, params, true, level+1);
            if (travRes==0)
            {
                ROS_INFO("Stopping traversal at %s", childLink->name.c_str());
                return 0;
            }
            else if (travRes<0)
            {
                ROS_ERROR("Error parsing branch of %s", childLink->name.c_str());
                return -1;
            }
            // childLink = params->result;
        }
        else
        {
            ROS_ERROR("root link: %s has a null child!", link->name.c_str());
            return -1;
        }
    }

    if (!includeLink)
    {
        return 1;
    }
    // ROS_INFO("Callback for link %s",link->name.c_str());
    params->setParams(link, level);
    int cbRet=link_cb(params);
    if (cbRet < 0)
    {
        ROS_ERROR("Error parsing branch of %s", link->name.c_str());
        return -1;
    }
    return cbRet;
}


bool UrdfTraverser::allRotationsToAxis(const std::string& fromLinkName, const Eigen::Vector3d& axis)
{
    // ROS_INFO_STREAM("### Transforming all rotations starting from "<<fromLinkName<<" to axis "<<axis);
    std::string rootLink=fromLinkName;
    if (rootLink.empty()){
        rootLink = getRootLinkName();
    }
    LinkPtr from_link=getLink(rootLink);
    if (!from_link)
    {
        ROS_ERROR("Link %s does not exist", fromLinkName.c_str());
        return false;
    }

    Vector3RecursionParams * vp = new Vector3RecursionParams(axis);
    RecursionParamsPtr p(vp);

    // traverse top-down, but don't include the link itself, as the method allRotationsToAxis()
    // operates on the links parent joints.
    int travRet = this->traverseTreeTopDown(from_link,
            boost::bind(&UrdfTraverser::allRotationsToAxis, this, _1), p, false, 0);
    if (travRet <= 0)
    {
        ROS_ERROR("Recursion to align all rotation axes failed");
        return false;
    }
    return true;
}


int UrdfTraverser::allRotationsToAxis(RecursionParamsPtr& p)
{
    LinkPtr link = p->link;
    if (!link)
    {
        ROS_ERROR("allRotationsToAxis: NULL link passed");
        return -1;
    }
    
    Vector3RecursionParams::Ptr param = baselib_binding_ns::dynamic_pointer_cast<Vector3RecursionParams>(p);
    if (!param)
    {
        ROS_ERROR("Wrong recursion parameter type");
        return -1;
    }
    
    JointPtr joint = link->parent_joint;
    if (!joint)
    {
        ROS_INFO_STREAM("allRotationsToAxis: Joint for link "<<link->name<<" is NULL, so this must be the root joint");
        return 1;
    }
    
    Eigen::Vector3d axis=param->vec;

    Eigen::Quaterniond alignAxis;
    if (jointTransformForAxis(*joint, axis, alignAxis))
    {
        // ROS_INFO_STREAM("Transforming axis for joint "<<joint->name<<" with transform "<<alignAxis);
        urdf_traverser::applyTransform(joint, EigenTransform(alignAxis), false);
        // the link has to receive the inverse transorm, so it stays at the original position
        Eigen::Quaterniond alignAxisInv = alignAxis.inverse();
        urdf_traverser::applyTransform(link, EigenTransform(alignAxisInv), true);

        // now, we have to fix the child joint's (1st order child joints) transform
        // to correct for this transformation.
        for (std::vector<JointPtr>::iterator pj = link->child_joints.begin();
                pj != link->child_joints.end(); pj++)
        {
            urdf_traverser::applyTransform(*pj, EigenTransform(alignAxisInv), true);
        }

        // finally, set the rotation axis to the target
        joint->axis.x = axis.x();
        joint->axis.y = axis.y();
        joint->axis.z = axis.z();
    }

    // all good, indicate that recursion can continue
    return 1;
}


urdf_traverser::LinkPtr UrdfTraverser::getLink(const std::string& name)
{
    LinkPtr ptr;
    this->model->getLink(name, ptr);
    return ptr;
}

urdf_traverser::LinkConstPtr UrdfTraverser::readLink(const std::string& name) const
{
    LinkPtr ptr;
    this->model->getLink(name, ptr);
    return ptr;
}

urdf_traverser::JointPtr UrdfTraverser::getJoint(const std::string& name)
{
    JointPtr ptr;
    if (this->model->joints_.find(name) == this->model->joints_.end()) ptr.reset();
    else ptr = this->model->joints_.find(name)->second;
    return ptr;
}

urdf_traverser::JointConstPtr UrdfTraverser::readJoint(const std::string& name) const
{
    JointConstPtr ptr;
    if (this->model->joints_.find(name) == this->model->joints_.end()) ptr.reset();
    else ptr = this->model->joints_.find(name)->second;
    return ptr;
}


bool equalAxes(const Eigen::Vector3d& z1, const Eigen::Vector3d& z2, double tolerance)
{
    Eigen::Vector3d _z1=z1;
    Eigen::Vector3d _z2=z2;
    _z1.normalize();
    _z2.normalize();
    double dot = _z1.dot(_z2);
    return (std::fabs(dot - 1.0)) < tolerance;
}


bool UrdfTraverser::jointTransformForAxis(const urdf::Joint& joint,
        const Eigen::Vector3d& axis, Eigen::Quaterniond& rotation)
{
    Eigen::Vector3d rotAxis(joint.axis.x, joint.axis.y, joint.axis.z);
    rotAxis.normalize();
    // ROS_INFO_STREAM("Rotation axis for joint "<<joint.name<<": "<<rotAxis);
    if (equalAxes(rotAxis, axis, 1e-06)) return false;

    rotation = Eigen::Quaterniond::FromTwoVectors(rotAxis, axis);
    // ROS_WARN_STREAM("z alignment: "<<rotation);
    return true;
}

void UrdfTraverser::printJointNames(const std::string& fromLink) 
{
    std::vector<std::string> jointNames;
    if (!getJointNames(fromLink, false, jointNames))
    {
        ROS_WARN("Could not retrieve joint names to print on screen");
    }
    else
    {
        ROS_INFO_STREAM("Joint names starting from "<<fromLink<<":");
        for (int i=0; i<jointNames.size(); ++i) ROS_INFO_STREAM(jointNames[i]);
        ROS_INFO("---");
    }
}


bool UrdfTraverser::loadModelFromFile(const std::string& urdfFilename)
{
    std::string xml_file;
    if (!getModelFromFile(urdfFilename, xml_file))
    {
        ROS_ERROR("Could not load file");
        return false;
    }

    if (!loadModelFromXMLString(xml_file))
    {
        ROS_ERROR("Could not load file");
        return false;
    }
    return true;
}



bool UrdfTraverser::loadModelFromXMLString(const std::string& xmlString)
{
    bool success = model->initString(xmlString);
    if (!success)
    {
        ROS_ERROR("Could not load model from XML string");
        return false;
    }
    return true;
}

/*bool UrdfTraverser::loadModelFromParameterServer()
{
    if (!model->initParam("robot_description")) return false;
    isScaled = false;
    return true;
}*/


bool UrdfTraverser::getModelFromFile(const std::string& filename, std::string& xml_string) const
{
    std::fstream xml_file(filename.c_str(), std::fstream::in);
    if (xml_file.is_open())
    {
        while (xml_file.good())
        {
            std::string line;
            std::getline(xml_file, line);
            xml_string += (line + "\n");
        }
        xml_file.close();
        return true;
    }
    else
    {
        ROS_ERROR("Could not open file [%s] for parsing.", filename.c_str());
    }
    return false;
}

urdf_traverser::EigenTransform UrdfTraverser::getTransform(const LinkPtr& from_link,  const JointPtr& to_joint)
{
    LinkPtr link1 = from_link;
    LinkPtr link2;
    this->model->getLink(to_joint->child_link_name, link2);
    if (!link1 || !link2)
    {
        ROS_ERROR("Invalid joint specifications (%s, %s), first needs parent and second child",
                  link1->name.c_str(), link2->name.c_str());
    }
    return urdf_traverser::getTransform(link1, link2);
}


