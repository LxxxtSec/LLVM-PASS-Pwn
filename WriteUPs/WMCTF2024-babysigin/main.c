void WMCTF_OPEN(char *name);
void WMCTF_READ(int cmd);
void WMCTF_MMAP(int cmd);
void WMCTF_WRITE(int cmd);
char *filename = "./flag";
char *flag = "./flag";
int cmd = 0x8888;
void f0(char* name);
void f1(char* name);
void f2(char* name);
void f3(char* name);


void func0(char *name) {
        WMCTF_OPEN(filename);
}

void func1(char* name) {
        func0(filename);
}

void func2(char* name) {
        func1(filename);
}

void func3(char *name) {
        func2(filename);
}

void func4(char *name) {
        func3(flag);
}

void func5() {
        flag = "./flag";
        func4(flag);
}

void funcmain() {
    WMCTF_MMAP(0x7890);
    WMCTF_READ(0x6666);
    WMCTF_WRITE(cmd);
}