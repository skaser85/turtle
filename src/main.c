#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"
#include "raymath.h"

#define STB_DS_IMPLEMENTATION
#include "../third-party/stb/stb_ds.h"

#define NOB_IMPLEMENTATION
#include "../nob.h"

#if 0
#define SW 1920
#define SH 1080
#define TSCALE 0.02
#define INPUT_FONT_SIZE 100
#else
#define SW 1600
#define SH 960
#define TSCALE 0.1
#define INPUT_FONT_SIZE 50 
#endif

#define MAX_INPUT_CHARS_COUNT 50


typedef struct {
  Vector2 start;
  Vector2 end;
  float thickness;
  Color color;
} TLine;

typedef struct {
  TLine* items;
  size_t capacity;
  size_t count;
} TLines;

typedef struct {
  bool down;
  Color color;
  int width;
} Pen;

typedef struct {
  Vector2 position;
  float rotation;
  float size;
  Pen pen;
  TLines lines;
} Turtle;

typedef enum {
  CMD_FD,
  CMD_BK,
  CMD_LT,
  CMD_RT,
  CMD_H,
  CMD_CS,
  CMD_PD,
  CMD_PU,
  CMD_SETPC,
  CMD_SETBG,
  CMD_RP,
  CMD_COUNT
} Cmd;

typedef enum {
  CAT_NONE,
  CAT_INT,
  CAT_COLOR,
  CAT_TEXT,
  CAT_COUNT
} CmdArgType;

typedef struct {
  const char* fullName;
  const char* shortName;
  Cmd cmd;
  const char* arg;
  bool argRequired;
  CmdArgType argType;
} TurtleCmd;

typedef struct {
  TurtleCmd* items;
  size_t count;
  size_t capacity;
} TurtleCmds;

typedef struct {
  Nob_String_View text;
  Color color;
} CmdHistoryEntry;

typedef struct {
  CmdHistoryEntry* items;
  size_t count;
  size_t capacity;
  size_t counter;
} CmdHistory;

typedef struct {
  const char* name;
  Color color;
} ColorItem;

char* ucase(const char *str) {
  // Allocate memory for the new string
  size_t len = strlen(str);
  char *upperStr = (char *)malloc(len + 1); // +1 for the null terminator
  if (upperStr == NULL) {
      return NULL; // Handle memory allocation failure
  }

  // Convert to uppercase
  for (size_t i = 0; i < len; i++) {
      upperStr[i] = toupper((unsigned char)str[i]);
  }
  upperStr[len] = '\0'; // Null-terminate the new string

  return upperStr; // Return the new string
}

typedef struct {
  ColorItem *items;
  size_t capacity;
  size_t count;
} Colors;

Colors GetColors() {
  Colors colors = {0};
  nob_da_append(&colors, ((ColorItem) { "LIGHTGRAY", LIGHTGRAY}));
  nob_da_append(&colors, ((ColorItem) { "GRAY", GRAY }));
  nob_da_append(&colors, ((ColorItem) { "DARKGRAY", DARKGRAY }));
  nob_da_append(&colors, ((ColorItem) { "YELLOW", YELLOW })); 
  nob_da_append(&colors, ((ColorItem) { "GOLD", GOLD }));
  nob_da_append(&colors, ((ColorItem) { "ORANGE", ORANGE }));
  nob_da_append(&colors, ((ColorItem) { "PINK", PINK }));
  nob_da_append(&colors, ((ColorItem) { "RED", RED }));
  nob_da_append(&colors, ((ColorItem) { "MAROON", MAROON }));
  nob_da_append(&colors, ((ColorItem) { "GREEN", GREEN }));
  nob_da_append(&colors, ((ColorItem) { "LIME", LIME }));
  nob_da_append(&colors, ((ColorItem) { "DARKGREEN", DARKGREEN }));
  nob_da_append(&colors, ((ColorItem) { "SKYBLUE", SKYBLUE }));
  nob_da_append(&colors, ((ColorItem) { "BLUE", BLUE }));
  nob_da_append(&colors, ((ColorItem) { "DARKBLUE", DARKBLUE }));
  nob_da_append(&colors, ((ColorItem) { "PURPLE", PURPLE }));
  nob_da_append(&colors, ((ColorItem) { "VIOLET", VIOLET }));
  nob_da_append(&colors, ((ColorItem) { "DARKPURPLE", DARKPURPLE }));
  nob_da_append(&colors, ((ColorItem) { "BEIGE", BEIGE }));
  nob_da_append(&colors, ((ColorItem) { "BROWN", BROWN }));
  nob_da_append(&colors, ((ColorItem) { "DARKBROWN", DARKBROWN }));
  nob_da_append(&colors, ((ColorItem) { "WHITE", WHITE }));
  nob_da_append(&colors, ((ColorItem) { "BLACK", BLACK }));
  nob_da_append(&colors, ((ColorItem) { "BLANK", BLANK }));
  nob_da_append(&colors, ((ColorItem) { "MAGENTA", MAGENTA }));
  nob_da_append(&colors, ((ColorItem) { "RAYWHITE", RAYWHITE }));
  return colors;
}

Color LookupColor(const char* value) {
  Colors colors = GetColors();
  value = ucase(value);
  for (size_t i = 0; i < colors.count; ++i) {
    if (strcmp(colors.items[i].name, value) == 0)
        return colors.items[i].color;
  }
  return ColorAlpha(WHITE, 0);
}

CmdHistoryEntry CreateCmdHistoryEntry(size_t count, Nob_String_View sv, const char* errorMsg) {
  char countStr[3];
  snprintf(countStr, sizeof(countStr), "%02lx", count);
  Nob_String_Builder nsb = {0};
  nob_sb_appendf(&nsb, "%s: %s", countStr, ucase(nob_temp_sv_to_cstr(sv)));
  Color color = WHITE;
  bool hasError = strlen(errorMsg) > 0;
  if (hasError) {
    color = RED;
    nob_sb_appendf(&nsb, ": %s", errorMsg);
  }
  Nob_String_View nsv = nob_sb_to_sv(nsb);
  CmdHistoryEntry ch = { nsv, color };
  return ch;
}

float d2r(float degrees) {
  return degrees * (PI / 180.0f);
}

Vector2 GetEnd(Vector2 v, float r, float len) {
  Vector2 end = (Vector2) {
    .x = v.x + len * cosf(r),
    .y = v.y + len * sinf(r)
  };
  return end;
}

void DrawTurtle(Turtle t, Font font) {
  DrawCircleV(t.position, t.size, t.pen.color);
  Vector2 end = GetEnd(t.position, t.rotation, t.size/2);
  DrawCircleV(end, t.size/2, ORANGE);
  DrawCircleV(GetEnd(t.position, t.rotation, t.size*0.8), t.size/6, BLACK);
  DrawCircleV(t.position, t.size/8, BLACK);
  const char* text = t.pen.down ? "PD" : "PU";
  int fontSize = 12;
  Vector2 m = MeasureTextEx(font, text, fontSize, 1);
  Vector2 c = { .x = t.position.x - (m.x/2), .y = t.position.y - (fontSize/2) };
  DrawTextEx(font, text, c, fontSize, 1, WHITE);
}

void InsertCmd(TurtleCmds *cmds, const char* fullName, const char* shortName, bool argRequired, Cmd cmd, CmdArgType catType) {
  TurtleCmd tc = { fullName, shortName, cmd, NULL, argRequired, catType };
  nob_da_append(cmds, tc);
}

TurtleCmd* GetCmd(TurtleCmds cmds, Nob_String_View cmdText) {
  char* ct = ucase(nob_temp_sv_to_cstr(cmdText));
  for (size_t i = 0; i < cmds.count; ++i) {
    TurtleCmd cmd = cmds.items[i];
    char* fn = ucase(cmd.fullName);
    char* sn = ucase(cmd.shortName);
    if (strcmp(ct, ucase(fn)) == 0 || strcmp(ct, ucase(sn)) == 0)
        return &cmds.items[i];
  }
  return NULL;
}

bool ValidateArg(TurtleCmd* cmd) {
  float amt = 0;
  Color color = WHITE;
  Nob_String_Builder sb = {0};
  if (cmd->argType == CAT_INT) {
    amt = atof(cmd->arg);
    if (amt == 0.0f)
      return false;
  } else if (cmd->argType == CAT_COLOR) {
    color = LookupColor(cmd->arg);
    if (color.a == 0)
      return false;
  } else if (cmd->argType == CAT_TEXT) {
    if (!nob_sb_appendf(&sb, "%s", cmd->arg)) {
      return false;
    }
  }
  return true;
}

bool UpdateTurtle(Turtle* t, TurtleCmd* cmd, TurtleCmds commands) {
  float amt = 0;
  Color color = WHITE;
  Nob_String_Builder sb = {0};
  if (cmd->argType == CAT_INT) {
    amt = atof(cmd->arg);
    if (amt == 0.0f)
      return false;
  } else if (cmd->argType == CAT_COLOR) {
    color = LookupColor(cmd->arg);
    if (color.a == 0)
      return false;
  } else if (cmd->argType == CAT_TEXT) {
    if (!nob_sb_appendf(&sb, "%s", cmd->arg)) {
      return false;
    }
  }

  switch (cmd->cmd) {
    case CMD_H: t->position = (Vector2) { .x = SW / 2, .y = SH / 2 }; break;
    case CMD_CS: break;
    case CMD_SETBG: break;
    case CMD_SETPC: t->pen.color = color; break;
    case CMD_PD: t->pen.down = true; break;
    case CMD_PU: t->pen.down = false; break;
    case CMD_FD:
    case CMD_BK: {
      int size = cmd->cmd == CMD_FD ? amt : -amt; 
      Vector2 end = GetEnd(t->position, t->rotation, size);
      if (t->pen.down) {
        TLine line = { .start = t->position, .end = end, .thickness = 5, .color = t->pen.color };
        nob_da_append(&t->lines, line);
      }
      t->position = end;
      break;
    }
    case CMD_LT: t->rotation -= d2r(amt); break;
    case CMD_RT: t->rotation += d2r(amt); break;
    case CMD_RP: {
      Nob_String_View sv = nob_sv_from_cstr(cmd->arg);
      Nob_String_View rpt = nob_sv_chop_by_delim(&sv, ' '); 
      int rptCount = atoi(rpt.data);
      if (rptCount == 0) {
        nob_log(NOB_ERROR, "Invalid rptCount: "SV_Fmt, SV_Arg(rpt));
        return false;
      }
      nob_log(NOB_INFO, "rptCount: %d", rptCount);
      Nob_String_View next = nob_sv_chop_left(&sv, 1);
      if (!nob_sv_eq(next, nob_sv_from_cstr("["))) {
        nob_log(NOB_ERROR, "Invalid repeat start: "SV_Fmt, SV_Arg(next));
        return false;
      }
      TurtleCmds cmds = {0};
      next = nob_sv_chop_by_delim(&sv, ' ');
      TurtleCmd* tc = GetCmd(commands, next);
      while (tc) {
        if (!tc) {
          nob_log(NOB_ERROR, "Invalid cmd: "SV_Fmt, SV_Arg(next));
          return false;
        }
        if (tc->argRequired) {
          next = nob_sv_chop_by_delim(&sv, ' ');
          tc->arg = nob_temp_sv_to_cstr(next);
          if (!ValidateArg(tc)) {
            nob_log(NOB_ERROR, "Invalid arg: "SV_Fmt, SV_Arg(next));
            return false;
          }
        }
        nob_da_append(&cmds, *tc);
        next = nob_sv_chop_by_delim(&sv, ' ');
        if (nob_sv_eq(next, nob_sv_from_cstr("]"))) {
          nob_log(NOB_INFO, "end of the rp!");
          return true;
        }
        tc = GetCmd(commands, next);
        nob_log(NOB_INFO, SV_Fmt, SV_Arg(sv));
      }
      for (int i = 0; i < rptCount; ++i) {
        for (size_t c = 0; c < cmds.count; ++c) {
          if (!UpdateTurtle(t, &cmds.items[i], commands)) {
            return false;
          }
        }
      }
      return true;
    } break;
    case CMD_COUNT: break;
  }
  return true;
}

TurtleCmd* ParseCommandText(Nob_String_View cmdText, TurtleCmds commands, CmdHistory* history) {
  Nob_String_View leftCmd = nob_sv_chop_by_delim(&cmdText, ' ');
  TurtleCmd* tc = GetCmd(commands, leftCmd);
  if (tc) {
    if (tc->argRequired) {
      if (cmdText.count == 0) {
        nob_log(NOB_ERROR, "No argument provided: "SV_Fmt"???", SV_Arg(leftCmd));
        CmdHistoryEntry ch = CreateCmdHistoryEntry(history->counter++, cmdText, "no arg");
        nob_da_append(history, ch);
        return NULL;
      } else {
        nob_log(NOB_INFO, SV_Fmt, SV_Arg(cmdText));
        CmdHistoryEntry ch = CreateCmdHistoryEntry(history->counter++, cmdText, "");
        nob_da_append(history, ch);
        tc->arg = cmdText.data;
        return tc;
      }
    } else {
      nob_log(NOB_INFO, SV_Fmt, SV_Arg(leftCmd));
      tc->arg = NULL;
      CmdHistoryEntry ch = CreateCmdHistoryEntry(history->counter++, cmdText, "");
      nob_da_append(history, ch);
      return tc;
    }
  } else {
    nob_log(NOB_ERROR, "Invalid cmd: "SV_Fmt, SV_Arg(leftCmd));
    CmdHistoryEntry ch = CreateCmdHistoryEntry(history->counter++, cmdText, "invalid cmd");
    nob_da_append(history, ch);
    return NULL;
  }
}

int main(void) {
  InitWindow(SW, SH, "turtle");

  Font space12 = LoadFontEx("./assets/fonts/spaceInputFontSize_Mono/spaceInputFontSizeMono-Regular.ttf", 12, NULL, 0);
  SetTextureFilter(space12.texture, TEXTURE_FILTER_BILINEAR);

  Font spaceInputFontSize = LoadFontEx("./assets/fonts/spaceInputFontSize_Mono/spaceInputFontSizeMono-Regular.ttf", INPUT_FONT_SIZE, NULL, 0);
  SetTextureFilter(spaceInputFontSize.texture, TEXTURE_FILTER_BILINEAR);
  
  TurtleCmds cmds = {0};
  InsertCmd(&cmds, "Forward", "FD", true, CMD_FD, CAT_INT);
  InsertCmd(&cmds, "Back", "BK", true, CMD_BK, CAT_INT);
  InsertCmd(&cmds, "Left", "LT", true, CMD_LT, CAT_INT);
  InsertCmd(&cmds, "Right", "RT", true, CMD_RT, CAT_INT);
  InsertCmd(&cmds, "ClearScreen", "CS", false, CMD_CS, CAT_NONE);
  InsertCmd(&cmds, "Home", "H", false, CMD_H, CAT_NONE);
  InsertCmd(&cmds, "PenDown", "PD", false, CMD_PD, CAT_NONE);
  InsertCmd(&cmds, "PenUp", "PU", false, CMD_PU, CAT_NONE);
  InsertCmd(&cmds, "SetPenColor", "SETPC", true, CMD_SETPC, CAT_COLOR);
  InsertCmd(&cmds, "SetBackground", "SETBG", true, CMD_SETBG, CAT_COLOR);
  InsertCmd(&cmds, "Repeat", "RP", true, CMD_RP, CAT_TEXT);

  TLines lines = {0};

  Pen tpen = { 
    .down = false,
    .color = LIME,
    .width = 5
  };

  Turtle turtle = {
    .position = CLITERAL(Vector2) { .x = SW / 2, .y = SH / 2 },
    .rotation = 0,
    .size = 30,
    .pen = tpen,
    .lines = lines
  };

  CmdHistory cmdHistory = {0};
  TurtleCmd* tc = NULL;

  Nob_String_Builder inputText = {0};
  Vector2 inputBoxPos = { .x = 20, .y = 20};

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(GetColor(0x181818FF));

    float degrees = 0.1;
    float speed = 0.1;
    if (IsKeyDown(KEY_LEFT)) {
      turtle.rotation -= d2r(degrees);
    } else if (IsKeyDown(KEY_RIGHT)) {
      turtle.rotation += d2r(degrees);
    } else if (IsKeyDown(KEY_UP)) {
      Vector2 end = GetEnd(turtle.position, turtle.rotation, 100);
      turtle.position = Vector2MoveTowards(turtle.position, end, speed);
    } else {
      // Get char pressed (unicode character) on the queue
      int key = GetCharPressed();

      // Check if more characters have been pressed on the same frame
      while (key > 0 && inputText.count < MAX_INPUT_CHARS_COUNT) {
          if ((key >= 32) && (key <= 125)) 
              nob_da_append(&inputText, (char)key);

          key = GetCharPressed();  // Check next character in the queue
      }

      if (IsKeyPressed(KEY_BACKSPACE)) {
        inputText.count--;
      } 
      else if (IsKeyPressed(KEY_ENTER)) {
        Nob_String_View text = nob_sb_to_sv(inputText);
        tc = ParseCommandText(text, cmds, &cmdHistory);  
        inputText.count = 0;
      }
    }

    if (tc) {
      if (!UpdateTurtle(&turtle, tc, cmds)) {
        CmdHistoryEntry ch = CreateCmdHistoryEntry(cmdHistory.counter-1, nob_sv_from_cstr(tc->arg), "invalid arg");
        nob_da_append(&cmdHistory, ch);
      }
      tc = NULL;
    }

    for (size_t i = 0; i < turtle.lines.count; ++i) {
      TLine line = turtle.lines.items[i];
      DrawLineEx(line.start, line.end, line.thickness, line.color);
    }

    DrawTurtle(turtle, space12);

    Nob_String_View sv = nob_sb_to_sv(inputText);
    const char* _text = (char*)nob_temp_sv_to_cstr(sv);
    DrawTextEx(spaceInputFontSize, _text, inputBoxPos, INPUT_FONT_SIZE, 1, WHITE);

    DrawLine(5, INPUT_FONT_SIZE*1.3, 500, INPUT_FONT_SIZE*1.3, WHITE);

    size_t y = inputBoxPos.y + INPUT_FONT_SIZE*1.5;
    for (int i = cmdHistory.count-1; i >= 0; i--) {
      CmdHistoryEntry ch = cmdHistory.items[i];
      Vector2 pos = { .x = 10, .y = y };
      char* _text = (char*)nob_temp_sv_to_cstr(ch.text);
      DrawTextEx(spaceInputFontSize, _text, pos, INPUT_FONT_SIZE*0.6, 1, ch.color);
      y += INPUT_FONT_SIZE*0.6;
      if (y >= SH) break;
    }

    EndDrawing();

    nob_temp_reset();
  }

  UnloadFont(space12);
  UnloadFont(spaceInputFontSize);
  CloseWindow();
}

