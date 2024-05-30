#include "Scene.h"
#include "Camera.h"

//--------------------------- Scene ---------------------------

Scene::Scene()
{
}

/*virtual*/ Scene::~Scene()
{
	this->Clear();
}

void Scene::Clear()
{
	this->renderObjectList.clear();
}

void Scene::AddRenderObject(Reference<RenderObject> renderObject)
{
	this->renderObjectList.push_back(renderObject);
}

void Scene::Render(Camera* camera, RenderPass renderPass)
{
	for (/*const*/ Reference<RenderObject>& renderObject : this->renderObjectList)
	{
		if (renderObject->IsHidden())
			continue;

		// TODO: Activate this code when ready.
		//if (!camera->IsApproximatelyVisible(renderObject.Get()))
		//	continue;
			
		renderObject->Render(camera, renderPass);
	}
}

//--------------------------- RenderObject ---------------------------

RenderObject::RenderObject()
{
	this->hide = false;
}

/*virtual*/ RenderObject::~RenderObject()
{
}