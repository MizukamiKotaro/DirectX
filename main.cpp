#include<Windows.h>
#include<cstdint>
//#include <string>
#include<format>

#include<d3d12.h>
#include<dxgi1_6.h>
#include<cassert>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#include <dxcapi.h>
#pragma comment(lib, "dxcompiler.lib")
#include "Vector4.h"
#include "Matrix4x4.h"
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#include "externals/DirectXTex/DirectXTex.h"
#include "VertexData.h"
#include "externals/DirectXTex/d3dx12.h"
#include <vector>
#include "calc.h"

#include<cmath>

#include "Transform.h"
#include "TransformationMatrix.h"
#include "Material.h"
#include "Light.h"

#include "WinApp.h"
#include "DebugLog.h"
#include "DirectXCommon.h"

//コンパイル用関数
IDxcBlob* CompileShader(
	//compilerするShaderファイルへパス
	const std::wstring& filePath,
	//compilerに使用するprofile
	const wchar_t* profile,
	//初期化で生成したもの3つ
	IDxcUtils* dxcUtils,
	IDxcCompiler3* dxcCompiler,
	IDxcIncludeHandler* includeHandler)
{
	// 1. hlslファイルを読む
	//これからシェーダーをコンパイルする旨をログに出す
	DebugLog::Log(DebugLog::ConvertString(std::format(L"Begin CompileShader,path:{},profile:{}\n", filePath, profile)));
	//hlslファイルを読む
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	//読めなかったら止める
	assert(SUCCEEDED(hr));
	//見込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;//UTF8の文字コードであることを通知

	// 2. compileする
	LPCWSTR arguments[] = {
		filePath.c_str(),//コンパイル対象のhlslファイル名
		L"-E",L"main",//エントリーポイントの指定。基本的にmain以外にはしない
		L"-T",profile,//ShaderProfileの設定
		L"-Zi",L"-Qembed_debug",//デバッグ用の情報を読み込む
		L"-Od",//最適化を外しておく
		L"-Zpr",//メモリレイアウトは優先
	};
	//実際にshaderをコンパイルする
	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,//読み込んだファイル
		arguments,          //コンパイルオプション
		_countof(arguments),//コンパイルオプションの数
		includeHandler,     //includeが含まれた諸々
		IID_PPV_ARGS(&shaderResult)//コンパイル結果
	);
	//コンパイルエラーではなくdxcが起動できないなど致命的な状況
	assert(SUCCEEDED(hr));
	
	// 3. 警告・エラーが出ていないか確認する
	IDxcBlobUtf8* shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		DebugLog::Log(shaderError->GetStringPointer());
		assert(false);
	}

	// 4. compile結果を受け取って返す
	//コンパイル結果から実行用のバイナリ部分を取得
	IDxcBlob* shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
	//成功したログを出す
	DebugLog::Log(DebugLog::ConvertString(std::format(L"Compile Succeeded, path:{}, prefile:{}\n", filePath, profile)));
	//もう使わないリソースを開放
	shaderSource->Release();
	shaderResult->Release();
	//実行用のバイナリを返却
	return shaderBlob;
}

////ディスクリプタヒープの作成関数
//ID3D12DescriptorHeap* CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {
//	//ディスクリプタヒープの生成
//	ID3D12DescriptorHeap* descriptorHeap = nullptr;
//	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
//	descriptorHeapDesc.Type = heapType; 
//	descriptorHeapDesc.NumDescriptors = numDescriptors;
//	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
//	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
//	//ディスクリプタヒープが作られなかったので起動しない
//	assert(SUCCEEDED(hr));
//	return descriptorHeap;
//}

//Resorce作成の関数
ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes) {

	//頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	//頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc{};
	//バッファリソース。テクスチャの場合はまた別の設定をする
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width = sizeInBytes;
	//バッファの場合はこれらは1にする決まり
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;
	//バッファの場合はこれにする決まり
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	//実際に頂点リソースを作る
	ID3D12Resource* vertexResource = nullptr;
	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
		&vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexResource));
	assert(SUCCEEDED(hr));

	return vertexResource;
}

//Textureデータを読み込む
DirectX::ScratchImage LoadTexture(const std::string& filePath) {
	//デスクトップファイルを読んでプログラムで使えるようにする
	DirectX::ScratchImage image{};
	std::wstring filePathW = DebugLog::ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	//ミップマップの生成
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));

	//ミップマップ付きのデータを返す
	return mipImages;
}

//DirectX12のTextureResourceを作る
ID3D12Resource* CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata) {

	//1.metadateを基にResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width); // Textureの幅
	resourceDesc.Height = UINT(metadata.height); // Textureの高さ
	resourceDesc.MipLevels = UINT(metadata.mipLevels); // mipmapの数
	resourceDesc.DepthOrArraySize = UINT(metadata.arraySize); // 奥行き　or 配列Textureの配列数
	resourceDesc.Format = metadata.format; // TextureのFormat
	resourceDesc.SampleDesc.Count = 1; // サンプリングカウント。1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);
	
	//2.利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; 
	
	//heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK; // WriteBackポリシーでCPUアクセス可能
	//heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0; // プロセッサの近くに配置
	
	//3.Resourceの生成
	ID3D12Resource* resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties, // Heapの設定
		D3D12_HEAP_FLAG_NONE, // Heapの特殊な設定。特になし
		&resourceDesc, // Resorceの設定
		D3D12_RESOURCE_STATE_COPY_DEST, // データ転送される設定
		nullptr, // Clear最適値。使わないのでnullptr
		IID_PPV_ARGS(&resource)); // 作成するResourceポインタへのポインタ
	assert(SUCCEEDED(hr));
	return resource;
}

// データを転送する関数
[[nodiscard]]
ID3D12Resource* UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages,
	ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(device, mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture, 0, UINT(subresources.size()));
	ID3D12Resource* intermediateResource = CreateBufferResource(device, intermediateSize);
	UpdateSubresources(commandList, texture, intermediateResource, 0, 0, UINT(subresources.size()), subresources.data());

	//Textureへの転送後は利用できるよう、D3D12_RESOURCE_STATE_COPY_DESからD3D12_RESOURCE_STATE_GENERIC_READへResourceStateを変更する
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);
	return intermediateResource;
}

//ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height) {
//	//生成するリソースの設定
//	D3D12_RESOURCE_DESC resourceDesc{};
//	resourceDesc.Width = width; // Textureの幅
//	resourceDesc.Height = height; // Textureの高さ
//	resourceDesc.MipLevels = 1; // mipmapの数
//	resourceDesc.DepthOrArraySize = 1; // 奥行き or 配列Textureの配列数
//	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // DepthStencilとしての利用可能なフォーマット
//	resourceDesc.SampleDesc.Count = 1; // サンプリングカウント。1固定
//	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2次元
//	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencilとして使う
//
//	//利用するヒープの設定
//	D3D12_HEAP_PROPERTIES heapProperties{};
//	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAMに作る
//
//	//深度のクリア設定
//	D3D12_CLEAR_VALUE depthClearValue{};
//	depthClearValue.DepthStencil.Depth = 1.0f; // 1.0f(最大値)でクリア
//	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // フォーマット。Resourceと合わせる
//
//	//Resourceの生成
//	ID3D12Resource* resource = nullptr;
//	HRESULT hr = device->CreateCommittedResource(
//		&heapProperties, // Heapの設定
//		D3D12_HEAP_FLAG_NONE, // Heapの特殊な設定。特になし
//		&resourceDesc, // Resourceの設定
//		D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値を書き込む状態にしておく
//		&depthClearValue, // Clear最適値
//		IID_PPV_ARGS(&resource)); // 作成するResourceポインタへのポインタ
//	assert(SUCCEEDED(hr));
//
//	return resource;
//}

D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}


//Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {

	/*WinApp* winApp = new WinApp();

	winApp->CreateGameWindow();*/

#ifdef _DEBUG
	ID3D12Debug1* debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		//デバッグレイヤーを有効にする
		debugController->EnableDebugLayer();
		//さらにGPU側でもチェックを行うようにする
		debugController->SetEnableGPUBasedValidation(TRUE);
	}
#endif // DEBUG

	DirectXCommon* directXCommon = new DirectXCommon();
	//directXCommon->Initialize(winApp);
	directXCommon->Initialize();

	HRESULT hr;

	////DXGIファクトリーの生成
	//IDXGIFactory7* dxgiFactory = nullptr;
	////HRESULはwindows系のエラーコードであり、
	////関数が成功したかどうかをSUCCEESESマクロで判断できる
	//HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	////初期化の根本的な部分でエラーが立た場合はプログラムが間違っているか
	////どうにもできない場合が多いのでassertにしておく
	//assert(SUCCEEDED(hr));

	////使用するアダプタ用の変数。最初にnullptrを入れておく
	//IDXGIAdapter4* useAdapter = nullptr;
	////良い順にアダプタを頼む
	//for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(
	//	i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
	//	//アダプタの情報を取得する
	//	DXGI_ADAPTER_DESC3 adapterDesc{};
	//	hr = useAdapter->GetDesc3(&adapterDesc);
	//	assert(SUCCEEDED(hr)); //取得できないのは一大事
	//	//ソフトウェアアダプタでなければ採用
	//	if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
	//		//採用したアダプタの情報をログに出力。wstringの方なので注意
	//		DebugLog::Log(DebugLog::ConvertString(std::format(L"Use Adapter : {}\n", adapterDesc.Description)));
	//		break;
	//	}
	//	useAdapter = nullptr;//ソフトウェアの場合見なかったことにする
	//}
	////適切なアダプタが見つからなかったので起動できない
	//assert(useAdapter != nullptr);

	//ID3D12Device* device = nullptr;
	////機能レベルとログ出力用の文字列
	//D3D_FEATURE_LEVEL featureLevels[] = {
	//	D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
	//};
	//const char* featureLevelStrings[] = { "12.2","12.1","12.0" };
	////高い順に生成できるか試していく
	//for (size_t i = 0; i < _countof(featureLevels); i++) {
	//	//採用したアダプタでデバイスを生成
	//	hr = D3D12CreateDevice(useAdapter, featureLevels[i], IID_PPV_ARGS(&device));
	//	//指定した機能レベルでデバイスが生成関たかを確認
	//	if (SUCCEEDED(hr)) {
	//		//生成できたのでログ出力を行ってループを抜ける
	//		DebugLog::Log(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
	//		break;
	//	}
	//}
	////デバイスの生成がうまくいかなかったので起動できない
	//assert(device != nullptr);
	//DebugLog::Log("Complete create D3D12Device!!!\n");// 初期化完了のログを出す

#ifdef _DEBUG
ID3D12InfoQueue* infoQueue = nullptr;
if (SUCCEEDED(directXCommon->device_->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
	//ヤバいエラーの時に止まる
	infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
	//エラーのときに止まる
	infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
	//警告時に止まる
	infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
	
	//抑制するメッセージのID
	D3D12_MESSAGE_ID denyIds[] = {
		//windows11でのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用バグによるエラーバグ
		//https:://stackoverflow.com/question/69805245/directx-12-application-is-crashing-in-windows-11
		D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
	};
	//抑制するレベル
	D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
	D3D12_INFO_QUEUE_FILTER filter{};
	filter.DenyList.NumIDs = _countof(denyIds);
	filter.DenyList.pIDList = denyIds;
	filter.DenyList.NumSeverities = _countof(severities);
	filter.DenyList.pSeverityList = severities;
	//指定したメッセージの表示を抑制する
	infoQueue->PushStorageFilter(&filter);
	
	//解放
	infoQueue->Release();
}
#endif // _DEBUG

	////コマンドキューを生成する
	//ID3D12CommandQueue* commandQueue = nullptr;
	//D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	//hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
	////コマンドキューの生成がうまくいかなかったので起動できない
	//assert(SUCCEEDED(hr));

	////コマンドアロケータを生成する
	//ID3D12CommandAllocator* commandAllocator = nullptr;
	//hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	////コマンドアロケータの生成がうまくいかなかったので起動できない
	//assert(SUCCEEDED(hr));

	////コマンドリストを生成する
	//ID3D12GraphicsCommandList* commandList = nullptr;
	//hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
	////コマンドリストの生成がうまくいかなかったので起動できない
	//assert(SUCCEEDED(hr));

	////スワップチェーンを生成する
	//IDXGISwapChain4* swapChain = nullptr;
	//DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	//swapChainDesc.Width = WinApp::kWindowWidth;	  //画面の幅。ウィンドウのクライアント領域を同じものにしておく
	//swapChainDesc.Height = WinApp::kWindowHeight; //画面の高さ。ウィンドウのクライアント領域を同じものにしておく
	//swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //色の形式
	//swapChainDesc.SampleDesc.Count = 1; //マルチサンプルしない
	//swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; //描画のターゲットとして利用
	//swapChainDesc.BufferCount = 2; //ダブルバッファ
	//swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; //モニタにうつしたら中身破棄
	////コマンドキュー、ウィンドウハンドル、設定を渡して生成する
	//hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue, winApp->GetHwnd(), &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(&swapChain));
	//assert(SUCCEEDED(hr));

	//ディスクリプタヒープの生成
	//RTV用のヒープでディスクリプタの数は2。RTVはShader内で触るものではないので、ShaderVisibleはfalse
	//ID3D12DescriptorHeap* rtvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	//SRV用のヒープでディスクリプタの数は128。SRVはShader内で触るものなので、ShaderVisibleはtrue
	ID3D12DescriptorHeap* srvDescriptorHeap = DirectXCommon::CreateDescriptorHeap(directXCommon->device_, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
	//DSV用のヒープでディスクリプタの数は1。DSVはShader内で触るものではないので、ShaderVisibleはfalse
	//ID3D12DescriptorHeap* dsvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	////SwapChainからReasourceを引っ張ってくる
	//ID3D12Resource* swapChainResources[2] = { nullptr };
	//hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
	////うまくできなければ起動しない
	//assert(SUCCEEDED(hr));
	//hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	//assert(SUCCEEDED(hr));

	////RTVの設定
	//D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	//rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; //出力結果をSRGBに変換して書き込む
	//rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; //2dテクスチャとして書き込む
	////ディスクリプトの先頭を取得する
	//D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	////RTVを2つ作るのでディスクリプタを2つ用意
	//D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2] = {};
	////まず1つ目を作る。1つ目は最初のところに作る。作る場所をこちらで指定してあげる必要がある。
	//rtvHandles[0] = rtvStartHandle;
	//device->CreateRenderTargetView(swapChainResources[0], &rtvDesc, rtvHandles[0]);
	////2つ目のディスクリプタハンドルを得る（自力で）
	//rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	////2つ目を作る
	//device->CreateRenderTargetView(swapChainResources[1], &rtvDesc, rtvHandles[1]);

	////DepthStencilTextureをウィンドウのサイズで作成
	//ID3D12Resource* depthStencilResource = CreateDepthStencilTextureResource(device, WinApp::kWindowWidth, WinApp::kWindowHeight);
	////DSVの設定
	//D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	//dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // Format。基本的にはResourceに合わせる
	//dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2dTexture
	//// DSVHeapの先頭にDSVを作る
	//device->CreateDepthStencilView(depthStencilResource, &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	////初期値0でFenceを作る
	//ID3D12Fence* fence = nullptr;
	//uint64_t fenceValue = 0;
	//hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	//assert(SUCCEEDED(hr));

	////FenceのSignalを持つためのイベントを作成する
	//HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	//assert(fence != nullptr);

	//DXCの初期化
	IDxcUtils* dxcUtils = nullptr;
	IDxcCompiler3* dxcCompiler = nullptr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	//現時点ではincludeはしないが、includeに対応するための設定を行っておく
	IDxcIncludeHandler* includeHandler = nullptr;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));

	//DescriptorRange
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0; // 0から始まる
	descriptorRange[0].NumDescriptors = 1; // 数は1つ
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // offsetを自動計算

	//PSO((Graphics)PipelineStateObject)の作成
	
	//RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	
	//RootParameter作成。複数設定ができるので配列。今回は結果1つだけなので長さ１の配列
	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[0].Descriptor.ShaderRegister = 0; // レジスタ番号0とバインド
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // VertexShaderで使う
	rootParameters[1].Descriptor.ShaderRegister = 0; // レジスタ番号0とバインド
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DesciptorTableを使う
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange; // Tableの中の配列を指定
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // Tableで利用する数
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[3].Descriptor.ShaderRegister = 1; // レジスタ番号1を使う
	descriptionRootSignature.pParameters = rootParameters; // ルートパラメータ配列へのポインタ
	descriptionRootSignature.NumParameters = _countof(rootParameters); // 配列の長さ

	//Samplerの設定
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // バイナリニアフィルタ
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 0~1の範囲外をリピート
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // 比較しない
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX; // ありったけのMipmapを使う
	staticSamplers[0].ShaderRegister = 0; // レジスタ番号0を使う
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	//シリアライズしてバイナリする
	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		DebugLog::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	//バイナリを元に生成
	ID3D12RootSignature* rootSignature = nullptr;
	hr = directXCommon->device_->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));

	//InputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	//BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};
	//すべての色要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	//裏面（時計回り）を表示
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	//三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	//Shaderをコンパイルする
	IDxcBlob* vertexShaderBlob = CompileShader(L"Object3d.VS.hlsl", L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(vertexShaderBlob != nullptr);
	
	IDxcBlob* pixelShaderBlob = CompileShader(L"Object3d.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(pixelShaderBlob != nullptr);

	// DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	// Depthの機能を有効化する
	depthStencilDesc.DepthEnable = true;;
	// 書き込みします
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	// 比較関数はLessEqual。つまり、近ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	
	//PSOを生成する
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature; // RootSignature
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc; // InputLayout
	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),vertexShaderBlob->GetBufferSize() }; // VertexShader
	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),pixelShaderBlob->GetBufferSize() }; // PixelShader
	graphicsPipelineStateDesc.BlendState = blendDesc; // BlendState
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc; // RasterizerState
	//書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	//利用するリポジトリ（形状）のタイプ。三角形
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//どのように画面に描き込むかの設定（気にしなくていい）
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	// DepthStencilの設定
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	//実際に生成
	ID3D12PipelineState* graphicsPipelineState = nullptr;
	hr = directXCommon->device_->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));

	//VertexResourceを生成する
	//実際に頂点リソースを作る
	ID3D12Resource* vertexResource = CreateBufferResource(directXCommon->device_, sizeof(VertexData) * 16 * 16 * 6);

	//マテリアル用のリソースを作る。今回はcolor1つ分を用意する
	ID3D12Resource* materialResource = CreateBufferResource(directXCommon->device_, sizeof(Material));
	//マテリアルデータを書き込む
	Material* materialData = nullptr;
	//書き込むためのアドレスを取得
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	//今回は赤を書き込んでいる
	*materialData = { Vector4(1.0f, 1.0f, 1.0f, 1.0f) , true };

	//WVP用のリソースを作る。Matrix4x4　1つ分のサイズを用意する
	ID3D12Resource* transformationMatrixResource = CreateBufferResource(directXCommon->device_, sizeof(TransformationMatrix));
	TransformationMatrix* transformationMatrixData = nullptr;
	transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));
	*transformationMatrixData = { Matrix4x4::MakeIdentity4x4() ,Matrix4x4::MakeIdentity4x4() };

	// Textureを読んで転送する
	DirectX::ScratchImage mipImages = LoadTexture("resources/uvChecker.png");
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	ID3D12Resource* textureResource = CreateTextureResource(directXCommon->device_, metadata);
	ID3D12Resource* intermediateResource = UploadTextureData(textureResource, mipImages, directXCommon->device_, directXCommon->commandList_);

	DirectX::ScratchImage mipImages2 = LoadTexture("resources/monsterBall.png");
	const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();
	ID3D12Resource* textureResource2 = CreateTextureResource(directXCommon->device_, metadata2);
	ID3D12Resource* intermediateResource2 = UploadTextureData(textureResource2, mipImages2, directXCommon->device_, directXCommon->commandList_);

	//metadataを基にSRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
	srvDesc2.Format = metadata2.format;
	srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);

	//SRVを作成するDescriptorHeapの場所を決める
	//サイズの取得。ゲーム実行中変化しない
	const uint32_t descriptorSizeSRV = directXCommon->device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const uint32_t descriptorSizeRTV = directXCommon->device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const uint32_t descriptorSizeDSV = directXCommon->device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	//先頭はImGuiが使っているのでその次を使う
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 1);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 1);
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 2);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 2);
	
	//SRVの生成
	directXCommon->device_->CreateShaderResourceView(textureResource, &srvDesc, textureSrvHandleCPU);
	directXCommon->device_->CreateShaderResourceView(textureResource2, &srvDesc2, textureSrvHandleCPU2);

	//VertexBufferViewを作成する
	//頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	//リソースの先頭のアドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	//使用するリソースのサイズは頂点3つ分のサイズ
	vertexBufferView.SizeInBytes = sizeof(VertexData) * 16 * 16 * 6;
	//頂点当たりのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	//Resourceにデータを書き込む
	//頂点リソースにデータを書き込む
	VertexData* vertexData = nullptr;
	//書き込むためのアドレスを取得
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	
	uint32_t kSubdivision = 16;
	float pi = 3.141592f;
	const float kLonEvery = pi * 2.0f / float(kSubdivision);
	const float kLatEvery = pi / float(kSubdivision);
	const float kEvery = 1.0f / float(kSubdivision);

	for (uint32_t latIndex = 0; latIndex < kSubdivision; latIndex++) {
		float lat = -pi / 2.0f + kLatEvery * latIndex;
		
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; lonIndex++) {
			float lon = lonIndex * kLonEvery;
			uint32_t start = (latIndex * kSubdivision + lonIndex) * 6;

			float u = float(lonIndex) / float(kSubdivision);
			float v = 1.0f - float(latIndex) / float(kSubdivision);

			//左下
			vertexData[start].position.x = std::cos(lat) * std::cos(lon);
			vertexData[start].position.y = std::sin(lat);
			vertexData[start].position.z = std::cos(lat) * std::sin(lon);
			vertexData[start].position.w = 1.0f;
			vertexData[start].texcoord.x = u;
			vertexData[start].texcoord.y = v;
			vertexData[start].normal.x = vertexData[start].position.x;
			vertexData[start].normal.y = vertexData[start].position.y;
			vertexData[start].normal.z = vertexData[start].position.z;
			//左上
			vertexData[start + 1].position.x = std::cos(lat + kLatEvery) * std::cos(lon);
			vertexData[start + 1].position.y = std::sin(lat + kLatEvery);
			vertexData[start + 1].position.z = std::cos(lat + kLatEvery) * std::sin(lon);
			vertexData[start + 1].position.w = 1.0f;
			vertexData[start + 1].texcoord.x = u;
			vertexData[start + 1].texcoord.y = v - kEvery;
			vertexData[start + 1].normal.x = vertexData[start + 1].position.x;
			vertexData[start + 1].normal.y = vertexData[start + 1].position.y;
			vertexData[start + 1].normal.z = vertexData[start + 1].position.z;
			//右下
			vertexData[start + 2].position.x = std::cos(lat) * std::cos(lon + kLonEvery);
			vertexData[start + 2].position.y = std::sin(lat);
			vertexData[start + 2].position.z = std::cos(lat) * std::sin(lon + kLonEvery);
			vertexData[start + 2].position.w = 1.0f;
			vertexData[start + 2].texcoord.x = u + kEvery;
			vertexData[start + 2].texcoord.y = v;
			vertexData[start + 2].normal.x = vertexData[start + 2].position.x;
			vertexData[start + 2].normal.y = vertexData[start + 2].position.y;
			vertexData[start + 2].normal.z = vertexData[start + 2].position.z;
			//右上
			vertexData[start + 3].position.x = std::cos(lat + kLatEvery) * std::cos(lon + kLonEvery);
			vertexData[start + 3].position.y = std::sin(lat + kLatEvery);
			vertexData[start + 3].position.z = std::cos(lat + kLatEvery) * std::sin(lon + kLonEvery);
			vertexData[start + 3].position.w = 1.0f;
			vertexData[start + 3].texcoord.x = u + kEvery;
			vertexData[start + 3].texcoord.y = v - kEvery;
			vertexData[start + 3].normal.x = vertexData[start + 3].position.x;
			vertexData[start + 3].normal.y = vertexData[start + 3].position.y;
			vertexData[start + 3].normal.z = vertexData[start + 3].position.z;
			//右下
			vertexData[start + 4].position.x = std::cos(lat) * std::cos(lon + kLonEvery);
			vertexData[start + 4].position.y = std::sin(lat);
			vertexData[start + 4].position.z = std::cos(lat) * std::sin(lon + kLonEvery);
			vertexData[start + 4].position.w = 1.0f;
			vertexData[start + 4].texcoord.x = u + kEvery;
			vertexData[start + 4].texcoord.y = v;
			vertexData[start + 4].normal.x = vertexData[start + 4].position.x;
			vertexData[start + 4].normal.y = vertexData[start + 4].position.y;
			vertexData[start + 4].normal.z = vertexData[start + 4].position.z;
			//左上
			vertexData[start + 5].position.x = std::cos(lat + kLatEvery) * std::cos(lon);
			vertexData[start + 5].position.y = std::sin(lat + kLatEvery);
			vertexData[start + 5].position.z = std::cos(lat + kLatEvery) * std::sin(lon);
			vertexData[start + 5].position.w = 1.0f;
			vertexData[start + 5].texcoord.x = u;
			vertexData[start + 5].texcoord.y = v - kEvery;
			vertexData[start + 5].normal.x = vertexData[start + 5].position.x;
			vertexData[start + 5].normal.y = vertexData[start + 5].position.y;
			vertexData[start + 5].normal.z = vertexData[start + 5].position.z;
		}
	}

	////左下
	//vertexDate[0].position = { -0.5f,-0.5f,0.0f,1.0f };
	//vertexDate[0].texcoord = { 0.0f,1.0f };
	////上
	//vertexDate[1].position = { 0.0f,0.5f,0.0f,1.0f };
	//vertexDate[1].texcoord = { 0.5f,0.0f };
	////右下
	//vertexDate[2].position = { 0.5f,-0.5f,0.0f,1.0f };
	//vertexDate[2].texcoord = { 1.0f,1.0f };

	////左下
	//vertexDate[3].position = { -0.5f,-0.5f,0.5f,1.0f };
	//vertexDate[3].texcoord = { 0.0f,1.0f };
	////上
	//vertexDate[4].position = { 0.0f,0.0f,0.0f,1.0f };
	//vertexDate[4].texcoord = { 0.5f,0.0f };
	////右下
	//vertexDate[5].position = { 0.5f,-0.5f,-0.5f,1.0f };
	//vertexDate[5].texcoord = { 1.0f,1.0f };

	//Sprite用の頂点リソースを作る
	ID3D12Resource* vertexResourceSprite = CreateBufferResource(directXCommon->device_, sizeof(VertexData) * 6);
	//頂点バッファーを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
	//リソースの先頭アドレスから使う
	vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
	//使用するリソースのサイズは頂点6つ分のサイズ
	vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;
	//1頂点あたりのサイズ
	vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

	VertexData* vertexDataSprite = nullptr;
	vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));
	//1枚目の三角形
	vertexDataSprite[0].position = { 0.0f,360.0f,0.0f,1.0f }; // 左下
	vertexDataSprite[0].texcoord = { 0.0f,1.0f }; 
	vertexDataSprite[0].normal = { 0.0f,0.0f, -1.0f };
	vertexDataSprite[1].position = { 0.0f,0.0f,0.0f,1.0f }; // 左上
	vertexDataSprite[1].texcoord = { 0.0f,0.0f };
	vertexDataSprite[1].normal = { 0.0f,0.0f, -1.0f };
	vertexDataSprite[2].position = { 640.0f,360.0f,0.0f,1.0f }; // 右下
	vertexDataSprite[2].texcoord = { 1.0f,1.0f };
	vertexDataSprite[2].normal = { 0.0f,0.0f, -1.0f };

	//2枚目の三角形
	vertexDataSprite[3].position = { 0.0f,00.0f,0.0f,1.0f }; // 左上
	vertexDataSprite[3].texcoord = { 0.0f,0.0f };
	vertexDataSprite[3].normal = { 0.0f,0.0f, -1.0f };
	vertexDataSprite[4].position = { 640.0f,0.0f,0.0f,1.0f }; // 右上
	vertexDataSprite[4].texcoord = { 1.0f,0.0f };
	vertexDataSprite[4].normal = { 0.0f,0.0f, -1.0f };
	vertexDataSprite[5].position = { 640.0f,360.0f,0.0f,1.0f }; // 右下
	vertexDataSprite[5].texcoord = { 1.0f,1.0f };
	vertexDataSprite[5].normal = { 0.0f,0.0f, -1.0f };

	//マテリアル用のリソースを作る。今回はcolor1つ分を用意する
	ID3D12Resource* materialResourceSprite = CreateBufferResource(directXCommon->device_, sizeof(Material));
	//マテリアルデータを書き込む
	Material* materialDataSprite = nullptr;
	//書き込むためのアドレスを取得\l
	materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));
	//今回は赤を書き込んでいる
	*materialDataSprite = { Vector4(1.0f, 1.0f, 1.0f, 1.0f) , false };

	//平行光源用のリソースを作る。
	ID3D12Resource* directionalLightResource = CreateBufferResource(directXCommon->device_, sizeof(DirectionalLight));
	//データを書き込む
	DirectionalLight* directionalLightData = nullptr;
	//書き込むためのアドレスを取得
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	//書き込んでいく
	directionalLightData->color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightData->direction = { 0.0f,-1.0f,0.0f };
	directionalLightData->intensity = 1.0f;

	//Sprite用のTransformationMatrix用のリソースを作る。Matrix4x4　1つ分のサイズを用意する
	ID3D12Resource* transformationMatrixResourceSprite = CreateBufferResource(directXCommon->device_, sizeof(TransformationMatrix));
	//データを書き込む
	TransformationMatrix* transformationMatrixDataSprite = nullptr;
	//書き込むためのアドレスを取得
	transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));
	//単位行列を書き込んでいく
	*transformationMatrixDataSprite = { Matrix4x4::MakeIdentity4x4() ,Matrix4x4::MakeIdentity4x4() };

	//ビューポート
	D3D12_VIEWPORT viewport{};
	//クライアント領域のサイズと一緒にして画面全体に表示
	viewport.Width = WinApp::kWindowWidth;
	viewport.Height = WinApp::kWindowHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	//シザー矩形
	D3D12_RECT scissorRect{};
	//基本的にビューポートと同じ矩形が構成されるようにする
	scissorRect.left = 0;
	scissorRect.right = WinApp::kWindowWidth;
	scissorRect.top = 0;
	scissorRect.bottom = WinApp::kWindowHeight;

	//ImGuiの初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(directXCommon->winApp_->GetHwnd());
	/*ImGui_ImplDX12_Init(directXCommon->device_,
		swapChainDesc.BufferCount,
		rtvDesc.Format,
		srvDescriptorHeap,
		srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());*/
	ImGui_ImplDX12_Init(directXCommon->device_,
		2,
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		srvDescriptorHeap,
		srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	//ゲームループ前。変数の作成、初期化スペース
	Transform transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

	Transform cameraTransform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-5.0f} };
	
	//CPUで動かす用のTransformを作る
	Transform transformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

	bool useMonsterBall = true;

	MSG msg{};
	//ウィンドウの×ボタンが押されるまでループ
	while (msg.message != WM_QUIT) {
		//windowにメッセージが来たら最優先で処理させる
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			//ゲーム処理

			//開発用UIの処理。実際に開発のUIを出す場合はここをゲーム固有の処理に置き換える
			//ImGui::ShowDemoWindow();
			ImGui::Checkbox("useMonsterBall", &useMonsterBall);
			ImGui::DragFloat4("directionalLightData.color", &directionalLightData->color.x, 0.01f);
			ImGui::DragFloat3("directionalLightData.direction", &directionalLightData->direction.x, 0.01f);
			ImGui::DragFloat("directionalLightData.intensity", &directionalLightData->intensity, 0.01f);

			directionalLightData->direction = Calc::Normalize(directionalLightData->direction);

			transform.rotate.y += 0.03f;
			Matrix4x4 worldMatrix = Matrix4x4::MakeAffinMatrix(transform.scale, transform.rotate, transform.translate);
			Matrix4x4 cameraMatrix = Matrix4x4::MakeAffinMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
			Matrix4x4 viewMatrix = Matrix4x4::Inverse(cameraMatrix);
			Matrix4x4 projectionMatrix = Matrix4x4::MakePerspectiveFovMatrix(0.45f, float(WinApp::kWindowWidth) / float(WinApp::kWindowHeight), 0.1f, 100.0f);
			Matrix4x4 worldViewProjectionMatrix = Matrix4x4::Multiply(worldMatrix, Matrix4x4::Multiply(viewMatrix, projectionMatrix));
			
			transformationMatrixData->WVP = worldViewProjectionMatrix;
			transformationMatrixData->World = worldMatrix;

			//Sprite用のWorldProjectionMatrixを作る
			Matrix4x4 worldMatrixSprite = Matrix4x4::MakeAffinMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
			Matrix4x4 viewMatrixSprite = Matrix4x4::MakeIdentity4x4();
			Matrix4x4 projectionMatrixSprite = Matrix4x4::MakeOrthographicMatrix(0.0f, 0.0f, float(WinApp::kWindowWidth), float(WinApp::kWindowHeight), 0.0f, 100.0f);
			Matrix4x4 worldViewProjectionMatrixSprite = Matrix4x4::Multiply(worldMatrixSprite, Matrix4x4::Multiply(viewMatrixSprite, projectionMatrixSprite));

			transformationMatrixDataSprite->WVP = worldViewProjectionMatrixSprite;
			transformationMatrixDataSprite->World = worldMatrixSprite;

			//ゲームの処理終了
			
			//描画の処理に入る前
			//ImGuiの内部コマンドを生成する
			ImGui::Render();

			//描画処理
			
			//これから書き込むバックバッファのインデックスを取得
			UINT backBufferIndex = directXCommon->swapChain_->GetCurrentBackBufferIndex();

			//TransitionBarrierの設定
			D3D12_RESOURCE_BARRIER barrier{};
			//今回のバリアはTransition
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			//Noneにしておく
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			//バリアを張る対象リソース。現在のバッグバッファに対して行う
			barrier.Transition.pResource = directXCommon->swapChainResources_[backBufferIndex];
			//遷移前（現在）のResourceState
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			//遷移後のResourceState
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			//TransitionBarrierを張る
			directXCommon->commandList_->ResourceBarrier(1, &barrier);

			//描画先のRTVを設定する
			// 描画先のRTVとDSVを設定する
			D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = directXCommon->dsvHeap_->GetCPUDescriptorHandleForHeapStart();
			directXCommon->commandList_->OMSetRenderTargets(1, &directXCommon->rtvHandles_[backBufferIndex], false, &dsvHandle);
			//directXCommon->commandList_->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, nullptr);
			//指定した深度で画面全体をクリアする
			directXCommon->commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
			//指定した色で画面全体をクリアする
			float clearColor[] = { 0.1f,0.25f,0.5f,1.0f }; //青っぽい色。RGBAの順
			directXCommon->commandList_->ClearRenderTargetView(directXCommon->rtvHandles_[backBufferIndex], clearColor, 0, nullptr);

			//描画用のDescriptorHeapの設定
			ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap };
			directXCommon->commandList_->SetDescriptorHeaps(1, descriptorHeaps);

			directXCommon->commandList_->RSSetViewports(1, &viewport); // Viewportを設定
			directXCommon->commandList_->RSSetScissorRects(1, &scissorRect); //Scissorを設定
			//RootSignatureを設定。PSOに設定しているけど別途設定が必要
			directXCommon->commandList_->SetGraphicsRootSignature(rootSignature);
			directXCommon->commandList_->SetPipelineState(graphicsPipelineState); // PSOを設定
			directXCommon->commandList_->IASetVertexBuffers(0, 1, &vertexBufferView); // VBVを設定
			//形状を設定。PSOに設定しているものとは別。同じものを設定すると考えておけばいい
			directXCommon->commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			//平行光源CBufferの場所を設定
			directXCommon->commandList_->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());

			//マテリアルCBufferの場所を設定
			directXCommon->commandList_->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
			//wvp用のBufferの場所を設定
			directXCommon->commandList_->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());
			//SRVのDescriptorTableの先頭に設定。2はrootParameter[2]である
			directXCommon->commandList_->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);
			//描画!!!!（DrawCall/ドローコール）。3頂点で1つのインスタンス。インスタンスについては今後
			directXCommon->commandList_->DrawInstanced(16 * 16 * 6, 1, 0, 0);
			//Spriteの描画。変更に必要なものだけ変更する
			directXCommon->commandList_->IASetVertexBuffers(0, 1, &vertexBufferViewSprite); // VBVを設定
			//マテリアルCBufferの場所を設定
			directXCommon->commandList_->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());
			//TransformationMatrixCBufferの場所を設定
			directXCommon->commandList_->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());

			directXCommon->commandList_->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);
			//描画!!!!（DrawCall/ドローコール）
			directXCommon->commandList_->DrawInstanced(6, 1, 0, 0);

			//実際のdirectXCommon->commandList_のImGuiの描画コマンドを積む。描画処理の終わったタイミング
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), directXCommon->commandList_);

			//描画処理終わり

			//画面に描く処理はすべて終わり、画面にうつすので、状態の遷移
			//今回はRenderTargetからPresentにする
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			//TransitionBarrierを張る
			directXCommon->commandList_->ResourceBarrier(1, &barrier);

			//コマンドリストの内容を確定させる
			hr = directXCommon->commandList_->Close();
			assert(SUCCEEDED(hr));

			//GPUにコマンドリストの実行を行わせる
			ID3D12CommandList* commandLists[] = { directXCommon->commandList_ };
			directXCommon->commandQueue_->ExecuteCommandLists(1, commandLists);
			//GPUとOSに画面の交換を行うように通知する
			directXCommon->swapChain_->Present(1, 0);

			//Fenceの値の更新
			directXCommon->fenceValue_++;
			//GPUがここまでたどり着いたときに、Fenceの値を指定した値に代入するようにSignelを送る
			directXCommon->commandQueue_->Signal(directXCommon->fence_, directXCommon->fenceValue_);

			//Fenceの値が指定したSignal値にたどりすいているか確認する
			//GetCompletedValueの初期値はFence作成時に渡した初期値
			if (directXCommon->fence_->GetCompletedValue() < directXCommon->fenceValue_) {
				//指定したSignalにたどり着いていないので、たどり着くまで待つようにイベントする
				directXCommon->fence_->SetEventOnCompletion(directXCommon->fenceValue_, directXCommon->fenceEvent_);
				//イベントを待つ
				WaitForSingleObject(directXCommon->fenceEvent_, INFINITE);
			}

			//次のフレーム用のコマンドリストを準備
			hr = directXCommon->commandAllocator_->Reset();
			assert(SUCCEEDED(hr));
			hr = directXCommon->commandList_->Reset(directXCommon->commandAllocator_, nullptr);
			assert(SUCCEEDED(hr));

		}
	}

	//COMの終了処理
	CoUninitialize();

	//解放処理
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	//CloseHandle(fenceEvent);
	//fence->Release();
	//rtvDescriptorHeap->Release();
	srvDescriptorHeap->Release();
	/*swapChainResources[0]->Release();
	swapChainResources[1]->Release();
	swapChain->Release();
	commandList->Release();
	commandAllocator->Release();
	commandQueue->Release();
	device->Release();
	useAdapter->Release();
	dxgiFactory->Release();*/
	vertexResource->Release();
	graphicsPipelineState->Release();
	signatureBlob->Release();
	if (errorBlob) {
		errorBlob->Release();
	}
	rootSignature->Release();
	pixelShaderBlob->Release();
	vertexShaderBlob->Release();
	materialResource->Release();
	materialResourceSprite->Release();
	directionalLightResource->Release();
	transformationMatrixResource->Release();
	textureResource->Release();
	intermediateResource->Release();
	textureResource2->Release();
	intermediateResource2->Release();
	//depthStencilResource->Release();
	//dsvDescriptorHeap->Release();
	vertexResourceSprite->Release();
	transformationMatrixResourceSprite->Release();

	
#ifdef _DEBUG
	debugController->Release();
#endif // _DEBUG
	//CloseWindow(winApp->GetHwnd());
	//delete winApp;
	delete directXCommon;

	//リソースリークチェック
	IDXGIDebug1* debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		debug->Release();
	}

	return 0;
}