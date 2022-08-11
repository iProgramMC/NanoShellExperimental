#ifndef _TASK_H
#define _TASK_H

#include <Variant.h>
#define C_MAX_TASKS 1024
#define C_STACK_BYTES_PER_TASK 32768//plenty, can change later if needed.

/***********************************************************

	A Task is a called function which runs independently 
	of the rest of the kernel. When the task is started 
	(with StartTask), a little helper function gets 
	called, which calls the task's m_function.
	When the m_function returns, the helper function
	automatically kills the task 
	(KillTask can also be used to kill a task prematurely)

************************************************************/

typedef void (*TaskedFunction) (VariantList* pVList);

// Of course, this does not save memory, only register states . 
// The memory should stay the same anyway
typedef struct CPUSaveState
{
	int eax, ebx, ecx, edx, 
	    esi, edi, ebp, esp, eip, 
		cs, eflags;
} CPUSaveState;

typedef struct Task
{
	bool           m_exists;//true in the case of running tasks
	TaskedFunction m_function;
	//TODO: m_cpuState
	CPUSaveState   m_cpuState;
	
	void*          m_allocatedStack;
	bool           m_featuresArgs;
	VariantList    m_arguments;
	bool           m_firstTime;
	
	const char*    m_callerFile;//__FILE__
	const char*    m_callerFunc;//__FUNCTION__
	int            m_callerLine;//__LINE__
} 
Task;

extern Task g_runningTasks[C_MAX_TASKS];

enum {
	TASK_SUCCESS,
	TASK_ERROR_TOO_MANY_TASKS, 
	TASK_ERROR_STACK_ALLOC_FAILED,
	TASK_ERROR_END,
} ErrorCode;

/***********************************************************
    Allows you to spawn a new task. Returns an error code 
	through errorCodeOut. You can pass in arguments via
	pPassedVarlist.
    
	The function needs to have a "void (VariantList*)" header.
	pPassedVarlist may be null, if the TaskedFunction does
	not require any arguments.
    
	errorCodeOut returns TASK_SUCCESS if the Task returned
	is not NULL.
***********************************************************/
Task*StartTaskD(
	TaskedFunction function,     // has to have a "void (VariantList*)" header
	VariantList* pPassedVarlist, // can also be NULL, if we don't require one
	int* errorCodeOut,           // error code, must be initialized with a value.
	const char*a, const char*b, int c
);
#define StartTask(function, varlist, errorPtr) \
		StartTaskD(function, varlist, errorPtr, __FILE__, __FUNCTION__, __LINE__)
/***********************************************************
    Allows you to kill the task at one point in time.
    NOTE: KillTask(KeGetRunningTask()) is UB, as the task function
    will still run until the next task switch interrupt is called.
	
	Returns TRUE if task was killed successfully.
***********************************************************/
bool KillTask(Task* pTask);
/***********************************************************
    Gets the currently running task.
	Returns NULL if this is the kernel task.
***********************************************************/
Task* GetRunningTask();

// Internal kernel function.
void InitializeTaskSystem();

// Allow switching to the next Task, or the kernel task


#endif//_TASK_H