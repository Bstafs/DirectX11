#include "Camera.h"

Camera::Camera(XMFLOAT3 position, XMFLOAT3 at, XMFLOAT3 up, FLOAT windowWidth, FLOAT windowHeight, FLOAT nearDepth, FLOAT farDepth)
{
	_eye = position;
	_at = at;
	_up = up;

	_windowWidth = windowWidth;
	_windowHeight = windowHeight;
	_nearDepth = nearDepth;
	_farDepth = farDepth;

}

Camera::~Camera()
{

}

void Camera::Update()
{

}

XMFLOAT3 Camera::GetPosition()
{
	return _eye;
}

void Camera::SetPosition(XMFLOAT3 position)
{
	_eye = position;
}

XMFLOAT3 Camera::GetLookAt()
{
	return _at;
}

void Camera::SetLookAt(XMFLOAT3 atPosition)
{
	_at = atPosition;
}

XMFLOAT3 Camera::GetUp()
{
	return _up;
}

void Camera::SetUp(XMFLOAT3 upPosition)
{
	_up = upPosition;
}

XMFLOAT4X4* Camera::GetView()
{
	return &_view;
}

void Camera::SetView()
{
	XMVECTOR Eye = XMVectorSet(_eye.x, _eye.y, _eye.z, 0.0f);
	XMVECTOR At = XMVectorSet(_at.x, _at.y, _at.z, 0.0f);
	XMVECTOR Up = XMVectorSet(_up.x, _up.y, _up.z, 0.0f);

	XMStoreFloat4x4(&_view, XMMatrixLookAtLH(Eye, At, Up));
}

XMFLOAT4X4* Camera::GetProjection()
{
	return &_projection;
}

void Camera::SetProjection()
{
	XMStoreFloat4x4(&_projection, XMMatrixPerspectiveFovLH(XM_PIDIV2, _windowWidth / (FLOAT)_windowHeight, _nearDepth, _farDepth));
}

void Camera::CombinedViewaAndProjection()
{

}

void Camera::Reshape(FLOAT windowWidth, FLOAT windowHeight, FLOAT nearDepth, FLOAT farDepth)
{
	_windowHeight = windowHeight;
	_windowHeight = windowHeight;
	_nearDepth = nearDepth;
	_farDepth = farDepth;
}



