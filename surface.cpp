#include <GL/freeglut.h>
#include <vector>
#include <cmath>
#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Geometry>
#include "surface.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#include "stb_image/stb_image_write.h"

void Light::setPointLight(Vector3f color, Vector3f position) {
    this->lightType = LIGHT_POINT;
    this->color = color;
    this->position = position;
}

void Light::setSunLight(Vector3f color, Vector3f direction) {
    this->lightType = LIGHT_SUN;
    this->color = color;
    this->direction = direction.normalized();
}

void Light::setSpotLight(Vector3f color, Vector3f position, Vector3f direction, float spotSize, float exponent) {
    this->lightType = LIGHT_SPOT;
    this->color = color;
    this->position = position;
    this->direction = direction.normalized();
    this->spotSize = min(max(spotSize, 0.0f), (float) M_PI / 2);
    this->exponent = exponent;
}

Material::Material() {
    setDefaultMaterial();
}

void Material::setDefaultMaterial() {
    this->name = "DefaultMaterial";
    this->Ka = Vector3f(0.0, 0.0, 0.0);
    this->Kd = Vector3f(0.8, 0.8, 0.8);
    this->Ks = Vector3f(0.1, 0.1, 0.1);
    this->Kr = Vector3f(0.0, 0.0, 0.0);
    this->Ns = 128;
    this->Ni = 1.45;
    this->illumType = Material::ILLUM_BASIC;
    this->hasImgKd = false;
}

int Material::loadMaterial(const string &filename, vector<Material> &materials) {
    int index = -1;
    Material material;
    string line;
    ifstream infile(filename);
    istringstream iss;

    if (!infile.is_open()) {
        return -1;
    }

    while (getline(infile, line)) {
        size_t commentIndex = line.find('#');
        if (commentIndex != string::npos) {
            line = line.substr(0, commentIndex);
        }

        iss.str(line);

        string word;
        if (!(iss >> word)) {
            iss.clear();
            continue;
        }

        if (word.compare("newmtl") == 0) {
            if(!(iss >> word)) {
                return -1;
            }
            if (index != -1) {
                materials.push_back(material);
                material.setDefaultMaterial();
            }
            index++;
            material.name = word;
        }
        else if (word.compare("Ns") == 0) {
            float value;
            if (!(iss >> value)) {
                return -1;
            }
            material.Ns = value;
        }
        else if (word.compare("Ni") == 0) {
            float value;
            if (!(iss >> value)) {
                return -1;
            }
            material.Ni = value;
        }
        else if (word.compare("illum") == 0) {
            int value;
            if (!(iss >> value)) {
                return -1;
            }
            material.illumType = (Material::IllumType) value;
        }
        else if (word.compare("Ka") == 0) {
            float v1, v2, v3;
            if (!(iss >> v1 >> v2 >> v3)) {
                return -1;
            }
            material.Ka << v1, v2, v3;
        }
        else if (word.compare("Kd") == 0) {
            float v1, v2, v3;
            if (!(iss >> v1 >> v2 >> v3)) {
                return -1;
            }
            material.Kd << v1, v2, v3;
        }
        else if (word.compare("Ks") == 0) {
            float v1, v2, v3;
            if (!(iss >> v1 >> v2 >> v3)) {
                return -1;
            }
            material.Ks << v1, v2, v3;
        }
        else if (word.compare("Kr") == 0) {
            float v1, v2, v3;
            if (!(iss >> v1 >> v2 >> v3)) {
                return -1;
            }
            material.Kr << v1, v2, v3;
        }
        else if (word.compare("map_Kd") == 0) {
            string imageFilename;
            if (!(iss >> imageFilename)) {
                return -1;
            }
            imageFilename = filename.substr(0, filename.find_last_of("/") + 1) + imageFilename;
            material.hasImgKd = true;
            material.imgKd.loadImage(imageFilename);
        }
        iss.clear();
    }
    materials.push_back(material);
    return 0;
}

void Object::clearModel() {
    // Initialize models
    this->materials.clear();
    this->faceMaterialIndex.clear();
    this->faceVertices.clear();
    this->faceUVs.clear();
    this->faceNormals.clear();
}

int Object::loadModel(const string& filename) {
    string line;
    ifstream infile(filename);
    istringstream iss;

    this->clearModel();
    
    vector<Vector3f> vertices, normals;
    vector<Vector2f> uvs;

    if (!infile.is_open()) {
        return -1;
    }

    vector<Material> materials;
    int materialIndex;

    while (getline(infile, line)) {
        size_t commentIndex = line.find('#');
        if (commentIndex != string::npos) {
            line = line.substr(0, commentIndex);
        }

        iss.str(line);

        string word;
        if (!(iss >> word)) {
            iss.clear();
            continue;
        }

        if (word.compare("mtllib") == 0) {
            // load material file, which can contain multiple materials
            string materialFilename;
            
            if (!(iss >> materialFilename)) {
                return -1;
            }
            
            // materialFilename is relative to the object file
            materialFilename = filename.substr(0, filename.find_last_of("/") + 1) + materialFilename;
            Material::loadMaterial(materialFilename, this->materials);
        }
        else if (word.compare("o") == 0) {
            // New Object
            materialIndex = -1;
        }
        else if (word.compare("v") == 0) {
            // Vertex
            float v1, v2, v3;
            if(!(iss >> v1 >> v2 >> v3)) {
                return -1;
            }
            vertices.push_back(Vector3f(v1, v2, v3));
        }
        else if (word.compare("vt") == 0) {
            // UV map
            float v1, v2;
            if(!(iss >> v1 >> v2)) {
                return -1;
            }
            uvs.push_back(Vector2f(v1, 1 - v2));
        }
        else if (word.compare("vn") == 0) {
            // Vertex normal
            float v1, v2, v3;
            if (!(iss >> v1 >> v2 >> v3)) {
                return -1;
            }
            normals.push_back(Vector3f(v1, v2, v3));
        }
        else if (word.compare("usemtl") == 0) {
            string materialName;
            if (!(iss >> materialName)) {
                materialIndex = -1;
            }
            else for (int i = 0; i < this->materials.size(); i++) {
                if(materialName.compare(this->materials[i].name) == 0) {
                    materialIndex = i;
                }
            }
        }
        else if (word.compare("f") == 0) {
            // Face
            vector<int> vertexIndex;
            vector<int> uvIndex;
            vector<int> normalIndex;
            int v1, v2, v3;

            while((iss >> word)) {
                size_t pos1 = word.find('/');
                if (string::npos == pos1) {
                    return -1;
                }
                size_t pos2 = word.find('/', pos1 + 1);
                v1 = atoi(word.substr(0, pos1).c_str()); 
                v2 = atoi(word.substr(pos1 + 1, pos2).c_str()); 
                v3 = atoi(word.substr(pos2 + 1).c_str()); 
                vertexIndex.push_back(v1);
                uvIndex.push_back(v2);
                normalIndex.push_back(v3);
            }
            for (int i = 2; i < vertexIndex.size(); i++) {
                int p[] = {0, i - 1, i};
                this->faceMaterialIndex.push_back(materialIndex);
                for (int j = 0; j < 3; j++) {
                    this->faceVertices.push_back(vertices[vertexIndex[p[j]] - 1]);
                    this->faceUVs.push_back(uvs[uvIndex[p[j]] - 1]);
                    this->faceNormals.push_back(normals[normalIndex[p[j]] - 1]);
                }
            }
        }
        iss.clear();
    }
    return 0;
}

vector<Vector2f> Section::subdivideSegment(vector<Vector2f> &controlPoints, int level) {
    vector<Vector2f> segment;
    segment.assign(controlPoints.begin(), controlPoints.end());
    for (int d = 0; d < level; d++) {
        vector<Vector2f> nextSegment;
        for (int i = 0; i < segment.size() - 1; i += 3) {
            Vector2f a0 = (segment[i] + segment[i+1]) / 2;
            Vector2f a1 = (segment[i+1] + segment[i+2]) / 2;
            Vector2f a2 = (segment[i+2] + segment[i+3]) / 2;
            Vector2f b0 = (a0 + a1) / 2;
            Vector2f b1 = (a1 + a2) / 2;
            Vector2f c0 = (b0 + b1) / 2;
            nextSegment.push_back(segment[i]);
            nextSegment.push_back(a0);
            nextSegment.push_back(b0);
            nextSegment.push_back(c0);
            nextSegment.push_back(b1);
            nextSegment.push_back(a2);
        }
        nextSegment.push_back(segment.back());
        segment = nextSegment;
    }

    vector<Vector2f> finalSegment;
    for (int i = 0; i < segment.size(); i += 3) { 
        finalSegment.push_back(segment[i]);
    }
    return finalSegment;
}

vector<Section> Section::subdivideSection(vector<Section> &controlSections, int level) {
    vector<Section> segment;
    segment.assign(controlSections.begin(), controlSections.end());
    for (int d = 0; d < level; d++) {
        vector<Section> nextSegment;
        for (int i = 0; i < segment.size() - 1; i += 3) {
            Section a0 = segment[i].getMidSection(segment[i+1]);
            Section a1 = segment[i+1].getMidSection(segment[i+2]);
            Section a2 = segment[i+2].getMidSection(segment[i+3]);
            Section b0 = a0.getMidSection(a1);
            Section b1 = a1.getMidSection(a2);
            Section c0 = b0.getMidSection(b1);
            nextSegment.push_back(segment[i]);
            nextSegment.push_back(a0);
            nextSegment.push_back(b0);
            nextSegment.push_back(c0);
            nextSegment.push_back(b1);
            nextSegment.push_back(a2);
        }
        nextSegment.push_back(segment.back());
        segment = nextSegment;
    }

    vector<Section> finalSegment;
    for (int i = 0; i < segment.size(); i += 3) { 
        finalSegment.push_back(segment[i]);
    }
    return finalSegment;
}

vector<Vector2f> Section::bSplineToBezier(vector<Vector2f>& controlPoints) {
    // Assume circular input and linear output
    int size = controlPoints.size();
    vector<Vector2f> segment;
    for (int i = 0; i < size; i++) {
        Vector2f v0 = controlPoints[i];
        Vector2f v1 = controlPoints[(i + 1) % size];
        Vector2f v2 = controlPoints[(i + 2) % size];
        segment.push_back((v0 + v1 * 4 + v2) / 6);
        segment.push_back((v1 * 4 + v2 * 2) / 6);
        segment.push_back((v1 * 2 + v2 * 4) / 6);
    }
    if (!segment.empty()) segment.push_back(segment[0]);
    return segment;
}

vector<Vector2f> Section::catmullRomToBezier(vector<Vector2f>& controlPoints) {
    // Assume circular input and linear output
    int size = controlPoints.size();
    vector<Vector2f> segment;
    for (int i = 0; i < size; i++) {
        Vector2f v0 = controlPoints[i];
        Vector2f v1 = controlPoints[(i + 1) % size];
        Vector2f v2 = controlPoints[(i + 2) % size];
        Vector2f v3 = controlPoints[(i + 3) % size];
        segment.push_back(v1);
        segment.push_back((v2 - v0) / 6 + v1);
        segment.push_back((v1 - v3) / 6 + v2);
    }
    if (!segment.empty()) segment.push_back(segment[0]);
    return segment;
}

Section::Section() {
    this->scale = 1;
    this->rotation = Quaternionf(1.0f, 0.0f, 0.0f, 0.0f);
    this->position = Vector3f(0.0f, 0.0f, 0.0f);
}

void Section::setControlPoints(vector<Vector2f>& controlPoints) {
    this->controlPoints.assign(controlPoints.begin(), controlPoints.end());
}

Section Section::getMidSection(Section& nextSection) {
    Section midSection;
    for (int i = 0; i < this->controlPoints.size(); i++) {
        midSection.controlPoints.push_back((this->controlPoints[i] + nextSection.controlPoints[i]) / 2);
    }
    midSection.scale = (this->scale + nextSection.scale) * 0.5;
    midSection.rotation = this->rotation.slerp(0.5, nextSection.rotation);
    midSection.position = (this->position + nextSection.position) * 0.5;

    return midSection;
}

Section Section::getCatmullRomControlSection(Section& sLeft, Section& sRight) {
    assert(sLeft.controlPoints.size() == this->controlPoints.size());
    assert(sRight.controlPoints.size() == this->controlPoints.size());
    Section newSection;
    for (int i = 0; i < this->controlPoints.size(); i++) {
        newSection.controlPoints.push_back(
            (sRight.controlPoints[i] - sLeft.controlPoints[i]) / 6 + this->controlPoints[i]
        );
    }
    newSection.scale = (sRight.scale - sLeft.scale) / 6 + this->scale;
    newSection.rotation = sLeft.rotation.slerp(1.0f / 6.0f, sRight.rotation) * sLeft.rotation.inverse() * this->rotation;
    newSection.position = (sRight.position - sLeft.position) / 6 + this->position;

    return newSection;
}

Section Section::getCatmullRomControlSection(Section& sRight) {
    assert(sRight.controlPoints.size() == this->controlPoints.size());
    Section newSection;
    for (int i = 0; i < this->controlPoints.size(); i++) {
        newSection.controlPoints.push_back(
            (sRight.controlPoints[i] - this->controlPoints[i]) / 3 + this->controlPoints[i]
        );
    }
    newSection.scale = (sRight.scale - this->scale) / 3 + this->scale;
    newSection.rotation = (this->rotation).slerp(1.0f / 3.0f, sRight.rotation);
    newSection.position = (sRight.position - this->position) / 3 + this->position;

    return newSection;
}

Section Section::getRenderedSection(int level) {
    Section newSection;
    newSection.controlPoints = Section::subdivideSegment(this->controlPoints, level);
    newSection.scale = this->scale;
    newSection.rotation = this->rotation;
    newSection.position = this->position;

    return newSection;
}

Vector3f Section::getGlobalPosition(int index) {
    Vector3f offset(this->controlPoints[index](0), 0, this->controlPoints[index](1));
    return this->position + this->scale * (this->rotation * Quaternionf(0, offset[0], offset[1], offset[2]) * this->rotation.inverse()).vec();
}

int SweptSurface::loadModel(const string& filename, int level, Material material) {
    string line;
    ifstream infile(filename);

    this->materials.push_back(material);

    if (!infile.is_open()) {
        return -1;
    }

    int headerIndex = 0;
    int numCrossSection;
    int numControlPoint;
    istringstream iss;
    string strTemp;
    vector<float> content;
    CurveType curveType;

    this->sections.clear();

    while (getline(infile, line)) {
        size_t commentIndex = line.find('#');
        if (commentIndex != string::npos) {
            line = line.substr(0, commentIndex);
        }
        iss.str(line);
        while (iss >> strTemp) {
            switch (headerIndex) {
                case 0:
                if (strTemp.compare("BSPLINE") == 0) {
                    curveType = CURVE_BSPLINE;
                }
                else if (strTemp.compare("CATMULL_ROM") == 0) {
                    curveType = CURVE_CATMULL_ROM;
                }
                else {
                    return -1;
                }
                headerIndex++;
                break;
                case 1:
                numCrossSection = atoi(strTemp.c_str());
                headerIndex++;
                break;
                case 2:
                numControlPoint = atoi(strTemp.c_str());
                headerIndex++;
                break;
                default:
                content.push_back(atof(strTemp.c_str()));
                break;
            }
        }
        iss.clear();
    }
    int contentPerCrossSection = numControlPoint * 2 + 8;
    if (numCrossSection * contentPerCrossSection > content.size()) {
        return -1;
    }

    vector<float>::iterator contentIter = content.begin();
    for (int i = 0; i < numCrossSection; i++) {
        Section section;
        vector<Vector2f> controlPoints;
        vector<Vector2f> convertedControlPoints;
        for (int j = 0; j < numControlPoint; j++) {
            controlPoints.push_back(Vector2f(*contentIter, *(contentIter + 1)));
            contentIter += 2;
        }
        if (curveType == CURVE_CATMULL_ROM) {
            convertedControlPoints = Section::catmullRomToBezier(controlPoints);
        }
        else if (curveType == CURVE_BSPLINE) {
            convertedControlPoints = Section::bSplineToBezier(controlPoints);
        }
        section.setControlPoints(convertedControlPoints);

        section.scale = *(contentIter++);
        section.rotation = AngleAxisf(*(contentIter), Vector3f(*(contentIter + 1), *(contentIter + 2), *(contentIter + 3)));
        contentIter += 4;
        section.position = Vector3f(*(contentIter), *(contentIter + 1), *(contentIter + 2));
        contentIter += 3;
        sections.push_back(section);
    }


    vector<Section> renderedSections;
    vector<Section> renderedSplineSections;

    for (int i = 0; i < sections.size(); i++) {
        renderedSections.push_back(sections[i].getRenderedSection(level));
    }

    for (int i = 0; i < sections.size() - 1; i++) {
        renderedSplineSections.push_back(renderedSections[i]);
        if (i == 0) {
            renderedSplineSections.push_back(renderedSections[i].getCatmullRomControlSection(renderedSections[i+1]));
        }
        else {
            renderedSplineSections.push_back(renderedSections[i].getCatmullRomControlSection(renderedSections[i-1], renderedSections[i+1]));
        }
        if (i == sections.size() - 2) {
            renderedSplineSections.push_back(renderedSections[i+1].getCatmullRomControlSection(renderedSections[i]));
        }
        else {
            renderedSplineSections.push_back(renderedSections[i+1].getCatmullRomControlSection(renderedSections[i+2], renderedSections[i]));
        }
    }
    renderedSplineSections.push_back(renderedSections.back());

    renderedSections = Section::subdivideSection(renderedSplineSections, level);

    {
        int numControlPoint;
        int numCrossSection;
        numCrossSection = renderedSections.size();
        if (numCrossSection == 0) return -1;
        numControlPoint = renderedSections[0].controlPoints.size() - 1;
        this->faceVertices.clear();
        this->faceUVs.clear();
        this->faceNormals.clear();
        vector<Vector3f> faceNormals;        
        vector<Vector3f> tmpVertices;
        vector<Vector3f> tmpNormals;
        vector<Vector2f> tmpUVs;
        for (int i = 0; i < numCrossSection; i++) {
            for (int j = 0; j < numControlPoint; j++) {
                Vector3f p = renderedSections[i].getGlobalPosition(j);
                this->faceVertices.push_back(p);
            }
        }
        for (int i = 0; i < numCrossSection - 1; i++) {
            for (int j = 0; j < numControlPoint; j++) {
                Vector3f p1 = this->faceVertices[numControlPoint * i + j];
                Vector3f p2 = this->faceVertices[numControlPoint * i  + (j + 1) % numControlPoint];
                Vector3f p3 = this->faceVertices[numControlPoint * (i + 1)  + j];
                Vector3f p4 = this->faceVertices[numControlPoint * (i + 1)  + (j + 1) % numControlPoint];
                Vector3f diff1 = p2 - p1;
                Vector3f diff2 = p2 - p4;
                Vector3f diff3 = p3 - p4;
                Vector3f diff4 = p3 - p1;
                Vector3f normal = diff3.cross(diff4).normalized() + diff1.cross(diff2).normalized();
                faceNormals.push_back(normal.normalized());
            }
        }
        for (int i = 0; i < numCrossSection; i++) {
            for (int j = 0; j < numControlPoint; j++) {
                Vector3f normal(0, 0, 0);
                int cntNormal = 0;
                if (i != 0) {
                    cntNormal += 2;
                    normal += faceNormals[numControlPoint * (i - 1) + (j + numControlPoint - 1) % numControlPoint];
                    normal += faceNormals[numControlPoint * (i - 1) + j];
                }
                if (i != numCrossSection - 1) {
                    cntNormal += 2;
                    normal += faceNormals[numControlPoint * i + (j + numControlPoint - 1) % numControlPoint];
                    normal += faceNormals[numControlPoint * i + j];
                }
                normal /= cntNormal;
                this->faceNormals.push_back(normal);
            }
        }
        for (int i = 0; i < numCrossSection - 1; i++) {
            for (int j = 0; j < numControlPoint; j++) {
                int index[] = {
                    numControlPoint * i + j,
                    numControlPoint * (i + 1) + j,
                    numControlPoint * (i + 1) + (j + 1) % numControlPoint,
                    numControlPoint * i + j,
                    numControlPoint * (i + 1) + (j + 1) % numControlPoint,
                    numControlPoint * i + (j + 1) % numControlPoint
                };
                this->faceMaterialIndex.push_back(0);
                this->faceMaterialIndex.push_back(0);
                for (int k = 0; k < 6; k++) {
                    tmpNormals.push_back(this->faceNormals[index[k]]);
                    tmpVertices.push_back(this->faceVertices[index[k]]);
                    tmpUVs.push_back(Vector2f(0, 0));
                }
            }
        }
        this->faceNormals = tmpNormals;
        this->faceVertices = tmpVertices;
        this->faceUVs = tmpUVs;
    }

    return 0;
}

void Scene::loadObject(Object& object) {
    for (auto it = object.faceVertices.begin(); it != object.faceVertices.end(); it++) {
        this->faceVertices.push_back(*it);
    }
    for (auto it = object.faceUVs.begin(); it != object.faceUVs.end(); it++) {
        this->faceUVs.push_back(*it);
    }
    for (auto it = object.faceNormals.begin(); it != object.faceNormals.end(); it++) {
        this->faceNormals.push_back(*it);
    }
    for (auto it = object.faceMaterialIndex.begin(); it != object.faceMaterialIndex.end(); it++) {
        this->faceMaterialIndex.push_back(this->materials.size() + (*it));
    }
    for (auto it = object.materials.begin(); it != object.materials.end(); it++) {
        this->materials.push_back(*it);
    }
}

void Scene::loadSphere(Vector3f position, float radius, Material material) {
    loadSphere(position, radius, material, Quaternionf());
}

void Scene::loadSphere(Vector3f position, float radius, Material material, Quaternionf uvOrientation) {
    this->spherePosition.push_back(position);
    this->sphereRadius.push_back(radius);
    this->sphereUV.push_back(uvOrientation);
    this->sphereMaterialIndex.push_back(this->materials.size());
    this->materials.push_back(material);
}

void Scene::loadLight(Light& light) {
    this->lights.push_back(light);
}

void Scene::setBackgroundLight(Vector3f light) {
    this->backgroundLight = light;
}

void Scene::buildBVH() {
    this->bvh.build(this->faceVertices);
}

bool Scene::rayTrace(Vector3f origin, Vector3f direction) {
    float nextParam;
    int nextIndex;
    Vector3f nextNormal;
    Vector2f nextUV;
    return rayTrace(origin, direction, nextParam, nextIndex, nextNormal, nextUV);
}

bool Scene::rayTrace(Vector3f origin, Vector3f direction, float& nextParam) {
    int nextIndex;
    Vector3f nextNormal;
    Vector2f nextUV;
    return rayTrace(origin, direction, nextParam, nextIndex, nextNormal, nextUV);
}

bool Scene::rayTrace(Vector3f origin, Vector3f direction, float& nextParam, int& nextMatIndex, Vector3f& nextNormal, Vector2f& nextUV) {
    vector<Vector3f> verts;
    vector<int> indices;
    vector<BVH*> bvhStack;
    BVH* bvhCurrent = &this->bvh;

    nextMatIndex = -1;

    while (1) {
        // Check intersection
        if (bvhCurrent->checkIntersection(origin, direction)) {
            if (bvhCurrent->isLeaf) {
                indices.insert(indices.end(), bvhCurrent->indices.begin(), bvhCurrent->indices.end());
                verts.insert(verts.end(), bvhCurrent->verts.begin(), bvhCurrent->verts.end());
                if (bvhStack.empty()) break;
                bvhCurrent = bvhStack.back();
                bvhStack.pop_back();
            }
            else {
                bvhStack.push_back(bvhCurrent->childL);
                bvhCurrent = bvhCurrent->childR;
            }
        }
        else {
            if (bvhStack.empty()) break;
            bvhCurrent = bvhStack.back();
            bvhStack.pop_back();
        }
    }

    int index = -1;
    float minU, minV;
    nextParam = numeric_limits<float>::max();

    for (int i = 0; i < verts.size(); i += 3) {
        Vector3f& base = verts[i];
        Vector3f v1 = verts[i+1] - base;
        Vector3f v2 = verts[i+2] - base;
        Vector3f directionV2 = direction.cross(v2);
        float det = v1.dot(directionV2);
        
        if (abs(det) < __FLT_EPSILON__) continue;

        float invDet = 1 / det;

        Vector3f target = origin - base;
        float u = target.dot(directionV2) * invDet;
        if (u < 0 || u > 1) continue;

        Vector3f targetV1 = target.cross(v1);
        float v = direction.dot(targetV1) * invDet;
        if (v < 0 || u + v > 1) continue;

        float t = v2.dot(targetV1) * invDet;
        if (t < nextParam & t > 1e-5) {
            nextParam = t;
            minU = u;
            minV = v;
            index = i;
        }
    }

    if (index >= 0) {
        int faceIndex = indices[index / 3];
        nextMatIndex = this->faceMaterialIndex[faceIndex];
        nextNormal = 
            faceNormals[faceIndex * 3] * (1 - minU - minV) 
            + faceNormals[faceIndex * 3 + 1] * minU
            + faceNormals[faceIndex * 3 + 2] * minV;
        nextNormal.normalize();
        nextUV =
            faceUVs[faceIndex * 3] * (1 - minU - minV)
            + faceUVs[faceIndex * 3 + 1] * minU
            + faceUVs[faceIndex * 3 + 2] * minV;

    }

    // Sphere logic
    for (int i = 0; i < this->sphereRadius.size(); i++) {
        float radius = this->sphereRadius[i];
        Vector3f delta = spherePosition[i] - origin;
        float b = delta.dot(direction);
        float c = delta.dot(delta) - radius * radius;
        float det = b * b - c;
        if (det < 0) {
            // no intersection 
            continue;
        }
        det = sqrt(det);
        float t = b - det;
        if (t > 1e-5 && t < nextParam) {
            nextParam = t;
            nextMatIndex = this->sphereMaterialIndex[i];
            nextNormal = (t * direction - delta).normalized();
            Vector3f orientation = this->sphereUV[i] * nextNormal;
            nextUV << atan2(orientation[1], orientation[0]) / 2 / M_PI, acos(orientation[2]) / M_PI;
        }
        t = b + det;
        if (t > 1e-5 && t < nextParam) {
            nextParam = t;
            nextMatIndex = this->sphereMaterialIndex[i];
            nextNormal = (t * direction - delta).normalized();
            Vector3f orientation = this->sphereUV[i] * nextNormal;
            nextUV << atan2(orientation[1], orientation[0]) / 2 / M_PI, acos(orientation[2]) / M_PI;
        }
    }

    return (nextMatIndex >= 0);
}

bool Scene::raySurface(Material& mat, Vector3f normal, Vector3f incoming, Vector2f uv, Vector3f& outgoing, Vector3f &weight) {
    // Importance sampling
    float p = (float) rand() / RAND_MAX;
    float x = (float) rand() / RAND_MAX;
    float y = (float) rand() / RAND_MAX;
    float yCos = cos(2 * M_PI * y);
    float ySin = sin(2 * M_PI * y);

    float weightDiffuse = mat.Kd.mean();
    float weightSpecular = mat.Ks.mean();
    if (p < weightDiffuse) {
        // Diffuse light
        Vector3f uNormal = normal.unitOrthogonal();
        Vector3f vNormal = normal.cross(uNormal);
        float normalWeight = sqrt(x);
        float notNormalWeight = sqrt(1 - x);
        outgoing = 
            normal * normalWeight + 
            (uNormal * yCos + vNormal * ySin) * notNormalWeight;
        if (mat.hasImgKd) {
            weight = mat.imgKd.getValue(uv) / weightDiffuse;
        }
        else {
            weight = mat.Kd / weightDiffuse;
        }
        return true;
    }
    if (p < weightDiffuse + weightSpecular) {
        // Specular light
        Vector3f ref = (-2 * incoming.dot(normal) * normal + incoming).normalized();
        Vector3f uRef = ref.unitOrthogonal();
        Vector3f vRef = ref.cross(uRef);
        float normalWeight = pow(x, 1.0 / (mat.Ns + 1.0));
        float notNormalWeight = sqrt(1 - normalWeight * normalWeight);
        outgoing = 
            ref * normalWeight + 
            (uRef * yCos + vRef * ySin) * notNormalWeight;
        weight = mat.Ks / weightSpecular;
        return true;
    }
    if (mat.illumType == Material::ILLUM_REFRACTION) {
        float weightRefractive = mat.Kr.mean();
        if (p < weightDiffuse + weightSpecular + weightRefractive) {
            // Refractive light
            float cos1 = normal.dot(-incoming);
            float sin1 = sqrt(max(1 - cos1 * cos1, 0.0f));
            float sin2 = cos1 > 0 ? sin1 / mat.Ni : sin1 * mat.Ni;

            if (sin2 > 1) {
                // Total reflection
                outgoing = incoming - 2 * normal.dot(incoming) * normal;
            }
            else if (cos1 > 0) {
                // Refraction, outside to inside
                outgoing = 1 / mat.Ni * incoming + (1 / mat.Ni * cos1 - sqrt(max(1 - sin2 * sin2, 0.0f))) * normal;
            }
            else {
                // Refraction, inside to outside
                outgoing = mat.Ni * incoming + (mat.Ni * cos1 + sqrt(max(1 - sin2 * sin2, 0.0f))) * normal;
            }
            weight = mat.Kr / weightRefractive;
            return true;
        }       
        return false;
    }

    return false;
}

Vector3f Scene::rayCollect(Material& mat, Vector3f origin, Vector3f normal, Vector3f incoming, Vector2f uv) {
    // Collect from all lights
    Vector3f totalIntensity;
    totalIntensity << 0, 0, 0;

    for (auto it = lights.begin(); it != lights.end(); it++) {
        Light& light = *it;
        Vector3f intensity;
        Vector3f outgoing;
        intensity << 0, 0, 0;

        if (light.lightType == Light::LIGHT_SUN) {
            bool collided = rayTrace(origin, -light.direction);
            if (collided) {
                continue;
            }
            outgoing = -light.direction;
            intensity = light.color;
        }
        else {
            Vector3f vec = light.position - origin;
            outgoing = vec.normalized();
            float dist2 = vec.dot(vec), param;
            bool collided = rayTrace(origin, outgoing, param);
            if (collided && param * param < dist2) {
                continue;
            }
            if (light.lightType == Light::LIGHT_POINT) {
                intensity = light.color / dist2;
            }
            else {
                // LIGHT_SPOT
                float cosAngle = min(light.direction.dot(-outgoing), 1.0f);
                float cosAngleMin = cos(light.spotSize);
                if (cosAngle > cosAngleMin) {
                    intensity = pow(cosAngle, light.exponent) * light.color / dist2;
                }
            }
        }

        // Diffuse
        if (mat.hasImgKd) {
            totalIntensity += max(outgoing.dot(normal), 0.0f) * intensity.cwiseProduct(mat.imgKd.getValue(uv)) / M_PI;
        }
        else {
            totalIntensity += max(outgoing.dot(normal), 0.0f) * intensity.cwiseProduct(mat.Kd) / M_PI;
        }

        // Specular
        Vector3f baseIncoming = -2 * outgoing.dot(normal) * normal + outgoing;
        totalIntensity += (mat.Ns + 2) / 2 / M_PI * pow(max(0.0f, min(1.0f, baseIncoming.dot(incoming))), mat.Ns) * intensity.cwiseProduct(mat.Ks);
    }
    return totalIntensity;
}

Camera::Camera() {
    this->fovy = 50.0f;
    this->width = 160;
    this->height = 90;
    this->sampleRate = 32;
    this->orientation = {1, 0, 0, 0};
    this->position = {10, 0, 0};
    this->fNumber = 9999;
    this->planeDist = 1.0;
    this->focusDist = 10.0;
}

void Camera::sampleImage(Scene &scene, const string& filename) {
    this->renderedImage.clear();
    for (int i = 0; i < this->width * this->height * 3; i++) {
        this->renderedImage.push_back(0);
    }

    float pxDist = tan(this->fovy * M_PI / 360) / this->height * 2;
    Matrix3f rotMat = this->orientation.toRotationMatrix();
    Vector3f cBase = -rotMat.col(2);
    Vector3f cXUnit = rotMat.col(0) * pxDist;
    Vector3f cYUnit = rotMat.col(1) * pxDist;
    int imgIndex = 0;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            for (int k = 0; k < sampleRate; k++) {
                float yPixel = (i - height / 2.0) + (float) random() / RAND_MAX;
                float xPixel = (j - width / 2.0) + (float) random() / RAND_MAX;

                // Perturbation scale (in pixel) for DOF
                float perturbScale = (this->planeDist / pxDist / this->fNumber) * sqrt((float) random() / RAND_MAX);
                float perturbAngle = ((float) random() / RAND_MAX) * 2 * M_PI;
                float xPerturb = perturbScale * cos(perturbAngle); 
                float yPerturb = perturbScale * sin(perturbAngle); 

                // Depth of field calculation
                Vector3f currentDirection = (cBase + cXUnit * xPixel - cYUnit * yPixel).normalized();
                Vector3f focalPoint = this->position + currentDirection * this->focusDist;
                Vector3f currentPosition = this->position + (cBase + cXUnit * (xPixel + xPerturb) - cYUnit * (yPixel + yPerturb)) * this->planeDist;
                currentDirection = (focalPoint - currentPosition).normalized();

                int maxCollision = 12;
                int index;
                float dist;
                Vector3f weight = Vector3f(1, 1, 1);
                Vector3f nextDirection;
                Vector3f normal;
                Vector3f weightMult;
                Vector2f uv;
                
                for (int collision = 1; collision <= maxCollision; collision++) {
                    bool collided = scene.rayTrace(currentPosition, currentDirection, dist, index, normal, uv);
                    if (!collided) {
                        renderedImage[imgIndex + 0] += scene.backgroundLight[0] * weight[0];
                        renderedImage[imgIndex + 1] += scene.backgroundLight[1] * weight[1];
                        renderedImage[imgIndex + 2] += scene.backgroundLight[2] * weight[2];
                        break;
                    }

                    currentPosition += currentDirection * dist;
                    Material& mat = scene.materials[index];
                    Vector3f shadowIntensity = scene.rayCollect(mat, currentPosition, normal, currentDirection, uv);
                    renderedImage[imgIndex + 0] += shadowIntensity[0] * weight[0];
                    renderedImage[imgIndex + 1] += shadowIntensity[1] * weight[1];
                    renderedImage[imgIndex + 2] += shadowIntensity[2] * weight[2];
                    if (!scene.raySurface(mat, normal, currentDirection, uv, nextDirection, weightMult)) {
                        break;
                    }
                    weight = weight.cwiseProduct(weightMult);
                    currentDirection = nextDirection;
                }
            }
            imgIndex += 3;
        }
    }

    vector<unsigned char> finalImage;
    for (auto it = renderedImage.begin(); it != renderedImage.end(); it++) {
        float value = (*it) / this->sampleRate;
        finalImage.push_back(value > 1 ? 0xFF : (unsigned char)(pow(value, 1.0 / 2.2) * 0xFF));
    }

    stbi_write_png(filename.c_str(), width, height, 3, &finalImage[0], width * 3);
}

BVH::BVH() {
    this->box = AlignedBox3f();
    this->childL = NULL;
    this->childR = NULL;
    this->isLeaf = true;
}

BVH::BVH(vector<Vector3f> &v) {
    this->build(v);
}

BVH::BVH(vector<Vector3f> &v, vector<int> &ind) {
    this->build(v, ind);
}

BVH::~BVH() {
    if (!isLeaf) {
        delete childL;
        delete childR;
    }
}

void BVH::clear() {
    if (!isLeaf) {
        delete childL;
        delete childR;
    }
    this->childL = this->childR = NULL;
    this->isLeaf = false;
    this->verts.clear();
    this->indices.clear();
    this->box = AlignedBox3f();
}

void BVH::build(vector<Vector3f> &v) {
    this->clear();

    vector<int> ind;
    for (int i = 0; i * 3 < v.size(); i++) {
        ind.push_back(i);
    }
    this->build(v, ind);
}

void BVH::build(vector<Vector3f> &v, vector<int> &ind) {
    int leafMax = 3;
    this->box = AlignedBox3f();
    for (auto it = v.begin(); it != v.end(); it++) {
        this->box.extend(*it);
    }

    if (ind.size() <= leafMax) {
        this->isLeaf = true;
        this->verts.assign(v.begin(), v.end());
        this->indices.assign(ind.begin(), ind.end());
        this->childL = this->childR = NULL;
        return;
    }

    this->isLeaf = false;
    int axis;
    Vector3f diffPos = this->box.sizes();
    if (diffPos[0] > diffPos[1]) {
        if (diffPos[0] > diffPos[2]) axis = 0;
        else axis = 2;
    }
    else if (diffPos[1] > diffPos[2]) axis = 1;
    else axis = 2;

    vector<Vector3f> vL, vR;
    vector<int> indL, indR;

    float midValue = this->box.center()[axis]; 
    vector<pair<float, int> > monoIndex;
    for (int i = 0; i < v.size(); i += 3) {
        float triMidValue = (v[i] + v[i+1] + v[i+2])[axis] / 3;
        monoIndex.push_back({triMidValue, i});
    }
    sort(monoIndex.begin(), monoIndex.end());

    for (int i = 0; i < monoIndex.size() / 2; i++) {
        for (int j = 0; j < 3; j++) {
            vL.push_back(v[monoIndex[i].second + j]);
        }
        indL.push_back(ind[monoIndex[i].second / 3]);
    }
    for (int i = monoIndex.size() / 2; i < monoIndex.size(); i++) {
        for (int j = 0; j < 3; j++) {
            vR.push_back(v[monoIndex[i].second + j]);
        }
        indR.push_back(ind[monoIndex[i].second / 3]);
    }
    childL = new BVH(vL, indL);
    childR = new BVH(vR, indR);
}

bool BVH::checkIntersection(const Vector3f& origin, const Vector3f& direction) {
    float tMin = numeric_limits<float>::min();
    float tMax = numeric_limits<float>::max();
    Vector3f pointMin = this->box.min();
    Vector3f pointMax = this->box.max();
    for (int i = 0; i < 3; i++) {
        float d = direction[i];
        float o = origin[i];
        float pMin = pointMin[i];
        float pMax = pointMax[i];
        if (abs(d) < __FLT_EPSILON__) {
            if ((pMin > o) || (pMax < o)) {
                return false;
            }
        }
        if (d > 0) {
            tMin = max(tMin, (pMin - o) / d);
            tMax = min(tMax, (pMax - o) / d);
        }
        else {
            tMin = max(tMin, (pMax - o) / d);
            tMax = min(tMax, (pMin - o) / d);
        }
    }
    return tMin <= tMax;
}

UVImage::UVImage() {
    this->width = 1;
    this->height = 1;
    this->data.push_back(Vector3f(0, 0, 0));
}

bool UVImage::loadImage(const string& filename) {
    int w, h, c;
    unsigned char *data = stbi_load(filename.c_str(), &w, &h, &c, 3);
    if (!data) {
        return false;
    }
    this->data.clear();
    this->width = w;
    this->height = h;

    int index = 0;
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) { 
            Vector3f value;
            for (int k = 0; k < 3; k++) {
                value[k] = pow((float) data[index + k] / 0xFF, 2.2);
            }
            this->data.push_back(value);
            index += 3;
        }
    }
    stbi_image_free(data);
    return true;
}

Vector3f UVImage::getValue(Vector2f uv) {
    // Get nearest value
    int x = (((int) (uv[0] * width + 0.5)) % width + width) % width;
    int y = (((int) (uv[1] * height + 0.5)) % height + height) % height;
    return this->data[y * width + x];
}