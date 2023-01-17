//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

//Function to output float to Debug Console
void fts(float f) {
    std::stringstream ss;
    ss << f;
    std::string str = ss.str();
    OutputDebugStringA(str.c_str());
    OutputDebugStringA(" ms\n");
}

Game::Game() noexcept(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);
    {
        m_deviceResources->CreateDeviceResources();
    }
    CreateDeviceDependentResources();
    {
        m_deviceResources->CreateWindowSizeDependentResources();
    }
    CreateWindowSizeDependentResources();
    
    // Set timer equal to framerate.
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(frametime);
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
            
    });
    Render();
}


// Main rendering loop.
void Game::Render()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto context = m_deviceResources->GetD3DDeviceContext();
    
    // Loop for interpolated frame.
    
    if (drawInterpolated) {
        
        // Get new frame and interpolate.
        auto start = std::chrono::high_resolution_clock::now();
        while (!GetFrame()) {}
        InterpolateFrame();
		auto end = std::chrono::high_resolution_clock::now();
        
        
        sleepDuration += end - start; totalcount++;

        Clear();

        // Create SRV from interpolated texture.
        D3D11_TEXTURE2D_DESC textureDesc;
        m_pInterpolateTexture2D[0]->GetDesc(&textureDesc);
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        if (m_texture != nullptr)m_texture->Release();
        auto hr=device->CreateShaderResourceView(m_pInterpolateTexture2D[0], &srvDesc, &m_texture);
        
        // Show the new frame.
        DrawFromSRV();
        m_deviceResources->Present();

        drawInterpolated = false;
    }
    else {
        
#ifdef _DEBUG
        fts(1000 * sleepDuration.count()/totalcount);
#endif
        
		// Sleep for the average duration.
        if(totalcount!=0) std::this_thread::sleep_for(sleepDuration/totalcount);

        Clear();

		// Create SRV from last frame.
        D3D11_TEXTURE2D_DESC textureDesc;
        lastFrame->GetDesc(&textureDesc);
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
		if (m_texture != nullptr)m_texture->Release();
        auto hr = device->CreateShaderResourceView(lastFrame, &srvDesc, &m_texture);

        // Show the new frame.
        DrawFromSRV();
        m_deviceResources->Present();

        drawInterpolated = true;
    }

}

// Helper method to clear the back buffers.
void Game::Clear()
{
    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::Black);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto const viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);
}

bool Game::GetFrame()
{
    // Acquire next frame.
    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    auto hr = pDeskDupl->AcquireNextFrame(1, &frameInfo, &desktopResource);
    if (hr == DXGI_ERROR_WAIT_TIMEOUT || hr != S_OK) return false;

    auto device = m_deviceResources->GetD3DDevice();
    auto context = m_deviceResources->GetD3DDeviceContext();
    desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)(&desktopTextureBGR));
    
    // Initialize source texture.
    D3D11_TEXTURE2D_DESC textureDesc;
    desktopTextureBGR->GetDesc(&textureDesc);
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    device->CreateShaderResourceView(desktopTextureBGR, &srvDesc, &m_textureDesktop);
    postProcess->SetSourceTexture(m_textureDesktop);
    postProcess->SetEffect(BasicPostProcess::Copy);

    // Initialize temporary render target.
    ID3D11RenderTargetView* tmpRTV = nullptr;
    D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
    renderTargetViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    renderTargetViewDesc.Texture2D.MipSlice = 0;
    device->CreateRenderTargetView(renderTargetTexture, &renderTargetViewDesc, &tmpRTV);

	// Render to temporary render target (scope added for mutex if used).
    {
        // Set to temporary render target view and viewport.
        D3D11_VIEWPORT tmpViewport = { 0.0f, 0.0f, static_cast<float>(desktop_width), static_cast<float>(desktop_height), 0.f, 1.f };
        context->OMSetRenderTargets(1, &tmpRTV, nullptr);
		context->RSSetViewports(1, &tmpViewport);
        
		// Render to temporary render target.
        postProcess->Process(context);
        
		// Reset render target view and viewport.
        auto renderTarget = m_deviceResources->GetRenderTargetView();
        auto depthStencil = m_deviceResources->GetDepthStencilView();
        auto const viewport = m_deviceResources->GetScreenViewport();
        context->OMSetRenderTargets(1, &renderTarget, depthStencil);
        context->RSSetViewports(1, &viewport);
    }

    // Update render index.
    lastRenderIndex = currRenderIndex;
	currRenderIndex = (currRenderIndex + 1) % 2;

    // Copy to render texture for NvOFFRUC.
    {
        context->CopySubresourceRegion(m_pRenderTexture2D[currRenderIndex], 0, 0, 0, 0, renderTargetTexture, 0, nullptr);
    }

	// Release resources.
    tmpRTV->Release();
    desktopTextureBGR->Release();
    pDeskDupl->ReleaseFrame();

    return true;
}

void Game::DrawFromSRV() {
    m_spriteBatch->Begin();

    // Draw main texture.
    auto tmp = m_texture;
    m_spriteBatch->Draw(tmp, m_screenPos, nullptr, Colors::White, 0.f, DirectX::SimpleMath::Vector2::Zero, m_scaleFactor);

    // Get current cursor position.
    POINT cursorPos;
    DirectX::SimpleMath::Vector2 m_mousePosition;
    DirectX::SimpleMath::Vector2 m_scaleFactor2;
    GetCursorPos(&cursorPos);
    
	// Only draw cursor if it is inside the screen.
    if (cursorPos.x < 0) {
        
        //// Reset sleep duration if cursor is moving.
        //if (cursorPos.x != lastCursorPos.x || cursorPos.y != lastCursorPos.y) {
        //    sleepDuration = std::chrono::duration<double>(0);
        //    totalcount = 0;
        //}
        
        // Scale cursor.
        m_mousePosition.x = ((float)cursorPos.x/resFactor + (float)desktop_width) * (float)m_scaleFactor.x;
        m_mousePosition.y = (float)cursorPos.y * (float)m_scaleFactor.y / resFactor;
        
        // Add screen offset position.
        m_mousePosition.x += m_screenPos.x;
        m_mousePosition.y += m_screenPos.y;
        m_scaleFactor2.x = m_scaleFactor.x / resFactor;
        m_scaleFactor2.y = m_scaleFactor.y / resFactor;
        m_spriteBatch->Draw(m_textureCursor.Get(), m_mousePosition, nullptr, Colors::White, 0.f, m_origin, m_scaleFactor2);
    }    
    m_spriteBatch->End();

    lastCursorPos = cursorPos;
}

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize).
}

void Game::OnWindowMoved()
{
    auto const r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnDisplayChange()
{
    m_deviceResources->UpdateColorSpace();
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();

    // TODO: Game window is being resized.
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const noexcept
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = desktop_width;
    height = desktop_height;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto context = m_deviceResources->GetD3DDeviceContext();
 
    // Create sprite batch for drawing.
    m_spriteBatch = std::make_unique<SpriteBatch>(context);
    
    // Initialize cursor texture.
    ComPtr<ID3D11Resource> resource;
    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, L"default.png",
            resource.GetAddressOf(),
            m_textureCursor.ReleaseAndGetAddressOf()));
    
    ComPtr<ID3D11Texture2D> cursor;
    DX::ThrowIfFailed(resource.As(&cursor));
    
    CD3D11_TEXTURE2D_DESC cursorDesc;                               
    cursor->GetDesc(&cursorDesc);
    
    m_origin.x = 0;
    m_origin.y = 0;

    // Initialize desktop duplication.
    CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory);
    factory->EnumAdapters1(0, &adapter);
    adapter->EnumOutputs(1, &pOutput);
    pOutput->QueryInterface(__uuidof(IDXGIOutput1), (void**)&pOutput1);
    pOutput1->DuplicateOutput(device, &pDeskDupl);
    
    // Get width and height for rendering.
    DXGI_OUTDUPL_DESC outputDesc = { 0 };                           
    pDeskDupl->GetDesc(&outputDesc);
    desktop_width = outputDesc.ModeDesc.Width/resFactor;
    desktop_height = outputDesc.ModeDesc.Height/resFactor;

    // Initialize NvOFFRUC.

    // Load NvOFFRUC dll.
    HMODULE hDLL=LoadLibrary("NvOFFRUC.dll");
    if (hDLL == nullptr) {
        throw;
    }
    NvOFFRUCCreate = (PtrToFuncNvOFFRUCCreate)GetProcAddress(hDLL,CreateProcName);
    NvOFFRUCRegisterResource = (PtrToFuncNvOFFRUCRegisterResource)GetProcAddress(hDLL,RegisterResourceProcName);
    NvOFFRUCUnregisterResource =(PtrToFuncNvOFFRUCUnregisterResource)GetProcAddress(hDLL,UnregisterResourceProcName);
    NvOFFRUCProcess = (PtrToFuncNvOFFRUCProcess)GetProcAddress(hDLL,ProcessProcName);
    NvOFFRUCDestroy = (PtrToFuncNvOFFRUCDestroy)GetProcAddress(hDLL,DestroyProcName);
    
	// Create NvOFFRUC instance.
    NvOFFRUC_CREATE_PARAM createParams = { 0 };
    createParams.pDevice = device;
    createParams.uiHeight = desktop_height;
    createParams.uiWidth = desktop_width;
    createParams.eResourceType = DirectX11Resource;
    createParams.eSurfaceFormat = ARGBSurface;
    createParams.eCUDAResourceType = CudaResourceCuDevicePtr;
    auto status = NvOFFRUCCreate(&createParams,&hFRUC);

	// Create textures for NvOFFRUC.
    CreateTextureBuffer();
    
    // Create fence for NvOFFRUC.
    device->QueryInterface<ID3D11Device5>(&m_pDevice5);
    context->QueryInterface<ID3D11DeviceContext4>(&m_pDeviceContext4);
    m_pDevice5->CreateFence(0, D3D11_FENCE_FLAG_SHARED, IID_PPV_ARGS(&m_pFence));
    m_hFenceEvent = CreateEvent(nullptr,FALSE,FALSE,nullptr);
    
	// Register resource to NvOFFRUC.
    GetResource(regOutParam.pArrResource);
    regOutParam.uiCount = 3;
    regOutParam.pD3D11FenceObj = m_pFence;
    status = NvOFFRUCRegisterResource(hFRUC,&regOutParam);

    // Initialize PostProcess for downscaling and conversion.
    postProcess = std::make_unique<BasicPostProcess>(device);

    // Initialize lastCursorPos to avoid possible errors.
    GetCursorPos(&lastCursorPos);

#ifdef _DEBUG
    // Allocate console for debugging.
    /*AllocConsole();
    freopen("CONOUT$", "w", stdout);*/
#endif
    
    // Set multithreading for future use.
	ID3D11Multithread* pMultiThread = nullptr;
	device->QueryInterface(__uuidof(ID3D11Multithread), (void**)&pMultiThread);
	pMultiThread->SetMultithreadProtected(TRUE);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // Get the new scale factors.
    RECT size = m_deviceResources->GetOutputSize();
    m_screenPos.x = -1.0f;
    m_screenPos.y = -1.0f;
    int width = size.right - size.left;
    int height = size.bottom - size.top;
    m_scaleFactor.x = float(width) / float(desktop_width);
    m_scaleFactor.y = float(height) / float(desktop_height);
    
	// Preserve aspect ratio of desktop.
    if (m_scaleFactor.y > m_scaleFactor.x) {
        m_scaleFactor.y = m_scaleFactor.x;
        m_screenPos.y = (float(height) - float(desktop_height) * m_scaleFactor.y) / 2.0f;
    }
    else if (m_scaleFactor.x > m_scaleFactor.y) {
        m_scaleFactor.x = m_scaleFactor.y;
        m_screenPos.x = (float(width) - float(desktop_width) * m_scaleFactor.x) / 2.0f;
    }
}

void Game::OnDeviceLost()
{
    
	// Release all resources.

    // Release desktop duplication resources.
    factory->Release();
    adapter->Release();
    pOutput->Release();
    pOutput1->Release();
    pDeskDupl->Release();
    
	// Release NvOFFRUC resources.
    m_pFence->Release();
    m_pDevice5->Release();
    m_pDeviceContext4->Release();
    
	// Unregister textures from NvOFFRUC.
    NvOFFRUC_UNREGISTER_RESOURCE_PARAM stUnregisterResourceParam = { 0 };
    memcpy(stUnregisterResourceParam.pArrResource,regOutParam.pArrResource,regOutParam.uiCount * sizeof(IUnknown*));
    stUnregisterResourceParam.uiCount = regOutParam.uiCount;
    auto status = NvOFFRUCUnregisterResource(hFRUC,&stUnregisterResourceParam);
    
	// Destroy NvOFFRUC instance.
    NvOFFRUCDestroy(hFRUC);
    
    // Release texture buffers.
    renderTargetTexture->Release();
    lastFrame->Release();
    for (int i = 0; i < 1; i++) {
        m_pInterpolateTexture2D[i]->Release();
    }
    for (int i = 0; i < 2; i++) {
        m_pRenderTexture2D[i]->Release();
    }
    m_texture->Release();
    m_textureDesktop->Release();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion

// Interpolation loop.
void Game::InterpolateFrame()
{    
    // Copy the last texture to the render texture.
    {
        auto context = m_deviceResources->GetD3DDeviceContext();
        context->CopySubresourceRegion(lastFrame, 0, 0, 0, 0, m_pRenderTexture2D[lastRenderIndex], 0, nullptr);
    }

    // Parameter for input.
    bool repeated;
    NvOFFRUC_PROCESS_IN_PARAMS stInParams = { 0 };
    stInParams.stFrameDataInput.pFrame = m_pRenderTexture2D[currRenderIndex];
    stInParams.stFrameDataInput.nTimeStamp = m_dLastRenderTime + m_constdRenderInterval;
    m_dLastRenderTime = m_dLastRenderTime + m_constdRenderInterval;
    stInParams.stFrameDataInput.nCuSurfacePitch = desktop_width * 4;
    stInParams.uSyncWait.FenceWaitValue.uiFenceValueToWaitOn = m_uiFenceValue;
    
	// Parameter for output.
    NvOFFRUC_PROCESS_OUT_PARAMS stOutParams = { 0 };
    stOutParams.stFrameDataOutput.pFrame = m_pInterpolateTexture2D[0];
    stOutParams.stFrameDataOutput.nTimeStamp = m_dLastRenderTime + (0.f - float(m_constdRenderInterval)) * 0.5;
    stOutParams.stFrameDataOutput.bHasFrameRepetitionOccurred = &repeated;
    stOutParams.stFrameDataOutput.nCuSurfacePitch = desktop_width * 4;
    stOutParams.uSyncSignal.FenceSignalValue.uiFenceValueToSignalOn = ++m_uiFenceValue;
    
	// Call NvOFFRUC to interpolate.
    auto status = NvOFFRUCProcess(hFRUC,&stInParams,&stOutParams);
}

// Initialize all textures.
void Game::CreateTextureBuffer()
{
    auto device = m_deviceResources->GetD3DDevice();
    
    // Initialize texture description.
    D3D11_TEXTURE2D_DESC desc = { 0 };
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = desktop_width;
    desc.Height = desktop_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    
	// Create texture for NvOFFRUC.
    for (int i = 0; i < 2; i++) {
       device->CreateTexture2D(&desc, NULL, &m_pRenderTexture2D[i]);
    }
    for (int i = 0; i < 1; i++) {
        device->CreateTexture2D(&desc, NULL, &m_pInterpolateTexture2D[i]);
    }
    
    // Create texture for duplication and rendering.
    device->CreateTexture2D(&desc, NULL, &lastFrame);
    device->CreateTexture2D(&desc, NULL, &renderTargetTexture);
}

// Code from NvOFFRUCSample to get resources.
void Game::GetResource(void** ppTexture)
{
    for (uint32_t i = 0; i < 1; i++)
    {
        if (m_pInterpolateTexture2D[i])
        {
            ppTexture[i] = m_pInterpolateTexture2D[i];
        }
    }
    ppTexture = ppTexture + 1;
    for (uint32_t i = 0; i < 2; i++)
    {
        if (m_pRenderTexture2D[i])
        {
            ppTexture[i] = m_pRenderTexture2D[i];
        }
    }
}