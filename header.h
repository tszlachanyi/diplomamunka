
#include <iostream>
#include <chrono>
#include <algorithm>
		 
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"


#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>



using namespace std::chrono;
using namespace glm;

const unsigned int SCREEN_WIDTH = 1024;
const unsigned int SCREEN_HEIGHT = 1024;

const unsigned int COMPUTE_WIDTH = 32;
const unsigned int COMPUTE_HEIGHT = 32;

const unsigned short OPENGL_MAJOR_VERSION = 4;
const unsigned short OPENGL_MINOR_VERSION = 6;

const unsigned int MAXIMUM_LENGTH = 100;

bool vSync = true;

GLuint screenTex;
GLuint computedTex;
GLuint loadTex;

GLuint textureVectors[MAXIMUM_LENGTH][COMPUTE_WIDTH * COMPUTE_HEIGHT];
GLuint white = 1;
GLuint black = 0;

GLint currentIteration = 0;
GLint computedIterations = 0;

GLuint computeProgram;
double mousexpos, mouseypos;
bool clickingAllowed = true;


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

// Converts from screen coordinates to coordinates of the textures
vec2 screenToTextureCoords(vec2 coords)
{
	float xRatio = SCREEN_WIDTH / COMPUTE_WIDTH;
	float yRatio = SCREEN_HEIGHT / COMPUTE_HEIGHT;

	int x = int(std::floor(coords.x / xRatio));
	int y = (COMPUTE_HEIGHT - 1) - int(std::floor(coords.y / yRatio));

	return vec2(x, y);
}

// Converts from matrix coordinate to texture coordinate of a pixel
int matrixToVecCoords(vec2 coords)
{
	return int((COMPUTE_WIDTH * coords.y) + (coords.x));
}

// Get current epoch time in milliseconds
uint64_t getEpochTime()
{
	milliseconds ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
	uint64_t time = ms.count();
	return time;
}



template <typename T, size_t N>
void printArray(T(&array)[N])
{
	std::cout << "[";

	for (size_t i = 0; i < N; i++)
	{
		std::cout << array[i];
		if (i != N - 1)
		{
			std::cout << ", ";
		}
	}
	
	std::cout << "]" << std::endl;
}

void assignToTextureVectorsArray(GLuint i, GLuint vector[COMPUTE_WIDTH * COMPUTE_HEIGHT])
{
	for (int j = 0; j <= COMPUTE_WIDTH * COMPUTE_HEIGHT; j++)
	{
		textureVectors[i][j] = vector[j];
	}
}

// Get vector format of a GL_R32UI texture
GLuint* getTextureVector(GLuint texture)
{
	GLuint* textureVector = new GLuint[COMPUTE_WIDTH * COMPUTE_HEIGHT];
	//GLuint textureVector[COMPUTE_WIDTH * COMPUTE_HEIGHT];

	glBindTexture(GL_TEXTURE_2D, texture);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, textureVector);
	glBindTexture(GL_TEXTURE_2D, 0);
	return textureVector;
}

void updateScreenTex()
{
	GLfloat rgbaVector[4 * COMPUTE_WIDTH * COMPUTE_HEIGHT];

	for (int j = 0; j < COMPUTE_WIDTH * COMPUTE_HEIGHT; j++)
	{
		for (int i = 0; i < 4; i++)
		{
			if (textureVectors[currentIteration][j] == white)
			{
				rgbaVector[4 * j + i] = 1;
			}
			else
			{
				rgbaVector[4 * j + i] = 0;
			}
		}
	}

	//printArray(textureVectors[currentIteration]);
	glTextureSubImage2D(screenTex, 0, 0, 0, COMPUTE_WIDTH, COMPUTE_HEIGHT, GL_RGBA, GL_FLOAT, rgbaVector);
}


// Use compute shader to create screenTex from current loadTex
void computeNext()
{

	if (computedIterations > currentIteration)
	{
		currentIteration++;
		updateScreenTex();

		std::cout << "currentIteration : " << currentIteration << " computedIterations : " << computedIterations << std::endl;
	}
	else
	{
		glTextureSubImage2D(loadTex, 0, 0, 0, COMPUTE_WIDTH, COMPUTE_HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_INT, textureVectors[currentIteration]);

		glUseProgram(computeProgram);
		glDispatchCompute(COMPUTE_WIDTH, COMPUTE_HEIGHT, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		currentIteration++;
		computedIterations = currentIteration;
		GLuint* textureVector = getTextureVector(computedTex);

		assignToTextureVectorsArray(currentIteration, textureVector);
		

		updateScreenTex();

		std::cout << "currentIteration : " << currentIteration << " computedIterations : " << computedIterations << std::endl;
	}


}

// Input
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}

	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
	{
		clickingAllowed = false;

		uint64_t startTime = getEpochTime();

		computeNext();

		uint64_t endTime = getEpochTime();
		std::cout << "elapsed time : " << endTime - startTime << " ms" << std::endl;

	}

	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
	{

		if (currentIteration >= 1)
		{
			currentIteration = currentIteration - 2;
			computeNext();
		}
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		vec2 coords = screenToTextureCoords(vec2(mousexpos, mouseypos));
		std::cout << " (" << coords.x << " , " << coords.y << ")" << std::endl;

		if (clickingAllowed)
		{
			
			if (button == GLFW_MOUSE_BUTTON_LEFT)
			{
				textureVectors[currentIteration][matrixToVecCoords(coords)] = white;
			}

			if (button == GLFW_MOUSE_BUTTON_RIGHT)
			{
				textureVectors[currentIteration][matrixToVecCoords(coords)] = black;
			}
		
			updateScreenTex();
		}
	}
}


	