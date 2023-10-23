#version 460 core
out vec4 FragColor;
uniform sampler2D screen;
in vec2 UVs;
void main()
{

	int SCREEN_WIDTH = 1024;
    int SCREEN_HEIGHT = 1024;
    int COMPUTE_WIDTH = 16;
    int COMPUTE_HEIGHT = 16;
    int thickness = 2;
    int gap;

    FragColor = texture(screen, UVs);

    // Vertical Grid
    gap = int(floor(SCREEN_WIDTH/COMPUTE_WIDTH));
    for (int i = 0; i < COMPUTE_WIDTH; i++)
    {
       if (gl_FragCoord.x > gap * i - thickness && gl_FragCoord.x < gap * i + thickness) 
       {
       FragColor = vec4(0, 0, 0, 1.0);
       } 
    }

    // Horizontal Grid
    gap = int(floor(SCREEN_HEIGHT/COMPUTE_HEIGHT));
    for (int i = 0; i < COMPUTE_HEIGHT; i++)
    {
       if (gl_FragCoord.y > gap * i - thickness && gl_FragCoord.y < gap * i + thickness) 
       {
       FragColor = vec4(0, 0, 0, 1.0);
       } 
    }

}