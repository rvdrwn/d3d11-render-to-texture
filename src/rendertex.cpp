#include <Windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <DirectXTex.h>
#include <string>
#include <memory>

struct vertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT2 col;
};

ID3D11VertexShader* m_pVertexShader = nullptr;
ID3D11InputLayout* m_pVertexLayout = nullptr;
ID3D11PixelShader* m_pPixelShader = nullptr;
ID3D11Buffer* m_pVertexBuffer = nullptr;
ID3D11Texture2D* m_pTextureBuffer = nullptr;
ID3D11Texture2D* m_pRenderedTexture = nullptr;
IDXGISwapChain* m_pSwapChain = nullptr;
ID3D11RenderTargetView* m_pRenderTargetView = nullptr;
ID3D11Device* m_pd3dDevice = nullptr;
ID3D11DeviceContext* m_pd3dDeviceContext = nullptr;

struct SharedMemoryData {
	void* SharedTexture;
	uint32_t gpu_device_id;
};

class SharedMemoryContext {
public:
	SharedMemoryData* m_pSharedData = nullptr;
};

std::shared_ptr<SharedMemoryContext> m_SharedMemory;

//Create device without window
HRESULT InitDeviceWithoutWindow()
{
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0,
	};

	IDXGIFactory1* pFactory = nullptr; //not sure whats this for
	IDXGIAdapter1* pRecommendedAdapter = nullptr;
	HRESULT hr = S_FALSE;
	hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&pFactory));
	if (SUCCEEDED(hr)) {
		IDXGIAdapter1* pAdapter = nullptr;
		UINT index = 0;
		while (pFactory->EnumAdapters1(index, &pAdapter) != DXGI_ERROR_NOT_FOUND) {
			DXGI_ADAPTER_DESC1 desc;
			pAdapter->GetDesc1(&desc);
			if (desc.DeviceId == m_SharedMemory->m_pSharedData->gpu_device_id) {
				pRecommendedAdapter = pAdapter;
				pRecommendedAdapter->AddRef();
				pAdapter->Release();
				break;
			}
			pAdapter->Release();
			index++;
		}
		pFactory->Release();
	}
	else {
		return hr;
	}

	hr = D3D11CreateDevice(
		pRecommendedAdapter,
		(pRecommendedAdapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE),
		NULL,
		0,
		featureLevelArray,
		2,
		D3D11_SDK_VERSION,
		&m_pd3dDevice,
		&featureLevel,
		&m_pd3dDeviceContext
	);
	if (FAILED(hr)) {
		return hr;
	}
	return S_OK;
}

//HRESULT CompileShader

//Shader
const auto v_Shader = R"V_SHADER(
	Texture2D txDiffuse : register( t0 );
	SamplerState sampLinear : register ( s0 );

	struct VS_INPUT
	{
		float4 pos : POSITION;
		float2 tex : TEXCOORD0;
	};

	struct PS_INPUT
	{
		float4 pos : SV_POSTION;
		float2 tex : TEXCOORD0; //unnecessary?
	};

	//Vertex Shader
	PS_INPUT VS( VS_INPUT input )
	{
		PS_INPUT output = (PS_INPUT)0;
		output.pos = input.pos;
		output.tex = input.tex;

		return output;
	}

	//Pixel Shader
	float4 PS( PS_INPUT input) : SV_Target
	{
		return tx.Diffuse.Sample( samLinear, input.tex );
	}
)V_SHADER";

void DestroyEverything()
{
	if (m_pVertexShader) m_pVertexShader->Release();
	if (m_pVertexLayout) m_pVertexLayout->Release();
	if (m_pPixelShader) m_pPixelShader->Release();
	if (m_pVertexBuffer) m_pVertexBuffer->Release();
	if (m_pTextureBuffer) m_pTextureBuffer->Release();
	if (m_pRenderedTexture) m_pRenderedTexture->Release();
	if (m_pSwapChain) m_pSwapChain->Release();
	if (m_pRenderTargetView) m_pRenderTargetView->Release();
	if (m_pd3dDevice) m_pd3dDevice->Release();
	if (m_pd3dDeviceContext) m_pd3dDeviceContext->Release();
}