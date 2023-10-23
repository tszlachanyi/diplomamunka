#include "functions.h"

int main()
{
	// Initialization
	srand(time(NULL));

	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OPENGL_MAJOR_VERSION);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OPENGL_MINOR_VERSION);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "OpenGL Compute Shaders", NULL, NULL);
	if (!window)
	{
		std::cout << "Failed to create the GLFW window\n";
		glfwTerminate();
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(vSync);
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize OpenGL context" << std::endl;
	}
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

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

	// Textures
	initTexture(&screenTex, 0, GL_WRITE_ONLY, GL_RGBA32F);
	initTexture(&loadTex, 1, GL_READ_WRITE, GL_R32UI);
	initTexture(&computedTex, 2, GL_READ_WRITE, GL_R32UI);
	initTexture(&screenTexDivided, 3, GL_WRITE_ONLY, GL_RGBA32F, vec2(COMPUTE_WIDTH * cellDivision, COMPUTE_HEIGHT * cellDivision));

	// Shaders
	screenVertexShader = loadShader(GL_VERTEX_SHADER, "vertexShader.vert");
	screenFragmentShader = loadShader(GL_FRAGMENT_SHADER, "fragmentShader.frag");
	computeShader = loadShader(GL_COMPUTE_SHADER, "computeShader.comp");

	screenShaderProgram = glCreateProgram();
	computeProgram = glCreateProgram();

	glAttachShader(screenShaderProgram, screenVertexShader);
	glAttachShader(screenShaderProgram, screenFragmentShader);
	glAttachShader(computeProgram, computeShader);
	glLinkProgram(screenShaderProgram);
	glLinkProgram(computeProgram);
	glDeleteShader(screenVertexShader);
	glDeleteShader(screenFragmentShader);

	// Uniforms for compute shader
	uLocationRules = glGetUniformLocation(computeProgram, "rules");
	uLocationRulesAmount = glGetUniformLocation(computeProgram, "rulesAmount");
	uLocationChosenValue = glGetUniformLocation(computeProgram, "chosenValue");
	uLocationCoordinates = glGetUniformLocation(computeProgram, "coordinates");


	// Load initial position

	initScreen();

	// Main Loop

	while (!glfwWindowShouldClose(window))
	{
		glfwGetCursorPos(window, &mousexpos, &mouseypos);
		Render();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
}