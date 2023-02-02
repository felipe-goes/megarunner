#include <genesis.h>
#include <resources.h>
#include <string.h>

#define ANIM_RUN 0
#define ANIM_JUMP 1

#define GROUND_VRAM 1
#define UNDERGROUND_VRAM 2
#define LIGHT_VRAM 3

/*General stuff*/
const char msg_start[22] = "Press START to Begin!\0";
const char msg_reset[22] = "Press START to Reset!\0";
const char label_score[6] = "SCORE\0";
char str_score[3] = "0";
bool game_on = FALSE;
bool score_added = FALSE;
int score = 0;

// Game data control
const int scrollspeed = 2;
const int floor_height = 128;
fix16 gravity = FIX16(.2);

// Player
Sprite *player;
const int player_x = 32;
fix16 player_y = FIX16(112);
fix16 player_vel_y = FIX16(0);
int player_height = 16;
bool jumping = FALSE;

// Obstacle
Sprite *obstacle;
int obstacle_x = 320;
int obstacle_vel_x = 0;

// Text
void showText(const char s[]) { VDP_drawText(s, 20 - strlen(s) / 2, 10); }
void clearText() { VDP_clearText(0, 10, 32); }
void updateScoreDisplay() {
  sprintf(str_score, "%d", score);
  VDP_clearText(10, 2, 3);
  VDP_drawText(str_score, 10, 2);
}

// Game start and end
void startGame() {
  VDP_drawText(label_score, 10, 1);
  score = 0;
  updateScoreDisplay();
  obstacle_x = 320;

  if (game_on == FALSE) {
    game_on = TRUE;
    clearText();
  }
}

void endGame() {
  if (game_on == TRUE) {
    showText(msg_reset);
    game_on = FALSE;
  }
}

// Joystick Handler
void myJoyHandler(u16 joy, u16 changed, u16 state) {
  if (joy == JOY_1) {
    /*Start game if START is pressed*/
    if (state & BUTTON_START) {
      if (game_on == FALSE) {
        startGame();
      }
    }
    if (state & BUTTON_C) {
      // Make player jump
      if (jumping == FALSE) {
        jumping = TRUE;
        player_vel_y = FIX16(-3);
        SPR_setAnim(player, ANIM_JUMP);
      }
    }
  }
}

int main() {
  // Initi system
  JOY_init();
  JOY_setEventHandler(&myJoyHandler);
  SPR_init();

  // Set plane size
  VDP_setPlanSize(32, 32);
  // Set scrolling mode
  // VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE); // Without parallax
  VDP_setScrollingMode(HSCROLL_TILE, VSCROLL_PLANE);
  // Set background color
  VDP_setPaletteColor(0, RGB24_TO_VDPCOLOR(0x6dc2ca));

  // Load tiles
  VDP_loadTileSet(floort.tileset, 1, DMA);
  VDP_loadTileSet(wall.tileset, 2, DMA);
  VDP_loadTileSet(light.tileset, 3, DMA);

  // Grab palette
  VDP_setPalette(PAL1, light.palette->data);
  VDP_setPalette(PAL2, runner.palette->data);

  // Add the floor
  VDP_fillTileMapRect(BG_B, TILE_ATTR_FULL(PAL1, 0, FALSE, FALSE, GROUND_VRAM),
                      0, 16, 32, 1);
  VDP_fillTileMapRect(BG_B,
                      TILE_ATTR_FULL(PAL1, 0, FALSE, TRUE, UNDERGROUND_VRAM), 0,
                      17, 32, 14);

  // Add the front floor
  VDP_fillTileMapRect(BG_B, TILE_ATTR_FULL(PAL1, 0, FALSE, FALSE, GROUND_VRAM),
                      0, 19, 32, 1);
  VDP_fillTileMapRect(BG_B,
                      TILE_ATTR_FULL(PAL1, 0, FALSE, TRUE, UNDERGROUND_VRAM), 0,
                      20, 32, 8);

  // Add light image
  VDP_fillTileMapRectInc(
      BG_B, TILE_ATTR_FULL(PAL1, 0, FALSE, FALSE, LIGHT_VRAM), 15, 13, 2, 3);

  showText(msg_start);

  // Add player and obstacle
  player = SPR_addSprite(&runner, player_x, fix16ToInt(player_y),
                         TILE_ATTR(PAL2, 0, FALSE, FALSE));
  SPR_setAnim(player, ANIM_RUN);
  obstacle =
      SPR_addSprite(&rock, obstacle_x, 128, TILE_ATTR(PAL2, 0, FALSE, FALSE));

  // Game loop
  // int offset = 0; // Used without parallax
  s16 scrollValues[15] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int i = 0;
  while (1) {
    // Used without parallax
    // VDP_setHorizontalScroll(BG_B, offset -= scrollspeed);
    // if (offset <= -256)
    //   offset = 0;
    VDP_setHorizontalScrollTile(BG_B, 13, scrollValues, 15, CPU);
    for (i = 0; i < 15; i++) {
      if (i <= 5) {
        scrollValues[i] -= scrollspeed;
      } else {
        scrollValues[i] -= (scrollspeed + 1);
      }
      if (scrollValues[i] <= -256)
        scrollValues[i] = 0;
    }

    // Apply velocity and gravity to the player
    player_y = fix16Add(player_y, player_vel_y);
    if (jumping == TRUE)
      player_vel_y = fix16Add(player_vel_y, gravity);

    // Check if player is on floor
    if (jumping == TRUE &&
        fix16ToInt(player_y) + player_height >= (floor_height)) {
      jumping = FALSE;
      player_vel_y = FIX16(0);
      player_y = intToFix16(floor_height - player_height);
      SPR_setAnim(player, ANIM_RUN);
      score_added = FALSE;
    }

    // Move the obstacle
    obstacle_vel_x = -scrollspeed;
    obstacle_x = obstacle_x + obstacle_vel_x;
    if (obstacle_x < -8)
      obstacle_x = 320;

    if (game_on == TRUE) {
      if (player_x < obstacle_x + 8 && player_x + 8 > obstacle_x) {
        if (jumping == FALSE) {
          endGame();
        } else {
          if (score_added == FALSE) {
            score++;
            updateScoreDisplay();
            score_added = TRUE;
          }
        }
      }

      // Update sprites
      SPR_setPosition(obstacle, obstacle_x, 120);
      SPR_setPosition(player, player_x, fix16ToInt(player_y));

      // Update Sprite Engine
      SPR_update();
    }
    SYS_doVBlankProcess();
  }
  return (0);
}
