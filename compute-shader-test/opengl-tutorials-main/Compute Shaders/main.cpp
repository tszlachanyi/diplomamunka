#include<iostream>
#include <chrono>

#include"glad/glad.h"
#include"GLFW/glfw3.h"

using namespace std::chrono;


const unsigned int SCREEN_WIDTH = 1024;
const unsigned int SCREEN_HEIGHT = 1024;

const unsigned int COMPUTE_WIDTH = 64;
const unsigned int COMPUTE_HEIGHT = 64;

const unsigned short OPENGL_MAJOR_VERSION = 4;
const unsigned short OPENGL_MINOR_VERSION = 6;

const unsigned int MAXIMUM_LENGTH = 1000;

bool vSync = true;

GLuint screenTex;
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


const char* screenVertexShaderSource = R"(#version 460 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uvs;
out vec2 UVs;
void main()
{
	gl_Position = vec4(pos.x, pos.y, pos.z, 1.0);
	UVs = uvs;
})";

const char* screenFragmentShaderSource = R"(#version 460 core
out vec4 FragColor;
uniform sampler2D screen;
in vec2 UVs;
void main()
{
	FragColor = texture(screen, UVs);
})";

const char* screenComputeShaderSource = R"(#version 460 core
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout (binding = 1, rgba32f) readonly uniform image2D inputImage;
layout (binding = 0, rgba32f) writeonly uniform image2D outputImage;

void main()
{
	vec4 pixel = vec4(0.075, 0.133, 0.173, 1.0);
	ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
	
	vec4 black = vec4(0, 0, 0, 1.0);
	vec4 white = vec4(1, 1, 1, 1.0);

	ivec2 neighbourCoords[8] = {ivec2(-1,-1), ivec2(-1,0), ivec2(-1,1), ivec2(0,-1), ivec2(0,1), ivec2(1,-1), ivec2(1,0), ivec2(1,1)};
	vec4 loadedPixel;
	int neighboursSum = 0;

	for (int i = 0; i <= 7; i++)
	{
		loadedPixel = imageLoad(inputImage, pixelCoords + neighbourCoords[i]);
		if (loadedPixel == white)
		{
			neighboursSum++;
		}
	}	

	loadedPixel = imageLoad(inputImage, pixelCoords);
	
	if (loadedPixel == white)
	{
		if (neighboursSum == 2 || neighboursSum == 3)
		{
			pixel = white;
		}
		else
		{
			pixel = black;
		}
		
	}
	else
	{
		if (neighboursSum == 3)
		{
			pixel = white;
		}
		else
		{
			pixel = black;
		}
	}
	

	
	

	imageStore(outputImage, pixelCoords, pixel);
})";

struct vec2 {
	float x, y;

	vec2(float x_val, float y_val)
		: x(x_val), y(y_val) {}

	float& operator[](int index) {
		if (index == 0) return x;
		if (index == 1) return y;
	}
};

struct vec3 {
	float x, y, z;

	vec3(float x_val, float y_val, float z_val)
		: x(x_val), y(y_val), z(z_val) {}

	float& operator[](int index) {
		if (index == 0) return x;
		if (index == 1) return y;
		if (index == 2) return z;
	}
};

struct vec4 {
	float x, y, z, w;

	vec4(float x_val, float y_val, float z_val, float w_val)
		: x(x_val), y(y_val), z(z_val), w(w_val) {}

	float& operator[](int index) {
		if (index == 0) return x;
		if (index == 1) return y;
		if (index == 2) return z;
		if (index == 3) return w;
	}
};

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

// Get vector format of a texture
GLfloat* getTextureVector(GLuint texture)
{
	GLfloat*  textureVector = new GLfloat[4 * COMPUTE_WIDTH * COMPUTE_HEIGHT];

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

	uint64_t startTime = getEpochTime();

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

	uint64_t endTime = getEpochTime();
	std::cout << "elapsed time : " << endTime - startTime << " ms" << std::endl ;
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
		computeNext();
		
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

int main()
{

	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OPENGL_MAJOR_VERSION);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OPENGL_MINOR_VERSION);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "OpenGL Compute Shaders", NULL, NULL);
	if (!window)
	{
		std::cout << "Failed to create the GLFW window\n";
		glfwTerminate();
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(vSync);
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize OpenGL context" << std::endl;
	}
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);


	GLuint VAO, VBO, EBO;
	glCreateVertexArrays(1, &VAO);
	glCreateBuffers(1, &VBO);
	glCreateBuffers(1, &EBO);

	glNamedBufferData(VBO, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glNamedBufferData(EBO, sizeof(indices), indices, GL_STATIC_DRAW);

	glEnableVertexArrayAttrib(VAO, 0);
	glVertexArrayAttribBinding(VAO, 0, 0);
	glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);

	glEnableVertexArrayAttrib(VAO, 1);
	glVertexArrayAttribBinding(VAO, 1, 0);
	glVertexArrayAttribFormat(VAO, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat));

	glVertexArrayVertexBuffer(VAO, 0, VBO, 0, 5 * sizeof(GLfloat));
	glVertexArrayElementBuffer(VAO, EBO);

	// Output Texture
	
	glCreateTextures(GL_TEXTURE_2D, 1, &screenTex);
	glTextureParameteri(screenTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(screenTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(screenTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(screenTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(screenTex, 1, GL_RGBA32F, COMPUTE_WIDTH, COMPUTE_HEIGHT);
	glBindImageTexture(0, screenTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);


	// Initial Texture
	
	glCreateTextures(GL_TEXTURE_2D, 1, &loadTex);
	glTextureParameteri(loadTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(loadTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(loadTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(loadTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(loadTex, 1, GL_RGBA32F, COMPUTE_WIDTH, COMPUTE_HEIGHT);
	glBindImageTexture(1, loadTex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	GLfloat* pixelData = new GLfloat[4 * COMPUTE_WIDTH * COMPUTE_HEIGHT];
	
		// // Still
		// colorTexture(pixelData, vec2(8, 4),  vec4(1, 1, 1, 1));
		// colorTexture(pixelData, vec2(9, 3),  vec4(1, 1, 1, 1));
		// colorTexture(pixelData, vec2(9, 5),  vec4(1, 1, 1, 1));
		// colorTexture(pixelData, vec2(10, 4), vec4(1, 1, 1, 1));
		// 
		// // Oscillator
		// colorTexture(pixelData, vec2(4, 4), vec4(1, 1, 1, 1));
		// colorTexture(pixelData, vec2(4, 5), vec4(1, 1, 1, 1));
		// colorTexture(pixelData, vec2(4, 6), vec4(1, 1, 1, 1));
		// 
		// // Spaceship
		// colorTexture(pixelData, vec2(4, 20), vec4(1, 1, 1, 1));
		// colorTexture(pixelData, vec2(5, 20), vec4(1, 1, 1, 1));
		// colorTexture(pixelData, vec2(6, 20), vec4(1, 1, 1, 1));
		// colorTexture(pixelData, vec2(6, 21), vec4(1, 1, 1, 1));
		// colorTexture(pixelData, vec2(5, 22), vec4(1, 1, 1, 1));

	assignToTextureVectorsArray(0, pixelData);
	

	GLuint screenVertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(screenVertexShader, 1, &screenVertexShaderSource, NULL);
	glCompileShader(screenVertexShader);
	GLuint screenFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(screenFragmentShader, 1, &screenFragmentShaderSource, NULL);
	glCompileShader(screenFragmentShader);

	GLuint screenShaderProgram = glCreateProgram();
	glAttachShader(screenShaderProgram, screenVertexShader);
	glAttachShader(screenShaderProgram, screenFragmentShader);
	glLinkProgram(screenShaderProgram);

	glDeleteShader(screenVertexShader);
	glDeleteShader(screenFragmentShader);


	GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(computeShader, 1, &screenComputeShaderSource, NULL);
	glCompileShader(computeShader);

	computeProgram = glCreateProgram();
	glAttachShader(computeProgram, computeShader);
	glLinkProgram(computeProgram);


	int work_grp_cnt[3];
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
	std::cout << "Max work groups per compute shader" << 
		" x:" << work_grp_cnt[0] <<
		" y:" << work_grp_cnt[1] <<
		" z:" << work_grp_cnt[2] << "\n";

	int work_grp_size[3];
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
	std::cout << "Max work group sizes" <<
		" x:" << work_grp_size[0] <<
		" y:" << work_grp_size[1] <<
		" z:" << work_grp_size[2] << "\n";

	int work_grp_inv;
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
	std::cout << "Max invocations count per work group: " << work_grp_inv << "\n";

	// Load initial position
	glTextureSubImage2D(screenTex, 0, 0, 0, COMPUTE_WIDTH, COMPUTE_HEIGHT, GL_RGBA, GL_FLOAT, textureVectors[currentIteration]);

	while (!glfwWindowShouldClose(window))
	{
		glfwGetCursorPos(window, &mousexpos, &mouseypos);

		glUseProgram(screenShaderProgram);
		glBindTextureUnit(0, screenTex);
		glUniform1i(glGetUniformLocation(screenShaderProgram, "screen"), 0);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(indices[0]), GL_UNSIGNED_INT, 0);

		glfwSwapBuffers(window);
		glfwPollEvents();
		
	}


	glfwDestroyWindow(window);
	glfwTerminate();
}