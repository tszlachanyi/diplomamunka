
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

const unsigned int COMPUTE_WIDTH = 32;
const unsigned int COMPUTE_HEIGHT = 32;

const unsigned short OPENGL_MAJOR_VERSION = 4;
const unsigned short OPENGL_MINOR_VERSION = 6;

const unsigned int MAXIMUM_ITERATIONS = 1000;
const unsigned int MAXIMUM_RULES = 100;
const unsigned int TILE_VALUES = 8;

bool vSync = true;

GLuint screenTex;
GLuint computedTex;
GLuint loadTex;

GLuint textureVectors[MAXIMUM_ITERATIONS][COMPUTE_WIDTH * COMPUTE_HEIGHT];

GLuint computeProgram;
GLint uLocationRules;
GLint uLocationRulesAmount;
GLint uLocationCoordinates;


GLint currentIteration = 0;
GLint computedIterations = 0;


double mousexpos, mouseypos;
bool clickingAllowed = true;

struct rule {
	GLuint x; // x offset
	GLuint y; // y offset
	GLuint allowList;// List of allowed and disallowed tiles as a 2 bit integer, on the position defined by the offsets

	// Constructor
	rule(GLuint X, GLuint Y, GLuint ALLOWLIST) : x(X), y(Y), allowList(ALLOWLIST) {}
};

vector<vector<vector<GLint>>> rules =
	 {
		{{1,0,0b0000111}, {1,1,0b0000110}, {-1,-1,0b0000011} , {-2,0,0b0000100}},
	 	{},
	 	{},
	 	{},
	 	{},
	 	{},
	 	{},
	 	{}
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


const vec4 tileColors[TILE_VALUES] =
{
	{vec4(1,0,0,1)},
	{vec4(0,1,0,1)},
	{vec4(0,0,1,1)},
	{},
	{},
	{},
	{},
	{}
};

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
	