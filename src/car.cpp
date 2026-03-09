#include "car.hpp"

#include <cmath>
#include <map>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shaders.hpp"

struct Material
{
    glm::vec4 emission;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec3 specular;
    GLfloat shininess;
};

using namespace gl;
using namespace glm;

namespace
{
    Material defaultCarMaterial =
    {
        {0.0f, 0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f, 0.0f},
        {1.0f, 1.0f, 1.0f, 0.0f},
        {0.7f, 0.7f, 0.7f},
        10.0f
    };
}

Car::Car()
    : position(0.0f, 0.0f, 0.0f)
    , orientation(0.0f, 0.0f)
    , speed(0.f)
    , wheelsRollAngle(0.f)
    , steeringAngle(0.f)
    , isHeadlightOn(false)
    , isBraking(false)
    , isLeftBlinkerActivated(false)
    , isRightBlinkerActivated(false)
    , isDrawingFront_(true)
    , isBlinkerOn(false)
    , blinkerTimer(0.f)
    , isDrawingLeftSide(false)
{
}

void Car::loadModels()
{
    frame_.load("../models/frame.ply");
    wheel_.load("../models/wheel.ply");
    blinker_.load("../models/blinker.ply");
    light_.load("../models/light.ply");

    const char* WINDOW_MODEL_PATHES[] =
    {
        "../models/window.f.ply",
        "../models/window.r.ply",
        "../models/window.fl.ply",
        "../models/window.fr.ply",
        "../models/window.rl.ply",
        "../models/window.rr.ply"
    };

    for (unsigned int i = 0; i < 6; ++i)
    {
        windows[i].load(WINDOW_MODEL_PATHES[i]);
    }
}

void Car::update(float deltaTime)
{
    if (isBraking)
    {
        const float LOW_SPEED_THRESHOLD = 0.1f;
        const float BRAKE_APPLIED_SPEED_THRESHOLD = 0.01f;
        const float BRAKING_FORCE = 4.f;

        if (fabs(speed) < LOW_SPEED_THRESHOLD)
            speed = 0.f;

        if (speed > BRAKE_APPLIED_SPEED_THRESHOLD)
            speed -= BRAKING_FORCE * deltaTime;
        else if (speed < -BRAKE_APPLIED_SPEED_THRESHOLD)
            speed += BRAKING_FORCE * deltaTime;
    }

    const float WHEELBASE = 2.7f;
    const float angularSpeed = speed * sin(-glm::radians(steeringAngle)) / WHEELBASE;
    orientation.y += angularSpeed * deltaTime;

    const glm::vec4 rotatedMove =
        glm::rotate(glm::mat4(1.0f), orientation.y, glm::vec3(0.0f, 1.0f, 0.0f))
        * glm::vec4(-speed, 0.f, 0.f, 1.f);

    const glm::vec3 positionMod(rotatedMove.x, rotatedMove.y, rotatedMove.z);
    position += positionMod * deltaTime;

    const float WHEEL_RADIUS = 0.2f;
    const float PI = glm::pi<float>();

    wheelsRollAngle += speed / (2.f * PI * WHEEL_RADIUS) * deltaTime;

    if (wheelsRollAngle > PI)
        wheelsRollAngle -= 2.f * PI;
    else if (wheelsRollAngle < -PI)
        wheelsRollAngle += 2.f * PI;

    if (isRightBlinkerActivated || isLeftBlinkerActivated)
    {
        const float BLINKER_PERIOD_SEC = 0.5f;
        blinkerTimer += deltaTime;
        if (blinkerTimer > BLINKER_PERIOD_SEC)
        {
            blinkerTimer = 0.f;
            isBlinkerOn = !isBlinkerOn;
        }
    }
    else
    {
        isBlinkerOn = true;
        blinkerTimer = 0.f;
    }

    carModel = glm::mat4(1.0f);
    carModel = glm::translate(carModel, position);
    carModel = glm::rotate(carModel, orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));
}

void Car::draw(glm::mat4& projView)
{
    projectionView_ = projView;

    currentMatrix_ = glm::translate(glm::mat4(1.0f), position);
    currentMatrix_ = glm::rotate(currentMatrix_, orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));

    glActiveTexture(GL_TEXTURE0);
    carTexture->use();

    material->updateData(&defaultCarMaterial, 0, sizeof(defaultCarMaterial));
    glm::mat4 carRoot = currentMatrix_;
    drawFrame();

    currentMatrix_ = carRoot;
    material->updateData(&defaultCarMaterial, 0, sizeof(defaultCarMaterial));
    drawWheels();
}

void Car::draw(glm::mat4& projView, glm::mat4& view)
{
    projectionView_ = projView;
    view_ = view;

    currentMatrix_ = glm::translate(glm::mat4(1.0f), position);
    currentMatrix_ = glm::rotate(currentMatrix_, orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));

    glActiveTexture(GL_TEXTURE0);
    carTexture->use();

    material->updateData(&defaultCarMaterial, 0, sizeof(defaultCarMaterial));
    glm::mat4 carRoot = currentMatrix_;
    drawFrame();

    currentMatrix_ = carRoot;
    material->updateData(&defaultCarMaterial, 0, sizeof(defaultCarMaterial));
    drawWheels();
}

void Car::drawOutline(glm::mat4& projView, glm::mat4& view)
{
    projectionView_ = projView;
    view_ = view;

    currentMatrix_ = glm::translate(glm::mat4(1.0f), position);
    currentMatrix_ = glm::rotate(currentMatrix_, orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));

    edgeEffectShader->use();

    glm::mat4 carRoot = currentMatrix_;
    drawFrameOutline();
    currentMatrix_ = carRoot;
    drawWheelsOutline();
}

void Car::drawWindows(glm::mat4& projView, glm::mat4& view)
{
    const glm::vec3 WINDOW_POSITION[] =
    {
        glm::vec3(-0.813f, 0.755f, 0.0f),
        glm::vec3(1.092f, 0.761f, 0.0f),
        glm::vec3(-0.3412f, 0.757f, 0.51f),
        glm::vec3(-0.3412f, 0.757f, -0.51f),
        glm::vec3(0.643f, 0.756f, 0.508f),
        glm::vec3(0.643f, 0.756f, -0.508f)
    };

    glm::mat4 carBase = glm::translate(glm::mat4(1.0f), position);
    carBase = glm::rotate(carBase, orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    carBase = glm::translate(carBase, glm::vec3(0.0f, 0.25f, 0.0f));

    std::multimap<float, unsigned int> sorted;
    for (unsigned int i = 0; i < 6; ++i)
    {
        glm::vec4 viewPos = view * carBase * glm::vec4(WINDOW_POSITION[i], 1.0f);
        sorted.emplace(-viewPos.z, i);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

    glActiveTexture(GL_TEXTURE0);
    carWindowTexture->use();

    for (auto it = sorted.rbegin(); it != sorted.rend(); ++it)
    {
        glm::mat4 mvp = projView * carBase;
        celShadingShader->setMatrices(mvp, view, carBase);
        windows[it->second].draw();
    }

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}

void Car::drawFrame()
{
    material->updateData(&defaultCarMaterial, 0, sizeof(defaultCarMaterial));

    glm::mat4 frameMatrix = glm::translate(currentMatrix_, glm::vec3(0.0f, 0.25f, 0.0f));
    glm::mat4 mvp = projectionView_ * frameMatrix;

    celShadingShader->setMatrices(mvp, view_, frameMatrix);
    frame_.draw();

    currentMatrix_ = frameMatrix;
    drawHeadlights();
}

void Car::drawFrameOutline()
{
    glm::mat4 frameMatrix = glm::translate(currentMatrix_, glm::vec3(0.0f, 0.25f, 0.0f));
    glm::mat4 mvp = projectionView_ * frameMatrix;

    glUniformMatrix4fv(edgeEffectShader->mvpULoc, 1, GL_FALSE, glm::value_ptr(mvp));
    frame_.draw();

    currentMatrix_ = frameMatrix;
    drawHeadlightsOutline();
}

void Car::drawWheel(float offset)
{
    material->updateData(&defaultCarMaterial, 0, sizeof(defaultCarMaterial));

    currentMatrix_ = glm::translate(currentMatrix_, glm::vec3(0.0f, 0.0f, offset));
    currentMatrix_ = glm::rotate(currentMatrix_, wheelsRollAngle, glm::vec3(0.0f, 0.0f, 1.0f));
    currentMatrix_ = glm::translate(currentMatrix_, glm::vec3(0.0f, 0.0f, -offset));

    glm::mat4 mvp = projectionView_ * currentMatrix_;
    celShadingShader->setMatrices(mvp, view_, currentMatrix_);
    wheel_.draw();
}

void Car::drawWheelOutline(float offset)
{
    currentMatrix_ = glm::translate(currentMatrix_, glm::vec3(0.0f, 0.0f, offset));
    currentMatrix_ = glm::rotate(currentMatrix_, wheelsRollAngle, glm::vec3(0.0f, 0.0f, 1.0f));
    currentMatrix_ = glm::translate(currentMatrix_, glm::vec3(0.0f, 0.0f, -offset));

    glm::mat4 mvp = projectionView_ * currentMatrix_;
    glUniformMatrix4fv(edgeEffectShader->mvpULoc, 1, GL_FALSE, glm::value_ptr(mvp));
    wheel_.draw();
}

void Car::drawWheels()
{
    const glm::vec3 WHEEL_POSITIONS[] =
    {
        glm::vec3(-1.29f, 0.245f, -0.57f),
        glm::vec3(-1.29f, 0.245f, 0.57f),
        glm::vec3(1.4f, 0.245f, -0.57f),
        glm::vec3(1.4f, 0.245f, 0.57f)
    };

    glm::mat4 carRoot = currentMatrix_;

    for (int i = 0; i < 4; ++i)
    {
        currentMatrix_ = glm::translate(carRoot, WHEEL_POSITIONS[i]);

        const bool isLeft = WHEEL_POSITIONS[i].z > 0.0f;
        const float offset = isLeft ? -0.10124f : 0.10124f;

        if (isLeft)
            currentMatrix_ = glm::rotate(currentMatrix_, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        if (WHEEL_POSITIONS[i].x < 0.0f)
        {
            currentMatrix_ = glm::translate(currentMatrix_, glm::vec3(0.0f, 0.0f, offset));
            currentMatrix_ = glm::rotate(currentMatrix_, glm::radians(-steeringAngle), glm::vec3(0.0f, 1.0f, 0.0f));
            currentMatrix_ = glm::translate(currentMatrix_, glm::vec3(0.0f, 0.0f, -offset));
        }

        drawWheel(offset);
    }
}

void Car::drawWheelsOutline()
{
    const glm::vec3 WHEEL_POSITIONS[] =
    {
        glm::vec3(-1.29f, 0.245f, -0.57f),
        glm::vec3(-1.29f, 0.245f, 0.57f),
        glm::vec3(1.4f, 0.245f, -0.57f),
        glm::vec3(1.4f, 0.245f, 0.57f)
    };

    glm::mat4 carRoot = currentMatrix_;

    for (int i = 0; i < 4; ++i)
    {
        currentMatrix_ = glm::translate(carRoot, WHEEL_POSITIONS[i]);

        const bool isLeft = WHEEL_POSITIONS[i].z > 0.0f;
        const float offset = isLeft ? -0.10124f : 0.10124f;

        if (isLeft)
            currentMatrix_ = glm::rotate(currentMatrix_, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        if (WHEEL_POSITIONS[i].x < 0.0f)
        {
            currentMatrix_ = glm::translate(currentMatrix_, glm::vec3(0.0f, 0.0f, offset));
            currentMatrix_ = glm::rotate(currentMatrix_, glm::radians(-steeringAngle), glm::vec3(0.0f, 1.0f, 0.0f));
            currentMatrix_ = glm::translate(currentMatrix_, glm::vec3(0.0f, 0.0f, -offset));
        }

        drawWheelOutline(offset);
    }
}

void Car::drawBlinker()
{
    const bool isBlinkerActivated =
        (isDrawingLeftSide && isRightBlinkerActivated) ||
        (!isDrawingLeftSide && isLeftBlinkerActivated);

    const glm::vec3 ON_COLOR(1.0f, 0.7f, 0.3f);
    const glm::vec3 OFF_COLOR(0.5f, 0.35f, 0.15f);
    const glm::vec3 color = (isBlinkerOn && isBlinkerActivated) ? ON_COLOR : OFF_COLOR;

    Material blinkerMat = { {0, 0, 0, 0}, {color, 0}, {color, 0}, {color}, 10.0f };
    material->updateData(&blinkerMat, 0, sizeof(blinkerMat));

    glm::mat4 blinkerM = currentMatrix_;

    if (!isDrawingLeftSide)
        blinkerM = glm::scale(blinkerM, glm::vec3(-1.0f, 1.0f, 1.0f));
    blinkerM = glm::translate(blinkerM, glm::vec3(0.0f, 0.0f, -0.06065f));

    glm::mat4 mvp = projectionView_ * blinkerM;
    celShadingShader->setMatrices(mvp, view_, blinkerM);
    blinker_.draw();
}

void Car::drawLight()
{
    const glm::vec3 FRONT_ON_COLOR(1.0f, 1.0f, 1.0f);
    const glm::vec3 FRONT_OFF_COLOR(0.5f, 0.5f, 0.5f);
    const glm::vec3 REAR_ON_COLOR(1.0f, 0.1f, 0.1f);
    const glm::vec3 REAR_OFF_COLOR(0.5f, 0.1f, 0.1f);

    glm::mat4 lightMatrix = glm::translate(currentMatrix_, glm::vec3(0.0f, 0.0f, 0.029f));
    const bool isFront = isDrawingFront_;

    Material lightMat;
    if (isFront)
    {
        glm::vec3 c = isHeadlightOn ? FRONT_ON_COLOR : FRONT_OFF_COLOR;
        lightMat = { {0, 0, 0, 0}, {c, 0}, {c, 0}, {c}, 10.0f };
    }
    else
    {
        glm::vec3 c = isBraking ? REAR_ON_COLOR : REAR_OFF_COLOR;
        lightMat = { {0, 0, 0, 0}, {c, 0}, {c, 0}, {c}, 10.0f };
    }
    material->updateData(&lightMat, 0, sizeof(lightMat));

    glm::mat4 mvp = projectionView_ * lightMatrix;
    celShadingShader->setMatrices(mvp, view_, lightMatrix);
    light_.draw();
}

void Car::drawHeadlight()
{
    glm::mat4 headlightRoot = currentMatrix_;
    drawLight();
    currentMatrix_ = headlightRoot;
    drawBlinker();
    currentMatrix_ = headlightRoot;
}

void Car::drawHeadlights()
{
    const glm::vec3 HEADLIGHT_POSITIONS[] =
    {
        glm::vec3(-1.9650f, 0.38f, -0.45f),
        glm::vec3(-1.9650f, 0.38f, 0.45f),
        glm::vec3(2.0019f, 0.38f, -0.45f),
        glm::vec3(2.0019f, 0.38f, 0.45f)
    };

    glm::mat4 frameBase = currentMatrix_;

    for (int i = 0; i < 4; ++i)
    {
        currentMatrix_ = glm::translate(frameBase, HEADLIGHT_POSITIONS[i]);

        isDrawingLeftSide = (HEADLIGHT_POSITIONS[i].z > 0.0f);
        isDrawingFront_ = (HEADLIGHT_POSITIONS[i].x < 0.0f);

        glm::mat4 headlightBase = currentMatrix_;

        if (isDrawingFront_)
            currentMatrix_ = glm::rotate(currentMatrix_, glm::radians(-5.0f), glm::vec3(0.0f, 0.0f, 1.0f));

        drawLight();

        currentMatrix_ = headlightBase;
        drawBlinker();
    }

    currentMatrix_ = frameBase;
}

void Car::drawHeadlightsOutline()
{
    const glm::vec3 HEADLIGHT_POSITIONS[] =
    {
        glm::vec3(-1.9650f, 0.38f, -0.45f),
        glm::vec3(-1.9650f, 0.38f, 0.45f),
        glm::vec3(2.0019f, 0.38f, -0.45f),
        glm::vec3(2.0019f, 0.38f, 0.45f)
    };

    glm::mat4 frameBase = currentMatrix_;

    for (int i = 0; i < 4; ++i)
    {
        currentMatrix_ = glm::translate(frameBase, HEADLIGHT_POSITIONS[i]);

        const bool isLeft = HEADLIGHT_POSITIONS[i].z > 0.0f;
        isDrawingFront_ = (HEADLIGHT_POSITIONS[i].x < 0.0f);

        glm::mat4 headlightBase = currentMatrix_;

        if (isDrawingFront_)
            currentMatrix_ = glm::rotate(currentMatrix_, glm::radians(-5.0f), glm::vec3(0.0f, 0.0f, 1.0f));

        glm::mat4 lightMatrix = glm::translate(currentMatrix_, glm::vec3(0.0f, 0.0f, 0.029f));
        glm::mat4 mvp = projectionView_ * lightMatrix;
        glUniformMatrix4fv(edgeEffectShader->mvpULoc, 1, GL_FALSE, glm::value_ptr(mvp));
        light_.draw();

        currentMatrix_ = headlightBase;

        glm::mat4 blinkerMatrix = currentMatrix_;
        if (!isLeft)
            blinkerMatrix = glm::scale(blinkerMatrix, glm::vec3(-1.0f, 1.0f, 1.0f));
        blinkerMatrix = glm::translate(blinkerMatrix, glm::vec3(0.0f, 0.0f, -0.06065f));

        mvp = projectionView_ * blinkerMatrix;
        glUniformMatrix4fv(edgeEffectShader->mvpULoc, 1, GL_FALSE, glm::value_ptr(mvp));
        blinker_.draw();
    }

    currentMatrix_ = frameBase;
}