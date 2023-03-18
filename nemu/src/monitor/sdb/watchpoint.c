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

#include "sdb.h"

#define NR_WP 32

static WP wp_pool[NR_WP] = {}; //static保证该类型不会被其他文件调用
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
WP* new_wp(char *ex){  //从free_的头部拿下一个监视点放在head的尾部
    assert(free_ != NULL);
    WP *p = free_;
    bool success = true;

    free_ = free_->next;
    p->next = NULL;
    strcpy(p->expr,ex);  //为该节点赋予表达式的值，并计算出结果存在节点中
    p->result = expr(p->expr,&success);
    assert(success);

    WP *q = head;
    if(!head){
      head = p;
    }else{
      while (q->next)
      {
        q = q->next;
      }
      q->next = p;
    }

    return p;
}

void free_wp(WP *wp){
  if(wp==head){
    head = head->next;
  }else{
    WP *q = head;
    while (q ->next != wp)
    {
      q = q->next;
    }
    q->next = wp->next;
  }
  wp->next = free_;
  free_ = wp;
}

int scan_watchpoint(void){ //如果监视点的表达式值都没有变的话返回-1,否则返回该监视点的NO值
  WP *flag = head;
  for(;flag!=NULL;flag = flag->next){
    bool success = true;
    uint64_t expr_result = expr(flag->expr,&success);
    assert(success);
    if(expr_result != flag->result){
      return flag->NO;
    }
  }
  return -1;
}

void watchpoint_display(void){
  WP *flag = head;
  for(;flag != NULL;flag = flag->next){
    printf("NO.%d watchpoint: expr = %s  result = %ld\n",flag->NO,flag->expr,flag->result);
  }
}

void delete_watchpoint(int N){
  WP *flag = head;
  while (flag->NO != N)
  {
    flag = flag->next;
  }
  free_wp(flag);
}