///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

///////////////////////////////////////////////////////////////////////////////
//  SceneManager()
//
//  The constructor for the class
///////////////////////////////////////////////////////////////////////////////
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

///////////////////////////////////////////////////////////////////////////////
//  ~SceneManager()
//
//  The destructor for the class
///////////////////////////////////////////////////////////////////////////////
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

///////////////////////////////////////////////////////////////////////////////
//  CreateGLTexture()
///////////////////////////////////////////////////////////////////////////////
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

///////////////////////////////////////////////////////////////////////////////
//  BindGLTextures()
///////////////////////////////////////////////////////////////////////////////
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

///////////////////////////////////////////////////////////////////////////////
//  DestroyGLTextures()
///////////////////////////////////////////////////////////////////////////////
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

///////////////////////////////////////////////////////////////////////////////
//  FindTextureID()
///////////////////////////////////////////////////////////////////////////////
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

///////////////////////////////////////////////////////////////////////////////
//  FindTextureSlot()
///////////////////////////////////////////////////////////////////////////////
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

///////////////////////////////////////////////////////////////////////////////
//  FindMaterial()
///////////////////////////////////////////////////////////////////////////////
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	size_t index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(bFound);
}

///////////////////////////////////////////////////////////////////////////////
//  SetTransformations()
///////////////////////////////////////////////////////////////////////////////
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	scale = glm::scale(scaleXYZ);
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

///////////////////////////////////////////////////////////////////////////////
//  SetShaderColor()
//  sets the color
///////////////////////////////////////////////////////////////////////////////
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

///////////////////////////////////////////////////////////////////////////////
//  SetShaderTexture()
//  sets the texture
///////////////////////////////////////////////////////////////////////////////
void SceneManager::SetShaderTexture(std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

///////////////////////////////////////////////////////////////////////////////
//  SetTextureUVScale()
//  sets the scale
///////////////////////////////////////////////////////////////////////////////
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

///////////////////////////////////////////////////////////////////////////////
//  SetShaderMaterial()
///////////////////////////////////////////////////////////////////////////////
void SceneManager::SetShaderMaterial(std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//  DefineObjectMaterials()
//
//  defines basic material settings for the desk and laptop surfaces.
///////////////////////////////////////////////////////////////////////////////
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL deskMaterial;
	deskMaterial.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	deskMaterial.specularColor = glm::vec3(0.35f, 0.30f, 0.22f);
	deskMaterial.shininess = 32.0f;
	deskMaterial.tag = "desk_material";
	m_objectMaterials.push_back(deskMaterial);

	OBJECT_MATERIAL laptopMaterial;
	laptopMaterial.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	laptopMaterial.specularColor = glm::vec3(0.25f, 0.25f, 0.25f);
	laptopMaterial.shininess = 16.0f;
	laptopMaterial.tag = "laptop_material";
	m_objectMaterials.push_back(laptopMaterial);

	OBJECT_MATERIAL ceramicMaterial;
	ceramicMaterial.diffuseColor = glm::vec3(0.92f, 0.92f, 0.90f);
	ceramicMaterial.specularColor = glm::vec3(0.55f, 0.55f, 0.55f);
	ceramicMaterial.shininess = 48.0f;
	ceramicMaterial.tag = "ceramic_material";
	m_objectMaterials.push_back(ceramicMaterial);

	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.diffuseColor = glm::vec3(0.75f, 0.75f, 0.75f);
	plasticMaterial.specularColor = glm::vec3(0.20f, 0.20f, 0.20f);
	plasticMaterial.shininess = 12.0f;
	plasticMaterial.tag = "plastic_material";
	m_objectMaterials.push_back(plasticMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.80f, 0.68f, 0.45f);
	woodMaterial.specularColor = glm::vec3(0.18f, 0.16f, 0.12f);
	woodMaterial.shininess = 10.0f;
	woodMaterial.tag = "wood_material";
	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL metalMaterial;
	metalMaterial.diffuseColor = glm::vec3(0.78f, 0.78f, 0.80f);
	metalMaterial.specularColor = glm::vec3(0.72f, 0.72f, 0.72f);
	metalMaterial.shininess = 64.0f;
	metalMaterial.tag = "metal_material";
	m_objectMaterials.push_back(metalMaterial);
}

///////////////////////////////////////////////////////////////////////////////
//  SetupSceneLights()
//
//  adds a soft overhead directional light and a small warm point light.
///////////////////////////////////////////////////////////////////////////////
void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// general room light keeps the scene visible.
	m_pShaderManager->setVec3Value("directionalLight.direction", -0.3f, -1.0f, -0.4f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.25f, 0.25f, 0.25f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.65f, 0.65f, 0.60f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.35f, 0.35f, 0.35f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// light to add highlights to the desk plane.
	m_pShaderManager->setVec3Value("pointLights[0].position", 0.0f, 3.0f, 2.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.08f, 0.07f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.55f, 0.48f, 0.38f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.60f, 0.55f, 0.45f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);
}

///////////////////////////////////////////////////////////////////////////////
//  PrepareScene()
//
//  this prepares the 3D scene by loading
//  the shapes and initializing texture assets into memory.
///////////////////////////////////////////////////////////////////////////////

void SceneManager::PrepareScene()
{
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTorusMesh();

	// this handles all loading
	LoadSceneTextures();

	DefineObjectMaterials();
	SetupSceneLights();
}

///////////////////////////////////////////////////////////////////////////////
//  RenderScene()
//
//  this is used for rendering the 3D scene by setting transformations
//  and drawing the individual meshes.
///////////////////////////////////////////////////////////////////////////////

void SceneManager::RenderScene()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	// enable texturing for the desk plane so it resets from the last loop
	m_pShaderManager->setIntValue(g_UseTextureName, true);
	SetShaderTexture("desk_surface");
	SetShaderMaterial("desk_material");
	SetTextureUVScale(4.0f, 4.0f);
	m_basicMeshes->DrawPlaneMesh();

	RenderLaptop();
	RenderMug();
	RenderPencilHolder();
}

//////////////////////////////////////////////////////////////////////////////
//  RenderMug()
//
//  renders a coffee mug
//  it has a rim lip, and a side handle.
///////////////////////////////////////////////////////////////////////////////
void SceneManager::RenderMug()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	m_pShaderManager->setIntValue(g_UseTextureName, false);
	SetShaderMaterial("ceramic_material");
	SetShaderColor(0.93f, 0.93f, 0.95f, 1.0f);

	// outer shell of the coffee mug on the desk.
	scaleXYZ = glm::vec3(0.58f, 0.95f, 0.58f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(2.45f, 0.0f, 0.55f); 
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawCylinderMesh(false, true, true);

	// the inner is dark to simulate coffee
	scaleXYZ = glm::vec3(0.50f, 0.70f, 0.50f);
	positionXYZ = glm::vec3(2.45f, 0.25f, 0.55f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.06f, 0.04f, 0.03f, 1.0f);
	m_basicMeshes->DrawCylinderMesh(true, false, true);

	// rounded rim lip at the opening/top
	scaleXYZ = glm::vec3(0.55f, 0.10f, 0.48f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, glm::vec3(2.45f, 0.95f, 0.55f));
	SetShaderColor(0.93f, 0.93f, 0.95f, 1.0f);
	m_basicMeshes->DrawTorusMesh();

	// the handle is a torus so it should slightly clip into the mug
	scaleXYZ = glm::vec3(0.33f, 0.30f, 0.25f);
	XrotationDegrees = 90.0f; // tips the flat ring up so it stands vertically beside the cup
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(3.28f, 0.475f, 0.55f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.93f, 0.93f, 0.95f, 1.0f);
	m_basicMeshes->DrawTorusMesh();
}

///////////////////////////////////////////////////////////////////////////////
//  RenderPencilHolder()
//
//  renders the hollow pencil holder and the two pencils inside it.
///////////////////////////////////////////////////////////////////////////////
void SceneManager::RenderPencilHolder()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	auto rotatedYOffset = [](float xDeg, float yDeg, float zDeg, float yOffset)
	{
		glm::mat4 rotationX = glm::rotate(glm::radians(xDeg), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 rotationY = glm::rotate(glm::radians(yDeg), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 rotationZ = glm::rotate(glm::radians(zDeg), glm::vec3(0.0f, 0.0f, 1.0f));
		return glm::vec3(rotationZ * rotationY * rotationX * glm::vec4(0.0f, yOffset, 0.0f, 0.0f));
	};

	m_pShaderManager->setIntValue(g_UseTextureName, false);

	// this is the pencil holder. i made multiple walls for it to look like a box
	scaleXYZ = glm::vec3(0.88f, 0.05f, 0.68f);
	positionXYZ = glm::vec3(-2.55f, 0.025f, 0.65f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.18f, 0.18f, 0.20f, 1.0f);
	SetShaderMaterial("plastic_material");
	m_basicMeshes->DrawBoxMesh();

	// back wall
	scaleXYZ = glm::vec3(0.88f, 1.15f, 0.05f);
	positionXYZ = glm::vec3(-2.55f, 0.60f, 0.315f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.18f, 0.18f, 0.20f, 1.0f);
	SetShaderMaterial("plastic_material");
	m_basicMeshes->DrawBoxMesh();

	// front wall
	positionXYZ = glm::vec3(-2.55f, 0.60f, 0.985f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.18f, 0.18f, 0.20f, 1.0f);
	SetShaderMaterial("plastic_material");
	m_basicMeshes->DrawBoxMesh();

	// left wall
	scaleXYZ = glm::vec3(0.05f, 1.15f, 0.68f);
	positionXYZ = glm::vec3(-2.99f, 0.60f, 0.65f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.18f, 0.18f, 0.20f, 1.0f);
	SetShaderMaterial("plastic_material");
	m_basicMeshes->DrawBoxMesh();

	// right wall
	positionXYZ = glm::vec3(-2.11f, 0.60f, 0.65f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.18f, 0.18f, 0.20f, 1.0f);
	SetShaderMaterial("plastic_material");
	m_basicMeshes->DrawBoxMesh();

	// the pencils (1st one)
	scaleXYZ = glm::vec3(0.045f, 0.78f, 0.045f);
	XrotationDegrees = 8.0f;
	YrotationDegrees = -6.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-2.62f, 0.06f, 0.62f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_pShaderManager->setIntValue(g_UseTextureName, false);
	SetShaderColor(0.96f, 0.64f, 0.18f, 1.0f);
	SetShaderMaterial("wood_material");
	m_basicMeshes->DrawCylinderMesh();

	// sharpened tip
	scaleXYZ = glm::vec3(0.045f, 0.16f, 0.045f);
	positionXYZ = glm::vec3(-2.62f, 0.06f, 0.62f) + rotatedYOffset(8.0f, -6.0f, 0.0f, 0.78f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.08f, 0.08f, 0.08f, 1.0f);
	SetShaderMaterial("metal_material");
	m_basicMeshes->DrawConeMesh();

	// graphite core
	scaleXYZ = glm::vec3(0.014f, 0.05f, 0.014f);
	positionXYZ = glm::vec3(-2.62f, 0.06f, 0.62f) + rotatedYOffset(8.0f, -6.0f, 0.0f, 0.92f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.02f, 0.02f, 0.02f, 1.0f);
	SetShaderMaterial("metal_material");
	m_basicMeshes->DrawConeMesh();

	// (2nd pencil) this is offset and tilted.
	scaleXYZ = glm::vec3(0.045f, 0.74f, 0.045f);
	XrotationDegrees = -6.0f;
	YrotationDegrees = 10.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-2.35f, 0.06f, 0.72f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_pShaderManager->setIntValue(g_UseTextureName, false);
	SetShaderColor(0.96f, 0.64f, 0.18f, 1.0f);
	SetShaderMaterial("wood_material");
	m_basicMeshes->DrawCylinderMesh();

	// metal
	scaleXYZ = glm::vec3(0.052f, 0.10f, 0.052f);
	positionXYZ = glm::vec3(-2.35f, 0.06f, 0.72f) + rotatedYOffset(-6.0f, 10.0f, 0.0f, 0.74f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.72f, 0.72f, 0.75f, 1.0f);
	SetShaderMaterial("metal_material");
	m_basicMeshes->DrawCylinderMesh();

	// eraser
	scaleXYZ = glm::vec3(0.05f, 0.03f, 0.05f);
	positionXYZ = glm::vec3(-2.35f, 0.02f, 0.72f) + rotatedYOffset(-6.0f, 10.0f, 0.0f, 0.87f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.93f, 0.55f, 0.64f, 1.0f);
	SetShaderMaterial("plastic_material");
	m_basicMeshes->DrawSphereMesh();
}


///////////////////////////////////////////////////////////////////////////////
//  RenderLaptop()
//
//  this is used for rendering the laptop complex object by defining 
//  transformations and sequences for the base, screen, and hinges.
///////////////////////////////////////////////////////////////////////////////
void SceneManager::RenderLaptop()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	///////////////////////////////////////////////////
	// 1. LAPTOP BASE (part one)
	///////////////////////////////////////////////////
	scaleXYZ = glm::vec3(3.0f, 0.1f, 2.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.05f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	SetShaderTexture("keyboard");
	SetShaderMaterial("laptop_material");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::box_top);

	// casing base shell
	SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);
	SetShaderMaterial("laptop_material");
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::box_bottom);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::box_front);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::box_back);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::box_left);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::box_right);

	/////////////////////////////////////////////
	// 2. LAPTOP SCREEN (part 2)
	/////////////////////////////////////////////
	scaleXYZ = glm::vec3(3.0f, 2.0f, 0.1f);
	XrotationDegrees = -15.0f; // // Tilt back slightly to simulate being open
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 1.05f, -1.2f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// color structural back and frame elements
	SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);
	SetShaderMaterial("laptop_material");
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::box_back);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::box_top);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::box_bottom);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::box_left);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::box_right);

	// bind display texture and project onto front panel last
	SetShaderTexture("screen_display");
	SetShaderMaterial("laptop_material");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::box_front);

	/////////////////////////////////////////////
	// 3. HINGES (part 3)
	/////////////////////////////////////////////
	// left hinge
	scaleXYZ = glm::vec3(0.1f, 0.4f, 0.1f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(-0.7f, 0.1f, -1.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f);
	SetShaderMaterial("laptop_material");
	m_basicMeshes->DrawCylinderMesh();

	// right hinge
	scaleXYZ = glm::vec3(0.1f, 0.4f, 0.1f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(1.0f, 0.1f, -1.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f);
	SetShaderMaterial("laptop_material");
	m_basicMeshes->DrawCylinderMesh();
}

///////////////////////////////////////////////////////////////////////////////
//  LoadSceneTextures()
//
//  this is used for loading all scene textures into memory once,
//  configuring pixel storage alignment, and binding them to texture slots.
///////////////////////////////////////////////////////////////////////////////
void SceneManager::LoadSceneTextures()
{
	m_loadedTextures = 0;
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	CreateGLTexture("textures/laptop_1.jpg", "keyboard");
	CreateGLTexture("textures/cherrywood.jpg", "desk_surface");
	CreateGLTexture("textures/screen_lights.jpg", "screen_display");
	

	BindGLTextures();
}
