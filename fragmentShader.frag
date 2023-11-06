#version 460 core

out vec4 FragColor;
in vec2 UVs;

layout (binding = 1, r32ui) readonly uniform uimage2D inputImage;

uniform uint gridThickness;
uniform uvec4 screenParams;
uniform uvec2 mouseCoords;
uniform bool DIVIDE_CELLS;
uniform uint CELL_DIVISION;
uniform uint TILE_VALUES;

uint SCREEN_WIDTH = screenParams[0];
uint SCREEN_HEIGHT = screenParams[1];
uint COMPUTE_WIDTH = screenParams[2];
uint COMPUTE_HEIGHT = screenParams[3];

// Gets the color of the cell based on it's GLuint value
vec4 getTileColor(uint number)
{
	vec4 color;

	switch (number) {
	case 1:
		color = vec4(1, 0, 0, 1);
		break;
	case 2:
		color = vec4(0, 1, 0, 1);
		break;
	case 4:
		color = vec4(0, 0, 1, 1);
		break;
	case 8:
		color = vec4(1, 1, 0, 1);
		break;
	case 0:
		color = vec4(0, 0, 0, 1);
		break;
	default:
		color = vec4(1, 1, 1, 1);
	}

	return color;
}

uint nthBit(uint number, uint n)
{
	return (number >> n) & 1;
}

bool tilePossible(uint number, uint i)
{
	return (nthBit(number, i) == 1);
}

uint entropy(uint number)
{
	uint entropy = 0;
	for (int i = 0; i < TILE_VALUES; i++)
	{
		if (tilePossible(number, i))
		{
			entropy ++;
		}
	}

	return entropy;
}

vec2 convertCoords(vec2 coords, vec2 oldSize, vec2 newSize)
{
	float xRatio = float(oldSize.x) / float(newSize.x);
	float yRatio = float(oldSize.y) / float(newSize.y);

	int x = int(floor(coords.x / xRatio));
	int y = int(floor(coords.y / yRatio));

	return vec2(x, y);
}

void main()
{
    
    float xgap;
    float ygap;
    ivec2 pixelCoords = ivec2(vec2(COMPUTE_WIDTH, COMPUTE_HEIGHT) * UVs);
	ivec2 dividedPixelCoords = ivec2(vec2(COMPUTE_WIDTH * CELL_DIVISION, COMPUTE_HEIGHT * CELL_DIVISION) * UVs);
	uint cellValue = imageLoad(inputImage, pixelCoords)[0];
	uint entr = entropy(cellValue);

	if (!DIVIDE_CELLS || entr == 1 || entr == 0)
	{
		// Cell color
		
		FragColor = getTileColor(cellValue);

		// Hover effect
		if (convertCoords(mouseCoords, vec2(SCREEN_WIDTH, SCREEN_HEIGHT),vec2(COMPUTE_WIDTH, COMPUTE_HEIGHT)) == pixelCoords)
		{
			FragColor = FragColor * 0.5 + 0.25;
		}
	}
	else
	{
		// Cell color
		ivec2 subCellCoords = dividedPixelCoords - pixelCoords * ivec2(CELL_DIVISION);
		uint subCellValue = subCellCoords.x + subCellCoords.y * CELL_DIVISION;
		
		if (tilePossible(cellValue, subCellValue))
		{
			subCellValue = uint(pow(2, subCellValue));
			FragColor = getTileColor(subCellValue) * vec4(0.75);
		}
		else
		{
			FragColor = getTileColor(0) ;
		}
		

		// Hover effect
		if (convertCoords(mouseCoords, vec2(SCREEN_WIDTH, SCREEN_HEIGHT),vec2(COMPUTE_WIDTH * CELL_DIVISION, COMPUTE_HEIGHT * CELL_DIVISION)) == dividedPixelCoords)
		{
			FragColor = FragColor * 0.5 + 0.25;
		}
	}


    // Grid
    xgap = float(SCREEN_WIDTH) / float(COMPUTE_WIDTH);
    ygap = float(SCREEN_HEIGHT) / float(COMPUTE_HEIGHT);

	if (gridThickness > mod(gl_FragCoord.x, xgap) || gridThickness > mod(gl_FragCoord.y, ygap) )
    {
        FragColor = vec4(0, 0, 0, 1.0);
    }

	

}