#include <GL/freeglut.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Geometry>
#include "surface.hpp"
#define COS(x) cos(x * M_PI / 180.)
#define SIN(x) sin(x * M_PI / 180.)
#define TAN(x) tan(x * M_PI / 180.)

int main(int argc, char** argv) {

	vector<Light> lights;
	vector<Material> materials;

	Scene scene;


	Light light;
	light.setSunLight(Vector3f(1.0, 0.896, 0.623) * 4, Vector3f(-7.26, -0.48, -4.60));
	scene.loadLight(light);
	light.setSpotLight(Vector3f(1.0, 0.03, 0.03) * 50, Vector3f(2.00, -5.15, 6.77), Vector3f(-0.30, 0.51, -0.74), 0.5, 1);
	scene.loadLight(light);
	light.setSunLight(Vector3f(0.296, 0.750, 1.0) * 10, Vector3f(2.26, 0.10, -0.70));
	scene.loadLight(light);
	scene.setBackgroundLight(Vector3f(0.4, 0.4, 0.4));

	Material::loadMaterial("./data/others.mtl", materials);
	scene.loadSphere(Vector3f(-4.96, 0.36, 1.18), 1.18, materials[0]);
	scene.loadSphere(Vector3f(-1.77, 3.14, 1.80), 1.80, materials[0]);
	scene.loadSphere(Vector3f(2.36, 2.85, 0.95), 0.95, materials[0]);
	scene.loadSphere(Vector3f(2.70, -0.13, 0.49), 0.49, materials[1], Quaternionf(AngleAxisf(M_PI * 1.7, Vector3f::UnitZ())));

	Camera camera;
    camera.width = 320 * 6;
    camera.height = 180 * 6;
	camera.sampleRate = 32 * 8;
	camera.orientation = {0.749, 0.508, 0.238, 0.352};
	camera.position = {7.1806, -6.3057, 4.1167};
	camera.fovy = 24.0f;
	camera.fNumber = 2.0;
	camera.planeDist = 0.5;
	camera.focusDist = 8.5;

	Object myObject;
	myObject.loadModel("./data/main.obj");
	scene.loadObject(myObject);

	SweptSurface mySweptSurface;
	mySweptSurface.loadModel("./data/knot.txt", 2, materials[2]);
	scene.loadObject(mySweptSurface);
	
	scene.buildBVH();
	camera.sampleImage(scene, "data/result.png");
	
	return 0;
}
