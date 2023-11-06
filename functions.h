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

template <typename T, std::size_t N>
void copyArray(const std::array<T, N>& source, std::array<T, N>& destination) {
	for (std::size_t i = 0; i < N; ++i) {
		destination[i] = source[i];
	}
}

template <typename T, std::size_t N>
void printArray(const std::array<T, N>& arr) {
	std::cout << "[";
	for (const T& element : arr) {
		std::cout << element << " ";
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

void initTexture(GLuint* texture, int unitIndex, int access, int format, vec2 size = vec2(COMPUTE_WIDTH, COMPUTE_HEIGHT))
{
	glCreateTextures(GL_TEXTURE_2D, 1, texture);
	glTextureParameteri(*texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(*texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(*texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(*texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(*texture, 1, format, size.x, size.y);
	glBindImageTexture(unitIndex, *texture, 0, GL_FALSE, 0, access, format);
}


void initShaderProgram(vector<int> shaderTypes, vector<const char*> shaderNames, vector<GLuint*> shaders, GLuint* program)
{
	*program = glCreateProgram();
	int n = shaders.size();

	for (int i = 0; i < n; i++)
	{
		*shaders[i] = loadShader(shaderTypes[i], shaderNames[i]);
		glAttachShader(*program, *shaders[i]);
	}

	glLinkProgram(*program);
	
	for (int i = 0; i < n; i++)
	{
		glDeleteShader(*shaders[i]);
	}
	
}


// Convert from one grid size to another
vec2 convertCoords(vec2 coords, vec2 oldSize, vec2 newSize)
{
	float xRatio = float(oldSize.x) / float(newSize.x);
	float yRatio = float(oldSize.y) / float(newSize.y);

	int x = int(floor(coords.x / xRatio));
	int y = int(floor(coords.y / yRatio));

	return vec2(x, y);
}

// Get the tile value of the tile clicked on the divided grid
GLuint getTileValueInDividedGrid(vec2 coords)
{
	ivec2 pixelCoords = convertCoords(coords, vec2(SCREEN_WIDTH, SCREEN_HEIGHT), vec2(COMPUTE_WIDTH, COMPUTE_HEIGHT));
	ivec2 dividedPixelCoords = convertCoords(coords, vec2(SCREEN_WIDTH, SCREEN_HEIGHT), vec2(COMPUTE_WIDTH, COMPUTE_HEIGHT) * vec2(CELL_DIVISION));
	
	ivec2 subCellCoords = dividedPixelCoords - pixelCoords * ivec2(CELL_DIVISION);
	uint subCellValue = subCellCoords.x + subCellCoords.y * CELL_DIVISION;

	return subCellValue;
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

// Checks if a coordinate is valid (inside the grid)
bool isInsideGrid(vec2 coords)
{
	if (coords.x < COMPUTE_WIDTH && coords.x >= 0 && coords.y < COMPUTE_HEIGHT && coords.y >= 0)
	{
		return true;
	}
	else
	{
		return false;
	}
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

// Get the amount of uncollapsed cells
GLuint uncollapsedAmount()
{
	GLuint n = 0;
	for (int i = 0; i < COMPUTE_HEIGHT * COMPUTE_WIDTH; i++)
	{	
		if (entropyVector[i] > 1)
		{
			n++;
		}
	}
	return n;
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

void updateScreenTex()
{
	glTextureSubImage2D(screenTex, 0, 0, 0, COMPUTE_WIDTH, COMPUTE_HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_INT, &textureVector);
}

void updateTextureVector()
{
	for (int i = 0; i < COMPUTE_WIDTH; i++)
	{
		for (int j = 0; j < COMPUTE_HEIGHT; j++)
		{
			textureVector[i + COMPUTE_WIDTH * j] = grid[i][j];
		}
	}
}

void Render()
{
	updateScreenTex();
	glUseProgram(screenShaderProgram);

	glBindImageTexture(1, screenTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

	glUniform1ui(glGetUniformLocation(screenShaderProgram, "gridThickness"), GRID_THICKNESS);
	glUniform4ui(glGetUniformLocation(screenShaderProgram, "screenParams"), SCREEN_WIDTH, SCREEN_HEIGHT, COMPUTE_WIDTH, COMPUTE_HEIGHT);
	glUniform2ui(glGetUniformLocation(screenShaderProgram, "mouseCoords"), uint(mousexpos), uint(mouseypos));
	glUniform1ui(glGetUniformLocation(screenShaderProgram, "DIVIDE_CELLS"), DIVIDE_CELLS);
	glUniform1ui(glGetUniformLocation(screenShaderProgram, "CELL_DIVISION"), CELL_DIVISION);
	glUniform1ui(glGetUniformLocation(screenShaderProgram, "TILE_VALUES"), TILE_VALUES);
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(indices[0]), GL_UNSIGNED_INT, 0);

	glfwSwapBuffers(window);
	glfwPollEvents();
}



// Initialize the screen with a blank texture
void initScreen()
{
	int v = pow(2, TILE_VALUES) - 1;

	for (int i = 0; i < COMPUTE_WIDTH; i++)
	{
		for (int j = 0; j < COMPUTE_HEIGHT; j++)
		{
			grid[i][j] = v;
		}
	}

	currentIteration = 0;
	updateTextureVector();
	updateScreenTex();
}



// Use compute shader to create screenTex2 from current screenTex1
void computeNext(vec2 coordinates, GLuint chosenValue = 0, bool manualValue = false)
{
	uint startTime = getEpochTime();
	GLuint currentValue = textureVector[matrixToVecCoords(coordinates)];
	vector<vec2> collapsedTiles;

	// Choose which value to give to the cell

	if (manualValue == false)
	{
		vector<GLuint> possibleValues = possibleTiles(currentValue);

		int r = rand() % possibleValues.size();
		chosenValue = pow(2, possibleValues[r]);

		
	}
	int tile = tileValue(chosenValue);


	// Get which tiles are affected by the rules (and are not collapsed already)
	for  (int i = 0; i < rules[tile].size(); i++)
	{
		vec2 affectedTile = coordinates + vec2(rules[tile][i][0], rules[tile][i][1]);
		if (isInsideGrid(affectedTile))
		{
			if (possibleTiles(textureVector[matrixToVecCoords(affectedTile)]).size() > 1)
			{
				collapsedTiles.push_back(affectedTile);
			}
		}

	}

	// Deep copy the values from grid to previousGrid
	for (int i = 0; i < COMPUTE_WIDTH; i++)
	{
		for (int j = 0; j < COMPUTE_HEIGHT; j++)
		{
			previousGrid[i][j] = grid[i][j];
		}
	}

	// Compute new grid values
	grid[coordinates.x][coordinates.y] = chosenValue;

	for (int i = 0; i < COMPUTE_WIDTH; i++)
	{
		for (int j = 0; j < COMPUTE_HEIGHT; j++)
		{
			for (int rule = 0; rule < rules[tile].size(); rule++)
			{
				if (vec2(i, j) == coordinates + vec2(rules[tile][rule][0], rules[tile][rule][1]))
				{
					//cout << i << "  " << j << endl;
					grid[i][j] = previousGrid[i][j] & rules[tile][rule][2];
				}
			}
		}
	}

	currentIteration++;

	updateTextureVector();

	// Print time
	uint endTime = getEpochTime();
	cout << "currentIteration : " << currentIteration << "   -   " << "elapsed time : " << endTime - startTime << " ms" << endl;

	// New iteration if collapsed a tile
	for (int i = 0; i < collapsedTiles.size(); i++)
	{
		if (possibleTiles(textureVector[matrixToVecCoords(collapsedTiles[i])]).size() == 1)
		computeNext(collapsedTiles[i]);
	}
}

void runOneIteration()
{
	uint startTime = getEpochTime();

	// Find entropy values
	for (int i = 0; i < COMPUTE_WIDTH * COMPUTE_HEIGHT; i++)
	{
		GLuint cellValue = textureVector[i];
		entropyVector[i] = possibleTiles(cellValue).size();
	}

	// Find minimum entropy
	GLuint minEntropy = TILE_VALUES + 1;

	for (int i = 0; i < COMPUTE_WIDTH * COMPUTE_HEIGHT; i++)
	{
		if (minEntropy > entropyVector[i] && entropyVector[i] > 1)
		{
			minEntropy = entropyVector[i];
		}
	}

	// Get all cells with minimum entropy, and randomly choose one
	vector<vec2> minCoords;
	for (int i = 0; i < COMPUTE_WIDTH * COMPUTE_HEIGHT; i++)
	{
		GLuint cellValue = textureVector[i];
		GLuint entropy = entropyVector[i];
		if (entropy == minEntropy)
		{
			minCoords.push_back(vecToMatrixCoords(i));
		}
	}

	if (minCoords.size() != 0)
	{
		// Choose random cell
		int r = rand() % minCoords.size();
		vec2 chosenCoords = minCoords[r];

		uint endTime = getEpochTime();
		cout << "elapsed time for choosing cell : " << endTime - startTime << " ms" << endl;

		// Collapse cell
		computeNext(chosenCoords);
	}
}

void runWFC()
{
	uint64_t startTime = getEpochTime();
	
	// Repeat until all cells are collapsed
	while (true)
	{
		runOneIteration();
		if (RENDER_DURING_WFC)
		{
			uint startTime = getEpochTime();
			Render();
			uint endTime = getEpochTime();
			cout << "elapsed time for rendering : " << endTime - startTime << " ms" << endl;
		}
	
		if (uncollapsedAmount() == 0)
		{
			break;
		}
			
	}

	////Test for run time
	//for (int i = 0; i < COMPUTE_WIDTH; i++)
	//{
	//	for (int j = 0; j < COMPUTE_HEIGHT; j++)
	//	{
	//		computeNext(vec2(i, j));
	//		//Render();
	//	}
	//	
	//}


	uint64_t endTime = getEpochTime();
	cout << "Whole algorithm - elapsed time : " << endTime - startTime << " ms" << endl;
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
	if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		initScreen();
	}

}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		vec2 mouseCoords = vec2(mousexpos, mouseypos);
		vec2 coords = convertCoords(mouseCoords, vec2(SCREEN_WIDTH, SCREEN_HEIGHT), vec2(COMPUTE_WIDTH, COMPUTE_HEIGHT));
		
		//cout << dividedCoords.x <<  " , " << dividedCoords.y << endl;

		if (button == GLFW_MOUSE_BUTTON_LEFT)
		{
			if (isInsideGrid(coords))
			{

				GLuint currentValue = textureVector[matrixToVecCoords(coords)];
				// If the cell is already collapsed (has only 1 or 0 possible tile), don't do anything
				if (possibleTiles(currentValue).size() > 1)
				{
					if (DIVIDE_CELLS == false)
					{
						computeNext(coords);
					}
					else
					{
						GLuint tileNumber = getTileValueInDividedGrid(mouseCoords);
						vector<GLuint> v = possibleTiles(currentValue);
						if (find(v.begin(), v.end(), tileNumber) != v.end()) // If the chosen value is still a possible value for the cell, choose it 
						{
							GLuint tileValue = pow(2, tileNumber);
							computeNext(convertCoords(mouseCoords, vec2(SCREEN_WIDTH, SCREEN_HEIGHT), vec2(COMPUTE_WIDTH, COMPUTE_HEIGHT)), tileValue, true);
						}
						
					}
					
				}
			}
			
		}

		if (button == GLFW_MOUSE_BUTTON_RIGHT)
		{
			cout << " (" << coords.x << " , " << coords.y << ")  ";
			bitset<TILE_VALUES> x(textureVector[matrixToVecCoords(coords)]);
			cout << x << endl;
			//cout << possibleTiles((textureVector[matrixToVecCoords(coords)])).size() << endl;

		}
		

	}
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	//vec2 coords = convertCoords(vec2(mousexpos, mouseypos), vec2(SCREEN_WIDTH, SCREEN_HEIGHT), vec2(COMPUTE_WIDTH, COMPUTE_HEIGHT));
	//cout << coords.x << " , " << coords.y << endl;
	;
}