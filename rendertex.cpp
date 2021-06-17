//--------------------------------------------------------------------------------------
// File: Tutorial07.cpp
//
// This application demonstrates texturing
//
// http://msdn.microsoft.com/en-us/library/windows/apps/ff729724.aspx
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License (MIT).
//--------------------------------------------------------------------------------------
#include <windows.h>
#include <d3d11_1.h>
#include <d3d11_2.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
#include "DDSTextureLoader.h"
#include "resource.h"

using namespace DirectX;

//? --------------------------------------------------------------------------------------
//? Structures
//? --------------------------------------------------------------------------------------
struct SimpleVertex
{
    XMFLOAT3 Pos;
    XMFLOAT2 Tex;
};

struct CBNeverChanges
{
    XMMATRIX mView;
};

struct CBChangeOnResize
{
    XMMATRIX mProjection;
};

struct CBChangesEveryFrame
{
    XMMATRIX mWorld;
    XMFLOAT4 vMeshColor;
};


//? --------------------------------------------------------------------------------------
//? Global Variables
//? --------------------------------------------------------------------------------------
HINSTANCE                           g_hInst = nullptr;
HWND                                g_hWnd = nullptr;
HWND                                g_hWndB = nullptr;
D3D_DRIVER_TYPE                     g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL                   g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*                       g_pd3dDevice = nullptr;
ID3D11Device1*                      g_pd3dDevice1 = nullptr;
ID3D11DeviceContext*                g_pImmediateContextA = nullptr;
ID3D11DeviceContext1*               g_pImmediateContext1A = nullptr;
IDXGISwapChain*                     g_pSwapChainA = nullptr;
IDXGISwapChain1*                    g_pSwapChain1A = nullptr;
ID3D11RenderTargetView*             g_pRenderTargetViewA = nullptr;
ID3D11DeviceContext*                g_pImmediateContextB = nullptr;
ID3D11DeviceContext1*               g_pImmediateContext1B = nullptr;
IDXGISwapChain*                     g_pSwapChainB = nullptr;
IDXGISwapChain1*                    g_pSwapChain1B = nullptr;
ID3D11RenderTargetView*             g_pRenderTargetViewB = nullptr;
ID3D11Texture2D*                    g_pDepthStencilA = nullptr;
ID3D11DepthStencilView*             g_pDepthStencilViewA = nullptr;
ID3D11Texture2D*                    g_pDepthStencilB = nullptr;
ID3D11DepthStencilView*             g_pDepthStencilViewB = nullptr;
ID3D11VertexShader*                 g_pVertexShader = nullptr;
ID3D11VertexShader*                 g_pVertexShader1 = nullptr;
ID3D11PixelShader*                  g_pPixelShader = nullptr;
ID3D11PixelShader*                  g_pPixelShader1 = nullptr;
ID3D11VertexShader*                 g_pVertexShaderB = nullptr;
ID3D11VertexShader*                 g_pVertexShader1B = nullptr;
ID3D11PixelShader*                  g_pPixelShaderB = nullptr;
ID3D11PixelShader*                  g_pPixelShader1B = nullptr;
ID3D11InputLayout*                  g_pVertexLayout = nullptr;
ID3D11InputLayout*                  g_pVertexLayoutB = nullptr;
ID3D11Buffer*                       g_pVertexBuffer = nullptr;
ID3D11Buffer*                       g_pVertexBufferB = nullptr;
ID3D11Buffer*                       g_pIndexBuffer = nullptr;
ID3D11Buffer*                       g_pIndexBufferB = nullptr;
ID3D11Buffer*                       g_pCBNeverChanges = nullptr;
ID3D11Buffer*                       g_pCBChangeOnResize = nullptr;
ID3D11Buffer*                       g_pCBChangesEveryFrame = nullptr;
ID3D11Buffer*                       g_pCBNeverChangesB = nullptr;
ID3D11Buffer*                       g_pCBChangeOnResizeB = nullptr;
ID3D11Buffer*                       g_pCBChangesEveryFrameB = nullptr;
ID3D11ShaderResourceView*           g_pTextureRV = nullptr;
ID3D11ShaderResourceView*           g_pTextureRV1 = nullptr;
ID3D11SamplerState*                 g_pSamplerLinear = nullptr;
ID3D11SamplerState*                 g_pSamplerLinear1 = nullptr;
ID3D11Texture2D*                    g_pRenderedTex;
XMMATRIX                            g_World;
XMMATRIX                            g_View;
XMMATRIX                            g_Projection;
XMFLOAT4                            g_vMeshColor( 1.0f, 1.0f, 1.0f, 1.0f );


//? --------------------------------------------------------------------------------------
//? Forward declarations
//? --------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
HRESULT InitDevice();
HRESULT InitDeviceB();
void CleanupDevice();
LRESULT CALLBACK    WndProc( HWND, UINT, WPARAM, LPARAM );
void Render();


//? --------------------------------------------------------------------------------------
//? Entry point to the program. Initializes everything and goes into a message processing
//? loop. Idle time is used to render the scene.
//? --------------------------------------------------------------------------------------
int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;

    if( FAILED( InitDevice() ) )
    {
        CleanupDevice();
        return 0;
    }

    if (FAILED(InitDeviceB()))
    {
        CleanupDevice();
        return 0;
    }

    // Main message loop
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            Render();
        }
    }

    CleanupDevice();

    return ( int )msg.wParam;
}

//? --------------------------------------------------------------------------------------
//? Shader stuff
//? --------------------------------------------------------------------------------------

const auto m_shader = R"SHADER(
    Texture2D txDiffuse : register(t0);
    SamplerState samLinear : register(s0);

    cbuffer cbNeverChanges : register( b0 )
    {
        matrix View;
    };

    cbuffer cbChangeOnResize : register( b1 )
    {
        matrix Projection;
    };

    cbuffer cbChangesEveryFrame : register( b2 )
    {
        matrix World;
        float4 vMeshColor;
    };

    struct VS_INPUT
    {
        float4 Pos : POSITION;
        float2 Tex : TEXCOORD0;
    };

    struct PS_INPUT
    {
        float4 Pos : SV_POSITION;
        float2 Tex : TEXCOORD0;
    };

    PS_INPUT VS(VS_INPUT input)
    {
        PS_INPUT output = (PS_INPUT)0;
        output.Pos = mul( input.Pos, World );
        output.Tex = input.Tex;

        return output;
    }

    float4 PS(PS_INPUT input) : SV_Target
    {
        return txDiffuse.Sample(samLinear, input.Tex);
    }
    )SHADER";

const auto m_shaderB = R"SHADERB(
    Texture2D txDiffuse : register(t0);
    SamplerState samLinear : register(s0);

    cbuffer cbNeverChanges : register( b0 )
    {
        matrix View;
    };

    cbuffer cbChangeOnResize : register( b1 )
    {
        matrix Projection;
    };

    cbuffer cbChangesEveryFrame : register( b2 )
    {
        matrix World;
        float4 vMeshColor;
    };

    struct VS_INPUT
    {
        float4 Pos : POSITION;
        float2 Tex : TEXCOORD0;
    };

    struct PS_INPUT
    {
        float4 Pos : SV_POSITION;
        float2 Tex : TEXCOORD0;
    };

    PS_INPUT VS(VS_INPUT input)
    {
        PS_INPUT output = (PS_INPUT)0;
        output.Pos = mul( input.Pos, View );
        output.Pos = mul( output.Pos, World );
        output.Pos = mul( output.Pos, Projection );
        output.Tex = input.Tex;

        return output;
    }

    float4 PS(PS_INPUT input) : SV_Target
    {
        return vMeshColor - txDiffuse.Sample(samLinear, input.Tex);
    }
    )SHADERB";

//? --------------------------------------------------------------------------------------
//? Register class and create window
//? --------------------------------------------------------------------------------------

HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    wcex.hCursor = LoadCursor( nullptr, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"WindowClass";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, 800, 600 };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( L"WindowClass", L"Window A",
                           WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
                           nullptr );
    if( !g_hWnd )
        return E_FAIL;

    g_hWndB = CreateWindow(L"WindowClass", L"Window B",
                           WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
                           nullptr);
    if (!g_hWndB)
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );
    ShowWindow( g_hWndB, nCmdShow );

    return S_OK;
}

//? shader from string method
HRESULT CompileShaderFromString(const char* shaderName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

    ID3DBlob* pErrorBlob = nullptr;
    hr = D3DCompile(
        shaderName,
        strlen(shaderName),
        nullptr,
        nullptr,
        nullptr,
        szEntryPoint,
        szShaderModel,
        dwShaderFlags,
        0,
        ppBlobOut,
        nullptr
    );

    if (FAILED(hr))
    {
        if (pErrorBlob)
        {
            OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
            pErrorBlob->Release();
        }
        return hr;
    }
    if (pErrorBlob) pErrorBlob->Release();

    return S_OK;
}

//? --------------------------------------------------------------------------------------
//? Create Direct3D device and swap chain
//? --------------------------------------------------------------------------------------

//! --------------------------------------------------------------------------------------
//! DEVICE A
//! --------------------------------------------------------------------------------------
HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect( g_hWnd, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDevice( nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
                                D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContextA );

        if ( hr == E_INVALIDARG )
        {
            // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
            hr = D3D11CreateDevice( nullptr, g_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
                                    D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContextA );
        }

        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;

    //! Obtain DXGI factory from device (since we used nullptr for pAdapter above)
    IDXGIFactory1* dxgiFactory = nullptr;
    {
        IDXGIDevice* dxgiDevice = nullptr;
        hr = g_pd3dDevice->QueryInterface( __uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice) );
        if (SUCCEEDED(hr))
        {
            IDXGIAdapter* adapter = nullptr;
            hr = dxgiDevice->GetAdapter(&adapter);
            if (SUCCEEDED(hr))
            {
                hr = adapter->GetParent( __uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory) );
                adapter->Release();
            }
            dxgiDevice->Release();
        }
    }
    if (FAILED(hr))
        return hr;

    //? Create swap chain
    IDXGIFactory2* dxgiFactory2 = nullptr;
    hr = dxgiFactory->QueryInterface( __uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2) );

        // DirectX 11.1 or later
        hr = g_pd3dDevice->QueryInterface( __uuidof(ID3D11Device1), reinterpret_cast<void**>(&g_pd3dDevice1) );
        if (SUCCEEDED(hr))
        {
            (void) g_pImmediateContextA->QueryInterface( __uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&g_pImmediateContext1A) );
        }

        DXGI_SWAP_CHAIN_DESC1 sd = {};
        sd.Width = width;
        sd.Height = height;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 1;

        hr = dxgiFactory2->CreateSwapChainForHwnd( g_pd3dDevice, g_hWnd, &sd, nullptr, nullptr, &g_pSwapChain1A );
        if (SUCCEEDED(hr))
        {
            hr = g_pSwapChain1A->QueryInterface( __uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_pSwapChainA) );
        }

        dxgiFactory2->Release();


    //! Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
    dxgiFactory->MakeWindowAssociation( g_hWnd, DXGI_MWA_NO_ALT_ENTER );

    dxgiFactory->Release();

    if (FAILED(hr))
        return hr;

    //! Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChainA->GetBuffer( 0, __uuidof( ID3D11Texture2D ), reinterpret_cast<void**>( &pBackBuffer ) );
    if( FAILED( hr ) )
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, nullptr, &g_pRenderTargetViewA );
    pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;

    //? Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth = {};
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;

    hr = g_pd3dDevice->CreateTexture2D( &descDepth, nullptr, &g_pDepthStencilA );
    if( FAILED( hr ) )
        return hr;

    //? Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = g_pd3dDevice->CreateDepthStencilView( g_pDepthStencilA, &descDSV, &g_pDepthStencilViewA );
    if( FAILED( hr ) )
        return hr;

    g_pImmediateContextA->OMSetRenderTargets( 1, &g_pRenderTargetViewA, g_pDepthStencilViewA );

    //? Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContextA->RSSetViewports( 1, &vp );

    //? Compile the vertex shader
    ID3DBlob* pVSBlob = nullptr;
    hr = CompileShaderFromString( m_shader, "VS", "vs_4_0", &pVSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( nullptr,
                    L"Vertex shader compilation failed.", L"Error", MB_OK );
        return hr;
    }

    //? Create the vertex shader
    hr = g_pd3dDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader );
    if( FAILED( hr ) )
    {
        pVSBlob->Release();
        return hr;
    }

    //? Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE( layout );

    //? Create the input layout
    hr = g_pd3dDevice->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
                                          pVSBlob->GetBufferSize(), &g_pVertexLayout );
    pVSBlob->Release();
    if( FAILED( hr ) )
        return hr;

    //? Set the input layout
    g_pImmediateContextA->IASetInputLayout( g_pVertexLayout );

    //? Compile the pixel shader
    ID3DBlob* pPSBlob = nullptr;
    hr = CompileShaderFromString( m_shader, "PS", "ps_4_0", &pPSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( nullptr,
                    L"Pixel shader compilation failed.", L"Error", MB_OK );
        return hr;
    }

    //? Create the pixel shader
    hr = g_pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader );
    pPSBlob->Release();
    if( FAILED( hr ) )
        return hr;

    //? Create vertex buffer
    SimpleVertex vertices[] =
    {
        { XMFLOAT3( -1.0f, -1.0f, 0.0f ), XMFLOAT2( 1.0f, 1.0f ) }, // vertex XYZ and texture map reference
        { XMFLOAT3( 1.0f, -1.0f, 0.0f ), XMFLOAT2( 0.0f, 1.0f ) },
        { XMFLOAT3( 1.0f, 1.0f, 0.0f ), XMFLOAT2( 0.0f, 0.0f ) },
        { XMFLOAT3( -1.0f, 1.0f, 0.0f ), XMFLOAT2( 1.0f, 0.0f ) }
    };

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( SimpleVertex ) * 4;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem = vertices;
    hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pVertexBuffer );
    if( FAILED( hr ) )
        return hr;

    //? Set vertex buffer
    UINT stride = sizeof( SimpleVertex );
    UINT offset = 0;
    g_pImmediateContextA->IASetVertexBuffers( 0, 1, &g_pVertexBuffer, &stride, &offset );

    //? Create index buffer
    //? Create vertex buffer
    WORD indices[] =
    {
        3,1,0,
        2,1,3
    };

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( WORD ) * 6;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    InitData.pSysMem = indices;
    hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pIndexBuffer );
    if( FAILED( hr ) )
        return hr;

    //? Set index buffer
    g_pImmediateContextA->IASetIndexBuffer( g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );

    //? Set primitive topology
    g_pImmediateContextA->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    //? Create the constant buffers
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(CBNeverChanges);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = g_pd3dDevice->CreateBuffer( &bd, nullptr, &g_pCBNeverChanges );
    if( FAILED( hr ) )
        return hr;

    bd.ByteWidth = sizeof(CBChangeOnResize);
    hr = g_pd3dDevice->CreateBuffer( &bd, nullptr, &g_pCBChangeOnResize );
    if( FAILED( hr ) )
        return hr;

    bd.ByteWidth = sizeof(CBChangesEveryFrame);
    hr = g_pd3dDevice->CreateBuffer( &bd, nullptr, &g_pCBChangesEveryFrame );
    if( FAILED( hr ) )
        return hr;

    //? Load the Texture
    hr = CreateDDSTextureFromFile( g_pd3dDevice, L"test.dds", nullptr, &g_pTextureRV );
    if( FAILED( hr ) )
        return hr;

    //? Create the sample state
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = g_pd3dDevice->CreateSamplerState( &sampDesc, &g_pSamplerLinear );
    if( FAILED( hr ) )
        return hr;

    //? Initialize the world matrices
    g_World = XMMatrixIdentity();

    //? Initialize the view matrix
    XMVECTOR Eye = XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f);    // cam rotation (Y-axis Rot, X-axis Rot, Z-axis?)
    XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);      // ? but cannot be empty/zero
    g_View = XMMatrixLookAtLH( Eye, At, Up );

    CBNeverChanges cbNeverChanges;
    cbNeverChanges.mView = XMMatrixTranspose( g_View );
    g_pImmediateContextA->UpdateSubresource( g_pCBNeverChanges, 0, nullptr, &cbNeverChanges, 0, 0 );

    //? Initialize the projection matrix
    g_Projection = XMMatrixPerspectiveFovLH( XM_PIDIV4, width / (FLOAT)height, 0.01f, 100.0f );

    CBChangeOnResize cbChangesOnResize;
    cbChangesOnResize.mProjection = XMMatrixTranspose( g_Projection );
    g_pImmediateContextA->UpdateSubresource( g_pCBChangeOnResize, 0, nullptr, &cbChangesOnResize, 0, 0 );

    return S_OK;
}

//! --------------------------------------------------------------------------------------
//! WINDOW A
//! --------------------------------------------------------------------------------------

HRESULT InitDeviceB()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect(g_hWndB, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
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
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
            D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContextB);

        if (hr == E_INVALIDARG)
        {
            // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
            hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
                D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContextB);
        }

        if (SUCCEEDED(hr))
            break;
    }
    if (FAILED(hr))
        return hr;

    //! Obtain DXGI factory from device (since we used nullptr for pAdapter above)
    IDXGIFactory1* dxgiFactory = nullptr;
    {
        IDXGIDevice* dxgiDevice = nullptr;
        hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
        if (SUCCEEDED(hr))
        {
            IDXGIAdapter* adapter = nullptr;
            hr = dxgiDevice->GetAdapter(&adapter);
            if (SUCCEEDED(hr))
            {
                hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
                adapter->Release();
            }
            dxgiDevice->Release();
        }
    }
    if (FAILED(hr))
        return hr;

    //? Create swap chain
    IDXGIFactory2* dxgiFactory2 = nullptr;
    hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));

    // DirectX 11.1 or later
    hr = g_pd3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&g_pd3dDevice1));
    if (SUCCEEDED(hr))
    {
        (void)g_pImmediateContextB->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&g_pImmediateContext1B));
    }

    DXGI_SWAP_CHAIN_DESC1 sd = {};
    sd.Width = width;
    sd.Height = height;
    sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 1;

    hr = dxgiFactory2->CreateSwapChainForHwnd(g_pd3dDevice, g_hWndB, &sd, nullptr, nullptr, &g_pSwapChain1B);
    if (SUCCEEDED(hr))
    {
        hr = g_pSwapChain1B->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_pSwapChainB));
    }

    dxgiFactory2->Release();

    //! Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
    dxgiFactory->MakeWindowAssociation(g_hWndB, DXGI_MWA_NO_ALT_ENTER);

    dxgiFactory->Release();

    if (FAILED(hr))
        return hr;

    //! Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChainB->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
    if (FAILED(hr))
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetViewB);
    pBackBuffer->Release();
    if (FAILED(hr))
        return hr;

    //? Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth = {};
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;

    hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &g_pDepthStencilB);
    if (FAILED(hr))
        return hr;

    //? Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencilB, &descDSV, &g_pDepthStencilViewB);
    if (FAILED(hr))
        return hr;

    g_pImmediateContextB->OMSetRenderTargets(1, &g_pRenderTargetViewB, g_pDepthStencilViewB);

    //? Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContextB->RSSetViewports(1, &vp);

    //? Compile the vertex shader
    ID3DBlob* pVSBlob1 = nullptr;
    hr = CompileShaderFromString(m_shaderB, "VS", "vs_4_0", &pVSBlob1);
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"Vertex shader compilation failed.", L"Error", MB_OK);
        return hr;
    }

    //? Create the vertex shader
    hr = g_pd3dDevice->CreateVertexShader(pVSBlob1->GetBufferPointer(), pVSBlob1->GetBufferSize(), nullptr, &g_pVertexShaderB);
    if (FAILED(hr))
    {
        pVSBlob1->Release();
        return hr;
    }

    //? Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE(layout);

    //? Create the input layout
    hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob1->GetBufferPointer(),
        pVSBlob1->GetBufferSize(), &g_pVertexLayoutB);
    pVSBlob1->Release();
    if (FAILED(hr))
        return hr;

    //? Set the input layout
    g_pImmediateContextB->IASetInputLayout(g_pVertexLayoutB);

    //? Compile the pixel shader
    ID3DBlob* pPSBlob1 = nullptr;
    hr = CompileShaderFromString(m_shaderB, "PS", "ps_4_0", &pPSBlob1);
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"Pixel shader compilation failed.", L"Error", MB_OK);
        return hr;
    }

    //? Create the pixel shader
    hr = g_pd3dDevice->CreatePixelShader(pPSBlob1->GetBufferPointer(), pPSBlob1->GetBufferSize(), nullptr, &g_pPixelShaderB);
    pPSBlob1->Release();
    if (FAILED(hr))
        return hr;

    //? Create vertex buffer
    SimpleVertex vertices[] =
    {
        { XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }, // vertex XYZ and texture map reference
        { XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) }
    };

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * 4;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem = vertices;
    hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBufferB);
    if (FAILED(hr))
        return hr;

    //? Set vertex buffer
    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    g_pImmediateContextB->IASetVertexBuffers(0, 1, &g_pVertexBufferB, &stride, &offset);

    //? Create index buffer
    //? Create vertex buffer
    WORD indices[] =
    {
        3,1,0,
        2,1,3
    };

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WORD) * 6;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    InitData.pSysMem = indices;
    hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pIndexBufferB);
    if (FAILED(hr))
        return hr;

    //? Set index buffer
    g_pImmediateContextB->IASetIndexBuffer(g_pIndexBufferB, DXGI_FORMAT_R16_UINT, 0);

    //? Set primitive topology
    g_pImmediateContextB->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    //? Create the constant buffers
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(CBNeverChanges);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pCBNeverChangesB);
    if (FAILED(hr))
        return hr;

    bd.ByteWidth = sizeof(CBChangeOnResize);
    hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pCBChangeOnResizeB);
    if (FAILED(hr))
        return hr;

    bd.ByteWidth = sizeof(CBChangesEveryFrame);
    hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pCBChangesEveryFrameB);
    if (FAILED(hr))
        return hr;

    //? Load the Texture
    /*hr = CreateDDSTextureFromFile(g_pd3dDevice, L"test.dds", nullptr, &g_pTextureRV1);
    if (FAILED(hr))
        return hr;*/

    //D3D11_SHADER_RESOURCE_VIEW_DESC& rd = {};
    //rd.Buffer = &g_pCBChangeOnResize;

    //    // g_pCBChangeOnResize
    hr = g_pSwapChainA->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&g_pRenderedTex));
    if (FAILED(hr))
        return hr;

    D3D11_SHADER_RESOURCE_VIEW_DESC rd = {};
    rd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    rd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

    hr = g_pd3dDevice->CreateShaderResourceView(g_pRenderedTex, &rd, &g_pTextureRV1);
    if (FAILED(hr))
        return hr;

    //? Create the sample state
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_pSamplerLinear1);
    if (FAILED(hr))
        return hr;

    //? Initialize the world matrices
    g_World = XMMatrixIdentity();

    //? Initialize the view matrix
    XMVECTOR Eye = XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f);    // cam rotation (Y-axis Rot, X-axis Rot, Z-axis?)
    XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);      // ? but cannot be empty/zero
    g_View = XMMatrixLookAtLH(Eye, At, Up);

    CBNeverChanges cbNeverChanges;
    cbNeverChanges.mView = XMMatrixTranspose(g_View);
    g_pImmediateContextB->UpdateSubresource(g_pCBNeverChangesB, 0, nullptr, &cbNeverChanges, 0, 0);

    //? Initialize the projection matrix
    g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / (FLOAT)height, 0.01f, 100.0f);

    CBChangeOnResize cbChangesOnResize;
    cbChangesOnResize.mProjection = XMMatrixTranspose(g_Projection);
    g_pImmediateContextB->UpdateSubresource(g_pCBChangeOnResizeB, 0, nullptr, &cbChangesOnResize, 0, 0);

    return S_OK;
}

//? --------------------------------------------------------------------------------------
//? Clean up the objects we've created
//? --------------------------------------------------------------------------------------
void CleanupDevice()
{
    if( g_pImmediateContextA ) g_pImmediateContextA->ClearState();
    if (g_pImmediateContextB) g_pImmediateContextB->ClearState();

    if( g_pSamplerLinear ) g_pSamplerLinear->Release();
    if( g_pTextureRV ) g_pTextureRV->Release();
    if( g_pCBNeverChanges ) g_pCBNeverChanges->Release();
    if( g_pCBChangeOnResize ) g_pCBChangeOnResize->Release();
    if( g_pCBChangesEveryFrame ) g_pCBChangesEveryFrame->Release();
    if( g_pSamplerLinear1 ) g_pSamplerLinear1->Release();
    if( g_pTextureRV1 ) g_pTextureRV1->Release();
    if( g_pCBNeverChangesB ) g_pCBNeverChangesB->Release();
    if( g_pCBChangeOnResizeB ) g_pCBChangeOnResizeB->Release();
    if( g_pCBChangesEveryFrameB ) g_pCBChangesEveryFrameB->Release();
    if( g_pVertexBuffer ) g_pVertexBuffer->Release();
    if( g_pIndexBuffer ) g_pIndexBuffer->Release();
    if( g_pVertexLayout ) g_pVertexLayout->Release();
    if( g_pVertexShader ) g_pVertexShader->Release();
    if( g_pPixelShader ) g_pPixelShader->Release();
    if( g_pVertexBufferB ) g_pVertexBufferB->Release();
    if( g_pIndexBufferB ) g_pIndexBufferB->Release();
    if( g_pVertexLayoutB ) g_pVertexLayoutB->Release();
    if( g_pVertexShaderB ) g_pVertexShaderB->Release();
    if( g_pPixelShaderB ) g_pPixelShaderB->Release();
    if( g_pDepthStencilA ) g_pDepthStencilA->Release();
    if( g_pDepthStencilViewA ) g_pDepthStencilViewA->Release();
    if( g_pRenderTargetViewA ) g_pRenderTargetViewA->Release();
    if( g_pSwapChain1A ) g_pSwapChain1A->Release();
    if( g_pSwapChainA ) g_pSwapChainA->Release();
    if( g_pImmediateContext1A ) g_pImmediateContext1A->Release();
    if( g_pImmediateContextA ) g_pImmediateContextA->Release();
    if( g_pDepthStencilB ) g_pDepthStencilB->Release();
    if( g_pDepthStencilViewB ) g_pDepthStencilViewB->Release();
    if( g_pRenderTargetViewB ) g_pRenderTargetViewB->Release();
    if( g_pSwapChain1B ) g_pSwapChain1B->Release();
    if( g_pSwapChainB ) g_pSwapChainB->Release();
    if( g_pImmediateContext1B ) g_pImmediateContext1B->Release();
    if( g_pImmediateContextB ) g_pImmediateContextB->Release();
    if( g_pd3dDevice1 ) g_pd3dDevice1->Release();
    if( g_pd3dDevice ) g_pd3dDevice->Release();
    if (g_pRenderedTex) g_pRenderedTex->Release();
}


//? --------------------------------------------------------------------------------------
//? Called every time the application receives a message
//? --------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
    {
    case WM_PAINT:
        hdc = BeginPaint( hWnd, &ps );
        EndPaint( hWnd, &ps );
        break;

    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;

        // Note that this tutorial does not handle resizing (WM_SIZE) requests,
        // so we created the window without the resize border.

    default:
        return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}

LRESULT CALLBACK WndProcB(HWND hWndB, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWndB, &ps);
        EndPaint(hWndB, &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

        // Note that this tutorial does not handle resizing (WM_SIZE) requests,
        // so we created the window without the resize border.

    default:
        return DefWindowProc(hWndB, message, wParam, lParam);
    }

    return 0;
}


//? --------------------------------------------------------------------------------------
//? Render a frame
//? --------------------------------------------------------------------------------------
void Render()
{
    // Update our time
    static float t = 0.0f;

    // Rotate cube around the origin
    g_World = XMMatrixRotationY( t );

    //
    // Clear the back buffer
    //
    g_pImmediateContextA->ClearRenderTargetView( g_pRenderTargetViewA, Colors::MidnightBlue );
    g_pImmediateContextB->ClearRenderTargetView(g_pRenderTargetViewB, Colors::OliveDrab);

    //
    // Clear the depth buffer to 1.0 (max depth)
    //
    g_pImmediateContextA->ClearDepthStencilView( g_pDepthStencilViewA, D3D11_CLEAR_DEPTH, 1.0f, 0 );
    g_pImmediateContextB->ClearDepthStencilView( g_pDepthStencilViewB, D3D11_CLEAR_DEPTH, 1.0f, 0 );

    //
    // Update variables that change once per frame
    //
    CBChangesEveryFrame cb;
    cb.mWorld = XMMatrixTranspose( g_World );
    cb.vMeshColor = g_vMeshColor;
    g_pImmediateContextA->UpdateSubresource( g_pCBChangesEveryFrame, 0, nullptr, &cb, 0, 0 );
    g_pImmediateContextB->UpdateSubresource( g_pCBChangesEveryFrameB, 0, nullptr, &cb, 0, 0 );

    //
    // Render the cube
    //
    g_pImmediateContextA->VSSetShader( g_pVertexShader, nullptr, 0 );
    g_pImmediateContextA->VSSetConstantBuffers( 0, 1, &g_pCBNeverChanges );
    g_pImmediateContextA->VSSetConstantBuffers( 1, 1, &g_pCBChangeOnResize );
    g_pImmediateContextA->VSSetConstantBuffers( 2, 1, &g_pCBChangesEveryFrame );
    g_pImmediateContextA->PSSetShader( g_pPixelShader, nullptr, 0 );
    g_pImmediateContextA->PSSetConstantBuffers( 2, 1, &g_pCBChangesEveryFrame );
    g_pImmediateContextA->PSSetShaderResources( 0, 1, &g_pTextureRV );
    g_pImmediateContextA->PSSetSamplers( 0, 1, &g_pSamplerLinear );
    g_pImmediateContextA->DrawIndexed( 36, 0, 0 );

    g_pImmediateContextB->VSSetShader( g_pVertexShaderB, nullptr, 0 );
    g_pImmediateContextB->VSSetConstantBuffers( 0, 1, &g_pCBNeverChangesB );
    g_pImmediateContextB->VSSetConstantBuffers( 1, 1, &g_pCBChangeOnResizeB );
    g_pImmediateContextB->VSSetConstantBuffers( 2, 1, &g_pCBChangesEveryFrameB );
    g_pImmediateContextB->PSSetShader( g_pPixelShaderB, nullptr, 0 );
    g_pImmediateContextB->PSSetConstantBuffers( 2, 1, &g_pCBChangesEveryFrameB );
    g_pImmediateContextB->PSSetShaderResources( 0, 1, &g_pTextureRV1 );
    g_pImmediateContextB->PSSetSamplers( 0, 1, &g_pSamplerLinear1 );
    g_pImmediateContextB->DrawIndexed( 36, 0, 0 );

    //
    // Present our back buffer to our front buffer
    //
    g_pSwapChainA->Present( 0, 0 );
    g_pSwapChainB->Present(0, 0);
}
