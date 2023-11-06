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
		cout << "Failed to initialize OpenGL context" << endl;
	}
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);


	// VAO, VBO, EBO
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
	initTexture(&computeTex1, 1, GL_READ_WRITE, GL_R32UI);
	initTexture(&computeTex2, 2, GL_READ_WRITE, GL_R32UI);
	initTexture(&entropyTex, 3, GL_WRITE_ONLY, GL_R32UI);

	// Shaders, programs
	initShaderProgram({GL_COMPUTE_SHADER}, {"computeShader.comp"}, {&computeShader}, &computeProgram);
	initShaderProgram({GL_COMPUTE_SHADER}, {"computeEntropyShader.comp"}, {&computeEntropyShader}, &computeEntropyProgram);
	initShaderProgram({GL_VERTEX_SHADER , GL_FRAGMENT_SHADER}, {"vertexShader.vert","fragmentShader.frag"}, {&screenVertexShader,&screenFragmentShader }, &screenShaderProgram);

	// Load initial position
	initScreen();

	// Main Loop
	while (!glfwWindowShouldClose(window))
	{
		glfwGetCursorPos(window, &mousexpos, &mouseypos);
		mouseypos = SCREEN_WIDTH - mouseypos;
		Render();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
}

