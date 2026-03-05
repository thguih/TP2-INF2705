#pragma once

#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>

#include "model.hpp"
#include "uniform_buffer.hpp"

class EdgeEffect;
class CelShading;

class Car
{   
public:
    Car();
    
    void loadModels();
    
    void update(float deltaTime);
    
    void draw(glm::mat4& projView);

    void draw(glm::mat4& projView, glm::mat4& view);

    void drawWindows(glm::mat4& projView, glm::mat4& view);
    
private:
    
    void drawFrame();
    
    void drawWheel(float offset);
    void drawWheels();
    
    void drawBlinker();
    void drawLight();    
    void drawHeadlight();
    void drawHeadlights();
    
private:    
    Model frame_;
    Model wheel_;
    Model blinker_;
    Model light_;

    Model windows_[6];
    glm::mat4 projectionView_;
    glm::mat4 currentMatrix_;
    
public:

    glm::mat4 carModel = glm::mat4(1.0f);

    EdgeEffect* edgeEffectShader = nullptr;
    CelShading* celShadingShader = nullptr;
    UniformBuffer* material = nullptr;

    glm::vec3 position;
    glm::vec2 orientation;    
    
    float speed;
    float wheelsRollAngle;
    float steeringAngle;
    bool isHeadlightOn;
    bool isBraking;
    bool isLeftBlinkerActivated;
    bool isRightBlinkerActivated;
    bool isDrawingFront_ = true;
    
    bool isBlinkerOn;
    float blinkerTimer;
    bool isDrawingLeftSide;
    
    GLuint colorModUniformLocation;
    GLuint mvpUniformLocation;
};


