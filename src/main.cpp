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

#include "happly.h"
#include <imgui/imgui.h>

#include <inf2705/OpenGLApplication.hpp>

#include "model.hpp"
#include "car.hpp"

#define CHECK_GL_ERROR printGLError(__FILE__, __LINE__)

using namespace gl;
using namespace glm;

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;
};

struct App : public OpenGLApplication
{
    App()
        : nSide_(5)
        , oldNSide_(0)
        , cameraPosition_(0.f, 0.f, 0.f)
        , cameraOrientation_(0.f, 0.f)
        , currentScene_(0)
        , isMouseMotionEnabled_(false)
    {
        basicSP_ = 0;
        transformSP_ = 0;
        colorModUniformLocation_ = 0;
        mvpUniformLocation_ = 0;

        vbo_ = 0;
        ebo_ = 0;
        vao_ = 0;
    }

    void checkShaderCompilingError(const char* name, GLuint id)
    {
        GLint success;
        GLchar infoLog[1024];

        glGetShaderiv(id, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(id, 1024, NULL, infoLog);
            glDeleteShader(id);
            std::cout << "Shader \"" << name << "\" compile error: " << infoLog << std::endl;
        }
    }

    void checkProgramLinkingError(const char* name, GLuint id)
    {
        GLint success;
        GLchar infoLog[1024];

        glGetProgramiv(id, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(id, 1024, NULL, infoLog);
            glDeleteProgram(id);
            std::cout << "Program \"" << name << "\" linking error: " << infoLog << std::endl;
        }
    }

    void init() override
    {
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

        glClearColor(0.7f, 0.7f, 0.7f, 1.0f);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        loadShaderPrograms();
        initShapeData();
        loadModels();
        initStaticMatrices();

        car_.position = glm::vec3(0.0f, 0.0f, 13.5f);
        car_.orientation = glm::vec3(0.0f, 0.0f, 0.0f);

        cameraPosition_ = glm::vec3(0.f, 5.f, 25.f);
        cameraOrientation_ = glm::vec2(-0.25f, 0.f); 
    }

    void initStaticMatrices()
    {
        // herbe
        groundModelMatrice_ = glm::mat4(1.0f);
        groundModelMatrice_ = glm::translate(groundModelMatrice_, glm::vec3(0.0f, -0.1f, 0.0f));
        groundModelMatrice_ = glm::scale(groundModelMatrice_, glm::vec3(50.0f, 1.0f, 50.0f));

        //arbres
        treeModelMatrice_ = glm::mat4(1.0f);
        treeModelMatrice_ = glm::translate(treeModelMatrice_, glm::vec3(0.0f, -0.15f, 0.0f));
        treeModelMatrice_ = glm::scale(treeModelMatrice_, glm::vec3(15.0f));

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

    void drawFrame() override
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui::Begin("Scene Parameters");
        ImGui::Combo("Scene", &currentScene_, SCENE_NAMES, N_SCENE_NAMES);
        ImGui::End();

        switch (currentScene_)
        {
        case 0: sceneShape();  break;
        case 1: sceneModels(); break;
        }
    }

    void onClose() override
    {

        if (vbo_) glDeleteBuffers(1, &vbo_);
        if (ebo_) glDeleteBuffers(1, &ebo_);
        if (vao_) glDeleteVertexArrays(1, &vao_);

        if (basicSP_) glDeleteProgram(basicSP_);
        if (transformSP_) glDeleteProgram(transformSP_);
    }

    void onKeyPress(const sf::Event::KeyPressed& key) override
    {
        using enum sf::Keyboard::Key;
        switch (key.code)
        {
        case Escape:
            window_.close();
            break;
        case Space:
            isMouseMotionEnabled_ = !isMouseMotionEnabled_;
            if (isMouseMotionEnabled_)
            {
                window_.setMouseCursorGrabbed(true);
                window_.setMouseCursorVisible(false);
            }
            else
            {
                window_.setMouseCursorGrabbed(false);
                window_.setMouseCursorVisible(true);
            }
            break;
        case T:
            currentScene_ = ++currentScene_ < N_SCENE_NAMES ? currentScene_ : 0;
            break;
        default: break;
        }
    }

    void onResize(const sf::Event::Resized& event) override
    {
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

    void loadModels()
    {
        car_.loadModels();
        
        tree_.load("../models/pine.ply");
        streetlight_.load("../models/streetlight.ply");
        grass_.load("../models/grass.ply");
        street_.load("../models/street.ply");
        streetcorner_.load("../models/streetcorner.ply");
    }

    GLuint loadShaderObject(GLenum type, const char* path)
    {

        std::string src = readFile(path);
        const char* csrc = src.c_str();

        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &csrc, nullptr);
        glCompileShader(shader);

        checkShaderCompilingError(path, shader);
        return shader;
    }

    void loadShaderPrograms()
    {

        // partie 1
        const char* BASIC_VERTEX_SRC_PATH = "./shaders/basic.vs.glsl";
        const char* BASIC_FRAGMENT_SRC_PATH = "./shaders/basic.fs.glsl";

        // partie 2
        const char* TRANSFORM_VERTEX_SRC_PATH = "./shaders/transform.vs.glsl";
        const char* TRANSFORM_FRAGMENT_SRC_PATH = "./shaders/transform.fs.glsl";

        GLuint vs = loadShaderObject(GL_VERTEX_SHADER, BASIC_VERTEX_SRC_PATH);
        GLuint fs = loadShaderObject(GL_FRAGMENT_SHADER, BASIC_FRAGMENT_SRC_PATH);

        basicSP_ = glCreateProgram();
        glAttachShader(basicSP_, vs);
        glAttachShader(basicSP_, fs);
        glLinkProgram(basicSP_);

        checkProgramLinkingError("basicSP", basicSP_);

        glDetachShader(basicSP_, vs);
        glDetachShader(basicSP_, fs);
        glDeleteShader(vs);
        glDeleteShader(fs);
        
        GLuint transVS = loadShaderObject(GL_VERTEX_SHADER, TRANSFORM_VERTEX_SRC_PATH);
        GLuint transFS = loadShaderObject(GL_FRAGMENT_SHADER, TRANSFORM_FRAGMENT_SRC_PATH);

        transformSP_ = glCreateProgram();
        glAttachShader(transformSP_, transVS);
        glAttachShader(transformSP_, transFS);
        glLinkProgram(transformSP_);

        checkProgramLinkingError("transformSP", transformSP_);

        glDetachShader(transformSP_, transVS);
        glDetachShader(transformSP_, transFS);
        glDeleteShader(transVS);
        glDeleteShader(transFS);

        mvpUniformLocation_ = glGetUniformLocation(transformSP_, "uMVP");
        colorModUniformLocation_ = glGetUniformLocation(transformSP_, "uColorMod");


        car_.mvpUniformLocation = mvpUniformLocation_;
        car_.colorModUniformLocation = colorModUniformLocation_;

        
    }

    void generateNgon()
    {
        const float RADIUS = 0.7f;

        vertices_[0].pos = glm::vec2(0.0f, 0.0f);
        vertices_[0].color = glm::vec3(1.0f, 1.0f, 1.0f);

        for (int i = 0; i < nSide_; ++i)
        {
            float a = 2.0f * glm::pi<float>() * (float)i / (float)nSide_;
            float x = RADIUS * std::cos(a);
            float y = RADIUS * std::sin(a);

            vertices_[i + 1].pos = glm::vec2(x, y);

            float t = (float)i / (float)nSide_;
            vertices_[i + 1].color = glm::vec3(
                0.5f + 0.5f * std::cos(a),
                0.5f + 0.5f * std::sin(a),
                1.0f - t
            );
        }

        for (int i = 0; i < nSide_; ++i)
        {
            elements_[i * 3 + 0] = 0;
            elements_[i * 3 + 1] = (GLuint)(i + 1);
            elements_[i * 3 + 2] = (GLuint)((i + 1) % nSide_ + 1);
        }
    }

    void initShapeData()
    {

        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glGenBuffers(1, &ebo_);

        glBindVertexArray(vao_);

        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_), nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements_), nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0, 2, GL_FLOAT, GL_FALSE,
            sizeof(Vertex),
            (void*)offsetof(Vertex, pos)
        );

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1, 3, GL_FLOAT, GL_FALSE,
            sizeof(Vertex),
            (void*)offsetof(Vertex, color)
        );

        glBindVertexArray(0);
    }

    void sceneShape()
    {
        ImGui::Begin("Scene Parameters");
        ImGui::SliderInt("Sides", &nSide_, MIN_N_SIDES, MAX_N_SIDES);
        ImGui::End();

        bool hasNumberOfSidesChanged = nSide_ != oldNSide_;
        if (hasNumberOfSidesChanged)
        {
            oldNSide_ = nSide_;
            generateNgon();

            glBindBuffer(GL_ARRAY_BUFFER, vbo_);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * (nSide_ + 1), vertices_.data());

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(GLuint) * (nSide_ * 3), elements_.data());
        }

        glUseProgram(basicSP_);
        glBindVertexArray(vao_);
        glDrawElements(GL_TRIANGLES, nSide_ * 3, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

    void drawStreetlights(glm::mat4& projView)
    {
        for (unsigned int i = 0; i < N_STREETLIGHTS; i++) {

            glm::mat4 mvp = projView * streetlightModelMatrices_[i];
            glUniformMatrix4fv(mvpUniformLocation_, 1, GL_FALSE, glm::value_ptr(mvp));
            glUniform3f(colorModUniformLocation_, 1.0f, 1.0f, 1.0f);
            streetlight_.draw();
        }

    }

    void drawTree(glm::mat4& projView)
    {
        
        glm::mat4 mvp = projView * treeModelMatrice_;
        glUniformMatrix4fv(mvpUniformLocation_, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniform3f(colorModUniformLocation_, 1.0f, 1.0f, 1.0f);

        GLboolean wasCull = glIsEnabled(GL_CULL_FACE);
        if (wasCull) glDisable(GL_CULL_FACE);
        tree_.draw();        
        if (wasCull) glEnable(GL_CULL_FACE);
    }


    void drawGround(glm::mat4& projView)
    {    
        //grass    
        glm::mat4 mvp = projView * groundModelMatrice_;
        glUniformMatrix4fv(mvpUniformLocation_, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniform3f(colorModUniformLocation_, 1.0f, 1.0f, 1.0f);
        grass_.draw();

        //road and corners
        for (unsigned int i = 0; i < N_STREET_PATCHES; ++i) {
            
            mvp = projView * streetPatchesModelMatrices_[i];
            glUniformMatrix4fv(mvpUniformLocation_, 1, GL_FALSE, glm::value_ptr(mvp));

            if (i < 7u * 4u) street_.draw();
            else streetcorner_.draw();
        }
    }

    glm::mat4 getViewMatrix()
    {

        glm::mat4 camera =  glm::mat4(1.0f);

        camera = glm::rotate(camera, -cameraOrientation_.x, glm::vec3(1.0f, 0.0f, 0.0f));
        camera = glm::rotate(camera, -cameraOrientation_.y, glm::vec3(0.0f, 1.0f, 0.0f));
        camera = glm::translate(camera, -cameraPosition_);

        return camera;
    }

    glm::mat4 getPerspectiveProjectionMatrix()
    {
        float fov = glm::radians(70.0f);
        float aspect = (float)window_.getSize().x / (float)window_.getSize().y;
        float near = 0.1f;
        float far = 300.0f;

        return glm::perspective(fov, aspect, near, far);
    }

    void sceneModels()
    {
        ImGui::Begin("Scene Parameters");
        ImGui::SliderFloat("Car speed", &car_.speed, -10.0f, 10.0f, "%.2f m/s");
        ImGui::SliderFloat("Steering Angle", &car_.steeringAngle, -30.0f, 30.0f, "%.2f°");
        if (ImGui::Button("Reset steering"))
            car_.steeringAngle = 0.f;
        ImGui::Checkbox("Headlight", &car_.isHeadlightOn);
        ImGui::Checkbox("Left Blinker", &car_.isLeftBlinkerActivated);
        ImGui::Checkbox("Right Blinker", &car_.isRightBlinkerActivated);
        ImGui::Checkbox("Brake", &car_.isBraking);
        ImGui::End();

        updateCameraInput();
        car_.update(deltaTime_);

      glm::mat4 projectedView = getPerspectiveProjectionMatrix() * getViewMatrix();

        glUseProgram(transformSP_);

        glUniform3f(colorModUniformLocation_, 1.f, 1.f, 1.f);       


        drawGround(projectedView);
        drawTree(projectedView);
        drawStreetlights(projectedView);

        car_.draw(projectedView);

        glUseProgram(0);
    }

private:
    GLuint basicSP_;
    GLuint transformSP_;
    GLuint colorModUniformLocation_;
    GLuint mvpUniformLocation_;

    GLuint vbo_, ebo_, vao_;

    static constexpr unsigned int MIN_N_SIDES = 5;
    static constexpr unsigned int MAX_N_SIDES = 12;

    std::array<Vertex, MAX_N_SIDES + 1> vertices_;
    std::array<GLuint, MAX_N_SIDES * 3> elements_;

    int nSide_, oldNSide_;

    Model tree_;
    Model streetlight_;
    Model grass_;
    Model street_;
    Model streetcorner_;

    Car car_;

    glm::vec3 cameraPosition_;
    glm::vec2 cameraOrientation_;

    static constexpr unsigned int N_STREETLIGHTS = 2 * 4;
    static constexpr unsigned int N_STREET_PATCHES = 7 * 4 + 4;
    glm::mat4 treeModelMatrice_;
    glm::mat4 groundModelMatrice_;
    glm::mat4 streetlightModelMatrices_[N_STREETLIGHTS];
    glm::mat4 streetPatchesModelMatrices_[N_STREET_PATCHES];

    const char* const SCENE_NAMES[2] = {
        "Introduction",
        "3D Model & transformation",
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
    app.run(argc, argv, "Tp1", settings);
}
