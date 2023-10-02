
#include<iostream>
#include <chrono>

#include"glad/glad.h"
#include"GLFW/glfw3.h"
#include"glm/glm.hpp"


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

GLfloat textureVectors[MAXIMUM_LENGTH][4 * COMPUTE_WIDTH * COMPUTE_HEIGHT];
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

	int x = std::floor(coords.x / xRatio);
	int y = (COMPUTE_HEIGHT - 1) - std::floor(coords.y / yRatio);

	return vec2(x, y);
}

// Converts from matrix coordinate to texture coordinate of a pixel and its rgba value 
int matrixToVecCoords(vec2 coords, int rgba)
{
	return (COMPUTE_WIDTH * coords.y * 4) + (coords.x * 4) + rgba;
}

// Changes the color of a coordinate in a texture vector 
void colorTexture(GLfloat* textureVector, vec2 coords, vec4 rgba)
{
	for (int i = 0; i <= 3; i++)
	{
		textureVector[matrixToVecCoords(coords, i)] = rgba[i];
	}

}

// Assign a texture vector to an element of the textureVectors array
void assignToTextureVectorsArray(GLuint i, GLfloat vector[4 * COMPUTE_WIDTH * COMPUTE_HEIGHT])
{
	for (int j = 0; j <= 4 * COMPUTE_WIDTH * COMPUTE_HEIGHT; j++)
	{
		textureVectors[i][j] = vector[j];
	}
}

// Get vector format of a RGBA texture
GLfloat* getTextureVector(GLuint texture)
{
	GLfloat* textureVector = new GLfloat[4 * COMPUTE_WIDTH * COMPUTE_HEIGHT];

	glBindTexture(GL_TEXTURE_2D, screenTex);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, textureVector);
	glBindTexture(GL_TEXTURE_2D, 0);

	return textureVector;
}

// Get current epoch time in milliseconds
uint64_t getEpochTime()
{
	milliseconds ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
	uint64_t time = ms.count();
	return time;
}

// Use compute shader to create screenTex from current loadTex
void computeNext()
{
	//std::cout << " (" << coords.x << " , " << coords.y << ")" << std::endl;



	if (computedIterations > currentIteration)
	{


		currentIteration++;
		glTextureSubImage2D(screenTex, 0, 0, 0, COMPUTE_WIDTH, COMPUTE_HEIGHT, GL_RGBA, GL_FLOAT, textureVectors[currentIteration]);

		std::cout << "currentIteration : " << currentIteration << " computedIterations : " << computedIterations << " loaded" << std::endl;
	}
	else
	{
		glTextureSubImage2D(loadTex, 0, 0, 0, COMPUTE_WIDTH, COMPUTE_HEIGHT, GL_RGBA, GL_FLOAT, textureVectors[currentIteration]);

		glUseProgram(computeProgram);
		glDispatchCompute(COMPUTE_WIDTH, COMPUTE_HEIGHT, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		currentIteration++;
		computedIterations = currentIteration;
		GLfloat* textureVector = getTextureVector(screenTex);
		assignToTextureVectorsArray(currentIteration, textureVector);

		std::cout << "currentIteration : " << currentIteration << " computedIterations : " << computedIterations << " computed" << std::endl;
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
		std::cout << "Right\n";

		clickingAllowed = false;

		uint64_t startTime = getEpochTime();

		computeNext();

		uint64_t endTime = getEpochTime();
		std::cout << "elapsed time : " << endTime - startTime << " ms" << std::endl;

	}

	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
	{
		std::cout << "Left\n";

		if (currentIteration >= 1)
		{
			currentIteration = currentIteration - 2;
			computeNext();
		}
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if ((button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT) && action == GLFW_PRESS)
	{
		vec2 coords = screenToTextureCoords(vec2(mousexpos, mouseypos));
		std::cout << " (" << coords.x << " , " << coords.y << ")" << std::endl;

		if (clickingAllowed)
		{
			GLfloat* pixelData = textureVectors[currentIteration];
			colorTexture(pixelData, coords, vec4(1, 1, 1, 1));
			assignToTextureVectorsArray(currentIteration, pixelData);
			glTextureSubImage2D(screenTex, 0, 0, 0, COMPUTE_WIDTH, COMPUTE_HEIGHT, GL_RGBA, GL_FLOAT, textureVectors[currentIteration]);
		}
	}
}