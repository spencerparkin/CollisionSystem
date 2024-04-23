#include "Task.h"

using namespace Collision;

TaskID Task::nextTaskID = 0;

Task::Task()
{
	this->taskID = nextTaskID++;
}

/*virtual*/ Task::~Task()
{
}

/*static*/ void Task::Free(Task* task)
{
	delete task;
}