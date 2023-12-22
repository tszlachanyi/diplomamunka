#include "header.h"

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

vec3 loadTextureFromFile(GLuint *texture, const char* fileName)
{
	stbi_set_flip_vertically_on_load(true);
	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// load and generate the texture
	int width, height, nrChannels;
	unsigned char* data = stbi_load(fileName, &width, &height, &nrChannels, 4);

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
	return vec3(width, height, nrChannels);
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

// Get the tile number of the tile clicked on the divided grid
GLuint getTileNumberInDividedGrid(vec2 coords)
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

// Checks if a coordinate is valid (inside the grid)
bool isInsideGrid(vec2 coords, vec2 gridSize)
{
	return (coords.x < gridSize.x && coords.x >= 0 && coords.y < gridSize.y && coords.y >= 0);
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
	currentIteration = 0;
	CELL_DIVISION = ceil(sqrt(TILE_VALUES));

	// Fill textures with initial values
	vector <int> arr1(COMPUTE_WIDTH * COMPUTE_HEIGHT, pow(2, TILE_VALUES) - 1);
	vector <int> arr2(COMPUTE_WIDTH * COMPUTE_HEIGHT, TILE_VALUES);
	glTextureSubImage2D(gridImage, 0, 0, 0, COMPUTE_WIDTH, COMPUTE_HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_INT, &arr1[0]);
	glTextureSubImage2D(entropyImage, 0, 0, 0, COMPUTE_WIDTH, COMPUTE_HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_INT, &arr2[0]);
}

// Read data to CPU from an opengl buffer
template <typename T>
T* readBuffer(GLuint buffer, GLuint type, GLsizeiptr size)
{
	glBindBuffer(type, buffer);
	T* ptr = (T*)glMapBufferRange(type, 0, size, GL_MAP_READ_BIT);
	glUnmapBuffer(type);
	glBindBuffer(type, 0);
	return ptr;
}

void runGetMinEntropyProgram()
{
	// Atomic Counter
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, minEntropyBuffer);
	GLuint atomicCounter = TILE_VALUES;
	glClearBufferData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &atomicCounter);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, minEntropyBuffer);

	// Use compute shader
	glUseProgram(getMinEntropyProgram);

	// Send uniform values to compute shader
	glUniform1ui(glGetUniformLocation(getMinEntropyProgram, "TILE_VALUES"), TILE_VALUES);

	// Run compute shader
	glDispatchCompute(COMPUTE_WIDTH, COMPUTE_HEIGHT, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void runGetMinEntropyCellsProgram()
{

	// Buffers
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, minEntropyCellsBuffer);
	ivec2 minEntropyCells = ivec2(-1, -1);
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &minEntropyCells);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, minEntropyCellsBuffer);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, minEntropyCellsAmountBuffer);
	GLuint minEntropyCellsAmount = 0;
	glClearBufferData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &minEntropyCellsAmount);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, minEntropyCellsAmountBuffer);

	// Use compute shader
	glUseProgram(getMinEntropyCellsProgram);

	// Send uniform values to compute shader
	glUniform1ui(glGetUniformLocation(getMinEntropyCellsProgram, "TILE_VALUES"), TILE_VALUES);

	// Run compute shader
	glDispatchCompute(COMPUTE_WIDTH, COMPUTE_HEIGHT, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

// Run one step of the algorithm
void computeNext(ivec2 coordinates = ivec2(0, 0), uint chosenValue = 0, bool manualCoords = false, bool manualValue = false)
{
	uint startTime = getEpochTime();
	
	// Get all cells with minimal entropy
	if (!manualCoords) 
	{
		// Calculate min entropy cells on gpu
		runGetMinEntropyProgram();
		runGetMinEntropyCellsProgram();

		// Read minEntropyCellsAmount atomic counter
		minEntropyCellsAmount = *(readBuffer <GLuint>(minEntropyCellsAmountBuffer, GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint)));

		uint endTime = getEpochTime();
		if (LOG_ELAPSED_TIMES) { cout << "Elapsed time for getting min entropy : " << endTime - startTime << " ms" << "\n"; }

		if (minEntropyCellsAmount == 0)
		{
			return;
		}
	}

	// Fill Buffers
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, collapsedCellsBuffer);
	ivec2 collapsedCell = ivec2(-1, -1);
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &collapsedCell);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, collapsedCellsBuffer);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, collapsedCellsAmountBuffer);
	GLuint collapsedCellsAmount = 0;
	glClearBufferData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &collapsedCellsAmount);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 4, collapsedCellsAmountBuffer);
	
	// Use compute shader
	glUseProgram(useRulesProgram);
	
	// Send uniform values to compute shader
	float random1 = ((double)rand()) / RAND_MAX;
	float random2 = ((double)rand()) / RAND_MAX;

	glUniform1ui(glGetUniformLocation(useRulesProgram, "userChosenValue"), chosenValue);
	glUniform2i(glGetUniformLocation(useRulesProgram, "userChosenCoords"), coordinates.x, coordinates.y);
	glUniform1ui(glGetUniformLocation(useRulesProgram, "manualCoords"), manualCoords);
	glUniform1ui(glGetUniformLocation(useRulesProgram, "manualValue"), manualValue);
	glUniform1f(glGetUniformLocation(useRulesProgram, "random1"), random1);
	glUniform1f(glGetUniformLocation(useRulesProgram, "random2"), random2);
	glUniform1ui(glGetUniformLocation(useRulesProgram, "TILE_VALUES"), TILE_VALUES);
	glUniform1ui(glGetUniformLocation(useRulesProgram, "SUDOKU"), SUDOKU);
	glUniform1ui(glGetUniformLocation(useRulesProgram, "NEIGHBOURS_AMOUNT"), NEIGHBOURS_AMOUNT);
	if (NEIGHBOURS_AMOUNT != 0)
	{
		glUniform2iv(glGetUniformLocation(useRulesProgram, "neighbours"), NEIGHBOURS_AMOUNT, &neighbours[0].x);
	}
	

	// Run compute shader
	glDispatchCompute(NEIGHBOURS_AMOUNT + 1, 1, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	currentIteration++;

	// Read buffers
	collapsedCellsAmount = *(readBuffer<GLuint>(collapsedCellsAmountBuffer, GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint)));

	if (collapsedCellsAmount > 0)
	{
		ivec2* collapsed = readBuffer<ivec2>(collapsedCellsBuffer, GL_SHADER_STORAGE_BUFFER, sizeof(ivec2) * collapsedCellsAmount);
		for (int i = 0; i < collapsedCellsAmount; i++)
		{
			collapsedCells.push_back(collapsed[i]);
		}
	}

	// Print time
	uint endTime = getEpochTime();
	if (LOG_ELAPSED_TIMES) { cout << "CurrentIteration : " << currentIteration << "   -   " << "Elapsed time : " << endTime - startTime << " ms \n"; }

	// Reiterate if needed
	if (collapsedCells.size() > collapsedCellIndex)
	{
		//cout << "reiterated " << collapsedCells[collapsedCellIndex].x << "   " << collapsedCells[collapsedCellIndex].y << "\n";
		collapsedCellIndex += 1;
		computeNext(collapsedCells[collapsedCellIndex - 1], 0, true, false);
	}
	
}

void runWFC()
{
	// Repeat until all cells are collapsed
	while (true)
	{
		computeNext();
	
		if (RENDER_DURING_WFC)
		{
			Render();
		}
	
		if (minEntropyCellsAmount == 0 || stop == true)
		{
			stop = false;
			break;
		}
	
		if (SPEED > 0 && 1000 > SPEED)
		{
			chrono::milliseconds duration(1000-SPEED);
			this_thread::sleep_for(duration);
		}
		
			
	}

	uint64_t endTime = getEpochTime();
	//cout << "Whole algorithm - elapsed time : " << endTime - startTime << " ms" << "\n";
}

// Compute cells by columns instead of calculating minimum entropy
void runModelSynthesis()
{
	for (int i = 0; i < COMPUTE_WIDTH; i++)
	{
		for (int j = 0; j < COMPUTE_HEIGHT; j++)
		{
			computeNext(vec2(i, j), 0, true, false);
		}
	}
}

// Input
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	ImGuiIO& io = ImGui::GetIO();
	if (!io.WantCaptureKeyboard)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, true);
		}
		if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
		{
			computeNext();

		}
		if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
		{
			runWFC();

		}
		if (key == GLFW_KEY_R && action == GLFW_PRESS)
		{
			initScreen();
		}
		if (key == GLFW_KEY_T && action == GLFW_PRESS)
		{
			runTests();
		}
	}
	

}

// Print the 2 bit representation of an integer
void print2bit(int number)
{
	vector<bool> bits(TILE_VALUES);

	// Extract the bits from the integer and store them in the vector
	for (int i = 0; i < TILE_VALUES; ++i) {
		bits[i] = (number & (1 << i)) != 0;
	}

	// Print the bits
	for (int i = TILE_VALUES - 1; i >= 0; --i) {
		std::cout << bits[i];
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	ImGuiIO& io = ImGui::GetIO();
	if (action == GLFW_PRESS && !io.WantCaptureMouse)
	{
		vec2 mouseCoords = vec2(mousexpos, mouseypos);
		vec2 coords = convertCoords(mouseCoords, vec2(SCREEN_WIDTH, SCREEN_HEIGHT), vec2(COMPUTE_WIDTH, COMPUTE_HEIGHT));
		
		//cout << dividedCoords.x <<  " , " << dividedCoords.y << "\n";

		if (button == GLFW_MOUSE_BUTTON_LEFT)
		{
			if (isInsideGrid(coords, vec2(COMPUTE_WIDTH, COMPUTE_HEIGHT)))
			{
				textureVector = getTextureVector(gridImage);
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
						GLuint tileNumber = getTileNumberInDividedGrid(mouseCoords);
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
			textureVector = getTextureVector(gridImage);

			int number = textureVector[matrixToVecCoords(coords)];
			print2bit(number);
			std::cout << "\n";

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

	// Send externally loaded textures to gpu
	for (int i = 0; i < loadedTextures.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, loadedTextures[i]);
		glUniform1i(glGetUniformLocation(screenShaderProgram, concatenate("loadedTexture", i)), i);

	}

	glActiveTexture(GL_TEXTURE0 + loadedTextures.size());
	glBindTexture(GL_TEXTURE_2D, testTexture);
	glUniform1i(glGetUniformLocation(screenShaderProgram, "testTexture"), loadedTextures.size());

	// Send uniforms
	glUniform1ui(glGetUniformLocation(screenShaderProgram, "gridThickness"), GRID_THICKNESS);
	glUniform4ui(glGetUniformLocation(screenShaderProgram, "screenParams"), SCREEN_WIDTH, SCREEN_HEIGHT, COMPUTE_WIDTH, COMPUTE_HEIGHT);
	glUniform2ui(glGetUniformLocation(screenShaderProgram, "mouseCoords"), uint(mousexpos), uint(mouseypos));
	glUniform1ui(glGetUniformLocation(screenShaderProgram, "DIVIDE_CELLS"), DIVIDE_CELLS);
	glUniform1ui(glGetUniformLocation(screenShaderProgram, "CELL_DIVISION"), CELL_DIVISION);
	glUniform1ui(glGetUniformLocation(screenShaderProgram, "TILE_VALUES"), TILE_VALUES);
	glUniform1ui(glGetUniformLocation(screenShaderProgram, "COLOR_FROM_TEXTURE"), COLOR_FROM_TEXTURE);
	glUniform1ui(glGetUniformLocation(screenShaderProgram, "SUDOKU"), SUDOKU);
	glUniform4fv(glGetUniformLocation(screenShaderProgram, "colorVector"), MAXIMUM_TILE_VALUES, &colorVector[0][0]);
	glBindVertexArray(VAO);

	// Draw
	glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(indices[0]), GL_UNSIGNED_INT, 0);

	// ImGUI
	bool runwfc = false;
	if (IMGUI)
	{
		ImGui::Begin("ImGUI window");
		ImGui::SetWindowSize(ImVec2(500, 400));
		ImGui::Checkbox("LOG ELAPSED TIMES", &LOG_ELAPSED_TIMES);
		ImGui::Checkbox("DIVIDE CELLS", &DIVIDE_CELLS);
		ImGui::Checkbox("COLOR FROM TEXTURE", &COLOR_FROM_TEXTURE);
		ImGui::Checkbox("SUDOKU", &SUDOKU);
		ImGui::Checkbox("RENDER DURING WFC", &RENDER_DURING_WFC);
		ImGui::SliderInt("GRID THICKNESS", &GRID_THICKNESS, 0, 10);
		ImGui::SliderInt("ALGORITHM SPEED", &SPEED, 1, 1000);
		if (ImGui::SliderInt("COMPUTE WIDTH", &COMPUTE_WIDTH, 1, 1024))
		{
			initOpenGLObjects();
			initScreen();
		}
		if (ImGui::SliderInt("COMPUTE HEIGHT", &COMPUTE_HEIGHT, 1, 1024))
		{
			initOpenGLObjects();
			initScreen();
		}
		if (ImGui::SliderInt("TILE VALUES", &TILE_VALUES, 1, 32))
		{
			CELL_DIVISION = ceil(sqrt(TILE_VALUES));
			initOpenGLObjects();
			initScreen();
		}


		if (ImGui::Button("Run Whole Algorithm (<-)"))
		{
			runwfc = true;

		}
		if (ImGui::Button("Run One Iteration (->)"))
		{
			computeNext();
		}
		if (ImGui::Button("Restart (r)"))
		{
			initScreen();
		}
		if (ImGui::Button("Stop"))
		{
			stop = true;
		}

		if (ImGui::Button("Get rules from image"))
		{
			COLOR_FROM_TEXTURE = false;
			getRulesFromTexture();
			initOpenGLObjects();
			initScreen();
		}

		ImGui::InputText("Input image location", ruleInputTextureLocation, sizeof(ruleInputTextureLocation));


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

void fillRulesBuffer()
{
	int rulesArray[MAXIMUM_TILE_VALUES * MAXIMUM_NEIGHBOURS];
	for (int i = 0; i < TILE_VALUES; i++)
	{
		for (int j = 0; j < NEIGHBOURS_AMOUNT; j++)
		{

			rulesArray[(i * NEIGHBOURS_AMOUNT) + j] = allRules[i][j];
		}
	}
	glGenBuffers(1, &rulesBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, rulesBuffer);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(int) * MAXIMUM_TILE_VALUES * MAXIMUM_NEIGHBOURS, rulesArray, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, rulesBuffer);
}

void initOpenGLObjects()
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

	glGenBuffers(1, &collapsedCellsBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, collapsedCellsBuffer);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(ivec2) * COMPUTE_WIDTH * COMPUTE_HEIGHT, nullptr, GL_MAP_READ_BIT);

	glGenBuffers(1, &collapsedCellsAmountBuffer);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, collapsedCellsAmountBuffer);
	glBufferStorage(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), nullptr, GL_MAP_READ_BIT);

	// Fill rules buffer
	static bool firstCall = true;
	if (firstCall)
	{
		fillRulesBuffer();
	
		firstCall = false;
	}

	// Textures
	initTexture(&gridImage, 1, GL_READ_WRITE, GL_R32UI);
	initTexture(&entropyImage, 2, GL_READ_WRITE, GL_R32UI);

	for (int i = 0; i < textureLocations.size(); i++)
	{
		loadedTextures.push_back(0);
		loadTextureFromFile(&loadedTextures[i], textureLocations[i]);
	}


	// Shaders, programs
	initShaderProgram({ GL_COMPUTE_SHADER }, { "useRulesShader.comp" }, { &useRulesShader }, &useRulesProgram);
	initShaderProgram({ GL_COMPUTE_SHADER }, { "getMinEntropyShader.comp" }, { &getMinEntropyShader }, &getMinEntropyProgram);
	initShaderProgram({ GL_COMPUTE_SHADER }, { "getMinEntropyCellsShader.comp" }, { &getMinEntropyCellsShader }, &getMinEntropyCellsProgram);
	initShaderProgram({ GL_VERTEX_SHADER , GL_FRAGMENT_SHADER }, { "vertexShader.vert","fragmentShader.frag" }, { &screenVertexShader,&screenFragmentShader }, &screenShaderProgram);
}

template <typename T>
bool vectorContains(vector<T> v, T element)
{
	for (int i = 0; i < v.size(); i++)
	{
		if (v[i] == element)
		{
			return true;
		}
	}
	return false;
}

// Find the index of an element in a vector
template <typename T>
int findElementIndexInVector(vector<T> v, T element)
{
	for (int i = 0; i < v.size(); i++)
	{
		if (v[i] == element)
		{
			return i;
		}
	}
	return -1;
}

void getRulesFromTexture()
{
	const uint KERNEL_WIDTH = 2;
	const uint KERNEL_HEIGH = 2;

	// 1. Load Texture
	vec3 params = loadTextureFromFile(&inputTexture, ruleInputTextureLocation);

	// 2. Convert it into matrix
	int width = params[0];
	int height = params[1];
	vector <GLubyte> v(width * height * 4);
	glBindTexture(GL_TEXTURE_2D, inputTexture);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &v[0]);
	glBindTexture(GL_TEXTURE_2D, 0);

	vector<vector<vec4>> image;

	for (int i = 0; i < width; ++i)
	{
		vector<vec4> column;
		for (int j = 0; j < height; ++j)
		{
			int index = (j * width + i) * 4;
			vec4 pixel = vec4(float(v[index]) / 255.0f, float(v[index + 1]) / 255.0f, float(v[index + 2]) / 255.0f, float(v[index + 3]) / 255.0f);
			column.push_back(pixel);
		}
		image.push_back(column);
	}

	// 3. Get all different pixels
	vector<vec4> tiles = {};
	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < height; j++)
		{
			vec4 pixel = image[i][j];
			if (vectorContains<vec4>(tiles, pixel) == false) //tiles doesnt contain pixel
			{
				tiles.push_back(pixel);
			}
		}
	}

	TILE_VALUES = tiles.size();

	// 4. Get rules from the image
	int rules[MAXIMUM_TILE_VALUES][MAXIMUM_NEIGHBOURS] = { {0} };

	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < height; j++)
		{
			// Read pixel
			vec4 pixel = image[i][j];
			uint tileNumber = findElementIndexInVector(tiles, pixel);
			uint tileValue = pow(2, tileNumber);

			for (int k = 0; k < neighbours.size(); k++)
			{
				ivec2 neighbourCoords = ivec2(i, j) + neighbours[k];
				if (isInsideGrid(neighbourCoords, ivec2(width, height)))
				{
					vec4 neighbourPixel = image[neighbourCoords.x][neighbourCoords.y];
					uint neighbourTileNumber = findElementIndexInVector(tiles, neighbourPixel);
					uint neighbourTileValue = pow(2, neighbourTileNumber);

					rules[tileNumber][k] = rules[tileNumber][k] | neighbourTileValue;
				}
			}
		}
	}

	// 5. Send rules to ssbo
	int rulesArray[MAXIMUM_TILE_VALUES * MAXIMUM_NEIGHBOURS];
	for (int i = 0; i < TILE_VALUES; i++)
	{
		for (int j = 0; j < NEIGHBOURS_AMOUNT; j++)
		{

			rulesArray[(i * NEIGHBOURS_AMOUNT) + j] = rules[i][j];
		}
	}
	glGenBuffers(1, &rulesBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, rulesBuffer);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(int) * MAXIMUM_TILE_VALUES * MAXIMUM_NEIGHBOURS, rulesArray, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, rulesBuffer);

	//glBindBuffer(GL_SHADER_STORAGE_BUFFER, rulesBuffer);
	//glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_INT, rules);
	//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, rulesBuffer);

	// 6. Set read colors into colorsVector
	for (int i = 0; i < TILE_VALUES; i++)
	{
		colorVector[i] = tiles[i];
	}


	// 7. Print extracted colors, rules
	cout << "Extracted colors" << "\n";
	for (int i = 0; i < tiles.size(); i++)
	{
		cout << "Tile " << i + 1 << " : ";
		cout << tiles[i].x << "   " << tiles[i].y << "   " << tiles[i].z << "   " << "\n";
	}
	cout << "\n";

	cout << "Extracted rules" << "\n";
	cout << "Neighbours : ";
	for (int i = 0; i < NEIGHBOURS_AMOUNT; i++)
	{
		cout << "(" << neighbours[i][0] << ", " << neighbours[i][1] << ") ";
	}
	cout << "\n";

	for (int i = 0; i < TILE_VALUES; i++)
	{
		cout << "Tile " << i + 1 << " : ";
		for (int j = 0; j < neighbours.size(); j++)
		{
			print2bit(rules[i][j]);
			cout << " ";
		}
		cout << "\n";
	}
	cout << "\n";
}

void runTest(int width, int height, int amount)
{
	COMPUTE_WIDTH = width;
	COMPUTE_HEIGHT = height;
	initOpenGLObjects();
	initScreen();
	//runWFC();

	ofstream f("tests.csv", std::ios_base::app);
	uint64_t time = 0;
	uint64_t sumTime = 0;
	uint64_t maxTime = 0;
	float deviation = 0;
	vector<uint64_t> times = {};

	for (int i = 0; i < amount; i++)
	{
		
		initScreen();

		uint64_t startTime = getEpochTime();
		runWFC();
		time = getEpochTime() - startTime;
		
		times.push_back(time);
		sumTime += time;

		//f << time << ",";
	}
	//f << "\n";
	int N = times.size();
	float avg = float(sumTime) / float(N);
	f << avg << ",";
	f.close();

	uint64_t minTime = times[0];

	for (int i = 0; i < N; i++)
	{
		if (times[i] > maxTime)
		{
			maxTime = times[i];
		}
		if (minTime > times[i])
		{
			minTime = times[i];
		}

		deviation = deviation + (avg - float(times[i])) * (avg - float(times[i]));
	}
	deviation = deviation / float(N);
	deviation = sqrt(deviation);

	printVector(times);
	cout << "Avg: " << avg << " Max: " << maxTime << " Min: " << minTime << " Deviation: " << deviation << "\n";
	

}

void runTests()
{
	RENDER_DURING_WFC = false;
	LOG_ELAPSED_TIMES = false;
	IMGUI = false;
	SUDOKU = false;
	vSync = false;

	for (int i = 0; i <= 10; i++)
	{
		int n = pow(2, i);
		cout << n << "x" << n << "\n";
		runTest(n, n, 1);
	}
	
	ofstream g("tests.csv", std::ios_base::app);
	g << "\n";
	g.close();
	
}

// Grid size test
void test1()
{
	for (int i = 0; i <= 10; i++)
	{
		int n = pow(2, i);
		cout << n << "x" << n << "\n";
		runTest(n, n, 100);
	}
}

// Tile Values amount test
void test2()
{
	for (size_t i = 2; i < 32; i++)
	{
		TILE_VALUES = i;
		for (int k = 0; k < TILE_VALUES; k++)
		{
			for (int j = 0; j < 4; j++)
			{
				allRules[k][j] = pow(2, TILE_VALUES) - 1 - pow(2, k);
			}

		}
		fillRulesBuffer();

		runTest(16, 16, 100);
	}
}

// Neighbours amount test
void test3()
{
	neighbours = {};

	TILE_VALUES = 31;
	for (size_t i = 1; i <= 15; i++)
	{
		neighbours.push_back(ivec2(i, 0));
		neighbours.push_back(ivec2(-1 * i, 0));
		NEIGHBOURS_AMOUNT = neighbours.size();

		for (int k = 0; k < TILE_VALUES; k++)
		{
			for (int j = 0; j < i * 2; j++)
			{
				allRules[k][j] = pow(2, TILE_VALUES) - 1 - pow(2, k);
			}

		}
		fillRulesBuffer();

		runTest(64, 1, 100);
	}
}
