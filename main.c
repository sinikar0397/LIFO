#include "./src/headers.h"
#include "./src/dfs/dfs.h"
#include "./src/people/login.h"
#include "./src/people/people.h"

int main(void) {

	printf("========================================\n");
	printf(" Login System Test\n");
	printf("========================================\n");

	login_init();

	while (1) {

		int cmd;

		printf("\n");
		printf("1. 회원가입\n");
		printf("2. 로그인\n");
		printf("3. ID 존재 여부 확인\n");
		printf("4. 계정 정보 조회\n");
		printf("5. Date Fit Survey 실행\n");
		printf("6. 이상형-상대 성격 유사도 계산\n");
		printf("7. 종료\n");
		printf("입력 : ");

		scanf("%d", &cmd);

		switch (cmd) {

		case 1: {
			printf("\n[회원가입]\n");

			People *p = login_sign_in_account();

			printf("\n저장된 계정 정보\n");
			people_print_people(p);

			people_delete_people(p);

			break;
		}

		case 2: {
			printf("\n[로그인]\n");

			People *p = login_log_in_account();

			if (p != NULL) {
				printf("\n로그인 성공\n");
				people_delete_people(p);
			}

			break;
		}

		case 3: {
			char id[MAX_ID_LEN];

			printf("검색할 ID : ");
			scanf("%s", id);

			if (login_does_ID_exist(id))
				printf("존재하는 ID입니다.\n");
			else
				printf("존재하지 않는 ID입니다.\n");

			break;
		}

		case 4: {
			char id[MAX_ID_LEN];

			printf("조회할 ID : ");
			scanf("%s", id);

			if (!login_does_ID_exist(id)) {
				printf("존재하지 않는 ID입니다.\n");
				break;
			}

			People *p = login_get_account(id);

			if (p == NULL) {
				printf("계정 로드 실패\n");
				break;
			}

			people_print_people(p);

			people_delete_people(p);

			break;
		}

		case 5: {
			char type[MAX_TYPE_LEN];

			dfs_run_date_fit_survey(type, "Date Fit Survey");
			printf("\n저장 가능한 최종 유형 코드 : %s\n", type);

			break;
		}

		case 6: {
			char ideal_owner_id[MAX_ID_LEN];
			char other_id[MAX_ID_LEN];

			printf("이상형 기준이 되는 사용자 ID : ");
			scanf("%19s", ideal_owner_id);
			printf("상대 사용자 ID : ");
			scanf("%19s", other_id);

			if (!login_does_ID_exist(ideal_owner_id) || !login_does_ID_exist(other_id)) {
				printf("존재하지 않는 ID가 있습니다.\n");
				break;
			}

			People *ideal_owner = login_get_account(ideal_owner_id);
			People *other = login_get_account(other_id);

			if (ideal_owner == NULL || other == NULL) {
				printf("계정 로드 실패\n");
				people_delete_people(ideal_owner);
				people_delete_people(other);
				break;
			}

			int distance = dfs_type_distance(ideal_owner->love_type, other->type);
			int similarity = dfs_type_similarity(ideal_owner->love_type, other->type);
			int total_score = compat(ideal_owner, other);

			printf("\n[%s의 이상형] %s\n", ideal_owner->id, ideal_owner->love_type);
			dfs_print_type_description(ideal_owner->love_type);
			printf("\n[%s의 성격] %s\n", other->id, other->type);
			dfs_print_type_description(other->type);
			printf("\nDistance = Depth(u) + Depth(v) - 2 * Depth(LCA(u, v)) = %d\n",
				   distance);
			printf("이상형-상대 성격 유사도 : %d / 100\n", similarity);
			printf("양방향 예상 호감도 : %d / 200\n", total_score);

			people_delete_people(ideal_owner);
			people_delete_people(other);

			break;
		}

		case 7: {
			printf("프로그램 종료\n");
			return 0;
		}

		default:
			printf("잘못된 입력입니다.\n");
		}
	}

	return 0;
}
