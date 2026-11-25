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
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <memory/vaddr.h>
#include <debug.h>
extern void isa_reg_display(void);

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");   //dynamic memory alloocation   free

  if (line_read && *line_read) {
    add_history(line_read);
  }
  return line_read;
}
static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}
static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;//4
  return -1;
}

static int cmd_si(char *args) {
  char *arg = strtok(NULL, "\n");  
  int NR=0;
if (arg == NULL) { NR=1;}
else {int length=strlen(arg); //"499"\0  //"7"\0
    for(int i=0;i<=length-1;i++){
     int nr=(int)((*(arg+i))-'0');
     NR=NR*10+nr;  //^ not m方
     }}
    cpu_exec(NR); 
//NR>=10,only put:nemu: HIT GOOD TRAP at pc = 0x8000000c
//NR==0,dnt exec hhh
   return 0;
}

static int cmd_info(char *args) {
  char *arg = strtok(NULL, "\n");  
  if (arg == NULL) {
   printf("info r : Print register status \n");
   printf("info w : Print watchpoint information \n");   
   return 0;
  }
  else {
 if(strcmp(arg,"r")==0){ isa_reg_display();}
 else {printf("error: is not r\n");}
}
return 0;
}
static int cmd_x(char * args) {
  char *arg= strtok(NULL, " "); 
  int N=0;
  sscanf(arg,"%u",&N);
  char *nemu_virtual_gcc_addr = strtok(NULL," "); 
  //x指令我应该输入的是已经抽象了的虚拟内存地址
  //类似于(gdb) x /10x  0x7fffefbff010，这里的addr是实际gcc或C语言cause的虚拟地址，如&peme[2];
  //我不知道虚拟机的对应的实际paddr， is not it？
  vaddr_t addr=0;
  sscanf(nemu_virtual_gcc_addr,"%x",&addr);
  for(int i=0;i<N;i++) {
    word_t ret=vaddr_read(addr+4*i,4);
    if((4*i)%16==0) {
      printf("0x%08x 0x%08x      ",addr+4*i,ret);
      if(i==N-1){printf("\n");}
    }
    else if((4*i+4)%16==0) {
      printf("0x%08x\n",ret);
      if(i==N-1){printf("\n");}
    } 
    else if(i==N-1) {
      printf("0x%08x\n",ret);
    }
    else printf("0x%08x      ",ret);
  }
  return 0;
}

static int cmd_p(char *args){
  char *arg= strtok(NULL, " "); 
  return 0;
}
static int cmd_w(char *args) {
  char *arg= strtok(NULL, " "); 
  return 0;
}
static int cmd_d(char *args) {
  char *arg= strtok(NULL, " "); 
  return 0;
}
static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  {"si","Program single-step execution",cmd_si},
  {"info","Print register status and watchpoint information",cmd_info},
  {"x","Scan memory",cmd_x},
  {"p","Expression evaluation",cmd_p}, 
  {"w","Set monitoring points",cmd_w}, 
  {"d","'Delete monitoring points",cmd_d},

  /* TODO: Add more commands */

}; 

extern word_t(int p,int q,bool success); 
static  static int cmd_p(char *args) {
 printf("Expression evaluation\n");
 strtok("")
}

#define NR_CMD ARRLEN(cmd_table)
//static int cmd_help(char *args);

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");  //state_keeping function,continue to deal with str(char *)
  int i;
  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}
void sdb_set_batch_mode() {
  is_batch_mode = true;
}
void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
  }
  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }
#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif
    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }
    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
