#version 460 core

out vec4 FragColor;
in vec2 UVs;

layout (binding = 1, r32ui) readonly uniform uimage2D inputImage;

uniform sampler2D loadedTexture1;
uniform sampler2D loadedTexture2;
uniform sampler2D loadedTexture3;
uniform sampler2D loadedTexture4;
uniform sampler2D loadedTexture5;
uniform sampler2D loadedTexture6;
uniform sampler2D loadedTexture7;
uniform sampler2D loadedTexture8;
uniform sampler2D loadedTexture9;

uniform uint gridThickness;
uniform uvec4 screenParams;
uniform uvec2 mouseCoords;
uniform bool DIVIDE_CELLS;
uniform uint CELL_DIVISION;
uniform uint TILE_VALUES;
uniform bool COLOR_FROM_TEXTURE;
uniform bool SUDOKU;

uint SCREEN_WIDTH = screenParams[0];
uint SCREEN_HEIGHT = screenParams[1];
uint COMPUTE_WIDTH = screenParams[2];
uint COMPUTE_HEIGHT = screenParams[3];

float xgap;
float ygap;
ivec2 cellCoords;	// Coordinates of the current cell in the grid
ivec2 dividedCellCoords;	// Coordinates of the current cell in the divided grid
vec2 textureCoords;		// Coordinates of the pixel relative to the current cell (determines where to sample texture)
vec2 dividedTextureCoords;		// Coordinates of the pixel relative to the current cell in the divided grid (determines where to sample texture)
uint cellValue;		// Tile value of the current cell
uint entr;		// Entropy value of the current cell

vec4 colorVector[9] = {vec4(1, 0, 0, 1), vec4(0, 1, 0, 1), vec4(0, 0, 1, 1), vec4(1, 1, 0, 1), vec4(0, 1, 1, 1), vec4(1, 0, 1, 1), vec4(1, 0.5, 0, 1), vec4(0, 1, 0.5, 1), vec4(0.5, 0, 1, 1)};

// Gets the color of the cell based on it's GLuint value
vec4 getTileColor(uint number)
{
	vec4 color;

	
	if (number == 0)
	{
		color = vec4(0, 0, 0, 1);
	}
	else
	{
		float l = log2(number);
		if (pow(2,l) == float(number))
		{
			color = colorVector[int(l)];
		}
		else
		{
			color = vec4(1, 1, 1, 1);
		}
	}

	return color;
}

// Gets the color for a pixel from the texture corresponding to it's tile value
vec4 getTextureColor(uint number, vec2 textureCoords)
{	
	vec4 color;

	switch (number) {
	case 1:
		color = texture(loadedTexture1, textureCoords);
		break;
	case 2:
		color = texture(loadedTexture2, textureCoords);
		break;
	case 4:
		color = texture(loadedTexture3, textureCoords);
		break;
	case 8:
		color = texture(loadedTexture4, textureCoords);
		break;
	case 16:
		color = texture(loadedTexture5, textureCoords);
		break;
	case 32:
		color = texture(loadedTexture6, textureCoords);
		break;
	case 64:
		color = texture(loadedTexture7, textureCoords);
		break;
	case 128:
		color = texture(loadedTexture8, textureCoords);
		break;
	case 256:
		color = texture(loadedTexture9, textureCoords);
		break;
	default:
		color = getTileColor(number);
	}

	return color;
}

vec4 getPixelColor(uint number, vec2 textureCoords)
{
	if(!COLOR_FROM_TEXTURE)
	{
		return getTileColor(number);
	}
	else
	{
		return getTextureColor(number, textureCoords);
	}
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
    cellCoords = ivec2(vec2(COMPUTE_WIDTH, COMPUTE_HEIGHT) * UVs);
	dividedCellCoords = ivec2(vec2(COMPUTE_WIDTH * CELL_DIVISION, COMPUTE_HEIGHT * CELL_DIVISION) * UVs);
	textureCoords = vec2(COMPUTE_WIDTH, COMPUTE_HEIGHT) * UVs - vec2(cellCoords);
	dividedTextureCoords = vec2(COMPUTE_WIDTH * CELL_DIVISION, COMPUTE_HEIGHT * CELL_DIVISION) * UVs - vec2(dividedCellCoords);
	cellValue = imageLoad(inputImage, cellCoords)[0];
	entr = entropy(cellValue);

	if (!DIVIDE_CELLS || entr == 1 || entr == 0)
	{
		// Cell color
		
		FragColor = getPixelColor(cellValue, textureCoords);

		// Hover effect
		if (convertCoords(mouseCoords, vec2(SCREEN_WIDTH, SCREEN_HEIGHT),vec2(COMPUTE_WIDTH, COMPUTE_HEIGHT)) == cellCoords)
		{
			FragColor = FragColor * 0.5 + 0.25;
		}
	}
	else
	{
		// Cell color
		ivec2 subCellCoords = dividedCellCoords - cellCoords * ivec2(CELL_DIVISION);
		uint subCellValue = subCellCoords.x + subCellCoords.y * CELL_DIVISION;
		
		if (tilePossible(cellValue, subCellValue))
		{
			subCellValue = uint(pow(2, subCellValue));
			FragColor = getPixelColor(subCellValue, dividedTextureCoords) * vec4(0.75);
		}
		else
		{
			FragColor = getPixelColor(0, textureCoords) ;
		}
		

		// Hover effect
		if (convertCoords(mouseCoords, vec2(SCREEN_WIDTH, SCREEN_HEIGHT),vec2(COMPUTE_WIDTH * CELL_DIVISION, COMPUTE_HEIGHT * CELL_DIVISION)) == dividedCellCoords)
		{
			FragColor = FragColor * 0.5 + 0.25;
		}
	}


    // Grid
    xgap = float(SCREEN_WIDTH) / float(COMPUTE_WIDTH);
    ygap = float(SCREEN_HEIGHT) / float(COMPUTE_HEIGHT);

	if (gridThickness > mod(gl_FragCoord.x, xgap) || gridThickness > abs(mod(gl_FragCoord.x, xgap) - xgap) || gridThickness > mod(gl_FragCoord.y, ygap) || gridThickness > abs(mod(gl_FragCoord.y, ygap) - ygap))
    {
        FragColor = vec4(0, 0, 0, 1.0);
    }

	// Sudoku Grid
	if (SUDOKU)
	{
		xgap = float(SCREEN_WIDTH) / float(COMPUTE_WIDTH) * 3;
		ygap = float(SCREEN_HEIGHT) / float(COMPUTE_HEIGHT) * 3;
	
		if (gridThickness * 3 > mod(gl_FragCoord.x, xgap) || gridThickness * 3 > mod(gl_FragCoord.y, ygap) )
		{
		    FragColor = vec4(0, 0, 0, 1.0);
		}
	}
    

}