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
	HINSTANCE               _hInst;
	HWND                    _hWnd;
	D3D_DRIVER_TYPE         _driverType;
	D3D_FEATURE_LEVEL       _featureLevel;
	ID3D11Device*           _pd3dDevice;
	ID3D11DeviceContext*    _pImmediateContext;
	IDXGISwapChain*         _pSwapChain;
	ID3D11RenderTargetView* _pRenderTargetView;
	//Shaders
	ID3D11VertexShader*     _pVertexShader;
	ID3D11PixelShader*      _pPixelShader;
	ID3D11InputLayout*      _pVertexLayout;
	//Buffers
	ID3D11Buffer*           _pCubeVertexBuffer;
	ID3D11Buffer*           _pCubeIndexBuffer;
	ID3D11Buffer*           _pPyramidVertexBuffer;
	ID3D11Buffer*           _pPyramidIndexBuffer;
	ID3D11Buffer*           _pPlaneVertexBuffer;
	ID3D11Buffer*           _pPlaneIndexBuffer;
	ID3D11Buffer*           _pConstantBuffer;
	// World Object
	XMFLOAT4X4              _world, _world2, _world3,_world4,_world5;
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
	ID3D11DepthStencilView* _depthStencilView;
	ID3D11Texture2D*        _depthStencilBuffer;
	ID3D11RasterizerState*  _wireFrame; 
	ID3D11RasterizerState*  _solidFrame;
	//Textures
	ID3D11ShaderResourceView* _pTextureRV = nullptr;
	ID3D11SamplerState* _pSamplerLinear = nullptr;
	//Objects
	MeshData                objPlane;
	MeshData                objSphere;
	//Time
	float gTime;
	// Camera
	Camera* _currentCamera;
	Camera* _camera1;
	Camera* _camera2;
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
	HRESULT InitPlaneVertexBuffer();
	HRESULT InitPlaneIndexBuffer();
	UINT _WindowHeight;
	UINT _WindowWidth;

public:
	Application();
	~Application();

	HRESULT Initialise(HINSTANCE hInstance, int nCmdShow);

	void Update();
	void Draw();
};
