//=============================================================================================
// Mintaprogram: Zold haromszog. Ervenyes 2019. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!! 
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Vali Valter
// Neptun : IK8F1D
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
#include "framework.h"

const char* const vertexSource = R"(
	#version 330
	precision highp float;

	layout(location = 0) in vec3 vp;
	layout(location = 1) in vec2 vertexUV;

	out vec2 texCoord;

	void main() {
		texCoord = vertexUV;
		gl_Position = vec4(vp.x/vp.z, vp.y/vp.z, 0, 1);
	}
)";

const char* const fragmentSource = R"(
	#version 330
	precision highp float;
			
	uniform int isProcedural;
	in vec2 texCoord;
	out vec4 outColor;

	vec4 computeColor(vec2 coord) {
		int i;
		for (i = 0; i < 50; i++) {
			if (coord.x < i + 0.1f && coord.x > i - 0.1f) {
				break; 
			}
		}
		if (coord.y < 0.1 && coord.y > -0.1) {
			return vec4(((i % 4) * 80 + 10) / 255.0f, (((i % 16) / 4) * 80 + 10) / 255.0f, ((i / 16) * 80 + 10) / 255.0f, 1);
		}
		else {
			return vec4(1 - (((i % 4) * 80 + 10) / 255.0f), 1 - ((((i % 16) / 4) * 80 + 10) / 255.0f), 1 - (((i / 16) * 80 + 10) / 255.0f), 1);
		}
	}

	void main() {
		if (isProcedural == 0) {
			outColor = vec4(0.4118, 0.7608, 0.7294, 1);
		}
		else {
			outColor = computeColor(texCoord);
		}
	}
)";

const int nVertices = 50;
const int nEdges = (nVertices * (nVertices - 1) / 2) / 20;

const int nTessellateVertices = 20;

GPUProgram gpuProgram;
unsigned int vao, vbo[2];

class Graph {
	vec3 vertices[nVertices];
	vec3 edges[2 * nEdges];
	float vertexRadius = 0.05;

	vec2 velocity[nVertices];

public:
	Graph() {

		for (int i = 0; i < nVertices; i++) {
			velocity[i] = vec2(0, 0);
		}

		for (int i = 0; i < nVertices; i++) {
			vec2 vertex = vec2(((rand() % 201) - 100) / (float)100, ((rand() % 201) - 100) / (float)100);
			while (length(vertex) >= 0.95)
				vertex = vec2(((rand() % 201) - 100) / (float)100, ((rand() % 201) - 100) / (float)100);
			vertices[i] = ndcToDescartes(vertex);
		}
		for (int i = 0; i < 2 * nEdges; i += 2) {
			int randIndex1 = rand() % nVertices;
			int randIndex2 = rand() % nVertices;
			while (randIndex1 == randIndex2 || isEdge(vertices[randIndex1], vertices[randIndex2]))
				randIndex2 = rand() % nVertices;
			
			edges[i] = vertices[randIndex1];
			edges[i + 1] = vertices[randIndex2];
		}
	}

	vec2 descartesToNdc(vec3 vertex) {
		return vec2(vertex.x / vertex.z, vertex.y / vertex.z);
	}

	vec3 ndcToDescartes(vec2 vertex) {
		float denom = sqrtf(1 - vertex.x * vertex.x - vertex.y * vertex.y);
		return vec3(vertex.x / denom, vertex.y / denom, 1 / denom);
	}

	float lorentzProduct(vec3 vertex1, vec3 vertex2) {
		return vertex1.x * vertex2.x + vertex1.y * vertex2.y - vertex1.z * vertex2.z;
	}

	bool isEdge(vec3 vertex1, vec3 vertex2) {
		for (int i = 0; i < 2 * nEdges; i += 2) {
			if ((edges[i].x == vertex1.x && edges[i].y == vertex1.y && edges[i].z == vertex1.z &&
				edges[i + 1].x == vertex2.x && edges[i + 1].y == vertex2.y && edges[i + 1].z == vertex2.z) ||
				(edges[i].x == vertex2.x && edges[i].y == vertex2.y && edges[i].z == vertex2.z &&
					edges[i + 1].x == vertex1.x && edges[i + 1].y == vertex1.y && edges[i + 1].z == vertex1.z)) {
				return true;
			}
		}
		return false;
	}

	void heuristic() {
		vec3 newVertices[nVertices];
		vec3 newEdges[2 * nEdges];
		for (int i = 0; i < nVertices; i++) {
			float x = 0, y = 0;
			int denom = 0;
			for (int j = 0; j < nVertices; j++) {
				if (i != j) {
					if (isEdge(vertices[i], vertices[j])) {
						x += descartesToNdc(vertices[j]).x;
						y += descartesToNdc(vertices[j]).y;
						denom++;
					}
					else {
						x -= 0.01 * descartesToNdc(vertices[j]).x;
						y -= 0.01 * descartesToNdc(vertices[j]).y;
					}
				}
			}
			
			if (denom == 0)
				newVertices[i] = vertices[i];
			else {
				x = x / denom;
				y = y / denom;
				newVertices[i] = ndcToDescartes(vec2(x, y));
			}
			
			for (int j = 0; j < 2 * nEdges; j++) {
				if (edges[j].x == vertices[i].x && edges[j].y == vertices[i].y && edges[j].z == vertices[i].z) {
					newEdges[j] = newVertices[i];
				}
			}
		}
		for (int i = 0; i < nVertices; i++) {
			vertices[i] = newVertices[i];
		}
		for (int i = 0; i < 2 * nEdges; i++) {
			edges[i] = newEdges[i];
		}
	}

	float vertexForce(float d, vec3 vertex1, vec3 vertex2) {
		if (isEdge(vertex1, vertex2)) {
			if (d > 0.2) {
				return (d - 0.2) * (d - 0.2);
			}
			else {
				return -40.0 * (d - 0.2) * (d - 0.2);
			}
		}
		else {
			return 0.3 * log10f(d + 0.01) - 0.02;
		}
	}

	void simulation() {
		for (int t = 0; t < 10; t += 1) {
			for (int i = 0; i < nVertices; i++) {
				vec2 force(0, 0);
				for (int j = 0; j < nVertices; j++) {
					if (j != i) {
						vec2 ndcVertex = descartesToNdc(vertices[i]);
						vec2 ndcVertex2 = descartesToNdc(vertices[j]);
						float d = length(ndcVertex2 - ndcVertex);
						force = force + vertexForce(d, vertices[i], vertices[j]) * normalize(ndcVertex2 - ndcVertex) - 10 * velocity[i];
					}
				}
				force = 0.003 * force - 0.003 * descartesToNdc(vertices[i]);
				velocity[i] = velocity[i] + force;
				vec2 newPosition = vec2(descartesToNdc(vertices[i]).x + velocity[i].x, descartesToNdc(vertices[i]).y + velocity[i].y);
				if (length(newPosition) >= 0.9) {
					newPosition = 0.8999 * normalize(newPosition);
				}
				for (int j = 0; j < 2 * nEdges; j++) {
					if (edges[j].x == vertices[i].x && edges[j].y == vertices[i].y && edges[j].z == vertices[i].z)
						edges[j] = ndcToDescartes(newPosition);
				}
				vertices[i] = ndcToDescartes(newPosition);
			}
		}
	}

	void translate(vec2 prevP, vec2 newP) {
		vec3 reflectionPoint1 = ndcToDescartes(0.05 * 0.25 * (newP - prevP));
		vec3 reflectionPoint2 = ndcToDescartes(0.05 * 0.75 * (newP - prevP));

		for (int i = 0; i < nVertices; i++) {
			float d = acoshf(-1 * lorentzProduct(vertices[i], reflectionPoint1));
			vec3 v = (reflectionPoint1 - vertices[i] * coshf(d)) / sinhf(d);
			vec3 tempPosition = vertices[i] * coshf(2 * d) + v * sinhf(2 * d);

			d = acoshf(-1 * lorentzProduct(tempPosition, reflectionPoint2));
			v = (reflectionPoint2 - tempPosition * coshf(d)) / sinhf(d);
			vec3 newPosition = tempPosition * coshf(2 * d) + v * sinhf(2 * d);

			for (int j = 0; j < 2 * nEdges; j++) {
				if (edges[j].x == vertices[i].x && edges[j].y == vertices[i].y && edges[j].z == vertices[i].z)
					edges[j] = newPosition;
			}
			vertices[i] = newPosition;
		}
	}

	void draw() {
		drawEdges();
		drawVertices();
	}

	void drawEdges() {

		gpuProgram.setUniform((int)false, "isProcedural");

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, 2 * nEdges * sizeof(vec3), &edges[0], GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		glDrawArrays(GL_LINES, 0, 2 * nEdges);
	}

	void drawVertices() {

		for (int i = 0; i < nVertices; i++) {
			vec2 vertex = descartesToNdc(vertices[i]);
			vec3 vertexPoints[nTessellateVertices];
			float phi = 0;
			for (int j = 0; j < nTessellateVertices / 2; j++) {
				phi = j / (float)nTessellateVertices * 2 * M_PI;
				vec2 vertexp1 = vertex + 0.000001 * vec2(cos(phi), sin(phi));
				vec2 vertexp2 = vertex - 0.000001 * vec2(cos(phi), sin(phi));
				vec3 wVertexp1 = ndcToDescartes(vertexp1);
				vec3 wVertexp2 = ndcToDescartes(vertexp2);
				vec3 radius1 = vertexRadius * normalize(wVertexp1 - wVertexp2);
				vec3 radius2 = -1 * radius1;
				vertexPoints[j] = vertices[i] + radius1;
				vertexPoints[j + nTessellateVertices / 2] = vertices[i] + radius2;
			}
			gpuProgram.setUniform((int)true, "isProcedural");

			glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
			glBufferData(GL_ARRAY_BUFFER, nTessellateVertices * sizeof(vec3), &vertexPoints[0], GL_DYNAMIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

			vec2 numbers[nTessellateVertices];
			for (int j = 0; j < nTessellateVertices; j++) {
				numbers[j] = vec2(i, j / (nTessellateVertices / 2));
			}

			glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
			glBufferData(GL_ARRAY_BUFFER, nTessellateVertices * sizeof(vec2), &numbers[0], GL_DYNAMIC_DRAW);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

			glDrawArrays(GL_TRIANGLE_FAN, 0, nTessellateVertices);
		}
	}
};

Graph* graph;

void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);
	glLineWidth(2.0f);

	graph = new Graph();

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(2, vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);

	gpuProgram.create(vertexSource, fragmentSource, "outColor");
}

void onDisplay() {
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindVertexArray(vao);
	graph->draw();
	glutSwapBuffers();
}

bool simulate = false;
long simulationStartTime;

void onKeyboard(unsigned char key, int pX, int pY) {
	if (key == ' ') {
		graph->heuristic();
		simulate = true;
		simulationStartTime = glutGet(GLUT_ELAPSED_TIME);
		glutPostRedisplay();
	}
}

void onKeyboardUp(unsigned char key, int pX, int pY) {
}

bool mousePressed = false;
vec2 prevPoint;

void onMouse(int button, int state, int pX, int pY) {
	switch (state) {
	case GLUT_DOWN: mousePressed = true;	break;
	case GLUT_UP:   mousePressed = false;	break;
	}
	float cX = 2.0f * pX / windowWidth - 1;
	float cY = 1.0f - 2.0f * pY / windowHeight;
	prevPoint = vec2(cX, cY);
}

void onMouseMotion(int pX, int pY) {
	float cX = 2.0f * pX / windowWidth - 1;
	float cY = 1.0f - 2.0f * pY / windowHeight;

	if (mousePressed) {
		graph->translate(prevPoint, vec2(cX, cY));
	}
	glutPostRedisplay();
}

void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME);
	if (simulate && time - simulationStartTime < 4000) {
		graph->simulation();
		glutPostRedisplay();
	}
}