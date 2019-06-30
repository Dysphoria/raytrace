/*
	Raytrace using openGL and SDL2
		Does a raytrace on the objects given in one of the ray.txt files, outputs the trace to a 512x512 png.
		Can be run on either cpu or gpu by changing the function in main.
*/

#include "stdafx.h"
#include "glfuncs.h"
#include "SDL.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "Program.h"
#include "Buffer.h"
#include "BufferTexture.h"
#include <fstream>
#include <chrono>
#include <vector>
#include <bitset>
#include <typeinfo>
#include <cmath>
#include "pngops.h"

using namespace std;

#define uint uint32_t

struct vec4 {
	float x, y, z, w;
	vec4(float a, float b, float c, float d) {
		this->x = a; this->y = b; this->z = c; this->w = d;
	}
	vec4() {
		x = y = z = w = 0;
	}
	float& operator[](int idx) {
		switch (idx) {
		case 0: return x;
		case 1: return y;
		case 2: return z;
		case 3: return w;
		default: throw runtime_error("?");
		}
	}
};

static_assert(sizeof(vec4) == 16, "X");

ostream& operator<<(ostream& o, const vec4& v) {
	o << "[" << v.x << " " << v.y << " " << v.z << " " << v.w << "]";
	return o;
}

struct mat4 {
	float data[4][4];
	mat4() {}
	mat4(vec4 row1, vec4 row2, vec4 row3, vec4 row4) {
		memcpy(&data[0][0], &row1, sizeof(row1));
		memcpy(&data[1][0], &row2, sizeof(row2));
		memcpy(&data[2][0], &row3, sizeof(row3));
		memcpy(&data[3][0], &row4, sizeof(row4));
	}
};

float dot(vec4 v, vec4 w) {
	float sum = 0;
	for (int i = 0; i<4; ++i)
		sum += v[i] * w[i];
	return sum;
}

vec4 mulSV(float s, vec4 v) {
	vec4 res;
	for (int i = 0; i<4; ++i)
		res[i] = s * v[i];
	return res;
}

vec4 mulVM(vec4 v, mat4 M) {
	vec4 res;
	for (int i = 0; i<4; ++i) {
		float sum = 0;
		for (int j = 0; j<4; ++j) {
			sum += v[j] * M.data[j][i];
		}
		res[i] = sum;
	}
	return res;
}

vec4 sub(vec4 v, vec4 w) {
	vec4 res;
	for (int i = 0; i<4; ++i)
		res[i] = v[i] - w[i];
	return res;
}

vec4 add(vec4 v, vec4 w) {
	vec4 res;
	for (int i = 0; i<4; ++i)
		res[i] = v[i] + w[i];
	return res;
}

float length(vec4 v) {
	return sqrt(dot(v, v));
}

vec4 normalize(vec4 v) {
	float le = length(v);
	vec4 rv = mulSV(1.0 / le, v);
	return rv;
}

vec4 cross(vec4 v, vec4 w) {
	return vec4(
		v[1] * w[2] - v[2] * w[1],
		v[2] * w[0] - v[0] * w[2],
		v[0] * w[1] - v[1] * w[0],
		0
	);
}

struct Object {
	alignas(16)	vec4 data;	//center x,y,z + radius for sphere; plane ABCD
	alignas(16)	vec4 color; //xyz=rgb; w = reflectivity
	alignas(16)	vec4 type;	//0=sphere, 1=plane
};

vec4 reflect(vec4 V, vec4 N) {
	float q = dot(V, N);
	vec4 Q = mulSV(q, N);
	vec4 R = sub(Q, V);
	return add(add(V, R), R);
}

float raySphereIntersect(vec4 e, vec4 v, vec4 centerAndRadius) {
	vec4 center = centerAndRadius; center.w = 1.0;
	float radius = centerAndRadius.w;
	vec4 alpha = sub(e, center);
	float a = dot(v, v);
	if (a == 0)
		return -1.0;
	float b = 2 * dot(alpha, v);
	float c = dot(alpha, alpha) - radius*radius;
	float disc = b*b - 4 * a*c;
	if (disc < 0)
		return -1.0;
	disc = sqrt(disc);
	float t1 = (-b + disc) / (2 * a);
	float t2 = (-b - disc) / (2 * a);
	if (t1 < 0 && t2 < 0)
		return -1;
	else if (t1 < 0)
		return t2;
	else if (t2 < 0)
		return t1;
	else if (t1 <= t2)
		return t1;
	else
		return t2;
}

float rayPlaneIntersect(vec4 e, vec4 v, vec4 pl) {
	float denom = dot(pl, v);
	if (denom == 0.0)
		return -1;
	float numer = -dot(pl, e);
	float t = numer / denom;
	if (t < 0)
		return -1;
	return t;
}

//Todo: convert this to glsl code (the objects array needs to be converted, and closestT and closestO)
void getRayIntersection(vector<Object>& objects, vec4 rayStart, vec4 rayDir,
	float& closestT, int& closestO) {
	closestT = -1;
	closestO = -1;
	for (unsigned i = 0; i<objects.size(); ++i) {
		float t;
		if (objects[i].type.x == 0)
			t = raySphereIntersect(rayStart, rayDir, objects[i].data);
		else
			t = rayPlaneIntersect(rayStart, rayDir, objects[i].data);
		if (t != -1.0 && t > 0.001 && (closestO == -1 || t < closestT)) {
			closestT = t;
			closestO = (int)i;
		}
	}
}

vec4 getColorForRay(vec4 lightPos, vector<Object>& objects, vec4 rayStart, vec4 rayDir) {
	float closestT;
	int closestO;
	getRayIntersection(objects, rayStart, rayDir, closestT, closestO);

	if (closestO != -1) {
		vec4 closestIp = add(rayStart, mulSV(closestT, rayDir));
		vec4 N;
		if (objects[closestO].type.x == 0) {
			//sphere
			N = sub(closestIp, objects[closestO].data);
			N.w = 0;
			N = normalize(N);
		}
		else {
			//plane
			N = objects[closestO].data;
			N.w = 0;
		}

		vec4 L = normalize(sub(lightPos, closestIp));
		float dp = max(0.0f, dot(N, L));
		float sp = 0;
		if (dp > 0) {
			vec4 V = normalize(sub(rayStart, closestIp));
			vec4 R = reflect(L, N);
			sp = dot(V, R);
			if (sp < 0)
				sp = 0;
			sp = pow(sp, 32);
		}
		vec4 col1 = add(mulSV(dp, objects[closestO].color), mulSV(sp, vec4(1, 1, 1, 1)));
		return col1;
	}
	else {
		return vec4(0, 0, 0, 1);
	}
}

void cpuTrace(vec4 lightPos, vector<Object>& objects, Image& img) {
	float fov = 45;
	float d = 1.0 / tan(fov / 180.0*3.14159265358979323);
	int w = img.w;
	int h = img.h;
	for (int ypix = 0; ypix<h; ++ypix) {
		float y = 1 + -2 * ypix / (h - 1.0);
		cout << ypix << " ";
		cout.flush();
		for (int xpix = 0; xpix<w; ++xpix) {
			float x = -1 + 2 * xpix / (w - 1.0);
			vec4 rayDir = vec4(x, y, -d, 0);
			rayDir = normalize(rayDir);
			vec4 col = getColorForRay(lightPos, objects, vec4(0, 0, 0, 1), rayDir);
			int r = col[0] * 255;
			int g = col[1] * 255;
			int b = col[2] * 255;
			if (r > 255) r = 255;
			if (g > 255) g = 255;
			if (b > 255) b = 255;
			int I = ypix*w * 4 + xpix * 4;
			img.data[I++] = r;
			img.data[I++] = g;
			img.data[I++] = b;
			img.data[I++] = 255;  //alpha
		}
	}
	cout << "\n";
}

void gpuTrace(vec4 lightPos, vector<Object>& data, Image& img, int size) {

	//Initialize the program
	Program prog("prog1.cs");
	prog.use();

	//Set the objects size uniform
	glUniform1i(1, size);

	//Set the render light uniform
	glUniform4f(2, lightPos.x, lightPos.y, lightPos.z, lightPos.w);

	//Set the image size uniform, using width twice here for a square image
	glUniform1i(3, img.w);

	//Initialize the objects buffer texture using GL_RGBA32F, RGBA each being one vec4(4 32-bit floats),
	//and size * 3 because each object uses 3 vec4s
	BufferTexture* bufferTex = new BufferTexture(size * 3, GL_RGBA32F);

	//Map the object data from the file to the buffer texture
	Object* objs = (Object*)bufferTex->map();
	for (int i = 0; i < size; i++)
		objs[i] = data[i];

	//Bind the buffer texture for input
	bufferTex->unmap();
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	bufferTex->bind(0);

	//Bind a buffer for the obj values, attached to binding 0 for output
	Buffer* objectBuffer = new Buffer(sizeof(Object) * size);
	objectBuffer->bind(0);

	//Bind a buffer for the image values, since we're using a square image I can
	//skip passing two uniforms for width and height and just do width twice
	Buffer* imageBuffer = new Buffer((sizeof(uint) * img.w * img.w));
	imageBuffer->bind(1);

	//Run the program, no multi-threading here for now
	prog.dispatch(1, 1, 1);

	//Map the imageBuffer values from the gpu
	uint * imgOut = (uint *)imageBuffer->map();

	//Set the image data to the gpu data and we're done
	img.data = (uint8_t *)imgOut;
}

void APIENTRY debugproc(GLenum source, GLenum type, GLenum id,
	GLenum severity, GLint length, const GLchar* message,
	const void* param)
{
	cout << message << endl;
}

int main(int argc, char* argv[])
{
	if (argc == 2)
	{
		SDL_Init(SDL_INIT_VIDEO);
		SDL_Window* win = SDL_CreateWindow("Raytrace", 20, 20, 100, 100, SDL_WINDOW_OPENGL);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
		if (0 == SDL_GL_CreateContext(win)) {
			cout << "Cannot create GL context\n";
			return 1;
		}

		glDebugMessageCallback(debugproc, NULL);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, true);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glEnable(GL_DEBUG_OUTPUT);

		vector<Object> objects;
		vec4 eye, coi, up;
		mat4 viewMatrix;
		vec4 lightPos;

		ifstream fp(argv[1]);
		while (true) {
			string line;
			getline(fp, line);
			if (fp.fail())
				break;
			istringstream iss(line);
			string word;
			iss >> word;
			if (word == "eye") {
				float cameraX, cameraY, cameraZ, coiX, coiY, coiZ, upX, upY, upZ;
				iss >> cameraX >> cameraY >> cameraZ;
				iss >> coiX >> coiY >> coiZ;
				iss >> upX >> upY >> upZ;
				eye = vec4(cameraX, cameraY, cameraZ, 1);
				coi = vec4(coiX, coiY, coiZ, 1);
				up = vec4(upX, upY, upZ, 0);
				vec4 W = normalize(sub(eye, coi));
				vec4 U = normalize(cross(up, W));
				vec4 V = normalize(cross(W, U));
				vec4 T = sub(vec4(0, 0, 0, 1), eye);
				viewMatrix = mat4(
					vec4(U[0], V[0], W[0], 0),
					vec4(U[1], V[1], W[1], 0),
					vec4(U[2], V[2], W[2], 0),
					vec4(dot(T, U), dot(T, V), dot(T, W), 1)
				);
			}
			else if (word == "light") {
				float lightX, lightY, lightZ;
				iss >> lightX;
				iss >> lightY;
				iss >> lightZ;
				lightPos = vec4(lightX, lightY, lightZ, 1);
			}
			else if (word == "sphere") {
				float cx, cy, cz, radius, r, g, b, refl;
				iss >> cx >> cy >> cz >> radius >> r >> g >> b >> refl;
				vec4 center = mulVM(vec4(cx, cy, cz, 1), viewMatrix);
				Object obj;
				obj.data = center;
				obj.data.w = radius;
				obj.color = vec4(r, g, b, refl);
				obj.type = vec4(0, 0, 0, 0);
				objects.push_back(obj);
			}
			else if (word == "plane") {
				float nx, ny, nz, px, py, pz, r, g, b, refl;
				iss >> nx >> ny >> nz >> px >> py >> pz >> r >> g >> b >> refl;
				vec4 normal = mulVM(vec4(nx, ny, nz, 0), viewMatrix);
				vec4 p = mulVM(vec4(px, py, pz, 1), viewMatrix);
				Object obj;
				obj.data = normal;
				obj.data.w = -dot(normal, p);
				obj.color = vec4(r, g, b, refl);
				obj.type = vec4(1, 0, 0, 0);
				objects.push_back(obj);
			}
		}

		Image img(512, 512, false);

		auto start = chrono::high_resolution_clock::now();
		//cpuTrace(lightPos, objects, img);
		gpuTrace(lightPos, objects, img, objects.size());
		auto end = chrono::high_resolution_clock::now();
		auto gpuTime = chrono::duration<double>(end - start);
		cout << gpuTime.count() << " seconds\n";

		img.write("gpu.png");
		return 0;
	}
	else
	{
		fprintf(stderr, "Wrong number of arguments given!");
		return -1;
	}
}
