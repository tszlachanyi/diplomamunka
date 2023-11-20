// TODO :

#include <iostream>
#include <chrono>
#include <algorithm>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <bitset>		
#include <array>
#include <cmath>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_glfw.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

using namespace std;
using namespace std::chrono;
using namespace glm;

const unsigned short OPENGL_MAJOR_VERSION = 4;
const unsigned short OPENGL_MINOR_VERSION = 6;

const unsigned int SCREEN_WIDTH = 1024;
const unsigned int SCREEN_HEIGHT = 1024;

int COMPUTE_WIDTH = 9;
int COMPUTE_HEIGHT = 9;

const unsigned int MAXIMUM_RULES = 100;
int TILE_VALUES = 9;

int GRID_THICKNESS = 2;

bool DIVIDE_CELLS = true;
unsigned int CELL_DIVISION = ceil(sqrt(TILE_VALUES));

bool RENDER_DURING_WFC = true;
bool LOG_ELAPSED_TIMES = false;
bool IMGUI = true;

bool COLOR_FROM_TEXTURE = true;

bool SUDOKU = true;

bool vSync = false;

GLuint computeTex2;
GLuint computeTex1;
GLuint entropyTex;

GLuint* activeTexture = &computeTex1;
GLuint* inactiveTexture = &computeTex2;

vector<GLuint> loadedTextures;
vector<const char*> textureLocations = {"textures/texture1.png", "textures/texture2.png"  ,"textures/texture3.png" ,"textures/texture4.png", "textures/texture5.png","textures/texture6.png","textures/texture7.png","textures/texture8.png","textures/texture9.png"};

GLuint screenVertexShader;
GLuint screenFragmentShader;
GLuint computeShader;
GLuint computeEntropyShader;
GLuint chooseTileValueShader;

GLuint minEntropyBuffer;
GLuint minEntropyCellsBuffer;
GLuint minEntropyCellsAmountBuffer;
GLuint minEntropyCellsAmount;
GLuint collapsedCellsBuffer;
GLuint collapsedCellsAmountBuffer;

vector <GLuint> textureVector;

GLuint screenShaderProgram;
GLuint computeProgram;
GLuint computeEntropyProgram;
GLuint chooseTileValueProgram;
GLuint VAO, VBO, EBO;
GLFWwindow* window;

GLint currentIteration = 0;

double mousexpos, mouseypos;

void runOneIteration();
void runWFC();

void initOpenGL();
void initScreen();

// example for assymetric rules
//vector<vector<vector<GLint>>> rules =
//{
//   {{0,1,0b0001}},
//   {{1,0,0b0010}},
//   {{0,-1,0b0100}},
//   {{-1,0,0b1000}},
//};

//array<array<array<GLint, 100>, MAXIMUM_RULES>, TILE_VALUES> rules =
//{{
//   {{{0,1,0b1110} , {1,0,0b1110} , {0,-1,0b1110}, {-1,0,0b1110}}},
//   {{{0,1,0b1101} , {1,0,0b1101} , {0,-1,0b1101}, {-1,0,0b1101}}},
//   {{{0,1,0b1011} , {1,0,0b1011} , {0,-1,0b1011}, {-1,0,0b1011}}},
//   {{{0,1,0b0111} , {1,0,0b0111} , {0,-1,0b0111}, {-1,0,0b0111}}},
//}};



// rules array explanation :
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

		std::cerr << "[glCompileShader] Shader compilation error in " << _fileName << ":\n" << &VertexShaderErrorMessage[0] << "\n";
	}

	return loadedShader;
}


//entropy kiszámolás - tile választás - érték választás - szabályok alkalmazása szomszédokra
//	gpu						cpu				 cpu						gpu