int main(int argc, char *argv[]);
void proc1(int param1, char *param2, int param3);


/** address: 0x00001d18 */
int main(int argc, char *argv[])
{
    int g4; 		// r4
    int local0; 		// m[g1 - 112]

    g4 = *(argv + 4);
    proc1(argc, g4, local0);
    printf("%d\n", argc);
    return 0;
}

/** address: 0x00001c74 */
void proc1(int param1, char *param2, int param3)
{
    int g3; 		// r3
    int g3_2; 		// r3{7}
    int g3_5; 		// r3{9}
    int local0; 		// m[g1 + 24]
    int local1; 		// m[g1 - 32]
    int local2; 		// param3{13}

    local2 = param3;
    if (param1 <= 2) {
        g3 = strlen(param2);
        local0 = g3;
    }
    else {
        g3_2 = strlen(param2);
        local0 = g3_2;
        g3_5 = strlen(param2);
        local1 = g3_5;
        local2 = local1;
        printf("%d", g3_2 + g3_5);
    }
    param3 = local2;
    printf("%d, %d", local0, param3);
    return;
}

