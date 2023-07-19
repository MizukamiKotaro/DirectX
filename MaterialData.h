#pragma once
#include <string>

struct MaterialData
{
	std::string textureFilePath;

	static MaterialData LoadMaterialTemplateFile(const std::string& direct, const std::string& fileName);
};

