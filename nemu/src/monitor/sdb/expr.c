/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
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
#include <memory/paddr.h>

bool check_parentheses(int p,int q);
bool check_all_bracket(int p,int q);
bool check_is_bracket_match(int p,int q);
int dominant_operator(int p,int q);
int find_matched_bra(int start);
uint64_t eval(int p,int q);


enum {
  TK_NOTYPE = 256, TK_EQ , TK_NUM ,TK_LB , TK_RB , TK_NEQ , TK_HEX, TK_AND, TK_REG, TK_DEREF, TK_NEG

  /* TODO: Add more token types */

};

static struct rule { // 一条规则是由正则表达式和token类型组成的二元组
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},        // spaces,空格加上+代表多个空格
  {"0x[0-9,a-f]+", TK_HEX}, // hexadecimal numbers
  {"[0-9]+", TK_NUM},       // digit
  {"\\$(\\$0|[0-9,a-z]+)", TK_REG}, //register
  {"\\(", TK_LB},           // left bracket
  {"\\)", TK_RB},           // right bracket
  {"\\+", '+'},             // plus，表示特殊字符需要用到转义字符，而转义字符本身也是特殊字符，所以需要用两个
  {"\\-", '-'},             // minus
  {"\\*", '*'},             // multiply
  {"\\/", '/'},             // divide
  {"==", TK_EQ},            // equal
  {"!=", TK_NEQ},           // not equal
  {"&&", TK_AND},           // and
};

#define NR_REGEX ARRLEN(rules) //宏NR_REGEX被定义为规则数组rules的长度

static regex_t re[NR_REGEX] = {}; //regex_t是编译后的正则表达式
//它的成员re_nsub 用来存储正则表达式中的子正则表达式的个数，子正则表达式就是用圆括号包起来的部分表达式。

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() { //init_regex()被编译成一些用于进行pattern匹配的内部信息, 这些内部信息是被库函数使用的
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED); //regcomp函数把指定的正则表达式pattern编译成一种特定的数据格式compiled，这样可以使匹配更有效
    if (ret != 0) { //regcomp执行成功返回0
      regerror(ret, &re[i], error_msg, 128);
      //当执行regcomp 或者regexec 产生错误的时候，就可以调用regerro()而返回一个包含错误信息的字符串
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type; //存放token类型
  char str[32]; //存放具体的字符串
} Token;

static Token tokens[32] __attribute__((used)) = {}; //tokens数组用于按顺序存放已经被识别出的token信息
static int nr_token __attribute__((used))  = 0; //nr_token指示已经被识别出的token数目

static bool make_token(char *e) { //make_token的作用是识别表达式中的token
  /*它用position变量来指示当前处理到的位置, 并且按顺序尝试用不同的规则来匹配当前位置的字符串. 
    当一条规则匹配成功, 并且匹配出的子串正好是position所在位置的时候, 我们就成功地识别出一个token, Log()宏会输出识别成功的信息*/
  int position = 0;
  int i;
  regmatch_t pmatch;
  /*typedef struct {

    regoff_t rm_so;存放匹配文本串在目标串中的开始位置

    regoff_t rm_eo;存放结束位置

} regmatch_t;*/

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position; //匹配后子串的起始位置
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_HEX:
               tokens[nr_token].type = TK_HEX;
               for(int k=0;k<substr_len;k++){
                tokens[nr_token].str[k] = *(e+position-substr_len+k);
               }
               nr_token++;
               break;
          case TK_NUM:
               tokens[nr_token].type = TK_NUM;
               for(int j=0;j<substr_len;j++){
                tokens[nr_token].str[j] = *(e+position-substr_len+j);
               }
               nr_token++;
               break;
          case TK_REG:
               tokens[nr_token].type = TK_REG;
               for(int a=0;a<substr_len-1;a++){ //删去$
                tokens[nr_token].str[a] = *(e+position-substr_len+a+1);
               }
               nr_token++;
               break;              
          case '+':
               tokens[nr_token++].type = '+';
               break;
          case '-':
               tokens[nr_token++].type = '-';
               break;
          case '*':
               tokens[nr_token++].type = '*';
               break;
          case '/':
               tokens[nr_token++].type = '/';
               break;
          case TK_EQ:
               tokens[nr_token++].type = TK_EQ;
               break;
          case TK_NEQ:
               tokens[nr_token++].type = TK_NEQ;
               break;
          case TK_LB:
               tokens[nr_token++].type = TK_LB;
               break;
          case TK_RB:
               tokens[nr_token++].type = TK_RB;
               break;
          case TK_AND:
               tokens[nr_token++].type = TK_AND;
               break;
          case TK_NOTYPE:
               break;
          default: 
               printf("Check your rules table, sth is wrong\n");
               assert(0);
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  //TODO();记得最后清空tokens数组
  for (int i = 0; i < nr_token; i ++) { //将符号和指针解引用的*转变
    if (tokens[i].type == '*' && (i == 0 || (tokens[i-1].type != TK_NUM && tokens[i-1].type !=TK_HEX && tokens[i-1].type != TK_REG && tokens[i-1].type != TK_RB)) ) {
      tokens[i].type = TK_DEREF;
    }
    if (tokens[i].type == '-' && (i == 0 || (tokens[i-1].type != TK_NUM && tokens[i-1].type !=TK_HEX && tokens[i-1].type != TK_REG && tokens[i-1].type != TK_RB)) ) {
      tokens[i].type = TK_NEG;
    }
  }

  for(int k=0;k<nr_token;k++){ //debug information
    printf("token_type: %d token_str: %s\n",tokens[k].type,tokens[k].str);
  }

  uint64_t result = eval(0,nr_token-1);

  for(int i=0;i<32;i++){
    tokens[i].type = -1;
    for(int j=0;j<32;j++){
      tokens[i].str[j] = '\0';
    }
  }

  return result;
}

uint64_t eval(int p,int q) {
  if (p > q) {
    /* Bad expression */
    printf("Bad expression!!!\n");
    assert(0);
  }
  else if (p == q) {
    if(tokens[p].type == TK_NUM){
      return (uint64_t)atoi(tokens[p].str);
    }else if(tokens[p].type == TK_HEX){
      uint64_t hexnum;
      sscanf(tokens[p].str,"0x%lx",&hexnum);
      return hexnum;
    }else if(tokens[p].type == TK_REG){
      bool success;
      uint64_t reg_num = isa_reg_str2val(tokens[p].str,&success);
      if(success){
        return reg_num;
      }else{
        printf("Cannot find reg name!!\n"); //当未找到相应的寄存器名字时，success指针值为false
        return 0;
      }
    }
    else{
      return -1;
    }

  }
  else if ( check_parentheses(p, q) == true ) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
  }
  else {
    int op = dominant_operator(p,q);
    if(tokens[op].type == TK_DEREF || tokens[op].type == TK_NEG){
      uint64_t val = eval(op + 1,q);

      switch (tokens[op].type)
      {
      case TK_NEG: return -val;
      case TK_DEREF: return paddr_read(val,4);
      default:assert(0);
      }

    }else{
      uint64_t val1 = eval(p, op - 1);
      uint64_t val2 = eval(op + 1, q);
      switch (tokens[op].type) 
      {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': return val1 / val2;
      case TK_AND: return val1 & val2;
      case TK_EQ: return val1 == val2;
      case TK_NEQ: return val1 != val2;
      default: assert(0);
      }
    }


  }
}


bool check_parentheses(int p,int q){
  if(tokens[p].type != TK_LB || tokens[q].type != TK_RB){
    return false; //式子不被左右括号包着，返回0
  }else{
    if(!check_all_bracket(p,q)){
      printf("bad expression with wrong bracket\n");
      assert(0);
    }else{
      if(check_is_bracket_match(p,q)){
        return true; //式中左右括号匹配，返回1
      }else{
        return false; //式子正确但是左右括号不匹配返回0
      }
    }
  }
}

bool check_all_bracket(int p,int q){  //此函数用来检测子串的括号使用是否合法并全部匹配,不匹配返回false
  int position = 0;
  for(int i=p;i<=q;i++){

    if(position<0){
      return false;
    }

    if(tokens[i].type == TK_LB){
      position++;
    }else if(tokens[i].type == TK_RB){
      position--;
    }
  }

  if(position == 0){
    return true;
  }else{
    return false;
  }

}

bool check_is_bracket_match(int p,int q){ //已经确定括号匹配，检查左右侧的括号是否是一对
  int position = 1;
  for(int i=p+1;i<=q;i++){
    
    if(position==0){
      return false;
    }

    if(tokens[i].type == TK_LB){
      position++;
    }else if(tokens[i].type == TK_RB){
      position--;
    }

  }

  if(position == 0){
    return true;
  }else{
    return false;
  }
}

int dominant_operator(int p,int q){ //找到表达式中的主运算符
  int op_array[32];
  int k = 0;
  for(int i=p;i<=q;i++){
    if(tokens[i].type == '+' || tokens[i].type == '-' || tokens[i].type == '*' || tokens[i].type == '/' || tokens[i].type == TK_AND 
    || tokens[i].type == TK_EQ || tokens[i].type == TK_NEQ || tokens[i].type == TK_DEREF || tokens[i].type == TK_NEG){
      op_array[k++] = i;
    }else if(tokens[i].type == TK_LB){
      i = find_matched_bra(i);
    }
  }
  for(int b=k-1;b>=0;b--){
    if(tokens[op_array[b]].type == TK_AND){
      return op_array[b];
    }
  }
  for(int a=k-1;a>=0;a--){
    if(tokens[op_array[a]].type == TK_EQ || tokens[op_array[a]].type == TK_NEQ){
      return op_array[a];
    }
  }
  for(int j=k-1;j>=0;j--){
    if(tokens[op_array[j]].type == '+' || tokens[op_array[j]].type == '-'){
      return op_array[j];
    }
  }
  for(int h=k-1;h>=0;h--){
    if(tokens[op_array[h]].type == '*' || tokens[op_array[h]].type == '/'){
      return op_array[h];
    }
  }
  for(int c=0;c<=k-1;c++){ //这里专门做成左边优先选择是因为负号的主符号应该在左侧
    if(tokens[op_array[c]].type == TK_DEREF || tokens[op_array[c]].type == TK_NEG){
      return op_array[c];
    }
  }

  return -1;
}

int find_matched_bra(int start){
  int flag = 1;
  for(int i=start+1;i<32;i++){
    if(flag == 0){
      return i-1; //返回右括号的位置
    }
    if(tokens[i].type == TK_LB){
      flag++;
    }else if(tokens[i].type == TK_RB){
      flag--;
    }
  }
  return -1;
}