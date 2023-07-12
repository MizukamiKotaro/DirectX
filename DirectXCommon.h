#pragma once

#include "WinApp.h"

#include<d3d12.h>
#include<dxgi1_6.h>
#include<cassert>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

class DirectXCommon
{
public: // メンバ関数

	DirectXCommon();
	~DirectXCommon();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(WinApp* winApp);

	void Initialize();

	/// <summary>
	/// デバイスの取得
	/// </summary>
	/// <returns>デバイス</returns>
	ID3D12Device* GetDevice() { return device_; }

	/// <summary>
	/// ディスクリプタヒープの作成関数
	/// </summary>
	static ID3D12DescriptorHeap* CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

private: // メンバ変数
public: //確認のため
	// ウィンドウズアプリケーション管理
	WinApp* winApp_;

	IDXGIFactory7* dxgiFactory_ = nullptr;
	ID3D12Device* device_ = nullptr;
	ID3D12CommandQueue* commandQueue_ = nullptr;
	ID3D12CommandAllocator* commandAllocator_ = nullptr;
	ID3D12GraphicsCommandList* commandList_ = nullptr;
	IDXGISwapChain4* swapChain_ = nullptr;
	ID3D12Resource* swapChainResources_[2] = {}; // ダブルバッファだから
	ID3D12DescriptorHeap* rtvHeap_ = nullptr;
	ID3D12DescriptorHeap* dsvHeap_ = nullptr;
	ID3D12Fence* fence_ = nullptr;
	uint64_t fenceValue_ = 0;
	HANDLE fenceEvent_ = nullptr;

	// 確認用
	//RTVを2つ作るのでディスクリプタを2つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[2];

private: // メンバ関数

	/// <summary>
	/// DXGIデバイス初期化
	/// </summary>
	void InitializeDXGIDevice();

	/// <summary>
	/// スワップチェーンの生成
	/// </summary>
	void CreateSwapChain();

	/// <summary>
	/// コマンド関連初期化
	/// </summary>
	void InitializeCommand();

	/// <summary>
	/// レンダーターゲット生成
	/// </summary>
	void CreateFinalRenderTargets();

	/// <summary>
	/// 深度バッファ生成
	/// </summary>
	void CreateDepthBuffer();

	ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height);

	/// <summary>
	/// フェンス生成
	/// </summary>
	void CreateFence();
};


