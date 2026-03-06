// TODO: À ajouter et compléter dans votre projet.
#include <cstddef>
#include <cstdint>
#include <array>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "model_data.hpp"
#include "shaders.hpp"
#include "textures.hpp"
#include "uniform_buffer.hpp"
#include <inf2705/OpenGLApplication.hpp>
#include <imgui/imgui.h>
#include "model.hpp"
#include "car.hpp"
#include "happly.h"


#define CHECK_GL_ERROR printGLError(__FILE__, __LINE__)

using namespace gl;
using namespace glm;

// Définition des structures pour la communication avec le shader. NE PAS MODIFIER.

struct Material
{
    glm::vec4 emission; // vec3, but padded
    glm::vec4 ambient;  // vec3, but padded
    glm::vec4 diffuse;  // vec3, but padded
    glm::vec3 specular;
    GLfloat shininess;
};

struct DirectionalLight
{
    glm::vec4 ambient;   // vec3, but padded
    glm::vec4 diffuse;   // vec3, but padded
    glm::vec4 specular;  // vec3, but padded    
    glm::vec4 direction; // vec3, but padded
};

struct SpotLight
{
    glm::vec4 ambient;   // vec3, but padded
    glm::vec4 diffuse;   // vec3, but padded
    glm::vec4 specular;  // vec3, but padded
    
    glm::vec4 position;  // vec3, but padded
    glm::vec3 direction;
    GLfloat exponent;
    GLfloat openingAngle;
    
    GLfloat padding[3];
};

// Matériels

Material defaultMat = 
{
    {0.0f, 0.0f, 0.0f, 0.0f},
    {1.0f, 1.0f, 1.0f, 0.0f},
    {1.0f, 1.0f, 1.0f, 0.0f},
    {0.7f, 0.7f, 0.7f},
    10.0f
};

Material grassMat = 
{
    {0.0f, 0.0f, 0.0f, 0.0f},
    {0.8f, 0.8f, 0.8f, 0.0f},
    {1.0f, 1.0f, 1.0f, 0.0f},
    {0.05f, 0.05f, 0.05f},
    100.0f
};

Material streetMat = 
{
    {0.0f, 0.0f, 0.0f, 0.0f},
    {0.7f, 0.7f, 0.7f, 0.0f},
    {1.0f, 1.0f, 1.0f, 0.0f},
    {0.025f, 0.025f, 0.025f},
    300.0f
};

Material streetlightMat = 
{
    {0.0f, 0.0f, 0.0f, 0.0f},
    {0.8f, 0.8f, 0.8f, 0.0f},
    {1.0f, 1.0f, 1.0f, 0.0f},
    {0.7f, 0.7f, 0.7f},
    10.0f
};

Material streetlightLightMat = 
{
    {0.8f, 0.7f, 0.5f, 0.0f},
    {1.0f, 1.0f, 1.0f, 0.0f},
    {1.0f, 1.0f, 1.0f, 0.0f},
    {0.7f, 0.7f, 0.7f},
    10.0f
};

Material windowMat = 
{
    {0.0f, 0.0f, 0.0f, 0.0f},
    {1.0f, 1.0f, 1.0f, 0.0f},
    {1.0f, 1.0f, 1.0f, 0.0f},
    {1.0f, 1.0f, 1.0f},
    2.0f
};


struct App : public OpenGLApplication
{
    App()
    : isDay_(true) // TODO: À ajouter.
    , cameraPosition_(0.f, 0.f, 0.f)
    , cameraOrientation_(0.f, 0.f)
    , currentScene_(0)
    , isMouseMotionEnabled_(false)
    {
    }
	
	void init() override
	{
		// Le message expliquant les touches de clavier.
		setKeybindMessage(
			"ESC : quitter l'application." "\n"
			"T : changer de scène." "\n"
			"W : déplacer la caméra vers l'avant." "\n"
			"S : déplacer la caméra vers l'arrière." "\n"
			"A : déplacer la caméra vers la gauche." "\n"
			"D : déplacer la caméra vers la droite." "\n"
			"Q : déplacer la caméra vers le bas." "\n"
			"E : déplacer la caméra vers le haut." "\n"
			"Flèches : tourner la caméra." "\n"
			"Souris : tourner la caméra" "\n"
			"Espace : activer/désactiver la souris." "\n"
		);

		// Config de base.
        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        
        // Partie 1
        
        // TODO:
        // Création des shaders program.
        // Fait appel à la méthode "create()".

        edgeEffectShader_.create();
        celShadingShader_.create();
        //std::cout << "CelShading program id = " << celShadingShader_.id() << "\n";
        skyShader_.create();
        

                
        // TODO: À ajouter.
        car_.edgeEffectShader = &edgeEffectShader_;
        car_.celShadingShader = &celShadingShader_;
        car_.material = &material_;
        car_.carTexture = &carTexture_;
        car_.carWindowTexture = &carWindowTexture_;

        cameraPosition_ = glm::vec3(0.f, 5.f, 25.f);
        cameraOrientation_ = glm::vec2(-0.25f, 0.f);

        car_.position = glm::vec3(0.0f, 0.0f, 13.5f);
        car_.orientation = glm::vec3(0.0f, 0.0f, 0.0f);
        
        
        // TODO: Chargement des textures, ainsi que la configuration de leurs paramètres.
        //
        //       Les textures ne se répètent pas, sauf le sol, la route (mais pas les coins), les arbres et les lampadaires.
        //
        //       Les textures ont un fini lisse, à l’exception des arbres, des lumières de lampadaire et
        //       des fenêtres de la voiture.
        //       
        //       Le mipmap __ne doit pas__ être activé pour toutes les textures, seulement le sol et la route.
        //
       
        // Simplement pour réduire l'effet "négatif" du mipmap qui rend la
        // texture flou trop près.
	    // streetTexture_.use();
	    // glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -1.0f);

        grassTexture_.load("../textures/grass.jpg");
        grassTexture_.setWrap(GL_REPEAT);
        grassTexture_.setFiltering(GL_LINEAR);
        grassTexture_.enableMipmap();

        streetTexture_.load("../textures/street.jpg");
        streetTexture_.setWrap(GL_REPEAT);
        streetTexture_.setFiltering(GL_LINEAR);
        streetTexture_.enableMipmap();

        streetcornerTexture_.load("../textures/streetcorner.jpg");
        streetcornerTexture_.setWrap(GL_CLAMP_TO_EDGE);     
        streetcornerTexture_.setFiltering(GL_LINEAR);

        treeTexture_.load("../textures/pine.jpg");
        treeTexture_.setWrap(GL_REPEAT);
        treeTexture_.setFiltering(GL_NEAREST);

        streetlightTexture_.load("../textures/streetlight.jpg");
        streetlightTexture_.setWrap(GL_REPEAT);
        streetlightTexture_.setFiltering(GL_NEAREST);

        carTexture_.load("../textures/car.png");
        carTexture_.setWrap(GL_CLAMP_TO_EDGE);
        carTexture_.setFiltering(GL_LINEAR);
        carTexture_.enableMipmap();

        carWindowTexture_.load("../textures/window.png");
        carWindowTexture_.setWrap(GL_CLAMP_TO_EDGE);
        carWindowTexture_.setFiltering(GL_NEAREST);
	    
        
        // TODO: Chargement des deux skyboxes.
        
        const char* pathes[] = {
            "../textures/skybox/Daylight Box_Right.bmp",
            "../textures/skybox/Daylight Box_Left.bmp",
            "../textures/skybox/Daylight Box_Top.bmp",
            "../textures/skybox/Daylight Box_Bottom.bmp",
            "../textures/skybox/Daylight Box_Front.bmp",
            "../textures/skybox/Daylight Box_Back.bmp",
        };        
        
        const char* nightPathes[] = {
            "../textures/skyboxNight/right.png",
            "../textures/skyboxNight/left.png",
            "../textures/skyboxNight/top.png",
            "../textures/skyboxNight/bottom.png",
            "../textures/skyboxNight/front.png",
            "../textures/skyboxNight/back.png",
        };

        skyboxTexture_.load(pathes);
        skyboxNightTexture_.load(nightPathes);
        
        loadModels();        
        initStaticModelMatrices();
        
        // Partie 3
        
        // TODO: À ajouter. Aucune modification nécessaire.
        material_.allocate(&defaultMat, sizeof(Material));
        material_.setBindingIndex(0);
        
        lightsData_.dirLight =
        {
            {0.2f, 0.2f, 0.2f, 0.0f},
            {1.0f, 1.0f, 1.0f, 0.0f},
            {0.5f, 0.5f, 0.5f, 0.0f},
            {0.5f, -1.0f, 0.5f, 0.0f}
        };
        
        for (unsigned int i = 0; i < N_STREETLIGHTS; i++)
        {                
            lightsData_.spotLights[i].position = glm::vec4(streetlightLightPositions[i], 0.0f);
            lightsData_.spotLights[i].direction = glm::vec3(0, -1, 0);
            lightsData_.spotLights[i].exponent = 6.0f;
            lightsData_.spotLights[i].openingAngle = 60.f;        
        }
        
        // Initialisation des paramètres de lumière des phares
        
        lightsData_.spotLights[N_STREETLIGHTS].position = glm::vec4(-1.6, 0.64, -0.45, 0.0f);
        lightsData_.spotLights[N_STREETLIGHTS].direction = glm::vec3(-10, -1, 0);
        lightsData_.spotLights[N_STREETLIGHTS].exponent = 4.0f;
        lightsData_.spotLights[N_STREETLIGHTS].openingAngle = 30.f;
            
        lightsData_.spotLights[N_STREETLIGHTS+1].position = glm::vec4(-1.6, 0.64, 0.45, 0.0f);
        lightsData_.spotLights[N_STREETLIGHTS+1].direction = glm::vec3(-10, -1, 0);
        lightsData_.spotLights[N_STREETLIGHTS+1].exponent = 4.0f;
        lightsData_.spotLights[N_STREETLIGHTS+1].openingAngle = 30.f;
            
        lightsData_.spotLights[N_STREETLIGHTS+2].position = glm::vec4(1.6, 0.64, -0.45, 0.0f);
        lightsData_.spotLights[N_STREETLIGHTS+2].direction = glm::vec3(10, -1, 0);
        lightsData_.spotLights[N_STREETLIGHTS+2].exponent = 4.0f;
        lightsData_.spotLights[N_STREETLIGHTS+2].openingAngle = 60.f;
                   
        lightsData_.spotLights[N_STREETLIGHTS+3].position = glm::vec4(1.6, 0.64, 0.45, 0.0f);
        lightsData_.spotLights[N_STREETLIGHTS+3].direction = glm::vec3(10, -1, 0);
        lightsData_.spotLights[N_STREETLIGHTS+3].exponent = 4.0f;
        lightsData_.spotLights[N_STREETLIGHTS+3].openingAngle = 60.f;
        
        
        toggleStreetlight();
        updateCarLight();
        
        setLightingUniform();
        
        lights_.allocate(&lightsData_, sizeof(lightsData_));
        lights_.setBindingIndex(1);
        
        CHECK_GL_ERROR;
	}
	

	// Appelée à chaque trame. Le buffer swap est fait juste après.
	void drawFrame() override
	{
	    CHECK_GL_ERROR;
	    // TODO: Partie 2: Ajouter le nettoyage du tampon de stencil
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
        
        ImGui::Begin("Scene Parameters");
        ImGui::Combo("Scene", &currentScene_, SCENE_NAMES, N_SCENE_NAMES);
        
        // TODO: À ajouter.
        // Et oui, il est désormais possible de recharger les shaders en gardant l'application ouvert.
        if (ImGui::Button("Reload Shaders"))
        {
            CHECK_GL_ERROR;
            edgeEffectShader_.reload();
            celShadingShader_.reload();
            skyShader_.reload();
            
            setLightingUniform();
            CHECK_GL_ERROR;
        }
        ImGui::End();
        
        switch (currentScene_)
        {
            case 0: sceneMain(); break;
        }
        CHECK_GL_ERROR;
    }


	// TODO: À supprimer, tout ce qui gère le chargement des shaders.
	//       Le chargement est fait dans la classe ShaderProgram.
    
    
    void loadModels()
    {
        car_.loadModels();
        tree_.load("../models/pine.ply");
        streetlight_.load("../models/streetlight.ply");
        streetlightLight_.load("../models/streetlight_light.ply");
        skybox_.load("../models/skybox.ply");
        
        // TODO: Ajouter le chargement du sol et de la route avec la nouvelle méthode load
        //       des modèles. Voir "model_data.hpp".

        grass_.load(ground, sizeof(ground), planeElements, sizeof(planeElements));
        street_.load(street, sizeof(street), planeElements, sizeof(planeElements));
        streetcorner_.load(streetcorner, sizeof(streetcorner), planeElements, sizeof(planeElements));
    }
    
    void initStaticModelMatrices()
    {
        //arbres
        treeModelMatrices_[0] = glm::mat4(1.0f);
        treeModelMatrices_[0] = glm::translate(treeModelMatrices_[0], glm::vec3(0.0f, -0.15f, 0.0f));
        treeModelMatrices_[0] = glm::scale(treeModelMatrices_[0], glm::vec3(15.0f));

        //patch de routes

        const float TOTAL_LENGTH = 30.f;
        const float NUM_SEGMENTS = 7.0F;
        const float SEGMENT_LENGTH = TOTAL_LENGTH / NUM_SEGMENTS;

        const float ROAD_WIDTH = 5.0f;
        const float HALF_LENGTH = TOTAL_LENGTH / 2.0f;
        const float ROAD_Y = 0.0f;
        const float CORNER_Y = 0.01f;
        const float LAMP_Y = -0.15f;
        const float LAMP_OFFSET = 0.5F;
        const float INNER_EDGE = HALF_LENGTH - (ROAD_WIDTH * 0.5f);
        const float INNER_POSITION = INNER_EDGE - LAMP_OFFSET;
        const float BASE_YAW = -90.0f;

        const float l0 = -HALF_LENGTH / 2.0f;
        const float l1 = HALF_LENGTH / 2.0f;

        unsigned int lampidx = 0;
        unsigned int idx = 0;

        for (unsigned int i = 0; i < 7; ++i) {

            const float t = -HALF_LENGTH + (i + 0.5f) * SEGMENT_LENGTH;

            //route du haut
            glm::mat4 roadSegment = glm::mat4(1.0f);
            roadSegment = glm::translate(roadSegment, glm::vec3(t, ROAD_Y, -HALF_LENGTH));
            roadSegment = glm::scale(roadSegment, glm::vec3(SEGMENT_LENGTH, 1.0f, ROAD_WIDTH));
            streetPatchesModelMatrices_[idx++] = roadSegment;

            //route du bas
            roadSegment = glm::mat4(1.0f);
            roadSegment = glm::translate(roadSegment, glm::vec3(t, ROAD_Y, HALF_LENGTH));
            roadSegment = glm::scale(roadSegment, glm::vec3(SEGMENT_LENGTH, 1.0f, ROAD_WIDTH));
            streetPatchesModelMatrices_[idx++] = roadSegment;

            //route droite
            roadSegment = glm::mat4(1.0f);
            roadSegment = glm::translate(roadSegment, glm::vec3(HALF_LENGTH, ROAD_Y, t));
            roadSegment = glm::rotate(roadSegment, glm::radians(90.0f), glm::vec3(0, 1, 0));
            roadSegment = glm::scale(roadSegment, glm::vec3(SEGMENT_LENGTH, 1.0f, ROAD_WIDTH));
            streetPatchesModelMatrices_[idx++] = roadSegment;

            //route gauche
            roadSegment = glm::mat4(1.0f);
            roadSegment = glm::translate(roadSegment, glm::vec3(-HALF_LENGTH, ROAD_Y, t));
            roadSegment = glm::rotate(roadSegment, glm::radians(90.0f), glm::vec3(0, 1, 0));
            roadSegment = glm::scale(roadSegment, glm::vec3(SEGMENT_LENGTH, 1.0f, ROAD_WIDTH));
            streetPatchesModelMatrices_[idx++] = roadSegment;
        }

        auto makeCorner = [&](float x, float z)
            {
                glm::mat4 corner = glm::mat4(1.0f);
                corner = glm::translate(corner, glm::vec3(x, CORNER_Y, z));
                corner = glm::scale(corner, glm::vec3(ROAD_WIDTH, 1.0f, ROAD_WIDTH));
                return corner;
            };

        streetPatchesModelMatrices_[idx++] = makeCorner(-HALF_LENGTH, -HALF_LENGTH);
        streetPatchesModelMatrices_[idx++] = makeCorner(HALF_LENGTH, -HALF_LENGTH);
        streetPatchesModelMatrices_[idx++] = makeCorner(HALF_LENGTH, HALF_LENGTH);
        streetPatchesModelMatrices_[idx++] = makeCorner(-HALF_LENGTH, HALF_LENGTH);

        auto pushLamp = [&](glm::vec3 pos, float degrees)
            {
                glm::mat4 lamp = glm::mat4(1.0f);
                lamp = glm::translate(lamp, pos);
                lamp = glm::rotate(lamp, glm::radians(degrees + BASE_YAW), glm::vec3(0, 1, 0));

                streetlightModelMatrices_[lampidx++] = lamp;
                streetlightLightPositions[lampidx - 1] = glm::vec3(streetlightModelMatrices_[lampidx -1] * glm::vec4(-2.77, 5.2, 0.0, 1.0));

            };

        // lampes du haut
        pushLamp(glm::vec3(l0, LAMP_Y, -INNER_POSITION), 0.0f);
        pushLamp(glm::vec3(l1, LAMP_Y, -INNER_POSITION), 0.0f);

        // lampes du bas
        pushLamp(glm::vec3(l0, LAMP_Y, +INNER_POSITION), 180.0f);
        pushLamp(glm::vec3(l1, LAMP_Y, +INNER_POSITION), 180.0f);

        // lampes droites
        pushLamp(glm::vec3(+INNER_POSITION, LAMP_Y, l0), -90.0f);
        pushLamp(glm::vec3(+INNER_POSITION, LAMP_Y, l1), -90.0f);

        // lampes gauche
        pushLamp(glm::vec3(-INNER_POSITION, LAMP_Y, l0), 90.0f);
        pushLamp(glm::vec3(-INNER_POSITION, LAMP_Y, l1), 90.0f);
    }
    
    // TODO: À modifier, ajouter les textures, et l'effet de contour.
    //       De plus, le modèle a été séparé en deux (pour la partie 3), adapter
    //       votre code pour faire le dessin des deux parties.
    void drawStreetlights(glm::mat4& projView, glm::mat4& view)
    {
        glDisable(GL_CULL_FACE);

        for (unsigned int i = 0; i < N_STREETLIGHTS; i++)
        {
            glm::mat4 mvp = projView * streetlightModelMatrices_[i];

            if (!isDay_)
                setMaterial(streetlightLightMat);
            else
                setMaterial(streetlightMat);
            glActiveTexture(GL_TEXTURE0);
            streetlightLightTexture_.use();
            celShadingShader_.setMatrices(mvp, view, streetlightModelMatrices_[i]);
            streetlightLight_.draw();

            setMaterial(streetlightMat);
            streetlightTexture_.use();
            celShadingShader_.setMatrices(mvp, view, streetlightModelMatrices_[i]);
            streetlight_.draw();
        }
    }
    
    // TODO: À modifier, ajouter les textures, et l'effet de contour.
    void drawTrees(glm::mat4& projView, glm::mat4& view)
    {
        glDisable(GL_CULL_FACE);
        setMaterial(grassMat);
        glActiveTexture(GL_TEXTURE0);
        treeTexture_.use();
        for (unsigned int i = 0; i < N_TREES; i++)
        {
            glm::mat4 mvp = projView * treeModelMatrices_[i];
            celShadingShader_.setMatrices(mvp, view, treeModelMatrices_[i]);
            tree_.draw();
        }
        glEnable(GL_CULL_FACE);
    }
    
    void drawGround(glm::mat4& projView, glm::mat4& view)
    {
        setMaterial(streetMat);
        glActiveTexture(GL_TEXTURE0);
        for (unsigned int i = 0; i < N_STREET_PATCHES; i++) {
            glm::mat4 mvp = projView * streetPatchesModelMatrices_[i];
            celShadingShader_.setMatrices(mvp, view, streetPatchesModelMatrices_[i]);
            if (i < 7u * 4u) {
                streetTexture_.use();
                street_.draw();
            }
            else{
                streetcornerTexture_.use();
                streetcorner_.draw();
            }
        }        

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -0.1f, 0.0f));
        model = glm::scale(model, glm::vec3(50.0f, -1.0f, 50.0f));
        setMaterial(grassMat);
        glActiveTexture(GL_TEXTURE0);
        grassTexture_.use();
        glm::mat4 mvp = projView * model;
        celShadingShader_.setMatrices(mvp, view, model);
        grass_.draw();
    }

    void drawSkyBox(glm::mat4& proj, glm::mat4& view) {

        glm::mat4 skyView = glm::mat4(glm::mat3(view));
        glm::mat4 mvp = proj * skyView;


        glDepthFunc(GL_LEQUAL);
        skyShader_.use();
        glUniformMatrix4fv(skyShader_.mvpULoc, 1, GL_FALSE, glm::value_ptr(mvp));
        glActiveTexture(GL_TEXTURE0);

        if (isDay_) {
            skyboxTexture_.use();
        }
        else {
            skyboxNightTexture_.use();
        }
        skybox_.draw();
        glDepthFunc(GL_LESS);

        celShadingShader_.use();

    }
    
    // Solution
    glm::mat4 getViewMatrix()
    {
        glm::mat4 view = glm::mat4(1.0);                
        view = glm::rotate(view, -cameraOrientation_.x, glm::vec3(1.0, 0.0, 0.0));
        view = glm::rotate(view, -cameraOrientation_.y, glm::vec3(0.0, 1.0, 0.0));
        view = glm::translate(view, -cameraPosition_);
        return view;
    }
    
    // TODO: À ajouter. Pas de modification.
    void setLightingUniform()
    {
        celShadingShader_.use();
        glUniform1i(celShadingShader_.nSpotLightsULoc, N_STREETLIGHTS+4);
        
        float ambientIntensity = 0.05;
        glUniform3f(celShadingShader_.globalAmbientULoc, ambientIntensity, ambientIntensity, ambientIntensity);
    }
    
    // TODO: À ajouter. Pas de modification.
    void toggleSun()
    {
        if (isDay_)
        {
            lightsData_.dirLight.ambient = glm::vec4(0.2f, 0.2f, 0.2f, 0.0f); 
            lightsData_.dirLight.diffuse = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
            lightsData_.dirLight.specular = glm::vec4(0.5f, 0.5f, 0.5f, 0.0f);
        }
        else
        {
            lightsData_.dirLight.ambient = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f); 
            lightsData_.dirLight.diffuse = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
            lightsData_.dirLight.specular = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
        }
    }
    
    // TODO: À ajouter. Pas de modification.
    void toggleStreetlight()
    {
        if (isDay_)
        {
            for (unsigned int i = 0; i < N_STREETLIGHTS; i++)
            {
                lightsData_.spotLights[i].ambient = glm::vec4(glm::vec3(0.0f), 0.0f);
                lightsData_.spotLights[i].diffuse = glm::vec4(glm::vec3(0.0f), 0.0f);
                lightsData_.spotLights[i].specular = glm::vec4(glm::vec3(0.0f), 0.0f);
            }
        }
        else
        {
            for (unsigned int i = 0; i < N_STREETLIGHTS; i++)
            {
                lightsData_.spotLights[i].ambient = glm::vec4(glm::vec3(0.02f), 0.0f);
                lightsData_.spotLights[i].diffuse = glm::vec4(glm::vec3(0.8f), 0.0f);
                lightsData_.spotLights[i].specular = glm::vec4(glm::vec3(0.4f), 0.0f);
            }
        }
    }
    
    // TODO: À ajouter.
    void updateCarLight()
    {
        if (car_.isHeadlightOn)
        {
            lightsData_.spotLights[N_STREETLIGHTS].ambient = glm::vec4(glm::vec3(0.01), 0.0f);
            lightsData_.spotLights[N_STREETLIGHTS].diffuse = glm::vec4(glm::vec3(1.0), 0.0f);
            lightsData_.spotLights[N_STREETLIGHTS].specular = glm::vec4(glm::vec3(0.4), 0.0f);
            
            lightsData_.spotLights[N_STREETLIGHTS+1].ambient = glm::vec4(glm::vec3(0.01), 0.0f);
            lightsData_.spotLights[N_STREETLIGHTS+1].diffuse = glm::vec4(glm::vec3(1.0), 0.0f);
            lightsData_.spotLights[N_STREETLIGHTS+1].specular = glm::vec4(glm::vec3(0.4), 0.0f);
            
            // TODO: Partie 3.
            //       Utiliser car_.carModel pour calculer la nouvelle position et orientation de la lumière.
            //       La lumière devrait suivre le véhicule qui se déplace.
            
            lightsData_.spotLights[N_STREETLIGHTS].position = glm::vec4(-1.6, 0.64, -0.45, 1.0f);
            lightsData_.spotLights[N_STREETLIGHTS].direction = glm::vec3(-10, -1, 0);
            
            lightsData_.spotLights[N_STREETLIGHTS+1].position = glm::vec4(-1.6, 0.64, 0.45, 1.0f);
            lightsData_.spotLights[N_STREETLIGHTS+1].direction = glm::vec3(-10, -1, 0);
        }
        else
        {
            lightsData_.spotLights[N_STREETLIGHTS].ambient = glm::vec4(0.0f);
            lightsData_.spotLights[N_STREETLIGHTS].diffuse = glm::vec4(0.0f);
            lightsData_.spotLights[N_STREETLIGHTS].specular = glm::vec4(0.0f);
            
            lightsData_.spotLights[N_STREETLIGHTS+1].ambient = glm::vec4(0.0f);
            lightsData_.spotLights[N_STREETLIGHTS+1].diffuse = glm::vec4(0.0f);
            lightsData_.spotLights[N_STREETLIGHTS+1].specular = glm::vec4(0.0f);
        }
        
        if (car_.isBraking)
        {
            lightsData_.spotLights[N_STREETLIGHTS+2].ambient = glm::vec4(0.01, 0.0, 0.0, 0.0f);
            lightsData_.spotLights[N_STREETLIGHTS+2].diffuse = glm::vec4(0.9, 0.1, 0.1, 0.0f);
            lightsData_.spotLights[N_STREETLIGHTS+2].specular = glm::vec4(0.35, 0.05, 0.05, 0.0f);
            
            lightsData_.spotLights[N_STREETLIGHTS+3].ambient = glm::vec4(0.01, 0.0, 0.0, 0.0f);
            lightsData_.spotLights[N_STREETLIGHTS+3].diffuse = glm::vec4(0.9, 0.1, 0.1, 0.0f);
            lightsData_.spotLights[N_STREETLIGHTS+3].specular = glm::vec4(0.35, 0.05, 0.05, 0.0f);
            
            // TODO: Partie 3.
            //       Utiliser car_.carModel pour calculer la nouvelle position et orientation de la lumière.
            //       La lumière devrait suivre le véhicule qui se déplace.
            
            lightsData_.spotLights[N_STREETLIGHTS+2].position = glm::vec4(1.6, 0.64, -0.45, 1.0f);        
            lightsData_.spotLights[N_STREETLIGHTS+2].direction = glm::vec3(10, -1, 0);
            
            lightsData_.spotLights[N_STREETLIGHTS+3].position = glm::vec4(1.6, 0.64, 0.45, 1.0f);
            lightsData_.spotLights[N_STREETLIGHTS+3].direction = glm::vec3(10, -1, 0);
        }
        else
        {
            lightsData_.spotLights[N_STREETLIGHTS+2].ambient = glm::vec4(0.0f);
            lightsData_.spotLights[N_STREETLIGHTS+2].diffuse = glm::vec4(0.0f);
            lightsData_.spotLights[N_STREETLIGHTS+2].specular = glm::vec4(0.0f);
            
            lightsData_.spotLights[N_STREETLIGHTS+3].ambient = glm::vec4(0.0f);
            lightsData_.spotLights[N_STREETLIGHTS+3].diffuse = glm::vec4(0.0f);
            lightsData_.spotLights[N_STREETLIGHTS+3].specular = glm::vec4(0.0f);
        }
    }


    
    // TODO: À ajouter. Pas de modification.
    void setMaterial(Material& mat)
    {
        // Ça vous donne une idée de comment utiliser les ubo dans car.cpp.
        material_.updateData(&mat, 0, sizeof(Material));
    }

    glm::mat4 getPerspectiveProjectionMatrix()
    {
        float fov = glm::radians(70.0f);
        float aspect = (float)window_.getSize().x / (float)window_.getSize().y;
        float near = 0.1f;
        float far = 300.0f;

        return glm::perspective(fov, aspect, near, far);
    }

    void onMouseMove(const sf::Event::MouseMoved& mouseDelta) override
    {
        if (isMouseMotionEnabled_)
            return;

    }

    void updateCameraInput()
    {
        if (!window_.hasFocus())
            return;

        const float KEYBOARD_MOUSE_SENSITIVITY = 1.5f;

        float cameraMouvementX = 0;
        float cameraMouvementY = 0;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
            cameraMouvementX -= KEYBOARD_MOUSE_SENSITIVITY;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
            cameraMouvementX += KEYBOARD_MOUSE_SENSITIVITY;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
            cameraMouvementY -= KEYBOARD_MOUSE_SENSITIVITY;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
            cameraMouvementY += KEYBOARD_MOUSE_SENSITIVITY;

        cameraOrientation_.y -= cameraMouvementY * deltaTime_;
        cameraOrientation_.x -= cameraMouvementX * deltaTime_;

        if (isMouseMotionEnabled_)
        {
            sf::Vector2u windowSize = window_.getSize();
            sf::Vector2i windowCenter(windowSize.x / 2, windowSize.y / 2);
            sf::Vector2i mousePos = sf::Mouse::getPosition(window_);

            sf::Vector2i delta = mousePos - windowCenter;

            const float MOUSE_SENSITIVITY = 0.002f;
            cameraOrientation_.y -= delta.x * MOUSE_SENSITIVITY;
            cameraOrientation_.x -= delta.y * MOUSE_SENSITIVITY;

            sf::Mouse::setPosition(windowCenter, window_);
        }

        glm::vec3 positionOffset = glm::vec3(0.0f);
        const float SPEED = 10.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
            positionOffset.z -= SPEED;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
            positionOffset.z += SPEED;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
            positionOffset.x -= SPEED;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
            positionOffset.x += SPEED;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q))
            positionOffset.y -= SPEED;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E))
            positionOffset.y += SPEED;

        positionOffset = glm::rotate(glm::mat4(1.0f), cameraOrientation_.y, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(positionOffset, 1.0f);
        cameraPosition_ += positionOffset * glm::vec3(deltaTime_);
    }
    
    // TODO: À ajouter et modifier.
    //       Ajouter les textures, les skyboxes, les fenêtres de la voiture,
    //       les effets de contour, etc.
    void sceneMain()
    {    
        ImGui::Begin("Scene Parameters");
        if (ImGui::Button("Toggle Day/Night"))
        {
            isDay_ = !isDay_;
            toggleSun();
            toggleStreetlight();
            lights_.updateData(&lightsData_, 0, sizeof(DirectionalLight) + N_STREETLIGHTS * sizeof(SpotLight));
        }
        ImGui::SliderFloat("Car Speed", &car_.speed, -10.0f, 10.0f, "%.2f m/s");
        ImGui::SliderFloat("Steering Angle", &car_.steeringAngle, -30.0f, 30.0f, "%.2f°");
        if (ImGui::Button("Reset Steering"))
            car_.steeringAngle = 0.f;
        ImGui::Checkbox("Headlight", &car_.isHeadlightOn);
        ImGui::Checkbox("Left Blinker", &car_.isLeftBlinkerActivated);
        ImGui::Checkbox("Right Blinker", &car_.isRightBlinkerActivated);
        ImGui::Checkbox("Brake", &car_.isBraking);
        ImGui::End();
    
        updateCameraInput();
        car_.update(deltaTime_);
        
        updateCarLight();
        lights_.updateData(&lightsData_.spotLights[N_STREETLIGHTS], sizeof(DirectionalLight) + N_STREETLIGHTS * sizeof(SpotLight), 4 * sizeof(SpotLight));
                
        glm::mat4 view = getViewMatrix();
        glm::mat4 proj = getPerspectiveProjectionMatrix();
        glm::mat4 projView = proj * view;

        celShadingShader_.use();   

        drawGround(projView, view);
        setMaterial(grassMat);

        drawTrees(projView, view);

        setMaterial(streetlightMat);
        drawStreetlights(projView, view);

        setMaterial(defaultMat);
        car_.draw(projView, view);

        setMaterial(windowMat);
        car_.drawWindows(projView, view);

        drawSkyBox(proj, view);
    }
    
private:
    // TODO: À ajouter.

    // Shaders
    EdgeEffect edgeEffectShader_;
    CelShading celShadingShader_;
    Sky skyShader_;
    
    // Textures
    Texture2D grassTexture_;
    Texture2D streetTexture_;
    Texture2D streetcornerTexture_;
    Texture2D carTexture_;
    Texture2D carWindowTexture_;
    Texture2D treeTexture_;
    Texture2D streetlightTexture_;
    Texture2D streetlightLightTexture_;
    TextureCubeMap skyboxTexture_;
    TextureCubeMap skyboxNightTexture_;
    
    // Uniform buffers
    UniformBuffer material_;
    UniformBuffer lights_;
    
    struct {
        DirectionalLight dirLight;
        SpotLight spotLights[16];
    } lightsData_;
    
    bool isDay_;
    
    Model tree_;
    Model streetlight_;
    Model streetlightLight_;
    Model grass_;
    Model street_;
    Model streetcorner_;
    Model skybox_;
    
    Car car_;
    
    glm::vec3 cameraPosition_;
    glm::vec2 cameraOrientation_;
    
    static constexpr unsigned int N_TREES = 1;
    static constexpr unsigned int N_STREETLIGHTS = 8;
    static constexpr unsigned int N_STREET_PATCHES = 7 * 4 + 4;
    glm::mat4 treeModelMatrices_[N_TREES];
    glm::mat4 streetlightModelMatrices_[N_STREETLIGHTS];
    glm::vec3 streetlightLightPositions[N_STREETLIGHTS];
    glm::mat4 streetPatchesModelMatrices_[N_STREET_PATCHES];

    // Imgui var
    const char* const SCENE_NAMES[1] = {
        "Main scene"
    };
    const int N_SCENE_NAMES = sizeof(SCENE_NAMES) / sizeof(SCENE_NAMES[0]);
    int currentScene_;
    
    bool isMouseMotionEnabled_;
};


int main(int argc, char* argv[])
{
	WindowSettings settings = {};
	settings.fps = 60;
	settings.context.depthBits = 24;
	settings.context.stencilBits = 8;
	settings.context.antiAliasingLevel = 4;
	settings.context.majorVersion = 3;
	settings.context.minorVersion = 3;
	settings.context.attributeFlags = sf::ContextSettings::Attribute::Core;

	App app;
	app.run(argc, argv, "Tp2", settings); // TODO: Modifier le nom, c'est le tp2 maintenant ;)
}
