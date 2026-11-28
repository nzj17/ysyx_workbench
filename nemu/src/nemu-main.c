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

#include <common.h>
#include <utils.h>
void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();
extern word_t expr(char *e, bool *success);
int main(int argc, char *argv[]) {
  /* Initialize the monitor. */
#ifdef CONFIG_TARGET_AM
  am_init_monitor();
#else
  init_monitor(argc, argv);
#endif
static char line[100+65536] = {}; 
uint32_t expected_result;
static char expr_buf[65536] = {}; //memory !!a!! generated expr

  FILE *fp=fopen("/home/nzj17/ysyx-workbench/nemu/tools/gen-expr/build/input","r");
  if (fp == NULL) { panic("fopen /nemu/tools/gen-expr/build/input failed"); return 1;  }
 while(fgets(line,sizeof(line),fp)!=NULL) { 
  //fgets()先遇到buf[65536]='\0' dont terminate,  meet \n then 读取terminate, then add '\0' after '\n' 
        int cnt = sscanf(line, "%u %65536[^\n]", &expected_result, expr_buf); 
//sscanf扫描集合占位符," [^\n]"跳过前空白读取，直到遇到\n不读取终止
        // 使用 sscanf 的高级用法，直接过滤掉空格，提取表达式
        // "%u" 读数字，" %[^\n]" 读空格后面的所有非换行字符
        if (cnt != 2) {
            printf("sscanf() doesn't get expected_result and expr_str error\n fgets() line: %s\n",line);
            continue;
        }
        bool success=true;//!!!must初始化为true，else success不一定为true/false；
/*because eval() if/else if/else pathes ,only define *success=false when if (...),
don't define *success=true when if(...). so assume all eval() running are true,
then *success keeping oringal is true/false/else 
then all eval运算完成，递归完成，
check if (*success == false) return 第一层表达式最终结果递归为0; //in expr.c line 297 ,then */
        word_t ans = expr(expr_buf, &success);
        //if 解析失败 || 结果不相等 ,output red error-information
        if(success==false || ans != expected_result) {
printf(ANSI_FMT("========== TEST FAILED ==========\nExpr     : %s\nExpected : %u\nGot      : %u\nSuccess  : %d\n=================================\n", ANSI_FG_RED) "\n",expr_buf,expected_result,ans,success);
 }
//0||1=1 Assert(success==true && ans == expected_result,"========== TEST FAILED ==========\nExpr     : %s\nExpected : %u\nGot      : %u\nSuccess  : %d\n=================================\n",expr_buf,expected_result,ans,success);
 }
     fclose(fp);
  /* Start engine. */
  engine_start();

  return is_exit_status_bad();
}
