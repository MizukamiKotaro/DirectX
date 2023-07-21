#pragma once

class Kyoko
{
public:
	static void Initialize();

	/// <summary>
	/// システム全体の終了
	/// </summary>
	static void Finalize();

	/// <summary>
	/// 画像ファイルを読み込む
	/// </summary>
	/// <param name="fileName">ファイル名</param>
	/// <returns>テクスチャのハンドル</returns>
	//static int LoadTexture(const char* fileName);

	/// <summary>
	/// 画像ファイルの読み込みを解除する
	/// </summary>
	/// <param name="textureHandle">テクスチャハンドル</param>
	//static void UnloadTexture(int textureHandle);

	/// <summary>
	/// Windowsのメッセージを処理する
	/// </summary>
	/// <returns>1: ゲーム終了 0: ゲーム継続</returns>
	//static int ProcessMessage();

	/// <summary>
	/// フレーム開始処理。ブレンドモードはkBlendNormalに設定されます。
	/// </summary>
	//static void BeginFrame();

	/// <summary>
	/// フレーム終了処理
	/// </summary>
	//static void EndFrame();
};

