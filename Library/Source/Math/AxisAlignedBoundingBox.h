#pragma once

#include "Vector3.h"

namespace Collision
{
	/**
	 * An AABB for short, these boxes whose sides are parallel to the XY, YZ or XZ planes.
	 * Being axis-aligned, many operations can be performed between them more efficiently.
	 * A general box might be defined in terms of an AABB and rigid-body transform.
	 * As a point-set, we consider these to be closed in the sense that corner, edge and
	 * face points of the box are members of the set.  Note that all methods are left
	 * undefined if the stored corners of this AABB are invalid.
	 */
	class AxisAlignedBoundingBox
	{
	public:
		AxisAlignedBoundingBox();
		AxisAlignedBoundingBox(const AxisAlignedBoundingBox& aabb);
		virtual ~AxisAlignedBoundingBox();

		/**
		 * Set this AABB as a copy of the given AABB.
		 * 
		 * @param[in] aabb This is the AABB to copy.
		 */
		void operator=(const AxisAlignedBoundingBox& aabb);

		/**
		 * Tell the caller if this AABB is valid.  We also check for Inf and NaN here.
		 * 
		 * @return True is returned if the stored minimum corner is less-than (component-wise) the stored maximum corner.
		 */
		bool IsValid() const;

		/**
		 * Tell the caller if the given point is a member of this AABB's point-set.
		 * 
		 * @return True is returned if the given point is interior to, or on the edge of, this AABB.
		 */
		bool ContainsPoint(const Vector3& point) const;

		/**
		 * Set this AABB to be the intersection, if any, of the two given AABBs.
		 * This is a commutative operation.
		 * 
		 * @param[in] aabbA The first AABB taken in the intersection operation.
		 * @param[in] aabbB The second AABB taken in the intersection operation.
		 * @return True is returned on success; false, otherwise, and this AABB is left undefined.
		 */
		bool Intersect(const AxisAlignedBoundingBox& aabbA, const AxisAlignedBoundingBox& aabbB);

		/**
		 * Minimally expand this AABB so that it includes the given point.
		 * 
		 * @param[in] point This is the point to include in the box.
		 */
		void Expand(const Vector3& point);

		/**
		 * Cut this AABB exactly in half along a plane such that the resulting two
		 * halfs are as close to cubical as possible.  That is, the longest dimension
		 * of this AABB is determined, and then the code is made orthogonal to this
		 * dimension.
		 * 
		 * @param[out] aabbA This will hold the first half.
		 * @param[out] aabbB This will hold the second half.
		 */
		void Split(AxisAlignedBoundingBox& aabbA, AxisAlignedBoundingBox& aabbB) const;

		/**
		 * Calculate and return the dimensions (lengths) of the sides of this AABB.
		 * 
		 * @param[out] xSize This is the size of the box along the X dimension.
		 * @param[out] ySize This is the size of the box along the Y dimension.
		 * @param[out] zSize This is the size of the box along the Z dimension.
		 */
		void GetDimensions(double& xSize, double& ySize, double& zSize) const;

		Vector3 minCorner;
		Vector3 maxCorner;
	};
}