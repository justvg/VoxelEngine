#pragma once

typedef void work_callback(void *Data);

struct job_system_entry
{
	work_callback *Callback;
	void *Data;
};
struct job_system_queue
{
	uint32 volatile CompletionGoal;
	uint32 volatile CompletionCount;

	uint32 volatile NextEntryToWrite;
	uint32 volatile NextEntryToRead;
	HANDLE Semaphore;
	
	job_system_entry Entries[128];
};

internal void
AddEntryToJobSystem(job_system_queue *JobSystem, work_callback *Callback, void *Data)
{
	uint32 NewNextEntryToWrite = (JobSystem->NextEntryToWrite + 1) % ArrayCount(JobSystem->Entries);
	Assert(NewNextEntryToWrite != JobSystem->NextEntryToRead);

	job_system_entry *Entry = JobSystem->Entries + JobSystem->NextEntryToWrite;
	Entry->Callback = Callback;
	Entry->Data = Data;
	JobSystem->CompletionGoal++;
	
	_WriteBarrier();

	JobSystem->NextEntryToWrite = NewNextEntryToWrite;
	ReleaseSemaphore(JobSystem->Semaphore, 1, 0);
}

internal bool32
DoNextJobQueueEntry(job_system_queue *JobSystem)
{
	bool32 Sleep = false;

	uint32 OriginalNextEntryToRead = JobSystem->NextEntryToRead;
	uint32 NewNextEntryToRead = (OriginalNextEntryToRead + 1) % ArrayCount(JobSystem->Entries);
	if(JobSystem->NextEntryToRead != JobSystem->NextEntryToWrite)
	{
		uint32 Index = InterlockedCompareExchange((LONG volatile *)&JobSystem->NextEntryToRead, NewNextEntryToRead, OriginalNextEntryToRead);
		
		if(Index == OriginalNextEntryToRead)
		{
			job_system_entry *Entry = JobSystem->Entries + Index;
			Entry->Callback(Entry->Data);
			InterlockedIncrement((LONG volatile *)&JobSystem->CompletionCount);
		}
	}
	else
	{
		Sleep = true;
	}

	return(Sleep);
}

inline void
CompleteAllWork(job_system_queue *JobSystem)
{
	while(JobSystem->CompletionGoal != JobSystem->CompletionCount)
	{
		DoNextJobQueueEntry(JobSystem);
	}

	JobSystem->CompletionGoal = 0;
	JobSystem->CompletionCount = 0;
}

DWORD WINAPI
ThreadProc(LPVOID lpParameter)
{
	job_system_queue *Queue = (job_system_queue *)lpParameter;

	while(1)
	{
		if(DoNextJobQueueEntry(Queue))
		{
			WaitForSingleObjectEx(Queue->Semaphore, INFINITE, false);
		}
	}

	return(0);
}

internal void
InitJobSystem(job_system_queue *JobSystem, uint32 ThreadCount)
{
	JobSystem->CompletionGoal = 0;
	JobSystem->CompletionCount = 0;

	JobSystem->NextEntryToRead = 0;
	JobSystem->NextEntryToWrite = 0;

	uint32 InitialCount = 0;
	JobSystem->Semaphore = CreateSemaphoreEx(0, InitialCount, ThreadCount, 0, 0, SEMAPHORE_ALL_ACCESS);

	for (uint32 ThreadIndex = 0; ThreadIndex < ThreadCount; ThreadIndex++)
	{
		HANDLE ThreadHandle = CreateThread(0, 0, ThreadProc, JobSystem, 0, 0);
		CloseHandle(ThreadHandle);
	}
}