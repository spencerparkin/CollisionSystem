#include "SkinnedRenderMesh.h"
#include "RenderObjects/AnimatedMeshInstance.h"
#include "Skeleton.h"

using namespace Collision;

SkinnedRenderMesh::SkinnedRenderMesh()
{
}

/*virtual*/ SkinnedRenderMesh::~SkinnedRenderMesh()
{
}

/*virtual*/ bool SkinnedRenderMesh::Load(const rapidjson::Document& jsonDoc, AssetCache* assetCache)
{
	if (!RenderMeshAsset::Load(jsonDoc, assetCache))
		return false;

	if (!jsonDoc.HasMember("skeleton") || !jsonDoc["skeleton"].IsString())
		return false;

	std::string skeletonFile = jsonDoc["skeleton"].GetString();
	Reference<Asset> asset;
	if (!assetCache->LoadAsset(skeletonFile, asset))
		return false;

	this->skeleton.SafeSet(asset.Get());
	if (!this->skeleton)
		return false;

	return true;
}

/*virtual*/ bool SkinnedRenderMesh::Unload()
{
	RenderMeshAsset::Unload();

	return true;
}

void SkinnedRenderMesh::DeformMesh()
{
	// TODO: Write this.
}

/*virtual*/ bool SkinnedRenderMesh::MakeRenderInstance(Reference<RenderObject>& renderObject)
{
	renderObject.Set(new AnimatedMeshInstance());
	auto instance = dynamic_cast<AnimatedMeshInstance*>(renderObject.Get());
	instance->SetRenderMesh(this);
	instance->SetBoundingBox(this->objectSpaceBoundingBox);
	return true;
}