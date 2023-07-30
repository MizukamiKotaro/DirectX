#pragma once

struct Vector2
{
public:


	Vector2& operator=(const Vector2& obj) {
		x = obj.x;
		y = obj.y;
		return *this;
	}

	void operator+=(const Vector2& obj) {
		this->x = this->x + obj.x;
		this->y = this->y + obj.y;
	}

	void  operator-=(const Vector2& obj) {
		this->x -= obj.x;
		this->y -= obj.y;
	}

	void operator*=(float a) {
		this->x *= a;
		this->y *= a;
	}

	void operator/=(float a) {
		this->x /= a;
		this->y /= a;
	}

public:
	float x;
	float y;
};

Vector2 operator+(const Vector2& obj1, const Vector2& obj2);

Vector2 operator-(const Vector2& obj1, const Vector2& obj2);

Vector2 operator*(const Vector2& obj, float a);

Vector2 operator*(float a, const Vector2& obj);

Vector2 operator/(const Vector2& obj, float a);

//DirectX::ScratchImage mipImages4 = LoadTexture(modelData.material.textureFilePath);
//const DirectX::TexMetadata& metadata4 = mipImages4.GetMetadata();
//ID3D12Resource* textureResource4 = CreateTextureResource(device, metadata4);
//ID3D12Resource* intermediateResource4 = UploadTextureData(textureResource4, mipImages4, device, commandList);
//
//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc4{};
//srvDesc4.Format = metadata4.format;
//srvDesc4.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
//srvDesc4.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
//srvDesc4.Texture2D.MipLevels = UINT(metadata4.mipLevels);