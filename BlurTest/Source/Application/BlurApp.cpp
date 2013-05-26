#include "BlurTestStd.h"
#include "BlurApp.h"

#include "../Scene/Box.h"

ID3D11RenderTargetView* pNullRTV;
ID3D11ShaderResourceView* pNullSRV;
ID3D11UnorderedAccessView*	pNullUAV;

BlurApp::BlurApp()
{
	m_mode = LM_Lambert;
	m_maskMode = MM_Everything;

	m_vCameraPos = XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f);
	m_vCameraTarget = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	m_vCameraUp	= XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); 

	m_fFovY = 0.4f * 3.14f;
	m_fAspectRatio = (float)SCREEN_WIDTH  / (float)SCREEN_HEIGHT;
	m_fNearZ = 1.0f;
	m_fFarZ = 100.0f; 
}

BlurApp::~BlurApp()
{
	while (!m_meshes.empty())
	{
		m_meshes.pop_back();
	}
}

void BlurApp::InitializeTextures()
{
	HRESULT hr;

	// ******************************************
	//
	//			Create Texture Resources
	//
	// ******************************************

	//Fill texture desc
	D3D11_TEXTURE2D_DESC tDesc;
	ZeroMemory(&tDesc, sizeof(D3D11_TEXTURE2D_DESC));
	tDesc.Width = SCREEN_WIDTH;
	tDesc.Height = SCREEN_HEIGHT;
	tDesc.ArraySize = 1;
	tDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	tDesc.CPUAccessFlags = 0;
	tDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	tDesc.SampleDesc.Count = 1;
	tDesc.Usage = D3D11_USAGE_DEFAULT;
	tDesc.MiscFlags = 0;
	tDesc.MipLevels = 1;

	///////////////////////////////////////////
	//Create textures
	hr = m_d3d11Device->CreateTexture2D(&tDesc, NULL, &m_pMaskTexture);
	if (hr != S_OK)
	{
		assert(0 && "error");
	}

	tDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS;
	hr = m_d3d11Device->CreateTexture2D(&tDesc, NULL, &m_pSceneTexture);
	if (hr != S_OK)
	{
		assert(0 && "error");
	}

	tDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	hr = m_d3d11Device->CreateTexture2D(&tDesc, NULL, &m_pBlurredTexture);
	if (hr != S_OK)
	{
		assert(0 && "error");
	}

	// ******************************************
	//
	//			Create Shader Resource Views
	//
	// ******************************************

	//fill shader resource view structure
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	srvDesc.Format = tDesc.Format;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

	/////////////////////////////////////////////
	//Initialize shader resource views
	hr = m_d3d11Device->CreateShaderResourceView(m_pMaskTexture, &srvDesc, &m_pMaskSRV);
	if (hr != S_OK)
	{
		assert(0 && "error");
	}

	hr = m_d3d11Device->CreateShaderResourceView(m_pSceneTexture, &srvDesc, &m_pSceneSRV);
	if (hr != S_OK)
	{
		assert(0 && "error");
	}

	hr = m_d3d11Device->CreateShaderResourceView(m_pBlurredTexture, &srvDesc, &m_pBlurredSRV);
	if (hr != S_OK)
	{
		assert(0 && "error");
	}

	hr = D3DX11CreateShaderResourceViewFromFile(m_d3d11Device, L"Resources\\bg_2.png", NULL, NULL, &m_pBackgroundSRV, NULL);


	// ******************************************
	//
	//			Create Render Target Views
	//
	// ******************************************

	//fill render target view structure
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	ZeroMemory(&rtvDesc, sizeof(&rtvDesc));
	rtvDesc.Format = tDesc.Format;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	//////////////////////////////////////////////
	//Initialize render target views
	hr = m_d3d11Device->CreateRenderTargetView(m_pMaskTexture, &rtvDesc, &m_pMaskRTV);
	if (hr != S_OK)
	{
		assert(0 && "error");
	}
	hr = m_d3d11Device->CreateRenderTargetView(m_pSceneTexture, &rtvDesc, &m_pSceneRTV);
	if (hr != S_OK)
	{
		assert(0 && "error");
	}

	// ******************************************
	//
	//		Create Unordered Access Views
	//
	// ******************************************
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	ZeroMemory(&uavDesc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
	uavDesc.Format = tDesc.Format;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	//////////////////////////////////////////////
	//Initialize unordered access views
	hr = m_d3d11Device->CreateUnorderedAccessView(m_pSceneTexture, &uavDesc, &m_pSceneUAV);
	if (hr != S_OK)
	{
		assert(0 && "error");
	}
	hr = m_d3d11Device->CreateUnorderedAccessView(m_pBlurredTexture, &uavDesc, &m_pBlurredUAV);
	if (hr != S_OK)
	{
		assert(0 && "error");
	}
}

void BlurApp::InitializeDepthBuffer()
{
	// ******************************************
	//	Initialize depth buffer texture and views
	// ******************************************
	D3D11_TEXTURE2D_DESC	depthDesc;
	ZeroMemory(&depthDesc, sizeof(D3D11_TEXTURE2D_DESC));
	depthDesc.Width = SCREEN_WIDTH;
	depthDesc.Height = SCREEN_HEIGHT;
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = 1;
	depthDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthDesc.CPUAccessFlags = 0;
	depthDesc.MiscFlags = 0;

	HRESULT hr = m_d3d11Device->CreateTexture2D(&depthDesc, nullptr, &m_pDepthStencilTexture);

	//Initialize depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	ZeroMemory(&dsvDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;

	hr = m_d3d11Device->CreateDepthStencilView(m_pDepthStencilTexture, &dsvDesc, &m_pDepthStencilView);
	if (hr != S_OK)
	{
		assert(0 && "Error creating depth stencil view!!!");
	}

	//Initialize shader resource view for depth texture
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	hr = m_d3d11Device->CreateShaderResourceView(m_pDepthStencilTexture, &srvDesc, &m_pDepthStencilSRV);
	if (hr != S_OK)
	{
		assert(0 && "Error creating shader resource view for depth texture!");
	}

	// ******************************************
	//	Initialize depth buffer states
	// ******************************************
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilDesc.StencilEnable = false;
	//depthStencilDesc.StencilReadMask = 0xFF;
	//depthStencilDesc.StencilWriteMask = 0xFF;
	
	//create depth stencil state
	hr = m_d3d11Device->CreateDepthStencilState(&depthStencilDesc, &m_pDepthEnable);
	if (hr != S_OK)
	{
		assert(0 && "error!!!");
	}

	depthStencilDesc.DepthEnable = false;
	hr = m_d3d11Device->CreateDepthStencilState(&depthStencilDesc, &m_pDepthDisable);
	if (hr != S_OK)
	{
		assert(0 && "error!!!");
	}

}

void BlurApp::InitializeSamplerStates()
{
	D3D11_SAMPLER_DESC sDesc;
	ZeroMemory(&sDesc, sizeof(D3D11_SAMPLER_DESC));
	sDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sDesc.MinLOD = 0;
	sDesc.MaxLOD = D3D11_FLOAT32_MAX;
	sDesc.MaxAnisotropy = 16;

	HRESULT hr = m_d3d11Device->CreateSamplerState(&sDesc, &m_pAnisotropicSampler);
	if (hr != S_OK)
	{
		assert(0 && "Error");
	}

	sDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

	hr = m_d3d11Device->CreateSamplerState(&sDesc, &m_pTiledSampler);
	if (hr != S_OK)
	{
		assert(0 && "Error");
	}
}

bool BlurApp::VInitSimulation()
{
	App::VInitSimulation();

	//Initialize textures and their views for rendering
	InitializeTextures();

	//Initialize depth stencil buffer
	InitializeDepthBuffer();

	//Initialize sampler states
	InitializeSamplerStates();

	HRESULT hr;

	///////////////////////////////////
	//Create shaders
	ID3D10Blob* pCompiledLightingVertexShader;
	ID3D10Blob* pCompiledLightingPixelShader;
	ID3D10Blob* pErrors;

	hr = D3DX11CompileFromFile(L"Shaders\\Lighting.hlsl", 0, 0, "VS", "vs_5_0", 0, 0, 0, &pCompiledLightingVertexShader, &pErrors, 0);
	if (hr != S_OK)
	{
		if (pErrors != 0)
			OutputDebugStringA((char*)pErrors->GetBufferPointer());
	}

	

	hr = D3DX11CompileFromFile(L"Shaders\\Lighting.hlsl", 0, 0, "PS", "ps_5_0", 0, 0, 0, &pCompiledLightingPixelShader, &pErrors, 0);
	if (hr != S_OK)
	{
		if (pErrors != 0)
			OutputDebugStringA((char*)pErrors->GetBufferPointer());
	}

	ID3D11ClassLinkage*	pClassLinkage = NULL;
	m_d3d11Device->CreateClassLinkage(&pClassLinkage);
	hr = m_d3d11Device->CreateVertexShader(pCompiledLightingVertexShader->GetBufferPointer(), pCompiledLightingVertexShader->GetBufferSize(), nullptr, &m_pLightingVertexShader);
	
	hr = m_d3d11Device->CreatePixelShader(pCompiledLightingPixelShader->GetBufferPointer(), pCompiledLightingPixelShader->GetBufferSize(), pClassLinkage, &m_pLightingPixelShader);

	ID3D11ClassInstance* pLambert = NULL;
	hr = pClassLinkage->GetClassInstance("g_lambert", 0, &pLambert);
	m_lightingClassInstances.insert(std::make_pair<LightingMode, ID3D11ClassInstance*>(LM_Lambert, (ID3D11ClassInstance*&&)pLambert));

	ID3D11ClassInstance* pLambertWrapAround = NULL;
	hr = pClassLinkage->GetClassInstance("g_lambertWrapAround", 0, &pLambertWrapAround);
	m_lightingClassInstances.insert(std::make_pair<LightingMode, ID3D11ClassInstance*>(LM_LambertWrapAround, (ID3D11ClassInstance*&&)pLambertWrapAround));

	ID3D11ClassInstance* pPhong = NULL;
	hr = pClassLinkage->GetClassInstance("g_phong", 0, &pPhong);
	m_lightingClassInstances.insert(std::make_pair<LightingMode, ID3D11ClassInstance*>(LM_Phong, (ID3D11ClassInstance*&&)pPhong));

	ID3D11ClassInstance* pBlinn = NULL;
	hr = pClassLinkage->GetClassInstance("g_blinn", 0, &pBlinn);
	m_lightingClassInstances.insert(std::make_pair<LightingMode, ID3D11ClassInstance*>(LM_Blinn, (ID3D11ClassInstance*&&)pBlinn));

	ID3D11ClassInstance* pToon = NULL;
	hr = pClassLinkage->GetClassInstance("g_toon", 0, &pToon);
	m_lightingClassInstances.insert(std::make_pair<LightingMode, ID3D11ClassInstance*>(LM_Toon, (ID3D11ClassInstance*&&)pToon));

	ID3D11ClassInstance* pIsotropicWard = NULL;
	hr = pClassLinkage->GetClassInstance("g_isotropicWard", 0, &pIsotropicWard);
	m_lightingClassInstances.insert(std::make_pair<LightingMode, ID3D11ClassInstance*>(LM_IsotropicWard, (ID3D11ClassInstance*&&)pIsotropicWard));
	
	///////////////////////////////////
	//Initialize layout
	D3D11_INPUT_ELEMENT_DESC layout[] = 
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TEXCOORDS", 0, DXGI_FORMAT_R32G32_FLOAT,	  1, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "BINORMAL",  0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	m_d3d11Device->CreateInputLayout(layout, 5, pCompiledLightingVertexShader->GetBufferPointer(), pCompiledLightingVertexShader->GetBufferSize(), &m_pGeometryLayout);

	/////////////////////////////////////
	//Create blur shader
	ID3D10Blob*	pCompiledHorizontalGaussComputeShader;
	ID3D10Blob*	pCompiledVerticalGaussComputeShader;
	ID3D10Blob* pCompiledGaussComputeShader;

	hr = D3DX11CompileFromFile(L"Shaders\\Blur.hlsl", 0, 0, "CS_Horizontal", "cs_5_0", 0, 0, 0, &pCompiledHorizontalGaussComputeShader, &pErrors, 0);
	if (hr != S_OK)
	{
		if (pErrors != 0)
			OutputDebugStringA((char*)pErrors->GetBufferPointer());
	}

	hr = D3DX11CompileFromFile(L"Shaders\\Blur.hlsl", 0, 0, "CS_Vertical", "cs_5_0", 0, 0, 0, &pCompiledVerticalGaussComputeShader, &pErrors, 0);
	if (hr != S_OK)
	{
		if (pErrors != 0)
			OutputDebugStringA((char*)pErrors->GetBufferPointer());
	}

	hr = D3DX11CompileFromFile(L"Shaders\\Gauss.hlsl", 0, 0, "CS", "cs_5_0", 0, 0, 0, &pCompiledGaussComputeShader, &pErrors, 0);
	if (hr != S_OK)
	{
		if (pErrors != 0)
			OutputDebugStringA((char*)pErrors->GetBufferPointer());
	}

	hr = m_d3d11Device->CreateComputeShader(pCompiledHorizontalGaussComputeShader->GetBufferPointer(), pCompiledHorizontalGaussComputeShader->GetBufferSize(), nullptr, &m_pGaussHorizontalComputeShader); 
	hr = m_d3d11Device->CreateComputeShader(pCompiledVerticalGaussComputeShader->GetBufferPointer(), pCompiledVerticalGaussComputeShader->GetBufferSize(), nullptr, &m_pGaussVerticalComputeShader); 
	hr = m_d3d11Device->CreateComputeShader(pCompiledGaussComputeShader->GetBufferPointer(), pCompiledGaussComputeShader->GetBufferSize(), nullptr, &m_pGaussComputeShader);

	/////////////////////////////////////
	//Create texture rendering  shaders
	ID3D10Blob* pCompiledTextureVertexShader;
	ID3D10Blob*	pCompiledTexturePixelShader;
	ID3D10Blob*	pCompiledBackgroundPixelShader;

	hr = D3DX11CompileFromFile(L"Shaders\\FinalPass.hlsl", 0, 0, "VS", "vs_5_0", 0, 0, 0, &pCompiledTextureVertexShader, &pErrors, 0);
	if (hr != S_OK)
	{
		if (pErrors != 0)
			OutputDebugStringA((char*)pErrors->GetBufferPointer());
	}

	hr = D3DX11CompileFromFile(L"Shaders\\FinalPass.hlsl", 0, 0, "PS", "ps_5_0", 0, 0, 0, &pCompiledTexturePixelShader, &pErrors, 0);
	if (hr != S_OK)
	{
		if (pErrors != 0)
			OutputDebugStringA((char*)pErrors->GetBufferPointer());
	}

	hr = D3DX11CompileFromFile(L"Shaders\\FinalPass.hlsl", 0, 0, "PSBackground", "ps_5_0", 0, 0, 0, &pCompiledBackgroundPixelShader, &pErrors, 0);
	if (hr != S_OK)
	{
		if (pErrors != 0)
			OutputDebugStringA((char*)pErrors->GetBufferPointer());
	}

	hr = m_d3d11Device->CreateVertexShader(pCompiledTextureVertexShader->GetBufferPointer(), pCompiledTextureVertexShader->GetBufferSize(), nullptr, &m_pFinalPassVertexShader);
	hr = m_d3d11Device->CreatePixelShader(pCompiledTexturePixelShader->GetBufferPointer(), pCompiledTexturePixelShader->GetBufferSize(), nullptr, &m_pFinalPassPixelShader);
	hr = m_d3d11Device->CreatePixelShader(pCompiledBackgroundPixelShader->GetBufferPointer(), pCompiledBackgroundPixelShader->GetBufferSize(), nullptr, &m_pBackgroundPixelShader);

	D3D11_INPUT_ELEMENT_DESC texlayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORDS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	hr = m_d3d11Device->CreateInputLayout(texlayout, 2, pCompiledTextureVertexShader->GetBufferPointer(), pCompiledTextureVertexShader->GetBufferSize(), &m_pTextureLayout);

	/////////////////////////////////////
	//Create mask shader
	ID3D10Blob*	pCompiledMaskPixelShader;
	ID3D10Blob* pCompiledMaskModifiedPixelShader;

	hr = D3DX11CompileFromFile(L"Shaders\\MaskGeneration.hlsl", 0, 0, "MaskSimplePS", "ps_5_0", 0, 0, 0, &pCompiledMaskPixelShader, &pErrors, 0);
	if (hr != S_OK)
	{
		if (pErrors != 0)
			OutputDebugStringA((char*)pErrors->GetBufferPointer());
	}

	hr = m_d3d11Device->CreatePixelShader(pCompiledMaskPixelShader->GetBufferPointer(), pCompiledMaskPixelShader->GetBufferSize(), nullptr, &m_pMaskPixelShader); 

	hr = D3DX11CompileFromFile(L"Shaders\\MaskGeneration.hlsl", 0, 0, "MaskDepthPS", "ps_5_0", 0, 0, 0, &pCompiledMaskModifiedPixelShader, &pErrors, 0);
	if (hr != S_OK)
	{
		if (pErrors != 0)
			OutputDebugStringA((char*)pErrors->GetBufferPointer());
	}

	hr = m_d3d11Device->CreatePixelShader(pCompiledMaskModifiedPixelShader->GetBufferPointer(), pCompiledMaskModifiedPixelShader->GetBufferSize(), nullptr, &m_pMaskModifiedPixelShader); 

	//release blobs
	pCompiledBackgroundPixelShader->Release();
	pCompiledMaskPixelShader->Release();
	pCompiledMaskModifiedPixelShader->Release();
	pCompiledHorizontalGaussComputeShader->Release();
	pCompiledVerticalGaussComputeShader->Release();
	pCompiledGaussComputeShader->Release();
	pCompiledLightingVertexShader->Release();
	pCompiledLightingPixelShader->Release();
	pCompiledTextureVertexShader->Release();
	pCompiledTexturePixelShader->Release();

	///////////////////////////////////////////
	//Set viewport
	///////////////////////////////////////////
	ZeroMemory(&m_viewport, sizeof(D3D11_VIEWPORT));

	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;
	m_viewport.Width = SCREEN_WIDTH;
	m_viewport.Height = SCREEN_HEIGHT;
	m_d3d11DeviceContext->RSSetViewports(1, &m_viewport);

	//////////////////////////////////////////
	//Initialize constant buffers
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));

	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(MatrixTexBuffer)*3; //???!!!

	m_d3d11Device->CreateBuffer(&bufferDesc, 0, &m_pcbMatrixTex);

	//update it
	MatrixTexBuffer buffer;
	buffer.WorldViewProjection = XMMatrixTranspose(XMMatrixIdentity() * m_matrixView * XMMatrixOrthographicLH(SCREEN_WIDTH, SCREEN_HEIGHT, m_fNearZ, m_fFarZ));
	m_d3d11DeviceContext->UpdateSubresource(m_pcbMatrixTex, 0, nullptr, &buffer, 0, 0);

	//screen data constant buffer
	bufferDesc.ByteWidth = sizeof(ScreenData);
	m_d3d11Device->CreateBuffer(&bufferDesc, 0, &m_pcbScreen);

	ScreenData screenBuffer;
	screenBuffer.screen_width = SCREEN_WIDTH;
	screenBuffer.screen_height = SCREEN_HEIGHT;
	m_d3d11DeviceContext->UpdateSubresource(m_pcbScreen, 0, nullptr, &screenBuffer, 0, 0);

	//lighting buffer
	bufferDesc.ByteWidth = sizeof(LightingData);
	m_d3d11Device->CreateBuffer(&bufferDesc, 0, &m_pcbLighting);

	LightingData lightingData;
	lightingData.lambertLightColor = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	lightingData.lambertWALightColorAndFactor = XMVectorSet(1.0f, 1.0f, 1.0f, 0.3f);
	lightingData.phongLightColorAndSpecPower = XMVectorSet(1.0f, 1.0f, 1.0f, 120.0f);
	lightingData.blinnLightColorAndSpecPower = XMVectorSet(1.0f, 1.0f, 1.0f, 22.0f);
	lightingData.toonLightColor = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	lightingData.isotropicWardLightColorAndRoughness = XMVectorSet(1.0f, 1.0f, 1.0f, 30.0f);
	m_d3d11DeviceContext->UpdateSubresource(m_pcbLighting, 0, nullptr, &lightingData, 0, 0);

	/////////////////////////////////////////////
	//Vertex Buffer Description 
	/////////////////////////////////////////////
	D3D11_BUFFER_DESC vbDesc;
	ZeroMemory(&vbDesc, sizeof(D3D11_BUFFER_DESC));

	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;
	vbDesc.MiscFlags = 0;
	vbDesc.ByteWidth = 20 * 6;

	float left = (float)(SCREEN_WIDTH / 2 * -1);
	float right = left + SCREEN_WIDTH;
	float top = (float)(SCREEN_HEIGHT / 2);
	float bottom = top - SCREEN_HEIGHT;

	RectVertex verts [] = 
	{
		RectVertex(left,	top,	0.0f,	0.0f,	0.0f ), //left top
		RectVertex(right,	top,	0.0f,	1.0f,	0.0f ), //right top
		RectVertex(left,	bottom, 0.0f,	0.0f,	1.0f ), //left bottom
		RectVertex(left,	bottom, 0.0f,	0.0f,	1.0f ), //left bottom
		RectVertex(right,	top,	0.0f,	1.0f,	0.0f ), //right top
		RectVertex(right,	bottom, 0.0f,	1.0f,	1.0f ), //right bottom
	};


	//////////////////////////////////////////////
	//Subresource data
	//////////////////////////////////////////////
	D3D11_SUBRESOURCE_DATA bufferData;
	ZeroMemory(&bufferData, sizeof(D3D11_SUBRESOURCE_DATA));
	bufferData.pSysMem = verts;
	hr = m_d3d11Device->CreateBuffer(&vbDesc, &bufferData, &m_pRectVertexBuffer);

	RectVertex sverts [] = 
	{
		RectVertex(left,	top,	0.0f,	0.0f,			0.0f ), //left top
		RectVertex(right,	top,	0.0f,	(float)SCREEN_WIDTH / 128.0f,	0.0f ), //right top
		RectVertex(left,	bottom, 0.0f,	0.0f,			(float)SCREEN_HEIGHT / 128.0f ), //left bottom
		RectVertex(left,	bottom, 0.0f,	0.0f,			(float)SCREEN_HEIGHT / 128.0f ), //left bottom
		RectVertex(right,	top,	0.0f,	(float)SCREEN_WIDTH / 128.0f,	0.0f ), //right top
		RectVertex(right,	bottom, 0.0f,	(float)SCREEN_WIDTH / 128.0f,	(float)SCREEN_HEIGHT / 128.0f ), //right bottom
	};

	bufferData.pSysMem = sverts;
	hr = m_d3d11Device->CreateBuffer(&vbDesc, &bufferData, &m_pScreenRectVertexBuffer);

	//???!!!delete [] verts;

	///////////////////////////////////////////
	//Initialize meshes
	///////////////////////////////////////////
	m_currentShape = box_shape;

	//add box shape to the list
	Box* pBox = new Box();
	pBox->VInitialize(m_d3d11Device);

	m_meshes.push_back(pBox);

	return true;
}

void BlurApp::VUpdate(real elapsedTime, real totalTime)
{
	for (Meshes::iterator it = m_meshes.begin(); it != m_meshes.end(); it++)
	{
		(*it)->VUpdate(this, elapsedTime, totalTime);
	}
}

void BlurApp::VRender(real elapsedTime, real totalTime)
{
	//App::VRender(elapsedTime, totalTime);

	UINT stride = sizeof(RectVertex);
	UINT offset = 0;

	////////////////////////////////////////////
	//Render background
	m_d3d11DeviceContext->IASetVertexBuffers(0, 1, &m_pScreenRectVertexBuffer, &stride, &offset);
	m_d3d11DeviceContext->IASetInputLayout(m_pTextureLayout);
	m_d3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_d3d11DeviceContext->VSSetShader(m_pFinalPassVertexShader, 0, 0);
	m_d3d11DeviceContext->VSSetConstantBuffers(0, 1, &m_pcbMatrixTex);
	m_d3d11DeviceContext->PSSetShader(m_pBackgroundPixelShader, 0, 0);
	m_d3d11DeviceContext->PSSetShaderResources(0, 1, &m_pBackgroundSRV);
	m_d3d11DeviceContext->PSSetSamplers(0, 1, &m_pTiledSampler);
	m_d3d11DeviceContext->OMSetDepthStencilState(m_pDepthDisable, 1);
	m_d3d11DeviceContext->OMSetRenderTargets(1, &m_pSceneRTV, nullptr);
	m_d3d11DeviceContext->RSSetViewports(1, &m_viewport);
	m_d3d11DeviceContext->Draw(6, 0);

	//unbind resources
	m_d3d11DeviceContext->PSSetShaderResources(0, 1, &pNullSRV);
	m_d3d11DeviceContext->OMSetRenderTargets(1, &pNullRTV, NULL);

	////////////////////////////////////////////
	//Render meshes
	m_d3d11DeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 1);

	//set necessary states first
	m_d3d11DeviceContext->IASetInputLayout(m_pGeometryLayout);
	m_d3d11DeviceContext->VSSetShader(m_pLightingVertexShader, 0, 0);
	m_d3d11DeviceContext->PSSetShader(m_pLightingPixelShader, &m_lightingClassInstances[m_mode], 1);
	m_d3d11DeviceContext->PSSetConstantBuffers(0, 1, &m_pcbLighting);
	m_d3d11DeviceContext->PSSetSamplers(0, 1, &m_pAnisotropicSampler);
	m_d3d11DeviceContext->OMSetDepthStencilState(m_pDepthEnable, 1);
	m_d3d11DeviceContext->OMSetRenderTargets(1, &m_pSceneRTV, m_pDepthStencilView);

	for (Meshes::iterator it = m_meshes.begin(); it != m_meshes.end(); it++)
	{
		(*it)->VPreRender(this, elapsedTime, totalTime);
		(*it)->VRender(m_d3d11DeviceContext, elapsedTime, totalTime);
		(*it)->VPostRender(this, elapsedTime, totalTime);
	}

	m_d3d11DeviceContext->OMSetRenderTargets(1, &pNullRTV, nullptr);

	//*******************************************************************//

	////////////////////////////////////////////
	//Generate mask (for blurring)
	m_d3d11DeviceContext->IASetVertexBuffers(0, 1, &m_pRectVertexBuffer, &stride, &offset);
	m_d3d11DeviceContext->IASetInputLayout(m_pTextureLayout);
	m_d3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_d3d11DeviceContext->VSSetShader(m_pFinalPassVertexShader, 0, 0);
	m_d3d11DeviceContext->VSSetConstantBuffers(0, 1, &m_pcbMatrixTex);
	if (m_maskMode == MM_Everything)	m_d3d11DeviceContext->PSSetShader(m_pMaskPixelShader, 0, 0);
	if (m_maskMode == MM_OnlyObject)	m_d3d11DeviceContext->PSSetShader(m_pMaskModifiedPixelShader, 0, 0);
	m_d3d11DeviceContext->PSSetShaderResources(0, 1, &m_pDepthStencilSRV);
	m_d3d11DeviceContext->PSSetConstantBuffers(0, 1, &m_pcbScreen);
	m_d3d11DeviceContext->OMSetDepthStencilState(m_pDepthDisable, 1);
	m_d3d11DeviceContext->OMSetRenderTargets(1, &m_pMaskRTV, nullptr);
	
	m_d3d11DeviceContext->Draw(6, 0);

	m_d3d11DeviceContext->PSSetShaderResources(0, 1, &pNullSRV);
	m_d3d11DeviceContext->OMSetRenderTargets(1, &pNullRTV, nullptr); 
	//*******************************************************************//

	////////////////////////////////////////////
	//Apply blur
	/*m_d3d11DeviceContext->CSSetShader(m_pGaussHorizontalComputeShader, 0, 0);
	m_d3d11DeviceContext->CSSetShaderResources(0, 1, &m_pSceneSRV);
	m_d3d11DeviceContext->CSSetShaderResources(2, 1, &m_pMaskSRV);
	m_d3d11DeviceContext->CSSetUnorderedAccessViews(0, 1, &m_pBlurredUAV, nullptr);
	m_d3d11DeviceContext->Dispatch(1, SCREEN_HEIGHT, 1); */
	m_d3d11DeviceContext->CSSetShader(m_pGaussComputeShader, 0, 0);
	m_d3d11DeviceContext->CSSetShaderResources(0, 1, &m_pSceneSRV);
	m_d3d11DeviceContext->CSSetShaderResources(1, 1, &m_pMaskSRV);
	m_d3d11DeviceContext->CSSetUnorderedAccessViews(0, 1, &m_pBlurredUAV, nullptr);
	m_d3d11DeviceContext->Dispatch((UINT)ceil(SCREEN_WIDTH / 32.0f), (UINT)ceil(SCREEN_HEIGHT / 32.0f), 1);
	m_d3d11DeviceContext->CSSetShaderResources(0, 1, &pNullSRV);
	m_d3d11DeviceContext->CSSetShaderResources(1, 1, &pNullSRV);
	m_d3d11DeviceContext->CSSetUnorderedAccessViews(0, 1, &pNullUAV, nullptr);

	/*m_d3d11DeviceContext->CSSetShader(m_pGaussVerticalComputeShader, 0, 0);
	m_d3d11DeviceContext->CSSetShaderResources(1, 1, &m_pBlurredSRV);
	m_d3d11DeviceContext->CSSetShaderResources(3, 1, &m_pMaskSRV);
	m_d3d11DeviceContext->CSSetUnorderedAccessViews(1, 1, &m_pSceneUAV, nullptr);
	m_d3d11DeviceContext->Dispatch(SCREEN_WIDTH, 1, 1);
	m_d3d11DeviceContext->CSSetShaderResources(1, 1, &pNullSRV);
	m_d3d11DeviceContext->CSSetShaderResources(3, 1, &pNullSRV);
	m_d3d11DeviceContext->CSSetUnorderedAccessViews(1, 1, &pNullUAV, nullptr); 
	*/
	//*******************************************************************//

	/////////////////////////////////////////////
	//Render texture to the screen
	//UINT stride = sizeof(RectVertex);
	//UINT offset = 0;
	m_d3d11DeviceContext->IASetVertexBuffers(0, 1, &m_pRectVertexBuffer, &stride, &offset);
	m_d3d11DeviceContext->IASetInputLayout(m_pTextureLayout);
	m_d3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_d3d11DeviceContext->VSSetShader(m_pFinalPassVertexShader, 0, 0);
	m_d3d11DeviceContext->VSSetConstantBuffers(0, 1, &m_pcbMatrixTex);
	m_d3d11DeviceContext->PSSetShader(m_pFinalPassPixelShader, 0, 0);
	//m_d3d11DeviceContext->PSSetShaderResources(0, 1, &m_pSceneSRV);
	m_d3d11DeviceContext->PSSetShaderResources(0, 1, &m_pBlurredSRV);
	m_d3d11DeviceContext->OMSetRenderTargets(1, &m_pbbRenderTargetView, nullptr);
	m_d3d11DeviceContext->OMSetDepthStencilState(m_pDepthDisable, 1);
	m_d3d11DeviceContext->RSSetViewports(1, &m_viewport);
	m_d3d11DeviceContext->Draw(6, 0);

	//unbind resources
	m_d3d11DeviceContext->PSSetShaderResources(0, 1, &pNullSRV);
	m_d3d11DeviceContext->OMSetRenderTargets(1, &pNullRTV, NULL);

	//flip back buffer
	m_SwapChain->Present(0, 0);

	//*******************************************************************//

	////////////////////////////////////////////////
	//Clear all render targets
	float color[4] = { 0, 0, 0, 1 };
	m_d3d11DeviceContext->ClearRenderTargetView(m_pbbRenderTargetView, color);
	m_d3d11DeviceContext->ClearRenderTargetView(m_pMaskRTV, color);
	m_d3d11DeviceContext->ClearRenderTargetView(m_pSceneRTV, color);

	//m_d3d11DeviceContext->ClearDepthStencilView(m_pDepthStencilView, 0, 0);

	//*******************************************************************//
}

void BlurApp::VKeyPressed(const Key key)
{
	switch(key)
	{
		//'A'
	case 0x41:
		m_mode = static_cast<LightingMode>(m_mode - 1);
		if (m_mode < 0)
			m_mode = static_cast<LightingMode>(LM_NumLightingModes - 1);
		break;

		//'D'
	case 0x44:
		m_mode = static_cast<LightingMode>(m_mode + 1);
		m_mode = static_cast<LightingMode>(static_cast<int>(m_mode) % (LM_NumLightingModes));
		break;

		//'Q'
	case 0x51:
		m_maskMode = static_cast<MaskMode>(m_maskMode + 1);
		m_maskMode = static_cast<MaskMode>(static_cast<int>(m_maskMode) % (MM_NumMaskModes));
		break;
	};
}