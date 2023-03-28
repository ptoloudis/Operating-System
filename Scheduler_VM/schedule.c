/* schedule.c
 * This file contains the primary logic for the 
 * scheduler.
 */
#include "schedule.h"
#include "macros.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

#define NEWTASKSLICE (NS_TO_JIFFIES(100000000))
#define a 0.5

/* Local Globals
 * rq - This is a pointer to the runqueue that the scheduler uses.
 * current - A pointer to the current running task.
 */
struct runqueue *rq;
struct task_struct *current;

/* External Globals
 * jiffies - A discrete unit of time used for scheduling.
 *			 There are HZ jiffies in a second, (HZ is 
 *			 declared in macros.h), and is usually
 *			 1 or 10 milliseconds.
 */
extern long long jiffies;
extern struct task_struct *idle;

/*-----------------Initilization/Shutdown Code-------------------*/
/* This code is not used by the scheduler, but by the virtual machine
 * to setup and destroy the scheduler cleanly.
 */
 
 /* initscheduler
  * Sets up and allocates memory for the scheduler, as well
  * as sets initial values. This function should also
  * set the initial effective priority for the "seed" task 
  * and enqueu it in the scheduler.
  * INPUT:
  * newrq - A pointer to an allocated rq to assign to your
  *			local rq.
  * seedTask - A pointer to a task to seed the scheduler and start
  * the simulation.
  */
void initschedule(struct runqueue *newrq, struct task_struct *seedTask)
{
	seedTask->next = seedTask->prev = seedTask;
	newrq->head = seedTask;
	newrq->nr_running++;
}

/* killschedule
 * This function should free any memory that 
 * was allocated when setting up the runqueu.
 * It SHOULD NOT free the runqueue itself.
 */

// TODO: Free memory allocated for the runqueue
void killschedule()
{
	return;
}


void print_rq () {
	struct task_struct *curr;
	
	printf("Rq: \n");
	curr = rq->head;
	if (curr)
		printf("%p", curr);
	while(curr->next != rq->head) {
		curr = curr->next;
		printf(", %p", curr);
	};
	printf("\n");
}

/*-------------Scheduler Code Goes Below------------*/
/* This is the beginning of the actual scheduling logic */

/* schedule
 * Gets the next task in the queue
 */
void schedule() {
    static struct task_struct *nxt = NULL;
    struct task_struct *curr, *tmp;
    long last_duration = 0, minExp_BurstTime = 0;

	/* Using this init for the Goodness Algorithm */
	long minGoodness, maxWaitingTime = 0;

    /* Debugging statements */
    // printf("In schedule\n");
    // print_rq();

    /* Reset the rescheduling flag of the current task */
    current->need_reschedule = 0;

    /* ifthere is only one task in the runqueue, switch to it */
    if (rq->nr_running == 1) {
        context_switch(rq->head);
        nxt = rq->head->next;
    } else {
        /* Calculate the statistics for the current task */
        last_duration = sched_clock() - current->BurstTime;
        current->WaitingTime = sched_clock();
        current->Exp_BurstTime = (last_duration + a * current->Exp_BurstTime) / (1 + a);

		/**************** Algorithm with the Exp Burst Time **********************/
		// minExp_BurstTime = rq->head->next->Exp_BurstTime;
        // tmp = rq->head->next;
        // for (int i = 0; i < rq->nr_running; i++) {
        //     if(nxt == rq->head)
        //         nxt = nxt->next;
        //     curr = nxt;
        //     nxt = nxt->next;
        //     if(minExp_BurstTime > curr->Exp_BurstTime) {
        //         minExp_BurstTime = curr->Exp_BurstTime;
        //         tmp = curr;
        //     }
        // }

		/**************** Algorithm with the Goodness **********************/
        /* Calculate the minimum expected burst time and maximum waiting time */
        minExp_BurstTime = rq->head->next->Exp_BurstTime;
        for (int i = 0; i < rq->nr_running; i++) {
            if(nxt == rq->head)
                nxt = nxt->next;
            curr = nxt;
            nxt = nxt->next;
            if(minExp_BurstTime > curr->Exp_BurstTime)
                minExp_BurstTime = curr->Exp_BurstTime;
            if(maxWaitingTime < sched_clock() - curr->WaitingTime)
                maxWaitingTime = sched_clock() - curr->WaitingTime;
        }

        /* Calculate the goodness values for all tasks */
        minGoodness = ((1 + rq->head->next->Exp_BurstTime)/(minExp_BurstTime + 1)) * ((1 + maxWaitingTime)/(1 + sched_clock() - rq->head->next->WaitingTime));
        tmp = rq->head->next;
        for (int i = 0; i < rq->nr_running; i++) {
            if(nxt == rq->head)
                nxt = nxt->next;
            curr = nxt;
            nxt = nxt->next;
            tmp->Goodness = ((1 + curr->Exp_BurstTime)/(minExp_BurstTime + 1)) * ((1 + maxWaitingTime)/(1 + sched_clock() - curr->WaitingTime));
            if((tmp->Goodness <minGoodness) && (curr != rq->head)) {
                minGoodness = tmp->Goodness;
                tmp = curr;
            }
        }


        /* Set the process to be executed next */
		// This is working for the both algorithms
        if(current != tmp)
            tmp->BurstTime = sched_clock();

        if(tmp->next == rq->head)
            nxt = tmp->next->next;
        else
            nxt = tmp->next;

        /* Debugging statement */
        if(tmp == rq->head)
            printf("### Insert at the head ###\n");

        context_switch(tmp);

        /* Round Robin scheduling */
        /*
        curr = nxt;
        nxt = nxt->next;
        if(nxt == rq->head)
            nxt = nxt->next;
        context_switch(curr);
        */
    }
}


/* sched_fork
 * Sets up schedule info for a newly forked task
 */
void sched_fork(struct task_struct *p)
{
	p->time_slice = 100;
	p->BurstTime = 0;
	p->Exp_BurstTime = 0;
	p->Goodness = -1;
	p->WaitingTime = sched_clock();
	
}

/* scheduler_tick
 * Updates information and priority
 * for the task that is currently running.
 */
void scheduler_tick(struct task_struct *p)
{
	schedule();
}

/* wake_up_new_task
 * Prepares information for a task
 * that is waking up for the first time
 * (being created).
 */
void wake_up_new_task(struct task_struct *p)
{	
	p->next = rq->head->next;
	p->prev = rq->head;
	p->next->prev = p;
	p->prev->next = p;
	rq->nr_running++;
}

/* activate_task
 * Activates a task that is being woken-up
 * from sleeping.
 */
void activate_task(struct task_struct *p)
{
	p->next = rq->head->next;
	p->prev = rq->head;
	p->next->prev = p;
	p->prev->next = p;
	rq->nr_running++;
    p->WaitingTime = sched_clock(); /* Set the waiting time to the current time */
}

/* deactivate_task
 * Removes a running task from the scheduler to
 * put it to sleep.
 */
void deactivate_task(struct task_struct *p)
{
	p->prev->next = p->next;
	p->next->prev = p->prev;
	p->next = p->prev = NULL; /* Make sure to set them to NULL *
							   * next is checked in cpu.c      */
	p->WaitingTime = 0;
	rq->nr_running--;
}
