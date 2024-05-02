#pragma once

#include "Defines.h"
#include "Shape.h"
#include "Math/LineSegment.h"
#include <vector>
#include <string>

namespace Collision
{
	class AxisAlignedBoundingBox;

	/**
	 * Derivatives of this class are collision query results; or, in other words,
	 * responses to collision queries.  There is not necessarily a one-to-one correspondance
	 * between derivatives of this class and that of the Query class, but there typically
	 * is a Result class derivative per Query class derivative.  When a query is made and
	 * a result given, the user will know, based on the query that was made, what derivatives
	 * of the Result class to which the query result pointer can be possibly cast.  Each
	 * Query class derivative should document the Result class derivatives that it uses.
	 * Users of the system should perform a dynamic cast on the returned Result class pointer.
	 * Any query can possibly return an instance of the ErrorResult class.
	 */
	class COLLISION_LIB_API Result
	{
	public:
		Result();
		virtual ~Result();

		/**
		 * Use this function to free any given collision result once you've
		 * had a chance to process it on the main thread.
		 * 
		 * @param result This is the result who's memory is to be reclaimed.
		 */
		static void Free(Result* result);
	};

	/**
	 * This result can be returned by any query, and indicates that something went wrong
	 * with the query.
	 */
	class COLLISION_LIB_API ErrorResult : public Result
	{
	public:
		ErrorResult();
		virtual ~ErrorResult();

		/**
		 * Return the error message associated with this result.  Hopefully this is
		 * a message that is useful enough to help trouble-shoot what went wrong with
		 * the query.
		 */
		const std::string& GetErrorMessage() const { return *this->errorMessage; }

		/**
		 * This is used internally to set the error message associated with the faulty query.
		 */
		void SetErrorMessage(const std::string& errorMessage) { *this->errorMessage = errorMessage; }

		/**
		 * Allocate and return a new ErrorResult instance.
		 */
		static ErrorResult* Create();

	private:
		std::string* errorMessage;
	};

	/**
	 * A result of a DebugRenderQuery class query, this class contains all of
	 * wire-frame drawing information the user can use to visualize (render)
	 * the collision system as far as what was requested in the query.
	 */
	class COLLISION_LIB_API DebugRenderResult : public Result
	{
	public:
		DebugRenderResult();
		virtual ~DebugRenderResult();

		/**
		 * An instance of this structure are used to generate wire-frame
		 * representations of shapes and other things in the collision system.
		 */
		struct RenderLine
		{
			LineSegment line;	///< The user should render this line's geometry, given here in world space coordinates.
			Vector3 color;		///< The user should render the line with this color to differentiate it from other elements of the collision world.
		};

		/**
		 * Return the list of RenderLine structure instances that the
		 * caller can use to visualize the state of the collision system.
		 * Of course, no drawing code is supported by the collision system.
		 * It's up to the caller to render the data.
		 */
		const std::vector<RenderLine>& GetRenderLineArray() const { return *this->renderLineArray; }

		/**
		 * Add a line to be rendered to the debug render result.
		 * This is typically just used internally by the collision system.
		 */
		void AddRenderLine(const RenderLine& renderLine);

		/**
		 * Add 12 lines for the given AABB.
		 * 
		 * @param[in] box This is the box to render as wire-frame.
		 * @param[in] color This is the color to use for the box.
		 */
		void AddLinesForBox(const AxisAlignedBoundingBox& box, const Vector3& color);

		/**
		 * Allocate and return a new DebugRenderResult object.
		 */
		static DebugRenderResult* Create();

	private:
		std::vector<RenderLine>* renderLineArray;
	};

	/**
	 * An instance of this class is returned as the result of a ray-cast query
	 * using the RayCastQuery class.
	 */
	class RayCastResult : public Result
	{
	public:
		RayCastResult();
		virtual ~RayCastResult();

		/**
		 * This structure organizes the characteristics of a ray-cast hit against a shape in the collision world.
		 */
		struct HitData
		{
			ShapeID shapeID;			///< This is the ID of the collision shape that was hit by the ray, if any.  It is zero if no hit occured.
			Vector3 surfacePoint;		///< This is the point on the surface of the shape where the ray hit it.
			Vector3 surfaceNormal;		///< This is the normal to the surface of the shape where it was hit.
			double alpha;				///< Mainly used for internal purposes, this is the distance from ray origin along the ray to the hit point.
		};

		/**
		 * Get the particular of the ray-cast result in the returned structure.
		 * If the returned hit-data has zero for the shape ID, then the ray did
		 * not hit any shape in the collision world.
		 */
		const HitData& GetHitData() const { return this->hitData; }

		/**
		 * This is used internally to set the hit-data on the ray-cast result object.
		 */
		void SetHitData(const HitData& hitData) { this->hitData = hitData; }

		/**
		 * Allocate and return a new RayCastResult instance.
		 */
		static RayCastResult* Create();

	private:
		HitData hitData;
	};

	/**
	 * This is the result of any query expected to return a transform.
	 */
	class COLLISION_LIB_API TransformResult : public Result
	{
	public:
		TransformResult();
		virtual ~TransformResult();

		static TransformResult* Create();

	public:
		Transform transform;		///< This is the returned transform.
	};

	class ShapePairCollisionStatus;

	/**
	 * Instances of this class are results of collision queries, and simply consist
	 * of a set of collision pairs.  See the ShapePairCollisionStatus class for
	 * more information.
	 * 
	 * Important: The results presented here are not thread-safe!  You will
	 * be given raw pointers to collision shape instances with read-only access,
	 * but these are only valid if no queries or commands are pending or in flight.
	 * The best practice is for the application to make all necessary queries,
	 * go do other important work, then (only when the results of the queries
	 * are absolutely necessary before the application can continue) the collision
	 * system should be stalled (or flushed, if you will), at which point, you
	 * can safely examine all query results, and then act upon them (e.g., resolve
	 * constraints, generate impulses, etc.)  All this would typically happen in
	 * a single frame.
	 */
	class COLLISION_LIB_API CollisionQueryResult : public Result
	{
	public:
		CollisionQueryResult();
		virtual ~CollisionQueryResult();

		/**
		 * Allocate and return a new instance of the CollisionQueryResult class.
		 */
		static CollisionQueryResult* Create();

		/**
		 * This is used internally to populate the query result.
		 */
		void AddCollisionStatus(ShapePairCollisionStatus* collisionStatus);

		/**
		 * Get this result's set of ShapePairCollisionStatus class instances.
		 * Each pair will be a valid collision pair between two shapes, one of
		 * which is the shape specified in the original collision query.
		 */
		const std::vector<ShapePairCollisionStatus*>& GetCollisionStatusArray() const { return *this->collisionStatusArray; }

		/**
		 * This is used internally to fulfill the query result.
		 */
		void SetShape(const Shape* shape) { this->shape = shape; }

		/**
		 * Get read-only access to the collision shape of the collision query.
		 * Be weary of thread-safety.  Note that you shuld never try to modify
		 * a shape directly.  Rather, you must do it indirectly via commands issued
		 * to the collision system, because any change to a shape may cause a number
		 * of spacial-sorting considerations to change, and cause cache invalidations
		 * to occur.
		 */
		const Shape* GetShape() const { return this->shape; }

	private:
		std::vector<ShapePairCollisionStatus*>* collisionStatusArray;	///< This is the set of collisions involving the collision query's shape.
		const Shape* shape;		///< For convenience, this holds the shape in question that was the subject of the collision query.
	};
}