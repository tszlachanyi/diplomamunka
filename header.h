// TODO :
// MAKE RANDOM WORK PROPERLY
// RENDER AFTER EACH ITERATION
// CHANGE TO MANUAL COLLAPSE CHOICE
// MULTIPLE ITERATIONS IN COMPUTE NEXT

#include <iostream>
#include <chrono>
#include <algorithm>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <bitset>		 

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

using namespace std;
using namespace std::chrono;
using namespace glm;

const unsigned int SCREEN_WIDTH = 1024;
const unsigned int SCREEN_HEIGHT = 1024;

const unsigned int COMPUTE_WIDTH = 16;
const unsigned int COMPUTE_HEIGHT = 16;

const unsigned short OPENGL_MAJOR_VERSION = 4;
const unsigned short OPENGL_MINOR_VERSION = 6;

const unsigned int MAXIMUM_ITERATIONS = 2000;
const unsigned int MAXIMUM_RULES = 100;
const unsigned int TILE_VALUES = 4;

bool vSync = true;

GLuint screenTex;
GLuint computedTex;
GLuint loadTex;

GLuint textureVectors[MAXIMUM_ITERATIONS][COMPUTE_WIDTH * COMPUTE_HEIGHT];
vector<vec2> uncollapsed;

GLuint computeProgram;
GLint uLocationRules;
GLint uLocationRulesAmount;
GLint uLocationChosenValue;
GLint uLocationCoordinates;


GLint currentIteration = 0;
GLint computedIterations = 0;



double mousexpos, mouseypos;
bool clickingAllowed = true;

vector<vector<vector<GLint>>> rules =
	 {
		{{0,1,0b1110} , {1,0,0b1110} , {0,-1,0b1110}, {-1,0,0b1110}},
	 	{{0,1,0b1101} , {1,0,0b1101} , {0,-1,0b1101}, {-1,0,0b1101}},
	 	{{0,1,0b1011} , {1,0,0b1011} , {0,-1,0b1011}, {-1,0,0b1011}},
	 	{{0,1,0b0111} , {1,0,0b0111} , {0,-1,0b0111}, {-1,0,0b0111}},
	 };

// rules vector explanation :
// in rules[a][b][c] gives one rule
//		a : The value of the current tile we are using the role on
//		b : Number of the rule
//		c : c can be any of these 3 values : 
//				[0] - x offset of the rule	
//				[1] - y offset of the rule		
//				[2] - list of allowed and disallowed tiles as a 2 bit integer, on the position defined by the offsets
//			Example : {1,0,0b0000010 } rule means that the cell on the right of the current cell can only be tile 2


GLfloat vertices[] =
{
	-1.0f, -1.0f , 0.0f, 0.0f, 0.0f,
	-1.0f,  1.0f , 0.0f, 0.0f, 1.0f,
	 1.0f,  1.0f , 0.0f, 1.0f, 1.0f,
	 1.0f, -1.0f , 0.0f, 1.0f, 0.0f,
};

GLuint indices[] =
{
	0, 2, 1,
	0, 3, 2
};

GLuint loadShader(GLenum _shaderType, const char* _fileName)
{
	// shader azonosito letrehozasa
	GLuint loadedShader = glCreateShader(_shaderType);

	// ha nem sikerult hibauzenet es -1 visszaadasa
	if (loadedShader == 0)
	{
		std::cerr << "[glCreateShader] Error during the initialization of shader: " << _fileName << "!\n";
		return 0;
	}

	// shaderkod betoltese _fileName fajlbol
	std::string shaderCode = "";

	// _fileName megnyitasa
	std::ifstream shaderStream(_fileName);

	if (!shaderStream.is_open())
	{
		std::cerr << "[std::ifstream] Error during the reading of " << _fileName << " shaderfile's source!\n";
		return 0;
	}

	// file tartalmanak betoltese a shaderCode string-be
	std::string line = "";
	while (std::getline(shaderStream, line))
	{
		shaderCode += line + "\n";
	}

	shaderStream.close();

	// fajlbol betoltott kod hozzarendelese a shader-hez
	const char* sourcePointer = shaderCode.c_str();
	glShaderSource(loadedShader, 1, &sourcePointer, nullptr);

	// shader leforditasa
	glCompileShader(loadedShader);

	// ellenorizzuk, h minden rendben van-e
	GLint result = GL_FALSE;
	int infoLogLength;

	// forditas statuszanak lekerdezese
	glGetShaderiv(loadedShader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(loadedShader, GL_INFO_LOG_LENGTH, &infoLogLength);

	if (GL_FALSE == result)
	{
		// hibauzenet elkerese es kiirasa
		std::vector<char> VertexShaderErrorMessage(infoLogLength);
		glGetShaderInfoLog(loadedShader, infoLogLength, nullptr, &VertexShaderErrorMessage[0]);

		std::cerr << "[glCompileShader] Shader compilation error in " << _fileName << ":\n" << &VertexShaderErrorMessage[0] << std::endl;
	}

	return loadedShader;
}