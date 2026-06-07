#include "model.hpp"

#include "happly.h"

using namespace gl;


void Model::load(const char* path)
{
    // Chargement des données du fichier .ply.
    // Ne modifier pas cette partie.
    happly::PLYData plyIn(path);

    happly::Element& vertex = plyIn.getElement("vertex");
    std::vector<float> positionX = vertex.getProperty<float>("x");
    std::vector<float> positionY = vertex.getProperty<float>("y");
    std::vector<float> positionZ = vertex.getProperty<float>("z");
    
    std::vector<float> nX = vertex.getProperty<float>("nx");
    std::vector<float> nY = vertex.getProperty<float>("ny");
    std::vector<float> nZ = vertex.getProperty<float>("nz");

    std::vector<float> s = vertex.getProperty<float>("s");
    std::vector<float> t = vertex.getProperty<float>("t");

    std::vector<std::vector<unsigned int>> facesIndices = plyIn.getFaceIndices<unsigned int>();    

    std::vector<ModelVertex> vertices;
    vertices.resize(positionX.size());

    for (size_t i = 0; i < vertices.size(); ++i) {
        vertices[i].pos = glm::vec3(positionX[i], positionY[i], positionZ[i]);
        vertices[i].normal = glm::vec3(nX[i], nY[i], nZ[i]);
        vertices[i].uv = glm::vec2(s[i], t[i]);
    }

    std::vector<unsigned int> indices;
    indices.reserve(facesIndices.size() * 3);

    for (const auto& face : facesIndices) {
        if (face.size() != 3) continue;
        indices.push_back(face[0]);
        indices.push_back(face[1]);
        indices.push_back(face[2]);
    }
    
    count_ = static_cast<GLsizei>(indices.size());
    
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
        vertices.size() * sizeof(ModelVertex),
        vertices.data(),
        GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW);



    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, pos));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, uv));

    glBindVertexArray(0);
}

Model::~Model()
{
    if (ebo_) glDeleteBuffers(1, &ebo_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);

    vao_ = vbo_ = ebo_ = 0;
    count_ = 0;
}

void Model::draw()
{
    if (vao_ == 0 || count_ == 0)
        return;

    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, count_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

