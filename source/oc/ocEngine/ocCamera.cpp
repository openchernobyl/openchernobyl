// Copyright (C) 2018 David Reid. See included LICENSE file.

void ocCameraInitOrtho(float left, float right, float top, float bottom, ocCamera* pCamera)
{
    if (pCamera == NULL) {
        return;
    }

    ocCameraSetOrtho(pCamera, left, right, top, bottom);
    pCamera->position = glm::vec4(0, 0, 0, 0);
    pCamera->rotation = glm::quat(1, 0, 0, 0);
}

void ocCameraInitPerspective(float fov, float aspect, float znear, float zfar, ocCamera* pCamera)
{
    if (pCamera == NULL) {
        return;
    }

    ocCameraSetPerspective(pCamera, fov, aspect, znear, zfar);
    pCamera->position = glm::vec4(0, 0, 0, 0);
    pCamera->rotation = glm::quat(1, 0, 0, 0);
}


void ocCameraSetOrtho(ocCamera* pCamera, float left, float right, float top, float bottom)
{
    if (pCamera == NULL) {
        return;
    }

    pCamera->projectionType = ocCameraProjectionType_Ortho;
    pCamera->projection = glm::ortho(left, right, bottom, top);
    pCamera->ortho.left = left;
    pCamera->ortho.top = top;
    pCamera->ortho.right = right;
    pCamera->ortho.bottom = bottom;
}

void ocCameraSetPerspective(ocCamera* pCamera, float fov, float aspect, float znear, float zfar)
{
    if (pCamera == NULL) {
        return;
    }

    pCamera->projectionType = ocCameraProjectionType_Perspective;
    pCamera->projection = glm::perspective(fov, aspect, znear, zfar);
    pCamera->perspective.fov = fov;
    pCamera->perspective.aspect = aspect;
    pCamera->perspective.znear = znear;
    pCamera->perspective.zfar = zfar;
}

glm::mat4 ocCameraGetProjectionMatrix(const ocCamera* pCamera)
{
    if (pCamera == NULL) {
        return glm::mat4();
    }

    return pCamera->projection;
}

glm::mat4 ocCameraGetViewMatrix(const ocCamera* pCamera)
{
    if (pCamera == NULL) {
        return glm::mat4();
    }

    return glm::mat4_cast(glm::inverse(pCamera->rotation)) * glm::translate(-glm::vec3(pCamera->position));
}


void ocCameraSetPosition(ocCamera* pCamera, float x, float y, float z)
{
    if (pCamera == NULL) {
        return;
    }

    pCamera->position.x = x;
    pCamera->position.y = y;
    pCamera->position.z = z;
}

void ocCameraSetPosition(ocCamera* pCamera, const glm::vec3 &position)
{
    ocCameraSetPosition(pCamera, position.x, position.y, position.z);
}


void ocCameraSetRotation(ocCamera* pCamera, const glm::quat &rotation)
{
    if (pCamera == NULL) {
        return;
    }

    pCamera->rotation = rotation;
}

void ocCameraRotate(ocCamera* pCamera, const glm::quat &rotation)
{
    if (pCamera == NULL) {
        return;
    }

    pCamera->rotation = pCamera->rotation * rotation;
}

void ocCameraRotateX(ocCamera* pCamera, float angleInRadians)
{
    if (pCamera == NULL) {
        return;
    }

    ocCameraSetRotation(pCamera, glm::angleAxis(angleInRadians, glm::vec3(1, 0, 0)) * pCamera->rotation);
}

void ocCameraRotateY(ocCamera* pCamera, float angleInRadians)
{
    if (pCamera == NULL) {
        return;
    }

    ocCameraSetRotation(pCamera, glm::angleAxis(angleInRadians, glm::vec3(0, 1, 0)) * pCamera->rotation);
}