// Copyright (C) 2018 David Reid. See included LICENSE file.

typedef ocComponent* (* ocCreateComponentProc)(ocEngineContext* pEngine, ocComponentType type, ocWorldObject* pObject, void* pUserData);
typedef void         (* ocDeleteComponentProc)(ocComponent* pComponent, void* pUserData);

struct ocComponentAllocatorInstance
{
    ocComponentType type;
    ocCreateComponentProc onCreate;
    ocDeleteComponentProc onDelete;
    void* pUserData;
};

struct ocComponentAllocator
{
    ocEngineContext* pEngine;
    ocComponentAllocatorInstance* pAllocators;
};

//
ocResult ocComponentAllocatorInit(ocEngineContext* pEngine, ocComponentAllocator* pAllocator);

//
void ocComponentAllocatorUninit(ocComponentAllocator* pAllocator);


// Registers an allocator for a specific type of component.
//
// This will return OC_INVALID_ARGS if an allocator for the specified type has already been registered.
ocResult ocComponentAllocatorRegister(ocComponentAllocator* pAllocator, ocComponentType type, ocCreateComponentProc onCreate, ocDeleteComponentProc onDelete, void* pUserData);


// Creates an instance of a component of the given type.
ocComponent* ocComponentAllocatorCreateComponent(ocComponentAllocator* pAllocator, ocComponentType type, ocWorldObject* pObject);

// Deletes an instance of a component.
void ocComponentAllocatorDeleteComponent(ocComponentAllocator* pAllocator, ocComponent* pComponent);