// Copyright (C) 2018 David Reid. See included LICENSE file.

OC_PRIVATE ocResult ocStreamWriterWriteOCDDataBlock(ocStreamWriter* pWriter, const ocOCDDataBlock &block)
{
    // Write nothing if the block is empty, but return successfully. Any block is allowed.
    if (block.dataSize == 0) {
        return OC_SUCCESS;
    }

    return ocStreamWriterWrite(pWriter, block.pData, block.dataSize, NULL);
}



///////////////////////////////////////////////////////////////////////////////
//
// ocOCDDataBlock
//
///////////////////////////////////////////////////////////////////////////////

ocResult ocOCDDataBlockInit(ocOCDDataBlock* pBlock)
{
    if (pBlock == NULL) {
        return OC_INVALID_ARGS;
    }

    ocZeroObject(pBlock);
    ocStreamWriterInit(&pBlock->pData, &pBlock->dataSize, pBlock);

    return OC_SUCCESS;
}

ocResult ocOCDDataBlockUninit(ocOCDDataBlock* pBlock)
{
    if (pBlock == NULL) {
        return OC_INVALID_ARGS;
    }

    ocFree(pBlock->pData);
    ocStreamWriterUninit(pBlock);

    return OC_SUCCESS;
}

ocResult ocOCDDataBlockWrite(ocOCDDataBlock* pBlock, const void* pData, ocSizeT dataSize, ocUInt64* pOffsetOut)
{
    if (pOffsetOut) {
        *pOffsetOut = 0;
    }

    if (pBlock == NULL || pData == NULL) {
        return OC_INVALID_ARGS;
    }

    if (pOffsetOut) {
        *pOffsetOut = pBlock->dataSize;
    }

    ocResult result = ocStreamWriterWrite(pBlock, pData, dataSize, NULL);
    if (result != OC_SUCCESS) {
        return result;
    }
    
    return result;
}

ocResult ocOCDDataBlockWriteString(ocOCDDataBlock* pBlock, const char* pString, ocUInt64* pOffsetOut)
{
    ocResult result = ocOCDDataBlockWrite(pBlock, pString, strlen(pString)+1, pOffsetOut);  // +1 to include the null terminator.
    if (result != OC_SUCCESS) {
        return result;
    }

    return ocOCDDataBlockWritePadding64(pBlock);
}

ocResult ocOCDDataBlockWritePadding64(ocOCDDataBlock* pBlock)
{
    ocUInt32 padding = 0;
    return ocOCDDataBlockWrite(pBlock, &padding, pBlock->dataSize & 0x7, NULL);
}



///////////////////////////////////////////////////////////////////////////////
//
// ocOCDImageBuilder
//
///////////////////////////////////////////////////////////////////////////////

ocResult ocOCDImageBuilderInit(ocImageFormat format, ocUInt32 width, ocUInt32 height, const void* pImageData, ocOCDImageBuilder* pBuilder)
{
    if (format == 0 || width == 0 || height == 0 || pImageData == NULL || pBuilder == NULL) {
        return OC_INVALID_ARGS;
    }

    ocZeroObject(pBuilder);

    pBuilder->format = format;
    ocStackInit(&pBuilder->mipmaps);
    ocOCDDataBlockInit(&pBuilder->imageDataBlock);

    return ocOCDImageBuilderAddNextMipmap(pBuilder, width, height, pImageData);
}

ocResult ocOCDImageBuilderUninit(ocOCDImageBuilder* pBuilder)
{
    if (pBuilder == NULL) {
        return OC_INVALID_ARGS;
    }

    ocOCDDataBlockUninit(&pBuilder->imageDataBlock);
    ocStackUninit(&pBuilder->mipmaps);

    return OC_SUCCESS;
}

ocResult ocOCDImageBuilderRender(ocOCDImageBuilder* pBuilder, ocStreamWriter* pWriter)
{
    if (pBuilder == NULL || pWriter == NULL) {
        return OC_INVALID_ARGS;
    }

    // OCD header.
    ocStreamWriterWrite<ocUInt32>(pWriter, OC_OCD_FOURCC);
    ocStreamWriterWrite<ocUInt32>(pWriter, OC_OCD_TYPE_ID_IMAGE);

    // IMG1 data.
    ocStreamWriterWrite<ocUInt32>(pWriter, pBuilder->format);
    ocStreamWriterWrite<ocUInt32>(pWriter, (ocUInt32)pBuilder->mipmaps.count);

    for (ocUInt32 iMipmap = 0; iMipmap < pBuilder->mipmaps.count; ++iMipmap) {
        ocStreamWriterWrite<ocUInt64>(pWriter, pBuilder->mipmaps.pItems[iMipmap].dataOffset);
        ocStreamWriterWrite<ocUInt64>(pWriter, pBuilder->mipmaps.pItems[iMipmap].dataSize);
        ocStreamWriterWrite<ocUInt32>(pWriter, pBuilder->mipmaps.pItems[iMipmap].width);
        ocStreamWriterWrite<ocUInt32>(pWriter, pBuilder->mipmaps.pItems[iMipmap].height);
    }

    ocStreamWriterWrite<ocUInt64>(pWriter, pBuilder->imageDataBlock.dataSize);
    ocStreamWriterWriteOCDDataBlock(pWriter, pBuilder->imageDataBlock);

    return OC_SUCCESS;
}

ocResult ocOCDImageBuilderAddNextMipmap(ocOCDImageBuilder* pBuilder, ocUInt32 width, ocUInt32 height, const void* pData)
{
    if (pBuilder == NULL || width == 0 || height == 0 || pData == NULL) {
        return OC_INVALID_ARGS;
    }

    // If we're adding the base level we don't need to do any size validation.
    if (pBuilder->mipmaps.count > 0) {
        ocUInt32 expectedWidth  = ocMax(1, pBuilder->mipmaps.pItems[pBuilder->mipmaps.count-1].width  >> 1);
        ocUInt32 expectedHeight = ocMax(1, pBuilder->mipmaps.pItems[pBuilder->mipmaps.count-1].height >> 1);
        if (width != expectedWidth || height != expectedHeight) {
            return OC_INVALID_ARGS;
        }
    }

    // If we get here it means the dimensions of the mipmap are correct. Now we just add the data to our data block.
    ocMipmapInfo mipmap;
    mipmap.width = width;
    mipmap.height = height;
    mipmap.dataSize = (ocUInt64)width * (ocUInt64)height * ocImageFormatBytesPerPixel(pBuilder->format);
    if (mipmap.dataSize > SIZE_MAX) {
        return OC_INVALID_ARGS;  // Too big.
    }

    ocResult result = ocOCDDataBlockWrite(&pBuilder->imageDataBlock, pData, (ocSizeT)mipmap.dataSize, &mipmap.dataOffset);
    if (result != OC_SUCCESS) {
        return result;
    }

    // Padding.
    result = ocOCDDataBlockWritePadding64(&pBuilder->imageDataBlock);
    if (result != OC_SUCCESS) {
        return result;
    }


    // Add the mipmap to the main list last.
    result = ocStackPush(&pBuilder->mipmaps, mipmap);
    if (result != OC_SUCCESS) {
        return result;
    }

    return OC_SUCCESS;
}

ocResult ocOCDImageBuilderGenerateMipmaps(ocOCDImageBuilder* pBuilder)
{
    if (pBuilder == NULL) {
        return OC_INVALID_ARGS;
    }

    // Need at least one prior mipmap.
    if (pBuilder->mipmaps.count == 0) {
        return OC_INVALID_ARGS;
    }

    for (;;) {
        ocUInt32 prevWidth  = pBuilder->mipmaps.pItems[pBuilder->mipmaps.count-1].width;
        ocUInt32 prevHeight = pBuilder->mipmaps.pItems[pBuilder->mipmaps.count-1].height;
        if (prevWidth == 1 && prevHeight == 1) {
            break;
        }

        void* pPrevData = ocOffsetPtr(pBuilder->imageDataBlock.pData, (ocSizeT)pBuilder->mipmaps.pItems[pBuilder->mipmaps.count-1].dataOffset);

        ocUInt32 nextWidth  = ocMax(1, prevWidth  >> 1);
        ocUInt32 nextHeight = ocMax(1, prevHeight >> 1);
        void* pNextData = ocMalloc(nextWidth * nextHeight * ocImageFormatBytesPerPixel(pBuilder->format));
        if (pNextData == NULL) {
            return OC_OUT_OF_MEMORY;
        }

        ocMipmapInfo mipmapInfo;
        ocResult result = ocGenerateMipmap(prevWidth, prevHeight, ocImageFormatComponentCount(pBuilder->format), pPrevData, pNextData, &mipmapInfo);
        if (result != OC_SUCCESS) {
            return result;
        }

        ocAssert(nextWidth  == mipmapInfo.width);
        ocAssert(nextHeight == mipmapInfo.height);
        
        result = ocOCDImageBuilderAddNextMipmap(pBuilder, nextWidth, nextHeight, pNextData);
        ocFree(pNextData);
        
        if (result != OC_SUCCESS) {
            return result;
        }
    }

    return OC_SUCCESS;
}




///////////////////////////////////////////////////////////////////////////////
//
// ocOCDSceneBuilder
//
///////////////////////////////////////////////////////////////////////////////

ocResult ocOCDSceneBuilderInit(ocOCDSceneBuilder* pBuilder)
{
    if (pBuilder == NULL) {
        return OC_INVALID_ARGS;
    }

    ocZeroObject(pBuilder);

    ocStackInit(&pBuilder->objectStack);
    ocStackInit(&pBuilder->subresources);
    ocStackInit(&pBuilder->objects);
    ocStackInit(&pBuilder->components);

    ocOCDDataBlockInit(&pBuilder->subresourceBlock);
    ocOCDDataBlockInit(&pBuilder->objectBlock);
    ocOCDDataBlockInit(&pBuilder->componentBlock);
    ocOCDDataBlockInit(&pBuilder->componentDataBlock);
    ocOCDDataBlockInit(&pBuilder->subresourceDataBlock);
    ocOCDDataBlockInit(&pBuilder->stringDataBlock);

    ocStackInit(&pBuilder->meshGroups);

    return OC_SUCCESS;
}

ocResult ocOCDSceneBuilderUninit(ocOCDSceneBuilder* pBuilder)
{
    if (pBuilder == NULL) {
        return OC_INVALID_ARGS;
    }

    ocStackUninit(&pBuilder->meshGroups);

    ocOCDDataBlockUninit(&pBuilder->stringDataBlock);
    ocOCDDataBlockUninit(&pBuilder->subresourceDataBlock);
    ocOCDDataBlockUninit(&pBuilder->componentDataBlock);
    ocOCDDataBlockUninit(&pBuilder->componentBlock);
    ocOCDDataBlockUninit(&pBuilder->objectBlock);
    ocOCDDataBlockUninit(&pBuilder->subresourceBlock);

    ocStackUninit(&pBuilder->components);
    ocStackUninit(&pBuilder->objects);
    ocStackUninit(&pBuilder->subresources);
    ocStackUninit(&pBuilder->objectStack);

    return OC_SUCCESS;
}

ocResult ocOCDSceneBuilderRender(ocOCDSceneBuilder* pBuilder, ocStreamWriter* pWriter)
{
    ocAssert(pBuilder != NULL);

    // Everything is written to an in-memory data block because we use a 2-pass algorithm which requires us to both read and write data which in turn means we need
    // access to the main data pointer, which ocOCDDataBlock provides.
    ocOCDDataBlock mainDataBlock;
    ocResult result = ocOCDDataBlockInit(&mainDataBlock);
    if (result != OC_SUCCESS) {
        return result;
    }

    // OCD header.
    ocStreamWriterWrite<ocUInt32>(&mainDataBlock, OC_OCD_FOURCC);
    ocStreamWriterWrite<ocUInt32>(&mainDataBlock, OC_OCD_TYPE_ID_SCENE);

    // SCN1 data (main data).
    //
    // At this point, all data offsets will be relative to the data blocks. We need to do rendering in two passes. The first pass just dumps each block to the main
    // stream, keeping track of their data offsets. The second pass updates each of the relative data offsets to absolute offsets.

    // Pass 1
    // ======

    // Header. Offsets are relative to the main payload chunk and updated in the second pass.
    ocUInt64 headerOffset;
    result = ocStreamWriterTell(&mainDataBlock, &headerOffset);
    if (result != OC_SUCCESS) {
        return result;
    }

    ocStreamWriterWrite<ocUInt32>(&mainDataBlock, (ocUInt32)pBuilder->subresources.count);
    ocStreamWriterWrite<ocUInt32>(&mainDataBlock, (ocUInt32)pBuilder->objects.count);
    ocStreamWriterWrite<ocUInt64>(&mainDataBlock, 0);
    ocStreamWriterWrite<ocUInt64>(&mainDataBlock, 0);
    ocStreamWriterWrite<ocUInt64>(&mainDataBlock, 0);      // Payload size. Updated in the second pass when the size is known for real.
    
    // Payload.
    ocUInt64 payloadOffset;
    result = ocStreamWriterTell(&mainDataBlock, &payloadOffset);
    if (result != OC_SUCCESS) {
        return result;
    }

    // Subresources.
    ocUInt64 subresourcesOffset;
    result = ocStreamWriterTell(&mainDataBlock, &subresourcesOffset);
    if (result != OC_SUCCESS) {
        return result;
    }
    {
        result = ocStreamWriterWriteOCDDataBlock(&mainDataBlock, pBuilder->subresourceBlock);
        if (result != OC_SUCCESS) {
            return result;
        }
    }


    // Objects.
    for (ocUInt32 iObject = 0; iObject < pBuilder->objects.count; ++iObject) {
        result = ocOCDDataBlockWrite(&pBuilder->objectBlock, pBuilder->objects.pItems[iObject]);
        if (result != OC_SUCCESS) {
            return result;
        }
    }

    ocUInt64 objectsOffset;
    result = ocStreamWriterTell(&mainDataBlock, &objectsOffset);
    if (result != OC_SUCCESS) {
        return result;
    }
    {
        result = ocStreamWriterWriteOCDDataBlock(&mainDataBlock, pBuilder->objectBlock);
        if (result != OC_SUCCESS) {
            return result;
        }
    }


    // Components.
    ocUInt64 componentsOffset;
    result = ocStreamWriterTell(&mainDataBlock, &componentsOffset);
    if (result != OC_SUCCESS) {
        return result;
    }
    {
        result = ocStreamWriterWriteOCDDataBlock(&mainDataBlock, pBuilder->componentBlock);
        if (result != OC_SUCCESS) {
            return result;
        }
    }

    // Component Data.
    ocUInt64 componentDataOffset;
    result = ocStreamWriterTell(&mainDataBlock, &componentDataOffset);
    if (result != OC_SUCCESS) {
        return result;
    }
    {
        result = ocStreamWriterWriteOCDDataBlock(&mainDataBlock, pBuilder->componentDataBlock);
        if (result != OC_SUCCESS) {
            return result;
        }
    }

    // Subresource Data.
    ocUInt64 subresourceDataOffset;
    result = ocStreamWriterTell(&mainDataBlock, &subresourceDataOffset);
    if (result != OC_SUCCESS) {
        return result;
    }
    {
        result = ocStreamWriterWriteOCDDataBlock(&mainDataBlock, pBuilder->subresourceDataBlock);
        if (result != OC_SUCCESS) {
            return result;
        }
    }

    // String Data.
    ocUInt64 stringDataOffset;
    result = ocStreamWriterTell(&mainDataBlock, &stringDataOffset);
    if (result != OC_SUCCESS) {
        return result;
    }
    {
        result = ocStreamWriterWriteOCDDataBlock(&mainDataBlock, pBuilder->stringDataBlock);
        if (result != OC_SUCCESS) {
            return result;
        }
    }



    // Pass 2
    // ======

    // Header. Offsets are relative to the main payload chunk.
    result = ocStreamWriterSeek(&mainDataBlock, (ocInt64)(headerOffset + sizeof(ocUInt32) + sizeof(ocUInt32)), ocSeekOrigin_Start);
    if (result != OC_SUCCESS) {
        return result;
    }

    ocStreamWriterWrite<ocUInt64>(&mainDataBlock, subresourcesOffset);
    ocStreamWriterWrite<ocUInt64>(&mainDataBlock, objectsOffset);

    ocUInt64 payloadSize;
    result = ocStreamWriterSize(&mainDataBlock, &payloadSize);
    if (result != OC_SUCCESS) {
        return result;
    }
    ocStreamWriterWrite<ocUInt64>(&mainDataBlock, payloadSize);


    // Subresources.
    ocOCDSceneBuilderSubresource* pSubresources = (ocOCDSceneBuilderSubresource*)ocOffsetPtr(mainDataBlock.pData, (ocSizeT)subresourcesOffset);
    for (ocUInt64 iSubresource = 0; iSubresource < pBuilder->subresources.count; ++iSubresource) {
        pSubresources[iSubresource].pathOffset += stringDataOffset;
        pSubresources[iSubresource].dataOffset += subresourceDataOffset;
    }

    // Objects.
    ocOCDSceneBuilderObject* pObjects = (ocOCDSceneBuilderObject*)ocOffsetPtr(mainDataBlock.pData, (ocSizeT)objectsOffset);
    for (ocUInt64 iObject = 0; iObject < pBuilder->objects.count; ++iObject) {
        pObjects[iObject].nameOffset += stringDataOffset;
        pObjects[iObject].componentsOffset += componentsOffset;
    }

    // Components.
    ocOCDSceneBuilderComponent* pComponents = (ocOCDSceneBuilderComponent*)ocOffsetPtr(mainDataBlock.pData, (ocSizeT)componentsOffset);
    for (ocUInt64 iComponent = 0; iComponent < pBuilder->components.count; ++iComponent) {
        pComponents[iComponent].dataOffset += componentDataOffset;
    }



    // The last thing to do is write our in-memory data block to the main stream.
    result = ocStreamWriterWriteOCDDataBlock(pWriter, mainDataBlock);
    if (result != OC_SUCCESS) {
        return result;
    }

    return OC_SUCCESS;
}

ocResult ocOCDSceneBuilderAddSubresource(ocOCDSceneBuilder* pBuilder, const char* path, ocUInt32* pIndex)
{
    if (pBuilder == NULL || path == NULL) {
        return OC_INVALID_ARGS;
    }

    // Check if the subresource already exists. If so, reuse it.
    for (ocUInt32 iSubresource = 0; iSubresource < (ocUInt32)pBuilder->subresources.count; ++iSubresource) {
        ocOCDSceneBuilderSubresource* pSubresource = &pBuilder->subresources.pItems[iSubresource];
        if (!(pSubresource->flags & OC_OCD_SCENE_SUBRESOURCE_FLAG_IS_INTERNAL)) {
            const char* existingSubresourcePath = (const char*)ocOffsetPtr(pBuilder->stringDataBlock.pData, (ocSizeT)pSubresource->pathOffset);
            if (strcmp(existingSubresourcePath, path) == 0) {
                if (pIndex) *pIndex = iSubresource;
                return OC_SUCCESS;
            }
        }
    }

    // If we get here it means the subresource does not already exist and thus needs to be appended.
    ocOCDSceneBuilderSubresource newSubresource;
    ocZeroObject(&newSubresource);
    
    ocResult result = ocOCDDataBlockWriteString(&pBuilder->stringDataBlock, path, &newSubresource.pathOffset);
    if (result != OC_SUCCESS) {
        return result;
    }

    result = ocStackPush(&pBuilder->subresources, newSubresource);
    if (result != OC_SUCCESS) {
        return result;
    }


    // Add the subresource to the data block.
    result = ocOCDDataBlockWrite(&pBuilder->subresourceBlock, newSubresource);
    if (result != OC_SUCCESS) {
        return result;
    }


    if (pIndex) {
        *pIndex = (ocUInt32)(pBuilder->subresources.count-1);
    }

    return OC_SUCCESS;
}

ocResult ocOCDSceneBuilderBeginObject(ocOCDSceneBuilder* pBuilder, const char* name, const glm::vec3 &absolutePosition, const glm::quat &absoluteRotation, const glm::vec3 &absoluteScale)
{
    if (pBuilder == NULL) {
        return OC_INVALID_ARGS;
    }

    ocResult result = OC_SUCCESS;

    ocOCDSceneBuilderObject object;
    ocZeroObject(&object);

    ocUInt32 newObjectIndex = (ocUInt32)pBuilder->objects.count;

    // Name.
    result = ocOCDDataBlockWriteString(&pBuilder->stringDataBlock, name, &object.nameOffset);
    if (result != OC_SUCCESS) {
        return result;
    }

    // Hierarchy.
    object.parentIndex      = OC_SCENE_OBJECT_NONE;
    object.firstChildIndex  = OC_SCENE_OBJECT_NONE;
    object.lastChildIndex   = OC_SCENE_OBJECT_NONE;
    object.prevSiblingIndex = OC_SCENE_OBJECT_NONE;
    object.nextSiblingIndex = OC_SCENE_OBJECT_NONE;

    if (pBuilder->objectStack.count > 0) {
        object.parentIndex = pBuilder->objectStack.pItems[pBuilder->objectStack.count - 1];

        // The new object's previous sibling is the last child of the parent.
        object.prevSiblingIndex = pBuilder->objects.pItems[object.parentIndex].lastChildIndex;

        // The parent's first and last children need to be updated.
        pBuilder->objects.pItems[object.parentIndex].lastChildIndex = newObjectIndex;
        if (pBuilder->objects.pItems[object.parentIndex].firstChildIndex == OC_SCENE_OBJECT_NONE) {
            pBuilder->objects.pItems[object.parentIndex].firstChildIndex = newObjectIndex;
        }
    } else {
        for (ocUInt32 i = (ocUInt32)pBuilder->objects.count; i > 0; --i) {
            if (pBuilder->objects.pItems[i-1].parentIndex == OC_SCENE_OBJECT_NONE) {
                object.prevSiblingIndex = i-1;
                break;
            }
        }
    }

    if (object.prevSiblingIndex != OC_SCENE_OBJECT_NONE) {
        pBuilder->objects.pItems[object.prevSiblingIndex].nextSiblingIndex = newObjectIndex;
    }


    // Transformation.
    object.absolutePosition = absolutePosition;
    object.absoluteRotation = absoluteRotation;
    object.absoluteScale    = absoluteScale;



    // Add the object to the stack.
    result = ocStackPush(&pBuilder->objectStack, newObjectIndex);
    if (result != OC_SUCCESS) {
        return result;
    }

    // Add the object to the main list.
    result = ocStackPush(&pBuilder->objects, object);
    if (result != OC_SUCCESS) {
        return result;
    }

    return OC_SUCCESS;
}

ocResult ocOCDSceneBuilderEndObject(ocOCDSceneBuilder* pBuilder)
{
    if (pBuilder == NULL) {
        return OC_INVALID_ARGS;
    }

    if (pBuilder->objectStack.count == 0) {
        return OC_INVALID_OPERATION;
    }

    ocResult result = ocStackPop(&pBuilder->objectStack);
    if (result != OC_SUCCESS) {
        return result;
    }

    return OC_SUCCESS;
}

OC_PRIVATE ocResult ocOCDSceneBuilder_AddComponent(ocOCDSceneBuilder* pBuilder, const ocOCDSceneBuilderComponent &component)
{
    ocAssert(pBuilder != NULL);
    ocAssert(pBuilder->objectStack.count > 0);

    // Important Note:
    //
    // The components of an object are stored in the Components block in tightly packed linear order. There can be no gaps in between the
    // components for any single object. This means that components must be added to an object all at once without any changes to the
    // hierarchy in between those component attachments. For example, this is not allowed:
    //
    // - Add Parent Object
    //   - Add Parent Component 1
    //   - Add Child Object
    //     - Add Child Component 1
    //   - Add Parent Component 2
    //
    // In the above example there will be gaps in between the components of the parent object. We need to do a validation check here to
    // guard against this situation, which should be rare in practice, but still possible. To check this we can simply compare the number
    // of components of the current object to the size of the components buffer to determine whether or not any extra components have been
    // added for different objects.
    ocOCDSceneBuilderObject &object = pBuilder->objects.pItems[pBuilder->objectStack.pItems[pBuilder->objectStack.count-1]];
    
    ocUInt32 componentsInObject = object.componentCount;
    if (componentsInObject > 0) {
        ocUInt64 componentBlockSize;
        ocResult result = ocStreamWriterSize(&pBuilder->componentBlock, &componentBlockSize);
        if (result != OC_SUCCESS) {
            return result;
        }

        ocUInt32 componentsInBlock = (ocUInt32)((componentBlockSize - object.componentsOffset) / sizeof(ocOCDSceneBuilderComponent));
        if (componentsInBlock != componentsInObject) {
            // The aforementioned validation check failed.
            ocAssert(OC_FALSE);
            return result;
        }
    }


    // First add the component to the Components data block.
    ocUInt64 componentOffset;
    ocResult result = ocOCDDataBlockWrite(&pBuilder->componentBlock, component, &componentOffset);
    if (result != OC_SUCCESS) {
        return result;
    }

    object.componentCount += 1;

    // If it's the first component attached to the object, make sure the offset is set. The way components work is that each object will
    // have each of it's components listed in a tightly packed group.
    if (object.componentCount == 1) {
        object.componentsOffset = componentOffset;
    }


    // The component needs to be added to the master list.
    result = ocStackPush(&pBuilder->components, component);
    if (result != OC_SUCCESS) {
        return OC_SUCCESS;
    }

    return OC_SUCCESS;
}

ocResult ocOCDSceneBuilderAddSceneComponent(ocOCDSceneBuilder* pBuilder, const char* path)
{
    if (pBuilder == NULL) {
        return OC_INVALID_ARGS;
    }

    if (pBuilder->objectStack.count == 0) {
        return OC_INVALID_OPERATION;
    }

    // The first thing to do is add the subresource. Then we just add the component to the object.
    ocUInt32 subresourceIndex;
    ocResult result = ocOCDSceneBuilderAddSubresource(pBuilder, path, &subresourceIndex);
    if (result != OC_SUCCESS) {
        return result;
    }

    // The component data is simple for scenes - it's just an index to the subresource followed by 4 bytes of 0 padding.
    ocUInt64 componentDataOffset;
    result = ocStreamWriterTell(&pBuilder->componentDataBlock, &componentDataOffset);
    if (result != OC_SUCCESS) {
        return result;
    }

    result = ocOCDDataBlockWrite<ocUInt32>(&pBuilder->componentDataBlock, subresourceIndex);
    if (result != OC_SUCCESS) {
        return result;
    }

    result = ocOCDDataBlockWrite<ocUInt32>(&pBuilder->componentDataBlock, 0);
    if (result != OC_SUCCESS) {
        return result;
    }

    ocUInt64 componentDataOffsetEnd;
    result = ocStreamWriterTell(&pBuilder->componentDataBlock, &componentDataOffsetEnd);
    if (result != OC_SUCCESS) {
        return result;
    }

    
    // Now just attach the component to the object.
    ocOCDSceneBuilderComponent component;
    ocZeroObject(&component);
    component.type       = OC_COMPONENT_TYPE_SCENE;
    component.dataSize   = componentDataOffsetEnd - componentDataOffset;
    component.dataOffset = componentDataOffset;
    result = ocOCDSceneBuilder_AddComponent(pBuilder, component);
    if (result != OC_SUCCESS) {
        return result;
    }

    return OC_SUCCESS;
}


ocResult ocOCDSceneBuilderBeginMeshComponent(ocOCDSceneBuilder* pBuilder)
{
    if (pBuilder == NULL || pBuilder->isAddingMeshComponent == OC_TRUE) {
        return OC_INVALID_ARGS;
    }

    if (pBuilder->objectStack.count == 0) {
        return OC_INVALID_OPERATION;
    }

    ocStackClear(&pBuilder->meshGroups);

    ocResult result = ocOCDDataBlockInit(&pBuilder->meshGroupVertexDataBlock);
    if (result != OC_SUCCESS) {
        return result;
    }

    result = ocOCDDataBlockInit(&pBuilder->meshGroupIndexDataBlock);
    if (result != OC_SUCCESS) {
        return result;
    }


    pBuilder->isAddingMeshComponent = OC_TRUE;
    return OC_SUCCESS;
}

ocResult ocOCDSceneBuilderEndMeshComponent(ocOCDSceneBuilder* pBuilder)
{
    if (pBuilder == NULL || pBuilder->isAddingMeshComponent == OC_FALSE) {
        return OC_INVALID_ARGS;
    }

    if (pBuilder->objectStack.count == 0) {
        return OC_INVALID_OPERATION;
    }

    ocResult result;
    ocUInt32 groupCount;
    ocUInt64 vertexDataOffset;
    ocUInt64 indexDataOffset;

    // We need the offset of this data block for later.
    ocUInt64 componentDataOffset;
    result = ocStreamWriterTell(&pBuilder->componentDataBlock, &componentDataOffset);
    if (result != OC_SUCCESS) {
        return result;
    }



    // Before writing the vertex and index data, we need to make sure they are padded for alignment.
    ocUInt64 vertexDataSize;
    result = ocStreamWriterSize(&pBuilder->meshGroupVertexDataBlock, &vertexDataSize);
    if (result != OC_SUCCESS) {
        goto done;
    }

    result = ocOCDDataBlockWritePadding64(&pBuilder->meshGroupVertexDataBlock);
    if (result != OC_SUCCESS) {
        goto done;
    }

    ocUInt64 vertexDataSizeWithPadding;
    result = ocStreamWriterSize(&pBuilder->meshGroupVertexDataBlock, &vertexDataSizeWithPadding);
    if (result != OC_SUCCESS) {
        goto done;
    }


    ocUInt64 indexDataSize;
    result = ocStreamWriterSize(&pBuilder->meshGroupIndexDataBlock, &indexDataSize);
    if (result != OC_SUCCESS) {
        goto done;
    }

    result = ocOCDDataBlockWritePadding64(&pBuilder->meshGroupIndexDataBlock);
    if (result != OC_SUCCESS) {
        goto done;
    }

    //ocUInt64 indexDataSizeWithPadding;
    //result = ocStreamWriterSize(&pBuilder->meshGroupIndexDataBlock, &indexDataSizeWithPadding);
    //if (result != OC_SUCCESS) {
    //    goto done;
    //}



    // Group count.
    groupCount = (ocUInt32)pBuilder->meshGroups.count;
    result = ocOCDDataBlockWrite<ocUInt32>(&pBuilder->componentDataBlock, groupCount);
    if (result != OC_SUCCESS) {
        goto done;
    }

    // Padding.
    result = ocOCDDataBlockWrite<ocUInt32>(&pBuilder->componentDataBlock, 0);
    if (result != OC_SUCCESS) {
        goto done;
    }

    // Vertex data size.
    result = ocOCDDataBlockWrite<ocUInt64>(&pBuilder->componentDataBlock, vertexDataSize);
    if (result != OC_SUCCESS) {
        goto done;
    }

    // Vertex data offset. Vertex data is located past the groups.
    vertexDataOffset = 40 + (groupCount * sizeof(ocOCDSceneBuilderMeshGroup)); // 40 = size of the header section.
    result = ocOCDDataBlockWrite<ocUInt64>(&pBuilder->componentDataBlock, vertexDataOffset);
    if (result != OC_SUCCESS) {
        goto done;
    }

    // Index data size.
    result = ocOCDDataBlockWrite<ocUInt64>(&pBuilder->componentDataBlock, indexDataSize);
    if (result != OC_SUCCESS) {
        goto done;
    }

    // Index data offset. Index data is located after the vertex data.
    indexDataOffset = vertexDataOffset + vertexDataSizeWithPadding;
    result = ocOCDDataBlockWrite<ocUInt64>(&pBuilder->componentDataBlock, indexDataOffset);
    if (result != OC_SUCCESS) {
        goto done;
    }

    // Groups.
    for (ocUInt32 iGroup = 0; iGroup < groupCount; ++iGroup) {
        result = ocOCDDataBlockWrite(&pBuilder->componentDataBlock, pBuilder->meshGroups.pItems[iGroup]);
        if (result != OC_SUCCESS) {
            goto done;
        }
    }

    // Vertex data.
    result = ocStreamWriterWriteOCDDataBlock(&pBuilder->componentDataBlock, pBuilder->meshGroupVertexDataBlock);
    if (result != OC_SUCCESS) {
        goto done;
    }

    // Index data.
    result = ocStreamWriterWriteOCDDataBlock(&pBuilder->componentDataBlock, pBuilder->meshGroupIndexDataBlock);
    if (result != OC_SUCCESS) {
        goto done;
    }


    ocUInt64 componentDataOffsetEnd;
    result = ocStreamWriterTell(&pBuilder->componentDataBlock, &componentDataOffsetEnd);
    if (result != OC_SUCCESS) {
        return result;
    }


    // Now just attach the component to the object.
    ocOCDSceneBuilderComponent component;
    ocZeroObject(&component);
    component.type       = OC_COMPONENT_TYPE_MESH;
    component.dataSize   = componentDataOffsetEnd - componentDataOffset;
    component.dataOffset = componentDataOffset;
    result = ocOCDSceneBuilder_AddComponent(pBuilder, component);
    if (result != OC_SUCCESS) {
        return result;
    }

done:
    ocOCDDataBlockUninit(&pBuilder->meshGroupVertexDataBlock);
    ocOCDDataBlockUninit(&pBuilder->meshGroupIndexDataBlock);
    pBuilder->isAddingMeshComponent = OC_FALSE;
    return result;
}

ocResult ocOCDSceneBuilderMeshComponentAddGroup(ocOCDSceneBuilder* pBuilder, const char* materialPath, ocGraphicsPrimitiveType primitiveType, ocGraphicsVertexFormat vertexFormat, ocUInt32 vertexCount, float* pVertexData, ocGraphicsIndexFormat indexFormat, ocUInt32 indexCount, void* pIndexData)
{
    if (pBuilder == NULL || pVertexData == NULL || pIndexData == NULL) {
        return OC_INVALID_ARGS;
    }

    ocOCDSceneBuilderMeshGroup meshGroup;
    ocResult result = ocOCDSceneBuilderAddSubresource(pBuilder, materialPath, &meshGroup.materialSubresourceIndex);
    if (result != OC_SUCCESS) {
        return result;
    }

    meshGroup.primitiveType = (ocUInt32)primitiveType;
    
    meshGroup.vertexFormat = (ocUInt32)vertexFormat;
    meshGroup.vertexCount = vertexCount;
    result = ocOCDDataBlockWrite(&pBuilder->meshGroupVertexDataBlock, pVertexData, ocGetVertexSizeFromFormat((ocGraphicsVertexFormat)meshGroup.vertexFormat) * vertexCount, &meshGroup.vertexDataOffset);
    if (result != OC_SUCCESS) {
        return result;
    }

    meshGroup.indexFormat = (ocUInt32)indexFormat;
    meshGroup.indexCount = indexCount;
    result = ocOCDDataBlockWrite(&pBuilder->meshGroupIndexDataBlock, pIndexData, ocGetIndexSizeFromFormat((ocGraphicsIndexFormat)meshGroup.indexFormat) * indexCount, &meshGroup.indexDataOffset);
    if (result != OC_SUCCESS) {
        return result;
    }

    result = ocStackPush(&pBuilder->meshGroups, meshGroup);
    if (result != OC_SUCCESS) {
        return result;
    }

    return OC_SUCCESS;
}

ocResult ocOCDSceneBuilderMeshComponentAddGroup(ocOCDSceneBuilder* pBuilder, const char* materialPath, ocGraphicsPrimitiveType primitiveType, ocGraphicsVertexFormat vertexFormat, ocUInt32 vertexCount, float* pVertexData, ocUInt32 indexCount, ocUInt32* pIndexData)
{
    return ocOCDSceneBuilderMeshComponentAddGroup(pBuilder, materialPath, primitiveType, vertexFormat, vertexCount, pVertexData, ocGraphicsIndexFormat_UInt32, indexCount, pIndexData);
}

ocResult ocOCDSceneBuilderMeshComponentAddGroup(ocOCDSceneBuilder* pBuilder, const char* materialPath, ocGraphicsPrimitiveType primitiveType, ocGraphicsVertexFormat vertexFormat, ocUInt32 vertexCount, float* pVertexData, ocUInt32 indexCount, ocUInt16* pIndexData)
{
    return ocOCDSceneBuilderMeshComponentAddGroup(pBuilder, materialPath, primitiveType, vertexFormat, vertexCount, pVertexData, ocGraphicsIndexFormat_UInt16, indexCount, pIndexData);
}
