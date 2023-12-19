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
#include <thread>

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

int COMPUTE_WIDTH = 6;
int COMPUTE_HEIGHT = 6;

// Order of rules must match order of coords in neighbours vector
// Everything allowed
vector <ivec2> neighbours = { ivec2(0,1), ivec2(1, 0), ivec2(0, -1), ivec2(-1,0) };
int TILE_VALUES = 4;
int allRules[4][4] =
{
   { 0b1110 , 0b1110 , 0b1110, 0b1110},
   { 0b1101 , 0b1101 , 0b1101, 0b1101},
   { 0b1011 , 0b1011 , 0b1011, 0b1011},
   { 0b0111 , 0b0111 , 0b0111, 0b0111},
};

//vector <ivec2> neighbours = { ivec2(0,1), ivec2(1, 0), ivec2(0, -1), ivec2(-1,0), ivec2(-1,1)  , ivec2(-1,-1)  ,ivec2(1,1)  ,ivec2(1,-1) };

const unsigned int MAXIMUM_NEIGHBOURS = 16;				// You also have to set it in compute shader
const unsigned int MAXIMUM_TILE_VALUES = 32;		// You also have to set it in compute shader
int NEIGHBOURS_AMOUNT = neighbours.size();

int GRID_THICKNESS = 2;
bool DIVIDE_CELLS = true;
unsigned int CELL_DIVISION = ceil(sqrt(TILE_VALUES));

int SPEED = 1000;

bool RENDER_DURING_WFC = true;
bool LOG_ELAPSED_TIMES = false;
bool IMGUI = true;

bool COLOR_FROM_TEXTURE = true;

bool SUDOKU = false;

bool vSync = false;

GLuint computeTex;
GLuint entropyTex;

GLuint inputTexture;
vector<GLuint> loadedTextures;
vector<const char*> textureLocations = {"textures/texture1.png", "textures/texture2.png"  ,"textures/texture3.png" ,"textures/texture4.png", "textures/texture5.png","textures/texture6.png","textures/texture7.png","textures/texture8.png","textures/texture9.png"};
char ruleInputTextureLocation[40] = "textures/testTexture1.png";

GLuint screenVertexShader;
GLuint screenFragmentShader;
GLuint useRulesShader;
GLuint getMinEntropyShader;
GLuint getMinEntropyCellsShader;
GLuint getTilesFromTextureShader;

GLuint screenShaderProgram;
GLuint useRulesProgram;
GLuint getMinEntropyProgram;
GLuint getMinEntropyCellsProgram;

GLuint minEntropyBuffer;
GLuint minEntropyCellsBuffer;
GLuint minEntropyCellsAmountBuffer;
GLuint minEntropyCellsAmount;
GLuint collapsedCellsBuffer;
GLuint collapsedCellsAmountBuffer;
GLuint rulesBuffer;

vector <GLuint> textureVector;
GLuint testTexture;


GLuint VAO, VBO, EBO;
GLFWwindow* window;

GLint currentIteration = 0;

vector<ivec2> collapsedCells = {};
uint collapsedCellIndex = 0;

bool stop = false;

double mousexpos, mouseypos;

void Render();
void runWFC();

void initOpenGLObjects();
void initScreen();

void getRulesFromTexture();

vec4 colorVector[MAXIMUM_TILE_VALUES] = { vec4(1, 0, 0, 1), vec4(0, 1, 0, 1), vec4(0, 0, 1, 1), vec4(1, 1, 0, 1), vec4(0, 1, 1, 1), vec4(1, 0, 1, 1), vec4(1, 0.5, 0, 1), vec4(0, 1, 0.5, 1), vec4(0.5, 0, 1, 1) };

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