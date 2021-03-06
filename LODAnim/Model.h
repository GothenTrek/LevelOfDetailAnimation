#pragma once
// Author:  George Othen
// Date: 14/02/2017
// Title: Model class for Model loading 

// Source: https://learnopengl.com
// Credit: JOEY DE VRIES

// Std. Includes
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

// GL Includes
#include <GL/glew.h> // Contains all the necessery OpenGL includes
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Importer.hpp>
#include <scene.h>
#include <postprocess.h>

// custom Includes
#include "Mesh.h"
#include "Shader.h"

using namespace std;

class Model
{
public:
	/*  Functions   */
	// Constructor, expects a filepath to a 3D model.
	Model(string path)
	{
		this->loadModel(path);
	}

	// Draws the model, and thus all its meshes
	void Draw(Shader shader)
	{
			this->meshes[0].Draw(shader);
	}

	// Change the colour applied to all vertices
	void changeColour(Shader shader, glm::vec3 Colour) {
		GLint colourLoc = glGetUniformLocation(shader.Program, "myColour");
		glUniform3f(colourLoc, Colour[0], Colour[1], Colour[2]);
	}

	// Change the scale of the Model
	void scale(Shader shader, glm::vec3 scale) {
		glm::mat4 model;
		model = glm::scale(model, scale);
		GLint modelLoc = glGetUniformLocation(shader.Program, "model");
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	}

	// Change the rotation of the Model
	void rotate(Shader shader, glm::vec3 rotationVector, float rotationAmount) {
		glm::mat4 model;
		model = glm::rotate(model, rotationAmount, rotationVector);
		GLint modelLoc = glGetUniformLocation(shader.Program, "model");
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	}

	// Change the rotation and scale the Model
	void rotateS(Shader shader, glm::vec3 rotationVector, float rotationAmount, glm::vec3 scale) {
		glm::mat4 model;
		model = glm::rotate(model, rotationAmount, rotationVector);
		model = glm::scale(model, scale);
		GLint modelLoc = glGetUniformLocation(shader.Program, "model");
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	}

	// Transform the Model
	void transform(Shader shader, glm::vec3 transform) {
		glm::mat4 model;
		model = glm::translate(model, transform);
		GLint modelLoc = glGetUniformLocation(shader.Program, "model");
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	}

	// Transform and Rotate the Model
	void transformR(Shader shader, glm::vec3 transform, glm::vec3 rotationVector, float rotationAmount, bool continuousRotate) {	
		glm::mat4 model;
		model = glm::translate(model, transform);
		if (continuousRotate)
			model = glm::rotate(model, (GLfloat)glfwGetTime() * glm::radians(rotationAmount), rotationVector);
		else
			model = glm::rotate(model, glm::radians(rotationAmount), rotationVector);
		GLint modelLoc = glGetUniformLocation(shader.Program, "model");
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	}

	// Transform, Rotate and Scale the Model
	void transformRS(Shader shader, glm::vec3 transform, glm::vec3 rotationVector, float rotationAmount, glm::vec3 scale) {
		glm::mat4 model;
		model = glm::translate(model, transform);
		model = glm::rotate(model, rotationAmount, rotationVector);
		model = glm::scale(model, scale);
		GLint modelLoc = glGetUniformLocation(shader.Program, "model");
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	}
private:
	/*  Model Data  */
	vector<Mesh> meshes;
	string directory;

	/*  Functions   */
	// Loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
	void loadModel(string path)
	{
		// Read file via ASSIMP
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate);
		// Check for errors
		if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
		{
			cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
			return;
		}
		// Retrieve the directory path of the filepath
		this->directory = path.substr(0, path.find_last_of('/'));

		// Process ASSIMP's root node recursively
		this->processNode(scene->mRootNode, scene);
	}

	// Processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
	void processNode(aiNode* node, const aiScene* scene)
	{
		// Process each mesh located at the current node
		for (GLuint i = 0; i < node->mNumMeshes; i++)
		{
			// The node object only contains indices to index the actual objects in the scene. 
			// The scene contains all the data, node is just to keep stuff organized (like relations between nodes).
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			this->meshes.push_back(this->processMesh(mesh, scene));
		}
		// After we've processed all of the meshes (if any) we then recursively process each of the children nodes
		for (GLuint i = 0; i < node->mNumChildren; i++)
		{
			this->processNode(node->mChildren[i], scene);
		}

	}

	Mesh processMesh(aiMesh* mesh, const aiScene* scene)
	{
		// Data to fill
		vector<Vertex> vertices;
		vector<GLuint> indices;

		// Walk through each of the mesh's vertices
		for (GLuint i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			glm::vec3 vector; // We declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
							  // Positions
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.Position = vector;
			// Normals
			vector.x = mesh->mNormals[i].x;
			vector.y = mesh->mNormals[i].y;
			vector.z = mesh->mNormals[i].z;
			vertex.Normal = vector;
			vertices.push_back(vertex);
		}
		// Now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
		for (GLuint i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			// Retrieve all indices of the face and store them in the indices vector
			for (GLuint j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}

		// Return a mesh object created from the extracted mesh data
		return Mesh(vertices, indices);
	}
};