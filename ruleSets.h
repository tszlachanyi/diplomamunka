#include <iostream>
#include <chrono>
#include <algorithm>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <bitset>		
#include <array>
#include <cmath>
#include <thread>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_glfw.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

using namespace std;
using namespace std::chrono;
using namespace glm;


// Neighbour cant be the same
vector <ivec2> neighbours = { ivec2(0,1), ivec2(1, 0), ivec2(0, -1), ivec2(-1,0) };
int TILE_VALUES = 4;
int allRules[4][4] =
{
   { 0b1110 , 0b1110 , 0b1110, 0b1110},
   { 0b1101 , 0b1101 , 0b1101, 0b1101},
   { 0b1011 , 0b1011 , 0b1011, 0b1011},
   { 0b0111 , 0b0111 , 0b0111, 0b0111},
};

// Everything allowed
vector <ivec2> neighbours = { ivec2(0,1), ivec2(1, 0), ivec2(0, -1), ivec2(-1,0) };
int TILE_VALUES = 4;
int allRules[4][4] =
{
   { 0b1111 , 0b1111 , 0b1111, 0b1111},
   { 0b1111 , 0b1111 , 0b1111, 0b1111},
   { 0b1111 , 0b1111 , 0b1111, 0b1111},
   { 0b1111 , 0b1111 , 0b1111, 0b1111},
};

// Thesis example 7
vector <ivec2> neighbours = { ivec2(0,-1), ivec2(0, 1), ivec2(1, 1), ivec2(1,-1)};
int TILE_VALUES = 2;
int allRules[2][4] =
{
   { 0b01 , 0b01 , 0b01, 0b01},
   { 0b11 , 0b11 , 0b11, 0b11}
};