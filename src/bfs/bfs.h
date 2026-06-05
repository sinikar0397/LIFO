#ifndef MATCHING_H
#define MATCHING_H

#include "../people/people.h"
#define MAX_PEOPLE 5000

typedef enum MatchStatus {
	AVAILABLE, // 매칭 가능
	PROPOSED,  // 추천된 상태, 아직 양쪽 수락 전
	MATCHED,   // 최종 매칭 완료
	PAUSED,	   // 추천 일시 중단
	DELETED	   // 탈퇴 또는 삭제
} MatchStatus;

typedef struct MatchingInfo {
	People *person;
	MatchStatus status;
	int preference[MAX_PEOPLE];
	int match_idx;
	int prefsize;
	int nextProposal;
} MatchingInfo;

typedef struct Pair {
	int p1;
	int p2;
} Pair;

typedef struct Candidate {
	int idx;
	int score;
} Candidate;

/* MatchingInfo initialization */

void initMatchingInfo(MatchingInfo *info, People *person);
void initMatchingInfos(MatchingInfo infos[], People *people[], int n);

/* User status functions */

void changeStatus(MatchingInfo infos[], int idx, MatchStatus status);

/* Candidate filtering */
/* User insertion / deletion */

void addUser(MatchingInfo infos[], int *n, People *newPerson);
void removeUser(MatchingInfo infos[], int userIdx);

int stableMatching(MatchingInfo infos[], int n, int proposers[],
				   int proposerCnt, int rankTable[][MAX_PEOPLE], Pair result[]);

/* Proposal and match confirmation */

void createProposal(MatchingInfo infos[], int u, int v);
void confirmMatch(MatchingInfo infos[], int u, int v);
void rejectMatch(MatchingInfo infos[], int u, int v);

#endif // MATCHING_H