// CPU Scheduling Simulator (CISC 361 Project 2) by Joe Campanelli

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

FILE * schedFile;
char str[100] = {0x0};
const char spc[2] = " ";
char *s = NULL;
char* args[1000];
char *token;
char *schedAlg;
int schedAlgID;
char *end;

int temp1[10000];

void fcfsnp();
void fcfsp();
void sjnp();

int contextSwitch;
int quantum;
int numProcesses;
int counter = 0;
int clockTime = -1;
int endTime;
int totalTimeProcessesonCPU = 0;

float cpuUtilization;
int averageTurnAroundTime;
int maxTurnAroundTime = 0;
int averageWaitTime;
int maxWaitTime = 0;
int averageResponseTime;
int maxResponseTime = 0;

int p = 0;
int i = 0;
int j = 0;
int argLength = 0;

typedef struct process process;
typedef struct node node;

void myInsert(struct process process);
void applyContextSwitchtoWaitingProcesses();
int readyJobsEmpty();

struct process {
    int pid;
    int arrivalTime;
    int numCPUBursts;
    int currBurst;
    int totalCPUBurstTime;
    int numTotal;
    int firstRunTime;
    int hasBeenRun;
    int completionTime;
    int turnAroundTime;
    int responseTime;
    int waitTime;
    int CPUBurstsandWaitTimes[10000];
};

struct process allProcesses[10000];
struct process completedProcesses[10000];
struct process waitingProcesses[10000];

struct process readyJobs[10000];

struct process emptyProcess;
struct process temp0;

void myInsert(struct process process) {
    int f = 0;
    int e;
    temp0.pid = process.pid;
    temp0.currBurst = process.currBurst;
    temp0.numCPUBursts = process.numCPUBursts;
    temp0.arrivalTime = process.arrivalTime;
    temp0.totalCPUBurstTime = process.totalCPUBurstTime;
    for(f = 0; f < (process.numCPUBursts*2); f++) {
        e = process.CPUBurstsandWaitTimes[f];
        temp0.CPUBurstsandWaitTimes[f] = e;
    }
    allProcesses[process.pid] = temp0;
}

// Queue implementation found at: http://www.sanfoundry.com/c-program-queue-using-linked-list/

int count = 0;

process frontelement();
void enq(struct process process);
void deq();
int empty();
void create();
void queuesize();
int isNull(node* node);

struct node {
    struct process process;
    struct node *ptr;
}*front,*rear,*temp,*front1;

/* Create an empty queue */
void create()
{
    front = rear = NULL;
}

/* Returns queue size */
void queuesize()
{
    printf("\n Queue size : %d", count);
}

/* Enqueing the queue */
void enq(struct process process)
{
    if (rear == NULL)
    {
        rear = (struct node *)malloc(1*sizeof(struct node));
        rear->ptr = NULL;
        rear->process = process;
        front = rear;
    }
    else
    {
        temp=(struct node *)malloc(1*sizeof(struct node));
        rear->ptr = temp;
        temp->process = process;
        temp->ptr = NULL;

        rear = temp;
    }
    count++;
}

/* Dequeing the queue */
void deq()
{
    front1 = front;

    if (front1 == NULL)
    {
        printf("\n Error: Trying to display elements from empty queue");
        return;
    }
    else
        if (front1->ptr != NULL)
        {
            front1 = front1->ptr;
            free(front);
            front = front1;
        }
        else
        {
            free(front);
            front = NULL;
            rear = NULL;
        }
        count--;
}

/* Returns the front element of queue */
process frontelement()
{
    if ((front != NULL) && (rear != NULL))
        return(front->process);
}

/* Display if queue is empty or not */
int empty()
{
    if ((front == NULL) && (rear == NULL)) return 1;
    else return 0;
}

int isNull(node* node) {
    if(node == NULL) return 1;
    else return 0;
}

void fcfsnp() {
    emptyProcess.pid = -1;
    for(i = 0; i < 10000; i++) {
        waitingProcesses[i] = emptyProcess;
    }
    int running = 0;
    int numWaiting = 0;
    int numCompleted = 0;
    process currRunning;
    create();
    p=-1;
    while(numCompleted != numProcesses) {
        p++;
        clockTime++;
        printf("Time %d", clockTime);
        printf("\n");
        // Put processes in the ready queue
        while((allProcesses[counter].arrivalTime <= clockTime) && (counter <= (numProcesses-1))) {
            printf("Placing process %d", allProcesses[counter].pid);
            printf(" on the ready queue.\n");
            enq(allProcesses[counter]);
            counter++;
        }
        // Place a process on the CPU
        if(running == 0 && empty() != 1) {
            currRunning = frontelement();
            if(currRunning.hasBeenRun == 1) {
                clockTime = clockTime + contextSwitch;
                applyContextSwitchtoWaitingProcesses();
            }
            printf("Placing process %d", currRunning.pid);
            printf(" on the CPU.\n");
            if(currRunning.hasBeenRun == 0) {
                currRunning.firstRunTime = clockTime;
                currRunning.hasBeenRun = 1;
            }
            running = 1;
            deq();
        }
        // Deal with a process running on the CPU
        else if(running == 1) {
            currRunning.CPUBurstsandWaitTimes[currRunning.currBurst]--;
            totalTimeProcessesonCPU++;
            if((currRunning.CPUBurstsandWaitTimes[currRunning.currBurst] <= 0)
            && (currRunning.currBurst != (currRunning.numCPUBursts*2 - 2))) {
                printf("Process %d", currRunning.pid);
                printf(" moved to wait state.\n");
                clockTime = clockTime + contextSwitch;
                applyContextSwitchtoWaitingProcesses();
                currRunning.currBurst++;
                waitingProcesses[currRunning.pid] = currRunning;
                running = 0;
            }
            else if((currRunning.CPUBurstsandWaitTimes[currRunning.currBurst] <= 0)
                 && (currRunning.currBurst >= (currRunning.numCPUBursts*2 - 2))) {
                printf("Process %d", currRunning.pid);
                printf(" has completed.\n");
                currRunning.completionTime = clockTime;
                completedProcesses[numCompleted] = currRunning;
                numCompleted++;
                running = 0;
            }
            else {
                printf("Process %d", currRunning.pid);
                printf(" still running on the CPU.\n");
            }
        }
        // Update waiting processes, put back on ready queue if done waiting
        for(i = 0; i < 10000; i++) {
            if(waitingProcesses[i].pid != -1) {
                waitingProcesses[i].CPUBurstsandWaitTimes[waitingProcesses[i].currBurst]--;
                if(waitingProcesses[i].CPUBurstsandWaitTimes[waitingProcesses[i].currBurst] <= 0) {
                    printf("Process %d", waitingProcesses[i].pid);
                    printf(" is done waiting, moving to ready queue.\n");
                    waitingProcesses[i].currBurst++;
                    enq(waitingProcesses[i]);
                    waitingProcesses[i] = emptyProcess;
                }
                else {
                    printf("Process %d", waitingProcesses[i].pid);
                    printf(" is waiting.\n");
                }
            }
        }
        printf("\n");
    }
    endTime = clockTime;
    return;
}

void fcfsp() {
    emptyProcess.pid = -1;
    for(i = 0; i < 10000; i++) {
        waitingProcesses[i] = emptyProcess;
    }
    int running = 0;
    int numWaiting = 0;
    int numCompleted = 0;
    int quantumCounter = 0;
    process currRunning;
    create();
    p=-1;
    while(numCompleted != numProcesses) {
        p++;
        clockTime++;
        printf("Time %d", clockTime);
        printf("\n");
        // Put processes in the ready queue
        while((allProcesses[counter].arrivalTime <= clockTime) && (counter <= (numProcesses-1))) {
            printf("Placing process %d", allProcesses[counter].pid);
            printf(" on the ready queue.\n");
            enq(allProcesses[counter]);
            counter++;
        }
        // Place a process on the CPU
        if(running == 0 && empty() != 1) {
            currRunning = frontelement();
            if(currRunning.hasBeenRun == 1) {
                clockTime = clockTime + contextSwitch;
                applyContextSwitchtoWaitingProcesses();
            }
            printf("Placing process %d", currRunning.pid);
            printf(" on the CPU.\n");
            if(currRunning.hasBeenRun == 0) {
                currRunning.firstRunTime = clockTime;
                currRunning.hasBeenRun = 1;
            }
            running = 1;
            deq();
        }
        // Deal with a process running on the CPU
        else if(running == 1) {
            currRunning.CPUBurstsandWaitTimes[currRunning.currBurst]--;
            totalTimeProcessesonCPU++;
            quantumCounter++;
            if((currRunning.CPUBurstsandWaitTimes[currRunning.currBurst] <= 0)
            && (currRunning.currBurst != (currRunning.numCPUBursts*2 - 2))) {
                printf("Process %d", currRunning.pid);
                printf(" moved to wait state.\n");
                clockTime = clockTime + contextSwitch;
                applyContextSwitchtoWaitingProcesses();
                currRunning.currBurst++;
                waitingProcesses[currRunning.pid] = currRunning;
                quantumCounter = 0;
                running = 0;
            }
            else if((currRunning.CPUBurstsandWaitTimes[currRunning.currBurst] <= 0)
                 && (currRunning.currBurst >= (currRunning.numCPUBursts*2 - 2))) {
                printf("Process %d", currRunning.pid);
                printf(" has completed.\n");
                currRunning.completionTime = clockTime;
                completedProcesses[numCompleted] = currRunning;
                numCompleted++;
                quantumCounter = 0;
                running = 0;
            }
            else if(quantumCounter >= quantum) {
                printf("Quantum expired, placing process %d", currRunning.pid);
                printf(" back on the ready queue.\n");
                enq(currRunning);
                quantumCounter = 0;
                running = 0;
            }
            else {
                printf("Process %d", currRunning.pid);
                printf(" still running on the CPU.\n");
            }
        }
        // Update waiting processes, put back on ready queue if done waiting
        for(i = 0; i < 10000; i++) {
            if(waitingProcesses[i].pid != -1) {
                waitingProcesses[i].CPUBurstsandWaitTimes[waitingProcesses[i].currBurst]--;
                if(waitingProcesses[i].CPUBurstsandWaitTimes[waitingProcesses[i].currBurst] <= 0) {
                    printf("Process %d", waitingProcesses[i].pid);
                    printf(" is done waiting, moving to ready queue.\n");
                    waitingProcesses[i].currBurst++;
                    enq(waitingProcesses[i]);
                    waitingProcesses[i] = emptyProcess;
                }
                else {
                    printf("Process %d", waitingProcesses[i].pid);
                    printf(" is waiting.\n");
                }
            }
        }
        printf("\n");
    }
    endTime = clockTime;
    return;
}

void sjnp() {
    emptyProcess.pid = -1;
    emptyProcess.currBurst = 0;
    emptyProcess.CPUBurstsandWaitTimes[0] = 10000000;
    for(i = 0; i < 10000; i++) {
        waitingProcesses[i] = emptyProcess;
    }
    for(i = 0; i < 10000; i++) {
        readyJobs[i] = emptyProcess;
    }
    int running = 0;
    int numWaiting = 0;
    int numCompleted = 0;
    int quantumCounter = 0;
    int shortestJobThisTime;
    int shortestJobPID;
    process currRunning;
    create();
    p=-1;
    while(numCompleted != numProcesses) {
        p++;
        clockTime++;
        printf("Time %d", clockTime);
        printf("\n");
        // Put processes in the ready queue
        while((allProcesses[counter].arrivalTime <= clockTime) && (counter <= (numProcesses-1))) {
            printf("Placing process %d", allProcesses[counter].pid);
            printf(" on the ready queue.\n");
            readyJobs[counter] = allProcesses[counter];
            counter++;
        }
        // Place a process on the CPU
        if(running == 0 && readyJobsEmpty() == 0) {
            currRunning = readyJobs[0];
            shortestJobThisTime = currRunning.CPUBurstsandWaitTimes[currRunning.currBurst];
            shortestJobPID = 0;
            for(i = 0; i < 10000; i++) {
                if(readyJobs[i].CPUBurstsandWaitTimes[readyJobs[i].currBurst] < shortestJobThisTime) {
                    currRunning = readyJobs[i];
                    shortestJobThisTime = currRunning.CPUBurstsandWaitTimes[currRunning.currBurst];
                    shortestJobPID = currRunning.pid;
                }
            }
            if(currRunning.hasBeenRun == 1) {
                clockTime = clockTime + contextSwitch;
                applyContextSwitchtoWaitingProcesses();
            }
            printf("Placing process %d", currRunning.pid);
            printf(" on the CPU.\n");
            if(currRunning.hasBeenRun == 0) {
                currRunning.firstRunTime = clockTime;
                currRunning.hasBeenRun = 1;
            }
            running = 1;
            readyJobs[shortestJobPID] = emptyProcess;
        }
        // Deal with a process running on the CPU
        else if(running == 1) {
            currRunning.CPUBurstsandWaitTimes[currRunning.currBurst]--;
            totalTimeProcessesonCPU++;
            quantumCounter++;
            if((currRunning.CPUBurstsandWaitTimes[currRunning.currBurst] <= 0)
            && (currRunning.currBurst != (currRunning.numCPUBursts*2 - 2))) {
                printf("Process %d", currRunning.pid);
                printf(" moved to wait state.\n");
                clockTime = clockTime + contextSwitch;
                applyContextSwitchtoWaitingProcesses();
                currRunning.currBurst++;
                waitingProcesses[currRunning.pid] = currRunning;
                quantumCounter = 0;
                running = 0;
            }
            else if((currRunning.CPUBurstsandWaitTimes[currRunning.currBurst] <= 0)
                 && (currRunning.currBurst >= (currRunning.numCPUBursts*2 - 2))) {
                printf("Process %d", currRunning.pid);
                printf(" has completed.\n");
                currRunning.completionTime = clockTime;
                completedProcesses[numCompleted] = currRunning;
                numCompleted++;
                quantumCounter = 0;
                running = 0;
            }
            else if(quantumCounter >= quantum) {
                printf("Quantum expired, placing process %d", currRunning.pid);
                printf(" back on the ready queue.\n");
                readyJobs[currRunning.pid] = currRunning;
                quantumCounter = 0;
                running = 0;
            }
            else {
                printf("Process %d", currRunning.pid);
                printf(" still running on the CPU.\n");
            }
        }
        // Update waiting processes, put back on ready queue if done waiting
        for(i = 0; i < 10000; i++) {
            if(waitingProcesses[i].pid != -1) {
                waitingProcesses[i].CPUBurstsandWaitTimes[waitingProcesses[i].currBurst]--;
                if(waitingProcesses[i].CPUBurstsandWaitTimes[waitingProcesses[i].currBurst] <= 0) {
                    printf("Process %d", waitingProcesses[i].pid);
                    printf(" is done waiting, moving to ready queue.\n");
                    waitingProcesses[i].currBurst++;
                    readyJobs[i] = waitingProcesses[i];
                    waitingProcesses[i] = emptyProcess;
                }
                else {
                    printf("Process %d", waitingProcesses[i].pid);
                    printf(" is waiting.\n");
                }
            }
        }
        printf("\n");
    }
    endTime = clockTime;
    return;
}

int readyJobsEmpty() {
    for(i = 0; i < 10000; i++) {
        if(readyJobs[i].pid != -1) return 0;
    }
    return 1;
}

void applyContextSwitchtoWaitingProcesses() {
    for(i = 0; i < 10000; i++) {
        if(waitingProcesses[i].pid != -1) {
            waitingProcesses[i].CPUBurstsandWaitTimes[waitingProcesses[i].currBurst] =
            waitingProcesses[i].CPUBurstsandWaitTimes[waitingProcesses[i].currBurst] - contextSwitch;
        }
    }
}

int main() {
    printf("Welcome to my CPU Scheduling Simulator!\n\n");
    printf("Enter filename (include .txt after filename): ");
    s = fgets(str, 100, stdin);
    s = strchr(str, '\n');
    if(s != NULL) *s = 0x0;
    schedFile = fopen(str, "r");

    if(schedFile != NULL) {
        printf("Success, file found.  Parsing through file.\n");
        char line [255];
        while(fgets (line, sizeof line, schedFile) != NULL) {
            token = strtok(line, spc);

            while(token != NULL) {
                args[i] = token;
                i++;
                token = strtok(NULL, spc);
                }

                process currStruct;
                currStruct.pid = p;
                currStruct.arrivalTime = strtol(args[0], &end, 10);
                currStruct.numCPUBursts = strtol(args[1], &end, 10);
                printf("%d", currStruct.arrivalTime);
                printf(" %d", currStruct.numCPUBursts);
                currStruct.currBurst = 0;
                currStruct.hasBeenRun = 0;
                currStruct.totalCPUBurstTime = 0;

                currStruct.numTotal = (currStruct.numCPUBursts*2 - 1);

                for(i = 2; i < (currStruct.numTotal + 2); i++) {
                    currStruct.CPUBurstsandWaitTimes[j] = strtol(args[i], &end, 10);
                    printf(" ");
                    printf("%d", currStruct.CPUBurstsandWaitTimes[j]);
                    j++;
                }

                for(i = 0; i < currStruct.numTotal; i=i+2) {
                    currStruct.totalCPUBurstTime = currStruct.totalCPUBurstTime + currStruct.CPUBurstsandWaitTimes[i];
                }

                printf("\n");
                j = 0;

                myInsert(currStruct);

                i = 0;
                p++;
                }

            numProcesses = p++;
            fclose(schedFile);
            printf("Parsing complete.  Enter scheduling algorithm (FCFSNP, FCFSP or SJNP): ");
            s = fgets(str, 100, stdin);
            s = strchr(str, '\n');
            if(s != NULL) *s = 0x0;
            schedAlg = str;
            if(strcmp(schedAlg, "FCFSNP") == 0) schedAlgID = 0;
            if(strcmp(schedAlg, "FCFSP") == 0) schedAlgID = 1;
            if(strcmp(schedAlg, "SJNP") == 0) schedAlgID = 2;

            if(schedAlgID == 1 || schedAlgID == 2) {
                printf("Enter quantum: ");
                s = fgets(str, 100, stdin);
                s = strchr(str, '\n');
                if(s != NULL) *s = 0x0;
                quantum = strtol(str, &end, 10);
            }

            printf("Enter cost of half a context switch: ");
            s = fgets(str, 100, stdin);
            s = strchr(str, '\n');
            if(s != NULL) *s = 0x0;
            contextSwitch = strtol(str, &end, 10);

            printf("Running scheduling simulator on your parameters... \n");
            printf("\n");

            if(schedAlgID == 0) { fcfsnp(); }
            else if(schedAlgID == 1) { fcfsp(); }
            else if(schedAlgID == 2) { sjnp(); }
            printf("Simulation completed!\n");

            printf("\nResults:\n");

            cpuUtilization = ((float)totalTimeProcessesonCPU/endTime) * (float)100;

            for(i = 0; i < numProcesses; i++) {
                completedProcesses[i].turnAroundTime =
                completedProcesses[i].completionTime - completedProcesses[i].arrivalTime;
                if(completedProcesses[i].turnAroundTime > maxTurnAroundTime)
                    maxTurnAroundTime = completedProcesses[i].turnAroundTime;
            }

            for(i = 0; i < numProcesses; i++) {
                averageTurnAroundTime = averageTurnAroundTime + completedProcesses[i].turnAroundTime;
            }
            averageTurnAroundTime = averageTurnAroundTime/numProcesses;

            for(i = 0; i < numProcesses; i++) {
                completedProcesses[i].responseTime =
                completedProcesses[i].firstRunTime - completedProcesses[i].arrivalTime;
                if(completedProcesses[i].responseTime > maxResponseTime)
                    maxResponseTime = completedProcesses[i].responseTime;
            }

            for(i = 0; i < numProcesses; i++) {
                averageResponseTime = averageResponseTime + completedProcesses[i].responseTime;
            }
            averageResponseTime = averageResponseTime/numProcesses;

            for(i = 0; i < numProcesses; i++) {
                completedProcesses[i].waitTime =
                completedProcesses[i].turnAroundTime - completedProcesses[i].totalCPUBurstTime;
                if(completedProcesses[i].waitTime > maxWaitTime)
                    maxWaitTime = completedProcesses[i].waitTime;
            }

            for(i = 0; i < numProcesses; i++) {
                averageWaitTime = averageWaitTime + completedProcesses[i].waitTime;
            }
            averageWaitTime = averageWaitTime/numProcesses;

            printf("\n");
            printf("CPU Utilization: ");
            printf("%f", cpuUtilization);
            printf("%%");
            printf("\n");
            printf("\n");
            printf("Average turn around time: ");
            printf("%d", averageTurnAroundTime);
            printf("\n");
            printf("Maximum turn around time: ");
            printf("%d", maxTurnAroundTime);
            printf("\n");
            printf("\n");
            printf("Average response time: ");
            printf("%d", averageResponseTime);
            printf("\n");
            printf("Maximum response time: ");
            printf("%d", maxResponseTime);
            printf("\n");
            printf("\n");
            printf("Average wait time: ");
            printf("%d", averageWaitTime);
            printf("\n");
            printf("Maximum wait time: ");
            printf("%d", maxWaitTime);
            printf("\n");
    }
    else printf("Failure, file couldn't be found!\n");

    return 0;
}
