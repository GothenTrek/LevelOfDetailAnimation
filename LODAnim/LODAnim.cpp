// Author:  George Othen
// Date: 10/02/2017
// Title: LOD Anim Class

// Std. Includes
#include <iostream>
#include <Windows.h>
#include <ctime>

// GL Includes
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/GLU.h>

// freetype Includes
#include <ft2build.h>
#include FT_FREETYPE_H

// custom Includes
#include "Model.h"
#include "Shader.h"
#include "stb_image.h"
#include "Camera.h"

// Height, Width and FOV constraints
const GLuint WIDTH = 1920, HEIGHT = 1080, FOV = 50;

// Character struct for Text Rendering
struct Character {
	GLuint TextureID;   // ID handle of the glyph texture
	glm::ivec2 Size;    // Size of glyph
	glm::ivec2 Bearing;  // Offset from baseline to left/top of glyph
	GLuint Advance;    // Horizontal offset to advance to next glyph
};
std::map<GLchar, Character> Characters;
GLuint VAO, VBO;

// Function prototype
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void RenderText(Shader &shader, std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color);
void key_triggered();
bool keys[1024];

// Define Position of Light Source
glm::vec3 lightPos(0.0f, 0.0f, 0.0f);

// Define Initial Camera Position
glm::vec3 cameraPosition(0.0f, 32.0f, 11.0f);
glm::vec3 cameraTarget(0.0f, 0.0f, 0.0f);
glm::vec3 LODPosition = cameraPosition;

// Camera Initial Transformation
glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 32.0f, 11.0f), // position
	glm::vec3(0.0f, 0.0f, 0.0f), // point to look at
	glm::vec3(0.0f, 1.0f, 47.0f)); // up direction

// Model Mode: LOD, LOD0, Wireframe LOD, Wireframe LOD0, Exaggerated LOD, LOD0 -> LOD4
int mode = 0, camPos = 0;

// Time
float currentTime = 0;

// Polygon Count
int polyCount[] = { 0, 0 };

// Toggle Wireframe
bool wireframe = false;

// Set Camera Transformation
void setCamera() {
	view = glm::lookAt(cameraPosition, // position
		glm::vec3(cameraTarget), // point to look at
		glm::vec3(0.0f, 1.0f, 37.0f)); // up direction
}

// Move Camera to Position 1
void cameraMovePos1() {
	cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
	cameraPosition = glm::vec3(0.0f, 32.0f, 11.0f);	
	LODPosition = cameraPosition;
	setCamera();
}

// Move Camera to Position 2
void cameraMovePos2() {	
	cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
	cameraPosition = glm::vec3(0.0f, -0.01f, 30.0f);
	LODPosition = cameraPosition;
	setCamera();
}

// Move Camera to Position 3
void cameraMovePos3() {	
	cameraTarget = glm::vec3(0.0f, -15.0f, 0.0f);
	cameraPosition = glm::vec3(0.0f, -3.1f, 0.0f);
	LODPosition = cameraPosition;
	setCamera();
}

// Calculate the Euclidean Distance between 2 Positions
float EuclideanDistance(glm::vec3 modelLoc, glm::vec3 referenceLoc) {
	float sum = 0;
	for (int i = 0; i < 3; i++) {
		sum += pow((referenceLoc[i] - modelLoc[i]), 2);
	}
	return sqrt(sum);
}

// Get Mode Type
string getMode() {
	switch (mode) {
	case 0:
		return "(Left | Right Arrows) Normal LOD Mode";
	case 1:
		return "(Left | Right Arrows) Exaggerated LOD Mode";
	case 2:
		return "(Left | Right Arrows) LOD All Level L0";
	case 3:
		return "(Left | Right Arrows) LOD All Level L1";
	case 4:
		return "(Left | Right Arrows) LOD All Level L2";
	case 5:
		return "(Left | Right Arrows) LOD All Level L3";
	case 6:
		return "(Left | Right Arrows) LOD All Level L4";
	default:
		return "(Left | Right Arrows) Mode out of range";			
	}
}

// Check the LOD
int CheckLevel(glm::vec3 objectT, float Distances[]) {
	float distance = EuclideanDistance(objectT, LODPosition);

	int level;
	if (distance < Distances[0]) {
		level = 0;
	}
	else if (distance < Distances[1]) {
		level = 1;
	}
	else if (distance < Distances[2]) {
		level = 2;
	}
	else if (distance < Distances[3]) {
		level = 3;
	}
	else if(distance < INFINITE){
		level = 4;
	}
	return level;
}

// Get polygon count
void polygonCount(int level) {
	if (polyCount[1] == 5) {
		polyCount[0] = 0;
		polyCount[1] = 0;
	}
	// Check levels, state polygon count based on current meshes
	switch (level) {
	case 0:
		polyCount[0] += 16128;
		break;
	case 1:
		polyCount[0] += 3968;
		break;
	case 2:
		polyCount[0] += 960;
		break;
	case 3:
		polyCount[0] += 224;
		break;
	case 4:
		polyCount[0] += 48;
	}
	polyCount[1] += 1;
}

// Draw Models Orbiting & Path
void Orbit(vector<Model> & planets, Model ring, Shader shaderProgram, float orbitRadius, float orbitSpeed, float rotationSpeed, vector<glm::vec3> Colour, glm::vec3 rotationVector) {	
	// LOD Distances
	float ExaggeratedDistances[] = { 30.0f, 37.0f, 44.0f, 51.0f };
	float Distances[] = { 35.0f, 55.0f, 65.0f, 75.0f };

	// LOD level
	int level;

	// Set Orbit Translation Vector
	glm::vec3 objectT = { (sin((currentTime + 20.0f) / orbitSpeed) * orbitRadius), (cos((currentTime + 20.0f) / orbitSpeed) * orbitRadius), 0.0f };

	// Check Model Detail Level base on Mode
	switch (mode) {
		case 0:
			level = CheckLevel(objectT, Distances);
			break;
		case 1:
			level = CheckLevel(objectT, ExaggeratedDistances);
			break;
		case 2:
			level = 0;
			break;
		case 3:
			level = 1;
			break;
		case 4:
			level = 2;
			break;
		case 5:
			level = 3;
			break;
		case 6:
			level = 4;
			break;
		default:
			level = 4;
	}
	polygonCount(level);

	// Apply Transformation to Current Model
	planets[level].transformR(shaderProgram, objectT, rotationVector, rotationSpeed, 1);

	// Change Colour of Model
	planets[level].changeColour(shaderProgram, Colour[level]);

	// Draw Model
	planets[level].Draw(shaderProgram);

	// Apply Transformations to Orbit Path Model
	ring.rotateS(shaderProgram, glm::vec3(1.0f, 0.0f, 0.0f), glm::radians(90.0f), glm::vec3(orbitRadius / 2.87f, 1.0f, orbitRadius / 2.87f));

	// Change Colour of Orbit Path Model
	ring.changeColour(shaderProgram, glm::vec3(0.0f, 0.0f, 1.0f));

	// Draw Orbit Path Model
	ring.Draw(shaderProgram);
}

// Setup Lighting Shader
void setupLightSource(Shader shaderProgram) {
	shaderProgram.Use(); // Activate Shader Program

	// Get Uniform Locations from shaders
	GLint objectColorLoc = glGetUniformLocation(shaderProgram.Program, "myColour");
	GLint lightColorLoc = glGetUniformLocation(shaderProgram.Program, "lightColour");
	GLint lightPosLoc = glGetUniformLocation(shaderProgram.Program, "lightPos");
	GLint viewPosLoc = glGetUniformLocation(shaderProgram.Program, "viewPos");

	// Define Shader Attributes
	glUniform3f(objectColorLoc, 1.0f, 0.9f, 0.8f);
	glUniform3f(lightColorLoc, 1.0f, 0.9f, 0.8f);
	glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);
	glUniform3f(viewPosLoc, 0.0f, 0.0f, 0.0f);	
}

// Draw Light Source
void drawSun(Shader& shaderProgram, Model& sunModel, float modelSize) {
	shaderProgram.Use(); // Activate Shader Program

	glm::mat4 model; // Create Model Transformation Matrix	

	// Translate Model to 'light position'
	model = glm::translate(model, lightPos);

	// Scale model to size
	model = glm::scale(model, glm::vec3(modelSize));

	// Send matrix to shaders
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));

	// Draw Model
	sunModel.Draw(shaderProgram);
}

// Pass Projection & View Matrices to Shader Programs
void passMatrixToShader(Shader shaderProgram, glm::mat4 projection, glm::mat4 view) {
	// Send view & projection location to shader
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

int main()
{
/// CLOCK -------------------------------------------------------------------------------------------------
/// -------------------------------------------------------------------------------------------------------
/// -------------------------------------------------------------------------------------------------------
	std::clock_t start;
	double duration;
	start = std::clock();


/// OPENGL ------------------------------------------------------------------------------------------------
/// -------------------------------------------------------------------------------------------------------
/// -------------------------------------------------------------------------------------------------------
	// Init GLFW
	glfwInit();

	// Set all the required options for GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	// Create a GLFWwindow object that we can use for GLFW's functions
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "LOD Animation", glfwGetPrimaryMonitor(), nullptr);
	glfwMakeContextCurrent(window);

	// Set the required callback functions
	glfwSetKeyCallback(window, key_callback);

	// Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
	glewExperimental = GL_TRUE;

	// Initialize GLEW to setup the OpenGL Function pointers
	glewInit();

	// Define the viewport dimensions
	glViewport(0, 0, WIDTH, HEIGHT);

	// Enable MSAA
	//glEnable(GL_MULTISAMPLE);

	// Enable Depth Test / Z Buffer
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// Enable Blending 
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


/// SHADERS -----------------------------------------------------------------------------------------------
/// -------------------------------------------------------------------------------------------------------
/// -------------------------------------------------------------------------------------------------------
	// Compile Shaders
	Shader lightingProgram("../shaders/lighting.vert", "../shaders/lighting.frag");
	Shader lampProgram("../shaders/lamp.vert", "../shaders/lamp.frag");
	Shader textProgram("../shaders/text.vert", "../shaders/text.frag");


/// TEXT --------------------------------------------------------------------------------------------------
/// -------------------------------------------------------------------------------------------------------
/// -------------------------------------------------------------------------------------------------------
	glm::mat4 textProjection = glm::ortho(0.0f, static_cast<GLfloat>(WIDTH), 0.0f, static_cast<GLfloat>(HEIGHT));
	textProgram.Use();
	glUniformMatrix4fv(glGetUniformLocation(textProgram.Program, "projection"), 1, GL_FALSE, glm::value_ptr(textProjection));
	// FreeType
	FT_Library ft;
	// All functions return a value different than 0 whenever an error occurred
	if (FT_Init_FreeType(&ft))
		std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;

	// Load font as face
	FT_Face face;
	if (FT_New_Face(ft, "../fonts/arial.ttf", 0, &face))
		std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;

	// Set size to load glyphs as
	FT_Set_Pixel_Sizes(face, 0, 48);

	// Disable byte-alignment restriction
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Load first 128 characters of ASCII set
	for (GLubyte c = 0; c < 128; c++)
	{
		// Load character glyph 
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
		{
			std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
			continue;
		}
		// Generate texture
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
		);
		// Set texture options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// Now store character for later use
		Character character = {
			texture,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			face->glyph->advance.x
		};
		Characters.insert(std::pair<GLchar, Character>(c, character));
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	// Destroy FreeType once we're finished
	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	// Configure VAO/VBO for texture quads
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


/// MODELS ------------------------------------------------------------------------------------------------
/// -------------------------------------------------------------------------------------------------------
/// -------------------------------------------------------------------------------------------------------
	// Create vector to store: Models, Wire Models, Model Colours, Distances
	vector<Model> Models, Wires; vector<glm::vec3> colours, white; float Distances[5];
	
	// Load Orbit Path Model
	Model circum("../Models/circumference.obj");
	
	// Load Models into vector
	for (int i = 0; i < 5; i++) {
		string filename = "../Models/";
		filename.append(to_string(i));
		filename.append(".obj");
		Model model(filename);
		Models.push_back(model);
	}

	// Load Wire Models into vector
	for (int i = 0; i < 5; i++) {
		string filename = "../Models/";
		filename.append(to_string(i));
		filename.append("w.obj");
		Model model(filename);
		Wires.push_back(model);
	}

	// Load colours into vector
	colours.push_back(glm::vec3(1.0f, 1.0f, 1.0f));
	colours.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
	colours.push_back(glm::vec3(1.0f, 1.0f, 0.0f));
	colours.push_back(glm::vec3(1.0f, 0.3f, 0.0f));
	colours.push_back(glm::vec3(1.0f, 0.0f, 0.0f));

	// Load white colours into vector
	for (int i = 0; i < 5; i++) {
		white.push_back(glm::vec3(1.0f, 1.0f, 1.0f));
	}

	// Define Orbit Attributes
	vector<int> Radius = { 6, 9, 12, 15, 18 };
	vector<float> Speed = { 2.0f, 3.3f, 5.3f, 8.9f, 11.7f };
	vector<float> RotateSpeed = { 150.0f, 100.0f, 75.0f, 55.0f, 37.0f };	

	// Define Rotation Axis
	glm::vec3 rotateZ = glm::vec3(0.0f, 0.0f, 1.0f), rotateY = glm::vec3(0.0f, 1.0f, 0.0f);


/// CAMERA ------------------------------------------------------------------------------------------------
/// -------------------------------------------------------------------------------------------------------
/// -------------------------------------------------------------------------------------------------------
	

	// Camera Movement
	float x = 0, y = 0, z = 0;

	// Projection Perspective Matrix
	glm::mat4 projection = glm::perspective(glm::radians((float)FOV), (float)WIDTH / (float)HEIGHT, 0.1f, 210.0f);

	// Start clock now that animation has loaded
	duration = clock() - start;

/// RENDER LOOP --------------------------------------------------------------------------------------------
/// --------------------------------------------------------------------------------------------------------
/// --------------------------------------------------------------------------------------------------------
	while (!glfwWindowShouldClose(window))
	{
		
		// Check if any events have taken place
		glfwPollEvents();

		// Clear the colorbuffer
		glClearColor(0.25f, 0.25f, 0.35f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// Activate Shader
		setupLightSource(lightingProgram);
		passMatrixToShader(lightingProgram, projection, view);	

		// Clock starts here
		currentTime = (clock() / 1000.0f) - (duration / 1000.0f) - (3);

		// Display Title
		if (currentTime > 0 & currentTime < 5) {
			RenderText(textProgram, "The Level of Detail Algorithm", 310.0f, 840.0f, 2.0f, glm::vec3(1.0f, 0.2f, 0.2f));			
		}

		// Display Orbiting Spheres
		if (currentTime > 0 & currentTime < 15) {
			RenderText(textProgram, getMode(), 5.0f, 5.0f, 0.5f, glm::vec3(1.0f, 0.0f, 0.0f)); // Render text: Mode
			RenderText(textProgram, "(C) Change Camera Position", 5.0f, 30.0f, 0.5f, glm::vec3(1.0f, 0.0f, 0.0f)); // Display Camera Position Instruction
			lightingProgram.Use(); // Switch back to correct shader

			// check if wireframe mode is enabled
			if (wireframe) { 
				// Draw Sphere's
				for (int i = 0; i < 5; i++) {
					Orbit(Wires, circum, lightingProgram, Radius[i], Speed[i], RotateSpeed[i], white, rotateZ);
				}
				RenderText(textProgram, "(W) Wireframe Mode: ON", 5.0f, 55.0f, 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));
			}
			else {
				// Draw Sphere's
				for (int i = 0; i < 5; i++) {
					Orbit(Models, circum, lightingProgram, Radius[i], Speed[i], RotateSpeed[i], white, rotateZ);
				}
				RenderText(textProgram, "(W) Wireframe Mode: OFF", 5.0f, 55.0f, 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));
			}

			// Create Polygon count string
			string polygon = "Polygon Count: ";
			polygon.append(to_string(polyCount[0]));

			// Render polygon count string
			RenderText(textProgram, polygon, 5.0f, 1050.0f, 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));

			// Draw Sun Light Source
			drawSun(lampProgram, Models[3], 3.0f);
			passMatrixToShader(lampProgram, projection, view);
		}
		// Display orbiting models
		if (currentTime > 32) {
			RenderText(textProgram, getMode(), 5.0f, 5.0f, 0.5f, glm::vec3(1.0f, 0.0f, 0.0f)); // Render text: Mode
			RenderText(textProgram, "(C) Change Camera Position", 5.0f, 30.0f, 0.5f, glm::vec3(1.0f, 0.0f, 0.0f)); // Display Camera Position Instruction
			lightingProgram.Use(); // Switch back to correct shader

			// Check if wireframe mode is enabled
			if (wireframe) {
				// Draw Sphere's
				for (int i = 0; i < 5; i++) {
					Orbit(Wires, circum, lightingProgram, Radius[i], Speed[i], RotateSpeed[i], colours, rotateZ);
				}
				RenderText(textProgram, "(W) Wireframe Mode: ON", 5.0f, 55.0f, 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));
			}
			else {
				// Draw Sphere's
				for (int i = 0; i < 5; i++) {
					Orbit(Models, circum, lightingProgram, Radius[i], Speed[i], RotateSpeed[i], colours, rotateZ);
				}
				RenderText(textProgram, "(W) Wireframe Mode: OFF", 5.0f, 55.0f, 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));
			}

			// Create Polygon count string
			string polygon = "Polygon Count: ";
			polygon.append(to_string(polyCount[0]));

			// Render polygon count string
			RenderText(textProgram, polygon, 0.0f, 1050.0f, 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));

			// Draw Sun Light Source
			drawSun(lampProgram, Models[3], 3.0f);
			passMatrixToShader(lampProgram, projection, view);
		}
		else if (currentTime > 15 & currentTime < 32) {
			// Display Level names
			RenderText(textProgram, "L0              L1               L2               L3               L4", 280.0f, 350.0f, 1.3f, glm::vec3(1.0f, 0.0f, 0.0f));
			lightingProgram.Use(); // Switch to correct Shader

			// Display each level in sequence
			for (int i = 0; i < 5; i++) {
				Models[4-i].transform(lightingProgram, glm::vec3((float) (i*2)- 4, 25.0f, 9.0f));
				Models[4-i].changeColour(lightingProgram, colours[4-i]);
				Models[4-i].Draw(lightingProgram);
			}
			mode = 1; // Switch to Exaggerated LOD mode
		}
		// Move Camera to Position 2, Switch back to Normal LOD mode
		if (currentTime > 47 & currentTime < 57) {
			cameraMovePos2();
			mode = 0;
		}

		// Move Camera back to Position 1
		if (currentTime > 57 & currentTime < 90){
			cameraMovePos1();
		}

		// Move Camera to Position 3
		if (currentTime > 90 & currentTime < 105) {
			cameraMovePos3();
			lightingProgram.Use(); // Switch to correct Shader

			// Render 5 sphere in far distance
			for (int i = 0; i < 5; i++) {
				Models[4 - i].transform(lightingProgram, glm::vec3((i*5) - 15, -160.0f, 2.0f));
				Models[4 - i].changeColour(lightingProgram, colours[4]);
				Models[4 - i].Draw(lightingProgram);
			}
		}
		if (currentTime > 105 & currentTime < 106)
			cameraMovePos1();

		// Swap Buffer
		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 0;
}

/// TEXT RENDERING -----------------------------------------------------------------------------------------
/// Credit: JOEY DE VRIES ----------------------------------------------------------------------------------
/// --------------------------------------------------------------------------------------------------------
void RenderText(Shader &s, std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color)
{
	// Activate corresponding render state	
	s.Use();
	glUniform3f(glGetUniformLocation(s.Program, "textColor"), color.x, color.y, color.z);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(VAO);

	// Iterate through all characters
	std::string::const_iterator c;
	for (c = text.begin(); c != text.end(); c++)
	{
		Character ch = Characters[*c];

		GLfloat xpos = x + ch.Bearing.x * scale;
		GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

		GLfloat w = ch.Size.x * scale;
		GLfloat h = ch.Size.y * scale;
		// Update VBO for each character
		GLfloat vertices[6][4] = {
			{ xpos,     ypos + h,   0.0, 0.0 },
			{ xpos,     ypos,       0.0, 1.0 },
			{ xpos + w, ypos,       1.0, 1.0 },

			{ xpos,     ypos + h,   0.0, 0.0 },
			{ xpos + w, ypos,       1.0, 1.0 },
			{ xpos + w, ypos + h,   1.0, 0.0 }
		};
		// Render glyph texture over quad
		glBindTexture(GL_TEXTURE_2D, ch.TextureID);
		// Update content of VBO memory
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		// Render quad
		glDrawArrays(GL_TRIANGLES, 0, 6);
		// Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	
}


/// CALLBACK FUNCTIONS -------------------------------------------------------------------------------------
/// Credit: JOEY DE VRIES ----------------------------------------------------------------------------------
/// --------------------------------------------------------------------------------------------------------

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	// EXIT if escape key pressed
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	// Set currently pressed keys
	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			keys[key] = true;
		else if (action == GLFW_RELEASE)
			keys[key] = false;
	}
	// Run function based off key
	key_triggered();
}

// Analyse Pressed keys
void key_triggered() {
	if (keys[GLFW_KEY_RIGHT]) {
		if (mode < 7)
			mode += 1; // Set Object Mode
		if(mode >= 7)
			mode = 0;
	}
	if (keys[GLFW_KEY_LEFT]) {
		if (mode >= 0)
			mode -= 1; // Set Object Mode
		if (mode < 0)
			mode = 6;
	}
	if (keys[GLFW_KEY_C]) {
		camPos++;
		if (camPos == 1)
			cameraMovePos1();
		if (camPos == 2)
			cameraMovePos2();
		if (camPos == 3) {
			cameraMovePos3();
			camPos = 0;
		}
	}
	if (keys[GLFW_KEY_W]) {
		wireframe = !wireframe;
	}
}

/// --------------------------------------------------------------------------------------------------------
/// --------------------------------------------------------------------------------------------------------
/// --------------------------------------------------------------------------------------------------------