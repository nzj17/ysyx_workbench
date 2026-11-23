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

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NUMBER,

  /* TODO: Add more token types */
  //256 257 258 ...
  //ASCII:0~255
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
  {"\\-", '-'},         // sub
//c语言\\被gcc编译成一个\转义符，
//\regex元字符 matches  signals of orginal meanings,instead of special meanings
  {"\\(", '('},           // left parenthesis
  {"\\)", ')'},           // right parenthesis
  {"\\*", '*'},         // multiply
  {"/", '/'},           // division
  {"(0u?|[1-9][0-9]*u?)", TK_NUMBER}, // decimal(10 count rule) integer
//0u? match 0,0u
//[1-9][0-9]*u? match 1-9,1-9 0-9(>=1),1-9 u,1-9 0-9(>=1) u
// | means or
};
#define NR_REGEX ARRLEN(rules) //9
static regex_t re[NR_REGEX] = {}; //存储编译后的regex(matching models,take a example:[1-9])
/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() { 
//regcomp(regex_t *point_regex_t_structure,,)编译正则表达式。
  int i;
  char error_msg[128];
  int ret;
  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);    
    }
  }
}
typedef struct token {
  int type;
  char str[32];
} Token;
//__attribute__ is a key creating of gcc extendsion  声明额外属性
static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;
static bool make_token(char *e) {
  char *e ="1e";
  int position = 0;
  int i;
  regmatch_t pmatch; //re_so,re_eo: a signed integer type 
  nr_token = 0;
  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) { 
      // regexec(&re[i], e + position, 1, &pmatch, 0) == 0 
      //从e+position位置找到,rm_so offset e+position
        char *substr_start = e + position; 
        int substr_len = pmatch.rm_eo;  
        //
        //re_so    rm_eo
        //1        e
        //0        1        2         3         4
        //char     char     char      char      char     char     char      char
        //re_so                       rm_eo                                           
        //string_start                next_char_start                            
        //e                           position
        //                            e+position
        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len; 
        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens , some extra actions should be performed.
         */
        switch (rules[i].token_type) {
          case TK_NOTYPE:{
            break;}
          case TK_NUMBER:{
            Assert(nr_token<32,"error:记录ability的structure tokens[32] is full");
            tokens[nr_token].type=TK_NUMBER;
            Assert(substr_len<32,"error:single multi_bytes token at most 31,strncpy() copys without '\0'");
            strncpy(tokens[nr_token].str,substr_start,substr_len);
            tokens[nr_token].str[substr_len]='\0';
            nr_token++;
            break;
          }
          default: 
            Assert(nr_token<32,"error:记录ability的 structure tokens[32] is full");
            tokens[nr_token].type=rules[i].token_type;
            nr_token++;
            break;
        }
        break;
      }
    }
    if (i == NR_REGEX) {  //i==9
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      //position, "" : position print position-number spaces ,then print ^
      return false;
    }
  }
  return true;
}
word_t eval(int p,int q,bool *success) {
  if (p > q) {
    *success = false;
    return 0;
  }
  else if (p == q) {
    //??????????????? 这里的p为什么是nr_token，那q是什么 ??????????????? 

  }
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
  }
  else {
    /* We should do more things here. */
  }
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  TODO();

  return 0;
}