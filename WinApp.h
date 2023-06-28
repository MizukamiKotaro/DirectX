#pragma once

#include <Windows.h>
#include<cstdint>

class WinApp
{
public: // 静的メンバ変数
	// ウィンドウサイズ
	static const int kWindowWidth = 1280; // 横幅
	static const int kWindowHeight = 720; // 縦幅

public: // 静的メンバ関数
	//ウィンドウプロシャープ
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

public: // メンバ関数
	WinApp();
	~WinApp();

	void CreateGameWindow();

	HWND GetHwnd() const { return hwnd_; }



private: // メンバ変数
	// Window関連
	HWND hwnd_ = nullptr;   // ウィンドウハンドル
};

