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


int main(void) {
  
  //const char* cmd = "rp 5 [fd 100, rt 45]";
  const char* cmd = "fd 100";

  Cmds cmds = {0};

  Nob_String_View cmdSV = nob_sv_from_cstr(cmd);
  Nob_String_View left = nob_sv_chop_by_delim(&cmdSV, ' ');

  nob_log(NOB_INFO, "left : "SV_Fmt, SV_Arg(left));
  nob_log(NOB_INFO, "cmdSV: "SV_Fmt, SV_Arg(cmdSV));

  TC tc = { nob_temp_sv_to_cstr(left), NULL };

  Nob_String_View argSV;
  if (cmdSV.count > 0) {
    if (nob_sv_end_with(cmdSV, ",")) {
      argSV = nob_sv_chop_by_delim(&cmdSV, ',');
    } else if (nob_sv_end_with(cmdSV, "]")) {
      argSV = nob_sv_chop_by_delim(&cmdSV, ']');
    } else {
      argSV = cmdSV;
      cmdSV.count = 0;
    }
    nob_log(NOB_INFO, "arg: "SV_Fmt, SV_Arg(argSV));
    nob_log(NOB_INFO, "cmdSV: "SV_Fmt, SV_Arg(cmdSV));
  }

  const char* arg;
  if (argSV.count > 0) {
    arg = nob_temp_sv_to_cstr(argSV); 
    nob_log(NOB_INFO, "%s", arg);
    tc.arg = &arg;
  }

  nob_da_append(&cmds, tc);

  for (size_t i = 0; i < cmds.count; ++i) {
    nob_log(NOB_INFO, "%s: "SV_Fmt, cmds.items[i].cmd, SV_Arg(nob_sv_from_cstr((const char*)cmds.items[i].arg)));
  }

  return 0;
}
