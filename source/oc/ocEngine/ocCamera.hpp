// Copyright (C) 2016 David Reid. See included LICENSE file.

enum ocCameraProjectionType
{
    ocCameraProjectionType_Ortho,
    ocCameraProjectionType_Perspective
};

struct ocCamera
{
    ocCameraProjectionType projectionType;
    glm::vec4 position;     // Using a vec4 for padding and future SSE optimizations.
    glm::quat rotation;
    glm::mat4 projection;   // Set by ocCameraSetOrtho() or ocCameraSetPerspective().

    union
    {
        struct
        {
            float left;
            float right;
            float top;
            float bottom;
        } ortho;

        struct
        {
            float fov;
            float aspect;
            float znear;
            float zfar;
        } perspective;
    };
};

// Initialization.
void ocCameraInitOrtho(ocCamera* pCamera, float left, float right, float top, float bottom);
void ocCameraInitPerspective(ocCamera* pCamera, float fov, float aspect, float znear, float zfar);

// Projection.
void ocCameraSetOrtho(ocCamera* pCamera, float left, float right, float top, float bottom);
void ocCameraSetPerspective(ocCamera* pCamera, float fov, float aspect, float znear, float zfar);
glm::mat4 ocCameraGetProjectionMatrix(ocCamera* pCamera);
glm::mat4 ocCameraGetViewMatrix(ocCamera* pCamera);

// Movement.
void ocCameraSetPosition(ocCamera* pCamera, float x, float y, float z);
void ocCameraSetPosition(ocCamera* pCamera, const glm::vec3 &position);

// Rotation.
void ocCameraSetRotation(ocCamera* pCamera, const glm::quat &rotation);