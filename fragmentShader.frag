#version 460 core
out vec4 FragColor;
uniform sampler2D screen;
in vec2 UVs;

uniform uint gridThickness;
uniform uvec4 screenParams;

void main()
{

	uint SCREEN_WIDTH = screenParams[0];
    uint SCREEN_HEIGHT = screenParams[1];
    uint COMPUTE_WIDTH = screenParams[2];
    uint COMPUTE_HEIGHT = screenParams[3];
    int gap;

    FragColor = texture(screen, UVs);

    // Vertical Grid
    gap = int(floor(SCREEN_WIDTH/COMPUTE_WIDTH));
    for (int i = 0; i < COMPUTE_WIDTH; i++)
    {
       if (gl_FragCoord.x > gap * i - gridThickness && gl_FragCoord.x < gap * i + gridThickness) 
       {
       FragColor = vec4(0, 0, 0, 1.0);
       } 
    }

    // Horizontal Grid
    gap = int(floor(SCREEN_HEIGHT/COMPUTE_HEIGHT));
    for (int i = 0; i < COMPUTE_HEIGHT; i++)
    {
       if (gl_FragCoord.y > gap * i - gridThickness && gl_FragCoord.y < gap * i + gridThickness) 
       {
       FragColor = vec4(0, 0, 0, 1.0);
       } 
    }

}