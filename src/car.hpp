#pragma once

#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>

#include "model.hpp"
#include "uniform_buffer.hpp"
#include <textures.hpp>

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

    // PARTIE 2 - contour
    void drawOutline(glm::mat4& projView, glm::mat4& view);

    void drawWindows(glm::mat4& projView, glm::mat4& view);
    
private:
    
    void drawFrame();
    void drawFrameOutline();     // PARTIE 2
    
    void drawWheel(float offset);
    void drawWheelOutline(float offset);   // PARTIE 2
    void drawWheels();
    void drawWheelsOutline();              // PARTIE 2
    
    void drawBlinker();
    void drawLight();    
    void drawHeadlights();
    void drawHeadlightsOutline();          // PARTIE 2
    
private:    
    Model frame_;
    Model wheel_;
    Model blinker_;
    Model light_;

    Model windows[6];

    glm::mat4 projectionView_;
    glm::mat4 currentMatrix_;
    glm::mat4 view_;
    
public:

    glm::mat4 carModel = glm::mat4(1.0f);

    EdgeEffect* edgeEffectShader = nullptr;
    CelShading* celShadingShader = nullptr;
    UniformBuffer* material = nullptr;

    Texture2D* carTexture = nullptr;
    Texture2D* carWindowTexture = nullptr;

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
};