#pragma once

#include "Defines.h"
#include "Math/AxisAlignedBoundingBox.h"
#include "Shape.h"
#include "Result.h"
#include "CollisionCache.h"
#include <vector>
#include <unordered_map>

namespace Collision
{
	class BoundingBoxNode;

	/**
	 * This class facilitates the broad-phase of collision detection.
	 * Note that it is not a user-facing class and so the collision system
	 * user will never have to interface with it directly.  This class,
	 * when appropriate, calls directly into the narrow-phase.
	 */
	class COLLISION_LIB_API BoundingBoxTree
	{
	public:
		BoundingBoxTree(const AxisAlignedBoundingBox& collisionWorldExtents);
		virtual ~BoundingBoxTree();

		/**
		 * Insert the given shape into this bounding-box tree.  Note that
		 * it is fine for the shape to already be in the tree; in which case,
		 * the position of the shape in the tree is adjusted.  The ideal
		 * location of a shape in our tree is for it to be as deep into the
		 * tree as it can possibly fit.  Also note that it is up to the
		 * caller to know when to re-insert a shape when its bounding box
		 * changes.  This class is non-the-wiser about changes made to
		 * shapes outside of its scope that would effect their bounding boxes,
		 * and therefore, their ideal positioning within this tree.  If a shape
		 * is changed without re-insertion, then the results of algorithms
		 * that operate on this tree are left undefined.
		 * 
		 * @param[in] shape This is the shape to insert into this tree.  It must not be a member of some other tree.  I can already be a member of this tree.
		 * @param[in] shapeSplittingAllowed For shapes that are typically meant to be static, this flag can be set to true, and then our insertion algorithm will try to split the shape during insertion to get it deeper into the tree.
		 * @return True is returned on success; false, otherwise.
		 */
		bool Insert(Shape* shape, bool shapeSplittingAllowed = false);

		/**
		 * Remove the given shape from this bounding-box tree.
		 * 
		 * @param[in] shape This is the shape to remove from this tree.  It must already be a member of this tree.
		 */
		bool Remove(Shape* shape);

		/**
		 * Remove all shapes from this tree and delete all nodes of the tree.
		 */
		void Clear();

		/**
		 * Provide a visualization of the tree for debugging purposes.
		 */
		void DebugRender(DebugRenderResult* renderResult) const;

		/**
		 * Perform a ray-cast against all collision shapes within the tree.
		 * 
		 * @param[in] ray This is the ray with which to perform the cast.
		 * @param[out] rayCastResult The hit result, if any, is put into the given RayCastResult instance.  If no hit, then the result will indicate as much.
		 */
		void RayCast(const Ray& ray, RayCastResult* rayCastResult) const;

		/**
		 * Determine the collision status of the given shape.
		 * 
		 * @param[in] shape This is the shape in question.
		 * @param[out] collisionResult The collision status is returned in this instance of the CollisionQueryResult class.
		 * @return True is returned on success; false, otherwise.
		 */
		bool CalculateCollision(const Shape* shape, CollisionQueryResult* collisionResult) const;

	private:
		BoundingBoxNode* rootNode;
		AxisAlignedBoundingBox collisionWorldExtents;
		mutable CollisionCache collisionCache;
	};

	/**
	 * Instances of this class form the nodes of the BoundingBoxTree class.
	 * This class is designed to have an arbitrary branching factor, but a
	 * binary tree is best, because it minimizes the chance of a shape's box
	 * having to straddle the boundary between sub-spaces which, admittedly,
	 * is a problem I'm not yet sure how to easily solve.
	 */
	class COLLISION_LIB_API BoundingBoxNode
	{
		friend class BoundingBoxTree;

	private:
		BoundingBoxNode(BoundingBoxNode* parentNode, BoundingBoxTree* tree);
		virtual ~BoundingBoxNode();

		/**
		 * Point the given shape to this node, and this node to the given shape.
		 */
		void BindToShape(Shape* shape);

		/**
		 * Unlink the pointers between this node and the given shape in both directions.
		 */
		void UnbindFromShape(Shape* shape);

		/**
		 * If this node has no children, create two children partitioning this node's
		 * space into two ideal-sized sub-spaces.
		 */
		void SplitIfNeeded(BoundingBoxTree* tree);

		/**
		 * Render this node's space as a simple wire-frame box.
		 */
		void DebugRender(DebugRenderResult* renderResult) const;

		/**
		 * Descend the tree, performing a ray-cast as we go.
		 * 
		 * @param[in] ray This is the ray with which to perform the ray-cast.
		 * @param[out] hitData This will contain info about what shape was hit and how, if any.
		 * @return True is returned if and only if a hit ocurred in this node of the tree.
		 */
		bool RayCast(const Ray& ray, RayCastResult::HitData& hitData) const;

	private:
		BoundingBoxTree* tree;								///< This is the tree of which this node is a part.
		AxisAlignedBoundingBox box;							///< This is the space represented by this node.
		std::vector<BoundingBoxNode*>* childNodeArray;		///< These are the sub-space partitions of this node.
		BoundingBoxNode* parentNode;						///< This is a pointer to the parent space containing this node.
		std::unordered_map<ShapeID, Shape*>* shapeMap;		///< These are shapes in this node's space that cannot fit in a sub-space.
	};
}