#pragma once

#include <d3d12.h>
#include <string>
#include <wrl.h>
#include <vector>
#include "externals/DirectXTex/DirectXTex.h"


class TextureManager
{
public:

	// デスクリプターの数
	static const size_t kNumDescriptors = 256;

	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	/// <returns>シングルトンインスタンス</returns>
	static TextureManager* GetInstance();

	/// <summary>
	/// テクスチャ
	/// </summary>
	struct Texture {
		// テクスチャリソース
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		// シェーダリソースビューのハンドル(CPU)
		D3D12_CPU_DESCRIPTOR_HANDLE cpuDescHandleSRV;
		// シェーダリソースビューのハンドル(CPU)
		D3D12_GPU_DESCRIPTOR_HANDLE gpuDescHandleSRV;
	};

	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t textureHandle) { return textures_[textureHandle].gpuDescHandleSRV; }

	/// <summary>
	/// 読み込み
	/// </summary>
	/// <param name="fileName">ファイル名</param>
	/// <returns>テクスチャハンドル</returns>
	static uint32_t Load(const std::string& fileName);

	/// <summary>
	/// システム初期化
	/// </summary>
	/// <param name="device">デバイス</param>
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string directoryPath = "Resources/");

private:
	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(const TextureManager&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;

	// デバイス
	static ID3D12Device* device_;

	static ID3D12GraphicsCommandList* commandList_;
	// デスクリプタサイズ
	UINT sDescriptorHandleIncrementSize_ = 0u;
	// ディレクトリパス
	static std::string directoryPath_;
	// デスクリプタヒープ
	static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap_;

	static std::vector<Texture> textures_;

private:

	static DirectX::ScratchImage LoadTexture(const std::string& filePath);

	static ID3D12Resource* CreateTextureResource(const DirectX::TexMetadata& metadata);

	static ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);

	static void UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);

};

