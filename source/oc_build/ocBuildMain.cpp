// Copyright (C) 2018 David Reid. See included LICENSE file.

#include "../oc/ocEngine/ocEngine.cpp"
#include "../external/json/json.c"

#define OC_BUILD_INTERMEDIATE_DIRECTORY "build/oc_build/bin/out"

struct ocBuildGraphicsContext;

struct ocBuildRenderPassAttachment
{
    const char* name;
    const char* format;
    const char* loadOp;
    const char* storeOp;
    const char* stencilLoadOp;
    const char* stencilStoreOp;
    const char* initialLayout;
    const char* finalLayout;
};

struct ocBuildRenderPassAttachmentReference
{
    const char* attachment;
    const char* layout;
};

struct ocBuildRenderPassSubpass
{
    const char* name;
    std::vector<ocBuildRenderPassAttachmentReference> inputAttachments;
    std::vector<ocBuildRenderPassAttachmentReference> colorAttachments;
    std::vector<ocBuildRenderPassAttachmentReference> resolveAttachments;
    std::vector<ocBuildRenderPassAttachmentReference> depthStencilAttachments;
};

struct ocBuildRenderPassSubpassDependency
{
    const char* srcSubpass;
    const char* dstSubpass;
    const char* srcStageMask;
    const char* dstStageMask;
    const char* srcAccessMask;
    const char* dstAccessMask;
    const char* dependencyFlags;
};

struct ocBuildRenderPass
{
    struct json_value_s* pJSONRoot;
    const char* name;
    std::vector<ocBuildRenderPassAttachment> attachments;
    std::vector<ocBuildRenderPassSubpass> subpasses;
    std::vector<ocBuildRenderPassSubpassDependency> dependencies;
};

#include "ocBuildUtils.hpp"
#include "ocBuildShaders.hpp"
#include "ocBuildPipelines.hpp"
#include "ocBuildMaterials.hpp"

struct ocBuildGraphicsContext
{
    std::vector<struct json_value_s*> pipelineJSONRoots;    // <-- Used for clean up.
    std::vector<ocBuildPipelineDefinition> pipelineDefinitions;
};


#include "ocBuildUtils.cpp"
#include "ocBuildShaders.cpp"
#include "ocBuildPipelines.cpp"
#include "ocBuildMaterials.cpp"



void ocBuildGraphicsContextUninit(ocBuildGraphicsContext* pGraphicsBuildContext)
{
    if (pGraphicsBuildContext == NULL) {
        return;
    }

    for (size_t i = 0; i < pGraphicsBuildContext->pipelineJSONRoots.size(); ++i) {
        free(pGraphicsBuildContext->pipelineJSONRoots[i]);
    }
}


int ocBuildGraphics(int argc, char** argv)
{
    ocResult result = OC_SUCCESS;

    const char* pFileHeaderText = 
        "// Copyright (C) 2019 David Reid. See included LICENSE file.\n"
        "\n"
        "// This file is auto-generated by a tool. Do not modify.\n"
        "\n";

    result = ocOpenAndWriteTextFile(OC_BUILD_INTERMEDIATE_DIRECTORY "/vulkan/ocGraphics_Vulkan_Autogen.cpp", pFileHeaderText);
    if (result != OC_SUCCESS) {
        return -2;
    }


    ocBuildGraphicsContext graphicsBuildContext;

    // Compile shaders first.
    result = ocBuildCompileShaders(argc, argv);
    if (result != OC_SUCCESS) {
        return -3;
    }

    // Pipelines come after shaders since they depend on them.
    result = ocBuildCompilePipelines(argc, argv, &graphicsBuildContext);
    if (result != OC_SUCCESS) {
        return -4;
    }

    // Materials depend on pipelines.
    result = ocBuildCompileMaterials(argc, argv, &graphicsBuildContext);
    if (result != 0) {
        return result;
    }


    ocBuildGraphicsContextUninit(&graphicsBuildContext);
    return 0;
}

int ocBuildGraphicsPost(int argc, char** argv)
{
    ocResult result = OC_SUCCESS;

    // Copy the final generated files over to the main source tree.
    if (!ocCopyFile(OC_BUILD_INTERMEDIATE_DIRECTORY "/vulkan/ocGraphics_Vulkan_Autogen.cpp", "source/oc/ocEngine/Graphics/Vulkan/ocGraphics_Vulkan_Autogen.cpp", OC_FALSE)) {
        return -6;
    }

    return 0;
}

int main(int argc, char** argv)
{
    // Set the root directory from the start so that everything going forward can assume it's set correctly. The root directory
    // should be set to the root directory of the source code repository.
    const char* pRootDir = ocCmdLineGetValue(argc, argv, "--rootdir");
    if (pRootDir != NULL) {
        ocSetCurrentDirectory(pRootDir);
    }

    int result = 0;

    // Graphics.
    result = ocBuildGraphics(argc, argv);
    if (result != 0) {
        return result;
    }



    // --- POST PROCESSING ---
    //
    // At this point everything was a success. Here is where we make the final changes to the main source tree.

    // Graphics
    result = ocBuildGraphicsPost(argc, argv);
    if (result != 0) {
        return result;
    }

    return result;
}