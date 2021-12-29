#pragma once

#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
#include "resource.h"
#include <time.h>
#include "DDSTextureLoader.h"
#include "OBJLoader.h"
#include "Structures.h"
#include "Camera.h"
#include <vector>
#include <fstream>
#include <iostream>

using namespace DirectX;

struct ConstantBuffer
{
	XMMATRIX mWorld;
	XMMATRIX mView;
	XMMATRIX mProjection;
	XMFLOAT4 DiffuseMtrl;

	XMFLOAT4 DiffuseLight;
	XMFLOAT3 LightVecw;
	float gTime;
	XMFLOAT4 AmbientMtrl;
	XMFLOAT4 AmbientLight;

	XMFLOAT4 SpecularLight;
	XMFLOAT4 SpecularMtrl;
	XMFLOAT3 eyePos;
	
	float SpecularPower;

	
};

class Application
{
private:
	HINSTANCE               hInst;
	HWND                    hWnd;
	D3D_DRIVER_TYPE         driverType;
	D3D_FEATURE_LEVEL       featureLevel;
	ID3D11Device*           pd3dDevice;
	ID3D11DeviceContext*    pImmediateContext;
	IDXGISwapChain*         pSwapChain;
	ID3D11RenderTargetView* pRenderTargetView;
	//Shaders
	ID3D11VertexShader*     pVertexShader;
	ID3D11PixelShader*      pPixelShader;
	ID3D11InputLayout*      pVertexLayout;
	//Buffers
	ID3D11Buffer*           pCubeVertexBuffer;
	ID3D11Buffer*           pCubeIndexBuffer;
	ID3D11Buffer*           pPyramidVertexBuffer;
	ID3D11Buffer*           pPyramidIndexBuffer;
	ID3D11Buffer*           pPlaneVertexBuffer;
	ID3D11Buffer*           pPlaneIndexBuffer;
	ID3D11Buffer*           pgridVertexBuffer;
	ID3D11Buffer*           pgridIndexBuffer;
	ID3D11Buffer*           pConstantBuffer;
	// World Object
	XMFLOAT4X4              sphere, pyramid, cube,hercules,car,terrain;
	//Lighting
	XMFLOAT3                lightDirection;
	XMFLOAT4                diffuseMaterial;
	XMFLOAT4                diffuseLight;
	XMFLOAT4                ambientMaterial;
	XMFLOAT4                ambientLight;
	XMFLOAT4                specularMaterial;
	XMFLOAT4                specularLight;
	float                   specularPower;
	XMFLOAT4                eyePos;
	//Depth Buffers
	ID3D11DepthStencilView* depthStencilView;
	ID3D11Texture2D*        depthStencilBuffer;
	ID3D11RasterizerState*  wireFrame; 
	ID3D11RasterizerState*  solidFrame;
	//SamplerState
	ID3D11SamplerState* pSamplerState = nullptr;
	//Textures
	ID3D11ShaderResourceView* pTextureCrate = nullptr;
	ID3D11ShaderResourceView* pTextureHercules = nullptr;
	ID3D11ShaderResourceView* pTextureSun = nullptr;
	ID3D11ShaderResourceView* pTextureMud = nullptr;
	ID3D11ShaderResourceView* pTextureSurface = nullptr;
	//Objects
	MeshData                objPlane;
	MeshData                objSphere;
	MeshData                objCar;
	//Time
	float gTime;
	// Camera
	Camera* pCurrentCamera;
	Camera* pCamera1;
	Camera* pCamera2;
	Camera* pCarCamera;
	Camera* pTopDownCamera;
	// Blending
	ID3D11BlendState* Transparency;
	bool isTransparent;
	// Car
	float speed;
	XMFLOAT3 carPosition;
	float currentPosZ;
	float currentPosX;
	float rotationX;
	XMMATRIX carMatrix;
	// Terrain
	UINT rows;
	UINT columns;
	int totalCells;
	UINT totalFaces;
	int totalVertices;
	int depth;
	int width;
	// Width & Depth
	float dx;
	float dz;
	//Texture Coords
	float du;
	float dv;
	// Height map
	XMFLOAT3* heightMap;
	UINT terrainWidth;
	UINT terrainHeight;
private:
	HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
	HRESULT InitDevice();
	void Cleanup();
	HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
	HRESULT InitShadersAndInputLayout();
	HRESULT InitCubeVertexBuffer();
	HRESULT InitCubeIndexBuffer();
	HRESULT InitPyramidVertexBuffer();
	HRESULT InitPyramidIndexBuffer();
	HRESULT CreateTerrain(char* filename);

	UINT _WindowHeight;
	UINT _WindowWidth;
public:
	Application();
	~Application();

	HRESULT Initialise(HINSTANCE hInstance, int nCmdShow);

	void Update();
	void Draw();

};

