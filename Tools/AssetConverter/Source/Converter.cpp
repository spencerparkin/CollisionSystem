#include "Converter.h"
#include "Log.h"
#include <wx/filename.h>
#include <fstream>
#include "assimp/postprocess.h"
#include "assimp/config.h"
#include "Math/AxisAlignedBoundingBox.h"
#include "AssetCache.h"
#include "App.h"
#include "Frame.h"
#include <wx/textdlg.h>
#include "rapidjson/reader.h"
#include "rapidjson/error/en.h"
#include "rapidjson/istreamwrapper.h"

Converter::Converter(const wxString& assetRootFolder)
{
	this->assetRootFolder = assetRootFolder;
}

/*virtual*/ Converter::~Converter()
{
}

bool Converter::Convert(const wxString& assetFile, uint32_t flags)
{
	LOG("Converting file: %s", (const char*)assetFile.c_str());

	wxFileName fileName(assetFile);
	this->assetFolder = fileName.GetPath();
	LOG("Assets will be dumped in folder: %s", (const char*)this->assetFolder.c_str());

	this->importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 1.0);

	LOG("Calling Ass-Imp to load file: %s", (const char*)assetFile.c_str());
	const aiScene* scene = importer.ReadFile(assetFile.c_str(), aiProcess_GlobalScale | aiProcess_PopulateArmatureData);
	if (!scene)
	{
		LOG("Import error: %s", importer.GetErrorString());
		return false;
	}

	if ((flags & Flag::CONVERT_MESHES) != 0)
	{
		LOG("Generating node-to-world transformation map...");
		this->nodeToWorldMap.clear();
		if (!this->GenerateNodeToWorldMap(scene->mRootNode))
		{
			LOG("Failed to generate transformation map.");
			return false;
		}

		LOG("Processing scene graph...");
		if (!this->ProcessSceneGraph(scene, scene->mRootNode))
		{
			LOG("Failed to process scene graph.");
			return false;
		}
	}

	if ((flags & Flag::CONVERT_ANIMATIONS) != 0)
	{
		LOG("Found %d animations.", scene->mNumAnimations);
		for (int i = 0; i < scene->mNumAnimations; i++)
		{
			LOG("Processing animation %d of %d.", i + 1, scene->mNumAnimations);
			const aiAnimation* animation = scene->mAnimations[i];
			if (!this->ProcessAnimation(scene, animation))
			{
				LOG("Failed to process animation.");
				return false;
			}
		}
	}

	return true;
}

bool Converter::ProcessAnimation(const aiScene* scene, const aiAnimation* animation)
{
	wxString animName(animation->mName.C_Str());
	animName.Replace(" ", "_");

	wxTextEntryDialog nameDialog(wxGetApp().GetFrame(), wxString::Format("The current animation will be saved as \"%s\".  Rename it?", animation->mName.C_Str()), "Rename?", animName);
	if (nameDialog.ShowModal() == wxID_OK)
		animName = nameDialog.GetValue();

	wxFileName animFile;
	animFile.SetPath(this->assetRootFolder + "/Animations");
	animFile.SetName(animName);
	animFile.SetExt("animation");

	Imzadi::Animation generatedAnimation;
	generatedAnimation.SetName((const char*)animName.c_str());
	double currentTick = -std::numeric_limits<double>::max();
	Imzadi::KeyFrame* keyFrame = nullptr;
	while (this->FindNextKeyFrame(animation, currentTick, keyFrame))
		generatedAnimation.AddKeyFrame(keyFrame);

	rapidjson::Document animDoc;
	generatedAnimation.Save(animDoc);
	if (!this->WriteJsonFile(animDoc, animFile.GetFullPath()))
		return false;

	return true;
}

bool Converter::FindNextKeyFrame(const aiAnimation* animation, double& currentTick, Imzadi::KeyFrame*& keyFrame)
{
	double soonestTick = std::numeric_limits<double>::max();

	for (int i = 0; i < animation->mNumChannels; i++)
	{
		const aiNodeAnim* nodeAnim = animation->mChannels[i];
		
		for (int j = 0; j < nodeAnim->mNumPositionKeys; j++)
		{
			const aiVectorKey* positionKey = &nodeAnim->mPositionKeys[j];
			if (positionKey->mTime > currentTick && positionKey->mTime < soonestTick)
				soonestTick = positionKey->mTime;
		}

		for (int j = 0; j < nodeAnim->mNumRotationKeys; j++)
		{
			const aiQuatKey* rotationKey = &nodeAnim->mRotationKeys[j];
			if (rotationKey->mTime > currentTick && rotationKey->mTime < soonestTick)
				soonestTick = rotationKey->mTime;
		}

		for (int j = 0; j < nodeAnim->mNumScalingKeys; j++)
		{
			const aiVectorKey* scalingKey = &nodeAnim->mScalingKeys[j];
			if (scalingKey->mTime > currentTick && scalingKey->mTime < soonestTick)
				soonestTick = scalingKey->mTime;
		}
	}

	if (soonestTick == std::numeric_limits<double>::max())
		return false;

	currentTick = soonestTick;

	keyFrame = new Imzadi::KeyFrame();
	keyFrame->SetTime(soonestTick / animation->mTicksPerSecond);

	// We pose all the nodes in a single key-frame.
	for (int i = 0; i < animation->mNumChannels; i++)
	{
		const aiNodeAnim* nodeAnim = animation->mChannels[i];

		Imzadi::KeyFrame::PoseInfo poseInfo;
		poseInfo.boneName = nodeAnim->mNodeName.C_Str();

		int findCount = 0;

		for (int j = 0; j < nodeAnim->mNumPositionKeys - 1; j++)
		{
			const aiVectorKey* positionKeyA = &nodeAnim->mPositionKeys[j];
			const aiVectorKey* positionKeyB = &nodeAnim->mPositionKeys[j + 1];

			if (positionKeyA->mTime <= currentTick && currentTick <= positionKeyB->mTime)
			{
				Imzadi::Vector3 translationA, translationB;
				this->MakeVector(translationA, positionKeyA->mValue);
				this->MakeVector(translationB, positionKeyB->mValue);
				double alpha = (currentTick - positionKeyA->mTime) / (positionKeyB->mTime - positionKeyA->mTime);
				poseInfo.childToParent.translation.Lerp(translationA, translationB, alpha);
				findCount++;
				break;
			}
		}

		for (int j = 0; j < nodeAnim->mNumRotationKeys - 1; j++)
		{
			const aiQuatKey* rotationKeyA = &nodeAnim->mRotationKeys[j];
			const aiQuatKey* rotationKeyB = &nodeAnim->mRotationKeys[j + 1];

			if (rotationKeyA->mTime <= currentTick && currentTick <= rotationKeyB->mTime)
			{
				Imzadi::Quaternion rotationA, rotationB;
				this->MakeQuat(rotationA, rotationKeyA->mValue);
				this->MakeQuat(rotationB, rotationKeyB->mValue);
				double alpha = (currentTick - rotationKeyA->mTime) / (rotationKeyB->mTime - rotationKeyA->mTime);
				poseInfo.childToParent.rotation.Interpolate(rotationA, rotationB, alpha);
				findCount++;
				break;
			}
		}

		for (int j = 0; j < nodeAnim->mNumScalingKeys - 1; j++)
		{
			const aiVectorKey* scalingKeyA = &nodeAnim->mScalingKeys[j];
			const aiVectorKey* scalingKeyB = &nodeAnim->mScalingKeys[j + 1];

			if (scalingKeyA->mTime <= currentTick && currentTick <= scalingKeyB->mTime)
			{
				Imzadi::Vector3 scaleA, scaleB;
				this->MakeVector(scaleA, scalingKeyA->mValue);
				this->MakeVector(scaleB, scalingKeyB->mValue);
				double alpha = (currentTick - scalingKeyA->mTime) / (scalingKeyB->mTime - scalingKeyA->mTime);
				poseInfo.childToParent.scale.Lerp(scaleA, scaleB, alpha);
				findCount++;
				break;
			}
		}

		if (!poseInfo.childToParent.IsValid())
			LOG("Warning: Encountered invalid child-to-parent transform!");

		if (findCount == 3)
			keyFrame->AddPoseInfo(poseInfo);
		else
			LOG("Failed to find scale, position and translation at tick %f!", currentTick);
	}
	
	return true;
}

bool Converter::GenerateNodeToWorldMap(const aiNode* node)
{
	Imzadi::Transform nodeToWorld;

	if (!node->mParent)
		nodeToWorld.SetIdentity();
	else
	{
		Imzadi::Transform parentNodeToWorld;
		if (!this->GetNodeToWorldTransform(node->mParent, parentNodeToWorld))
		{
			LOG("Failed to find parent node's transform in the map.");
			return false;
		}

		Imzadi::Transform childToParent;
		if (!this->MakeTransform(childToParent, node->mTransformation))
		{
			LOG("Failed to make child-to-parent transform from node matrix.");
			return false;
		}

		nodeToWorld = parentNodeToWorld * childToParent;
	}

	this->nodeToWorldMap.insert(std::pair<const aiNode*, Imzadi::Transform>(node, nodeToWorld));

	for (int i = 0; i < node->mNumChildren; i++)
	{
		const aiNode* childNode = node->mChildren[i];
		this->GenerateNodeToWorldMap(childNode);
	}

	return true;
}

bool Converter::GetNodeToWorldTransform(const aiNode* node, Imzadi::Transform& nodeToWorld)
{
	std::unordered_map<const aiNode*, Imzadi::Transform>::iterator iter = this->nodeToWorldMap.find(node);
	if (iter == this->nodeToWorldMap.end())
		return false;

	nodeToWorld = iter->second;
	return true;
}

bool Converter::ProcessSceneGraph(const aiScene* scene, const aiNode* node)
{
	LOG("Procesing node: %s", node->mName.C_Str());

	if (node->mNumMeshes > 0)
	{
		LOG("Found %d meshe(s).", node->mNumMeshes);

		for (int i = 0; i < node->mNumMeshes; i++)
		{
			LOG("Processing mesh %d of %d.", i + 1, node->mNumMeshes);
			const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			if (!this->ProcessMesh(scene, node, mesh))
			{
				LOG("Mesh processing failed!");
				return false;
			}
		}
	}
	
	for (int i = 0; i < node->mNumChildren; i++)
	{
		const aiNode* childNode = node->mChildren[i];
		if (!this->ProcessSceneGraph(scene, childNode))
			return false;
	}

	return true;
}

bool Converter::ProcessMesh(const aiScene* scene, const aiNode* node, const aiMesh* mesh)
{
	LOG("Processing mesh: %s", mesh->mName.C_Str());

	wxFileName meshFileName;
	meshFileName.SetPath(this->assetFolder);
	meshFileName.SetName(mesh->mName.C_Str());
	meshFileName.SetExt(mesh->HasBones() ? "skinned_render_mesh" : "render_mesh");

	wxFileName textureFileName;
	textureFileName.SetPath(this->assetFolder);
	textureFileName.SetName(mesh->mName.C_Str());
	textureFileName.SetExt("texture");

	wxFileName vertexBufferFileName;
	vertexBufferFileName.SetPath(this->assetFolder);
	vertexBufferFileName.SetName(wxString(mesh->mName.C_Str()) + "_Vertices");
	vertexBufferFileName.SetExt("buffer");

	wxFileName indexBufferFileName;
	indexBufferFileName.SetPath(this->assetFolder);
	indexBufferFileName.SetName(wxString(mesh->mName.C_Str()) + "_Indices");
	indexBufferFileName.SetExt("buffer");

	rapidjson::Document meshDoc;
	meshDoc.SetObject();

	meshDoc.AddMember("primitive_type", rapidjson::Value().SetString("TRIANGLE_LIST"), meshDoc.GetAllocator());
	meshDoc.AddMember("shader", rapidjson::Value().SetString("Shaders/Standard.shader", meshDoc.GetAllocator()), meshDoc.GetAllocator());
	meshDoc.AddMember("shadow_shader", rapidjson::Value().SetString("Shaders/StandardShadow.shader", meshDoc.GetAllocator()), meshDoc.GetAllocator());
	meshDoc.AddMember("texture", rapidjson::Value().SetString(this->MakeAssetFileReference(textureFileName.GetFullPath()), meshDoc.GetAllocator()), meshDoc.GetAllocator());
	meshDoc.AddMember("index_buffer", rapidjson::Value().SetString(this->MakeAssetFileReference(indexBufferFileName.GetFullPath()), meshDoc.GetAllocator()), meshDoc.GetAllocator());
	meshDoc.AddMember("vertex_buffer", rapidjson::Value().SetString(this->MakeAssetFileReference(vertexBufferFileName.GetFullPath()), meshDoc.GetAllocator()), meshDoc.GetAllocator());

	if (scene->mNumMaterials <= mesh->mMaterialIndex)
	{
		LOG("Error: Bad material index: %d", mesh->mMaterialIndex);
		return false;
	}

	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
	if (material->GetTextureCount(aiTextureType_DIFFUSE) != 1)
	{
		LOG("Error: Material does not have exactly one diffuse texture.");
		return false;
	}

	aiString texturePath;
	if (aiReturn_SUCCESS != material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath))
	{
		LOG("Failed to acquire texture path from material.");
		return false;
	}

	wxString textureFullPath = this->assetFolder + wxString::Format("/%s", texturePath.C_Str());
	LOG("Found texture: %s", (const char*)textureFullPath.c_str());

	rapidjson::Document textureDoc;
	textureDoc.SetObject();
	textureDoc.AddMember("flip_vertical", rapidjson::Value().SetBool(true), textureDoc.GetAllocator());
	textureDoc.AddMember("image_file", rapidjson::Value().SetString(this->MakeAssetFileReference(textureFullPath), textureDoc.GetAllocator()), textureDoc.GetAllocator());

	if (mesh->mNumVertices == 0)
	{
		LOG("Error: No vertices found.");
		return false;
	}

	if (mesh->mNumUVComponents[0] != 2)
	{
		LOG("Error: Expected exactly 2 UV components in first channel.");
		return false;
	}

	rapidjson::Document verticesDoc;
	verticesDoc.SetObject();

	rapidjson::Value vertexBufferValue;
	vertexBufferValue.SetArray();

	Imzadi::AxisAlignedBoundingBox* boundingBox = nullptr;

	Imzadi::Transform nodeToWorld;
	if (!this->GetNodeToWorldTransform(node, nodeToWorld))
	{
		LOG("Error: Failed to get node-to-world transform for mesh node.");
		return false;
	}

	for (int i = 0; i < mesh->mNumVertices; i++)
	{
		Imzadi::Vector3 position, normal;
		Imzadi::Vector2 texCoords;

		if (!this->MakeVector(position, mesh->mVertices[i]))
			return false;

		if (!this->MakeTexCoords(texCoords, mesh->mTextureCoords[0][i]))
			return false;

		if (!this->MakeVector(normal, mesh->mNormals[i]))
			return false;

		position = nodeToWorld.TransformPoint(position);
		normal = nodeToWorld.TransformVector(normal);

		if (!normal.Normalize())
			return false;

		vertexBufferValue.PushBack(rapidjson::Value().SetFloat(position.x), verticesDoc.GetAllocator());
		vertexBufferValue.PushBack(rapidjson::Value().SetFloat(position.y), verticesDoc.GetAllocator());
		vertexBufferValue.PushBack(rapidjson::Value().SetFloat(position.z), verticesDoc.GetAllocator());
		vertexBufferValue.PushBack(rapidjson::Value().SetFloat(texCoords.x), verticesDoc.GetAllocator());
		vertexBufferValue.PushBack(rapidjson::Value().SetFloat(texCoords.y), verticesDoc.GetAllocator());
		vertexBufferValue.PushBack(rapidjson::Value().SetFloat(normal.x), verticesDoc.GetAllocator());
		vertexBufferValue.PushBack(rapidjson::Value().SetFloat(normal.y), verticesDoc.GetAllocator());
		vertexBufferValue.PushBack(rapidjson::Value().SetFloat(normal.z), verticesDoc.GetAllocator());

		if (boundingBox)
			boundingBox->Expand(position);
		else
			boundingBox = new Imzadi::AxisAlignedBoundingBox(position);
	}

	verticesDoc.AddMember("bind", rapidjson::Value().SetString("vertex", verticesDoc.GetAllocator()), verticesDoc.GetAllocator());
	verticesDoc.AddMember("stride", rapidjson::Value().SetInt(8), verticesDoc.GetAllocator());
	verticesDoc.AddMember("type", rapidjson::Value().SetString("float", verticesDoc.GetAllocator()), verticesDoc.GetAllocator());
	verticesDoc.AddMember("buffer", vertexBufferValue, verticesDoc.GetAllocator());

	if (mesh->HasBones())
	{
		verticesDoc.AddMember("usage", rapidjson::Value().SetString("dynamic", verticesDoc.GetAllocator()), verticesDoc.GetAllocator());
		verticesDoc.AddMember("bare_buffer", rapidjson::Value().SetBool(true), verticesDoc.GetAllocator());
	}

	rapidjson::Value boundingBoxValue;
	Imzadi::Asset::SaveBoundingBox(boundingBoxValue, *boundingBox, &meshDoc);
	meshDoc.AddMember("bounding_box", boundingBoxValue, meshDoc.GetAllocator());
	delete boundingBox;

	if (mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
	{
		LOG("Error: Only triangle primitive currently supported.");
		return false;
	}

	if (mesh->mNumFaces == 0)
	{
		LOG("Error: No faces found.");
		return false;
	}

	rapidjson::Document indicesDoc;
	indicesDoc.SetObject();

	rapidjson::Value indexBufferValue;
	indexBufferValue.SetArray();

	for (int i = 0; i < mesh->mNumFaces; i++)
	{
		const aiFace* face = &mesh->mFaces[i];
		if (face->mNumIndices != 3)
		{
			LOG("Error: Expected exactly 3 indices in face.");
			return false;
		}

		for (int j = 0; j < face->mNumIndices; j++)
		{
			unsigned int index = face->mIndices[j];
			indexBufferValue.PushBack(rapidjson::Value().SetUint(index), indicesDoc.GetAllocator());

			if (index != static_cast<unsigned int>(static_cast<unsigned short>(index)))
			{
				LOG("Error: Index (%d) doesn't fit in an unsigned short.", index);
				return false;
			}
		}
	}

	indicesDoc.AddMember("bind", rapidjson::Value().SetString("index", indicesDoc.GetAllocator()), indicesDoc.GetAllocator());
	indicesDoc.AddMember("stride", rapidjson::Value().SetInt(1), indicesDoc.GetAllocator());
	indicesDoc.AddMember("type", rapidjson::Value().SetString("ushort", indicesDoc.GetAllocator()), indicesDoc.GetAllocator());
	indicesDoc.AddMember("buffer", indexBufferValue, indicesDoc.GetAllocator());

	if (mesh->HasBones())
	{
		wxFileName skeletonFileName;
		skeletonFileName.SetPath(this->assetFolder);
		skeletonFileName.SetName(mesh->mName.C_Str());
		skeletonFileName.SetExt("skeleton");

		wxFileName skinWeightsFileName;
		skinWeightsFileName.SetPath(this->assetFolder);
		skinWeightsFileName.SetName(mesh->mName.C_Str());
		skinWeightsFileName.SetExt("skin_weights");

		meshDoc.AddMember("position_offset", rapidjson::Value().SetInt(0), meshDoc.GetAllocator());
		meshDoc.AddMember("normal_offset", rapidjson::Value().SetInt(20), meshDoc.GetAllocator());
		meshDoc.AddMember("skeleton", rapidjson::Value().SetString(this->MakeAssetFileReference(skeletonFileName.GetFullPath()), meshDoc.GetAllocator()), meshDoc.GetAllocator());
		meshDoc.AddMember("skin_weights", rapidjson::Value().SetString(this->MakeAssetFileReference(skinWeightsFileName.GetFullPath()), meshDoc.GetAllocator()), meshDoc.GetAllocator());

		// Note that for this to work with 3Ds Max, the scene should have been exported
		// with the model in bind-pose.  Even if posed, I don't know why it doesn't already
		// do that, but whatever.
		Imzadi::Skeleton skeleton;
		if (!this->GenerateSkeleton(skeleton, mesh))
		{
			LOG("Failed to generate skeleton.");
			return false;
		}

		std::string error;

		rapidjson::Document skeletonDoc;
		skeletonDoc.SetObject();
		if (!skeleton.Save(skeletonDoc))
		{
			LOG("Failed to save skeleton.");
			return false;
		}

		Imzadi::SkinWeights skinWeights;
		if (!this->GenerateSkinWeights(skinWeights, mesh))
		{
			LOG("Failed to generate skin weights.");
			return false;
		}

		rapidjson::Document skinWeightsDoc;
		skinWeightsDoc.SetObject();
		if (!skinWeights.Save(skinWeightsDoc))
		{
			LOG("Failed to save skin-weights.");
			return false;
		}

		if (!this->WriteJsonFile(skeletonDoc, skeletonFileName.GetFullPath()))
			return false;

		if (!this->WriteJsonFile(skinWeightsDoc, skinWeightsFileName.GetFullPath()))
			return false;

		rapidjson::Value animationsArrayValue;
		animationsArrayValue.SetArray();
		this->GatherApplicableAnimations(animationsArrayValue, &skeleton, &meshDoc);
		meshDoc.AddMember("animations", animationsArrayValue, meshDoc.GetAllocator());
	}

	if (!this->WriteJsonFile(meshDoc, meshFileName.GetFullPath()))
		return false;

	if (!this->WriteJsonFile(textureDoc, textureFileName.GetFullPath()))
		return false;

	if (!this->WriteJsonFile(verticesDoc, vertexBufferFileName.GetFullPath()))
		return false;

	if (!this->WriteJsonFile(indicesDoc, indexBufferFileName.GetFullPath()))
		return false;

	return true;
}

void Converter::GatherApplicableAnimations(rapidjson::Value& animationsArrayValue, const Imzadi::Skeleton* skeleton, rapidjson::Document* doc)
{
	wxString animationsFolder = this->assetRootFolder + "/Animations";
	std::filesystem::path animationsFolderPath((const char*)animationsFolder.c_str());

	try
	{
		for (const auto dirEntry : std::filesystem::recursive_directory_iterator(animationsFolderPath))
		{
			if (dirEntry.is_regular_file())
			{
				std::string ext = dirEntry.path().extension().string();
				if (ext == ".animation")
				{
					wxString animationPath(dirEntry.path().c_str());
					if (this->IsAnimationApplicable(animationPath, skeleton))
					{
						wxString animationRef = this->MakeAssetFileReference(animationPath);
						rapidjson::Value animationRefValue;
						animationRefValue.SetString(animationRef, doc->GetAllocator());
						animationsArrayValue.PushBack(animationRefValue, doc->GetAllocator());
					}
				}
			}
		}
	}
	catch (std::filesystem::filesystem_error error)
	{
		LOG("Error: %s", error.what());
	}
}

bool Converter::IsAnimationApplicable(const wxString& animationFile, const Imzadi::Skeleton* skeleton)
{
	rapidjson::Document animDoc;
	if (!this->ReadJsonFile(animDoc, animationFile))
	{
		LOG("Warning: Could not open file: %s", (const char*)animationFile.c_str());
		return false;
	}

	Imzadi::Animation animation;
	if (!animation.Load(animDoc, nullptr))
	{
		LOG("Warning: Could not load animation file: %s", (const char*)animationFile.c_str());
		return false;
	}

	return animation.CanAnimateSkeleton(skeleton, 0.5);
}

bool Converter::GenerateSkeleton(Imzadi::Skeleton& skeleton, const aiMesh* mesh)
{
	if (mesh->mNumBones == 0)
	{
		LOG("No bones found!");
		return false;
	}

	// It is not obvious at all what subset of the node hierarchy actually constitutes the skeleton.
	// I get the impression that all this data is just a ridiculous mess.
	std::unordered_set<const aiNode*> boneSet;
	for (int i = 0; i < mesh->mNumBones; i++)
	{
		const aiBone* bone = mesh->mBones[i];
		boneSet.insert(bone->mNode);
	}

	const aiNode* rootBoneNode = nullptr;
	for (int i = 0; i < mesh->mNumBones; i++)
	{
		const aiBone* bone = mesh->mBones[i];
		if (!this->FindParentBones(bone->mNode, boneSet))
		{
			if (!rootBoneNode)
				rootBoneNode = bone->mNode;
			else
			{
				LOG("Error: There can be only one root bone node, but found another!");
				return false;
			}
		}

		for (int j = 0; j < bone->mNode->mNumChildren; j++)
			boneSet.insert(bone->mNode->mChildren[j]);
	}

	if (!rootBoneNode)
	{
		LOG("Error: Did not find root bone of skeleton.");
		return false;
	}

	while (rootBoneNode->mParent)
	{
		rootBoneNode = rootBoneNode->mParent;
		boneSet.insert(rootBoneNode);
	}

	skeleton.SetRootBone(new Imzadi::Bone());
	if (!this->GenerateSkeleton(skeleton.GetRootBone(), rootBoneNode, boneSet))
		return false;

	return true;
}

bool Converter::FindParentBones(const aiNode* boneNode, std::unordered_set<const aiNode*>& boneSet)
{
	const aiNode* parentNode = boneNode->mParent;
	if (!parentNode)
		return false;

	if (boneSet.find(parentNode) != boneSet.end())
		return true;

	if (!this->FindParentBones(parentNode, boneSet))
		return false;

	boneSet.insert(parentNode);
	return true;
}

bool Converter::GenerateSkeleton(Imzadi::Bone* bone, const aiNode* boneNode, const std::unordered_set<const aiNode*>& boneSet)
{
	if (boneSet.find(boneNode) == boneSet.end())
		return false;

	bone->SetName(boneNode->mName.C_Str());
	
	Imzadi::Transform childToParent;
	if (!this->MakeTransform(childToParent, boneNode->mTransformation))
		return false;

	bone->SetBindPoseChildToParent(childToParent);

	for (int i = 0; i < boneNode->mNumChildren; i++)
	{
		const aiNode* childNode = boneNode->mChildren[i];
		if (boneSet.find(childNode) != boneSet.end())
		{
			auto childBone = new Imzadi::Bone();
			bone->AddChildBone(childBone);
			if (!this->GenerateSkeleton(childBone, childNode, boneSet))
				return false;
		}
	}

	return true;
}

bool Converter::GenerateSkinWeights(Imzadi::SkinWeights& skinWeights, const aiMesh* mesh)
{
	skinWeights.SetNumVertices(mesh->mNumVertices);

	for (int i = 0; i < mesh->mNumBones; i++)
	{
		const aiBone* bone = mesh->mBones[i];

		for (int j = 0; j < bone->mNumWeights; j++)
		{
			const aiVertexWeight* vertexWeight = &bone->mWeights[j];

			if (vertexWeight->mVertexId >= skinWeights.GetNumVertices())
			{
				LOG("Error: Vertex weight index (%d) out of range (max: %d).", vertexWeight->mVertexId, skinWeights.GetNumVertices() - 1);
				return false;
			}

			std::vector<Imzadi::SkinWeights::BoneWeight>& boneWeightArray = skinWeights.GetBonesWeightsForVertex(vertexWeight->mVertexId);
			Imzadi::SkinWeights::BoneWeight boneWeight;
			boneWeight.boneName = bone->mName.C_Str();
			boneWeight.weight = vertexWeight->mWeight;
			boneWeightArray.push_back(boneWeight);
		}
	}

	return true;
}

bool Converter::MakeTransform(Imzadi::Transform& transformOut, const aiMatrix4x4& matrixIn)
{
	if (matrixIn.d1 != 0.0 || matrixIn.d2 != 0.0 || matrixIn.d3 != 0.0 || matrixIn.d4 != 1.0)
		return false;

	transformOut.matrix.ele[0][0] = matrixIn.a1;
	transformOut.matrix.ele[0][1] = matrixIn.a2;
	transformOut.matrix.ele[0][2] = matrixIn.a3;

	transformOut.matrix.ele[1][0] = matrixIn.b1;
	transformOut.matrix.ele[1][1] = matrixIn.b2;
	transformOut.matrix.ele[1][2] = matrixIn.b3;

	transformOut.matrix.ele[2][0] = matrixIn.c1;
	transformOut.matrix.ele[2][1] = matrixIn.c2;
	transformOut.matrix.ele[2][2] = matrixIn.c3;

	transformOut.translation.x = matrixIn.a4;
	transformOut.translation.y = matrixIn.b4;
	transformOut.translation.z = matrixIn.c4;

	return true;
}

bool Converter::MakeVector(Imzadi::Vector3& vectorOut, const aiVector3D& vectorIn)
{
	vectorOut.SetComponents(vectorIn.x, vectorIn.y, vectorIn.z);
	return true;
}

bool Converter::MakeTexCoords(Imzadi::Vector2& texCoordsOut, const aiVector3D& texCoordsIn)
{
	texCoordsOut.SetComponents(texCoordsIn.x, texCoordsIn.y);
	return true;
}

bool Converter::MakeQuat(Imzadi::Quaternion& quaternionOut, const aiQuaternion& quaternionIn)
{
	quaternionOut.w = quaternionIn.w;
	quaternionOut.x = quaternionIn.x;
	quaternionOut.y = quaternionIn.y;
	quaternionOut.z = quaternionIn.z;
	return true;
}

bool Converter::WriteJsonFile(const rapidjson::Document& jsonDoc, const wxString& assetFile)
{
	std::filesystem::path assetPath((const char*)assetFile.c_str());
	if (std::filesystem::exists(assetPath))
	{
		std::filesystem::remove(assetPath);
		LOG("Deleted file: %s", (const char*)assetFile.c_str());
	}

	std::ofstream fileStream;
	fileStream.open((const char*)assetFile.c_str(), std::ios::out);
	if (!fileStream.is_open())
	{
		LOG("Failed to open (for writing) the file: %s", (const char*)assetFile.c_str());
		return false;
	}

	rapidjson::StringBuffer stringBuffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> prettyWriter(stringBuffer);
	if (!jsonDoc.Accept(prettyWriter))
	{
		LOG("Failed to generate JSON text from JSON data for file: %s", (const char*)assetFile.c_str());
		return false;
	}

	fileStream << stringBuffer.GetString();
	fileStream.close();
	LOG("Wrote file: %s", (const char*)assetFile.c_str());
	return true;
}

bool Converter::ReadJsonFile(rapidjson::Document& jsonDoc, const wxString& assetFile)
{
	std::ifstream fileStream;
	fileStream.open((const char*)assetFile.c_str(), std::ios::in);
	if (!fileStream.is_open())
	{
		LOG("Failed to open (for reading) the file: %s", (const char*)assetFile.c_str());
		return false;
	}

	rapidjson::IStreamWrapper streamWrapper(fileStream);
	jsonDoc.ParseStream(streamWrapper);
	if (jsonDoc.HasParseError())
	{
		LOG("Failed to parse file: %s", (const char*)assetFile.c_str());
		rapidjson::ParseErrorCode errorCode = jsonDoc.GetParseError();
		LOG("Parser error: %s", rapidjson::GetParseError_En(errorCode));
		return false;
	}

	fileStream.close();
	return true;
}

wxString Converter::MakeAssetFileReference(const wxString& assetFile)
{
	wxFileName fileName(assetFile);
	fileName.MakeRelativeTo(this->assetRootFolder);
	wxString relativeAssetPath = fileName.GetFullPath();
	return relativeAssetPath;
}