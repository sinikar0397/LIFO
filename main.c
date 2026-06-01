#include "headers.h"
#include "people.h"

int main(void) {
    char path[] = "./database/peoples/JuHwan.json";

    People* p1 = createPeople(
        "JuHwan",
        "INTJ",
        Male,
        18
    );

    if (p1 == NULL) {
        printf("createPeople failed\n");
        return 1;
    }

    printf("Saving...\n");
    savePeople(p1, path);

    printf("\n=== Original ===\n");
    printf("name : %s\n", p1->name);
    printf("type : %s\n", p1->type);
    printf("gen  : %d\n", p1->gen);
    printf("age  : %d\n", p1->age);

    People* p2 = readPeople(path);

    if (p2 == NULL) {
        printf("readPeople failed\n");
        free(p1);
        return 1;
    }

    printf("\n=== Loaded ===\n");
    printf("name : %s\n", p2->name);
    printf("type : %s\n", p2->type);
    printf("gen  : %d\n", p2->gen);
    printf("age  : %d\n", p2->age);

    free(p1);
    free(p2);

    return 0;
}