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
  TK_NOTYPE = 256, TK_EQ, TK_NUMBER,TK_NEGATIVE,

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
   //第二项的‘+’本身是int，不是char
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
  {"(0u?|[1-9][0-9]*u?)", TK_NUMBER}, // c/c++ 无符号十进制字面量：25u
//0u? match 0,0u
//[1-9][0-9]*u? match 1-9,1-9 0-9(>=1),1-9 u,1-9 0-9(>=1) u
// | means or in regex
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
        //0        1        2         3         4
        //char     char     char      char      char     char     char      char
        //re_so                       rm_eo                                           
        //string_start                next_char_start                            
        //e+position=e+0=e                        
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

void token_negative() {
    int i;
    for (i = 0;i < nr_token; i++){
        if (tokens[i].type =='-'){
          if(i==0 ||(tokens[i - 1].type != TK_NUMBER && tokens[i - 1].type != ')')) 
          {tokens[i].type =TK_NEGATIVE;  }
        }
    }
}
//make_tokens()把负号和减号都识别为‘-’。then，进一步识别负号
//负号：1出现在要识别的字符串开头  2前面符号是(,+,-,*,/,不是)  3前面不能是数字
//minor :不出现在要识别的字符串开头  2前面符号是），不是(,+,-,*,/  3前面是数字
typedef struct{
  int top;
  int type;
 } stack;
//检查表达式的左右括号是否匹配
//make_nr_token=5 ((1)) (12)) ((12) )(12)  p=0,q=4
//(1()) 
//(1)*2 
bool check_all_parentheses_match(int p, int q){
  int stack_top = -1;
  bool flag = true;
  for (int i = p; i <= q ; i++){
    if (tokens[i].type =='('){
        stack_top++; 
    }
    else if(tokens[i].type == ')'){    
      if (stack_top >= 0){
        stack_top--;          
      } 
      else {
        flag = false;
        break;
      }
    }
  }
  return (stack_top==(-1) && flag == true) ;
}

//(4 + 3)) * ((2 - 1) right
//1+1 error
bool check_parentheses(int p, int q){
  if (tokens[p].type == '(' && tokens[q].type ==')'){ //判断表达式是否被一对括号包围着
    return check_all_parentheses_match(p + 1, q - 1);  
    //最外层已经有了a paird parentheses,so p-1,q+1
  }
  return false;
}

int priority(int op_type){
    switch (op_type)
    {
    case '(':
    case ')':
        return 0;
        break;
    case '+':
    case '-':
        return 1;
        break;
    case '*':
    case '/':
        return 2;
        break;
    case TK_NEGATIVE:
        return 3;
        break;
    default:
        Assert(0, "No corresponding operator found");
    }
}

word_t number;
word_t eval(int p,int q,bool *success) {
  if (p > q) { //empty token expr
    *success = false;
    return 0;
  }
  else if (p == q) {
//这里的p，q为什么是tokens array下标 ??????????????? 
//为了在token表达式中指示表达式, 用两个整数p和q来指示这个表达式(1u)的开始位置和结束位置
  if (sscanf(tokens[p].str,"%u", &number) < 1){
      *success = false;
      return 0;
    }
    return number;
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