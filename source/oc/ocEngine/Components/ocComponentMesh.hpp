// Copyright (C) 2016 David Reid. See included LICENSE file.

#define OC_COMPONENT_MESH(pComponent) ((ocComponentMesh*)pComponent)
struct ocComponentMesh : public ocComponent
{
    ocGraphicsMesh* pMesh;          // Used as the source for the mesh object.
    ocGraphicsObject* pMeshObject;  // Initially set to NULL, and then initialized when the object is added to the world.
};

//
ocResult ocComponentMeshInit(ocWorldObject* pObject, ocComponentMesh* pComponent);

//
void ocComponentMeshUninit(ocComponentMesh* pComponent);

// Sets the mesh to use for the graphical representation.
//
// This will fail if the object is already in the world. To change the mesh of an object dynamically,
// you'll need to first remove the object from the world, call this function, and then re-add the object.
ocResult ocComponentMeshSetMesh(ocComponentMesh* pComponent, ocGraphicsMesh* pMesh);