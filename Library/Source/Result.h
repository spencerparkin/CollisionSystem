#pragma once

#include "Defines.h"
#include "Shape.h"
#include "Math/LineSegment.h"
#include <vector>

namespace Collision
{
	class AxisAlignedBoundingBox;

	/**
	 * Derivatives of this class are collision query results; or, in other words,
	 * responses to collision queries.  There is not necessarily a one-to-one correspondance
	 * between derivatives of this class and that of the Query class, but there typically
	 * is a Result class derivative per Query class derivative.  When a query is made and
	 * a result given, the user will know, based on the query that was made, the derivative
	 * of the Result class to which the Result class pointer can be safely cast.  Each
	 * Query class derivative should document the Result class derivative that it uses.
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
			LineSegment line;	//< The user should render this line's geometry, given here in world space coordinates.
			Vector3 color;		//< The user should render the line with this color to differentiate it from other elements of the collision world.
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
			ShapeID shapeID;			//< This is the ID of the collision shape that was hit by the ray, if any.  It is zero if not hit occured.
			Vector3 surfacePoint;		//< This is the point on the surface of the shape where the ray hit it.
			Vector3 surfaceNormal;		//< This is the normal to the surface of the shape where it was hit.
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
}