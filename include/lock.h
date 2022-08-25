// NanoShell Lock shim - Useful when porting to the real deal!



#ifndef _LOCK_H
#define _LOCK_H

#include <main.h>

typedef struct
{
	bool  m_held;
	void* m_task_owning_it;
}
SafeLock;
void LockAcquireD (SafeLock *pLock, const char * pFile, int nLine);
void LockFreeD (SafeLock *pLock, const char * pFile, int nLine);

#define LockAcquire(pLock) LockAcquireD(pLock,__FILE__,__LINE__)
#define LockFree(pLock)    LockFreeD   (pLock,__FILE__,__LINE__)

#endif