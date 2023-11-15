#include "header.h"

void runOneIteration();
void runWFC();

void initOpenGL();
void initScreen();

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
	std::cout << "]" << "\n";
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

	std::cout << "]" << "\n";
}

void loadTextureFromFile(GLuint *texture, const char* fileName)
{
	stbi_set_flip_vertically_on_load(true);
	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);
	// set the texture wrapping/filtering options (on the currently bound texture object)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// load and generate the texture
	int width, height, nrChannels;
	unsigned char* data = stbi_load(fileName, &width, &height, &nrChannels, 0);

	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << "\n";
	}
	stbi_image_free(data);
	
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

// Get vector format of a GL_R32UI texture
vector <GLuint> getTextureVector(GLuint texture)
{
	uint startTime = getEpochTime();
	vector <GLuint> vector (COMPUTE_WIDTH * COMPUTE_HEIGHT, 0);

	glBindTexture(GL_TEXTURE_2D, texture);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &vector[0]);
	glBindTexture(GL_TEXTURE_2D, 0);

	uint endTime = getEpochTime();
	if (LOG_ELAPSED_TIMES) { cout << "elapsed time for getting texture vector :  " << endTime - startTime << " ms" << "\n"; }
	return vector;
}

// Concatenates a string (char*) with an int
char* concatenate(const char* c, int i)
{
	char buffer[20];

	// Convert int to char*
	sprintf_s(buffer, "%d", i + 1);

	char str1[] = "loadedTexture";
	char result[100];

	// Copy the first string to the result
	strcpy_s(result, str1); 
	// Concatenate the second string to the result
	strcat_s(result, buffer); 
	
	return result;
}

// Initialize the screen with a blank texture
void initScreen()
{
	vector <int> arr(COMPUTE_WIDTH * COMPUTE_HEIGHT, pow(2, TILE_VALUES) - 1);

	currentIteration = 0;
	glTextureSubImage2D(computeTex1, 0, 0, 0, COMPUTE_WIDTH, COMPUTE_HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_INT, &arr[0]);

	activeTexture = &computeTex1;
	inactiveTexture = &computeTex2;
}

void Render()
{
	//// Imgui init
	if (IMGUI)
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	// Use compute program
	glUseProgram(screenShaderProgram);

	// Get the current computeTex to render
	glBindImageTexture(1, *activeTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

	// Send externally loaded textures to gpu
	for (int i = 0; i < loadedTextures.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0+i);
		glBindTexture(GL_TEXTURE_2D, loadedTextures[i]);
		glUniform1i(glGetUniformLocation(screenShaderProgram, concatenate("loadedTexture", i)), i);
	}

	// Send uniforms
	glUniform1ui(glGetUniformLocation(screenShaderProgram, "gridThickness"), GRID_THICKNESS);
	glUniform4ui(glGetUniformLocation(screenShaderProgram, "screenParams"), SCREEN_WIDTH, SCREEN_HEIGHT, COMPUTE_WIDTH, COMPUTE_HEIGHT);
	glUniform2ui(glGetUniformLocation(screenShaderProgram, "mouseCoords"), uint(mousexpos), uint(mouseypos));
	glUniform1ui(glGetUniformLocation(screenShaderProgram, "DIVIDE_CELLS"), DIVIDE_CELLS);
	glUniform1ui(glGetUniformLocation(screenShaderProgram, "CELL_DIVISION"), CELL_DIVISION);
	glUniform1ui(glGetUniformLocation(screenShaderProgram, "TILE_VALUES"), TILE_VALUES);
	glUniform1ui(glGetUniformLocation(screenShaderProgram, "COLOR_FROM_TEXTURE"), COLOR_FROM_TEXTURE);
	glUniform1ui(glGetUniformLocation(screenShaderProgram, "SUDOKU"), SUDOKU);
	glBindVertexArray(VAO);

	// Draw
	glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(indices[0]), GL_UNSIGNED_INT, 0);

	//// ImGUI
	bool runwfc = false;
	if (IMGUI)
	{
		ImGui::Begin("ImGUI window");
		ImGui::SetWindowSize(ImVec2(500, 300));
		ImGui::Checkbox("LOG ELAPSED TIMES", &LOG_ELAPSED_TIMES);
		ImGui::Checkbox("DIVIDE CELLS", &DIVIDE_CELLS);
		ImGui::Checkbox("COLOR FROM TEXTURE", &COLOR_FROM_TEXTURE);
		ImGui::Checkbox("SUDOKU", &SUDOKU);
		ImGui::Checkbox("RENDER DURING WFC", &RENDER_DURING_WFC);
		ImGui::SliderInt("GRID THICKNESS", &GRID_THICKNESS, 0, 10);
		if (ImGui::SliderInt("COMPUTE WIDTH", &COMPUTE_WIDTH, 1, 1024))
		{
			initOpenGL();
			initScreen();
		}
		if (ImGui::SliderInt("COMPUTE HEIGHT", &COMPUTE_HEIGHT, 1, 1024))
		{
			initOpenGL();
			initScreen();
		}
		
		
		if (ImGui::Button("Run Whole Algorithm (<-)"))
		{
			runwfc = true;
			
		}
		if (ImGui::Button("Run One Iteration (->)"))
		{
			runOneIteration();
		}
		if (ImGui::Button("Restart (r)"))
		{
			initScreen();
		}
		ImGui::End();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}
	

	glfwSwapBuffers(window);
	glfwPollEvents();

	if (runwfc)
	{
		runWFC();
	}
}

void swap(GLuint* a, GLuint* b)
{
	GLuint temp = *a;
	*a = *b;
	*b = temp;
}

// Use compute shader to create next iteration in one of the computeTextures
void computeNext(ivec2 coordinates = ivec2(0, 0), uint chosenValue = 0, bool manualCoords = false, bool manualValue = false)
{
	uint startTime = getEpochTime();

	// Set input and output textures
	glBindImageTexture(1, *activeTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	glBindImageTexture(2, *inactiveTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	
	
	// Use compute shader
	if (!SUDOKU)
	{
		glUseProgram(computeProgram);
	}
	else
	{
		glUseProgram(sudokuComputeProgram);
	}
	
	// Send uniform values to compute shader
	float random1 = ((double)rand()) / RAND_MAX;
	float random2 = ((double)rand()) / RAND_MAX;

	glUniform1ui(glGetUniformLocation(computeProgram, "userChosenValue"), chosenValue);
	glUniform2i(glGetUniformLocation(computeProgram, "userChosenCoords"), coordinates.x, coordinates.y);
	glUniform1ui(glGetUniformLocation(computeProgram, "manualCoords"), manualCoords);
	glUniform1ui(glGetUniformLocation(computeProgram, "manualValue"), manualValue);
	glUniform1f(glGetUniformLocation(computeProgram, "random1"), random1);
	glUniform1f(glGetUniformLocation(computeProgram, "random2"), random2);
	glUniform1ui(glGetUniformLocation(computeProgram, "TILE_VALUES"), TILE_VALUES);

	// Run compute shader
	glDispatchCompute(COMPUTE_WIDTH, COMPUTE_HEIGHT, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	currentIteration++;
	//swap(&activeTexture, &inactiveTexture);

	swap(activeTexture, inactiveTexture);

	// Print time
	uint endTime = getEpochTime();
	if (LOG_ELAPSED_TIMES) { cout << "currentIteration : " << currentIteration << "   -   " << "elapsed time : " << endTime - startTime << " ms \n"; }
}

void runComputeEntropyProgram()
{
	// Set input texture
	glBindImageTexture(1, *activeTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	
	// Atomic Counter
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, minEntropyBuffer);
	GLuint atomicCounter = TILE_VALUES;
	glClearBufferData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &atomicCounter);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, minEntropyBuffer);

	// Use compute shader
	glUseProgram(computeEntropyProgram);
	
	// Send uniform values to compute shader
	glUniform1ui(glGetUniformLocation(computeEntropyProgram, "TILE_VALUES"), TILE_VALUES);
	
	// Run compute shader
	glDispatchCompute(COMPUTE_WIDTH, COMPUTE_HEIGHT, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void runChooseTileValueProgram()
{
	// Set input texture
	glBindImageTexture(1, *activeTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

	// Buffers
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, minEntropyCellsBuffer);
	ivec2 minEntropyCells = ivec2(-1,-1);
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &minEntropyCells);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, minEntropyCellsBuffer);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, minEntropyCellsAmountBuffer);
	GLuint minEntropyCellsAmount = 0;
	glClearBufferData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &minEntropyCellsAmount);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, minEntropyCellsAmountBuffer);

	// Use compute shader
	glUseProgram(chooseTileValueProgram);

	// Send uniform values to compute shader
	glUniform1ui(glGetUniformLocation(chooseTileValueProgram, "TILE_VALUES"), TILE_VALUES);

	// Run compute shader
	glDispatchCompute(COMPUTE_WIDTH, COMPUTE_HEIGHT, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void runOneIteration()
{
	uint startTime = getEpochTime();

	// Find entropy values
	runComputeEntropyProgram();

	runChooseTileValueProgram();

	// atomic
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, minEntropyCellsAmountBuffer);
	GLuint* ptr = (GLuint*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT);
	minEntropyCellsAmount = *ptr;
	glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

	uint endTime = getEpochTime();
	
	if (LOG_ELAPSED_TIMES) { cout << "elapsed time for choosing cell : " << endTime - startTime << " ms" << "\n";}

	computeNext();
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
			if (LOG_ELAPSED_TIMES) {cout << "elapsed time for rendering : " << endTime - startTime << " ms" << "\n";}
		}
	
		if (minEntropyCellsAmount == 0)
		{
			break;
		}
			
	}

	uint64_t endTime = getEpochTime();
	cout << "Whole algorithm - elapsed time : " << endTime - startTime << " ms" << "\n";
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
		
		//cout << dividedCoords.x <<  " , " << dividedCoords.y << "\n";

		if (button == GLFW_MOUSE_BUTTON_LEFT)
		{
			if (isInsideGrid(coords))
			{
				textureVector = getTextureVector(*activeTexture);
				GLuint currentValue = textureVector[matrixToVecCoords(coords)];
				// If the cell is already collapsed (has only 1 or 0 possible tile), don't do anything
				if (possibleTiles(currentValue).size() > 1)
				{
					if (DIVIDE_CELLS == false)
					{
						computeNext(coords, 0, true, false);
					}
					else
					{
						GLuint tileNumber = getTileValueInDividedGrid(mouseCoords);
						vector<GLuint> v = possibleTiles(currentValue);
						if (find(v.begin(), v.end(), tileNumber) != v.end()) // If the chosen value is still a possible value for the cell, choose it 
						{
							GLuint tileValue = pow(2, tileNumber);
							computeNext(convertCoords(mouseCoords, vec2(SCREEN_WIDTH, SCREEN_HEIGHT), vec2(COMPUTE_WIDTH, COMPUTE_HEIGHT)), tileValue, true, true);
						}
						
					}
					
				}
			}
			
		}

		if (button == GLFW_MOUSE_BUTTON_RIGHT)
		{
			cout << " (" << coords.x << " , " << coords.y << ")  ";
			textureVector = getTextureVector(*activeTexture);
			bitset<TILE_VALUES> x(textureVector[matrixToVecCoords(coords)]);
			cout << x << "\n";
			//cout << possibleTiles((textureVector[matrixToVecCoords(coords)])).size() << "\n";

		}
		

	}
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	//vec2 coords = convertCoords(vec2(mousexpos, mouseypos), vec2(SCREEN_WIDTH, SCREEN_HEIGHT), vec2(COMPUTE_WIDTH, COMPUTE_HEIGHT));
	//cout << coords.x << " , " << coords.y << "\n";
	;
}

void initOpenGL()
{
	// Buffers
	glGenBuffers(1, &minEntropyBuffer);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, minEntropyBuffer);
	glBufferStorage(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), nullptr, GL_MAP_READ_BIT);

	glGenBuffers(1, &minEntropyCellsBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, minEntropyCellsBuffer);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(ivec2) * COMPUTE_WIDTH * COMPUTE_HEIGHT, nullptr, GL_MAP_READ_BIT);

	glGenBuffers(1, &minEntropyCellsAmountBuffer);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, minEntropyCellsAmountBuffer);
	glBufferStorage(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), nullptr, GL_MAP_READ_BIT);


	// Textures
	initTexture(&computeTex1, 1, GL_READ_WRITE, GL_R32UI);
	initTexture(&computeTex2, 2, GL_READ_WRITE, GL_R32UI);
	initTexture(&entropyTex, 3, GL_READ_WRITE, GL_R32UI);

	for (int i = 0; i < textureLocations.size(); i++)
	{
		loadedTextures.push_back(0);
		loadTextureFromFile(&loadedTextures[i], textureLocations[i]);
	}


	// Shaders, programs
	initShaderProgram({ GL_COMPUTE_SHADER }, { "computeShader.comp" }, { &computeShader }, &computeProgram);
	initShaderProgram({ GL_COMPUTE_SHADER }, { "computeEntropyShader.comp" }, { &computeEntropyShader }, &computeEntropyProgram);
	initShaderProgram({ GL_COMPUTE_SHADER }, { "chooseTileValueShader.comp" }, { &chooseTileValueShader }, &chooseTileValueProgram);
	initShaderProgram({ GL_COMPUTE_SHADER }, { "sudokuComputeShader.comp" }, { &sudokuComputeShader }, &sudokuComputeProgram);
	initShaderProgram({ GL_VERTEX_SHADER , GL_FRAGMENT_SHADER }, { "vertexShader.vert","fragmentShader.frag" }, { &screenVertexShader,&screenFragmentShader }, &screenShaderProgram);

}