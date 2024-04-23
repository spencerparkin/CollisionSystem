#include "Thread.h"
#include "Error.h"
#include "Result.h"
#include "Command.h"
#include "Query.h"
#include "Shape.h"
#include <format>

using namespace Collision;

Thread::Thread(const AxisAlignedBoundingBox& collisionWorldExtents)
{
	this->collisionWorldExtents = collisionWorldExtents;
	this->thread = nullptr;
	this->signaledToExit = false;
	this->taskQueue = new std::list<Task*>();
	this->taskQueueMutex = new std::mutex();
	this->taskQueueSemaphore = new std::counting_semaphore<4096>(0);
	this->resultMap = new std::unordered_map<TaskID, Result*>();
	this->resultMapMutex = new std::mutex();
	this->shapeMap = new std::unordered_map<ShapeID, Shape*>();
}

/*virtual*/ Thread::~Thread()
{
	delete this->taskQueue;
	delete this->taskQueueMutex;
	delete this->taskQueueSemaphore;
	delete this->resultMap;
	delete this->resultMapMutex;
	delete this->shapeMap;
}

bool Thread::Startup()
{
	if (this->thread)
	{
		GetError()->AddErrorMessage("Thread already created!");
		return false;
	}

	this->signaledToExit = false;
	this->thread = new std::thread(&Thread::EntryFunc, this);

	return true;
}

bool Thread::Shutdown()
{
	if (this->thread)
	{
		this->SendTask(ExitThreadCommand::Create());
		this->thread->join();
		delete this->thread;
		this->thread = nullptr;
	}

	return true;
}

/*static*/ void Thread::EntryFunc(Thread* thread)
{
	thread->Run();
}

void Thread::Run()
{
	while (!this->signaledToExit)
	{
		// Don't eat up any CPU resources if there are no tasks queued.
		// The semaphore count mirrors the size of the task queue.
		this->taskQueueSemaphore->acquire();

		// Grab a task off the queue.  Make the mutex scope-lock as tight as possible.
		Task* task = nullptr;
		{
			std::lock_guard<std::mutex> guard(*this->taskQueueMutex);
			std::list<Task*>::iterator iter = this->taskQueue->begin();
			task = *iter;
			this->taskQueue->erase(iter);
		}

		if (task)
		{
			task->Execute(this);
			Task::Free(task);
		}
	}

	this->ClearTasks();
	this->ClearResults();
	this->ClearShapes();
}

void Thread::ClearTasks()
{
	std::lock_guard<std::mutex> guard(*this->taskQueueMutex);
	while (this->taskQueue->size() > 0)
	{
		std::list<Task*>::iterator iter = this->taskQueue->begin();
		Task* task = *iter;
		Task::Free(task);
		this->taskQueue->erase(iter);
	}
}

void Thread::ClearResults()
{
	std::lock_guard<std::mutex> guard(*this->resultMapMutex);
	while (this->resultMap->size() > 0)
	{
		std::unordered_map<TaskID, Result*>::iterator iter = this->resultMap->begin();
		Result* result = iter->second;
		Result::Free(result);
		this->resultMap->erase(iter);
	}
}

void Thread::ClearShapes()
{
	// TODO: Blow away spacial-partitioning tree too.

	while (this->shapeMap->size() > 0)
	{
		std::unordered_map<ShapeID, Shape*>::iterator iter = this->shapeMap->begin();
		Shape* shape = iter->second;
		Shape::Free(shape);
		this->shapeMap->erase(iter);
	}
}

void Thread::AddShape(Shape* shape)
{
	std::unordered_map<ShapeID, Shape*>::iterator iter = this->shapeMap->find(shape->GetShapeID());
	if (iter == this->shapeMap->end())
	{
		this->shapeMap->insert(std::pair<ShapeID, Shape*>(shape->GetShapeID(), shape));
		// TODO: Account for spacial-partitionining tree insertion/removal.
	}
	else
	{
		GetError()->AddErrorMessage(std::format("Cannot add shape.  A shape with ID {} already exists in the system.", shape->GetShapeID()));
		Shape::Free(shape);
	}
}

void Thread::RemoveShape(ShapeID shapeID)
{
	std::unordered_map<ShapeID, Shape*>::iterator iter = this->shapeMap->find(shapeID);
	if (iter != this->shapeMap->end())
	{
		Shape* shape = iter->second;
		// TODO: Acount for spacial-partitioning tree insertion/removal.
		Shape::Free(shape);
		this->shapeMap->erase(iter);
	}
	else
	{
		GetError()->AddErrorMessage(std::format("Cannot remove shape.  No shape with ID {} was found in the system.", shapeID));
	}
}

TaskID Thread::SendTask(Task* task)
{
	TaskID taskId = task->GetTaskID();

	// Add the task to the queue.  Make the mutex scope-lock as tight as possible.
	{
		std::lock_guard<std::mutex> guard(*this->taskQueueMutex);
		this->taskQueue->push_back(task);
	}

	// Signal the collision thread that a task is available.
	this->taskQueueSemaphore->release();

	return taskId;
}

Result* Thread::ReceiveResult(TaskID taskID)
{
	Result* result = nullptr;

	// Look-up the result, if it's ready.  Make the mutex scope-lock as tight as possible.
	{
		std::lock_guard<std::mutex> guard(*this->resultMapMutex);
		std::unordered_map<TaskID, Result*>::iterator iter = this->resultMap->find(taskID);
		if (iter != this->resultMap->end())
		{
			result = iter->second;
			this->resultMap->erase(iter);
		}
	}

	return result;
}

void Thread::StoreResult(Result* result, TaskID taskID)
{
	std::lock_guard<std::mutex> guard(*this->resultMapMutex);
	std::unordered_map<TaskID, Result*>::iterator iter = this->resultMap->find(taskID);
	if (iter == this->resultMap->end())
		this->resultMap->insert(std::pair<TaskID, Result*>(taskID, result));
	else
	{
		Result::Free(iter->second);
		iter->second = result;
	}
}