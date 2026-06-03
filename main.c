#include "./src/headers.h"
#include "./src/people/people.h"
#include "./src/people/login.h"


int main(void){

    printf("========================================\n");
    printf(" Login System Test\n");
    printf("========================================\n");

    initializeLogin();

    while(1){

        int cmd;

        printf("\n");
        printf("1. 회원가입\n");
        printf("2. 로그인\n");
        printf("3. ID 존재 여부 확인\n");
        printf("4. 계정 정보 조회\n");
        printf("5. 종료\n");
        printf("입력 : ");

        scanf("%d", &cmd);

        switch(cmd){

            case 1:{
                printf("\n[회원가입]\n");

                People* p = signInAccount();

                printf("\n저장된 계정 정보\n");
                printPeople(p);

                deletePeople(p);

                break;
            }

            case 2:{
                printf("\n[로그인]\n");

                People* p = logInAccount();

                if(p != NULL){
                    printf("\n로그인 성공\n");
                    deletePeople(p);
                }

                break;
            }

            case 3:{
                char id[MAX_ID_LEN];

                printf("검색할 ID : ");
                scanf("%s", id);

                if(existID(id))
                    printf("존재하는 ID입니다.\n");
                else
                    printf("존재하지 않는 ID입니다.\n");

                break;
            }

            case 4:{
                char id[MAX_ID_LEN];

                printf("조회할 ID : ");
                scanf("%s", id);

                if(!existID(id)){
                    printf("존재하지 않는 ID입니다.\n");
                    break;
                }

                People* p = getAccount(id);

                if(p == NULL){
                    printf("계정 로드 실패\n");
                    break;
                }

                printPeople(p);

                deletePeople(p);

                break;
            }

            case 5:{
                printf("프로그램 종료\n");
                return 0;
            }

            default:
                printf("잘못된 입력입니다.\n");
        }
    }

    return 0;
}