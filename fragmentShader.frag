#version 460 core
out vec4 FragColor;
uniform sampler2D screen;
in vec2 UVs;
void main()
{
	FragColor = texture(screen, UVs);

	// if (UVs.x > 0.5)
	// {
	// 	FragColor = vec4(0.5,0.5,0.5,1);
	// }
}