#pragma once

#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>

using namespace gl;

struct ModelVertex {
    glm::vec3 pos;
    glm::vec3 color;
};

class Model
{
public:
    void load(const char* path);
    
    void load(float* vertices, size_t verticesSize, unsigned int* elements, size_t elementsSize);

    ~Model();
    
    void draw();

private:
    GLuint vao_, vbo_, ebo_;
    GLsizei count_;
};

