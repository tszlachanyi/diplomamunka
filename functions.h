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

// Converts from screen coordinates to coordinates of the textures
vec2 screenToTextureCoords(vec2 coords)
{
	float xRatio = float(SCREEN_WIDTH) / float(COMPUTE_WIDTH);
	float yRatio = float(SCREEN_HEIGHT) / float(COMPUTE_HEIGHT);

	int x = int(floor(coords.x / xRatio));
	int y = (COMPUTE_HEIGHT - 1) - int(floor(coords.y / yRatio));

	return vec2(x, y);
}

// Converts from screen coordinates to coordinates of the divided texture
vec2 screenToDividedTextureCoords(vec2 coords)
{
	float xRatio = float(SCREEN_WIDTH) / float(COMPUTE_WIDTH) / float(CELL_DIVISION);
	float yRatio = float(SCREEN_HEIGHT) / float(COMPUTE_HEIGHT) / float(CELL_DIVISION);

	int x = int(floor(coords.x / xRatio));
	int y = (COMPUTE_HEIGHT * CELL_DIVISION - 1) - int(floor(coords.y / yRatio));

	return vec2(x, y);
}

// Get the tile value of the tile clicked on the divided grid
GLuint getTileValueInDividedGrid(vec2 coords)
{
	vec2 textureCoords = screenToTextureCoords(coords);
	vec2 dividedTextureCoords = screenToDividedTextureCoords(coords);
	vec2 tileCoords = vec2(dividedTextureCoords.x, dividedTextureCoords.y) - vec2(textureCoords.x * CELL_DIVISION, textureCoords.y * CELL_DIVISION);
	
	int tileNumber = tileCoords.x + tileCoords.y * CELL_DIVISION;
	
	//cout << tileCoords.x << "," << tileCoords.y << "   " << tileValue << endl;

	return tileNumber;
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

// Get vector format of a GL_R32UI texture
array <GLuint, COMPUTE_WIDTH * COMPUTE_HEIGHT> getTextureVector(GLuint texture)
{
	uint startTime = getEpochTime();
	array <GLuint, COMPUTE_WIDTH * COMPUTE_HEIGHT> vector;

	glBindTexture(GL_TEXTURE_2D, texture);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &vector);
	glBindTexture(GL_TEXTURE_2D, 0);

	uint endTime = getEpochTime();
	cout << "elapsed time for getting texture vector :  " << endTime - startTime << " ms" << endl;
	return vector;
}

// Get the value for screenTex from textureVector
void updateScreenTex()
{
	vec2 mouseCoords = screenToTextureCoords(vec2(mousexpos, mouseypos));

	if (DIVIDE_CELLS == false)
	{
		GLfloat rgbaVector[4 * COMPUTE_WIDTH * COMPUTE_HEIGHT];


		// Get colors for the screen from the textureVectors array
		for (int j = 0; j < COMPUTE_WIDTH * COMPUTE_HEIGHT; j++)
		{
			vec4 tileColor = getTileColor(textureVector[j]);


			for (int i = 0; i <= 3; i++)
			{
				rgbaVector[4 * j + i] = tileColor[i];
			}
		}

		// Effect for hovering on cells
		if (mouseCoords.x >= 0 && mouseCoords.y >= 0 && mouseCoords.x < COMPUTE_WIDTH && mouseCoords.y < COMPUTE_HEIGHT)
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
	else
	{
		GLfloat rgbaVector[4 * COMPUTE_WIDTH * COMPUTE_HEIGHT * CELL_DIVISION * CELL_DIVISION];

		// Get colors for the screen from the textureVectors array
		for (int j = 0; j < COMPUTE_HEIGHT * CELL_DIVISION; j++)
		{
			for (int i = 0; i < COMPUTE_WIDTH * CELL_DIVISION; i++)
			{
				vec4 tileColor;
				vec2 cellCoords = vec2(floor(i / CELL_DIVISION) , floor(j / CELL_DIVISION));
				vec2 tileCoords = vec2(i,j) - vec2(cellCoords.x * CELL_DIVISION, cellCoords.y * CELL_DIVISION);
				int tileNumber = tileCoords.x + tileCoords.y * CELL_DIVISION;
				GLuint cellValue = textureVector[matrixToVecCoords(cellCoords)];
				//cout << cellCoords.x << " , " << cellCoords.y << "     " << tileNumber << endl;

				
				if (possibleTiles(cellValue).size() > 1) // If cell not collapsed yet, display all possible tiles
				{
					int tileValue = pow(2, tileNumber);
					tileColor = getTileColor(tileValue & cellValue);
					tileColor = tileColor * vec4(0.8, 0.8, 0.8, 1);
				}
				else // If cell already collapsed, only display the tile value it got
				{
					tileColor = getTileColor(cellValue);
				}
				

				for (int k = 0; k <= 3; k++)
				{
					rgbaVector[4 * (COMPUTE_HEIGHT * CELL_DIVISION * j + i) + k] = tileColor[k];
				}
			}
		}

		glTextureSubImage2D(screenTexDivided, 0, 0, 0, COMPUTE_WIDTH * CELL_DIVISION, COMPUTE_HEIGHT * CELL_DIVISION, GL_RGBA, GL_FLOAT, rgbaVector);
	}



}

void Render()
{
	updateScreenTex();

	glUseProgram(screenShaderProgram);

	if (DIVIDE_CELLS)
	{
		glBindTextureUnit(0, screenTexDivided);
	}
	else
	{
		glBindTextureUnit(0, screenTex);
	}

	glUniform1ui(glGetUniformLocation(screenShaderProgram, "gridThickness"), GRID_THICKNESS);
	glUniform4ui(glGetUniformLocation(screenShaderProgram, "screenParams"), SCREEN_WIDTH, SCREEN_HEIGHT, COMPUTE_WIDTH, COMPUTE_HEIGHT);
	glUniform1i(glGetUniformLocation(screenShaderProgram, "screen"), 0);
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(indices[0]), GL_UNSIGNED_INT, 0);

	glfwSwapBuffers(window);
	glfwPollEvents();
}


// Initialize the screen with a blank texture
void initScreen()
{
	int v = pow(2, TILE_VALUES) - 1;

	array <GLuint, COMPUTE_WIDTH* COMPUTE_HEIGHT> arr;
	for (int i = 0; i < COMPUTE_WIDTH * COMPUTE_HEIGHT; i++) {
		arr[i] = v;
	}

	currentIteration = 0;
	copyArray(arr, textureVector);
	glTextureSubImage2D(computeTex1, 0, 0, 0, COMPUTE_WIDTH, COMPUTE_HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_INT, &textureVector);
	updateScreenTex();
}

// Use compute shader to create computeTex2 from current computeTex1
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

	// Converting rules into required format
	vector<GLint> v = flatten(rules[tile]);
	GLint* flattenedRules = new int[v.size()];
	copy(v.begin(), v.end(), flattenedRules);

	// Set input and output textures
	if (currentIteration % 2 == 0)
	{
		glBindImageTexture(1, computeTex1, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
		glBindImageTexture(2, computeTex2, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	}
	else
	{
		glBindImageTexture(1, computeTex2, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
		glBindImageTexture(2, computeTex1, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	}
	
	// Run compute shader
	glUseProgram(computeProgram);

	// Send uniform values to compute shader
	//float r = fract(sin(startTime) * 43758.5453); // random value between 0 and 1

	glUniform3iv(glGetUniformLocation(computeProgram, "rules"), MAXIMUM_RULES, flattenedRules);
	glUniform1ui(glGetUniformLocation(computeProgram, "rulesAmount"), GLuint(v.size() / 3));
	glUniform1ui(glGetUniformLocation(computeProgram, "chosenValue"), chosenValue);
	glUniform2ui(glGetUniformLocation(computeProgram, "coordinates"), coordinates.x, coordinates.y);

	glDispatchCompute(COMPUTE_WIDTH, COMPUTE_HEIGHT, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	// Save the new iteration
	if (currentIteration % 2 == 0)
	{
		textureVector = getTextureVector(computeTex2);
	}
	else
	{
		textureVector = getTextureVector(computeTex1);
	}
	
	currentIteration++;

	// Print time
	uint endTime = getEpochTime();
	cout << "currentIteration : " << currentIteration << "   -   " << "elapsed time : " << endTime - startTime << " ms" << endl;
	delete[] flattenedRules;

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
			Render();
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
		vec2 coords = screenToTextureCoords(mouseCoords);
		
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
						printVector(v);
						if (find(v.begin(), v.end(), tileNumber) != v.end()) // If the chosen value is still a possible value for the cell, chose it 
						{
							GLuint tileValue = pow(2, tileNumber);
							computeNext(screenToTextureCoords(mouseCoords), tileValue, true);
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
	;
}