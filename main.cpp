#include "header.h"

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

	// Screen Texture
	
	glCreateTextures(GL_TEXTURE_2D, 1, &screenTex);
	glTextureParameteri(screenTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(screenTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(screenTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(screenTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(screenTex, 1, GL_RGBA32F, COMPUTE_WIDTH, COMPUTE_HEIGHT);
	glBindImageTexture(0, screenTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	// Load Texture
	
	glCreateTextures(GL_TEXTURE_2D, 1, &loadTex);
	glTextureParameteri(loadTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(loadTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(loadTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(loadTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(loadTex, 1, GL_RGBA32F, COMPUTE_WIDTH, COMPUTE_HEIGHT);
	glBindImageTexture(1, loadTex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	// Computed Texture

	glCreateTextures(GL_TEXTURE_2D, 1, &computedTex);
	glTextureParameteri(computedTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(computedTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(computedTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(computedTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(computedTex, 1, GL_RGBA32F, COMPUTE_WIDTH, COMPUTE_HEIGHT);
	glBindImageTexture(2, computedTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	// Shaders

	GLuint screenVertexShader = loadShader(GL_VERTEX_SHADER, "vertexShader.vert");
	GLuint screenFragmentShader = loadShader(GL_FRAGMENT_SHADER, "fragmentShader.frag");
	GLuint computeShader = loadShader(GL_COMPUTE_SHADER, "computeShader.comp");

	GLuint screenShaderProgram = glCreateProgram();
	glAttachShader(screenShaderProgram, screenVertexShader);
	glAttachShader(screenShaderProgram, screenFragmentShader);
	glLinkProgram(screenShaderProgram);

	glDeleteShader(screenVertexShader);
	glDeleteShader(screenFragmentShader);

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