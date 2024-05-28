#pragma once

#include "Entity.h"
#include "Camera.h"

class FreeCam;

/**
 * This class gives us our 3rd-person view of the player's character.
 */
class FollowCam : public Entity
{
public:
	FollowCam();
	virtual ~FollowCam();

	virtual bool Setup() override;
	virtual bool Shutdown(bool gameShuttingDown) override;
	virtual bool Tick(double deltaTime) override;

	void SetSubject(Entity* entity) { this->subject.SafeSet(entity); }
	void SetCamera(Camera* camera) { this->camera.SafeSet(camera); }

	struct FollowParams
	{
		double followingDistance;
		double hoverHeight;
		double rotationRate;
		Collision::Vector3 objectSpaceFocalPoint;
	};

	const FollowParams& GetFollowParams() const { return this->followParams; }
	void SetFollowParams(const FollowParams& followParams) { this->followParams = followParams; }

private:
	Reference<Entity> subject;
	Reference<Camera> camera;
	Reference<FreeCam> freeCam;
	FollowParams followParams;
};