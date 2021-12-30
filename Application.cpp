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
	hInst = nullptr;
	hWnd = nullptr;
	driverType = D3D_DRIVER_TYPE_NULL;
	featureLevel = D3D_FEATURE_LEVEL_11_0;
	pd3dDevice = nullptr;
	pImmediateContext = nullptr;
	pSwapChain = nullptr;
	pRenderTargetView = nullptr;
	pVertexShader = nullptr;
	pPixelShader = nullptr;
	pVertexLayout = nullptr;
	pCubeVertexBuffer = nullptr;
	pPyramidVertexBuffer = nullptr;
	pPlaneVertexBuffer = nullptr;
	pCubeIndexBuffer = nullptr;
	pPyramidIndexBuffer = nullptr;
	pPlaneIndexBuffer = nullptr;
	pConstantBuffer = nullptr;
	
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
	GetClientRect(hWnd, &rc);
	_WindowWidth = rc.right - rc.left;
	_WindowHeight = rc.bottom - rc.top;

	if (FAILED(InitDevice()))
	{
		Cleanup();

		return E_FAIL;
	}

	// Initialize the world matrix
	XMStoreFloat4x4(&sphere, XMMatrixIdentity());

	pCamera1 = new Camera(XMFLOAT3(0.0f, 10.0f, -20.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), _WindowWidth, _WindowHeight, 0.01f, 10000.0f);
	pCamera2 = new Camera(XMFLOAT3(0.0f, 10.0f, 20.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), _WindowWidth, _WindowHeight, 0.01f, 10000.0f);
	pCarCamera = new Camera(XMFLOAT3(0.0f, 10.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), _WindowWidth, _WindowHeight, 0.01f, 10000.0f);
	pTopDownCamera = new Camera(XMFLOAT3(0.0f, 30.0f, 10.0f), XMFLOAT3(0.0f, -60.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), _WindowWidth, _WindowHeight, 0.01f, 10000.0f);
	pCurrentCamera = pCamera1;
	pCurrentCamera->SetView();
	pCurrentCamera->SetProjection();

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
	pd3dDevice->CreateSamplerState(&sampDesc, &pSamplerState);

	objPlane = OBJLoader::Load("Hercules.obj", pd3dDevice);
	objCar = OBJLoader::Load("car.obj", pd3dDevice);
	objSphere = OBJLoader::Load("sphere.obj", pd3dDevice, false);

	//Application::HeightMapLoad("Heightmap.bmp");
	Application::CreateTerrain("Heightmap.bmp");

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
	hr = pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &pVertexShader);

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
	hr = pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &pPixelShader);
	pPSBlob->Release();

	if (FAILED(hr))
		return hr;

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &pVertexLayout);
	pVSBlob->Release();

	if (FAILED(hr))
		return hr;

	// Set the input layout
	pImmediateContext->IASetInputLayout(pVertexLayout);

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

	hr = pd3dDevice->CreateBuffer(&bd, &InitData, &pCubeVertexBuffer);


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

	hr = pd3dDevice->CreateBuffer(&bd, &InitData, &pPyramidVertexBuffer);


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
	hr = pd3dDevice->CreateBuffer(&bd, &InitData, &pCubeIndexBuffer);




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

		0,2,1, // Base Square
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
	hr = pd3dDevice->CreateBuffer(&bd, &InitData, &pPyramidIndexBuffer);

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
	hInst = hInstance;
	RECT rc = { 0, 0, 1920, 1080 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	hWnd = CreateWindow(L"TutorialWindowClass", L"DX11 Framework", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
		nullptr);
	if (!hWnd)
		return E_FAIL;

	ShowWindow(hWnd, nCmdShow);

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
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(nullptr, driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &sd, &pSwapChain, &pd3dDevice, &featureLevel, &pImmediateContext);
		if (SUCCEEDED(hr))
			break;
	}

	if (FAILED(hr))
		return hr;



	// Create a render target view
	ID3D11Texture2D* pBackBuffer = nullptr;
	hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

	if (FAILED(hr))
		return hr;

	hr = pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTargetView);
	pBackBuffer->Release();

	if (FAILED(hr))
		return hr;

	pd3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &depthStencilBuffer);
	pd3dDevice->CreateDepthStencilView(depthStencilBuffer, nullptr, &depthStencilView);
	pImmediateContext->OMSetRenderTargets(1, &pRenderTargetView, depthStencilView);




	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)_WindowWidth;
	vp.Height = (FLOAT)_WindowHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	pImmediateContext->RSSetViewports(1, &vp);

	InitShadersAndInputLayout();

	//Call Vertex Buffers 
	InitCubeVertexBuffer();
	InitPyramidVertexBuffer();


	// Set vertex buffer
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	pImmediateContext->IASetVertexBuffers(0, 1, &pCubeVertexBuffer, &stride, &offset);
	pImmediateContext->IASetVertexBuffers(0, 1, &pPyramidVertexBuffer, &stride, &offset);
	pImmediateContext->IASetVertexBuffers(0, 1, &pPlaneVertexBuffer, &stride, &offset);

	InitCubeIndexBuffer();
	InitPyramidIndexBuffer();


	// Set index buffer
	pImmediateContext->IASetIndexBuffer(pCubeIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	pImmediateContext->IASetIndexBuffer(pPyramidVertexBuffer, DXGI_FORMAT_R16_UINT, 0);
	pImmediateContext->IASetIndexBuffer(pPlaneVertexBuffer, DXGI_FORMAT_R16_UINT, 0);
	// Set primitive topology
	pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Create the constant buffer
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = pd3dDevice->CreateBuffer(&bd, nullptr, &pConstantBuffer);
	
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
	carPosition = XMFLOAT3(currentPosX,-5.0f,currentPosZ);

	// Rasterizer Structure For Wire Frame
	D3D11_RASTERIZER_DESC wfdesc;
	ZeroMemory(&wfdesc, sizeof(D3D11_RASTERIZER_DESC));
	wfdesc.FillMode = D3D11_FILL_WIREFRAME;
	wfdesc.CullMode = D3D11_CULL_NONE;
	hr = pd3dDevice->CreateRasterizerState(&wfdesc, &wireFrame);

	if (FAILED(hr))
		return hr;

	// Rasterizer Structure For Solid Frame
	D3D11_RASTERIZER_DESC wfdesc2;
	ZeroMemory(&wfdesc2, sizeof(D3D11_RASTERIZER_DESC));
	wfdesc2.FillMode = D3D11_FILL_SOLID;
	wfdesc2.CullMode = D3D11_CULL_BACK;
	hr = pd3dDevice->CreateRasterizerState(&wfdesc2, &solidFrame);

	if (FAILED(hr))
		return hr;

	//Texture Loading
	hr = CreateDDSTextureFromFile(pd3dDevice, L"Hercules_COLOR.dds", nullptr, &pTextureHercules);
	hr = CreateDDSTextureFromFile(pd3dDevice, L"Crate_COLOR.dds", nullptr, &pTextureCrate);
	hr = CreateDDSTextureFromFile(pd3dDevice, L"Sun_COLOR.dds", nullptr, &pTextureSun);
	hr = CreateDDSTextureFromFile(pd3dDevice, L"Mud_COLOR.dds", nullptr, &pTextureMud);
	hr = CreateDDSTextureFromFile(pd3dDevice, L"Surface_COLOR.dds", nullptr, &pTextureSurface);

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
		pd3dDevice->CreateBlendState(&blendDesc, &Transparency);

		if (FAILED(hr))
			return hr;

	return S_OK;
}



void Application::Cleanup()
{
	if (pImmediateContext) pImmediateContext->ClearState();

	if (pConstantBuffer) pConstantBuffer->Release();
	if (pCubeVertexBuffer) pCubeVertexBuffer->Release();
	if (pCubeIndexBuffer) pCubeIndexBuffer->Release();
	if (pPyramidVertexBuffer) pPyramidVertexBuffer->Release();
	if (pPyramidIndexBuffer) pPyramidIndexBuffer->Release();
	if (pPlaneVertexBuffer) pPlaneVertexBuffer->Release();
	if (pPlaneIndexBuffer) pPlaneIndexBuffer->Release();
	if (pgridVertexBuffer) pgridVertexBuffer->Release();
	if (pgridIndexBuffer) pgridIndexBuffer->Release();
	if (pVertexLayout) pVertexLayout->Release();
	if (pVertexShader) pVertexShader->Release();
	if (pPixelShader) pPixelShader->Release();
	if (pRenderTargetView) pRenderTargetView->Release();
	if (pSwapChain) pSwapChain->Release();
	if (pImmediateContext) pImmediateContext->Release();
	if (pd3dDevice) pd3dDevice->Release();
	if (depthStencilView) depthStencilView->Release();
	if (depthStencilBuffer) depthStencilBuffer->Release();
	if (wireFrame) wireFrame->Release();
	if (solidFrame) solidFrame->Release();
	if (Transparency) Transparency->Release();
		
	//Continue Solid Objects
}

void Application::Update()
{
	// Update our time

	static float t = 0.0f;

	if (driverType == D3D_DRIVER_TYPE_REFERENCE)
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


	pCurrentCamera->Update();

	int speed = 0;

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

	if (GetAsyncKeyState('W') && GetAsyncKeyState(0x20))
	{
		currentPosZ += 0.012f * cos(rotationX);
		currentPosX += 0.012f * sin(rotationX);
	}
	if (GetAsyncKeyState('D') && GetAsyncKeyState(0x20))
	{
		rotationX += 0.0012f;
	}
	if (GetAsyncKeyState('A') && GetAsyncKeyState(0x20))
	{
		rotationX -= 0.0012f;
	}
	if (GetAsyncKeyState('S') && GetAsyncKeyState(0x20))
	{
		currentPosZ -= 0.012f * cos(rotationX);
		currentPosX -= 0.012f * sin(rotationX);
	}


	pCarCamera->SetPosition(XMFLOAT3(currentPosX - sin(rotationX), 3, currentPosZ - cos(rotationX)));
	pCarCamera->SetLookAt(XMFLOAT3(currentPosX,3.0f, currentPosZ));
	pCarCamera->SetView();



	XMMATRIX translation;
	XMMATRIX rotation;
	translation = XMMatrixIdentity();
	translation	= XMMatrixTranslation(currentPosX, carPosition.y,currentPosZ);
	rotation = XMMatrixRotationY(rotationX);

	XMStoreFloat4x4(&sphere, XMMatrixRotationX(t) * XMMatrixRotationY(t) * XMMatrixTranslation(0.0f, 0.0f, 0.0f)); // Sphere
	XMStoreFloat4x4(&pyramid, XMMatrixTranslation(5.0f, 0.0f, 0.0f) * XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixRotationY(t)); // Pyramid
	XMStoreFloat4x4(&cube, XMMatrixTranslation(-5.0f, 8.0f, 0.0f) * XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixRotationZ(t)); // Cube
	XMStoreFloat4x4(&hercules, XMMatrixTranslation(0.0f, 0.0f, 20.0f)); // Hercules Plane
	XMStoreFloat4x4(&car, XMMatrixScaling(0.1f, 0.1f, 0.1f) * rotation * translation); // Car
	XMStoreFloat4x4(&terrain, XMMatrixTranslation(0.0f,-12.0f,0.0f)); // Car

	if (GetAsyncKeyState('3'))
	{
		pCurrentCamera = pCamera1;
	}
	if (GetAsyncKeyState('4'))
	{
		pCurrentCamera = pCamera2;
	}	
	if (GetAsyncKeyState('5'))
	{
		pCurrentCamera = pCarCamera;
	}
	if (GetAsyncKeyState('8'))
	{
		pCurrentCamera = pTopDownCamera;
	}
	pCurrentCamera->SetView();
	pCurrentCamera->SetProjection();

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
	pImmediateContext->ClearRenderTargetView(pRenderTargetView, ClearColor);
	pImmediateContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	XMMATRIX world = XMLoadFloat4x4(&sphere);
	XMMATRIX view = XMLoadFloat4x4(pCurrentCamera->GetView());
	XMMATRIX projection = XMLoadFloat4x4(pCurrentCamera->GetProjection());
	//
	// Update variables
	//
	ConstantBuffer cb;

	//Wireframe Modes

	if (GetAsyncKeyState(0x31))
	{
		pImmediateContext->RSSetState(solidFrame);
	}
	else if (GetAsyncKeyState(0x32))
	{
		pImmediateContext->RSSetState(wireFrame);
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

	pImmediateContext->IASetVertexBuffers(0, 1, &objSphere.VertexBuffer, &stride, &offset);
	pImmediateContext->IASetIndexBuffer(objSphere.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	pImmediateContext->UpdateSubresource(pConstantBuffer, 0, nullptr, &cb, 0, 0);

	if (isTransparent == false)
	{
		float blendFactor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		pImmediateContext->OMSetBlendState(Transparency, blendFactor, 0xffffffff);
	}
	if (isTransparent == true)
	{
		float blendFactor2[] = { 0.9f, 0.9f, 0.9f, 1.0f };
		pImmediateContext->OMSetBlendState(Transparency, blendFactor2, 0xffffffff);
	}
	//
	// Renders a triangle
	//
	pImmediateContext->PSSetShaderResources(0, 1, &pTextureSun);
	pImmediateContext->PSSetSamplers(0, 1, &pSamplerState);
	pImmediateContext->VSSetShader(pVertexShader, nullptr, 0);
	pImmediateContext->VSSetConstantBuffers(0, 1, &pConstantBuffer);
	pImmediateContext->PSSetConstantBuffers(0, 1, &pConstantBuffer);
	pImmediateContext->PSSetShader(pPixelShader, nullptr, 0);
	pImmediateContext->DrawIndexed(objSphere.IndexCount, 0, 0);


	// Pyramid
	pImmediateContext->PSSetShaderResources(0, 1, &pTextureCrate);
	pImmediateContext->PSSetSamplers(0, 1, &pSamplerState);
	pImmediateContext->IASetVertexBuffers(0, 1, &pPyramidVertexBuffer, &stride, &offset);
	pImmediateContext->IASetIndexBuffer(pPyramidIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	pImmediateContext->UpdateSubresource(pConstantBuffer, 0, nullptr, &cb, 0, 0);

	world = XMLoadFloat4x4(&pyramid);
	cb.mWorld = XMMatrixTranspose(world);
	pImmediateContext->UpdateSubresource(pConstantBuffer, 0, nullptr, &cb, 0, 0);
	pImmediateContext->DrawIndexed(18, 0, 0);

	//Cube
	pImmediateContext->PSSetShaderResources(0, 1, &pTextureCrate);
	pImmediateContext->PSSetSamplers(0, 1, &pSamplerState);
	pImmediateContext->IASetVertexBuffers(0, 1, &pCubeVertexBuffer, &stride, &offset);
	pImmediateContext->IASetIndexBuffer(pCubeIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	world = XMLoadFloat4x4(&cube);
	cb.mWorld = XMMatrixTranspose(world);
	pImmediateContext->UpdateSubresource(pConstantBuffer, 0, nullptr, &cb, 0, 0);
	pImmediateContext->DrawIndexed(36, 0, 0);

	// Hercules Plane
	pImmediateContext->PSSetShaderResources(0, 1, &pTextureHercules);
	pImmediateContext->PSSetSamplers(0, 1, &pSamplerState);
	pImmediateContext->IASetVertexBuffers(0, 1, &objPlane.VertexBuffer, &stride, &offset);
	pImmediateContext->IASetIndexBuffer(objPlane.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	world = XMLoadFloat4x4(&hercules);
	cb.mWorld = XMMatrixTranspose(world);
	pImmediateContext->UpdateSubresource(pConstantBuffer, 0, nullptr, &cb, 0, 0);
	pImmediateContext->DrawIndexed(objPlane.IndexCount, 0, 0);

	//Car
	pImmediateContext->PSSetShaderResources(0, 1, &pTextureCrate);
	pImmediateContext->PSSetSamplers(0, 1, &pSamplerState);
	pImmediateContext->IASetVertexBuffers(0, 1, &objCar.VertexBuffer, &stride, &offset);
	pImmediateContext->IASetIndexBuffer(objCar.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	world = XMLoadFloat4x4(&car);
	cb.mWorld = XMMatrixTranspose(world);
	pImmediateContext->UpdateSubresource(pConstantBuffer, 0, nullptr, &cb, 0, 0);
	pImmediateContext->DrawIndexed(objCar.IndexCount, 0, 0);

	//Terrain
	pImmediateContext->PSSetShaderResources(0, 1, &pTextureMud);
	pImmediateContext->PSSetSamplers(0, 1, &pSamplerState);
	pImmediateContext->IASetVertexBuffers(0, 1, &pgridVertexBuffer, &stride, &offset);
	pImmediateContext->IASetIndexBuffer(pgridIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	world = XMLoadFloat4x4(&terrain);
	cb.mWorld = XMMatrixTranspose(world);
	pImmediateContext->UpdateSubresource(pConstantBuffer, 0, nullptr, &cb, 0, 0);
	pImmediateContext->DrawIndexed(totalFaces, 0, 0);

	// Present our back buffer to our front buffer
	//
	pSwapChain->Present(0, 0);
}

HRESULT Application::CreateTerrain(char* filename)
{
	HRESULT hr;

	//Bitmap Loading

	FILE* fileptr;
	BITMAPFILEHEADER bitmapFileHeader;
	BITMAPINFOHEADER bitmapInfoHeader;

	int imageSize, index;
	unsigned char height;

	//Open The File
	fileptr = fopen(filename, "rb");

	// Read the size of the bitmap
	fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, fileptr);
	fread(&bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, fileptr);

	//Set the terrain width and height the size of the image loading in.
	terrainWidth = bitmapInfoHeader.biWidth;
	terrainHeight = bitmapInfoHeader.biHeight;

	imageSize = terrainHeight * ((terrainWidth * 3) + 1);

	//Creating an array to create memory for the bitmap image data.
	unsigned char* bitmapImage = new unsigned char[imageSize];

	//Moving to the beginning of the bitmap data.
	fseek(fileptr, bitmapFileHeader.bfOffBits, SEEK_SET);

	//Reading the bitmap data
	fread(bitmapImage, 1, imageSize, fileptr);

	fclose(fileptr);

	heightMap = new XMFLOAT3[terrainWidth * terrainHeight];

	int p = 0;

	float heightFactor = 10.0f;

	for (int j = 0; j < terrainHeight; j++)
	{
		for (int i = 0; i < terrainWidth; i++)
		{
			index = (terrainWidth * (terrainHeight - 1 - j)) + i;

			height = bitmapImage[p];

			heightMap[index].y = (float)height / heightFactor;

			p += 3;
		}
		p++;
	}

	delete[] bitmapImage;
	bitmapImage = 0;

	// Grid Generation

	rows = terrainWidth;
	columns = terrainHeight;

	depth = columns;
	width = rows;

	rows = rows - 1;
	columns = columns - 1;
	totalCells = (rows - 1) * (columns - 1);
	totalFaces = (rows - 1) * (columns - 1) * 2 * 6;
	totalVertices = rows * columns;

	dx = width / (columns - 1);
	dz = depth / (rows - 1);

	du = 1.0f / (columns - 1);
	dv = 1.0f / (rows - 1);

	std::vector<SimpleVertex> v(totalVertices);

	// Vertex Generation
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < columns; j++)
		{
			v[i * columns + j].Pos = XMFLOAT3((-0.5 * width) + (float)j * dx, heightMap[i*columns+j].y, (0.5 * depth) - (float)i * dz);
			v[i * columns + j].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
		
			v[i * columns + j].TexC.x = j * du;
			v[i * columns + j].TexC.y = i * dv;

		}
	}

	std::vector<WORD> indices(totalFaces * 3);

	int k = 0;
	// Index Generation
	for (UINT i = 0; i < rows - 1; i++)
	{
		for (UINT j = 0; j < columns -1; j++)
		{
			indices[k] = i * columns + j;		
			indices[k + 1] = i * columns + j + 1;
			indices[k + 2] = (i + 1)* columns + j;	
			indices[k + 3] = (i + 1) * columns + j;
			indices[k + 4] = i * columns + j + 1;
			indices[k + 5] = (i + 1) * columns + j + 1;
			
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
	hr = pd3dDevice->CreateBuffer(&bd, &InitData, &pgridVertexBuffer);

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
	hr = pd3dDevice->CreateBuffer(&bd2, &InitData2, &pgridIndexBuffer);

	if (FAILED(hr))
		return hr;


	return hr;
}
