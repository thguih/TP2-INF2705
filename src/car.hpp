#pragma once

#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>

#include "model.hpp"

class Car
{   
public:
    Car();
    
    void loadModels();
    
    void update(float deltaTime);
    
    void draw(glm::mat4& projView);
    
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
    glm::mat4 projectionView_;
    glm::mat4 currentMatrix_;
    
public:
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


