#include "DebugLines.h"
#include "AssetCache.h"
#include "Game.h"
#include "Camera.h"
#include "Math/Matrix4x4.h"

using namespace Collision;

DebugLines::DebugLines()
{
	this->vertexBuffer = nullptr;
	this->maxLines = 10000;
}

/*virtual*/ DebugLines::~DebugLines()
{
	this->shader.Reset();
	SafeRelease(this->vertexBuffer);
}

/*virtual*/ void DebugLines::Render(Camera* camera, RenderPass renderPass)
{
	if (renderPass != RenderPass::MAIN_PASS)
		return;

	if (this->lineArray.size() == 0)
		return;

	if (!this->shader)
	{
		Reference<Asset> asset;
		if (!Game::Get()->GetAssetCache()->LoadAsset("Shaders/DebugLine.shader", asset))
			return;

		this->shader.SafeSet(asset.Get());
	}

	if (!this->vertexBuffer)
	{
		D3D11_BUFFER_DESC bufferDesc{};
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.ByteWidth = sizeof(float) * 12 * this->maxLines;		// 12 floats per line; 2 vertices, 2 colors, each 3 floats.
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		HRESULT result = Game::Get()->GetDevice()->CreateBuffer(&bufferDesc, NULL, &this->vertexBuffer);
		if (FAILED(result))
			return;
	}

	assert(this->lineArray.size() <= this->maxLines);

	ID3D11DeviceContext* deviceContext = Game::Get()->GetDeviceContext();

	D3D11_MAPPED_SUBRESOURCE mappedSubresource{};
	HRESULT result = deviceContext->Map(this->vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
	if (FAILED(result))
		return;

	float* floatPtr = static_cast<float*>(mappedSubresource.pData);
	for (const Line& line : this->lineArray)
	{
		*floatPtr++ = line.segment.point[0].x;
		*floatPtr++ = line.segment.point[0].y;
		*floatPtr++ = line.segment.point[0].z;

		*floatPtr++ = line.color.x;
		*floatPtr++ = line.color.y;
		*floatPtr++ = line.color.z;

		*floatPtr++ = line.segment.point[1].x;
		*floatPtr++ = line.segment.point[1].y;
		*floatPtr++ = line.segment.point[1].z;

		*floatPtr++ = line.color.x;
		*floatPtr++ = line.color.y;
		*floatPtr++ = line.color.z;
	}

	deviceContext->Unmap(this->vertexBuffer, 0);

	ID3D11Buffer* constantsBuffer = this->shader->GetConstantsBuffer();
	if (!constantsBuffer)
		return;

	ZeroMemory(&mappedSubresource, sizeof(mappedSubresource));
	result = deviceContext->Map(constantsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
	if (FAILED(result))
		return;

	Matrix4x4 worldToCameraMat;
	camera->GetWorldToCameraTransform().GetToMatrix(worldToCameraMat);

	Matrix4x4 cameraToProjMat;
	camera->GetProjectionMatrix(cameraToProjMat);

	Matrix4x4 worldToProjMat;
	worldToProjMat = cameraToProjMat * worldToCameraMat;

	const Shader::Constant* constant = nullptr;
	if (this->shader->GetConstantInfo("worldToProjection", constant))
		StoreShaderConstant(&mappedSubresource, constant, &worldToProjMat);

	deviceContext->Unmap(constantsBuffer, 0);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	deviceContext->IASetInputLayout(shader->GetInputLayout());

	deviceContext->VSSetShader(shader->GetVertexShader(), NULL, 0);
	deviceContext->PSSetShader(shader->GetPixelShader(), NULL, 0);

	deviceContext->VSSetConstantBuffers(0, 1, &constantsBuffer);

	UINT stride = 6 * sizeof(float);
	UINT offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &this->vertexBuffer, &stride, &offset);

	deviceContext->Draw(this->lineArray.size() * 2, 0);
}

/*virtual*/ void DebugLines::GetWorldBoundingSphere(Collision::Vector3& center, double& radius) const
{
	// TODO: Write this.
}

bool DebugLines::AddLine(const Line& line)
{
	if (this->lineArray.size() >= this->maxLines)
		return false;

	this->lineArray.push_back(line);
	return true;
}

void DebugLines::Clear()
{
	this->lineArray.clear();
}

/*virtual*/ int DebugLines::SortKey() const
{
	// Always draw debug lines after everything else.
	return std::numeric_limits<int>::max();
}