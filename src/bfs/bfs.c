#include "bfs.h"
#include "../headers.h"

typedef struct Queue {
	int data[MAX_PEOPLE];
	int front;
	int rear;
} Queue;

Queue *createQueue() {
	Queue *q = (Queue *)malloc(sizeof(Queue));
	q.front = 0;
	q.rear = 0;
	return q;
};

int isQueueEmpty(Queue *q) {
	if (q.front == q.rear)
		return 1;
	return 0;
};

int isQueueFull(Queue *q) {
	if (q.front == q.rear + 1 || q.rear - q.front == MAX_PEOPLE - 1)
		return 1;
	return 0;
}

void pushQueue(Queue *q, int x) {
	if (isQueueFull(q)) {
		printf("Error: Queue is Full");
		return;
	}
	q.data[q.rear] = x;
	if (q.rear == MAX_PEOPLE - 1)
		q.rear = 0;
	else
		q.rear++;
};

int popQueue(Queue *q) {
	if (isEmpty(q)) {
		printf("Error: Queue is Empty");
		return -1;
	}
	int x = q.data[q.front];
	if (q.front == MAX_PEOPLE - 1)
		q.front = 0;
	else
		q.front++;
};

void initMatchingInfo(MatchingInfo *info, People *person) {
	if (info == NULL)
		return;
	info->person = person;
	info->status = AVAILABLE;
	info->match_idx = -1;
	info->prefsize = 0;
	info->nextProposal = 0;

	for (int i = 0; i < MAX_PEOPLE; i++) {
		info->preference[i] = -1;
	}
};

void initMatchingInfos(MatchingInfo infos[], People *people[], int n) {
	if (infos == NULL || people == NULL)
		return;
	for (int i = 0; i < n; i++) {
		initMatchingInfo(&infos[i], people[i]);
	}
};