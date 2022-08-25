// NanoShell Lock shim - Useful when porting to the real deal!

#include <lock.h>

void LockAcquireD (SafeLock *pLock, const char * pFile, int nLine) // An attempt at a safe lock
{
	SLogMsg("LockAcquireD ( %p, '%s', %d )" , pLock, pFile, nLine);
	
	//because we have no real multitasking, use this to debug stuff
	//This is basic enough, if there's any deadlock or anything this should be more than informative!
	if (pLock->m_held)
	{
		LogMsg("ERROR: Lock %p already held!  (%s:%d)", pLock, pFile, nLine);
		KeStopSystem();
	}
	
	pLock->m_held = true;
}

void LockFreeD (SafeLock *pLock, const char * pFile, int nLine)
{
	SLogMsg("LockFreeD ( %p, '%s', %d )" , pLock, pFile, nLine);
	
	if (!pLock->m_held)
	{
		LogMsg("ERROR: Lock %p not held!  (%s:%d)", pLock, pFile, nLine);
		KeStopSystem();
	}
	
	pLock->m_held = false;
}
