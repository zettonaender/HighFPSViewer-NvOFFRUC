//
// Game.h
//

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include <SpriteBatch.h>
#include <WICTextureLoader.h>
#include <SimpleMath.h>
#include <libloaderapi.h>
#include <../../Interface/NvOFFRUC.h>
#include "DirectXTex.h"
#include <synchapi.h>
#include <d3d11_3.h>
#include <d3d11_4.h>
#include <iostream>
#include <chrono>
#include <sstream>
#include "PostProcess.h"
#include <queue>
#include <thread>

// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game final : public DX::IDeviceNotify
{
public:

    Game() noexcept(false);
    ~Game() = default;

    Game(Game&&) = default;
    Game& operator= (Game&&) = default;

    Game(Game const&) = delete;
    Game& operator= (Game const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnDisplayChange();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize( int& width, int& height ) const noexcept;

    // Drawing Stuff
    ID3D11ShaderResourceView* m_texture = nullptr;                         //Released
    ID3D11ShaderResourceView* m_textureDesktop = nullptr;                  //Released
    
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_textureCursor;
    
    std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;
    std::unique_ptr<DirectX::DX11::BasicPostProcess> postProcess;
    std::unique_ptr<DirectX::DX11::BasicPostProcess> postProcess2;
    
    DirectX::SimpleMath::Vector2 m_origin;          
    DirectX::SimpleMath::Vector2 m_scaleFactor;     
    DirectX::SimpleMath::Vector2 m_screenPos;    
    
    POINT lastCursorPos;

    // Desktop Duplication Stuff
    IDXGIFactory1* factory = nullptr;                                      //Released
    IDXGIAdapter1* adapter = nullptr;                                      //Released
    IDXGIOutput* pOutput = nullptr;                                        //Released
    IDXGIOutput1* pOutput1 = nullptr;                                      //Released
    IDXGIOutputDuplication* pDeskDupl = nullptr;                           //Released
    IDXGIResource* desktopResource = nullptr;                              //Released
    ID3D11Texture2D* desktopTextureBGR = nullptr;                          //Released
    ID3D11Texture2D* lastFrame = nullptr;                                  //Released
    ID3D11Texture2D* renderTargetTexture = nullptr;                        //Released  
    
    int desktop_width = 1280, desktop_height = 720;

    // Function for Rendering
    bool GetFrame();
    void DrawFromSRV();

    // NVOF Stuff
    PtrToFuncNvOFFRUCCreate NvOFFRUCCreate;
    PtrToFuncNvOFFRUCRegisterResource NvOFFRUCRegisterResource;
    PtrToFuncNvOFFRUCUnregisterResource NvOFFRUCUnregisterResource;
    PtrToFuncNvOFFRUCProcess NvOFFRUCProcess;
    PtrToFuncNvOFFRUCDestroy NvOFFRUCDestroy;
    NvOFFRUCHandle hFRUC;
    NvOFFRUC_REGISTER_RESOURCE_PARAM regOutParam = { 0 };

    // NvOFFRUC Functions
    void InterpolateFrame();
    void CreateTextureBuffer();
    void GetResource(void** ppTexture);

    // NvOFFRUC Objects
    ID3D11Fence* m_pFence = nullptr;                                       //Released
    ID3D11Device5* m_pDevice5 = nullptr;                                   //Released
    ID3D11Texture2D* m_pRenderTexture2D[2];                                //Released
    ID3D11Texture2D* m_pInterpolateTexture2D[1];                           //Released
    ID3D11DeviceContext4* m_pDeviceContext4 = nullptr;                     //Released
    HANDLE m_hFenceEvent = NULL;

    // NvOFFRUC Variables
    const double m_constdRenderInterval = 1;
    double m_dLastRenderTime=0;
    int m_uiFenceValue = 0;
    int currRenderIndex = 1;
    int lastRenderIndex = 0;
    bool drawInterpolated = true;

    // Important Variables
    bool isOnTheLeft = true;
    int monitorIndex = 1;
    double resFactor = 2;

    // Timing Objects
    std::chrono::duration<double> sleepDuration = std::chrono::duration<double>(0);
    int totalcount = 0;
    double fps = 120;
    double frametime = 1.f / fps;
private:

    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>    m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                           m_timer;
};
