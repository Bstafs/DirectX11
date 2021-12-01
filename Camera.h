#pragma once
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
#include "resource.h"


using namespace DirectX;

class Camera
{
private:

	XMFLOAT3 _eye;
	XMFLOAT3 _at;
	XMFLOAT3 _up;
	
	FLOAT _windowWidth;
	FLOAT _windowHeight;
	FLOAT _nearDepth;
	FLOAT _farDepth;

	XMFLOAT4X4 _view;
	XMFLOAT4X4 _projection;




public:

	Camera(XMFLOAT3 position, XMFLOAT3 at, XMFLOAT3 up, FLOAT windowWidth, FLOAT windowHeight, FLOAT nearDepth, FLOAT farDepth);

	~Camera();

	void Update();

	//Return Position, Lookat and up;
	XMFLOAT3 GetPosition();
	void SetPosition(XMFLOAT3 position);
	XMFLOAT3 GetLookAt();
	void SetLookAt(XMFLOAT3 atPosition);
	XMFLOAT3 GetUp();
	void SetUp(XMFLOAT3 upPosition);

	//Return View, Projection and combined viewProjection;
	XMFLOAT4X4* GetView();
	void SetView();
	XMFLOAT4X4* GetProjection();
	void SetProjection();

	void CombinedViewaAndProjection();


	void Reshape(FLOAT windowWidth, FLOAT windowHeight, FLOAT nearDepth, FLOAT farDepth);
};

