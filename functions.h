#include "header.h"

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

// Flatten vector of vectors into vector
template<typename T>
vector<T> flatten(vector<vector<T>> const& vec)
{
	vector<T> flattened;
	for (auto const& v : vec) {
		flattened.insert(flattened.end(), v.begin(), v.end());
	}
	return flattened;
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

// Return nth bit of base-2 representation (counting from the end, starting with 0)
GLuint nthBit(GLuint number, GLuint n)
{
	return (number >> n) & 1;
}

// Gets the color of the cell based on it's GLuint value
vec4 getTileColor(GLuint number)
{
	int possibleTiles = 0;	// amount of tiles the cell can get
	int tile = 0;			// number of the tile the cell gets the color from
	vec4 color = vec4(0.5, 0.5, 0.5, 1);

	for (int i = 0; i < TILE_VALUES; i++)
	{
		if (nthBit(number, i) == 1)
		{
			tile = i;
			possibleTiles++;
		}
	}

	if (possibleTiles == 1)
	{
		color = tileColors[tile];
	}

	return color;
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

// Assign an array to the ith array of textureVectors
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
	vec2 mouseCoords = screenToTextureCoords(vec2(mousexpos, mouseypos));
	
	// Get colors for the screen from the textureVectors array
	for (int j = 0; j < COMPUTE_WIDTH * COMPUTE_HEIGHT; j++)
	{
		vec4 tileColor = getTileColor(textureVectors[currentIteration][j]);
		

		for (int i = 0; i <= 3; i++)
		{
			rgbaVector[4 * j + i] = tileColor[i];
		}
	}

	// Effect for hovering on cells
	if (mousexpos >= 0 && mouseypos >= 0 && mousexpos < SCREEN_WIDTH && mouseypos < SCREEN_HEIGHT)
	{
		for (int j = 0; j <= 3; j++)
		{
			GLfloat* color = &rgbaVector[int(mouseCoords.x) * 4 + int(mouseCoords.y) * COMPUTE_WIDTH * 4 + j];
			*color = *color * 0.5 + 0.25;
		}
	}
	

	// Set the colors for screenTex
	glTextureSubImage2D(screenTex, 0, 0, 0, COMPUTE_WIDTH, COMPUTE_HEIGHT, GL_RGBA, GL_FLOAT, rgbaVector);
}


// Use compute shader to create computedTex from current loadTex
void computeNext(vec2 coordinates)
{

	if (computedIterations > currentIteration)
	{
		currentIteration++;
		updateScreenTex();

		cout << "currentIteration : " << currentIteration << " computedIterations : " << computedIterations << endl;
	}
	else
	{
		// Load data from textureVectors into loadTex
		glTextureSubImage2D(loadTex, 0, 0, 0, COMPUTE_WIDTH, COMPUTE_HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_INT, textureVectors[currentIteration]);

		// Converting rules into required format
		vector<GLint> v = flatten(rules[0]);
		GLint* flattenedRules = new int[v.size()];
		copy(v.begin(), v.end(), flattenedRules);

		// Run compute shader
		glUseProgram(computeProgram);

		// Send uniform values to compute shader
		glUniform3iv(uLocationRules, MAXIMUM_RULES, flattenedRules);
		glUniform1ui(uLocationRulesAmount, GLuint(v.size() / 3));
		glUniform2ui(uLocationCoordinates, coordinates.x, coordinates.y);

		glDispatchCompute(COMPUTE_WIDTH, COMPUTE_HEIGHT, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		// Save the new iteration
		currentIteration++;
		computedIterations = currentIteration;
		GLuint* textureVector = getTextureVector(computedTex);
		assignToTextureVectorsArray(currentIteration, textureVector);

		// Render it
		updateScreenTex();

		cout << "currentIteration : " << currentIteration  << endl;
		delete[] flattenedRules;
	}


}

// Input
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}

}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		vec2 coords = screenToTextureCoords(vec2(mousexpos, mouseypos));
		

		if (clickingAllowed)
		{

			if (button == GLFW_MOUSE_BUTTON_LEFT)
			{
				uint64_t startTime = getEpochTime();
				computeNext(screenToTextureCoords(vec2(mousexpos, mouseypos)));
				uint64_t endTime = getEpochTime();
				std::cout << "elapsed time : " << endTime - startTime << " ms" << std::endl;
			}

			if (button == GLFW_MOUSE_BUTTON_RIGHT)
			{
				cout << " (" << coords.x << " , " << coords.y << ")" << endl;
				bitset<8> x(textureVectors[currentIteration][matrixToVecCoords(coords)]);
				cout << x << endl;

			}

			updateScreenTex();
		}

	}
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	updateScreenTex();
}