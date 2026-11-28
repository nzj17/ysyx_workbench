/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <string.h>
#include <sys/wait.h>   // WIFSIGNALED / WTERMSIG / WIFEXITED
//状态解析宏 含义
//WIFSIGNALED(status) 子进程是否被信号杀死       // wait if signaled
//WTERMSIG(status) 若是，取终止信号号           // wait termation  signal
//WIFEXITED(status) 子进程是否正常exit()/return // wait if exited
//WEXITSTATUS(status) 若是，取退出码（0–255）   //wait exit status code
#include <signal.h>     // SIGFPE:由于运算错误触发的进程信号
// this should be enough
static char buf[65536] = {}; //memory !!a!! generated expr
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";
//c language supports 相邻字符串字面量自动拼接
//\"表示这是一个双引号字符"，不是字符串结束符(c editor 词法分析）
//"  printf(\"%%u\", result); " is printf("%%u", result); 在sprintf(),%%u被转义%u

static uint32_t choose(uint32_t n) {
  return (rand())%n;  //[0,n-1] //rand() [0,a little larger than 30,000]
}
static void gen_num() {
  char my_concat[10]="";
  sprintf(my_concat,"%u",(unsigned)(rand()%50)); //sprintf()把42变成"42" memory
//  buf[length]=(rand()%50);  (rand()%50)=48 equals '\0'  =10 '\n'
  int length1=strlen(my_concat);
  my_concat[length1]='u'; //保证(gcc)表达式进行无符号decimal运算
  strcat(buf,my_concat); //automatically add '\0'
}
static void gen(char character) {  //add a charcter to buf[] 
  int length=strlen(buf);
  buf[length]=character;
  buf[length+1]='\0'; 
} 
static void gen_space(){  //randomly insert space/NOTYPE
   int len=(rand())%3;
  for (int i = 1;i <= len; i++){
   gen(' ');
  }
}
static void gen_rand_op(){
  switch (choose(4)){
  case 0: gen('+'); break;
  case 1: gen('-'); break;
  case 2: gen('*'); break;
  default: gen('/'); break;
  }
}
static void gen_rand_expr(uint32_t current_depth) {
  uint32_t choosing=choose(3);
  if(current_depth>=10) {  
    //recurse/switch 11 counts //不会使buf溢出
   //current_depth 0 1 2 .. 9 10     
    choosing=0;
  }
  switch (choosing) {
    case 0: gen_num(); break;
    case 1:{
       gen('('); 
       gen_rand_expr(current_depth+1); 
       gen(')'); 
       break; 
    }
    default:{
      gen_rand_expr(current_depth+1); 
      gen_space(); 
      gen_rand_op(current_depth+1);
     gen_space();
      gen_rand_expr(current_depth+1); 
      break;
    }
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0); //return now time
  srand(seed);
//seed_rand(seed):set rand()'s initial state 同一个seed，产生同一串随机数
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  
  for (uint32_t i = 0; i < loop; i++) {
    buf[0]='\0';
    gen_rand_expr(0); 
    sprintf(code_buf, code_format, buf); //write to code_buf string.
    FILE *fp = fopen("/tmp/.code.c", "w");  
  //w mode:文件中只是存入for的每一次循环的 gen_rand_expr()产生的一个表达式
    assert(fp != NULL);
    fputs(code_buf, fp); //writes to FILE *stream, without its '\0'.
    fclose(fp);
    int ret = system("gcc -Werror /tmp/.code.c -o /tmp/.expr");
//    int ret = system("gcc -Werror /tmp/.code.c -o /tmp/.expr");
/*过滤分母常量式0：编译时只能检测常量除0并产生warning:division by zero，
加-Werror使waring变成error使ret！0 */
//create a subprocess execing shell command //每次循环，gcc编译一次，编译一个表达式
    if (ret != 0) continue;
    fp = popen("/tmp/.expr", "r");   //run /tmp/.expr 
//creating subprosess,and a single_direction pipe connecting main prosess,subprosess.
    assert(fp!= NULL);
    uint32_t result;
    int ret1 = fscanf(fp, "%u", &result); //read running result of subprosess
    pclose(fp);
/*  int status = pclose(fp);
pclose返回的是 process wait status,关闭文件流fp stream所关联的管道，并等待与之关联的子进程终止。*/
/*popen执行的是/bin/sh -c "tmp/.expr"，
这里的status是shell的退出码=128+SIGFPE=128+8=136(shell exit code ,正常)，
总而言之,当分母是字母变量式为0时，WIFSIGNALED(status)一直等于1,
总之晕头，先别用下面这堆了，目前只能排查分母数字式是0的常量,啧*/
//    if (ret1!= 1 && WIFSIGNALED(status)==1) { 
//        int sig = WTERMSIG(status);
//        if (sig == SIGFPE) { continue;} 
/*运行时报运算错误：floating point exception (core dumped)，这里只会是因为/0，生成信号SIGFPE
能排查分母是字母变量式为0的表达式。因为gcc编译时只会展开常量，只能识别分母数字式是0并警告
    } */
  if(ret1!=1) {continue;}  //all or simple 过滤除0表达式的方法
// 如果fscanf返回值不是1,即result没有得到表达式运算正确值(这里只有/0这种情况，ret1=EOF)
printf("%u %s\n", result, buf); //100 65536(include '\0') \n
}
  return 0;
}
