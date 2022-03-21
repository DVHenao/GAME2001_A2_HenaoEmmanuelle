/** @file Week7-2-TreeBillboardsApp.cpp
 *  @brief Tree Billboarding Demo
 *   Adding Billboarding to our previous Hills, Mountain, Crate, and Wave Demo
 * 
 *   Controls:
 *   Hold the left mouse button down and move the mouse to rotate.
 *   Hold the right mouse button down and move the mouse to zoom in and out.
 *
 *  @author Hooman Salamat
 */

#include "../Common/d3dApp.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"
#include "../Common/GeometryGenerator.h"
#include "FrameResource.h"
#include "Waves.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

const int gNumFrameResources = 3;

XMFLOAT3 pLight1Pos = { -9.25, 7, -9.25 } ;
XMFLOAT3 pLight2Pos;
XMFLOAT3 pLight3Pos;
XMFLOAT3 pLight4Pos;

XMFLOAT3 sLight1Pos = { 0, 10, -20 };
XMFLOAT3 sLight1Dir = { 0, 2, -10 };

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
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // DrawIndexedInstanced parameters.
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
};

enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	AlphaTestedTreeSprites,
	Count
};

class TreeBillboardsApp : public D3DApp
{
public:
    TreeBillboardsApp(HINSTANCE hInstance);
    TreeBillboardsApp(const TreeBillboardsApp& rhs) = delete;
    TreeBillboardsApp& operator=(const TreeBillboardsApp& rhs) = delete;
    ~TreeBillboardsApp();

    virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

    void OnKeyboardInput(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void AnimateMaterials(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void UpdateWaves(const GameTimer& gt); 

	void LoadTextures();
    void BuildRootSignature();
	void BuildDescriptorHeaps();
    void BuildShadersAndInputLayouts();
    void BuildLandGeometry();
    void BuildWavesGeometry();
	void BuildBoxGeometry();
	void BuildTreeSpritesGeometry();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildMaterials();
    void BuildRenderItems();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

    float GetHillsHeight(float x, float z)const;
    XMFLOAT3 GetHillsNormal(float x, float z)const;

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
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mStdInputLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mTreeSpriteInputLayout;

    RenderItem* mWavesRitem = nullptr;

	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

	std::unique_ptr<Waves> mWaves;

    PassConstants mMainPassCB;

	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

    float mTheta = 1.5f*XM_PI;
    float mPhi = XM_PIDIV2 - 0.1f;
    float mRadius = 50.0f;

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
        TreeBillboardsApp theApp(hInstance);
        if(!theApp.Initialize())
            return 0;

        return theApp.Run();
    }
    catch(DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

TreeBillboardsApp::TreeBillboardsApp(HINSTANCE hInstance)
    : D3DApp(hInstance)
{
}

TreeBillboardsApp::~TreeBillboardsApp()
{
    if(md3dDevice != nullptr)
        FlushCommandQueue();
}

bool TreeBillboardsApp::Initialize()
{
    if(!D3DApp::Initialize())
        return false;

    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    // Get the increment size of a descriptor in this heap type.  This is hardware specific, 
	// so we have to query this information.
    mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    mWaves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);
 
	LoadTextures();
    BuildRootSignature();
	BuildDescriptorHeaps();
    BuildShadersAndInputLayouts();
    BuildLandGeometry();
    BuildWavesGeometry();
	BuildBoxGeometry();
	BuildTreeSpritesGeometry();
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
 
void TreeBillboardsApp::OnResize()
{
    D3DApp::OnResize();

    // The window resized, so update the aspect ratio and recompute the projection matrix.
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);
}

void TreeBillboardsApp::Update(const GameTimer& gt)
{
    OnKeyboardInput(gt);
	UpdateCamera(gt);

    // Cycle through the circular frame resource array.
    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

    // Has the GPU finished processing the commands of the current frame resource?
    // If not, wait until the GPU has completed commands up to this fence point.
    if(mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

	AnimateMaterials(gt);
	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);
    UpdateWaves(gt);
}

void TreeBillboardsApp::Draw(const GameTimer& gt)
{
    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    ThrowIfFailed(cmdListAlloc->Reset());

    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Clear the back buffer and depth buffer.
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&mMainPassCB.FogColor, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Specify the buffers we are going to render to.
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

	mCommandList->SetPipelineState(mPSOs["alphaTested"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTested]);

	mCommandList->SetPipelineState(mPSOs["treeSprites"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites]);

	mCommandList->SetPipelineState(mPSOs["transparent"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Transparent]);

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

void TreeBillboardsApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void TreeBillboardsApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void TreeBillboardsApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

        // Update angles based on input to orbit camera around box.
        mTheta += dx;
        mPhi += dy;

        // Restrict the angle mPhi.
        mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
    }
    else if((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.2 unit in the scene.
        float dx = 0.2f*static_cast<float>(x - mLastMousePos.x);
        float dy = 0.2f*static_cast<float>(y - mLastMousePos.y);

        // Update the camera radius based on input.
        mRadius += dx - dy;

        // Restrict the radius.
        mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}
 
void TreeBillboardsApp::OnKeyboardInput(const GameTimer& gt)
{
}
 
void TreeBillboardsApp::UpdateCamera(const GameTimer& gt)
{
	// Convert Spherical to Cartesian coordinates.
	mEyePos.x = mRadius*sinf(mPhi)*cosf(mTheta);
	mEyePos.z = mRadius*sinf(mPhi)*sinf(mTheta);
	mEyePos.y = mRadius*cosf(mPhi);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);
}

void TreeBillboardsApp::AnimateMaterials(const GameTimer& gt)
{
	// Scroll the water material texture coordinates.
	auto waterMat = mMaterials["water"].get();

	float& tu = waterMat->MatTransform(3, 0);
	float& tv = waterMat->MatTransform(3, 1);

	tu += 0.1f * gt.DeltaTime();
	tv += 0.02f * gt.DeltaTime();

	if(tu >= 1.0f)
		tu -= 1.0f;

	if(tv >= 1.0f)
		tv -= 1.0f;

	waterMat->MatTransform(3, 0) = tu;
	waterMat->MatTransform(3, 1) = tv;

	// Material has changed, so need to update cbuffer.
	waterMat->NumFramesDirty = gNumFrameResources;
}

void TreeBillboardsApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for(auto& e : mAllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if(e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}
}

void TreeBillboardsApp::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for(auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if(mat->NumFramesDirty > 0)
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

void TreeBillboardsApp::UpdateMainPassCB(const GameTimer& gt)
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
	mMainPassCB.AmbientLight =  { 0.15f, 0.15f, 0.25f, 1.0f };

	//pointlights
	mMainPassCB.Lights[0].Position = { -20.25f, 7.0f, -20.25f};
	mMainPassCB.Lights[0].Strength = { 0.5f, 0.0f, 0.0f };

	mMainPassCB.Lights[1].Position = { -9.25f, 7.0f, 9.25f};
	mMainPassCB.Lights[1].Strength = { 0.0f, 0.0f, 0.5f };

	mMainPassCB.Lights[2].Position = { 9.25f, 5, -9.25f};
	mMainPassCB.Lights[2].Strength = { 0.5f, 0.0f, 0.0f };

	mMainPassCB.Lights[3].Position = { 9.25f, 5, 9.25f};
	mMainPassCB.Lights[3].Strength = { 0.0f, 0.0f, 0.5f };


	//spotlight
	mMainPassCB.Lights[4].Position = sLight1Pos;
	mMainPassCB.Lights[4].Direction = sLight1Dir;
	mMainPassCB.Lights[4].Strength = { 0.0f, 0.0f, 0.0f };


	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void TreeBillboardsApp::UpdateWaves(const GameTimer& gt)
{
	// Every quarter second, generate a random wave.
	static float t_base = 0.0f;
	if((mTimer.TotalTime() - t_base) >= 0.25f)
	{
		t_base += 0.25f;

		int i = MathHelper::Rand(4, mWaves->RowCount() - 5);
		int j = MathHelper::Rand(4, mWaves->ColumnCount() - 5);

		float r = MathHelper::RandF(0.2f, 0.5f);

		mWaves->Disturb(i, j, r);
	}

	// Update the wave simulation.
	mWaves->Update(gt.DeltaTime());

	// Update the wave vertex buffer with the new solution.
	auto currWavesVB = mCurrFrameResource->WavesVB.get();
	for(int i = 0; i < mWaves->VertexCount(); ++i)
	{
		Vertex v;

		v.Pos = mWaves->Position(i);
		v.Normal = mWaves->Normal(i);
		
		// Derive tex-coords from position by 
		// mapping [-w/2,w/2] --> [0,1]
		v.TexC.x = 0.5f + v.Pos.x / mWaves->Width();
		v.TexC.y = 0.5f - v.Pos.z / mWaves->Depth();

		currWavesVB->CopyData(i, v);
	}

	// Set the dynamic VB of the wave renderitem to the current frame VB.
	mWavesRitem->Geo->VertexBufferGPU = currWavesVB->Resource();
}

void TreeBillboardsApp::LoadTextures()
{
	auto grassTex = std::make_unique<Texture>();
	grassTex->Name = "grassTex";
	grassTex->Filename = L"../Textures/grass.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), grassTex->Filename.c_str(),
		grassTex->Resource, grassTex->UploadHeap));

	auto waterTex = std::make_unique<Texture>();
	waterTex->Name = "waterTex";
	waterTex->Filename = L"../Textures/water1.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), waterTex->Filename.c_str(),
		waterTex->Resource, waterTex->UploadHeap));

	auto fenceTex = std::make_unique<Texture>();
	fenceTex->Name = "fenceTex";
	fenceTex->Filename = L"../Textures/WireFence.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), fenceTex->Filename.c_str(),
		fenceTex->Resource, fenceTex->UploadHeap));

	auto brickType1Tex = std::make_unique<Texture>();
	brickType1Tex->Name = "brickType1Tex";
	brickType1Tex->Filename = L"../Textures/bricks.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), brickType1Tex->Filename.c_str(),
		brickType1Tex->Resource, brickType1Tex->UploadHeap));

	auto brickType2Tex = std::make_unique<Texture>();
	brickType2Tex->Name = "brickType2Tex";
	brickType2Tex->Filename = L"../Textures/bricks2.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), brickType2Tex->Filename.c_str(),
		brickType2Tex->Resource, brickType2Tex->UploadHeap));

	auto stoneTex = std::make_unique<Texture>();
	stoneTex->Name = "stoneTex";
	stoneTex->Filename = L"../Textures/stone.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), stoneTex->Filename.c_str(),
		stoneTex->Resource, stoneTex->UploadHeap));

	auto tileTex = std::make_unique<Texture>();
	tileTex->Name = "tileTex";
	tileTex->Filename = L"../Textures/tile.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), tileTex->Filename.c_str(),
		tileTex->Resource, tileTex->UploadHeap));


	// CHANGE THE FILENAME TO WOOD.DDS WHEN ITS WORKING AGAIN
	auto woodTex = std::make_unique<Texture>();
	woodTex->Name = "woodTex";
	woodTex->Filename = L"../Textures/tile.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), woodTex->Filename.c_str(),
		woodTex->Resource, woodTex->UploadHeap));


	auto treeArrayTex = std::make_unique<Texture>();
	treeArrayTex->Name = "treeArrayTex";
	treeArrayTex->Filename = L"../Textures/treeArray.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), treeArrayTex->Filename.c_str(),
		treeArrayTex->Resource, treeArrayTex->UploadHeap));


	mTextures[grassTex->Name] = std::move(grassTex);
	mTextures[waterTex->Name] = std::move(waterTex);
	mTextures[fenceTex->Name] = std::move(fenceTex);
	mTextures[brickType1Tex->Name] = std::move(brickType1Tex);
	mTextures[brickType2Tex->Name] = std::move(brickType2Tex);
	mTextures[stoneTex->Name] = std::move(stoneTex);
	mTextures[tileTex->Name] = std::move(tileTex);
	mTextures[woodTex->Name] = std::move(woodTex);
	mTextures[treeArrayTex->Name] = std::move(treeArrayTex);

}

void TreeBillboardsApp::BuildRootSignature()
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

    if(errorBlob != nullptr)
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

void TreeBillboardsApp::BuildDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 9;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	//
	// Fill out the heap with actual descriptors.
	//
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	auto grassTex = mTextures["grassTex"]->Resource;
	auto waterTex = mTextures["waterTex"]->Resource;
	auto fenceTex = mTextures["fenceTex"]->Resource;
	auto brickType1Tex = mTextures["brickType1Tex"]->Resource;
	auto brickType2Tex = mTextures["brickType2Tex"]->Resource;
	auto stoneTex = mTextures["stoneTex"]->Resource;
	auto tileTex = mTextures["tileTex"]->Resource;
	auto woodTex = mTextures["woodTex"]->Resource;
	auto treeArrayTex = mTextures["treeArrayTex"]->Resource;


	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = grassTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	md3dDevice->CreateShaderResourceView(grassTex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = waterTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(waterTex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = fenceTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(fenceTex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = brickType1Tex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(brickType1Tex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = brickType2Tex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(brickType2Tex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = stoneTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(stoneTex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = tileTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(tileTex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = woodTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(woodTex.Get(), &srvDesc, hDescriptor);


	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	auto desc = treeArrayTex->GetDesc();
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Format = treeArrayTex->GetDesc().Format;
	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2DArray.MipLevels = -1;
	srvDesc.Texture2DArray.FirstArraySlice = 0;
	srvDesc.Texture2DArray.ArraySize = treeArrayTex->GetDesc().DepthOrArraySize;
	md3dDevice->CreateShaderResourceView(treeArrayTex.Get(), &srvDesc, hDescriptor);
}

void TreeBillboardsApp::BuildShadersAndInputLayouts()
{
	const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", defines, "PS", "ps_5_1");
	mShaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_1");
	
	mShaders["treeSpriteVS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["treeSpriteGS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "GS", "gs_5_1");
	mShaders["treeSpritePS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", alphaTestDefines, "PS", "ps_5_1");

    mStdInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

	mTreeSpriteInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void TreeBillboardsApp::BuildLandGeometry()
{
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.0f, 160.0f, 50, 50);

    //
    // Extract the vertex elements we are interested and apply the height function to
    // each vertex.  In addition, color the vertices based on their height so we have
    // sandy looking beaches, grassy low hills, and snow mountain peaks.
    //

    std::vector<Vertex> vertices(grid.Vertices.size());
    for(size_t i = 0; i < grid.Vertices.size(); ++i)
    {
        auto& p = grid.Vertices[i].Position;
        vertices[i].Pos = p;
        vertices[i].Pos.y = 1+ GetHillsHeight(p.x, p.z);
        vertices[i].Normal = GetHillsNormal(p.x, p.z);
		vertices[i].TexC = grid.Vertices[i].TexC;
    }

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

    std::vector<std::uint16_t> indices = grid.GetIndices16();
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "landGeo";

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

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	mGeometries["landGeo"] = std::move(geo);
}

void TreeBillboardsApp::BuildWavesGeometry()
{
    std::vector<std::uint16_t> indices(3 * mWaves->TriangleCount()); // 3 indices per face
	assert(mWaves->VertexCount() < 0x0000ffff);

    // Iterate over each quad.
    int m = mWaves->RowCount();
    int n = mWaves->ColumnCount();
    int k = 0;
    for(int i = 0; i < m - 1; ++i)
    {
        for(int j = 0; j < n - 1; ++j)
        {
            indices[k] = i*n + j;
            indices[k + 1] = i*n + j + 1;
            indices[k + 2] = (i + 1)*n + j;

            indices[k + 3] = (i + 1)*n + j;
            indices[k + 4] = i*n + j + 1;
            indices[k + 5] = (i + 1)*n + j + 1;

            k += 6; // next quad
        }
    }

	UINT vbByteSize = mWaves->VertexCount()*sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size()*sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "waterGeo";

	// Set dynamically.
	geo->VertexBufferCPU = nullptr;
	geo->VertexBufferGPU = nullptr;

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	mGeometries["waterGeo"] = std::move(geo);
}

void TreeBillboardsApp::BuildBoxGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);
	GeometryGenerator::MeshData wall = geoGen.CreateBox(9.0f, 2.0f, 1.0f, 1);
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
	GeometryGenerator::MeshData wallPillar = geoGen.CreateCylinder(1.0f, 1.0f, 3.0f, 4, 4);
	GeometryGenerator::MeshData fountainPillar = geoGen.CreateCylinder(1.0f, 1.0f, 3.0f, 8, 8);
	GeometryGenerator::MeshData wallPillarTop = geoGen.CreateCylinder(1.0f, 0.0f, 1.0f, 4, 5);
	GeometryGenerator::MeshData fountainPillarTop = geoGen.CreateCylinder(1.0f, 0.0f, 1.0f, 8, 1);
	GeometryGenerator::MeshData centerFountain = geoGen.CreateCylinder(2.0f, 0.0f, 1.0f, 4, 5);

	// if things dont work, add the offset stuff here
	UINT boxVertexOffset = 0;
	UINT wallVertexOffset = (UINT)box.Vertices.size();
	UINT gridVertexOffset = wallVertexOffset + (UINT)wall.Vertices.size();
	UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
	UINT wallPillarVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();
	UINT fountainPillarVertexOffset = wallPillarVertexOffset + (UINT)wallPillar.Vertices.size();
	UINT wallPillarTopVertexOffset = fountainPillarVertexOffset + (UINT)fountainPillar.Vertices.size();
	UINT fountainPillarTopVertexOffset = wallPillarTopVertexOffset + (UINT)wallPillarTop.Vertices.size();
	UINT centerFountainVertexOffset = fountainPillarTopVertexOffset + (UINT)fountainPillarTop.Vertices.size();

	UINT boxIndexOffset = 0;
	UINT wallIndexOffset = (UINT)box.Indices32.size();
	UINT gridIndexOffset = wallIndexOffset + (UINT)wall.Indices32.size();
	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
	UINT wallPillarIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();
	UINT fountainPillarIndexOffset = wallPillarIndexOffset + (UINT)wallPillar.Indices32.size();
	UINT wallPillarTopIndexOffset = fountainPillarIndexOffset + (UINT)fountainPillar.Indices32.size();
	UINT fountainPillarTopIndexOffset = wallPillarTopIndexOffset + (UINT)wallPillarTop.Indices32.size();
	UINT centerFountainIndexOffset = fountainPillarTopIndexOffset + (UINT)fountainPillarTop.Indices32.size();

	auto totalVertexCount =
		box.Vertices.size() +
		wall.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		wallPillar.Vertices.size() +
		fountainPillar.Vertices.size() +
		wallPillarTop.Vertices.size() +
		fountainPillarTop.Vertices.size() +
		centerFountain.Vertices.size();

	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;

	// if things dont work, change the each for loop auto p into auto x, y, z etc.

	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		auto& p = box.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].TexC = box.Vertices[i].TexC;
	}

	for (size_t i = 0; i < wall.Vertices.size(); ++i, ++k)
	{
		auto& p = wall.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = wall.Vertices[i].Normal;
		vertices[k].TexC = box.Vertices[i].TexC;
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		auto& p = grid.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = grid.Vertices[i].Normal;
		vertices[k].TexC = grid.Vertices[i].TexC;
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		auto& p = sphere.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = sphere.Vertices[i].Normal;
		vertices[k].TexC = sphere.Vertices[i].TexC;
	}

	for (size_t i = 0; i < wallPillar.Vertices.size(); ++i, ++k)
	{
		auto& p = wallPillar.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = wallPillar.Vertices[i].Normal;
		vertices[k].TexC = wallPillar.Vertices[i].TexC;
	}

	for (size_t i = 0; i < fountainPillar.Vertices.size(); ++i, ++k)
	{
		auto& p = fountainPillar.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = fountainPillar.Vertices[i].Normal;
		vertices[k].TexC = fountainPillar.Vertices[i].TexC;
	}

	for (size_t i = 0; i < wallPillarTop.Vertices.size(); ++i, ++k)
	{
		auto& p = wallPillarTop.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = wallPillarTop.Vertices[i].Normal;
		vertices[k].TexC = wallPillarTop.Vertices[i].TexC;
	}

	for (size_t i = 0; i < fountainPillarTop.Vertices.size(); ++i, ++k)
	{
		auto& p = fountainPillarTop.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = fountainPillarTop.Vertices[i].Normal;
		vertices[k].TexC = fountainPillarTop.Vertices[i].TexC;
	}

	for (size_t i = 0; i < centerFountain.Vertices.size(); ++i, ++k)
	{
		auto& p = centerFountain.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = centerFountain.Vertices[i].Normal;
		vertices[k].TexC = centerFountain.Vertices[i].TexC;
	}

	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(wall.GetIndices16()), std::end(wall.GetIndices16()));
	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(wallPillar.GetIndices16()), std::end(wallPillar.GetIndices16()));
	indices.insert(indices.end(), std::begin(fountainPillar.GetIndices16()), std::end(fountainPillar.GetIndices16()));
	indices.insert(indices.end(), std::begin(wallPillarTop.GetIndices16()), std::end(wallPillarTop.GetIndices16()));
	indices.insert(indices.end(), std::begin(fountainPillarTop.GetIndices16()), std::end(fountainPillarTop.GetIndices16()));
	indices.insert(indices.end(), std::begin(centerFountain.GetIndices16()), std::end(centerFountain.GetIndices16()));


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

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)box.Indices32.size();
	submesh.StartIndexLocation = boxIndexOffset;
	submesh.BaseVertexLocation = boxVertexOffset;

	SubmeshGeometry wallSubmesh;
	wallSubmesh.IndexCount = (UINT)wall.Indices32.size();
	wallSubmesh.StartIndexLocation = wallIndexOffset;
	wallSubmesh.BaseVertexLocation = wallVertexOffset;

	SubmeshGeometry gridSubmesh;
	gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubmesh.StartIndexLocation = gridIndexOffset;
	gridSubmesh.BaseVertexLocation = gridVertexOffset;

	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.StartIndexLocation = sphereIndexOffset;
	sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

	SubmeshGeometry wallPillarSubmesh;
	wallPillarSubmesh.IndexCount = (UINT)wallPillar.Indices32.size();
	wallPillarSubmesh.StartIndexLocation = wallPillarIndexOffset;
	wallPillarSubmesh.BaseVertexLocation = wallPillarVertexOffset;

	SubmeshGeometry fountainPillarSubmesh;
	fountainPillarSubmesh.IndexCount = (UINT)wall.Indices32.size();
	fountainPillarSubmesh.StartIndexLocation = fountainPillarIndexOffset;
	fountainPillarSubmesh.BaseVertexLocation = fountainPillarVertexOffset;

	SubmeshGeometry wallPillarTopSubmesh;
	wallPillarTopSubmesh.IndexCount = (UINT)wall.Indices32.size();
	wallPillarTopSubmesh.StartIndexLocation = wallPillarTopIndexOffset;
	wallPillarTopSubmesh.BaseVertexLocation = wallPillarTopVertexOffset;

	SubmeshGeometry fountainPillarTopSubmesh;
	fountainPillarTopSubmesh.IndexCount = (UINT)wall.Indices32.size();
	fountainPillarTopSubmesh.StartIndexLocation = fountainPillarTopIndexOffset;
	fountainPillarTopSubmesh.BaseVertexLocation = fountainPillarTopVertexOffset;

	SubmeshGeometry centerFountainSubmesh;
	centerFountainSubmesh.IndexCount = (UINT)wall.Indices32.size();
	centerFountainSubmesh.StartIndexLocation = centerFountainIndexOffset;
	centerFountainSubmesh.BaseVertexLocation = centerFountainVertexOffset;

	geo->DrawArgs["box"] = submesh;
	geo->DrawArgs["wall"] = wallSubmesh;
	geo->DrawArgs["grid"] = gridSubmesh;
	geo->DrawArgs["sphere"] = sphereSubmesh;
	geo->DrawArgs["wallPillar"] = wallPillarSubmesh;
	geo->DrawArgs["fountainPillar"] = fountainPillarSubmesh;
	geo->DrawArgs["wallPillarTop"] = wallPillarTopSubmesh;
	geo->DrawArgs["fountainPillarTop"] = fountainPillarTopSubmesh;
	geo->DrawArgs["centerFountain"] = centerFountainSubmesh;

	mGeometries["boxGeo"] = std::move(geo);
}

void TreeBillboardsApp::BuildTreeSpritesGeometry()
{
	//step5
	struct TreeSpriteVertex
	{
		XMFLOAT3 Pos;
		XMFLOAT2 Size;
	};

	static const int treeCount = 16;
	std::array<TreeSpriteVertex, 16> vertices;
	//corner pieces
	vertices[0].Pos = XMFLOAT3(-15, 8, -15);
	vertices[0].Size = XMFLOAT2(20.0f, 20.0f);
	vertices[1].Pos = XMFLOAT3(15, 8, -15);
	vertices[1].Size = XMFLOAT2(20.0f, 20.0f);
	vertices[2].Pos = XMFLOAT3(-15, 8, 15);
	vertices[2].Size = XMFLOAT2(20.0f, 20.0f);
	vertices[3].Pos = XMFLOAT3(15, 8, 15);
	vertices[3].Size = XMFLOAT2(20.0f, 20.0f);

	//pathway
	vertices[4].Pos = XMFLOAT3(-5, 3, -20);
	vertices[4].Size = XMFLOAT2(5.0f, 5.0f);
	vertices[5].Pos = XMFLOAT3(-5, 3, -30);
	vertices[5].Size = XMFLOAT2(5.0f, 5.0f);

	vertices[6].Pos = XMFLOAT3(5, 1.5, -20);
	vertices[6].Size = XMFLOAT2(5.0f, 5.0f);
	vertices[7].Pos = XMFLOAT3(5, 1.5, -30);
	vertices[7].Size = XMFLOAT2(5.0f, 5.0f);

	for (UINT i = 8; i < treeCount; ++i)
	{
		float x = 10 + MathHelper::RandF(-45.0f, 45.0f);
		float z = 10 + MathHelper::RandF(-45.0f, 45.0f);
		float y = GetHillsHeight(x, z);

		// Move tree slightly above land height.
		y += 8.0f;

		vertices[i].Pos = XMFLOAT3(x, y, z);
		vertices[i].Size = XMFLOAT2(20.0f, 20.0f);
	}

	



	std::array<std::uint16_t, 16> indices =
	{
		0, 1, 2, 3, 4, 5, 6, 7,
		8, 9, 10, 11, 12, 13, 14, 15
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(TreeSpriteVertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "treeSpritesGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(TreeSpriteVertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["points"] = submesh;

	mGeometries["treeSpritesGeo"] = std::move(geo);
}

void TreeBillboardsApp::BuildPSOs()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
    ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mStdInputLayout.data(), (UINT)mStdInputLayout.size() };
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

	//there is abug with F2 key that is supposed to turn on the multisampling!
//Set4xMsaaState(true);
	//m4xMsaaState = true;

	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

	//
	// PSO for transparent objects
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//transparentPsoDesc.BlendState.AlphaToCoverageEnable = true;

	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs["transparent"])));

	//
	// PSO for alpha tested objects
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
	alphaTestedPsoDesc.PS = 
	{ 
		reinterpret_cast<BYTE*>(mShaders["alphaTestedPS"]->GetBufferPointer()),
		mShaders["alphaTestedPS"]->GetBufferSize()
	};
	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs["alphaTested"])));

	//
	// PSO for tree sprites
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC treeSpritePsoDesc = opaquePsoDesc;
	treeSpritePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpriteVS"]->GetBufferPointer()),
		mShaders["treeSpriteVS"]->GetBufferSize()
	};
	treeSpritePsoDesc.GS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpriteGS"]->GetBufferPointer()),
		mShaders["treeSpriteGS"]->GetBufferSize()
	};
	treeSpritePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpritePS"]->GetBufferPointer()),
		mShaders["treeSpritePS"]->GetBufferSize()
	};
	//step1
	treeSpritePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	treeSpritePsoDesc.InputLayout = { mTreeSpriteInputLayout.data(), (UINT)mTreeSpriteInputLayout.size() };
	treeSpritePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&treeSpritePsoDesc, IID_PPV_ARGS(&mPSOs["treeSprites"])));
}

void TreeBillboardsApp::BuildFrameResources()
{
    for(int i = 0; i < gNumFrameResources; ++i)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
            1, (UINT)mAllRitems.size(), (UINT)mMaterials.size(), mWaves->VertexCount()));
    }
}

void TreeBillboardsApp::BuildMaterials()
{
	auto grass = std::make_unique<Material>();
	grass->Name = "grass";
	grass->MatCBIndex = 0;
	grass->DiffuseSrvHeapIndex = 0;
	grass->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	grass->Roughness = 0.125f;

	// This is not a good water material definition, but we do not have all the rendering
	// tools we need (transparency, environment reflection), so we fake it for now.
	auto water = std::make_unique<Material>();
	water->Name = "water";
	water->MatCBIndex = 1;
	water->DiffuseSrvHeapIndex = 1;
	water->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	water->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	water->Roughness = 0.0f;

	auto wirefence = std::make_unique<Material>();
	wirefence->Name = "wirefence";
	wirefence->MatCBIndex = 2;
	wirefence->DiffuseSrvHeapIndex = 2;
	wirefence->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	wirefence->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	wirefence->Roughness = 0.25f;

	auto brickType1 = std::make_unique<Material>();
	brickType1->Name = "brickType1";
	brickType1->MatCBIndex = 3;
	brickType1->DiffuseSrvHeapIndex = 3;
	brickType1->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	brickType1->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	brickType1->Roughness = 0.125f;

	auto brickType2 = std::make_unique<Material>();
	brickType2->Name = "brickType2";
	brickType2->MatCBIndex = 4;
	brickType2->DiffuseSrvHeapIndex = 4;
	brickType2->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	brickType2->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	brickType2->Roughness = 0.125f;

	auto stone = std::make_unique<Material>();
	stone->Name = "stone";
	stone->MatCBIndex = 5;
	stone->DiffuseSrvHeapIndex = 5;
	stone->DiffuseAlbedo = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
	stone->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	stone->Roughness = 0.125f;

	auto tile = std::make_unique<Material>();
	tile->Name = "tile";
	tile->MatCBIndex = 6;
	tile->DiffuseSrvHeapIndex = 6;
	tile->DiffuseAlbedo = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
	tile->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	tile->Roughness = 0.125f;

	auto wood = std::make_unique<Material>();
	wood->Name = "wood";
	wood->MatCBIndex = 7;
	wood->DiffuseSrvHeapIndex = 7;
	wood->DiffuseAlbedo = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	wood->FresnelR0 = XMFLOAT3(0.5f, 0.51f, 0.51f);
	wood->Roughness = 0.125f;

	auto treeSprites = std::make_unique<Material>();
	treeSprites->Name = "treeSprites";
	treeSprites->MatCBIndex = 8;
	treeSprites->DiffuseSrvHeapIndex = 8;
	treeSprites->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	treeSprites->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	treeSprites->Roughness = 0.125f;

	mMaterials["grass"] = std::move(grass);
	mMaterials["water"] = std::move(water);
	mMaterials["wirefence"] = std::move(wirefence);
	mMaterials["brickType1"] = std::move(brickType1);
	mMaterials["brickType2"] = std::move(brickType2);
	mMaterials["stone"] = std::move(stone);
	mMaterials["tile"] = std::move(tile);
	mMaterials["wood"] = std::move(wood);
	mMaterials["treeSprites"] = std::move(treeSprites);
}

void TreeBillboardsApp::BuildRenderItems()
{

	XMVECTOR xAxis = { 1.0f,0.0f,0.0f };
	XMVECTOR yAxis = { 0.0f,1.0f,0.0f };
	XMVECTOR zAxis = { 0.0f,0.0f,1.0f };
	UINT ObjCBIndex = 4;
	float degreeRotation45 = 0.785398;
	float degreeRotation90 = 1.5708;


    auto wavesRitem = std::make_unique<RenderItem>();
    wavesRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&wavesRitem->TexTransform, XMMatrixScaling(4.0f, 4.0f, 1.0f) * XMMatrixTranslation(0.0f, 1.0f, 0.0f));
	wavesRitem->ObjCBIndex = 0;
	wavesRitem->Mat = mMaterials["water"].get();
	wavesRitem->Geo = mGeometries["waterGeo"].get();
	wavesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wavesRitem->IndexCount = wavesRitem->Geo->DrawArgs["grid"].IndexCount;
	wavesRitem->StartIndexLocation = wavesRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	wavesRitem->BaseVertexLocation = wavesRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

    mWavesRitem = wavesRitem.get();

	mRitemLayer[(int)RenderLayer::Transparent].push_back(wavesRitem.get());

    auto gridRitem = std::make_unique<RenderItem>();
    gridRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f) * XMMatrixTranslation(0.0f, 1.5f, 0.0f));
	gridRitem->ObjCBIndex = 1;
	gridRitem->Mat = mMaterials["grass"].get();
	gridRitem->Geo = mGeometries["landGeo"].get();
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
    gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
    gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());

	//center fountain
	auto centerFountainRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&centerFountainRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 2.0f, 0.0f));
	centerFountainRitem->ObjCBIndex = 2;
	centerFountainRitem->Mat = mMaterials["stone"].get();
	centerFountainRitem->Geo = mGeometries["boxGeo"].get();
	centerFountainRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	centerFountainRitem->IndexCount = centerFountainRitem->Geo->DrawArgs["centerFountain"].IndexCount;
	centerFountainRitem->StartIndexLocation = centerFountainRitem->Geo->DrawArgs["centerFountain"].StartIndexLocation;
	centerFountainRitem->BaseVertexLocation = centerFountainRitem->Geo->DrawArgs["centerFountain"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::AlphaTested].push_back(centerFountainRitem.get());

	
	auto treeSpritesRitem = std::make_unique<RenderItem>();
	treeSpritesRitem->World = MathHelper::Identity4x4();
	treeSpritesRitem->ObjCBIndex = 3;
	treeSpritesRitem->Mat = mMaterials["treeSprites"].get();
	treeSpritesRitem->Geo = mGeometries["treeSpritesGeo"].get();
	//step2
	treeSpritesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	treeSpritesRitem->IndexCount = treeSpritesRitem->Geo->DrawArgs["points"].IndexCount;
	treeSpritesRitem->StartIndexLocation = treeSpritesRitem->Geo->DrawArgs["points"].StartIndexLocation;
	treeSpritesRitem->BaseVertexLocation = treeSpritesRitem->Geo->DrawArgs["points"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites].push_back(treeSpritesRitem.get());
	
	// 5 walls (front wall is 2 walls with opening)
	for (int i = 0; i < 1; ++i)
	{
		auto wallRitemFront1 = std::make_unique<RenderItem>();
		auto wallRitemFront2 = std::make_unique<RenderItem>();
		auto wallRitemBack = std::make_unique<RenderItem>();
		auto wallRitemLeft = std::make_unique<RenderItem>();
		auto wallRitemRight = std::make_unique<RenderItem>();

		XMStoreFloat4x4(&wallRitemFront1->World, XMMatrixScaling(.75f, 2.0f, 2.0f) * XMMatrixTranslation(5.0f, 2.0f, -9.5f));
		wallRitemFront1->ObjCBIndex = ObjCBIndex++;
		wallRitemFront1->Mat = mMaterials["brickType1"].get();
		wallRitemFront1->Geo = mGeometries["boxGeo"].get();
		wallRitemFront1->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallRitemFront1->IndexCount = wallRitemFront1->Geo->DrawArgs["wall"].IndexCount;
		wallRitemFront1->StartIndexLocation = wallRitemFront1->Geo->DrawArgs["wall"].StartIndexLocation;
		wallRitemFront1->BaseVertexLocation = wallRitemFront1->Geo->DrawArgs["wall"].BaseVertexLocation;
		
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallRitemFront1.get());

		XMStoreFloat4x4(&wallRitemFront2->World, XMMatrixScaling(.75f, 2.0f, 2.0f) * XMMatrixTranslation(-5.0f, 2.0f, -9.5f));
		wallRitemFront2->ObjCBIndex = ObjCBIndex++;
		wallRitemFront2->Mat = mMaterials["brickType1"].get();
		wallRitemFront2->Geo = mGeometries["boxGeo"].get();
		wallRitemFront2->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallRitemFront2->IndexCount = wallRitemFront2->Geo->DrawArgs["wall"].IndexCount;
		wallRitemFront2->StartIndexLocation = wallRitemFront2->Geo->DrawArgs["wall"].StartIndexLocation;
		wallRitemFront2->BaseVertexLocation = wallRitemFront2->Geo->DrawArgs["wall"].BaseVertexLocation;

		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallRitemFront2.get());

		XMStoreFloat4x4(&wallRitemBack->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 2.0f, 9.5f));
		wallRitemBack->ObjCBIndex = ObjCBIndex++;
		wallRitemBack->Mat = mMaterials["brickType1"].get();
		wallRitemBack->Geo = mGeometries["boxGeo"].get();
		wallRitemBack->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallRitemBack->IndexCount = wallRitemBack->Geo->DrawArgs["wall"].IndexCount;
		wallRitemBack->StartIndexLocation = wallRitemBack->Geo->DrawArgs["wall"].StartIndexLocation;
		wallRitemBack->BaseVertexLocation = wallRitemBack->Geo->DrawArgs["wall"].BaseVertexLocation;

		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallRitemBack.get());

		XMStoreFloat4x4(&wallRitemLeft->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 2.0f, 9.5f) * XMMatrixRotationAxis(yAxis, degreeRotation90));
		wallRitemLeft->ObjCBIndex = ObjCBIndex++;
		wallRitemLeft->Mat = mMaterials["brickType1"].get();
		wallRitemLeft->Geo = mGeometries["boxGeo"].get();
		wallRitemLeft->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallRitemLeft->IndexCount = wallRitemLeft->Geo->DrawArgs["wall"].IndexCount;
		wallRitemLeft->StartIndexLocation = wallRitemLeft->Geo->DrawArgs["wall"].StartIndexLocation;
		wallRitemLeft->BaseVertexLocation = wallRitemLeft->Geo->DrawArgs["wall"].BaseVertexLocation;

		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallRitemLeft.get());


		XMStoreFloat4x4(&wallRitemRight->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 2.0f, -9.5f) * XMMatrixRotationAxis(yAxis, degreeRotation90));
		wallRitemRight->ObjCBIndex = ObjCBIndex++;
		wallRitemRight->Mat = mMaterials["brickType1"].get();
		wallRitemRight->Geo = mGeometries["boxGeo"].get();
		wallRitemRight->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallRitemRight->IndexCount = wallRitemRight->Geo->DrawArgs["wall"].IndexCount;
		wallRitemRight->StartIndexLocation = wallRitemRight->Geo->DrawArgs["wall"].StartIndexLocation;
		wallRitemRight->BaseVertexLocation = wallRitemRight->Geo->DrawArgs["wall"].BaseVertexLocation;

		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallRitemRight.get());


		mAllRitems.push_back(std::move(wallRitemFront1));
		mAllRitems.push_back(std::move(wallRitemFront2));
		mAllRitems.push_back(std::move(wallRitemBack));
		mAllRitems.push_back(std::move(wallRitemLeft));
		mAllRitems.push_back(std::move(wallRitemRight));
	}

	//fences on top of walls
	for (int i = 0; i < 1; i++)
	{
		auto wallRitemFront1FenceFront = std::make_unique<RenderItem>();
		auto wallRitemFront2FenceFront = std::make_unique<RenderItem>();
		auto wallRitemBackFenceFront = std::make_unique<RenderItem>();
		auto wallRitemLeftFenceFront = std::make_unique<RenderItem>();
		auto wallRitemRightFenceFront = std::make_unique<RenderItem>();

		auto wallRitemFront1FenceBack = std::make_unique<RenderItem>();
		auto wallRitemFront2FenceBack = std::make_unique<RenderItem>();
		auto wallRitemBackFenceBack = std::make_unique<RenderItem>();
		auto wallRitemLeftFenceBack = std::make_unique<RenderItem>();
		auto wallRitemRightFenceBack = std::make_unique<RenderItem>();

		auto wallRitemFront1FenceSide = std::make_unique<RenderItem>();
		auto wallRitemFront2FenceSide = std::make_unique<RenderItem>();

		XMStoreFloat4x4(&wallRitemFront1FenceFront->World, XMMatrixScaling(6.0f, 1.0f, 0.1f) * XMMatrixTranslation(5.0f, 4.5, -10.25f));
		wallRitemFront1FenceFront->ObjCBIndex = ObjCBIndex++;
		wallRitemFront1FenceFront->Mat = mMaterials["wirefence"].get();
		wallRitemFront1FenceFront->Geo = mGeometries["boxGeo"].get();
		wallRitemFront1FenceFront->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallRitemFront1FenceFront->IndexCount = wallRitemFront1FenceFront->Geo->DrawArgs["box"].IndexCount;
		wallRitemFront1FenceFront->StartIndexLocation = wallRitemFront1FenceFront->Geo->DrawArgs["box"].StartIndexLocation;
		wallRitemFront1FenceFront->BaseVertexLocation = wallRitemFront1FenceFront->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallRitemFront1FenceFront.get());


		XMStoreFloat4x4(&wallRitemFront2FenceFront->World, XMMatrixScaling(6.0f, 1.0f, 0.1f) * XMMatrixTranslation(-5.0f, 4.5, -10.25f));
		wallRitemFront2FenceFront->ObjCBIndex = ObjCBIndex++;
		wallRitemFront2FenceFront->Mat = mMaterials["wirefence"].get();
		wallRitemFront2FenceFront->Geo = mGeometries["boxGeo"].get();
		wallRitemFront2FenceFront->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallRitemFront2FenceFront->IndexCount = wallRitemFront2FenceFront->Geo->DrawArgs["box"].IndexCount;
		wallRitemFront2FenceFront->StartIndexLocation = wallRitemFront2FenceFront->Geo->DrawArgs["box"].StartIndexLocation;
		wallRitemFront2FenceFront->BaseVertexLocation = wallRitemFront2FenceFront->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallRitemFront2FenceFront.get());


		XMStoreFloat4x4(&wallRitemBackFenceFront->World, XMMatrixScaling(16.0f, 1.0f, 0.1f) * XMMatrixTranslation(0.0f, 4.5, 10.25f));
		wallRitemBackFenceFront->ObjCBIndex = ObjCBIndex++;
		wallRitemBackFenceFront->Mat = mMaterials["wirefence"].get();
		wallRitemBackFenceFront->Geo = mGeometries["boxGeo"].get();
		wallRitemBackFenceFront->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallRitemBackFenceFront->IndexCount = wallRitemBackFenceFront->Geo->DrawArgs["box"].IndexCount;
		wallRitemBackFenceFront->StartIndexLocation = wallRitemBackFenceFront->Geo->DrawArgs["box"].StartIndexLocation;
		wallRitemBackFenceFront->BaseVertexLocation = wallRitemBackFenceFront->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallRitemBackFenceFront.get());


		XMStoreFloat4x4(&wallRitemLeftFenceFront->World, XMMatrixScaling(16.0f, 1.0f, 0.1f) * XMMatrixTranslation(0.0f, 4.5, 10.25f) * XMMatrixRotationAxis(yAxis, degreeRotation90));
		wallRitemLeftFenceFront->ObjCBIndex = ObjCBIndex++;
		wallRitemLeftFenceFront->Mat = mMaterials["wirefence"].get();
		wallRitemLeftFenceFront->Geo = mGeometries["boxGeo"].get();
		wallRitemLeftFenceFront->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallRitemLeftFenceFront->IndexCount = wallRitemLeftFenceFront->Geo->DrawArgs["box"].IndexCount;
		wallRitemLeftFenceFront->StartIndexLocation = wallRitemLeftFenceFront->Geo->DrawArgs["box"].StartIndexLocation;
		wallRitemLeftFenceFront->BaseVertexLocation = wallRitemLeftFenceFront->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallRitemLeftFenceFront.get());


		XMStoreFloat4x4(&wallRitemRightFenceFront->World, XMMatrixScaling(16.0f, 1.0f, 0.1f) * XMMatrixTranslation(0.0f, 4.5, -10.25f) * XMMatrixRotationAxis(yAxis, degreeRotation90));
		wallRitemRightFenceFront->ObjCBIndex = ObjCBIndex++;
		wallRitemRightFenceFront->Mat = mMaterials["wirefence"].get();
		wallRitemRightFenceFront->Geo = mGeometries["boxGeo"].get();
		wallRitemRightFenceFront->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallRitemRightFenceFront->IndexCount = wallRitemRightFenceFront->Geo->DrawArgs["box"].IndexCount;
		wallRitemRightFenceFront->StartIndexLocation = wallRitemRightFenceFront->Geo->DrawArgs["box"].StartIndexLocation;
		wallRitemRightFenceFront->BaseVertexLocation = wallRitemRightFenceFront->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallRitemRightFenceFront.get());

		//BACK FENCES//

		XMStoreFloat4x4(&wallRitemFront1FenceBack->World, XMMatrixScaling(6.0f, 1.0f, 0.1f) * XMMatrixTranslation(5.0f, 4.5, -8.75f));
		wallRitemFront1FenceBack->ObjCBIndex = ObjCBIndex++;
		wallRitemFront1FenceBack->Mat = mMaterials["wirefence"].get();
		wallRitemFront1FenceBack->Geo = mGeometries["boxGeo"].get();
		wallRitemFront1FenceBack->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallRitemFront1FenceBack->IndexCount = wallRitemFront1FenceBack->Geo->DrawArgs["box"].IndexCount;
		wallRitemFront1FenceBack->StartIndexLocation = wallRitemFront1FenceBack->Geo->DrawArgs["box"].StartIndexLocation;
		wallRitemFront1FenceBack->BaseVertexLocation = wallRitemFront1FenceBack->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallRitemFront1FenceBack.get());


		XMStoreFloat4x4(&wallRitemFront2FenceBack->World, XMMatrixScaling(6.0f, 1.0f, 0.1f) * XMMatrixTranslation(-5.0f, 4.5, -8.75f));
		wallRitemFront2FenceBack->ObjCBIndex = ObjCBIndex++;
		wallRitemFront2FenceBack->Mat = mMaterials["wirefence"].get();
		wallRitemFront2FenceBack->Geo = mGeometries["boxGeo"].get();
		wallRitemFront2FenceBack->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallRitemFront2FenceBack->IndexCount = wallRitemFront2FenceBack->Geo->DrawArgs["box"].IndexCount;
		wallRitemFront2FenceBack->StartIndexLocation = wallRitemFront2FenceBack->Geo->DrawArgs["box"].StartIndexLocation;
		wallRitemFront2FenceBack->BaseVertexLocation = wallRitemFront2FenceBack->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallRitemFront2FenceBack.get());


		XMStoreFloat4x4(&wallRitemBackFenceBack->World, XMMatrixScaling(16.0f, 1.0f, 0.1f) * XMMatrixTranslation(0.0f, 4.5, 8.75f));
		wallRitemBackFenceBack->ObjCBIndex = ObjCBIndex++;
		wallRitemBackFenceBack->Mat = mMaterials["wirefence"].get();
		wallRitemBackFenceBack->Geo = mGeometries["boxGeo"].get();
		wallRitemBackFenceBack->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallRitemBackFenceBack->IndexCount = wallRitemBackFenceBack->Geo->DrawArgs["box"].IndexCount;
		wallRitemBackFenceBack->StartIndexLocation = wallRitemBackFenceBack->Geo->DrawArgs["box"].StartIndexLocation;
		wallRitemBackFenceBack->BaseVertexLocation = wallRitemBackFenceBack->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallRitemBackFenceBack.get());


		XMStoreFloat4x4(&wallRitemLeftFenceBack->World, XMMatrixScaling(16.0f, 1.0f, 0.1f) * XMMatrixTranslation(0.0f, 4.5, 8.75f) * XMMatrixRotationAxis(yAxis, degreeRotation90));
		wallRitemLeftFenceBack->ObjCBIndex = ObjCBIndex++;
		wallRitemLeftFenceBack->Mat = mMaterials["wirefence"].get();
		wallRitemLeftFenceBack->Geo = mGeometries["boxGeo"].get();
		wallRitemLeftFenceBack->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallRitemLeftFenceBack->IndexCount = wallRitemLeftFenceBack->Geo->DrawArgs["box"].IndexCount;
		wallRitemLeftFenceBack->StartIndexLocation = wallRitemLeftFenceBack->Geo->DrawArgs["box"].StartIndexLocation;
		wallRitemLeftFenceBack->BaseVertexLocation = wallRitemLeftFenceBack->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallRitemLeftFenceBack.get());


		XMStoreFloat4x4(&wallRitemRightFenceBack->World, XMMatrixScaling(16.0f, 1.0f, 0.1f) * XMMatrixTranslation(0.0f, 4.5, -8.75f) * XMMatrixRotationAxis(yAxis, degreeRotation90));
		wallRitemRightFenceBack->ObjCBIndex = ObjCBIndex++;
		wallRitemRightFenceBack->Mat = mMaterials["wirefence"].get();
		wallRitemRightFenceBack->Geo = mGeometries["boxGeo"].get();
		wallRitemRightFenceBack->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallRitemRightFenceBack->IndexCount = wallRitemRightFenceBack->Geo->DrawArgs["box"].IndexCount;
		wallRitemRightFenceBack->StartIndexLocation = wallRitemRightFenceBack->Geo->DrawArgs["box"].StartIndexLocation;
		wallRitemRightFenceBack->BaseVertexLocation = wallRitemRightFenceBack->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallRitemRightFenceBack.get());

		//SIDE FENCES//

		XMStoreFloat4x4(&wallRitemFront1FenceSide->World, XMMatrixScaling(1.5f, 1.0f, 0.1f) * XMMatrixTranslation(9.5f, 4.5, 2.0f) * XMMatrixRotationAxis(yAxis, degreeRotation90));
		wallRitemFront1FenceSide->ObjCBIndex = ObjCBIndex++;
		wallRitemFront1FenceSide->Mat = mMaterials["wirefence"].get();
		wallRitemFront1FenceSide->Geo = mGeometries["boxGeo"].get();
		wallRitemFront1FenceSide->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallRitemFront1FenceSide->IndexCount = wallRitemFront1FenceSide->Geo->DrawArgs["box"].IndexCount;
		wallRitemFront1FenceSide->StartIndexLocation = wallRitemFront1FenceSide->Geo->DrawArgs["box"].StartIndexLocation;
		wallRitemFront1FenceSide->BaseVertexLocation = wallRitemFront1FenceSide->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallRitemFront1FenceSide.get());


		XMStoreFloat4x4(&wallRitemFront2FenceSide->World, XMMatrixScaling(1.5f, 1.0f, 0.1f) * XMMatrixTranslation(9.5f, 4.5, -2.0f) * XMMatrixRotationAxis(yAxis, degreeRotation90));
		wallRitemFront2FenceSide->ObjCBIndex = ObjCBIndex++;
		wallRitemFront2FenceSide->Mat = mMaterials["wirefence"].get();
		wallRitemFront2FenceSide->Geo = mGeometries["boxGeo"].get();
		wallRitemFront2FenceSide->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallRitemFront2FenceSide->IndexCount = wallRitemFront2FenceSide->Geo->DrawArgs["box"].IndexCount;
		wallRitemFront2FenceSide->StartIndexLocation = wallRitemFront2FenceSide->Geo->DrawArgs["box"].StartIndexLocation;
		wallRitemFront2FenceSide->BaseVertexLocation = wallRitemFront2FenceSide->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallRitemFront2FenceSide.get());


		mAllRitems.push_back(std::move(wallRitemFront1FenceFront));
		mAllRitems.push_back(std::move(wallRitemFront2FenceFront));
		mAllRitems.push_back(std::move(wallRitemBackFenceFront));
		mAllRitems.push_back(std::move(wallRitemLeftFenceFront));
		mAllRitems.push_back(std::move(wallRitemRightFenceFront));

		mAllRitems.push_back(std::move(wallRitemFront1FenceBack));
		mAllRitems.push_back(std::move(wallRitemFront2FenceBack));
		mAllRitems.push_back(std::move(wallRitemBackFenceBack));
		mAllRitems.push_back(std::move(wallRitemLeftFenceBack));
		mAllRitems.push_back(std::move(wallRitemRightFenceBack));

		mAllRitems.push_back(std::move(wallRitemFront1FenceSide));
		mAllRitems.push_back(std::move(wallRitemFront2FenceSide));
	}

	// 4 pillars
	for (int i = 0; i < 1; ++i)
	{
		auto wallPillarFLRitem = std::make_unique<RenderItem>();
		auto wallPillarFRRitem = std::make_unique<RenderItem>();
		auto wallPillarBLRitem = std::make_unique<RenderItem>();
		auto wallPillarBRRitem = std::make_unique<RenderItem>();

		//XMStoreFloat4x4(&wallRitemFront->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 1.0f, -10.0f) * XMMatrixRotationAxis(yAxis, 1.57));

		XMStoreFloat4x4(&wallPillarFLRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 3.0f, -13.0f) * XMMatrixRotationAxis(yAxis, degreeRotation45));
		wallPillarFLRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarFLRitem->Mat = mMaterials["brickType2"].get();
		wallPillarFLRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarFLRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarFLRitem->IndexCount = wallPillarFLRitem->Geo->DrawArgs["wallPillar"].IndexCount;
		wallPillarFLRitem->StartIndexLocation = wallPillarFLRitem->Geo->DrawArgs["wallPillar"].StartIndexLocation;
		wallPillarFLRitem->BaseVertexLocation = wallPillarFLRitem->Geo->DrawArgs["wallPillar"].BaseVertexLocation;
		
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarFLRitem.get());


		XMStoreFloat4x4(&wallPillarFRRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(13.0f, 3.0f, 0.0f) * XMMatrixRotationAxis(yAxis, degreeRotation45));
		wallPillarFRRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarFRRitem->Mat = mMaterials["brickType2"].get();
		wallPillarFRRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarFRRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarFRRitem->IndexCount = wallPillarFRRitem->Geo->DrawArgs["wallPillar"].IndexCount;
		wallPillarFRRitem->StartIndexLocation = wallPillarFRRitem->Geo->DrawArgs["wallPillar"].StartIndexLocation;
		wallPillarFRRitem->BaseVertexLocation = wallPillarFRRitem->Geo->DrawArgs["wallPillar"].BaseVertexLocation;
		
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarFRRitem.get());

		XMStoreFloat4x4(&wallPillarBLRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(-13.0f, 3.0f, 0.0f) * XMMatrixRotationAxis(yAxis, degreeRotation45));
		wallPillarBLRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarBLRitem->Mat = mMaterials["brickType2"].get();
		wallPillarBLRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarBLRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarBLRitem->IndexCount = wallPillarBLRitem->Geo->DrawArgs["wallPillar"].IndexCount;
		wallPillarBLRitem->StartIndexLocation = wallPillarBLRitem->Geo->DrawArgs["wallPillar"].StartIndexLocation;
		wallPillarBLRitem->BaseVertexLocation = wallPillarBLRitem->Geo->DrawArgs["wallPillar"].BaseVertexLocation;
		
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarBLRitem.get());

		XMStoreFloat4x4(&wallPillarBRRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 3.0f, 13.0f) * XMMatrixRotationAxis(yAxis, degreeRotation45));
		wallPillarBRRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarBRRitem->Mat = mMaterials["brickType2"].get();
		wallPillarBRRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarBRRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarBRRitem->IndexCount = wallPillarBRRitem->Geo->DrawArgs["wallPillar"].IndexCount;
		wallPillarBRRitem->StartIndexLocation = wallPillarBRRitem->Geo->DrawArgs["wallPillar"].StartIndexLocation;
		wallPillarBRRitem->BaseVertexLocation = wallPillarBRRitem->Geo->DrawArgs["wallPillar"].BaseVertexLocation;

		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarBRRitem.get());

		mAllRitems.push_back(std::move(wallPillarFLRitem));
		mAllRitems.push_back(std::move(wallPillarFRRitem));
		mAllRitems.push_back(std::move(wallPillarBLRitem));
		mAllRitems.push_back(std::move(wallPillarBRRitem));
	}

	// 4 pillar tops
	for (int i = 0; i < 1; ++i)
	{
		auto wallPillarFLTopRitem = std::make_unique<RenderItem>();
		auto wallPillarFRTopRitem = std::make_unique<RenderItem>();
		auto wallPillarBLTopRitem = std::make_unique<RenderItem>();
		auto wallPillarBRTopRitem = std::make_unique<RenderItem>();

		//XMStoreFloat4x4(&wallRitemFront->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 1.0f, -10.0f) * XMMatrixRotationAxis(yAxis, 1.57));

		XMStoreFloat4x4(&wallPillarFLTopRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 6.0f, -13.0f) * XMMatrixRotationAxis(yAxis, degreeRotation45));
		wallPillarFLTopRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarFLTopRitem->Mat = mMaterials["stone"].get();
		wallPillarFLTopRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarFLTopRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarFLTopRitem->IndexCount = wallPillarFLTopRitem->Geo->DrawArgs["wallPillarTop"].IndexCount;
		wallPillarFLTopRitem->StartIndexLocation = wallPillarFLTopRitem->Geo->DrawArgs["wallPillarTop"].StartIndexLocation;
		wallPillarFLTopRitem->BaseVertexLocation = wallPillarFLTopRitem->Geo->DrawArgs["wallPillarTop"].BaseVertexLocation;

		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarFLTopRitem.get());

		XMStoreFloat4x4(&wallPillarFRTopRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(13.0f, 6.0f, 0.0f) * XMMatrixRotationAxis(yAxis, degreeRotation45));
		wallPillarFRTopRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarFRTopRitem->Mat = mMaterials["stone"].get();
		wallPillarFRTopRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarFRTopRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarFRTopRitem->IndexCount = wallPillarFRTopRitem->Geo->DrawArgs["wallPillarTop"].IndexCount;
		wallPillarFRTopRitem->StartIndexLocation = wallPillarFRTopRitem->Geo->DrawArgs["wallPillarTop"].StartIndexLocation;
		wallPillarFRTopRitem->BaseVertexLocation = wallPillarFRTopRitem->Geo->DrawArgs["wallPillarTop"].BaseVertexLocation;

		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarFRTopRitem.get());

		XMStoreFloat4x4(&wallPillarBLTopRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(-13.0f, 6.0f, 0.0f) * XMMatrixRotationAxis(yAxis, degreeRotation45));
		wallPillarBLTopRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarBLTopRitem->Mat = mMaterials["stone"].get();
		wallPillarBLTopRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarBLTopRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarBLTopRitem->IndexCount = wallPillarBLTopRitem->Geo->DrawArgs["wallPillarTop"].IndexCount;
		wallPillarBLTopRitem->StartIndexLocation = wallPillarBLTopRitem->Geo->DrawArgs["wallPillarTop"].StartIndexLocation;
		wallPillarBLTopRitem->BaseVertexLocation = wallPillarBLTopRitem->Geo->DrawArgs["wallPillarTop"].BaseVertexLocation;

		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarBLTopRitem.get());

		XMStoreFloat4x4(&wallPillarBRTopRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 6.0f, 13.0f) * XMMatrixRotationAxis(yAxis, degreeRotation45));
		wallPillarBRTopRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarBRTopRitem->Mat = mMaterials["stone"].get();
		wallPillarBRTopRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarBRTopRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarBRTopRitem->IndexCount = wallPillarBRTopRitem->Geo->DrawArgs["wallPillarTop"].IndexCount;
		wallPillarBRTopRitem->StartIndexLocation = wallPillarBRTopRitem->Geo->DrawArgs["wallPillarTop"].StartIndexLocation;
		wallPillarBRTopRitem->BaseVertexLocation = wallPillarBRTopRitem->Geo->DrawArgs["wallPillarTop"].BaseVertexLocation;

		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarBRTopRitem.get());

		mAllRitems.push_back(std::move(wallPillarFLTopRitem));
		mAllRitems.push_back(std::move(wallPillarFRTopRitem));
		mAllRitems.push_back(std::move(wallPillarBLTopRitem));
		mAllRitems.push_back(std::move(wallPillarBRTopRitem));
	}

	// blocks on top of pillars around pillar tops
	for (int i = 0; i < 1; ++i)
	{
		//front left pillar tops
		auto wallPillarFLTopFLBlockRitem = std::make_unique<RenderItem>();
		auto wallPillarFLTopFRBlockRitem = std::make_unique<RenderItem>();
		auto wallPillarFLTopBLBlockRitem = std::make_unique<RenderItem>();
		auto wallPillarFLTopBRBlockRitem = std::make_unique<RenderItem>();

		//front right pillar tops
		auto wallPillarFRTopFLBlockRitem = std::make_unique<RenderItem>();
		auto wallPillarFRTopFRBlockRitem = std::make_unique<RenderItem>();
		auto wallPillarFRTopBLBlockRitem = std::make_unique<RenderItem>();
		auto wallPillarFRTopBRBlockRitem = std::make_unique<RenderItem>();

		//back left pillar tops
		auto wallPillarBLTopFLBlockRitem = std::make_unique<RenderItem>();
		auto wallPillarBLTopFRBlockRitem = std::make_unique<RenderItem>();
		auto wallPillarBLTopBLBlockRitem = std::make_unique<RenderItem>();
		auto wallPillarBLTopBRBlockRitem = std::make_unique<RenderItem>();

		//back right pillar tops
		auto wallPillarBRTopFLBlockRitem = std::make_unique<RenderItem>();
		auto wallPillarBRTopFRBlockRitem = std::make_unique<RenderItem>();
		auto wallPillarBRTopBLBlockRitem = std::make_unique<RenderItem>();
		auto wallPillarBRTopBRBlockRitem = std::make_unique<RenderItem>();

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

		XMStoreFloat4x4(&wallPillarFLTopFLBlockRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-10.1f, 6.5f, -10.1f));
		wallPillarFLTopFLBlockRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarFLTopFLBlockRitem->Mat = mMaterials["brickType2"].get();
		wallPillarFLTopFLBlockRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarFLTopFLBlockRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarFLTopFLBlockRitem->IndexCount = wallPillarFLTopFLBlockRitem->Geo->DrawArgs["box"].IndexCount;
		wallPillarFLTopFLBlockRitem->StartIndexLocation = wallPillarFLTopFLBlockRitem->Geo->DrawArgs["box"].StartIndexLocation;
		wallPillarFLTopFLBlockRitem->BaseVertexLocation = wallPillarFLTopFLBlockRitem->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarFLTopFLBlockRitem.get());


		XMStoreFloat4x4(&wallPillarFLTopFRBlockRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-8.2f, 6.5f, -10.1f));
		wallPillarFLTopFRBlockRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarFLTopFRBlockRitem->Mat = mMaterials["brickType2"].get();
		wallPillarFLTopFRBlockRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarFLTopFRBlockRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarFLTopFRBlockRitem->IndexCount = wallPillarFLTopFRBlockRitem->Geo->DrawArgs["box"].IndexCount;
		wallPillarFLTopFRBlockRitem->StartIndexLocation = wallPillarFLTopFRBlockRitem->Geo->DrawArgs["box"].StartIndexLocation;
		wallPillarFLTopFRBlockRitem->BaseVertexLocation = wallPillarFLTopFRBlockRitem->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarFLTopFRBlockRitem.get());


		XMStoreFloat4x4(&wallPillarFLTopBLBlockRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-10.1f, 6.5f, -8.2f));
		wallPillarFLTopBLBlockRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarFLTopBLBlockRitem->Mat = mMaterials["brickType2"].get();
		wallPillarFLTopBLBlockRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarFLTopBLBlockRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarFLTopBLBlockRitem->IndexCount = wallPillarFLTopBLBlockRitem->Geo->DrawArgs["box"].IndexCount;
		wallPillarFLTopBLBlockRitem->StartIndexLocation = wallPillarFLTopBLBlockRitem->Geo->DrawArgs["box"].StartIndexLocation;
		wallPillarFLTopBLBlockRitem->BaseVertexLocation = wallPillarFLTopBLBlockRitem->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarFLTopBLBlockRitem.get());


		XMStoreFloat4x4(&wallPillarFLTopBRBlockRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-8.2f, 6.5f, -8.2f));
		wallPillarFLTopBRBlockRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarFLTopBRBlockRitem->Mat = mMaterials["brickType2"].get();
		wallPillarFLTopBRBlockRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarFLTopBRBlockRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarFLTopBRBlockRitem->IndexCount = wallPillarFLTopBRBlockRitem->Geo->DrawArgs["box"].IndexCount;
		wallPillarFLTopBRBlockRitem->StartIndexLocation = wallPillarFLTopBRBlockRitem->Geo->DrawArgs["box"].StartIndexLocation;
		wallPillarFLTopBRBlockRitem->BaseVertexLocation = wallPillarFLTopBRBlockRitem->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarFLTopBRBlockRitem.get());

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

		XMStoreFloat4x4(&wallPillarFRTopFLBlockRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(8.2f, 6.5f, -10.1f));
		wallPillarFRTopFLBlockRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarFRTopFLBlockRitem->Mat = mMaterials["brickType2"].get();
		wallPillarFRTopFLBlockRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarFRTopFLBlockRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarFRTopFLBlockRitem->IndexCount = wallPillarFRTopFLBlockRitem->Geo->DrawArgs["box"].IndexCount;
		wallPillarFRTopFLBlockRitem->StartIndexLocation = wallPillarFRTopFLBlockRitem->Geo->DrawArgs["box"].StartIndexLocation;
		wallPillarFRTopFLBlockRitem->BaseVertexLocation = wallPillarFRTopFLBlockRitem->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarFRTopFLBlockRitem.get());


		XMStoreFloat4x4(&wallPillarFRTopFRBlockRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(10.1f, 6.5f, -10.1f));
		wallPillarFRTopFRBlockRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarFRTopFRBlockRitem->Mat = mMaterials["brickType2"].get();
		wallPillarFRTopFRBlockRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarFRTopFRBlockRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarFRTopFRBlockRitem->IndexCount = wallPillarFRTopFRBlockRitem->Geo->DrawArgs["box"].IndexCount;
		wallPillarFRTopFRBlockRitem->StartIndexLocation = wallPillarFRTopFRBlockRitem->Geo->DrawArgs["box"].StartIndexLocation;
		wallPillarFRTopFRBlockRitem->BaseVertexLocation = wallPillarFRTopFRBlockRitem->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarFRTopFRBlockRitem.get());

		XMStoreFloat4x4(&wallPillarFRTopBLBlockRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(8.2f, 6.5f, -8.2f));
		wallPillarFRTopBLBlockRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarFRTopBLBlockRitem->Mat = mMaterials["brickType2"].get();
		wallPillarFRTopBLBlockRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarFRTopBLBlockRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarFRTopBLBlockRitem->IndexCount = wallPillarFRTopBLBlockRitem->Geo->DrawArgs["box"].IndexCount;
		wallPillarFRTopBLBlockRitem->StartIndexLocation = wallPillarFRTopBLBlockRitem->Geo->DrawArgs["box"].StartIndexLocation;
		wallPillarFRTopBLBlockRitem->BaseVertexLocation = wallPillarFRTopBLBlockRitem->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarFRTopBLBlockRitem.get());

		XMStoreFloat4x4(&wallPillarFRTopBRBlockRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(10.1f, 6.5f, -8.2f));
		wallPillarFRTopBRBlockRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarFRTopBRBlockRitem->Mat = mMaterials["brickType2"].get();
		wallPillarFRTopBRBlockRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarFRTopBRBlockRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarFRTopBRBlockRitem->IndexCount = wallPillarFRTopBRBlockRitem->Geo->DrawArgs["box"].IndexCount;
		wallPillarFRTopBRBlockRitem->StartIndexLocation = wallPillarFRTopBRBlockRitem->Geo->DrawArgs["box"].StartIndexLocation;
		wallPillarFRTopBRBlockRitem->BaseVertexLocation = wallPillarFRTopBRBlockRitem->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarFRTopBRBlockRitem.get());

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

		XMStoreFloat4x4(&wallPillarBLTopFLBlockRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-10.1f, 6.5f, 8.2f));
		wallPillarBLTopFLBlockRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarBLTopFLBlockRitem->Mat = mMaterials["brickType2"].get();
		wallPillarBLTopFLBlockRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarBLTopFLBlockRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarBLTopFLBlockRitem->IndexCount = wallPillarBLTopFLBlockRitem->Geo->DrawArgs["box"].IndexCount;
		wallPillarBLTopFLBlockRitem->StartIndexLocation = wallPillarBLTopFLBlockRitem->Geo->DrawArgs["box"].StartIndexLocation;
		wallPillarBLTopFLBlockRitem->BaseVertexLocation = wallPillarBLTopFLBlockRitem->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarBLTopFLBlockRitem.get());


		XMStoreFloat4x4(&wallPillarBLTopFRBlockRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-8.2f, 6.5f, 8.2));
		wallPillarBLTopFRBlockRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarBLTopFRBlockRitem->Mat = mMaterials["brickType2"].get();
		wallPillarBLTopFRBlockRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarBLTopFRBlockRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarBLTopFRBlockRitem->IndexCount = wallPillarBLTopFRBlockRitem->Geo->DrawArgs["box"].IndexCount;
		wallPillarBLTopFRBlockRitem->StartIndexLocation = wallPillarBLTopFRBlockRitem->Geo->DrawArgs["box"].StartIndexLocation;
		wallPillarBLTopFRBlockRitem->BaseVertexLocation = wallPillarBLTopFRBlockRitem->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarBLTopFRBlockRitem.get());

		XMStoreFloat4x4(&wallPillarBLTopBLBlockRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-10.1f, 6.5f, 10.1f));
		wallPillarBLTopBLBlockRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarBLTopBLBlockRitem->Mat = mMaterials["brickType2"].get();
		wallPillarBLTopBLBlockRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarBLTopBLBlockRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarBLTopBLBlockRitem->IndexCount = wallPillarBLTopBLBlockRitem->Geo->DrawArgs["box"].IndexCount;
		wallPillarBLTopBLBlockRitem->StartIndexLocation = wallPillarBLTopBLBlockRitem->Geo->DrawArgs["box"].StartIndexLocation;
		wallPillarBLTopBLBlockRitem->BaseVertexLocation = wallPillarBLTopBLBlockRitem->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarBLTopBLBlockRitem.get());

		XMStoreFloat4x4(&wallPillarBLTopBRBlockRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-8.2f, 6.5f, 10.1f));
		wallPillarBLTopBRBlockRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarBLTopBRBlockRitem->Mat = mMaterials["brickType2"].get();
		wallPillarBLTopBRBlockRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarBLTopBRBlockRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarBLTopBRBlockRitem->IndexCount = wallPillarBLTopBRBlockRitem->Geo->DrawArgs["box"].IndexCount;
		wallPillarBLTopBRBlockRitem->StartIndexLocation = wallPillarBLTopBRBlockRitem->Geo->DrawArgs["box"].StartIndexLocation;
		wallPillarBLTopBRBlockRitem->BaseVertexLocation = wallPillarBLTopBRBlockRitem->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarBLTopBRBlockRitem.get());

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

		XMStoreFloat4x4(&wallPillarBRTopFLBlockRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(8.2f, 6.5f, 8.2f));
		wallPillarBRTopFLBlockRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarBRTopFLBlockRitem->Mat = mMaterials["brickType2"].get();
		wallPillarBRTopFLBlockRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarBRTopFLBlockRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarBRTopFLBlockRitem->IndexCount = wallPillarBRTopFLBlockRitem->Geo->DrawArgs["box"].IndexCount;
		wallPillarBRTopFLBlockRitem->StartIndexLocation = wallPillarBRTopFLBlockRitem->Geo->DrawArgs["box"].StartIndexLocation;
		wallPillarBRTopFLBlockRitem->BaseVertexLocation = wallPillarBRTopFLBlockRitem->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarBRTopFLBlockRitem.get());


		XMStoreFloat4x4(&wallPillarBRTopFRBlockRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(10.1f, 6.5f, 8.2f));
		wallPillarBRTopFRBlockRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarBRTopFRBlockRitem->Mat = mMaterials["brickType2"].get();
		wallPillarBRTopFRBlockRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarBRTopFRBlockRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarBRTopFRBlockRitem->IndexCount = wallPillarBRTopFRBlockRitem->Geo->DrawArgs["box"].IndexCount;
		wallPillarBRTopFRBlockRitem->StartIndexLocation = wallPillarBRTopFRBlockRitem->Geo->DrawArgs["box"].StartIndexLocation;
		wallPillarBRTopFRBlockRitem->BaseVertexLocation = wallPillarBRTopFRBlockRitem->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarBRTopFRBlockRitem.get());

		XMStoreFloat4x4(&wallPillarBRTopBLBlockRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(8.2f, 6.5f, 10.1f));
		wallPillarBRTopBLBlockRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarBRTopBLBlockRitem->Mat = mMaterials["brickType2"].get();
		wallPillarBRTopBLBlockRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarBRTopBLBlockRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarBRTopBLBlockRitem->IndexCount = wallPillarBRTopBLBlockRitem->Geo->DrawArgs["box"].IndexCount;
		wallPillarBRTopBLBlockRitem->StartIndexLocation = wallPillarBRTopBLBlockRitem->Geo->DrawArgs["box"].StartIndexLocation;
		wallPillarBRTopBLBlockRitem->BaseVertexLocation = wallPillarBRTopBLBlockRitem->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarBRTopBLBlockRitem.get());

		XMStoreFloat4x4(&wallPillarBRTopBRBlockRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(10.1f, 6.5f, 10.1f));
		wallPillarBRTopBRBlockRitem->ObjCBIndex = ObjCBIndex++;
		wallPillarBRTopBRBlockRitem->Mat = mMaterials["brickType2"].get();
		wallPillarBRTopBRBlockRitem->Geo = mGeometries["boxGeo"].get();
		wallPillarBRTopBRBlockRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wallPillarBRTopBRBlockRitem->IndexCount = wallPillarBRTopBRBlockRitem->Geo->DrawArgs["box"].IndexCount;
		wallPillarBRTopBRBlockRitem->StartIndexLocation = wallPillarBRTopBRBlockRitem->Geo->DrawArgs["box"].StartIndexLocation;
		wallPillarBRTopBRBlockRitem->BaseVertexLocation = wallPillarBRTopBRBlockRitem->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(wallPillarBRTopBRBlockRitem.get());

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

		mAllRitems.push_back(std::move(wallPillarFLTopFLBlockRitem));
		mAllRitems.push_back(std::move(wallPillarFLTopFRBlockRitem));
		mAllRitems.push_back(std::move(wallPillarFLTopBLBlockRitem));
		mAllRitems.push_back(std::move(wallPillarFLTopBRBlockRitem));

		mAllRitems.push_back(std::move(wallPillarFRTopFLBlockRitem));
		mAllRitems.push_back(std::move(wallPillarFRTopFRBlockRitem));
		mAllRitems.push_back(std::move(wallPillarFRTopBLBlockRitem));
		mAllRitems.push_back(std::move(wallPillarFRTopBRBlockRitem));

		mAllRitems.push_back(std::move(wallPillarBLTopFLBlockRitem));
		mAllRitems.push_back(std::move(wallPillarBLTopFRBlockRitem));
		mAllRitems.push_back(std::move(wallPillarBLTopBLBlockRitem));
		mAllRitems.push_back(std::move(wallPillarBLTopBRBlockRitem));

		mAllRitems.push_back(std::move(wallPillarBRTopFLBlockRitem));
		mAllRitems.push_back(std::move(wallPillarBRTopFRBlockRitem));
		mAllRitems.push_back(std::move(wallPillarBRTopBLBlockRitem));
		mAllRitems.push_back(std::move(wallPillarBRTopBRBlockRitem));


	}

	// Center pillars
	for (int i = 0; i < 1; ++i)
	{
		auto centerPillarFrontRitem = std::make_unique<RenderItem>();
		auto centerPillarBackRitem = std::make_unique<RenderItem>();
		auto centerPillarLeftRitem = std::make_unique<RenderItem>();
		auto centerPillarRightRitem = std::make_unique<RenderItem>();

		//XMStoreFloat4x4(&wallRitemFront->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 1.0f, -10.0f) * XMMatrixRotationAxis(yAxis, 1.57));

		XMStoreFloat4x4(&centerPillarFrontRitem->World, XMMatrixScaling(0.5f, 3.0f, 0.5f) * XMMatrixTranslation(3.0f, 4.5f, -3.0f) * XMMatrixRotationAxis(yAxis, degreeRotation45));
		centerPillarFrontRitem->ObjCBIndex = ObjCBIndex++;
		centerPillarFrontRitem->Mat = mMaterials["brickType2"].get();
		centerPillarFrontRitem->Geo = mGeometries["boxGeo"].get();
		centerPillarFrontRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		centerPillarFrontRitem->IndexCount = centerPillarFrontRitem->Geo->DrawArgs["fountainPillar"].IndexCount;
		centerPillarFrontRitem->StartIndexLocation = centerPillarFrontRitem->Geo->DrawArgs["fountainPillar"].StartIndexLocation;
		centerPillarFrontRitem->BaseVertexLocation = centerPillarFrontRitem->Geo->DrawArgs["fountainPillar"].BaseVertexLocation;

		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(centerPillarFrontRitem.get());

		XMStoreFloat4x4(&centerPillarBackRitem->World, XMMatrixScaling(0.5f, 3.0f, 0.5f) * XMMatrixTranslation(-3.0f, 4.5f, 3.0f) * XMMatrixRotationAxis(yAxis, degreeRotation45));
		centerPillarBackRitem->ObjCBIndex = ObjCBIndex++;
		centerPillarBackRitem->Mat = mMaterials["brickType2"].get();
		centerPillarBackRitem->Geo = mGeometries["boxGeo"].get();
		centerPillarBackRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		centerPillarBackRitem->IndexCount = centerPillarBackRitem->Geo->DrawArgs["fountainPillar"].IndexCount;
		centerPillarBackRitem->StartIndexLocation = centerPillarBackRitem->Geo->DrawArgs["fountainPillar"].StartIndexLocation;
		centerPillarBackRitem->BaseVertexLocation = centerPillarBackRitem->Geo->DrawArgs["fountainPillar"].BaseVertexLocation;

		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(centerPillarBackRitem.get());

		XMStoreFloat4x4(&centerPillarLeftRitem->World, XMMatrixScaling(0.5f, 3.0f, 0.5f) * XMMatrixTranslation(-3.0f, 4.5f, -3.0f) * XMMatrixRotationAxis(yAxis, degreeRotation45));
		centerPillarLeftRitem->ObjCBIndex = ObjCBIndex++;
		centerPillarLeftRitem->Mat = mMaterials["brickType2"].get();
		centerPillarLeftRitem->Geo = mGeometries["boxGeo"].get();
		centerPillarLeftRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		centerPillarLeftRitem->IndexCount = centerPillarLeftRitem->Geo->DrawArgs["fountainPillar"].IndexCount;
		centerPillarLeftRitem->StartIndexLocation = centerPillarLeftRitem->Geo->DrawArgs["fountainPillar"].StartIndexLocation;
		centerPillarLeftRitem->BaseVertexLocation = centerPillarLeftRitem->Geo->DrawArgs["fountainPillar"].BaseVertexLocation;

		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(centerPillarLeftRitem.get());

		XMStoreFloat4x4(&centerPillarRightRitem->World, XMMatrixScaling(0.5f, 3.0f, 0.5f) * XMMatrixTranslation(3.0f, 4.5f, 3.0f) * XMMatrixRotationAxis(yAxis, degreeRotation45));
		centerPillarRightRitem->ObjCBIndex = ObjCBIndex++;
		centerPillarRightRitem->Mat = mMaterials["brickType2"].get();
		centerPillarRightRitem->Geo = mGeometries["boxGeo"].get();
		centerPillarRightRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		centerPillarRightRitem->IndexCount = centerPillarRightRitem->Geo->DrawArgs["fountainPillar"].IndexCount;
		centerPillarRightRitem->StartIndexLocation = centerPillarRightRitem->Geo->DrawArgs["fountainPillar"].StartIndexLocation;
		centerPillarRightRitem->BaseVertexLocation = centerPillarRightRitem->Geo->DrawArgs["fountainPillar"].BaseVertexLocation;

		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(centerPillarRightRitem.get());


		mAllRitems.push_back(std::move(centerPillarFrontRitem));
		mAllRitems.push_back(std::move(centerPillarBackRitem));
		mAllRitems.push_back(std::move(centerPillarLeftRitem));
		mAllRitems.push_back(std::move(centerPillarRightRitem));
	}

	// center pillar tops
	for (int i = 0; i < 1; ++i)
	{
		auto centerPillarFrontTopRitem = std::make_unique<RenderItem>();
		auto centerPillarBackTopRitem = std::make_unique<RenderItem>();
		auto centerPillarLeftTopRitem = std::make_unique<RenderItem>();
		auto centerPillarRightTopRitem = std::make_unique<RenderItem>();

		//XMStoreFloat4x4(&wallRitemFront->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 1.0f, -10.0f) * XMMatrixRotationAxis(yAxis, 1.57));

		XMStoreFloat4x4(&centerPillarFrontTopRitem->World, XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(3.0f, 3.6f, -3.0f) * XMMatrixRotationAxis(yAxis, degreeRotation45));
		centerPillarFrontTopRitem->ObjCBIndex = ObjCBIndex++;
		centerPillarFrontTopRitem->Mat = mMaterials["stone"].get();
		centerPillarFrontTopRitem->Geo = mGeometries["boxGeo"].get();
		centerPillarFrontTopRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		centerPillarFrontTopRitem->IndexCount = centerPillarFrontTopRitem->Geo->DrawArgs["fountainPillarTop"].IndexCount;
		centerPillarFrontTopRitem->StartIndexLocation = centerPillarFrontTopRitem->Geo->DrawArgs["fountainPillarTop"].StartIndexLocation;
		centerPillarFrontTopRitem->BaseVertexLocation = centerPillarFrontTopRitem->Geo->DrawArgs["fountainPillarTop"].BaseVertexLocation;

		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(centerPillarFrontTopRitem.get());

		XMStoreFloat4x4(&centerPillarBackTopRitem->World, XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(-3.0f, 3.6f, 3.0f) * XMMatrixRotationAxis(yAxis, degreeRotation45));
		centerPillarBackTopRitem->ObjCBIndex = ObjCBIndex++;
		centerPillarBackTopRitem->Mat = mMaterials["stone"].get();
		centerPillarBackTopRitem->Geo = mGeometries["boxGeo"].get();
		centerPillarBackTopRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		centerPillarBackTopRitem->IndexCount = centerPillarBackTopRitem->Geo->DrawArgs["fountainPillarTop"].IndexCount;
		centerPillarBackTopRitem->StartIndexLocation = centerPillarBackTopRitem->Geo->DrawArgs["fountainPillarTop"].StartIndexLocation;
		centerPillarBackTopRitem->BaseVertexLocation = centerPillarBackTopRitem->Geo->DrawArgs["fountainPillarTop"].BaseVertexLocation;

		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(centerPillarBackTopRitem.get());

		XMStoreFloat4x4(&centerPillarLeftTopRitem->World, XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(-3.0f, 3.6f, -3.0f) * XMMatrixRotationAxis(yAxis, degreeRotation45));
		centerPillarLeftTopRitem->ObjCBIndex = ObjCBIndex++;
		centerPillarLeftTopRitem->Mat = mMaterials["stone"].get();
		centerPillarLeftTopRitem->Geo = mGeometries["boxGeo"].get();
		centerPillarLeftTopRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		centerPillarLeftTopRitem->IndexCount = centerPillarLeftTopRitem->Geo->DrawArgs["fountainPillarTop"].IndexCount;
		centerPillarLeftTopRitem->StartIndexLocation = centerPillarLeftTopRitem->Geo->DrawArgs["fountainPillarTop"].StartIndexLocation;
		centerPillarLeftTopRitem->BaseVertexLocation = centerPillarLeftTopRitem->Geo->DrawArgs["fountainPillarTop"].BaseVertexLocation;

		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(centerPillarLeftTopRitem.get());

		XMStoreFloat4x4(&centerPillarRightTopRitem->World, XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(3.0f, 3.6f, 3.0f) * XMMatrixRotationAxis(yAxis, degreeRotation45));
		centerPillarRightTopRitem->ObjCBIndex = ObjCBIndex++;
		centerPillarRightTopRitem->Mat = mMaterials["stone"].get();
		centerPillarRightTopRitem->Geo = mGeometries["boxGeo"].get();
		centerPillarRightTopRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		centerPillarRightTopRitem->IndexCount = centerPillarRightTopRitem->Geo->DrawArgs["fountainPillarTop"].IndexCount;
		centerPillarRightTopRitem->StartIndexLocation = centerPillarRightTopRitem->Geo->DrawArgs["fountainPillarTop"].StartIndexLocation;
		centerPillarRightTopRitem->BaseVertexLocation = centerPillarRightTopRitem->Geo->DrawArgs["fountainPillarTop"].BaseVertexLocation;

		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(centerPillarRightTopRitem.get());


		mAllRitems.push_back(std::move(centerPillarFrontTopRitem));
		mAllRitems.push_back(std::move(centerPillarBackTopRitem));
		mAllRitems.push_back(std::move(centerPillarLeftTopRitem));
		mAllRitems.push_back(std::move(centerPillarRightTopRitem));


	}

	//door NEEDS TO BE WOOD TEXTURE
	for (int i = 0; i < 1; i++)
	{
		auto door = std::make_unique<RenderItem>();
		auto leftAnchor = std::make_unique<RenderItem>();
		auto rightAnchor = std::make_unique<RenderItem>();

		///////////MAKE WOOD TEXTURE///////////////

		XMStoreFloat4x4(&door->World, XMMatrixScaling(3.5f, 0.1f, 4.0f) * XMMatrixTranslation(0.0f, -6.5f, -10.0f) * XMMatrixRotationAxis(xAxis, degreeRotation45)) ;
		door->ObjCBIndex = ObjCBIndex++;
		door->Mat = mMaterials["wood"].get();
		door->Geo = mGeometries["boxGeo"].get();
		door->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		door->IndexCount = door->Geo->DrawArgs["box"].IndexCount;
		door->StartIndexLocation = door->Geo->DrawArgs["box"].StartIndexLocation;
		door->BaseVertexLocation = door->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(door.get());

		////////////////////////////////////////////

		XMStoreFloat4x4(&leftAnchor->World, XMMatrixScaling(0.1f, 1.0f, 0.1f) * XMMatrixTranslation(-1.7f, -10.7f, -3)* XMMatrixRotationAxis(xAxis, degreeRotation90)) ;
		leftAnchor->ObjCBIndex = ObjCBIndex++;
		leftAnchor->Mat = mMaterials["stone"].get();
		leftAnchor->Geo = mGeometries["boxGeo"].get();
		leftAnchor->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftAnchor->IndexCount = leftAnchor->Geo->DrawArgs["wallPillar"].IndexCount;
		leftAnchor->StartIndexLocation = leftAnchor->Geo->DrawArgs["wallPillar"].StartIndexLocation;
		leftAnchor->BaseVertexLocation = leftAnchor->Geo->DrawArgs["wallPillar"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(leftAnchor.get());

		XMStoreFloat4x4(&rightAnchor->World, XMMatrixScaling(0.1f, 1.0f, 0.1f) * XMMatrixTranslation(1.7f, -10.7f, -3)* XMMatrixRotationAxis(xAxis, degreeRotation90)) ;
		rightAnchor->ObjCBIndex = ObjCBIndex++;
		rightAnchor->Mat = mMaterials["stone"].get();
		rightAnchor->Geo = mGeometries["boxGeo"].get();
		rightAnchor->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightAnchor->IndexCount = rightAnchor->Geo->DrawArgs["wallPillar"].IndexCount;
		rightAnchor->StartIndexLocation = rightAnchor->Geo->DrawArgs["wallPillar"].StartIndexLocation;
		rightAnchor->BaseVertexLocation = rightAnchor->Geo->DrawArgs["wallPillar"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(rightAnchor.get());


		mAllRitems.push_back(std::move(door));
		mAllRitems.push_back(std::move(leftAnchor));
		mAllRitems.push_back(std::move(rightAnchor));

	}

	//floor
	for (int i = 0; i < 1; i++)
	{
		auto floor = std::make_unique<RenderItem>();

		XMStoreFloat4x4(&floor->World, XMMatrixScaling(20.5, 0.5f, 20.5f) * XMMatrixTranslation(0.0f, 1.35f, 0.0f)) ;
		floor->ObjCBIndex = ObjCBIndex++;
		floor->Mat = mMaterials["tile"].get();
		floor->Geo = mGeometries["boxGeo"].get();
		floor->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		floor->IndexCount = floor->Geo->DrawArgs["box"].IndexCount;
		floor->StartIndexLocation = floor->Geo->DrawArgs["box"].StartIndexLocation;
		floor->BaseVertexLocation = floor->Geo->DrawArgs["box"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::AlphaTested].push_back(floor.get());

		mAllRitems.push_back(std::move(floor));
	}
	 
    mAllRitems.push_back(std::move(wavesRitem));
    mAllRitems.push_back(std::move(gridRitem));
	mAllRitems.push_back(std::move(centerFountainRitem));
	mAllRitems.push_back(std::move(treeSpritesRitem));
}

void TreeBillboardsApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
    UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();

    // For each render item...
    for(size_t i = 0; i < ritems.size(); ++i)
    {
        auto ri = ritems[i];

        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		//step3
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
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> TreeBillboardsApp::GetStaticSamplers()
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

float TreeBillboardsApp::GetHillsHeight(float x, float z)const
{
    return 0.05f*(z*sinf(0.1f*x) + x*cosf(0.1f*z));
}

XMFLOAT3 TreeBillboardsApp::GetHillsNormal(float x, float z)const
{
    // n = (-df/dx, 1, -df/dz)
    XMFLOAT3 n(
        -0.03f*z*cosf(0.1f*x) - 0.3f*cosf(0.1f*z),
        1.0f,
        -0.3f*sinf(0.1f*x) + 0.03f*x*sinf(0.1f*z));

    XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
    XMStoreFloat3(&n, unitNormal);

    return n;
}
