int main(int argc, char *argv[]);


/** address: 0x0804837c */
int main(int argc, char *argv[])
{
    if ((unsigned int)argc > 7) {
bb0x80483a8:
        puts("Other!");
        break;
    }
    switch(argc) {
    case 4:
        if (7 - argc <= 5) {
            switch(10 - argc) {
            case 8:
bb0x80483bc:
                puts("Two!");
                break;
            case 3:
bb0x80483d0:
                puts("Seven!");
                break;
            case 4:
bb0x80483e4:
                puts("Six!");
                break;
            case 5:
bb0x80483f8:
                puts("Five!");
                break;
            case 7:
bb0x804840c:
                puts("Three!");
                break;
            case 6:
                puts("Four!");
                break;
            }
            return 0;
        }
        goto bb0x80483a8;
    case 0:
    case 1:
        goto bb0x80483a8;
    case 2:
        goto bb0x80483bc;
    case 7:
        goto bb0x80483d0;
    case 6:
        goto bb0x80483e4;
    case 5:
        goto bb0x80483f8;
    case 3:
        goto bb0x804840c;
    }
    return 0;
}

