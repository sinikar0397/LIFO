#include "./src/headers.h"
#include "./src/people/people.h"

void printDigest(Password pw){
    for(int i=0;i<16;i++){
        printf("%02x", pw.digest[i]);
    }
    printf("\n");
}

int main(void){

    printf("===== Password Test =====\n");

    Password pw1 = hashPassword("1234");
    Password pw2 = hashPassword("1234");
    Password pw3 = hashPassword("abcd");

    printf("pw1 : ");
    printDigest(pw1);

    printf("pw2 : ");
    printDigest(pw2);

    printf("pw3 : ");
    printDigest(pw3);

    printf("pw1 == pw2 : %s\n",
           isPasswordSame(pw1, pw2) ? "TRUE" : "FALSE");

    printf("pw1 == pw3 : %s\n",
           isPasswordSame(pw1, pw3) ? "TRUE" : "FALSE");


    printf("\n===== CreatePeople Test =====\n");

    People* p1 = createPeople(
        "Alice",
        "alice01",
        "1234",
        "INTJ",
        "ENFP",
        Female,
        21
    );

    if(p1 == NULL){
        printf("createPeople failed\n");
        return 1;
    }

    printPeople(p1);


    printf("\n===== ChangePeople Test =====\n");

    changePeopleName(p1, "Bob");
    changePeopleId(p1, "bob01");
    changePeoplePw(p1, "5678");
    changePeopleType(p1, "ENTP");
    changePeopleLoveType(p1, "ISFJ");
    changePeopleGen(p1, Male);
    changePeopleAge(p1, 25);

    printPeople(p1);


    printf("\n===== Save Test =====\n");

    savePeople(p1, "./database/peoples/person.json");

    printf("Saved to person.json\n");


    printf("\n===== Read Test =====\n");

    People* p2 = readPeople("./database/peoples/person.json");

    if(p2 == NULL){
        printf("readPeople failed\n");
        deletePeople(p1);
        return 1;
    }

    printPeople(p2);


    printf("\n===== Compare Loaded Password =====\n");

    if(isPasswordSame(p1->pw, p2->pw))
        printf("Password Match\n");
    else
        printf("Password Mismatch\n");


    printf("\n===== Cleanup =====\n");

    deletePeople(p1);
    deletePeople(p2);

    printf("All tests completed.\n");

    return 0;
}