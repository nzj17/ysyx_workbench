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

void init_regex() { //在monitor.c中init_sdb()调用
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
  char str[100]; //匹配超长数字
} Token;

//__attribute__ is a key creating of gcc extendsion  声明额外属性
static Token tokens[65536] __attribute__((used)) = {}; //65536是gen-expr的字符数组长度（include '\0')
static int nr_token __attribute__((used))  = 0;

//一次词法解析一个字符串表达式到Token tokens[65536]
static bool make_token(char *e) { 
  int position = 0;
  regmatch_t pmatch; //re_so,re_eo: a signed integer type 
  nr_token = 0;
  while (e[position] != '\0') {
    int i;
    /* Try all rules one by one. */
    for (i= 0; i < NR_REGEX; i++) {
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
            Assert(nr_token<65536,"error:记录ability的structure tokens[32] is full");
            tokens[nr_token].type=TK_NUMBER;
            Assert(substr_len<32,"error:single multi_bytes token at most 31,strncpy() copys without \\0");
            strncpy(tokens[nr_token].str,substr_start,substr_len);
            tokens[nr_token].str[substr_len]='\0';
            nr_token++;  //
            break;
          }
          default: 
            Assert(nr_token<65536,"error:记录ability的 structure tokens[32] is full");
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
//检查表达式的左右括号是否匹配
//make_nr_token=5 ((1)) (12)) ((12) )(12)  p=0,q=4
//(1()) 
//(1)*2 
bool check_all_parentheses_match(int p, int q){
  /*检查表达式的all左右括号是否匹配, left ( number equals right ) nr ，if 表达式 is in law*/
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
    /*检查去掉一层外括号对后，表达式的all左右括号是否匹配*/
  }
  return false;
}

int priority(int type){
    switch (type)
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

typedef struct{
  int idx;
  int type;
 } stack_frame; 
//static stack_frame stack[100];

word_t eval(int p,int q,bool *success) {
//-----------------------------------------------
  if (p > q) { //empty token expr
    *success = false;
    return 0;
  }
//-----------------------------------------------
  else if (p == q) {
//这里的p，q为什么是tokens array下标 ??????????????? 
//为了在token表达式中指示表达式, 用两个整数p和q来指示这个表达式(1u)的开始位置和结束位置
  word_t number=0;
  if (sscanf(tokens[p].str,"%u", &number) < 1){ 
    // sscanf("9u", "%u",&number);>>> number=9
      *success = false;
      return 0;
    }
    return number;
  }
//-----------------------------------------------
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses and
    去掉一层外括号对后， 表达式的all左右括号匹配 in law
     * If that is the case, just throw away the parentheses.
     */

    return eval(p + 1, q - 1,success);
  }
//-----------------------------------------------
  else { 
    if (check_all_parentheses_match(p, q) == false){
      *success = false;
      return 0;
    } 
    //then,now case:左右括号数目相等，既是合法，但外层没有套一对括号的表达式

    stack_frame stack[100]; int top = -1;
    for (int i=p; i <= q; i++){
      Assert(top<100, "stack  overflow!"); //这里top什么时候大于等于100？？？？
        if(tokens[i].type == TK_NUMBER) continue;
        if(top < 0){
            top++;
            stack[top].idx = i;
            stack[top].type = tokens[i].type;
            continue;
        }
        if(tokens[i].type =='('){
            top++;
            stack[top].idx = i;
            stack[top].type = tokens[i].type;
            continue;
        }
        if (tokens[i].type ==')'){
            /*出现在一对括号中的token不是主运算符*/
            while (top>=0&&stack[top].type!= '('){  //出栈'('到')'之间的运算符
                top--;  
            }
            top--;  continue;  //出栈'('
        } 
        /*优先级越低越在stack下面,主运算符的优先级在表达式中是最低的.当有多个运算符的优先级都是最低时, 根据结合性, 最后被结合的运算符才是主运算符*/
        while (top >= 0 && priority(tokens[i].type) <= priority(stack[top].type) ){
            //当负号前面是负号时，主运算符是前面的负号，如--(3*5)
            if(tokens[i].type==stack[top].type&&stack[top].type==TK_NEGATIVE) {break;}
            top--; //从栈顶出
        }
            top++;
            stack[top].idx = i;
            stack[top].type = tokens[i].type;
    }
    Assert(top<100, "stack  overflow!");
    //==========================================
    word_t val1;
    word_t val2;
    int type;
    /*If the primary operator cannot be found, the expression is incorrect.*/
    if (top < 0){
      *success = false;
      return 0;
    }
    else if (stack[0].type == TK_NEGATIVE){
      val1 = -1;
      val2 = eval(stack[0].idx + 1, q, success);  //--(3*5)=-1 * -(3*5)
      type = '*';
    } 
    else {
      val1 = eval(p, stack[0].idx - 1, success); 
      /*val1=eval()多层递归计算完成后，由于递归会修改全局static stack_frame stack[100]，
      使得stack[0].idx - 1）已经改变，不再是整体表达式的主运算符,所以val2 mistakes ，把
      static stack_frame stack[100] 改成   else（line 239）里面的局部变量，
      这样各递归之间各自有独立的一份stack[100],
      递归本质上是在内存栈区中不断push，直到递归取得数值，然后不断pull*/
      val2 = eval(stack[0].idx + 1, q, success);
      type = stack[0].type;
    }
    //========================================
    if (*success == false) return 0;
    switch (type){
    case '+':
      return val1 + val2;
    case '-':
      return val1 - val2;
    case '*':
      return val1 * val2;
    case '/': {
      if (val2 == 0){
        *success = false;
        return 0;
      }  // 除0处理(但是其实，生成的几千测试表达式没有除0)
      return val1 / val2;
    }
    default:
      Assert(0, "No corresponding operator found");
    }
    //=======================================
  }
//-----------------------------------------------
}
word_t expr(char *e, bool *success) {
  if (make_token(e)==false) {
    *success = false;
    return 0;
  }
  /* TODO: Insert codes to evaluate the expression. */
  //TODO();
  token_negative();
  word_t MyExprResult=eval(0, nr_token - 1, success);
  if((*success)==true) {return MyExprResult;}
  else {return 0;}
}