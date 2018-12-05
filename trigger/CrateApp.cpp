//***************************************************************************************
// CrateApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "d3dApp.h"
#include "MathHelper.h"
#include "UploadBuffer.h"
#include "GeometryGenerator.h"
#include "FrameResource.h"

#include "trigger_lua.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

const int gNumFrameResources = 3;

// Lightweight structure stores parameters to draw a shape.  This will
// vary from app-to-app.
struct RenderItem
{
	RenderItem() = default;

	// World matrix of the shape that describes the object's local space
	// relative to the world space, which defines the position, orientation,
	// and scale of the object in the world.
	XMFLOAT4X4 World = MathHelper::Identity4x4();

	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;

	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT ObjCBIndex = -1;

	Material* Mat = nullptr;
	MeshGeometry* Geo = nullptr;

	// Primitive topology.
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced parameters.
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};

class CrateApp : public D3DApp
{
public:
	CrateApp(HINSTANCE hInstance);
	CrateApp(const CrateApp& rhs) = delete;
	CrateApp& operator=(const CrateApp& rhs) = delete;
	~CrateApp();

	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	virtual void OnKeyboardInput(WPARAM btnState)override;
	virtual void OnKeyboardInputUp(WPARAM btnState)override;
	void UpdateCamera(const GameTimer& gt);
	void AnimateMaterials(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);

	void BuildProperty(trigger::component *comp);

	void LoadTextures();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildShapeGeometry();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterials();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

private:
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	UINT mCbvSrvDescriptorSize = 0;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	ComPtr<ID3D12PipelineState> mOpaquePSO = nullptr;

	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mOpaqueRitems;

	PassConstants mMainPassCB;

	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();
	XMFLOAT3 direction;

	float mTheta = 1.3f*XM_PI;
	float mPhi = 0.4f*XM_PI;
	float mRadius = 2.5f;

	//TEST
	float * pos;
	float * rot;
	float * scale;
	trigger::actor *target;

	bool console_open;
	trigger::ui::console *console;

	float h = 0, v = 0;

	Camera cam;

	POINT mLastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		CrateApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

CrateApp::CrateApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{}

CrateApp::~CrateApp()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

bool CrateApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Get the increment size of a descriptor in this heap type.  This is hardware specific, 
	// so we have to query this information.
	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	trigger::tool::make_tex("../tools/512_untitled_texture.png");

	//TEST
	pos = new float[3];
	pos[0] = 0;
	pos[1] = 0;
	pos[2] = 0;

	rot = new float[3];
	rot[0] = 0;
	rot[1] = 0;
	rot[2] = 0;

	scale = new float[3];
	scale[0] = 0;
	scale[1] = 0;
	scale[2] = 0;
	cam.SetOrthographic(false);
	target = new trigger::actor();
	console = new trigger::ui::console(selected_world);
	//mEyePos = XMFLOAT3(1, 1, 1);
	cam.SetLens(0.6f * MathHelper::Pi, 1.833f, 0.00001f, 1000.0f);

	//test lua
	trigger::tlua::init(console,this->selected_world);
	trigger::tlua::run("lua/test_actor.lua");

	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildShapeGeometry();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSOs();

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	return true;
}

void CrateApp::OnResize()
{
	D3DApp::OnResize();

	cam.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 0.00001f, 1000.0f);
	//cam.SetLens(0.6f * MathHelper::Pi, 1.83f, 0.00001f, 1000.0f);
}

float tick;
void CrateApp::Update(const GameTimer& gt)
{
	UpdateCamera(gt);

	// Cycle through the circular frame resource array.
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	AnimateMaterials(gt);
	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);

	if (tick <= 1.0f)
	{
		tick += gt.DeltaTime();
	}
	else
	{
		tick = 0;
		trigger::tlua::_load_update_func(gt.DeltaTime());
	}
}


void CrateApp::Draw(const GameTimer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mOpaquePSO.Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

	DrawRenderItems(mCommandList.Get(), mOpaqueRitems);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void CrateApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	if (btnState & MK_LBUTTON)
	{
		//ray
		XMVECTOR mouseNear = XMVectorSet((float)mLastMousePos.x, (float)mLastMousePos.y, 0.0f, 0.0f);
		XMVECTOR mouseFar = XMVectorSet((float)mLastMousePos.x, (float)mLastMousePos.y, 1.0f, 0.0f);
		XMMATRIX pro = XMLoadFloat4x4(&mMainPassCB.Proj);
		XMMATRIX view = XMLoadFloat4x4(&mMainPassCB.View);

		XMVECTOR unprojected_near = XMVector3Unproject(mouseNear, 0, 0, mScreenViewport.Width, mScreenViewport.Height,
			mMainPassCB.NearZ, mMainPassCB.FarZ,
			pro, view, XMMatrixIdentity());

		XMVECTOR unprojected_far = XMVector3Unproject(mouseFar, 0, 0, mScreenViewport.Width, mScreenViewport.Height,
			mMainPassCB.NearZ, mMainPassCB.FarZ,
			pro, view, XMMatrixIdentity());
		XMVECTOR result = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(unprojected_far, unprojected_near));

		XMStoreFloat3(&direction, result);
	}


	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void CrateApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void CrateApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		cam.Pitch(dy);
		cam.RotateY(dx);
		//console.AddLog("[log] %f, %f", dx, dy);
		// Update angles based on input to orbit camera around box.
		//mTheta += dx;
		//mPhi += dy;

		// Restrict the angle mPhi.
		//mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
		//OnKeyboardInput( btnState );
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void CrateApp::OnKeyboardInputUp(WPARAM btnState)
{
	if (btnState == 87 || btnState == 0x53)
	{
		v = 0;

	}
	if (btnState == 0x41 || btnState == 0x44)
	{
		h = 0;
	}
}

void CrateApp::OnKeyboardInput(WPARAM btnState)
{
	if (btnState == 87 || btnState == 0x53)
	{
		if (btnState == 87)
		{
			v = 1;
		}
		else if (btnState == 0x53)
		{
			v = -1;
		}
	}


	if (btnState == 0x41 || btnState == 0x44)
	{
		if (btnState == 0x41)
		{
			h = 1;
		}
		if (btnState == 0x44)
		{
			h = -1;
		}
	}
}

void CrateApp::UpdateCamera(const GameTimer& gt)
{
	float velocity = 1.0f;
	if (GetAsyncKeyState(VK_LSHIFT) & 0x8000)
	{
		velocity += 5.0f;
	}
	if (GetAsyncKeyState('W') & 0x8000)
	{
		cam.Walk(velocity * gt.DeltaTime());
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{
		cam.Walk(-velocity * gt.DeltaTime());
	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		cam.Strafe(-velocity * gt.DeltaTime());
	}
	if (GetAsyncKeyState('D') & 0x8000)
	{
		cam.Strafe(velocity * gt.DeltaTime());
	}	

	if (GetAsyncKeyState('Q') * 0x8000)
	{
		auto a = cam.GetPosition3f();
		a.y += velocity * gt.DeltaTime();
		cam.SetPosition(a);
	}
	if (GetAsyncKeyState('E') * 0x8000)
	{
		auto a = cam.GetPosition3f();
		a.y -= velocity * gt.DeltaTime();
		cam.SetPosition(a);
	}

	cam.UpdateViewMatrix();
	XMMATRIX view = cam.GetView();
	XMMATRIX p = cam.GetProj();

	XMStoreFloat4x4(&mView, view);
	XMStoreFloat4x4(&mProj, p);
}

void CrateApp::AnimateMaterials(const GameTimer& gt)
{

}

void CrateApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	XMMATRIX world = XMMatrixIdentity();
	XMMATRIX texTransform = XMMatrixIdentity();
	for (auto& e : mAllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if (e->NumFramesDirty > 0)
		{
			world = world * XMLoadFloat4x4(&e->World);
			texTransform = world * XMLoadFloat4x4(&e->TexTransform);

			ObjectConstants objConstants;
			XMMATRIX x = XMMatrixRotationX(target->s_transform.rotation.x);
			XMMATRIX y = XMMatrixRotationY(target->s_transform.rotation.y);
			XMMATRIX z = XMMatrixRotationZ(target->s_transform.rotation.z);

			XMMATRIX s = XMMatrixScaling(target->s_transform.scale.x, target->s_transform.scale.y, target->s_transform.scale.z);

			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world * s * (x * y * z) * (XMMatrixTranslation(target->s_transform.position.x, target->s_transform.position.y, target->s_transform.position.z))));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			//e->NumFramesDirty--;
		}
	}
}

void CrateApp::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for (auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void CrateApp::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}



void CrateApp::LoadTextures()
{
	auto woodCrateTex = std::make_unique<Texture>();
	woodCrateTex->Name = "woodCrateTex";
	woodCrateTex->Filename = L"../tools/512_untitled_texture.DDS";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), woodCrateTex->Filename.c_str(),
		woodCrateTex->Resource, woodCrateTex->UploadHeap));

	auto woodCrateTex1 = std::make_unique<Texture>();
	woodCrateTex1->Name = "woodCrateTex1";
	woodCrateTex1->Filename = L"../tools/512_untitled_texture.DDS";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), woodCrateTex1->Filename.c_str(),
		woodCrateTex1->Resource, woodCrateTex1->UploadHeap));

	auto woodCrateTex2 = std::make_unique<Texture>();
	woodCrateTex2->Name = "woodCrateTex2";
	woodCrateTex2->Filename = L"../tools/512_untitled_texture.DDS";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), woodCrateTex2->Filename.c_str(),
		woodCrateTex2->Resource, woodCrateTex2->UploadHeap));

	mTextures[woodCrateTex->Name] = std::move(woodCrateTex);
	mTextures[woodCrateTex1->Name] = std::move(woodCrateTex1);
	mTextures[woodCrateTex2->Name] = std::move(woodCrateTex2);
}

void CrateApp::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0);
	slotRootParameter[2].InitAsConstantBufferView(1);
	slotRootParameter[3].InitAsConstantBufferView(2);

	auto staticSamplers = GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void CrateApp::BuildDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 4;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ImGui_ImplWin32_Init(mhMainWnd);
		ImGui_ImplDX12_Init(md3dDevice.Get(), 3, DXGI_FORMAT_R8G8B8A8_UNORM,
			mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	}
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	{
		auto woodCrateTex = mTextures["woodCrateTex"]->Resource;
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = woodCrateTex->GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = woodCrateTex->GetDesc().MipLevels;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		md3dDevice->CreateShaderResourceView(woodCrateTex.Get(), &srvDesc, hDescriptor);
	}
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	{
		auto woodCrateTex = mTextures["woodCrateTex1"]->Resource;
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = woodCrateTex->GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = woodCrateTex->GetDesc().MipLevels;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		md3dDevice->CreateShaderResourceView(woodCrateTex.Get(), &srvDesc, hDescriptor);
	}
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	{
		auto woodCrateTex = mTextures["woodCrateTex2"]->Resource;
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = woodCrateTex->GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = woodCrateTex->GetDesc().MipLevels;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		md3dDevice->CreateShaderResourceView(woodCrateTex.Get(), &srvDesc, hDescriptor);
	}
}


void CrateApp::BuildShadersAndInputLayout()
{
	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS", "ps_5_0");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void CrateApp::BuildShapeGeometry()
{
	GeometryGenerator geoGen;
	{
		GeometryGenerator::MeshData grid = geoGen.CreateGrid(_FMAX, _FMAX, _FMAX, _FMAX);

		SubmeshGeometry gridSubmesh;
		gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
		gridSubmesh.StartIndexLocation = 0;
		gridSubmesh.BaseVertexLocation = 0;

		std::vector<Vertex> vertices(grid.Vertices.size());

		for (size_t i = 0; i < grid.Vertices.size(); ++i)
		{
			vertices[i].Pos = grid.Vertices[i].Position;
			vertices[i].Normal = grid.Vertices[i].Normal;
			vertices[i].TexC = grid.Vertices[i].TexC;
		}

		std::vector<std::uint16_t> indices = grid.GetIndices16();

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
		const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

		auto geo = std::make_unique<MeshGeometry>();
		geo->Name = "gridGeo";

		ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
		CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
		CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
			mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

		geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
			mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

		geo->VertexByteStride = sizeof(Vertex);
		geo->VertexBufferByteSize = vbByteSize;
		geo->IndexFormat = DXGI_FORMAT_R16_UINT;
		geo->IndexBufferByteSize = ibByteSize;

		geo->DrawArgs["grid"] = gridSubmesh;

		mGeometries[geo->Name] = std::move(geo);
	}

	{
		GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);

		SubmeshGeometry boxSubmesh;
		boxSubmesh.IndexCount = (UINT)box.Indices32.size();
		boxSubmesh.StartIndexLocation = 0;
		boxSubmesh.BaseVertexLocation = 0;

		std::vector<Vertex> vertices(box.Vertices.size());

		for (size_t i = 0; i < box.Vertices.size(); ++i)
		{
			vertices[i].Pos = box.Vertices[i].Position;
			vertices[i].Normal = box.Vertices[i].Normal;
			vertices[i].TexC = box.Vertices[i].TexC;
		}

		std::vector<std::uint16_t> indices = box.GetIndices16();

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
		const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

		auto geo = std::make_unique<MeshGeometry>();
		geo->Name = "boxGeo";

		ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
		CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
		CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
			mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

		geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
			mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

		geo->VertexByteStride = sizeof(Vertex);
		geo->VertexBufferByteSize = vbByteSize;
		geo->IndexFormat = DXGI_FORMAT_R16_UINT;
		geo->IndexBufferByteSize = ibByteSize;

		geo->DrawArgs["box"] = boxSubmesh;

		mGeometries[geo->Name] = std::move(geo);
	}
	
}

void CrateApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mOpaquePSO)));
}

void CrateApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
			1, (UINT)mAllRitems.size(), (UINT)mMaterials.size()));
	}
}

void CrateApp::BuildMaterials()
{
	{
		auto woodCrate = std::make_unique<Material>();
		woodCrate->Name = "woodCrate";
		woodCrate->MatCBIndex = 0;
		woodCrate->DiffuseSrvHeapIndex = 1; // <- 이게 문제였음 ImGui 에서 할당을 한번 하니까 +1 해줘야 했음.
		woodCrate->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		woodCrate->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
		woodCrate->Roughness = 0.2f;

		mMaterials["woodCrate"] = std::move(woodCrate);
	}

	{
		auto woodCrate = std::make_unique<Material>();
		woodCrate->Name = "woodCrate2";
		woodCrate->MatCBIndex = 0;
		woodCrate->DiffuseSrvHeapIndex = 2;
		woodCrate->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		woodCrate->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
		woodCrate->Roughness = 0.2f;

		mMaterials["woodCrate2"] = std::move(woodCrate);
	}

	{
		auto woodCrate = std::make_unique<Material>();
		woodCrate->Name = "woodCrate3";
		woodCrate->MatCBIndex = 0;
		woodCrate->DiffuseSrvHeapIndex = 3;
		woodCrate->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		woodCrate->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
		woodCrate->Roughness = 0.2f;

		mMaterials["woodCrate3"] = std::move(woodCrate);
	}

}

void CrateApp::BuildRenderItems()
{
	{
		auto boxRitem = std::make_unique<RenderItem>();
		boxRitem->ObjCBIndex = 0;
		boxRitem->Mat = mMaterials["woodCrate"].get();
		boxRitem->Geo = mGeometries["boxGeo"].get();
		boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount / 3;
		boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
		boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
		mAllRitems.push_back(std::move(boxRitem));
	}

	{
		auto boxRitem = std::make_unique<RenderItem>();
		boxRitem->ObjCBIndex = 1;
		boxRitem->Mat = mMaterials["woodCrate2"].get();
		boxRitem->Geo = mGeometries["boxGeo"].get();
		boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount / 3;
		boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation + (boxRitem->Geo->DrawArgs["box"].IndexCount / 3);
		boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
		mAllRitems.push_back(std::move(boxRitem));
	}

	{
		auto boxRitem = std::make_unique<RenderItem>();
		boxRitem->ObjCBIndex = 2;
		boxRitem->Mat = mMaterials["woodCrate3"].get();
		boxRitem->Geo = mGeometries["boxGeo"].get();
		boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount / 3;
		boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation + (boxRitem->Geo->DrawArgs["box"].IndexCount / 3) * 2;
		boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
		mAllRitems.push_back(std::move(boxRitem));
	}

	{
		auto gridRitem = std::make_unique<RenderItem>();
		gridRitem->ObjCBIndex = 0;
		gridRitem->Mat = mMaterials["woodCrate3"].get();
		gridRitem->Geo = mGeometries["gridGeo"].get();
		gridRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
		gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
		gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
		gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
		mAllRitems.push_back(std::move(gridRitem));
	}

	// All the render items are opaque.
	for (auto& e : mAllRitems)
		mOpaqueRitems.push_back(e.get());
}

//TODO
void CrateApp::BuildProperty(trigger::component * comp)
{
	for (auto& j : comp->get_variables())
	{
		if (j.is_number_float())
		{
			ImGui::Separator();
			ImGui::InputFloat(comp->class_name.c_str(), &comp->time_scale);
		}
	}
}
void CrateApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{

	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();
	// For each render item...
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex*objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex*matCBByteSize;

		cmdList->SetGraphicsRootDescriptorTable(0, tex);
		cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
		cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);

	}

	static std::string path = "";
	static std::string name = "";
	static bool openFileSaveDialog = false;
	static bool openFileLoadDialog = false;
	static bool openWorldNameChange = false;
	static string world_name;
	//Draw Gui
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (openWorldNameChange)
	{
		ImGui::Begin("name change", &openWorldNameChange);
		ImGui::InputText(":World Name", &world_name);
		selected_world->set_name(world_name);
		ImGui::End();
	}

	if (openFileSaveDialog)
	{
		if (ImGuiFileDialog::Instance()->FileDialog("Save File", (const char*)0, ".", selected_world->get_name().c_str()))
		{
			if (ImGuiFileDialog::Instance()->IsOk == true)
			{
				name = ImGuiFileDialog::Instance()->GetCurrentFileName();
				if (name.find(".map", 0) == std::string::npos)
				{
					name += ".map";
				}
				path = ImGuiFileDialog::Instance()->GetCurrentPath();
				if (trigger::component_world::save_world(path, name, selected_world))
				{
					console->AddLog("[log] Save this World, %s, %s", path.c_str(), name.c_str());
				}
				else
				{
					console->AddLog("[error] Save Failed this World, %s, %s", path.c_str(), name.c_str());
				}
			}
			else
			{
				path = "";
				name = "";
			}
			openFileSaveDialog = false;
		}
	}

	if (openFileLoadDialog)
	{
		if (ImGuiFileDialog::Instance()->FileDialog("Load File", (const char*)0, ".", selected_world->get_name().c_str()))
		{
			if (ImGuiFileDialog::Instance()->IsOk == true)
			{
				name = ImGuiFileDialog::Instance()->GetCurrentFileName();
				bool already = false;
				for (auto map : worlds)
				{
					if (name.find(map->get_name() + ".map", 0) != std::string::npos)
					{
						already = true;
						break;
					}
				}

				if (already)
				{
					console->AddLog("[error] Load Failed already work in Engine, %s, %s", path.c_str(), name.c_str());
					path = "";
					name = "";
					openFileLoadDialog = false;
				}
				else if (name.find(".map", 0) == std::string::npos)
				{
					console->AddLog("[error] Load Failed this file is not Map File. %s, %s", path.c_str(), name.c_str());
					path = "";
					name = "";
					openFileLoadDialog = false;
				}
				else
				{
					path = ImGuiFileDialog::Instance()->GetCurrentPath();
					auto t = trigger::component_world::load_world(path + "/" + name);
					if (t)
					{
						worlds.push_back(t);
						console->AddLog("[log] Load World, %s, %s", path.c_str(), name.c_str());
					}
					else
					{
						console->AddLog("[error] Load Failed, %s, %s", path.c_str(), name.c_str());
						path = "";
						name = "";
					}
				}
			}
			else
			{
				path = "";
				name = "";
			}
			openFileLoadDialog = false;
		}
	}

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Action"))
		{
			if (ImGui::MenuItem("Reload Lua"))
			{
				trigger::tlua::run("lua/test_actor.lua");
			}
			if (ImGui::MenuItem("Save"))
			{
				//TODO 현재 열린 맵 저장하기!
				console->AddLog("[log]  Save! ");
			}
			if (ImGui::MenuItem("Save As"))
			{
				openFileSaveDialog = true;
				console->AddLog("[log]  Save as ... ");
			}
			if (ImGui::MenuItem("Load"))
			{
				openFileLoadDialog = true;
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Exit"))
			{
				console->AddLog("[log] Bye Bye");
				exit(0);
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Window"))
		{
			if (ImGui::MenuItem("Console"))
			{
				if (console_open)
				{
					console_open = false;
				}
				else
				{
					console_open = true;
				}
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Game"))
		{
			if (ImGui::MenuItem("World Name Change"))
			{
				if (openWorldNameChange)
				{
					openWorldNameChange = false;
				}
				else
				{
					openWorldNameChange = true;
				}
			}

			if (ImGui::MenuItem("Creat new Actor"))
			{
				auto t = new trigger::actor();
				t->name = "actor" + to_string(selected_world->get_components<trigger::actor>().size());
				selected_world->add(t);
				console->AddLog("[log] new Actor spawn in World.");
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	if (console_open)
	{
		console->Draw("Trigger Console", &console_open);
	}

	ImGui::Begin("World");
	for (auto w : worlds)
	{
		if (ImGui::TreeNode(w->get_name().c_str()))
		{
			selected_world = w;
			auto tmp = w->get_components<trigger::actor>();
			for (auto i : tmp)
			{
				if (ImGui::Selectable(i->name.c_str()))
				{
					target = i;
				}
			}
			ImGui::TreePop();
		}

	}

	ImGui::End();

	if (target != nullptr)
	{
		if (ImGui::Begin("Controller"))
		{
			if (ImGui::BeginMenu("Actions", "some Action for this Object"))
			{
				if (ImGui::MenuItem("delete"))
				{
					if (selected_world->delete_component(target))
					{
						trigger::tlua::_load_destroy_func();
						console->AddLog("[log] %s is Deleted in World!", target->name.c_str());
					}
					else
					{
						console->AddLog("[error] %s , cant Find in World!", target->name.c_str());
					}
				}
				ImGui::EndMenu();
			}
			ImGui::Separator();

			if (ImGui::Checkbox("", &target->active))
			{

			}
			ImGui::SameLine();
			if (ImGui::Checkbox("Static", &target->is_static))
			{
				if (&target->is_static)
				{
					target->s_transform.position.w = 0;
					target->s_transform.rotation.w = 0;
					target->s_transform.scale.w = 0;
				}
				else
				{
					target->s_transform.position.w = 1;
					target->s_transform.rotation.w = 1;
					target->s_transform.scale.w = 1;
				}
			}
			ImGui::SameLine();
			ImGui::InputText("Name", &target->name);
			if (!target->is_static)
			{
				ImGui::InputFloat3("position ", pos, -10, 10);
				target->s_transform.position.x = pos[0];
				target->s_transform.position.y = pos[1];
				target->s_transform.position.z = pos[2];
				ImGui::Separator();

				//deg -> rad
				ImGui::InputFloat("X", &target->s_transform.rotation.x);
				ImGui::InputFloat("Y", &target->s_transform.rotation.y);
				ImGui::InputFloat("Z", &target->s_transform.rotation.z);
				ImGui::Separator();
				ImGui::InputFloat("W", &target->s_transform.scale.x);
				ImGui::InputFloat("H", &target->s_transform.scale.y);
				ImGui::InputFloat("D", &target->s_transform.scale.z);
				ImGui::Separator();
			}
			BuildProperty(target);
			//drawGui() <- component On State

		}
		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> CrateApp::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}

