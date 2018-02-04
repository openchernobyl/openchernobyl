// Copyright (C) 2018 David Reid. See included LICENSE file.

enum ocResourceType
{
    ocResourceType_Unknown,
    ocResourceType_Image,
    ocResourceType_Material,
    ocResourceType_Scene
};

struct ocResourceLoader
{
    ocFileSystem* pFS;
};

// Initializes the resource loader.
ocResult ocResourceLoaderInit(ocFileSystem* pFS, ocResourceLoader* pLoader);

// Uninitializes the resource loader.
void ocResourceLoaderUninit(ocResourceLoader* pLoader);

// Determines the type of resource to load.
//
// This will use the file path to determine the type, but when the path is ambiguous (such as .ocd files) it
// will look at the contents of the file.
ocResult ocResourceLoaderDetermineResourceType(ocResourceLoader* pLoader, const char* filePath, ocResourceType* pType);




///////////////////////////////////////////////////////////////////////////////
//
// Images
//
///////////////////////////////////////////////////////////////////////////////

struct ocImageData
{
    ocImageFormat format;
    uint32_t mipmapCount;
    ocMipmapInfo* pMipmaps;
    size_t imageDataSize;
    void* pImageData;

    // [Internal Use Only] Dynamically allocated data as a single allocation.
    //
    // Format:
    //   sizeof(ocMipmapInfo) * mipmapCount
    //   Raw image data of imageDataSize bytes
    void* _pPayload;
};

// Loads an image.
ocResult ocResourceLoaderLoadImage(ocResourceLoader* pLoader, const char* filePath, ocImageData* pData);

// Unloads an image.
void ocResourceLoaderUnloadImage(ocResourceLoader* pLoader, ocImageData* pData);




///////////////////////////////////////////////////////////////////////////////
//
// Scenes
//
///////////////////////////////////////////////////////////////////////////////

// NOTES:
// - All offsets are relative to the main payload pointer.
// - All structures need to map to the OCD format spec exactly because they are mapped to the original file data.

#define OC_SCENE_OBJECT_NONE    (~0UL)

struct ocSceneSubresource
{
    ocUInt64 pathOffset;
    ocUInt64 flags;
    ocUInt64 dataSize;
    ocUInt64 dataOffset;
};

struct ocSceneObject
{
    ocUInt64 nameOffset;

    // Hierarchy information. If any of these are -1 (~0) then it's considred NULL. For example, if parentIndex == ~0 it means
    // the object does not have a parent. Use OC_SCENE_OBJECT_NONE.
    ocUInt32 parentIndex;
    ocUInt32 firstChildIndex;
    ocUInt32 lastChildIndex;
    ocUInt32 prevSiblingIndex;
    ocUInt32 nextSiblingIndex;

    // Transformation information.
    float relativePositionX;
    float relativePositionY;
    float relativePoyitionZ;
    float relativeRotationX;
    float relativeRotationY;
    float relativeRotationZ;
    float relativeRotationW;
    float relativeScaleX;
    float relativeScaleY;
    float relativeScaleZ;

    // Components.
    ocUInt32 componentCount;
    ocUInt64 componentsOffset;
};

struct ocSceneObjectComponent
{
    ocUInt32 type;
    ocUInt32 padding0;
    ocUInt64 dataSize;
    ocUInt64 dataOffset;
};

struct ocSceneData
{
    ocUInt32 subresourceCount;
    ocSceneSubresource* pSubresources;  // An offset of pPayload.

    ocUInt32 objectCount;
    ocSceneObject* pObjects;            // An offset of pPayload.

    // Dynamically allocated data as a single allocation. This is the entire raw OCD file data verbatim.
    ocUInt8* pPayload;
};

// Loads a scene.
ocResult ocResourceLoaderLoadScene(ocResourceLoader* pLoader, const char* filePath, ocSceneData* pData);

// Unloads a scene.
void ocResourceLoaderUnloadScene(ocResourceLoader* pLoader, ocSceneData* pData);
