#pragma once
#include <vector>
#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Geometry>

using namespace Eigen;
using namespace std;

class Light {
    // Support point, sun and spot light
    // Default : point light
    public:
    enum LightType {
        LIGHT_POINT,
        LIGHT_SUN,
        LIGHT_SPOT
    };
    
    LightType lightType;
    Vector3f color;
    Vector3f position;
    Vector3f direction;

    float spotSize;
    float exponent;
    
    void setPointLight(Vector3f color, Vector3f position);
    void setSunLight(Vector3f color, Vector3f direction);
    void setSpotLight(Vector3f color, Vector3f position, Vector3f direction, float spotSize, float exponent);
};

class UVImage {
    public:
    // Image for diffuse color
    int width;
    int height;
    vector<Vector3f> data;
    UVImage();
    bool loadImage(const string& filename);
    Vector3f getValue(Vector2f uv);
};

class Material {
    // Based on mtl file format
    // See http://paulbourke.net/dataformats/mtl/

    public:

    string name;

    // Phong shading + refraction
    // Ambient, Diffuse, Specular, Refraction
    Vector3f Ka, Kd, Ks, Kr;

    // Specular exponent
    float Ns;

    // Optical density
    float Ni;

    // UV image
    bool hasImgKd;
    UVImage imgKd;

    // Illumination model (0 ~ 10)
    //
    // Supported version -
    //
    // 3 : Lambertian shading + Phong specular illumination model + Whitted's illumination model
    // 6 : model 3, plus refraction

    enum IllumType {
        ILLUM_BASIC = 3,
        ILLUM_REFRACTION = 6
    };

    IllumType illumType;
    
    // Material(Vector3f Ka, Vector3f Kd, Vector3f Ks, float Ns, float Ni, float Tf, float d, IllumType illumType);
    Material();
    void setDefaultMaterial();

    // Load material from file
    static int loadMaterial(const string &filename, vector<Material> &materials);
};

class Object {
    public:
    vector<Material> materials;

    vector<int> faceMaterialIndex;
    vector<Vector3f> faceVertices;
    vector<Vector3f> faceNormals;
    vector<Vector2f> faceUVs;

    // Load model from file
    void clearModel();
    int loadModel(const string& filename);
};

class Section {
    public:
    vector<Vector2f> controlPoints;
    float scale;
    Quaternionf rotation;
    Vector3f position;

    Section();
    void setControlPoints(vector<Vector2f>& controlPoints);
    Section getMidSection(Section& nextSection);
    Section getCatmullRomControlSection(Section& sLeft, Section& sRight);
    Section getCatmullRomControlSection(Section& sRight);
    Section getRenderedSection(int level);
    Vector3f getGlobalPosition(int index);

    static vector<Vector2f> subdivideSegment(vector<Vector2f> &controlPoints, int level);
    static vector<Section> subdivideSection(vector<Section> &controlSections, int level);
    static vector<Vector2f> bSplineToBezier(vector<Vector2f>& controlPoints);
    static vector<Vector2f> catmullRomToBezier(vector<Vector2f>& controlPoints);
};

class SweptSurface : public Object{
    public:
    enum CurveType {
        CURVE_BSPLINE,
        CURVE_CATMULL_ROM
    };
    vector<Section> sections;
    int loadModel(const string& filename, int level, Material material);
};

class BVH {
    public:
    AlignedBox3f box;
    BVH *childL;
    BVH *childR;
    vector<Vector3f> verts;
    vector<int> indices;
    bool isLeaf;

    BVH();
    BVH(vector<Vector3f> &v);
    BVH(vector<Vector3f> &v, vector<int> &ind);
    ~BVH();
    void clear();
    void build(vector<Vector3f> &v);
    void build(vector<Vector3f> &v, vector<int> &ind);
    bool checkIntersection(const Vector3f& origin, const Vector3f& direction);
};

class Scene {
    public:
    BVH bvh;
    vector<Material> materials;
    vector<Light> lights;
    Vector3f backgroundLight;

    vector<Vector3f> faceVertices;
    vector<Vector3f> faceNormals;
    vector<Vector2f> faceUVs;
    vector<int> faceMaterialIndex;

    vector<Vector3f> spherePosition;
    vector<float> sphereRadius;
    vector<Quaternionf> sphereUV;
    vector<int> sphereMaterialIndex;

    void loadObject(Object& object);
    void loadSphere(Vector3f position, float radius, Material material);
    void loadSphere(Vector3f position, float radius, Material material, Quaternionf uvOrientation);
    void loadLight(Light& light);
    void setBackgroundLight(Vector3f light);
    void buildBVH();
    bool raySurface(Material& mat, Vector3f normal, Vector3f incoming, Vector2f uv, Vector3f& outgoing, Vector3f& weight);
    bool rayTrace(Vector3f origin, Vector3f direction, float& nextParam, int& nextIndex, Vector3f& nextNormal, Vector2f& nextUV);
    bool rayTrace(Vector3f origin, Vector3f direction, float& nextParam);
    bool rayTrace(Vector3f origin, Vector3f direction);
    Vector3f rayCollect(Material& mat, Vector3f origin, Vector3f normal, Vector3f incoming, Vector2f uv);
};

class Camera {
    public:
    int width;
    int height;
    int sampleRate;
    float fovy;
    float focusDist;
    float planeDist;
    float fNumber;
    Eigen::Quaternionf orientation;
    Eigen::Vector3f position;
    vector<float> renderedImage;

    Camera();
    void sampleImage(Scene &scene, const string& filename);
};