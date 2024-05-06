#include "CollisionCalculator.h"
#include "CollisionCache.h"
#include "Shape.h"
#include "Shapes/Sphere.h"
#include "Shapes/Capsule.h"
#include "Shapes/Box.h"
#include "Math/LineSegment.h"
#include "Math/Plane.h"
#include "Error.h"

using namespace Collision;

//------------------------------ CollisionCalculator ------------------------------

CollisionCalculator::CollisionCalculator()
{
}

/*virtual*/ CollisionCalculator::~CollisionCalculator()
{
}

//------------------------------ SphereSphereCollisionCalculator ------------------------------

SphereSphereCollisionCalculator::SphereSphereCollisionCalculator()
{
}

/*virtual*/ SphereSphereCollisionCalculator::~SphereSphereCollisionCalculator()
{
}

/*virtual*/ ShapePairCollisionStatus* SphereSphereCollisionCalculator::Calculate(const Shape* shapeA, const Shape* shapeB)
{
	auto sphereA = dynamic_cast<const SphereShape*>(shapeA);
	auto sphereB = dynamic_cast<const SphereShape*>(shapeB);

	if (!sphereA || !sphereB)
	{
		GetError()->AddErrorMessage("Failed to cast given shapes to spheres.");
		return nullptr;
	}

	auto collisionStatus = new ShapePairCollisionStatus(shapeA, shapeB);

	Vector3 centerA = sphereA->GetObjectToWorldTransform().TransformPoint(sphereA->GetCenter());
	Vector3 centerB = sphereB->GetObjectToWorldTransform().TransformPoint(sphereB->GetCenter());

	Vector3 centerDelta = centerB - centerA;
	double distance = centerDelta.Length();
	double radiiSum = sphereA->GetRadius() + sphereB->GetRadius();

	if (distance < radiiSum)
	{
		collisionStatus->inCollision = true;
		collisionStatus->collisionCenter = LineSegment(centerA, centerB).Lerp(sphereA->GetRadius() / radiiSum);	// TODO: Need to test this calculation.
		collisionStatus->separationDelta = centerDelta.Normalized() * (distance - radiiSum);
	}

	return collisionStatus;
}

//------------------------------ SphereCapsuleCollisionCalculator ------------------------------

SphereCapsuleCollisionCalculator::SphereCapsuleCollisionCalculator()
{
}

/*virtual*/ SphereCapsuleCollisionCalculator::~SphereCapsuleCollisionCalculator()
{
}

/*virtual*/ ShapePairCollisionStatus* SphereCapsuleCollisionCalculator::Calculate(const Shape* shapeA, const Shape* shapeB)
{
	auto sphere = dynamic_cast<const SphereShape*>(shapeA);
	auto capsule = dynamic_cast<const CapsuleShape*>(shapeB);
	double directionFactor = -1.0;

	if (!sphere || !capsule)
	{
		sphere = dynamic_cast<const SphereShape*>(shapeB);
		capsule = dynamic_cast<const CapsuleShape*>(shapeA);
		directionFactor = 1.0;
	}

	if (!sphere || !capsule)
	{
		GetError()->AddErrorMessage("Failed to cast given shapes to sphere and capsule.");
		return nullptr;
	}

	auto collisionStatus = new ShapePairCollisionStatus(shapeA, shapeB);

	LineSegment capsuleSpine(capsule->GetVertex(0), capsule->GetVertex(1));
	capsuleSpine = capsule->GetObjectToWorldTransform().TransformLineSegment(capsuleSpine);
	Vector3 sphereCenter = sphere->GetObjectToWorldTransform().TransformPoint(sphere->GetCenter());

	Vector3 closestPoint = capsuleSpine.ClosestPointTo(sphereCenter);
	Vector3 delta = sphereCenter - closestPoint;
	double distance = delta.Length();
	double radiiSum = sphere->GetRadius() + capsule->GetRadius();

	if (distance < radiiSum)
	{
		collisionStatus->inCollision = true;
		collisionStatus->collisionCenter = closestPoint + delta * (capsule->GetRadius() / radiiSum);	// TODO: Need to test this calculation.
		collisionStatus->separationDelta = delta.Normalized() * (distance - radiiSum) * directionFactor;
	}

	return collisionStatus;
}

//------------------------------ CapsuleCapsuleCollisionCalculator ------------------------------

CapsuleCapsuleCollisionCalculator::CapsuleCapsuleCollisionCalculator()
{
}

/*virtual*/ CapsuleCapsuleCollisionCalculator::~CapsuleCapsuleCollisionCalculator()
{
}

/*virtual*/ ShapePairCollisionStatus* CapsuleCapsuleCollisionCalculator::Calculate(const Shape* shapeA, const Shape* shapeB)
{
	auto collisionStatus = new ShapePairCollisionStatus(shapeA, shapeB);

	auto capsuleA = dynamic_cast<const CapsuleShape*>(shapeA);
	auto capsuleB = dynamic_cast<const CapsuleShape*>(shapeB);

	if (!capsuleA || !capsuleB)
	{
		GetError()->AddErrorMessage("Failed to cast given shapes to capsules.");
		return nullptr;
	}

	LineSegment spineA = capsuleA->GetObjectToWorldTransform().TransformLineSegment(capsuleA->GetSpine());
	LineSegment spineB = capsuleB->GetObjectToWorldTransform().TransformLineSegment(capsuleB->GetSpine());

	LineSegment shortestConnector;
	if (shortestConnector.SetAsShortestConnector(spineA, spineB))
	{
		double distance = shortestConnector.Length();
		double radiiSum = capsuleA->GetRadius() + capsuleB->GetRadius();

		if (distance < radiiSum)
		{
			collisionStatus->inCollision = true;
			collisionStatus->collisionCenter = Vector3(0.0, 0.0, 0.0);	// TODO: Figure this out.
			collisionStatus->separationDelta = shortestConnector.GetDelta().Normalized() * (distance - radiiSum);
		}
	}

	return collisionStatus;
}

//------------------------------ SphereBoxCollisionCalculator ------------------------------

SphereBoxCollisionCalculator::SphereBoxCollisionCalculator()
{
}

/*virtual*/ SphereBoxCollisionCalculator::~SphereBoxCollisionCalculator()
{
}

/*virtual*/ ShapePairCollisionStatus* SphereBoxCollisionCalculator::Calculate(const Shape* shapeA, const Shape* shapeB)
{
	auto sphere = dynamic_cast<const SphereShape*>(shapeA);
	auto box = dynamic_cast<const BoxShape*>(shapeB);
	double directionFactor = 1.0;

	if (!sphere || !box)
	{
		sphere = dynamic_cast<const SphereShape*>(shapeB);
		box = dynamic_cast<const BoxShape*>(shapeA);
		directionFactor = -1.0;
	}

	if (!sphere || !box)
	{
		GetError()->AddErrorMessage("Failed to cast given shapes to sphere and box.");
		return nullptr;
	}

	auto collisionStatus = new ShapePairCollisionStatus(shapeA, shapeB);

	Transform worldToBox = box->GetWorldToObjectTransform();
	Transform sphereToWorld = sphere->GetObjectToWorldTransform();
	Vector3 sphereCenter = worldToBox.TransformPoint(sphereToWorld.TransformPoint(sphere->GetCenter()));

	AxisAlignedBoundingBox objectSpaceBox;
	box->GetAxisAlignedBox(objectSpaceBox);

	Vector3 closestBoxPoint = objectSpaceBox.ClosestPointTo(sphereCenter);

	Vector3 delta = sphereCenter - closestBoxPoint;
	double distance = delta.Length();
	if (distance < sphere->GetRadius())
	{
		collisionStatus->inCollision = true;
		collisionStatus->collisionCenter = Vector3(0.0, 0.0, 0.0);	// TODO: Figure this out.

		double boxBorderThickness = 1e-4;
		if (distance < boxBorderThickness)
		{
			collisionStatus->separationDelta = closestBoxPoint.Normalized() * sphere->GetRadius() * directionFactor;
		}
		else if (objectSpaceBox.ContainsPoint(sphereCenter))
		{
			collisionStatus->separationDelta = -delta.Normalized() * (sphere->GetRadius() + distance) * directionFactor;
		}
		else
		{
			collisionStatus->separationDelta = delta.Normalized() * (sphere->GetRadius() - distance) * directionFactor;
		}

		collisionStatus->separationDelta = box->GetObjectToWorldTransform().TransformNormal(collisionStatus->separationDelta);
	}

	return collisionStatus;
}