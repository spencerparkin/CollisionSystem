#include "Skeleton.h"
#include "Game.h"

using namespace Imzadi;

//-------------------------------- Skeleton --------------------------------

Skeleton::Skeleton()
{
	this->rootBone = nullptr;
	this->boneMapValid = false;
}

/*virtual*/ Skeleton::~Skeleton()
{
	this->SetRootBone(nullptr);
}

/*virtual*/ bool Skeleton::Load(const rapidjson::Document& jsonDoc, AssetCache* assetCache)
{
	if (!jsonDoc.HasMember("root_bone") || !jsonDoc["root_bone"].IsObject())
		return false;

	const rapidjson::Value& rootBoneValue = jsonDoc["root_bone"];
	this->SetRootBone(new Bone());
	if (!this->rootBone->Load(rootBoneValue))
		return false;
	
	this->UpdateCachedTransforms(BoneTransformType::BIND_POSE);
	this->ResetCurrentPose();

	return true;
}

/*virtual*/ bool Skeleton::Unload()
{
	this->SetRootBone(nullptr);
	return true;
}

/*virtual*/ bool Skeleton::Save(rapidjson::Document& jsonDoc) const
{
	if (!this->rootBone)
		return false;

	jsonDoc.SetObject();

	rapidjson::Value rootBoneValue;
	if (!this->rootBone->Save(rootBoneValue, &jsonDoc))
		return false;

	jsonDoc.AddMember("root_bone", rootBoneValue, jsonDoc.GetAllocator());

	return true;
}

void Skeleton::SetRootBone(Bone* bone)
{
	delete this->rootBone;
	this->rootBone = bone;
	this->boneMapValid = false;
}

void Skeleton::InvalidateBoneMap() const
{
	this->boneMapValid = false;
}

Bone* Skeleton::FindBone(const std::string& name)
{
	if (!this->rootBone)
		return nullptr;

	if (!this->boneMapValid)
	{
		this->boneMap.clear();
		this->rootBone->PopulateBoneMap(this->boneMap);
		this->boneMapValid = true;
	}

	BoneMap::iterator iter = this->boneMap.find(name);
	if (iter == this->boneMap.end())
		return nullptr;

	return iter->second;
}

const Bone* Skeleton::FindBone(const std::string& name) const
{
	return const_cast<Skeleton*>(this)->FindBone(name);
}

void Skeleton::DebugDraw(BoneTransformType transformType, const Transform& objectToWorld) const
{
	if (this->rootBone)
	{
		this->UpdateCachedTransforms(transformType);
		this->rootBone->DebugDraw(transformType, objectToWorld);
	}
}

void Skeleton::UpdateCachedTransforms(BoneTransformType transformType) const
{
	if (this->rootBone)
		const_cast<Bone*>(this->rootBone)->UpdateCachedTransforms(transformType);
}

void Skeleton::MakeBasicBiped()
{
	this->SetRootBone(new Bone());

	// Root.
	Bone* root = this->GetRootBone();
	root->SetName("Root");
	root->SetWeightable(false);
	root->SetBindPoseLength(2.7);
	root->SetBindPoseOrientation(Matrix3x3(Vector3(0.0, 0.0, 1.0), M_PI / 2.0));

	// Torso.
	Bone* torso = new Bone();
	torso->SetName("Torso");
	torso->SetBindPoseLength(1.7);
	torso->SetBindPoseOrientation(Matrix3x3().SetIdentity());
	root->AddChildBone(torso);

	// Neck.
	Bone* neck = new Bone();
	neck->SetName("Neck");
	neck->SetBindPoseLength(0.5);
	neck->SetBindPoseOrientation(Matrix3x3().SetIdentity());
	torso->AddChildBone(neck);

	// Head.
	Bone* head = new Bone();
	head->SetName("Head");
	head->SetBindPoseLength(1.0);
	head->SetBindPoseOrientation(Matrix3x3().SetIdentity());
	neck->AddChildBone(head);

	// Left shoulder.
	Bone* leftShoulder = new Bone();
	leftShoulder->SetName("LeftShoulder");
	leftShoulder->SetBindPoseLength(0.8);
	leftShoulder->SetBindPoseOrientation(Matrix3x3(Vector3(0.0, 0.0, 1.0), M_PI / 2.0));
	torso->AddChildBone(leftShoulder);

	// Right shoulder.
	Bone* rightShoulder = new Bone();
	rightShoulder->SetName("RightShoulder");
	rightShoulder->SetBindPoseLength(0.8);
	rightShoulder->SetBindPoseOrientation(Matrix3x3(Vector3(0.0, 0.0, 1.0), -M_PI / 2.0));
	torso->AddChildBone(rightShoulder);

	// Left upper arm.
	Bone* leftUpperArm = new Bone();
	leftUpperArm->SetName("LeftUpperArm");
	leftUpperArm->SetBindPoseLength(1.2);
	leftUpperArm->SetBindPoseOrientation(Matrix3x3().SetIdentity());
	leftShoulder->AddChildBone(leftUpperArm);

	// Right upper arm.
	Bone* rightUpperArm = new Bone();
	rightUpperArm->SetName("RightUpperArm");
	rightUpperArm->SetBindPoseLength(1.2);
	rightUpperArm->SetBindPoseOrientation(Matrix3x3().SetIdentity());
	rightShoulder->AddChildBone(rightUpperArm);

	// Left lower arm.
	Bone* leftLowerArm = new Bone();
	leftLowerArm->SetName("LeftLowerArm");
	leftLowerArm->SetBindPoseLength(1.2);
	leftLowerArm->SetBindPoseOrientation(Matrix3x3().SetIdentity());
	leftUpperArm->AddChildBone(leftLowerArm);

	// Right lower arm.
	Bone* rightLowerArm = new Bone();
	rightLowerArm->SetName("RightLowerArm");
	rightLowerArm->SetBindPoseLength(1.2);
	rightLowerArm->SetBindPoseOrientation(Matrix3x3().SetIdentity());
	rightUpperArm->AddChildBone(rightLowerArm);

	// Left hip socket.
	Bone* leftHip = new Bone();
	leftHip->SetName("LeftHip");
	leftHip->SetBindPoseLength(0.5);
	leftHip->SetBindPoseOrientation(Matrix3x3(Vector3(0.0, 0.0, 1.0), M_PI / 2.0));
	root->AddChildBone(leftHip);

	// Right hip socket.
	Bone* rightHip = new Bone();
	rightHip->SetName("RightHip");
	rightHip->SetBindPoseLength(0.5);
	rightHip->SetBindPoseOrientation(Matrix3x3(Vector3(0.0, 0.0, 1.0), -M_PI / 2.0));
	root->AddChildBone(rightHip);

	// Left upper leg.
	Bone* leftUpperLeg = new Bone();
	leftUpperLeg->SetName("LeftUpperLeg");
	leftUpperLeg->SetBindPoseLength(1.25);
	leftUpperLeg->SetBindPoseOrientation(Matrix3x3(Vector3(0.0, 0.0, 1.0), M_PI / 2.0));
	leftHip->AddChildBone(leftUpperLeg);

	// Right upper leg.
	Bone* rightUpperLeg = new Bone();
	rightUpperLeg->SetName("RightUpperLeg");
	rightUpperLeg->SetBindPoseLength(1.25);
	rightUpperLeg->SetBindPoseOrientation(Matrix3x3(Vector3(0.0, 0.0, 1.0), -M_PI / 2.0));
	rightHip->AddChildBone(rightUpperLeg);

	// Left lower leg.
	Bone* leftLowerLeg = new Bone();
	leftLowerLeg->SetName("LeftLowerLeg");
	leftLowerLeg->SetBindPoseLength(1.25);
	leftLowerLeg->SetBindPoseOrientation(Matrix3x3().SetIdentity());
	leftUpperLeg->AddChildBone(leftLowerLeg);

	// Right lower leg.
	Bone* rightLowerLeg = new Bone();
	rightLowerLeg->SetName("RightLowerLeg");
	rightLowerLeg->SetBindPoseLength(1.25);
	rightLowerLeg->SetBindPoseOrientation(Matrix3x3().SetIdentity());
	rightUpperLeg->AddChildBone(rightLowerLeg);
}

void Skeleton::ResetCurrentPose()
{
	std::vector<Bone*> boneArray;

	if (this->GatherBones(boneArray))
	{
		for (Bone* bone : boneArray)
		{
			bone->SetCurrentPoseOrientation(bone->GetBindPoseOrientation());
			bone->SetCurrentPoseLength(bone->GetBindPoseLength());
		}
	}
}

bool Skeleton::GatherBones(std::vector<Bone*>& boneArray)
{
	if (!this->rootBone)
		return false;

	boneArray.clear();
	std::list<Bone*> boneQueue;
	boneQueue.push_back(this->rootBone);
	while (boneQueue.size() > 0)
	{
		Bone* bone = boneQueue.front();
		boneQueue.pop_front();

		for (int i = 0; i < bone->GetNumChildBones(); i++)
			boneQueue.push_back(bone->GetChildBone(i));

		boneArray.push_back(bone);
	}

	return true;
}

bool Skeleton::GatherBones(const Vector3& position, BoneTransformType boneTransformType, std::vector<Bone*>& boneArray)
{
	if (!this->GatherBones(boneArray))
		return false;

	std::sort(boneArray.begin(), boneArray.end(), [&position, boneTransformType](const Bone* boneA, const Bone* boneB) -> bool {
		const Vector3& centerA = boneA->CalcObjectSpaceCenter(boneTransformType);
		const Vector3& centerB = boneB->CalcObjectSpaceCenter(boneTransformType);
		double distanceA = (position - centerA).Length();
		double distanceB = (position - centerB).Length();
		return distanceA < distanceB;
	});

	return true;
}

//-------------------------------- Bone --------------------------------

Bone::Bone()
{
	this->canBeWeightedAgainst = true;
	this->parentBone = nullptr;
	this->bindPose.boneState.orientation.SetIdentity();
	this->bindPose.boneState.length = 1.0;
	this->currentPose.boneState.orientation.SetIdentity();
	this->currentPose.boneState.length = 1.0;
}

/*virtual*/ Bone::~Bone()
{
	this->DeleteAllChildBones();
}

bool Bone::Load(const rapidjson::Value& boneValue)
{
	if (!boneValue.HasMember("name") || !boneValue["name"].IsString())
		return false;

	this->name = boneValue["name"].GetString();

	if (!boneValue.HasMember("bind_pose_orientation"))
		return false;

	if (!Asset::LoadMatrix(boneValue["bind_pose_orientation"], this->bindPose.boneState.orientation))
		return false;

	if (!boneValue.HasMember("bind_pose_length") || !boneValue["bind_pose_length"].IsFloat())
		return false;

	this->bindPose.boneState.length = boneValue["bind_pose_length"].GetFloat();

	if (!boneValue.HasMember("weightable") || !boneValue["weightable"].IsBool())
		return false;

	this->canBeWeightedAgainst = boneValue["weightable"].GetBool();

	if (!boneValue.HasMember("child_bone_array") || !boneValue["child_bone_array"].IsArray())
		return false;

	const rapidjson::Value& childBoneArrayValue = boneValue["child_bone_array"];
	this->DeleteAllChildBones();
	for (int i = 0; i < childBoneArrayValue.Size(); i++)
	{
		const rapidjson::Value& childBoneValue = childBoneArrayValue[i];
		Bone* childBone = new Bone();
		childBone->SetParentBone(this);
		this->childBoneArray.push_back(childBone);
		if (!childBone->Load(childBoneValue))
			return false;
	}

	return true;
}

bool Bone::Save(rapidjson::Value& boneValue, rapidjson::Document* doc) const
{
	rapidjson::Value bindPoseOrientationValue;
	Asset::SaveMatrix(bindPoseOrientationValue, this->bindPose.boneState.orientation, doc);

	boneValue.SetObject();
	boneValue.AddMember("name", rapidjson::Value().SetString(this->name.c_str(), doc->GetAllocator()), doc->GetAllocator());
	boneValue.AddMember("bind_pose_orientation", bindPoseOrientationValue, doc->GetAllocator());
	boneValue.AddMember("bind_pose_length", rapidjson::Value().SetFloat(this->bindPose.boneState.length), doc->GetAllocator());
	boneValue.AddMember("weightable", rapidjson::Value().SetBool(this->canBeWeightedAgainst), doc->GetAllocator());

	rapidjson::Value childBoneArrayValue;
	childBoneArrayValue.SetArray();

	for (const Bone* bone : this->childBoneArray)
	{
		rapidjson::Value childBoneValue;
		if (!bone->Save(childBoneValue, doc))
			return false;

		childBoneArrayValue.PushBack(childBoneValue, doc->GetAllocator());
	}

	boneValue.AddMember("child_bone_array", childBoneArrayValue, doc->GetAllocator());

	return true;
}

void Bone::AddChildBone(Bone* bone)
{
	this->childBoneArray.push_back(bone);
	bone->SetParentBone(this);
}

void Bone::DeleteAllChildBones()
{
	for (Bone* bone : this->childBoneArray)
		delete bone;

	this->childBoneArray.clear();
}

const Bone::Transforms* Bone::GetTransforms(BoneTransformType transformType) const
{
	return const_cast<Bone*>(this)->GetTransforms(transformType);
}

Bone::Transforms* Bone::GetTransforms(BoneTransformType transformType)
{
	switch (transformType)
	{
	case BoneTransformType::BIND_POSE:
		return &this->bindPose;
	case BoneTransformType::CURRENT_POSE:
		return &this->currentPose;
	}

	return nullptr;
}

void Bone::UpdateCachedTransforms(BoneTransformType transformType)
{
	Transforms* parentTransforms = this->parentBone ? this->parentBone->GetTransforms(transformType) : nullptr;
	Transforms* transforms = this->GetTransforms(transformType);

	Vector3 boneVector(transforms->boneState.length, 0.0, 0.0);
	boneVector = transforms->boneState.orientation * boneVector;

	if (!parentTransforms)
	{
		transforms->boneToObject.matrix = transforms->boneState.orientation;
		transforms->boneToObject.translation = boneVector;
	}
	else
	{
		transforms->boneToObject.matrix = transforms->boneState.orientation * parentTransforms->boneToObject.matrix;
		transforms->boneToObject.translation = parentTransforms->boneToObject.TransformPoint(boneVector);
	}
	
	bool inverted = transforms->objectToBone.Invert(transforms->boneToObject);
	assert(inverted);

	for (Bone* childBone : this->childBoneArray)
		childBone->UpdateCachedTransforms(transformType);
}

Vector3 Bone::CalcObjectSpaceCenter(BoneTransformType transformType) const
{
	const Transforms* transforms = this->GetTransforms(transformType);
	return transforms->boneToObject.TransformPoint(Vector3(-transforms->boneState.length / 2.0, 0.0, 0.0));
}

void Bone::PopulateBoneMap(BoneMap& boneMap) const
{
	boneMap.insert(std::pair<std::string, Bone*>(this->name, const_cast<Bone*>(this)));

	for (Bone* childBone : this->childBoneArray)
		childBone->PopulateBoneMap(boneMap);
}

void Bone::DebugDraw(BoneTransformType transformType, const Transform& objectToWorld) const
{
	DebugLines* debugLines = Game::Get()->GetDebugLines();
	if (!debugLines)
		return;

	const Transforms* transforms = this->GetTransforms(transformType);

	Vector3 origin = transforms->boneToObject.TransformPoint(Vector3(0.0, 0.0, 0.0));
	Vector3 xAxis = transforms->boneToObject.TransformNormal(Vector3(0.1, 0.0, 0.0));
	Vector3 yAxis = transforms->boneToObject.TransformNormal(Vector3(0.0, 0.1, 0.0));
	Vector3 zAxis = transforms->boneToObject.TransformNormal(Vector3(0.0, 0.0, 0.1));

	DebugLines::Line line;

	line.color.SetComponents(1.0, 0.0, 0.0);
	line.segment.point[0] = origin;
	line.segment.point[1] = origin + xAxis;
	line.segment = objectToWorld.TransformLineSegment(line.segment);
	debugLines->AddLine(line);

	line.color.SetComponents(0.0, 1.0, 0.0);
	line.segment.point[0] = origin;
	line.segment.point[1] = origin + yAxis;
	line.segment = objectToWorld.TransformLineSegment(line.segment);
	debugLines->AddLine(line);

	line.color.SetComponents(0.0, 0.0, 1.0);
	line.segment.point[0] = origin;
	line.segment.point[1] = origin + zAxis;
	line.segment = objectToWorld.TransformLineSegment(line.segment);
	debugLines->AddLine(line);

	const Transforms* parentTransforms = this->parentBone ? this->parentBone->GetTransforms(transformType) : nullptr;

	line.color.SetComponents(0.5, 0.5, 0.5);
	line.segment.point[0] = origin;
	line.segment.point[1] = parentTransforms ? parentTransforms->boneToObject.translation : Vector3(0.0, 0.0, 0.0);
	line.segment = objectToWorld.TransformLineSegment(line.segment);
	debugLines->AddLine(line);

	for (const Bone* childBone : this->childBoneArray)
		childBone->DebugDraw(transformType, objectToWorld);
}