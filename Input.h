#pragma once
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")
#include <wrl.h>

#include <vector>

class Input
{
public:

	static Input* GetInstance();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(const HWND hwnd, const HINSTANCE hInstance);

	/// <summary>
	/// 毎フレーム処理
	/// </summary>
	void Update();

private:
	Input() = default;
	~Input() = default;
	Input(const Input&) = delete;
	const Input& operator=(const Input&) = delete;

private:
	Microsoft::WRL::ComPtr<IDirectInput8> dInput_;
	Microsoft::WRL::ComPtr<IDirectInputDevice8> devKeyboard_;
	Microsoft::WRL::ComPtr<IDirectInputDevice8> devMouse_;
	std::vector<Joystick> devJoysticks_;
	
	BYTE key_[256] = {};
	BYTE keyPre_[256] = {};

};

