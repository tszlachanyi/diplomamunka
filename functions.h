#include "header.h"

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
template <typename T>
void printVector(vector<T> vector)
{
	int N = vector.size();
	std::cout << "[";

	for (int i = 0; i < N; i++)
	{
		std::cout << vector[i];
		if (i != N - 1)
		{
			std::cout << ", ";
		}
	}

	std::cout << "]" << std::endl;
}

void initTexture(GLuint* texture, int unitIndex, int access, int format)
{
	glCreateTextures(GL_TEXTURE_2D, 1, texture);
	glTextureParameteri(*texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(*texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(*texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(*texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(*texture, 1, format, COMPUTE_WIDTH, COMPUTE_HEIGHT);
	glBindImageTexture(unitIndex, *texture, 0, GL_FALSE, 0, access, format);
}

void Render()
{
	glUseProgram(screenShaderProgram);
	glBindTextureUnit(0, screenTex);
	glUniform1i(glGetUniformLocation(screenShaderProgram, "screen"), 0);
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(indices[0]), GL_UNSIGNED_INT, 0);

	glfwSwapBuffers(window);
	glfwPollEvents();
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

// Converts from matrix coordinate to texture coordinate of a pixel
vec2 vecToMatrixCoords(int coords)
{
	int y = floor(coords / COMPUTE_WIDTH);
	int x = coords - y * COMPUTE_WIDTH;
	return vec2(x, y);
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

// All different tiles the cell can be currently		entropy is the size of this vector
vector<GLuint> possibleTiles(GLuint number)
{
	vector<GLuint> v;
	for (GLuint i = 0; i < TILE_VALUES; i++)
	{
		if (nthBit(number, i) == 1)
		{
			v.push_back(i);
		}
	}
	return v;
}

// Get the coordinates of all uncollapsed cells
vector<vec2> uncollapsedCells()
{
	vector<vec2> tiles;
	for (int i = 0; i < COMPUTE_HEIGHT * COMPUTE_WIDTH; i++)
	{	
		GLuint entropy = possibleTiles(textureVectors[currentIteration][i]).size();
		if (entropy > 1)
		{
			tiles.push_back(vecToMatrixCoords(i));
		}
	}
	return tiles;
}

// Get which tile the 2bit value corresponds to (assuming there is only one possible value)
int tileValue(GLuint number)
{
	int n = 999;
	if (possibleTiles(number).size() == 1)
	{
		for (int i = 0; i < TILE_VALUES; i++)
		{
			if (number >> i == 1)
			{
				n = i;
			}
		}
	}
	
	return n;
}

// Gets the color of the cell based on it's GLuint value
vec4 getTileColor(GLuint number)
{
	vec4 color;

	switch (number) {
	case 0b0001:
		color = vec4(1, 0, 0, 1);
		break;
	case 0b0010:
		color = vec4(0, 1, 0, 1);
		break;
	case 0b0100:
		color = vec4(0, 0, 1, 1);
		break;
	case 0b1000:
		color = vec4(1, 1, 0, 1);
		break;
	case 0b0000:
		color = vec4(0, 0, 0, 1);
		break;
	default:
		color = vec4(1, 1, 1, 1);
	}

	return color;
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

// Get the value for screenTex from the current iteration of textureVectors 
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
	uint64_t startTime = getEpochTime();
	GLuint currentValue = textureVectors[currentIteration][matrixToVecCoords(coordinates)];
	
	// If the cell is already collapsed (has only 1 or 0 possible tile), don't do anything
	if (possibleTiles(currentValue).size() > 1)
	{
		// Choose which value to give to the cell
		GLuint chosenValue = 0;
		vector<GLuint> possibleValues = possibleTiles(currentValue);
		
		
		int r = rand() % possibleValues.size();
		chosenValue = pow(2, possibleValues[r]);
		
		int tile = tileValue(chosenValue);

		// Converting rules into required format
		vector<GLint> v = flatten(rules[tile]);
		GLint* flattenedRules = new int[v.size()];
		copy(v.begin(), v.end(), flattenedRules);

		// Load data from textureVectors into loadTex
		glTextureSubImage2D(loadTex, 0, 0, 0, COMPUTE_WIDTH, COMPUTE_HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_INT, textureVectors[currentIteration]);

		// Run compute shader
		glUseProgram(computeProgram);

		// Send uniform values to compute shader
		glUniform3iv(uLocationRules, MAXIMUM_RULES, flattenedRules);
		glUniform1ui(uLocationRulesAmount, GLuint(v.size() / 3));
		glUniform1ui(uLocationChosenValue, chosenValue);
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

		cout << "currentIteration : " << currentIteration << "   -   ";
		delete[] flattenedRules;
		uint64_t endTime = getEpochTime();
		std::cout << "elapsed time : " << endTime - startTime << " ms" << std::endl;
	}

}

void runOneIteration()
{
	// Find minimum entropy
	uncollapsed = uncollapsedCells();
	GLuint minEntropy = TILE_VALUES + 1;

	for (int i = 0; i < uncollapsed.size(); i++)
	{
		vec2 coords = uncollapsed[i];
		GLuint cellValue = textureVectors[currentIteration][matrixToVecCoords(coords)];
		GLuint entropy = possibleTiles(cellValue).size();

		if (minEntropy > entropy)
		{
			minEntropy = entropy;
		}
	}

	// Get all cells with minimum entropy, and randomly choose one
	vector<vec2> minCoords;
	for (int i = 0; i < uncollapsed.size(); i++)
	{
		vec2 coords = uncollapsed[i];
		GLuint cellValue = textureVectors[currentIteration][matrixToVecCoords(coords)];
		GLuint entropy = possibleTiles(cellValue).size();
		if (entropy == minEntropy)
		{
			minCoords.push_back(coords);
		}
	}

	if (minCoords.size() != 0)
	{
		// Choose random cell
		
		int r = rand() % minCoords.size();
		vec2 chosenCoords = minCoords[r];

		// Collapse cell
		computeNext(chosenCoords);
	}
}

void runWFC()
{
	uncollapsed = uncollapsedCells();

	// Repeat until all cells are collapsed
	while (uncollapsed.size() != 0)
	{
		runOneIteration();
		Render();
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
		runOneIteration();
		
	}
	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
	{
		runWFC();

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
				
				computeNext(screenToTextureCoords(vec2(mousexpos, mouseypos)));
				
			}

			if (button == GLFW_MOUSE_BUTTON_RIGHT)
			{
				cout << " (" << coords.x << " , " << coords.y << ")  ";
				bitset<TILE_VALUES> x(textureVectors[currentIteration][matrixToVecCoords(coords)]);
				cout << x << endl;
				//cout << possibleTiles((textureVectors[currentIteration][matrixToVecCoords(coords)])).size() << endl;

			}

			updateScreenTex();
		}

	}
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	updateScreenTex();
}