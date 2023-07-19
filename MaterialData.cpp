#include "MaterialData.h"
#include <fstream>
#include <sstream>
#include <cassert>

MaterialData MaterialData::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& fileName) {

	// 1. 中で必要となる変数の宣言
	MaterialData materialData; // 構築するMaterialData
	std::string line; // ファイルから読んだ１行を格納するもの

	// 2. ファイルを開く
	std::ifstream file(directoryPath + "/" + fileName); // ファイルを開く
	assert(file.is_open());

	// 3. 実際にファイルを読み、MaterialDataを構築していく
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		// identifierに応じた処理
		if (identifier == "map_Kd") {
			std::string textureFileName;
			s >> textureFileName;
			// 連結してファイルパスにする
			materialData.textureFilePath = directoryPath + "/" + textureFileName;

		}

	}

	// 4. MaterialDataを返す
	return materialData;
}