#version 430

struct Object { vec4 data, color, type; };

layout(local_size_x=1,local_size_y=1,local_size_z=1) in;

layout(location=0) uniform samplerBuffer bufferIn;

layout(location=1) uniform int objectsSize;

layout(location=2) uniform vec4 lightPos;

layout(location=3) uniform int imgSize;

layout(std430,binding=0,row_major) buffer ObjectBuffer{
    Object objs[];
};

layout(std430,binding=1,row_major) buffer ImageBuffer{
    uint img[];
};

float closestT;
int closestO;

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
			sum += v[j] * M[j][i];
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

void getRayIntersection(vec4 rayStart, vec4 rayDir) {
	closestT = -1;
	closestO = -1;
	for (int i = 0; i < objectsSize; ++i) {
		float t;
		if (objs[i].type.r == 0)
			t = raySphereIntersect(rayStart, rayDir, objs[i].data);
		else
			t = rayPlaneIntersect(rayStart, rayDir, objs[i].data);
		if (t != -1.0 && t > 0.001 && (closestO == -1 || t < closestT)) {
			closestT = t;
			closestO = i;
		}
	}
}

vec4 getColorForRay(vec4 lightPos, vec4 rayStart, vec4 rayDir) {
	getRayIntersection(rayStart, rayDir);
	if (closestO != -1) {
		vec4 closestIp = add(rayStart, mulSV(closestT, rayDir));
		vec4 N;
		if (objs[closestO].type.r == 0) {
			//sphere
			N = sub(closestIp, objs[closestO].data);
			N.w = 0;
			N = normalize(N);
		}
		else {
			//plane
			N = objs[closestO].data;
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
		vec4 col1 = add(mulSV(dp, objs[closestO].color), mulSV(sp, vec4(1, 1, 1, 1)));
		return col1;
	}
	else {
		return vec4(0, 0, 0, 1);
	}
}

void main(){
	//Set the objects array
	for (int i = 0; i < objectsSize; i++) {
        objs[i].data	= vec4(texelFetch(bufferIn, (i*3)).rgba);
        objs[i].color	= vec4(texelFetch(bufferIn, (i*3)+1).rgba);
        objs[i].type	= vec4(texelFetch(bufferIn, (i*3)+2).rgba);
    }

    //Do the trace
    float fov = 45;
	float d = 1.0 / tan(fov / 180.0*3.14159265358979323);
	int w = imgSize;
	int h = imgSize;
	for (int ypix = 0; ypix<h; ++ypix) {
		float y = 1 + -2 * ypix / (h - 1.0);
		for (int xpix = 0; xpix < w; ++xpix) {
			float x = -1 + 2 * xpix / (w - 1.0);
			vec4 rayDir = vec4(x, y, -d, 0);
			rayDir = normalize(rayDir);
			vec4 col = getColorForRay(lightPos, vec4(0, 0, 0, 1), rayDir);
			int r = int(col[0] * 255);
			int g = int(col[1] * 255);
			int b = int(col[2] * 255);
			if (r > 255) r = 255;
			if (g > 255) g = 255;
			if (b > 255) b = 255;
			int idx = (ypix * imgSize) + xpix;
			img[idx] = (0xFF << 24) | (b << 16) | (g << 8) | r;
		}
	}
}