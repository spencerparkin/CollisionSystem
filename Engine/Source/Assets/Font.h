#pragma once

#include "AssetCache.h"
#include "Assets/Texture.h"
#include "Math/Vector2.h"

namespace Imzadi
{
	/**
	 * An instance of this class contains all the information we need to
	 * render a certain font.
	 */
	class IMZADI_API Font : public Asset
	{
	public:
		Font();
		virtual ~Font();

		virtual bool Load(const rapidjson::Document& jsonDoc, AssetCache* assetCache) override;
		virtual bool Unload() override;

	private:
		struct CharacterInfo
		{
			Vector2 minUV;
			Vector2 maxUV;
		};

		typedef std::vector<CharacterInfo> CharacterInfoArray;

		CharacterInfoArray charInfoArray;
		Reference<Texture> textureAtlas;
	};
}