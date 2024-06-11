#pragma once

#include "Reference.h"
#include "Math/AxisAlignedBoundingBox.h"
#include <list>

namespace Imzadi
{
	class RenderObject;
	class Camera;

	enum RenderPass
	{
		MAIN_PASS,
		SHADOW_PASS
	};

	/**
	 * This class represents the entire renderable scene and how we're viewing it.
	 * It is a collection of RenderObject instances and can be asked to update and draw each frame.
	 */
	class IMZADI_API Scene : public ReferenceCounted
	{
	public:
		Scene();
		virtual ~Scene();

		/**
		 * Remove all RenderObject instances being tracked by this scene.  Once cleared,
		 * nothing will render when the scene is rendered.
		 */
		void Clear();

		/**
		 * Add a RenderObject instance to the scene.  It will get drawn if it intersects the view frustum.
		 */
		void AddRenderObject(Reference<RenderObject> renderObject);

		/**
		 * Submit draw-calls for everything approximately deemed visible in the scene
		 * to the given camera.
		 *
		 * @param[in] camera This is the camera to use to render the scene.
		 * @param[in] renderPass Indicate the purpose of this render.
		 */
		void Render(Camera* camera, RenderPass renderPass);

	private:
		typedef std::list<Reference<RenderObject>> RenderObjectList;

		// Note that a more sophisticated system would spacially sort scene objects
		// or put them in some sort of hierarchy or something like that.  I'm just
		// going to do something super simple here and just have a list of render
		// objects.  That's it.  I'll cull them based on the view frustum, but
		// that's as far as I'm going to take visible surface determination in
		// this simple application.
		RenderObjectList* renderObjectList;
	};

	/**
	 * This is the base class for anything that can get rendered in the scene.
	 */
	class IMZADI_API RenderObject : public ReferenceCounted
	{
	public:
		RenderObject();
		virtual ~RenderObject();

		virtual void Render(Camera* camera, RenderPass renderPass) = 0;

		virtual void GetWorldBoundingSphere(Imzadi::Vector3& center, double& radius) const = 0;

		virtual int SortKey() const;

		bool IsHidden() const { return this->hide; }
		void SetHidden(bool hide) { this->hide = hide; }

	private:
		bool hide;
	};
}