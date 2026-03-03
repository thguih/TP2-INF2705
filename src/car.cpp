#include "car.hpp"

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


using namespace gl;
using namespace glm;

    
Car::Car()
: position(0.0f, 0.0f, 0.0f), orientation(0.0f, 0.0f), speed(0.f)
, wheelsRollAngle(0.f), steeringAngle(0.f)
, isHeadlightOn(false), isBraking(false)
, isLeftBlinkerActivated(false), isRightBlinkerActivated(false)
, isBlinkerOn(false), blinkerTimer(0.f)
{}

void Car::loadModels()
{
    frame_.load("../models/frame.ply");
    wheel_.load("../models/wheel.ply");
    blinker_.load("../models/blinker.ply");
    light_.load("../models/light.ply");
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
    float angularSpeed = speed * sin(-glm::radians(steeringAngle)) / WHEELBASE;
    orientation.y += angularSpeed * deltaTime;
    
    glm::vec3 positionMod = glm::rotate(glm::mat4(1.0f), orientation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(-speed, 0.f, 0.f, 1.f);
    position += positionMod * deltaTime;
    
    const float WHEEL_RADIUS = 0.2f;
    wheelsRollAngle += speed / (2.f * M_PI * WHEEL_RADIUS) * deltaTime;
    
    if (wheelsRollAngle > M_PI)
    wheelsRollAngle -= 2.f * M_PI;

    else if (wheelsRollAngle < -M_PI)
        wheelsRollAngle += 2.f * M_PI;
        
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
}

void Car::draw(glm::mat4& projView)
{

    projectionView_ = projView;

        currentMatrix_ = glm::translate(glm::mat4(1.0f), position);
    currentMatrix_ = glm::rotate(currentMatrix_, orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 carRoot = currentMatrix_;
    drawFrame();
    currentMatrix_ = carRoot;
    drawWheels();

}
    
void Car::drawFrame()
{
    glm::mat4 frameMatrix = glm::translate(currentMatrix_, glm::vec3(0.0f, 0.25f, 0.0f));
    glm::mat4 mvp = projectionView_ * frameMatrix;

    glUniformMatrix4fv(mvpUniformLocation, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3f(colorModUniformLocation, 1.0f, 1.0f, 1.0f);
    frame_.draw();
    currentMatrix_ = frameMatrix; 
    drawHeadlights();

}

void Car::drawWheel(float offset)
{
    currentMatrix_ = glm::translate(currentMatrix_, glm::vec3(0, 0, offset));
    currentMatrix_ = glm::rotate(currentMatrix_, wheelsRollAngle, glm::vec3(0, 0, 1));
    currentMatrix_ = glm::translate(currentMatrix_, glm::vec3(0, 0, -offset));


    glm::mat4 mvp = projectionView_ * currentMatrix_;
    glUniformMatrix4fv(mvpUniformLocation, 1, GL_FALSE, glm::value_ptr(mvp));

    wheel_.draw();
}

void Car::drawWheels()
{
    const glm::vec3 WHEEL_POSITIONS[] =
    {
        glm::vec3(-1.29f, 0.245f, -0.57f),
        glm::vec3(-1.29f, 0.245f,  0.57f),
        glm::vec3(1.4f , 0.245f, -0.57f),
        glm::vec3(1.4f , 0.245f,  0.57f)
    };

    glm::mat4 carRoot = currentMatrix_;
    glUniform3f(colorModUniformLocation, 1.0f, 1.0f, 1.0f);

    for (int i = 0; i < 4; ++i)
    {
        currentMatrix_ = glm::translate(carRoot, WHEEL_POSITIONS[i]);

        bool isLeft = WHEEL_POSITIONS[i].z > 0.0f;
        float offset = isLeft ? -0.10124f : 0.10124f;

        if (isLeft)
            currentMatrix_ = glm::rotate(currentMatrix_, glm::radians(180.0f), glm::vec3(0, 1, 0));

        if (WHEEL_POSITIONS[i].x < 0.0f)
        {
            currentMatrix_ = glm::translate(currentMatrix_, glm::vec3(0, 0, offset));
            currentMatrix_ = glm::rotate(currentMatrix_, glm::radians(-steeringAngle), glm::vec3(0, 1, 0));
            currentMatrix_ = glm::translate(currentMatrix_, glm::vec3(0, 0, -offset));
        }
        
        drawWheel(offset);
    }
}


void Car::drawBlinker()
{
    bool isBlinkerActivated =
        (isDrawingLeftSide && isLeftBlinkerActivated) ||
        (!isDrawingLeftSide && isRightBlinkerActivated);
    const glm::vec3 ON_COLOR(1.0f, 0.7f, 0.3f);
    const glm::vec3 OFF_COLOR(0.5f, 0.35f, 0.15f);
    glm::vec3 color = (isBlinkerOn && isBlinkerActivated) ? ON_COLOR : OFF_COLOR;

    glm::mat4 blinkerM = currentMatrix_;

    if (!isDrawingLeftSide)
        blinkerM = glm::scale(blinkerM, glm::vec3(-1.0f, 1.0f, 1.0f));

    blinkerM = glm::translate(blinkerM, glm::vec3(0.0f, 0.0f, -0.06065f));

    glUniform3fv(colorModUniformLocation, 1, glm::value_ptr(color));
    glUniformMatrix4fv(mvpUniformLocation, 1, GL_FALSE, glm::value_ptr(projectionView_ * blinkerM));
    blinker_.draw();
}



void Car::drawLight()
{
    glm::mat4 lightMatrix = glm::translate(currentMatrix_, glm::vec3(0.0f, 0.0f, 0.029f));

    const bool isFront = isDrawingFront_;

    glm::vec3 color;
    if (isFront)
    {
        color = isHeadlightOn ? glm::vec3(1.0f, 1.0f, 1.0f)
            : glm::vec3(0.5f, 0.5f, 0.5f);
    }
    else
    {
        color = isBraking ? glm::vec3(1.0f, 0.1f, 0.1f)
            : glm::vec3(0.5f, 0.1f, 0.1f);
    }

    glUniform3fv(colorModUniformLocation, 1, glm::value_ptr(color));
    glUniformMatrix4fv(mvpUniformLocation, 1, GL_FALSE, glm::value_ptr(projectionView_ * lightMatrix));
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
        glm::vec3(-1.9650f, 0.38f,  0.45f),
        glm::vec3(2.0019f, 0.38f, -0.45f),
        glm::vec3(2.0019f, 0.38f,  0.45f)
    };

    glm::mat4 frameBase = currentMatrix_;

    for (int i = 0; i < 4; ++i)
    {
        currentMatrix_ = glm::translate(frameBase, HEADLIGHT_POSITIONS[i]);

        isDrawingLeftSide = (HEADLIGHT_POSITIONS[i].z > 0.0f);

        isDrawingFront_ = (HEADLIGHT_POSITIONS[i].x < 0.0f);

        glm::mat4 headlightBase = currentMatrix_;

        if (isDrawingFront_)
        {
            currentMatrix_ = glm::rotate(currentMatrix_, glm::radians(-5.0f), glm::vec3(0, 0, 1));
        }

        drawLight();

        currentMatrix_ = headlightBase;
        drawBlinker();
    }

    currentMatrix_ = frameBase;
}
