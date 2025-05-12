#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"
#include "raymath.h"

#define STB_DS_IMPLEMENTATION
#include "../third-party/stb/stb_ds.h"

#define NOB_IMPLEMENTATION
#include "../nob.h"

#define SW 1920
#define SH 1080
#define TSCALE 0.02

#define INPUT_FONT_SIZE 100

typedef struct {
  Vector2 start;
  Vector2 end;
  float thickness;
  Color color
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
  CMD_COUNT
} Cmd;

typedef enum {
  CAT_NONE,
  CAT_INT,
  CAT_COLOR,
  CAT_COUNT
} CmdArgType;

typedef struct {
  const char* fullName;
  const char* shortName;
  Cmd cmd;
  char* arg;
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
} CmdBuff;

typedef struct {
  CmdBuff* items;
  size_t count;
  size_t capacity;
} Commands;

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

Color LookupColor(char* value) {
  Colors colors = GetColors();
  value = ucase(value);
  for (size_t i = 0; i < colors.count; ++i) {
    if (strcmp(colors.items[i].name, value) == 0)
        return colors.items[i].color;
  }
  return ColorAlpha(WHITE, 0);
}

CmdBuff CreateCmdBuff(size_t count, Nob_String_View sv, const char* errorMsg) {
  char countStr[3];
  snprintf(countStr, sizeof(countStr), "%02x", count);
  Nob_String_Builder nsb = {0};
  nob_sb_appendf(&nsb, "%s: %s", countStr, ucase(nob_temp_sv_to_cstr(sv)));
  Color color = WHITE;
  bool hasError = strlen(errorMsg) > 0;
  if (hasError) {
    color = RED;
    nob_sb_appendf(&nsb, ": %s", errorMsg);
  }
  Nob_String_View nsv = nob_sb_to_sv(nsb);
  CmdBuff cb = { nsv, color };
  return cb;
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

void DrawTurtle(Turtle t) {
  DrawCircleV(t.position, t.size, t.pen.color);
  Vector2 end = GetEnd(t.position, t.rotation, t.size/2);
  DrawCircleV(end, t.size/2, ORANGE);
  DrawCircleV(GetEnd(t.position, t.rotation, t.size*0.8), t.size/6, BLACK);
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

bool UpdateTurtle(Turtle* t, TurtleCmd* cmd) {
  float amt = 0;
  Color color = WHITE;
  if (cmd->argType == CAT_INT) {
    amt = atof(cmd->arg);
    if (amt == 0.0f)
      return false;
  } else if (cmd->argType == CAT_COLOR) {
    color = LookupColor(cmd->arg);
    if (color.a == 0)
      return false;
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
  }
  return true;
}

int main(void) {
  InitWindow(SW, SH, "turtle");

  Font space = LoadFont("./assets/fonts/Space_Mono/SpaceMono-Regular.ttf");

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

  TLines lines = {0};

  Pen tpen = { 
    .down = false,
    .color = LIME,
    .width = 5
  };

  Turtle turtle = {
    .position = CLITERAL(Vector2) { .x = SW / 2, .y = SH / 2 },
    .rotation = 0,//d2r(180.0f),
    .size = 30,
    .pen = tpen,
    .lines = lines
  };

  Commands commands = {0};
  size_t commandsCounter = 0;
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
      while (key > 0)
      {
          if ((key >= 32) && (key <= 125)) 
              nob_da_append(&inputText, (char)key);

          key = GetCharPressed();  // Check next character in the queue
      }

      if (IsKeyPressed(KEY_BACKSPACE))
      {
        inputText.count--;
      } 
      else if (IsKeyPressed(KEY_ENTER)) {
        Nob_String_View text = nob_sb_to_sv(inputText);
        Nob_String_View leftCmd = nob_sv_chop_by_delim(&text, ' ');
        tc = GetCmd(cmds, leftCmd);
        if (tc) {
          if (tc->argRequired) {
            if (text.count == 0) {
              nob_log(NOB_ERROR, "No argument provided: "SV_Fmt"???", SV_Arg(leftCmd));
              CmdBuff cb = CreateCmdBuff(commandsCounter++, nob_sb_to_sv(inputText), "no arg");
              nob_da_append(&commands, cb);
              tc = NULL;
            } else {
              nob_log(NOB_INFO, SV_Fmt, SV_Arg(text));
              CmdBuff cb = CreateCmdBuff(commandsCounter++, nob_sb_to_sv(inputText), "");
              nob_da_append(&commands, cb);
              tc->arg = nob_temp_sv_to_cstr(text);
            }
          } else {
            nob_log(NOB_INFO, SV_Fmt, SV_Arg(leftCmd));
            CmdBuff cb = CreateCmdBuff(commandsCounter++, nob_sb_to_sv(inputText), "");
            nob_da_append(&commands, cb);
          }
        } else {
          nob_log(NOB_ERROR, "Invalid cmd: "SV_Fmt, SV_Arg(leftCmd));
          CmdBuff cb = CreateCmdBuff(commandsCounter++, nob_sb_to_sv(inputText), "invalid cmd");
          nob_da_append(&commands, cb);
          tc = NULL;
        }
        inputText.count = 0;
      }
    }

    if (tc) {
      if (!UpdateTurtle(&turtle, tc)) {
        CmdBuff cb = CreateCmdBuff(commandsCounter-1, nob_sv_from_cstr(tc->arg), "invalid arg");
        nob_da_append(&commands, cb);
      }
      tc = NULL;
    }

    for (size_t i = 0; i < turtle.lines.count; ++i) {
      TLine line = turtle.lines.items[i];
      DrawLineEx(line.start, line.end, line.thickness, line.color);
    }

    DrawTurtle(turtle);

    Nob_String_View sv = nob_sb_to_sv(inputText);
    DrawTextEx(space, nob_temp_sv_to_cstr(sv), inputBoxPos, INPUT_FONT_SIZE, 0, WHITE);

    DrawLine(5, 140, 500, 140, WHITE);

    size_t y = inputBoxPos.y + INPUT_FONT_SIZE*1.5;
    for (int i = commands.count-1; i >= 0; i--) {
      CmdBuff cb = commands.items[i];
      Vector2 pos = { .x = 10, .y = y };
      DrawTextEx(space, nob_temp_sv_to_cstr(cb.text), pos, INPUT_FONT_SIZE*0.6, 0, cb.color);
      y += INPUT_FONT_SIZE*0.6;
      if (y >= SH) break;
    }

    EndDrawing();
  }

  UnloadFont(space);
  CloseWindow();
}

