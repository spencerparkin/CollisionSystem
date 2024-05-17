#pragma once

#include "Reference.h"
#include "rapidjson/document.h"
#include <string>
#include <unordered_map>
#include <filesystem>

class Game;
class Asset;
class RenderObject;

/**
 * This is supposed to be one-stop-shopping for any asset we'd want to load,
 * whether that be a render-mesh, texture, shader, etc.
 */
class AssetCache : public ReferenceCounted
{
public:
	AssetCache();
	virtual ~AssetCache();

	bool GrabAsset(const std::string& assetFile, Reference<Asset>& asset);

	/**
	 * Remove all assets from this cache.
	 */
	void Clear();

	void SetAssetFolder(const std::string& assetFolder) { this->assetFolder = assetFolder; }

	std::string GetAssetFolder() const { return this->assetFolder.string(); }

private:

	bool ResolveAssetPath(std::string& assetFile);

	std::filesystem::path assetFolder;

	typedef std::unordered_map<std::string, Reference<Asset>> AssetMap;
	AssetMap assetMap;
};

/**
 * This is the base class for all asset types.
 */
class Asset : public ReferenceCounted
{
public:
	Asset();
	virtual ~Asset();

	virtual bool Load(const rapidjson::Document& jsonDoc, AssetCache* assetCache) = 0;
	virtual bool Unload() = 0;
	virtual bool CanBeCached() const { return true; }
	virtual bool MakeRenderInstance(Reference<RenderObject>& renderObject) { return false; }
};