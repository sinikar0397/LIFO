#ifndef MATCHING_H
#define MATCHING_H
#include "../people/people.h"

#define MAX_PEOPLE 5000


/*
 * Stable Matching 알고리즘 실행에 필요한 사용자 정보
 * People 구조체는 실제 사용자 정보 저장
 * MatchingInfo는 매칭 상태, 선호도 리스트, 현재 매칭 상대 등 관리
 */
typedef struct MatchingInfo {
	People *person;
	int preference[MAX_PEOPLE];
	int match_idx;
	int pref_size;
	int next_proposal;
} MatchingInfo;

/*
 * Stable Matching 결과로 생성된 pair 사용자 index
 */
typedef struct Pair {
	int p1;
	int p2;
} Pair;

/*
 * 선호도 리스트를 만들 때 사용하는 임시 후보 정보
 * idx   : 후보 사용자 index
 * score : 해당 후보와의 compatibility 점수
 */
typedef struct Candidate {
	int idx;
	int score;
} Candidate;

/*
 * 두 사용자 사이의 예상 호감도 점수를 계산한다.
 * DFS 모듈의 실제 호감도 계산 함수가 완성되면 같은 시그니처로 연결하면 된다.
 */
int compat(People *a, People *b);

/* MatchingInfo initialization */

/*
 * 특정 person의 MatchingInfo 하나를 초기화
 */
void initMatchingInfo(MatchingInfo *info, People *person);

/*
 * 모든 people의 MatchingInfo 배열 초기화
 * people[i]에 해당하는 사용자 정보를 infos[i]에 연결
 */
void initMatchingInfos(MatchingInfo infos[], People *people[], int n);

/* User status functions */

/*
 * idx번 사용자 매칭 상태를 status로 변경
 */
void changeStatus(MatchingInfo infos[], int idx, MatchStatus status);

/* User insertion / deletion */

/*
 * 사용자를 MatchingInfo 배열 끝에 추가
 */
void addUser(MatchingInfo infos[], int *n, People *new_person);

/*
 * user_idx번 사용자를 제거
 * 매칭 상대가 있었다면 상대의 매칭 상태도 해제
 */
void removeUser(MatchingInfo infos[], int user_idx);

/* Candidate filtering */

/*
 * 전체 사용자 중 gen 성별, AVAILABLE 상태인 사용자만 모으는 함수
 * 반환값은 조건 만족한 사용자 수
 */
int collectUser(MatchingInfo infos[], int n, enum Gender gen, int result[]);

/*
 * user_idx 사용자의 선호도 리스트 생성
 * candidates에 대해 compatibility 계산
 * 점수가 높은 순서대로 infos[user_idx].preference에 저장
 */
void makePrefList(MatchingInfo infos[], int user_idx, int candidates[],
				  int cand_size);

/*
 * 모든 AVAILABLE 사용자에 대해 선호도 리스트 생성
 */
void makePrefLists(MatchingInfo infos[], int n);

/* Proposal and match confirmation */

/*
 * u와 v를 추천 쌍으로 설정
 * 사용자 상태를 PROPOSED로 변경
 */
void createProposal(MatchingInfo infos[], int u, int v);

/*
 * u와 v의 추천을 최종 수락 처리
 * 두 사용자 상태를 MATCHED로 변경
 */
void confirmMatch(MatchingInfo infos[], int u, int v);

/*
 * u와 v의 추천을 거절 처리
 * match_idx 초기화, DELETED가 아니면 AVAILABLE로 되돌림
 */
void rejectMatch(MatchingInfo infos[], int u, int v);

/* Main Algorithm */

/*
 * Gale-Shapley Stable Matching 알고리즘을 실행한다.
 *
 * infos        : 전체 사용자 매칭 정보 배열
 * n            : 전체 사용자 수
 * proposers    : 제안자 그룹의 사용자 index 배열
 * proposer_cnt : 제안자 수
 * rank_table   : rank_table[i][j] = i가 j를 몇 번째로 선호하는지 저장하는 배열
 * result       : 최종 추천 쌍을 저장할 Pair 배열
 *
 * return       : 생성된 추천 쌍의 개수
 * 생성된 쌍은 createProposal을 통해 PROPOSED 상태로 변경
 */
int stableMatching(MatchingInfo infos[], int n, int proposers[],
				   int proposer_cnt, int rank_table[][MAX_PEOPLE],
				   Pair result[]);

/*
 * a가 b를 차단했는지 확인하는 함수
 * a가 b를 차단했다면 1, 아니면 0을 반환
 */
int isBlocked(People *a, People *b);

/*
 * a가 b를 차단한다
 * a와 b 두 사람 사이 매칭이 이루어지지 않게 만든다
 */
void blockUser(People *a, People *b);

/*
 * a가 b를 차단 해제한다
 * a도 b를, b도 a를 차단하지 않아야 매칭이 된다
 */
void unblockUser(People *a, People *b);

#endif // MATCHING_H
