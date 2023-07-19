#pragma once
#include "VertexData.h"
#include <vector>
#include <string>
#include "MaterialData.h"

struct ModelData
{
	std::vector<VertexData> verteces;
	MaterialData material;

	//static ModelData LoadObjeFile(const std::string& fileName);

	static ModelData LoadObjeFile(const std::string& directoryPath, const std::string& fileName);
};
