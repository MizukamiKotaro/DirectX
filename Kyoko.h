#pragma once
class Kyoko
{

public:
	/// <summary>
	/// システム全体の初期化
	/// <param name="title">タイトルバー</param>
	/// <param name="width">ウィンドウ（クライアント領域）の幅</param>
	/// <param name="height">ウィンドウ（クライアント領域）の高さ</param>
	/// </summary>
	static void Initialize(const char* title, int width = 1280, int height = 720);

	/// <summary>
	/// システム全体の終了
	/// </summary>
	static void Finalize();
};

