#include "car.hpp"

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shaders.hpp"
#include <map>


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

    carModel = glm::mat4(1.0f);
    carModel = glm::translate(carModel, position);
    carModel = glm::rotate(carModel, orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));
}


void Car::draw(glm::mat4& projView, glm::mat4& view) {

    projectionView_ = projView;
    view_ = view;

    currentMatrix_ = glm::translate(glm::mat4(1.0f), position);
    currentMatrix_ = glm::rotate(currentMatrix_, orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));

    glActiveTexture(GL_TEXTURE0);
    carTexture->use();
    
    glm::mat4 carRoot = currentMatrix_;
    drawFrame();
    currentMatrix_ = carRoot;
    drawWheels();
}


void Car::drawWindows(glm::mat4& projView, glm::mat4& view)
{
    const glm::vec3 WINDOW_POSITION[] =
    {
        glm::vec3(-0.813, 0.755, 0.0),
        glm::vec3(1.092, 0.761, 0.0),
        glm::vec3(-0.3412, 0.757, 0.51),
        glm::vec3(-0.3412, 0.757, -0.51),
        glm::vec3(0.643, 0.756, 0.508),
        glm::vec3(0.643, 0.756, -0.508)
    };

    // TODO: À ajouter et compléter.
    //       Dessiner les vitres de la voiture. Celles-ci ont une texture transparente,
    //       il est donc nécessaire d'activer le mélange des couleurs (GL_BLEND).
    //       De plus, vous devez dessiner les fenêtres du plus loin vers le plus proche
    //       pour éviter les problèmes de mélange.
    //       Utiliser un map avec la distance en clef pour trier les fenêtres (les maps trient
    //       à l'insertion).
    //       Les fenêtres doivent être visibles des deux sens.
    //       Il est important de restaurer l'état du contexte qui a été modifié à la fin de la méthode.

    glm::mat4 carBase = glm::translate(glm::mat4(1.0f), position);
    carBase = glm::rotate(carBase, orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    carBase = glm::translate(carBase, glm::vec3(0.0f, 0.25f, 0.0f));


    // Les fenêtres sont par rapport au chassi, à considérer dans votre matrice
    // model = glm::translate(model, glm::vec3(0.0f, 0.25f, 0.0f));

    std::map<float, unsigned int> sorted;
    for (unsigned int i = 0; i < 6; i++)
    {        
        glm::vec4 viewPos = view * carBase * glm::vec4(WINDOW_POSITION[i], 1.0f);
        sorted[-viewPos.z] = i;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

    glActiveTexture(GL_TEXTURE0);
    carWindowTexture->use();

    // TODO: Itération à l'inverse (de la plus grande distance jusqu'à la plus petit)
    for (std::map<float, unsigned int>::reverse_iterator it = sorted.rbegin(); it != sorted.rend(); ++it)
    {
        // TODO: Dessin des fenêtres
        glm::mat4 mvp = projView * carBase;
        celShadingShader->setMatrices(mvp, view, carBase);
        windows[it->second].draw();
    }

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}
    
void Car::drawFrame()
{
    glm::mat4 frameMatrix = glm::translate(currentMatrix_, glm::vec3(0.0f, 0.25f, 0.0f));
    glm::mat4 mvp = projectionView_ * frameMatrix;

    // NEW - use the CelShading shader's setMatrices
    celShadingShader->setMatrices(mvp, view_, frameMatrix);
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
    celShadingShader->setMatrices(mvp, view_, currentMatrix_);
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

    Material blinkerMat = { {0,0,0,0}, {color,0}, {color,0}, {color}, 10.0f };
    material->updateData(&blinkerMat, 0, sizeof(blinkerMat));

    glm::mat4 blinkerM = currentMatrix_;

    if (!isDrawingLeftSide)
        blinkerM = glm::scale(blinkerM, glm::vec3(-1.0f, 1.0f, 1.0f));
    blinkerM = glm::translate(blinkerM, glm::vec3(0.0f, 0.0f, -0.06065f));

    glm::mat4 mvp = projectionView_ * blinkerM;
    celShadingShader->setMatrices(mvp, view_, blinkerM);
    blinker_.draw();

    // TODO: À ajouter dans votre méthode. À compléter pour la partie 3.
    /*Material blinkerMat =
    {
        {0.0f, 0.0f, 0.0f, 0.0f},
        {OFF_COLOR, 0.0f},
        {OFF_COLOR, 0.0f},
        {OFF_COLOR},
        10.0f
    };*/

    //if (isBlinkerOn && isBlinkerActivated)
    //    TODO: Modifier le matériel pour qu'il ait l'air d'émettre de la lumière.
    //    ... = glm::vec4(ON_COLOR, 0.0f);

    // TODO: Envoyer le matériel au shader. Partie 3.

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
    if (isFront) {
        glm::vec3 c = isHeadlightOn ? FRONT_ON_COLOR : FRONT_OFF_COLOR;
        lightMat = {{0,0,0,0}, {c,0}, {c,0}, {c}, 10.0f };
    }
    else {
        glm::vec3 c = isBraking ? REAR_ON_COLOR : REAR_OFF_COLOR;
        lightMat = { {0,0,0,0}, {c,0}, {c,0}, {c}, 10.0f };
    }
    material->updateData(&lightMat, 0, sizeof(lightMat));

    glm::mat4 mvp = projectionView_ * lightMatrix;
    celShadingShader->setMatrices(mvp, view_, lightMatrix);
    light_.draw();

    

    // TODO: À ajouter dans votre méthode. À compléter pour la partie 3.

    Material lightFrontMat =
    {
        {0.0f, 0.0f, 0.0f, 0.0f},
        {FRONT_OFF_COLOR, 0.0f},
        {FRONT_OFF_COLOR, 0.0f},
        {FRONT_OFF_COLOR},
        10.0f
    };

    Material lightRearMat =
    {
        {0.0f, 0.0f, 0.0f, 0.0f},
        {REAR_OFF_COLOR, 0.0f},
        {REAR_OFF_COLOR, 0.0f},
        {REAR_OFF_COLOR},
        10.0f
    };

    //if (isFront)
    //{
    //    // if (isHeadlightOn)
    //    //    TODO: Modifier le matériel pour qu'il ait l'air d'émettre de la lumière.
    //    //    ... = glm::vec4(FRONT_ON_COLOR, 0);

    //    // TODO: Envoyer le matériel au shader. Partie 3.
    //}
    //else
    //{
    //    // if (isBraking)
    //    //    TODO: Modifier le matériel pour qu'il ait l'air d'émettre de la lumière.
    //    //    ... = glm::vec4(REAR_ON_COLOR, 0);

    //    // TODO: Envoyer le matériel au shader. Partie 3.
    //}
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
