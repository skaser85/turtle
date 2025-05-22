#include <stdio.h>

#define NOB_IMPLEMENTATION
#include "../nob.h"

typedef struct {
  const char* cmd;
  void* arg;
} TC;

typedef struct {
  TC* items;
  size_t capacity;
  size_t count;
} Cmds;

void eat_until(char* text, char what, char* buff) {
  size_t i = 0;
  while (text[i]) {
    if (text[i] == what)
      break;
    buff[i] = text[i];
    i++;
  }
}

int main (void) {
  
  char *text = "fd 100";

  Cmds cmds = {0};

  char buff = eat_until(text, ' ', 20);

  //size_t i = 0;
  //while(text[i]) {
  //  nob_log(NOB_INFO, "%c", text[i]);
  //  if (text[i] == ' ')
  //    break;
  //  buff[i] = text[i];
  //  i++;
  //}

  nob_log(NOB_INFO, "%s", buff);
}
