#include "Application.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

Application::Application()
{
	_hInst = nullptr;
	_hWnd = nullptr;
	_driverType = D3D_DRIVER_TYPE_NULL;
	_featureLevel = D3D_FEATURE_LEVEL_11_0;
	_pd3dDevice = nullptr;
	_pImmediateContext = nullptr;
	_pSwapChain = nullptr;
	_pRenderTargetView = nullptr;
	_pVertexShader = nullptr;
	_pPixelShader = nullptr;
	_pVertexLayout = nullptr;
	_pCubeVertexBuffer = nullptr;
	_pPyramidVertexBuffer = nullptr;
	_pPlaneVertexBuffer = nullptr;
	_pCubeIndexBuffer = nullptr;
	_pPyramidIndexBuffer = nullptr;
	_pPlaneIndexBuffer = nullptr;
	_pConstantBuffer = nullptr;
	
}

Application::~Application()
{
	Cleanup();
}

HRESULT Application::Initialise(HINSTANCE hInstance, int nCmdShow)
{
	if (FAILED(InitWindow(hInstance, nCmdShow)))
	{
		return E_FAIL;
	}

	RECT rc;
	GetClientRect(_hWnd, &rc);
	_WindowWidth = rc.right - rc.left;
	_WindowHeight = rc.bottom - rc.top;

	if (FAILED(InitDevice()))
	{
		Cleanup();

		return E_FAIL;
	}

	// Initialize the world matrix
	XMStoreFloat4x4(&sphere, XMMatrixIdentity());

	_camera1 = new Camera(XMFLOAT3(0.0f, 10.0f, -20.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), _WindowWidth, _WindowHeight, 0.01f, 1000.0f);
	_camera2 = new Camera(XMFLOAT3(0.0f, 10.0f, 20.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), _WindowWidth, _WindowHeight, 0.01f, 1000.0f);
	_carCamera = new Camera(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), _WindowWidth, _WindowHeight, 0.01f, 1000.0f);
	_currentCamera = _camera1;
	_currentCamera->SetView();
	_currentCamera->SetProjection();

	// Create the sample state

	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	_pd3dDevice->CreateSamplerState(&sampDesc, &_pSamplerState);

	objPlane = OBJLoader::Load("Hercules.obj", _pd3dDevice);
	objCar = OBJLoader::Load("car.obj", _pd3dDevice);
	objSphere = OBJLoader::Load("sphere.obj", _pd3dDevice, false);

	//Application::HeightMapLoad("Heightmap.bmp");
	Application::CreatGrid(128, 128, 128, 128, "Heightmap.BMP");

	return S_OK;
}

HRESULT Application::InitShadersAndInputLayout()
{
	HRESULT hr;

	// Compile the vertex shader
	ID3DBlob* pVSBlob = nullptr;
	hr = CompileShaderFromFile(L"DX11 Framework.fx", "VS", "vs_4_0", &pVSBlob);

	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the vertex shader
	hr = _pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &_pVertexShader);

	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"DX11 Framework.fx", "PS", "ps_4_0", &pPSBlob);

	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = _pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &_pPixelShader);
	pPSBlob->Release();

	if (FAILED(hr))
		return hr;

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = _pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &_pVertexLayout);
	pVSBlob->Release();

	if (FAILED(hr))
		return hr;

	// Set the input layout
	_pImmediateContext->IASetInputLayout(_pVertexLayout);

	return hr;
}

HRESULT Application::InitCubeVertexBuffer()
{
	HRESULT hr;

	// Create vertex buffer
	SimpleVertex verticesCube[] =
	{
		{ XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT3(-0.666667f, -0.666667f, 0.333333f), XMFLOAT2(0,0) },//top left 0
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(0.816497, 0.408248f, 0.408248f), XMFLOAT2(1,0) },//top right 1
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT3(-0.666667f, -0.666667f, 0.333333f), XMFLOAT2(0,1)},//bottom left 2
		{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT3(0.408248f, -0.408248f, 0.816497f), XMFLOAT2(1,1) },//bottom right 3

		{ XMFLOAT3(-1.0f, 1.0f, 2.0f), XMFLOAT3(-0.816497f, 0.408248f, -0.408248f), XMFLOAT2(1,1)},//top left 4 
		{ XMFLOAT3(1.0f, 1.0f, 2.0f), XMFLOAT3(0.333333f, 0.666667f, -0.666667f), XMFLOAT2(0,1) },//top right 5 
		{ XMFLOAT3(-1.0f, -1.0f, 2.0f), XMFLOAT3(-0.408248f, -0.408248f, -0.816497f), XMFLOAT2(1,0) },//bottom left 6 
		{ XMFLOAT3(1.0f, -1.0f, 2.0f), XMFLOAT3(0.666667f, -0.666667f, -0.333333f), XMFLOAT2(0,0) },//bottom right 7
	};




	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 8;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = verticesCube;

	hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pCubeVertexBuffer);


	if (FAILED(hr))
		return hr;

	return S_OK;
}

HRESULT Application::InitPyramidVertexBuffer()
{
	HRESULT hr;

	// Create vertex buffer
	SimpleVertex verticesPyramid[] =
	{
		{ XMFLOAT3(-1.0f, 0.0f,  1.0f), XMFLOAT3(-1.0f, 0.0f, 1.0f), XMFLOAT2(1,1)},
		{ XMFLOAT3(1.0f,  0.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f), XMFLOAT2(0,1) },
		{ XMFLOAT3(-1.0f, 0.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, -1.0f), XMFLOAT2(1,0) },
		{ XMFLOAT3(1.0f, 0.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, -1.0f), XMFLOAT2(0,0) },


		{ XMFLOAT3(0.0f, 3.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0,0) }, // point 4


	};

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 5;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = verticesPyramid;

	hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pPyramidVertexBuffer);


	if (FAILED(hr))
		return hr;

	return S_OK;
}

HRESULT Application::InitCubeIndexBuffer()
{
	HRESULT hr;

	// Create index buffer cube
	WORD indicesCube[] =
	{
		0,1,2,
		2,1,3, // front

		6,5,4,
		7,5,6, //back

		1,5,3,
		3,5,7, // right

		2,4,0,
		6,4,2, //left


		6,2,7, // bottom
		7,2,3,

		4,5,0,
		0,5,1, // top
	};


	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(WORD) * 36;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = indicesCube;
	hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pCubeIndexBuffer);




	if (FAILED(hr))
		return hr;


	return S_OK;
}

HRESULT Application::InitPyramidIndexBuffer()
{

	HRESULT hr;

	// index buffer pyramid

	WORD indicesPyramid[] =
	{

		0,2,1,
		1,2,3,

		0,1,4, // back
		1,3,4, // front

		3,2,4, // left
		2,0,4, //right

	};



	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(WORD) * 18;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = indicesPyramid;
	hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pPyramidIndexBuffer);

	if (FAILED(hr))
		return hr;


	return S_OK;
}

HRESULT Application::InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TUTORIAL1);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = L"TutorialWindowClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);
	if (!RegisterClassEx(&wcex))
		return E_FAIL;

	// Create window
	_hInst = hInstance;
	RECT rc = { 0, 0, 1920, 1080 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	_hWnd = CreateWindow(L"TutorialWindowClass", L"DX11 Framework", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
		nullptr);
	if (!_hWnd)
		return E_FAIL;

	ShowWindow(_hWnd, nCmdShow);

	return S_OK;
}

HRESULT Application::CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

	ID3DBlob* pErrorBlob;
	hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, ppBlobOut, &pErrorBlob);

	if (FAILED(hr))
	{
		if (pErrorBlob != nullptr)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());

		if (pErrorBlob) pErrorBlob->Release();

		return hr;
	}

	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}

HRESULT Application::InitDevice()
{
	HRESULT hr = S_OK;

	UINT createDeviceFlags = 0;

	D3D11_TEXTURE2D_DESC depthStencilDesc;

	depthStencilDesc.Width = _WindowWidth;
	depthStencilDesc.Height = _WindowHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;



#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};

	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};

	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = _WindowWidth;
	sd.BufferDesc.Height = _WindowHeight;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = _hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(nullptr, _driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &sd, &_pSwapChain, &_pd3dDevice, &_featureLevel, &_pImmediateContext);
		if (SUCCEEDED(hr))
			break;
	}

	if (FAILED(hr))
		return hr;



	// Create a render target view
	ID3D11Texture2D* pBackBuffer = nullptr;
	hr = _pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

	if (FAILED(hr))
		return hr;

	hr = _pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &_pRenderTargetView);
	pBackBuffer->Release();

	if (FAILED(hr))
		return hr;

	_pd3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &_depthStencilBuffer);

	_pd3dDevice->CreateDepthStencilView(_depthStencilBuffer, nullptr, &_depthStencilView);

	_pImmediateContext->OMSetRenderTargets(1, &_pRenderTargetView, _depthStencilView);




	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)_WindowWidth;
	vp.Height = (FLOAT)_WindowHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	_pImmediateContext->RSSetViewports(1, &vp);

	InitShadersAndInputLayout();

	//Call Vertex Buffers 
	InitCubeVertexBuffer();
	InitPyramidVertexBuffer();


	// Set vertex buffer
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	_pImmediateContext->IASetVertexBuffers(0, 1, &_pCubeVertexBuffer, &stride, &offset);
	_pImmediateContext->IASetVertexBuffers(0, 1, &_pPyramidVertexBuffer, &stride, &offset);
	_pImmediateContext->IASetVertexBuffers(0, 1, &_pPlaneVertexBuffer, &stride, &offset);

	InitCubeIndexBuffer();
	InitPyramidIndexBuffer();


	// Set index buffer
	_pImmediateContext->IASetIndexBuffer(_pCubeIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	_pImmediateContext->IASetIndexBuffer(_pPyramidVertexBuffer, DXGI_FORMAT_R16_UINT, 0);
	_pImmediateContext->IASetIndexBuffer(_pPlaneVertexBuffer, DXGI_FORMAT_R16_UINT, 0);
	// Set primitive topology
	_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Create the constant buffer
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = _pd3dDevice->CreateBuffer(&bd, nullptr, &_pConstantBuffer);
	
	//Lighting Values
	lightDirection = XMFLOAT3(0.25f, 0.5f, 1.0f);
	
	diffuseMaterial = XMFLOAT4(0.8f, 0.5f, 0.5f, 1.0f);
	diffuseLight = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	ambientMaterial = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	ambientLight = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);

	specularMaterial = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	specularLight = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	specularPower = 2.0f;
	eyePos = XMFLOAT4(0.0f, 0.0f, -10.0f, 0.0f);

	// Car Position
	speed = 0.5f;
	currentPosZ = 10.0f;
	currentPosX = 0.0f;
	rotationX = 0.0f;
	carPosition = XMFLOAT3(currentPosX,-10.0f,currentPosZ);

	// Rasterizer Structure For Wire Frame
	D3D11_RASTERIZER_DESC wfdesc;
	ZeroMemory(&wfdesc, sizeof(D3D11_RASTERIZER_DESC));
	wfdesc.FillMode = D3D11_FILL_WIREFRAME;
	wfdesc.CullMode = D3D11_CULL_NONE;
	hr = _pd3dDevice->CreateRasterizerState(&wfdesc, &_wireFrame);

	if (FAILED(hr))
		return hr;

	// Rasterizer Structure For Solid Frame
	D3D11_RASTERIZER_DESC wfdesc2;
	ZeroMemory(&wfdesc2, sizeof(D3D11_RASTERIZER_DESC));
	wfdesc2.FillMode = D3D11_FILL_SOLID;
	wfdesc2.CullMode = D3D11_CULL_NONE;
	hr = _pd3dDevice->CreateRasterizerState(&wfdesc2, &_solidFrame);

	if (FAILED(hr))
		return hr;

	//Texture Loading
	hr = CreateDDSTextureFromFile(_pd3dDevice, L"Hercules_COLOR.dds", nullptr, &_pTextureHercules);

		if (FAILED(hr))
			return hr;

	hr = CreateDDSTextureFromFile(_pd3dDevice, L"Crate_COLOR.dds", nullptr, &_pTextureCrate);

	if (FAILED(hr))
		return hr;

	hr = CreateDDSTextureFromFile(_pd3dDevice, L"Sun_COLOR.dds", nullptr, &_pTextureSun);

	if (FAILED(hr))
		return hr;

		D3D11_BLEND_DESC blendDesc;
		ZeroMemory(&blendDesc, sizeof(blendDesc));
		D3D11_RENDER_TARGET_BLEND_DESC rtbd;
		ZeroMemory(&rtbd, sizeof(rtbd));
		rtbd.BlendEnable = true;
		rtbd.SrcBlend = D3D11_BLEND_SRC_COLOR;
		rtbd.DestBlend = D3D11_BLEND_BLEND_FACTOR;
		rtbd.BlendOp = D3D11_BLEND_OP_ADD;
		rtbd.SrcBlendAlpha = D3D11_BLEND_ONE;
		rtbd.DestBlendAlpha = D3D11_BLEND_ZERO;
		rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		rtbd.RenderTargetWriteMask = D3D10_COLOR_WRITE_ENABLE_ALL;
		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.RenderTarget[0] = rtbd;
		_pd3dDevice->CreateBlendState(&blendDesc, &Transparency);

		if (FAILED(hr))
			return hr;

	return S_OK;
}



void Application::Cleanup()
{
	if (_pImmediateContext) _pImmediateContext->ClearState();

	if (_pConstantBuffer) _pConstantBuffer->Release();
	if (_pCubeVertexBuffer) _pCubeVertexBuffer->Release();
	if (_pCubeIndexBuffer) _pCubeIndexBuffer->Release();
	if (_pPyramidVertexBuffer) _pPyramidVertexBuffer->Release();
	if (_pPyramidIndexBuffer) _pPyramidIndexBuffer->Release();
	if (_pPlaneVertexBuffer) _pPlaneVertexBuffer->Release();
	if (_pPlaneIndexBuffer) _pPlaneIndexBuffer->Release();
	if (_pgridVertexBuffer) _pgridVertexBuffer->Release();
	if (_pgridIndexBuffer) _pgridIndexBuffer->Release();
	if (_pVertexLayout) _pVertexLayout->Release();
	if (_pVertexShader) _pVertexShader->Release();
	if (_pPixelShader) _pPixelShader->Release();
	if (_pRenderTargetView) _pRenderTargetView->Release();
	if (_pSwapChain) _pSwapChain->Release();
	if (_pImmediateContext) _pImmediateContext->Release();
	if (_pd3dDevice) _pd3dDevice->Release();
	if (_depthStencilView) _depthStencilView->Release();
	if (_depthStencilBuffer) _depthStencilBuffer->Release();
	if (_wireFrame) _wireFrame->Release();
	if (_solidFrame) _solidFrame->Release();
	if (Transparency) Transparency->Release();
		
	//Continue Solid Objects
}

void Application::Update()
{
	// Update our time

	static float t = 0.0f;

	if (_driverType == D3D_DRIVER_TYPE_REFERENCE)
	{
		t += (float)XM_PI * 0.0125f;
	}
	else
	{
		static DWORD dwTimeStart = 0;
		DWORD dwTimeCur = GetTickCount();

		if (dwTimeStart == 0)
			dwTimeStart = dwTimeCur;

		t = (dwTimeCur - dwTimeStart) / 1000.0f;

	}

	gTime = t;


	_currentCamera->Update();

	if (GetAsyncKeyState('W'))
	{
		currentPosZ += 0.002f * cos(rotationX);
		currentPosX += 0.002f  * sin(rotationX);
	}
	if (GetAsyncKeyState('S'))
	{
		currentPosZ -= 0.002f * cos(rotationX);
		currentPosX -= 0.002f * sin(rotationX);
	}
	if (GetAsyncKeyState('A'))
	{
		rotationX -= 0.0002f;
	}
	if (GetAsyncKeyState('D'))
	{
		rotationX += 0.0002f;
	}
	_carCamera->SetPosition(XMFLOAT3(currentPosX - sin(rotationX), 1.4, currentPosZ - cos(rotationX)));
	_carCamera->SetLookAt(XMFLOAT3(currentPosX,0.0f, currentPosZ));
	_carCamera->SetView();

	XMMATRIX translation;
	XMMATRIX rotation;
	translation = XMMatrixIdentity();
	translation	= XMMatrixTranslation(currentPosX, carPosition.y,currentPosZ);
	rotation = XMMatrixRotationY(rotationX);

	//direction.x = 1 * cos(rotationAngle);
	//direction.z = 1 * sin(rotationAngle);

	XMStoreFloat4x4(&sphere, XMMatrixRotationX(t) * XMMatrixRotationY(t) * XMMatrixTranslation(0.0f, 0.0f, 0.0f)); // Sphere
	XMStoreFloat4x4(&pyramid, XMMatrixTranslation(5.0f, 0.0f, 0.0f) * XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixRotationY(t)); // Pyramid
	XMStoreFloat4x4(&cube, XMMatrixTranslation(-5.0f, 8.0f, 0.0f) * XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixRotationZ(t)); // Cube
	XMStoreFloat4x4(&hercules, XMMatrixTranslation(0.0f, 0.0f, 20.0f)); // Hercules Plane
	XMStoreFloat4x4(&car, XMMatrixScaling(0.1f, 0.1f, 0.1f) * rotation * translation); // Car
	XMStoreFloat4x4(&terrain, XMMatrixTranslation(0.0f,-12.0f,0.0f)); // Car

	if (GetAsyncKeyState('3'))
	{
		_currentCamera = _camera1;
	}
	if (GetAsyncKeyState('4'))
	{
		_currentCamera = _camera2;
	}	
	if (GetAsyncKeyState('5'))
	{
		_currentCamera = _carCamera;
	}

	_currentCamera->SetView();
	_currentCamera->SetProjection();

	if (GetAsyncKeyState('6'))
	{
		isTransparent = true;
	}
	if (GetAsyncKeyState('7'))
	{
		isTransparent = false;
	}


}

void Application::Draw()
{
	//
	// Clear the back buffer
	//
	float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; // red,green,blue,alpha
	_pImmediateContext->ClearRenderTargetView(_pRenderTargetView, ClearColor);
	_pImmediateContext->ClearDepthStencilView(_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	XMMATRIX world = XMLoadFloat4x4(&sphere);
	XMMATRIX view = XMLoadFloat4x4(_currentCamera->GetView());
	XMMATRIX projection = XMLoadFloat4x4(_currentCamera->GetProjection());
	//
	// Update variables
	//
	ConstantBuffer cb;



	if (GetAsyncKeyState(0x31))
	{
		_pImmediateContext->RSSetState(_solidFrame);
	}
	else if (GetAsyncKeyState(0x32))
	{
		_pImmediateContext->RSSetState(_wireFrame);
	}


	cb.mWorld = XMMatrixTranspose(world);
	cb.mView = XMMatrixTranspose(view);
	cb.mProjection = XMMatrixTranspose(projection);

	cb.LightVecw = lightDirection;
	cb.DiffuseLight = diffuseLight;
	cb.DiffuseMtrl = diffuseMaterial;
	cb.AmbientLight = ambientLight;
	cb.AmbientMtrl = ambientMaterial;
	cb.SpecularLight = specularLight;
	cb.SpecularMtrl = specularMaterial;
	cb.SpecularPower = specularPower;

	cb.gTime = gTime;

	// Set vertex buffer
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;

	_pImmediateContext->IASetVertexBuffers(0, 1, &objSphere.VertexBuffer, &stride, &offset);
	_pImmediateContext->IASetIndexBuffer(objSphere.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);

	if (isTransparent == false)
	{
		float blendFactor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		_pImmediateContext->OMSetBlendState(Transparency, blendFactor, 0xffffffff);
	}
	if (isTransparent == true)
	{
		float blendFactor2[] = { 0.9f, 0.9f, 0.9f, 1.0f };
		_pImmediateContext->OMSetBlendState(Transparency, blendFactor2, 0xffffffff);
	}
	//
	// Renders a triangle
	//
	_pImmediateContext->PSSetShaderResources(0, 1, &_pTextureSun);
	_pImmediateContext->PSSetSamplers(0, 1, &_pSamplerState);
	_pImmediateContext->VSSetShader(_pVertexShader, nullptr, 0);
	_pImmediateContext->VSSetConstantBuffers(0, 1, &_pConstantBuffer);
	_pImmediateContext->PSSetConstantBuffers(0, 1, &_pConstantBuffer);
	_pImmediateContext->PSSetShader(_pPixelShader, nullptr, 0);
	_pImmediateContext->DrawIndexed(objSphere.IndexCount, 0, 0);


	// Pyramid
	_pImmediateContext->PSSetShaderResources(0, 1, &_pTextureCrate);
	_pImmediateContext->PSSetSamplers(0, 1, &_pSamplerState);
	_pImmediateContext->IASetVertexBuffers(0, 1, &_pPyramidVertexBuffer, &stride, &offset);
	_pImmediateContext->IASetIndexBuffer(_pPyramidIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);

	world = XMLoadFloat4x4(&pyramid);
	cb.mWorld = XMMatrixTranspose(world);
	_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);
	_pImmediateContext->DrawIndexed(18, 0, 0);

	//Cube
	_pImmediateContext->PSSetShaderResources(0, 1, &_pTextureCrate);
	_pImmediateContext->PSSetSamplers(0, 1, &_pSamplerState);
	_pImmediateContext->IASetVertexBuffers(0, 1, &_pCubeVertexBuffer, &stride, &offset);
	_pImmediateContext->IASetIndexBuffer(_pCubeIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	world = XMLoadFloat4x4(&cube);
	cb.mWorld = XMMatrixTranspose(world);
	_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);
	_pImmediateContext->DrawIndexed(36, 0, 0);

	// Hercules Plane
	_pImmediateContext->PSSetShaderResources(0, 1, &_pTextureHercules);
	_pImmediateContext->PSSetSamplers(0, 1, &_pSamplerState);
	_pImmediateContext->IASetVertexBuffers(0, 1, &objPlane.VertexBuffer, &stride, &offset);
	_pImmediateContext->IASetIndexBuffer(objPlane.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	world = XMLoadFloat4x4(&hercules);
	cb.mWorld = XMMatrixTranspose(world);
	_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);
	_pImmediateContext->DrawIndexed(objPlane.IndexCount, 0, 0);

	//Car
	_pImmediateContext->PSSetShaderResources(0, 1, &_pTextureCrate);
	_pImmediateContext->PSSetSamplers(0, 1, &_pSamplerState);
	_pImmediateContext->IASetVertexBuffers(0, 1, &objCar.VertexBuffer, &stride, &offset);
	_pImmediateContext->IASetIndexBuffer(objCar.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	world = XMLoadFloat4x4(&car);
	cb.mWorld = XMMatrixTranspose(world);
	_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);
	_pImmediateContext->DrawIndexed(objCar.IndexCount, 0, 0);

	//Terrain
	_pImmediateContext->IASetVertexBuffers(0, 1, &_pgridVertexBuffer, &stride, &offset);
	_pImmediateContext->IASetIndexBuffer(_pgridIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	world = XMLoadFloat4x4(&terrain);
	cb.mWorld = XMMatrixTranspose(world);
	_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);
	_pImmediateContext->DrawIndexed(m_totalFaces, 0, 0);

	// Present our back buffer to our front buffer
	//
	_pSwapChain->Present(0, 0);
}

HRESULT Application::CreatGrid(float rows, float columns, float width, float depth, char* filename)
{
	HRESULT hr;


	//std::ifstream inFile;
	FILE* fileptr;
	BITMAPFILEHEADER bitmapFileHeader;
	BITMAPINFOHEADER bitmapInfoHeader;

	int imageSize, index;
	unsigned char height;

	//inFile.open(filename);
	fileptr = fopen(filename, "rb");

	//inFile.read((char*)&bitmapFileHeader, sizeof(BITMAPFILEHEADER));
	//inFile.read((char*)&bitmapInfoHeader, sizeof(BITMAPINFOHEADER));

	fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, fileptr);
	fread(&bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, fileptr);

	terrainWidth = bitmapInfoHeader.biWidth;
	terrainHeight = bitmapInfoHeader.biHeight;

	imageSize = terrainWidth * terrainHeight * 3;

	unsigned char* bitmapImage = new unsigned char[imageSize];

	//inFile.seekg(bitmapFileHeader.bfOffBits, SEEK_SET);
	fseek(fileptr, bitmapFileHeader.bfOffBits, SEEK_SET);

	//inFile.read((char*)bitmapImage, imageSize);
	fread(bitmapImage, 1, imageSize, fileptr);

	//inFile.close();
	fclose(fileptr);

	heightMap = new XMFLOAT3[terrainWidth * terrainHeight];

	int p = 0;

	float heightFactor = 20.0f;

	for (int j = 0; j < terrainHeight; j++)
	{
		for (int i = 0; i < terrainWidth; i++)
		{
			height = bitmapImage[p];

			index = (terrainHeight * j) + i;

			heightMap[index].x = (float)i;
			heightMap[index].y = (float)height / heightFactor;
			heightMap[index].z = (float)j;

			p += 3;
		}
	}

	delete[] bitmapImage;
	bitmapImage = 0;

	//Application::LoadHeightMap("Heightmap.BMP");

	m_rows = rows;
	m_columns = columns;

	m_depth = depth;
	m_width = width;

	m_rows = m_rows - 1;
	m_columns = m_columns - 1;
	m_totalCells = (m_rows - 1) * (m_columns - 1);
	m_totalFaces = (m_rows - 1) * (m_columns - 1) * 2 * 6;
	m_totalVertices = m_rows * m_columns;

	dx = m_width / (m_columns - 1);
	dz = m_depth / (m_rows - 1);

	du = 1.0f / (m_columns - 1);
	dv = 1.0f / (m_rows - 1);

	std::vector<SimpleVertex> v(m_totalVertices);

	// Vertex Generation
	for (int i = 0; i < m_rows; i++)
	{
		for (int j = 0; j < m_columns; j++)
		{
			//v[i * m_columns + j].Pos = XMFLOAT3((-0.5 * m_width) + (float)j * dx, heightMap[i*m_columns+j].y, (0.5 * m_depth) - (float)i * dz);
			v[i * m_columns + j].Pos = heightMap[i * m_columns + j];
			v[i * m_columns + j].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);

			v[i * m_columns + j].TexC.x = j * du;
			v[i * m_columns + j].TexC.y = i * dv;

		}
	}

	std::vector<WORD> indices(m_totalFaces * 3);

	int k = 0;
	// Index Generation
	for (UINT i = 0; i < m_rows - 1; i++)
	{
		for (UINT j = 0; j < m_columns -1; j++)
		{
			indices[k] = i * m_columns + j;
			indices[k + 1] = i * m_columns + j + 1;
			indices[k + 2] = (i + 1)* m_columns + j;

			indices[k + 3] = (i + 1) * m_columns + j;
			indices[k + 4] = i * m_columns + j + 1;
			indices[k + 5] = (i + 1) * m_columns + j + 1;
			
			k += 6;
		}
	}

	// Vertex Buffer
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * v.size();
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = &v[0];
	hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pgridVertexBuffer);

	if (FAILED(hr))
		return hr;

	// Index Buffer
	D3D11_BUFFER_DESC bd2;
	ZeroMemory(&bd2, sizeof(bd2));

	bd2.Usage = D3D11_USAGE_DEFAULT;
	bd2.ByteWidth = sizeof(WORD) * indices.size();
	bd2.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd2.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData2;
	ZeroMemory(&InitData2, sizeof(InitData2));
	InitData2.pSysMem = &indices[0];
	hr = _pd3dDevice->CreateBuffer(&bd2, &InitData2, &_pgridIndexBuffer);

	if (FAILED(hr))
		return hr;

	//Normals

	//Now we will compute the normals for each vertex using normal averaging
	std::vector<XMFLOAT3> tempNormal;

	//normalized and unnormalized normals
	XMFLOAT3 unnormalized = XMFLOAT3(0.0f, 0.0f, 0.0f);

	//Used to get vectors (sides) from the position of the verts
	float vecX, vecY, vecZ;

	//Two edges of our triangle
	XMVECTOR edge1 = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR edge2 = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

	//Compute face normals
	for (int i = 0; i < m_totalFaces; ++i)
	{
		//Get the vector describing one edge of our triangle (edge 0,2)
		vecX = v[indices[(i * 3)]].Pos.x - v[indices[(i * 3) + 2]].Pos.x;
		vecY = v[indices[(i * 3)]].Pos.y - v[indices[(i * 3) + 2]].Pos.y;
		vecZ = v[indices[(i * 3)]].Pos.z - v[indices[(i * 3) + 2]].Pos.z;
		edge1 = XMVectorSet(vecX, vecY, vecZ, 0.0f);    //Create our first edge

		//Get the vector describing another edge of our triangle (edge 2,1)
		vecX = v[indices[(i * 3) + 2]].Pos.x - v[indices[(i * 3) + 1]].Pos.x;
		vecY = v[indices[(i * 3) + 2]].Pos.y - v[indices[(i * 3) + 1]].Pos.y;
		vecZ = v[indices[(i * 3) + 2]].Pos.z - v[indices[(i * 3) + 1]].Pos.z;
		edge2 = XMVectorSet(vecX, vecY, vecZ, 0.0f);    //Create our second edge

		//Cross multiply the two edge vectors to get the un-normalized face normal
		XMStoreFloat3(&unnormalized, XMVector3Cross(edge1, edge2));
		tempNormal.push_back(unnormalized);            //Save unormalized normal (for normal averaging)
	}

	//Compute vertex normals (normal Averaging)
	XMVECTOR normalSum = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	int facesUsing = 0;
	float tX;
	float tY;
	float tZ;

	//Go through each vertex
	for (int i = 0; i < m_totalVertices; ++i)
	{
		//Check which triangles use this vertex
		for (int j = 0; j < m_totalFaces; ++j)
		{
			if (indices[j * 3] == i ||
				indices[(j * 3) + 1] == i ||
				indices[(j * 3) + 2] == i)
			{
				tX = XMVectorGetX(normalSum) + tempNormal[j].x;
				tY = XMVectorGetY(normalSum) + tempNormal[j].y;
				tZ = XMVectorGetZ(normalSum) + tempNormal[j].z;

				normalSum = XMVectorSet(tX, tY, tZ, 0.0f);    //If a face is using the vertex, add the unormalized face normal to the normalSum
				facesUsing++;
			}
		}

		//Get the actual normal by dividing the normalSum by the number of faces sharing the vertex
		normalSum = normalSum / facesUsing;

		//Normalize the normalSum vector
		normalSum = XMVector3Normalize(normalSum);

		//Store the normal in our current vertex
		v[i].Normal.x = XMVectorGetX(normalSum);
		v[i].Normal.y = XMVectorGetY(normalSum);
		v[i].Normal.z = XMVectorGetZ(normalSum);

		//Clear normalSum and facesUsing for next vertex
		normalSum = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
		facesUsing = 0;
	}


	return hr;
}

//bool Application::LoadHeightMap(char* filename)
//{
//	//std::ifstream inFile;
//	FILE* fileptr;
//	BITMAPFILEHEADER bitmapFileHeader;
//	BITMAPINFOHEADER bitmapInfoHeader;
//
//	int imageSize, index;
//	unsigned char height;
//
//	//inFile.open(filename);
//	fileptr = fopen(filename, "rb");
//
//	//inFile.read((char*)&bitmapFileHeader, sizeof(BITMAPFILEHEADER));
//	//inFile.read((char*)&bitmapInfoHeader, sizeof(BITMAPINFOHEADER));
//
//	fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, fileptr);
//	fread(&bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, fileptr);
//
//	terrainWidth = bitmapInfoHeader.biWidth;
//	terrainHeight = bitmapInfoHeader.biHeight;
//
//	imageSize = terrainWidth * terrainHeight * 3;
//
//	unsigned char* bitmapImage = new unsigned char[imageSize];
//
//	//inFile.seekg(bitmapFileHeader.bfOffBits, SEEK_SET);
//	fseek(fileptr, bitmapFileHeader.bfOffBits, SEEK_SET);
//
//	//inFile.read((char*)bitmapImage, imageSize);
//	fread(bitmapImage, 1, imageSize, fileptr);
//
//	//inFile.close();
//	fclose(fileptr);
//
//	heightMap = new XMFLOAT3[terrainWidth * terrainHeight];
//
//	int p = 0;
//
//	float heightFactor = 20.0f;
//
//	for (int j = 0; j < terrainHeight; j++)
//	{
//		for (int i = 0; i < terrainWidth; i++)
//		{
//			height = bitmapImage[p];
//
//			index = (terrainHeight * j) + i;
//
//			heightMap[index].x = (float)i;
//			heightMap[index].y = (float)height / heightFactor;
//			heightMap[index].z = (float)j;
//
//			p += 3;
//		}
//	}
//
//	delete[] bitmapImage;
//	bitmapImage = 0;
//
//	return true;
//}


