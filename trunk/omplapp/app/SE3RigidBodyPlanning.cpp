#include "SE3RigidBodyPlanning.h"
#include "PQPStateValidityChecker.h"
#include <assimp.hpp>     
#include <aiScene.h>      
#include <aiPostProcess.h>
#include <limits>

void ompl::app::SE3RigidBodyPlanning::setMeshes(const std::string &robot, const std::string &env)
{
    Assimp::Importer importerR;
    const aiScene* robotScene = importerR.ReadFile(robot.c_str(),
						   aiProcess_Triangulate            |
						   aiProcess_JoinIdenticalVertices  |
						   aiProcess_SortByPType);
    std::vector<const aiMesh*> robotMesh;
    if (robotScene)
    {
	if (robotScene->HasMeshes())
	{
	    for (unsigned int i = 0 ; i < robotScene->mNumMeshes ; ++i)
		robotMesh.push_back(robotScene->mMeshes[i]);
	}
	else
	    msg_.error("There is no mesh specified in the indicated robot resource: %s", robot.c_str());
    }
    else
	msg_.error("Unable to load robot scene: %s", robot.c_str());
    
    std::vector<const aiMesh*> envMesh;
    if (!env.empty())
    {
	Assimp::Importer importerE;
	const aiScene* envScene = importerE.ReadFile(env.c_str(),
						     aiProcess_Triangulate            |
						     aiProcess_JoinIdenticalVertices  |
						     aiProcess_SortByPType);
	if (envScene)
	{
	    if (envScene->HasMeshes())
	    {
		for (unsigned int i = 0 ; i < envScene->mNumMeshes ; ++i)
		    envMesh.push_back(envScene->mMeshes[i]);
	    }
	    else
		msg_.error("There is no mesh specified in the indicated environment resource: %s", env.c_str());
	}
	else
	    msg_.error("Unable to load environment scene: %s", env.c_str());
	
	// infer some bounding box for state manifold if we have meshes and a valid bounding box has not been set
	if (envScene->HasMeshes())
	{
	    base::RealVectorBounds bounds = getStateManifold()->as<base::SE3StateManifold>()->getBounds();
	    
	    // if bounds are not valid
	    if (bounds.getVolume() < std::numeric_limits<double>::epsilon())
	    {
		double minX = std::numeric_limits<double>::infinity();
		double minY = minX;
		double minZ = minX;
		double maxX = -minX;
		double maxY = maxX;
		double maxZ = maxX;
		for (unsigned int j = 0 ; j < envScene->mNumMeshes ; ++j)
		{	    
		    const aiMesh *a = envScene->mMeshes[j];
		    for (unsigned int i = 0 ; i < a->mNumVertices ; ++i)
		    {
			if (minX > a->mVertices[i].x) minX = a->mVertices[i].x;
			if (maxX < a->mVertices[i].x) maxX = a->mVertices[i].x;
			if (minY > a->mVertices[i].y) minY = a->mVertices[i].y;
			if (maxY < a->mVertices[i].y) maxY = a->mVertices[i].y;
			if (minZ > a->mVertices[i].z) minZ = a->mVertices[i].z;
			if (maxZ < a->mVertices[i].z) maxZ = a->mVertices[i].z;
		    }
		}
		double dx = (maxX - minX) * 0.10;
		double dy = (maxY - minY) * 0.10;
		double dz = (maxZ - minZ) * 0.10;
		bounds.low[0] = minX - dx; bounds.low[1] = minY - dy; bounds.low[2] = minZ - dz;
		bounds.high[0] = maxX + dx; bounds.high[1] = maxY + dy; bounds.high[2] = maxZ + dz;
		getStateManifold()->as<base::SE3StateManifold>()->setBounds(bounds);
	    }
	}
    }
    
    setStateValidityChecker(base::StateValidityCheckerPtr(new PQPStateValidityChecker(getSpaceInformation(), robotMesh, envMesh)));
}

void ompl::app::SE3RigidBodyPlanning::setup(void)
{
    // update the bounds based on start states, if needed
    base::RealVectorBounds bounds = getStateManifold()->as<base::SE3StateManifold>()->getBounds();
    
    std::vector<const base::State*> states;
    getProblemDefinition()->getInputStates(states);

    double minX = std::numeric_limits<double>::infinity();
    double minY = minX;
    double minZ = minX;
    double maxX = -minX;
    double maxY = maxX;
    double maxZ = maxX;
    for (unsigned int i = 0 ; i < states.size() ; ++i)
    {
	double x = states[i]->as<base::SE3StateManifold::StateType>()->getX();
	double y = states[i]->as<base::SE3StateManifold::StateType>()->getY();
	double z = states[i]->as<base::SE3StateManifold::StateType>()->getZ();
	if (minX > x) minX = x;
	if (maxX < x) maxX = x;
	if (minY > y) minY = y;
	if (maxY < y) maxY = y;
	if (minZ > z) minZ = z;
	if (maxZ < z) maxZ = z;
    }
    double dx = (maxX - minX) * 0.10;
    double dy = (maxY - minY) * 0.10;
    double dz = (maxZ - minZ) * 0.10;
    
    if (bounds.low[0] > minX - dx) bounds.low[0] = minX - dx;
    if (bounds.low[1] > minY - dy) bounds.low[1] = minY - dy;
    if (bounds.low[2] > minZ - dz) bounds.low[2] = minZ - dz;

    if (bounds.high[0] < maxX + dx) bounds.high[0] = maxX + dx;
    if (bounds.high[1] < maxY + dy) bounds.high[1] = maxY + dy;
    if (bounds.high[2] < maxZ + dz) bounds.high[2] = maxZ + dz;
    
    geometric::SimpleSetup::setup();    
}