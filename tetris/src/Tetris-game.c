#include <SDL2/SDL.h>
#include <stdio.h>
#include <SDL2/SDL_ttf.h>
#include <time.h>
#include <SDL2/SDL_mixer.h>

#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 800
#define SIDE_BAR 240
#define BOARD_COLS 10
#define BOARD_ROWS 20
#define BLOCK_SIZE 4
#define CELL_SIZE (WINDOW_HEIGHT / BOARD_ROWS)

#define NEXT_X 11
#define NEXT_Y 4

#define WEIGHT_LANDING_HEIGHT     (-4.500158825082766)
#define WEIGHT_ROWS_ELIMINATED    (3.4181268101392694)
#define WEIGHT_ROW_TRANSITIONS    (-3.2178882868487753)
#define WEIGHT_COLUMN_TRANSITIONS (-9.348695305445199)
#define WEIGHT_HOLES              (-7.899265427351652)
#define WEIGHT_WELL_SUMS          (-3.3855972247263626)

#define EASY_MODE 1
#define NORMAL_MODE 2
#define HARD_MODE 3

/*
/usr/share/fonts/truetype/ubuntu/UbuntuMono-RI.ttf: Ubuntu Mono:style=Italic
*/

#define INIT_DELAY 500
#define ACCELERATE_SPEED 5

void print_board(int board[BOARD_ROWS][BOARD_COLS]) {
    for (int i = 0; i < BOARD_ROWS; i++) {
        for (int j = 0; j < BOARD_COLS; j++) {
            printf("%d ", board[i][j]);
        }
        printf("\n");
    }
}

typedef struct {
    SDL_Rect rect;
    SDL_Color bgColor;
    SDL_Color textColor;
    const char* label;
} Button;

typedef struct {
    int col;
    int angle;
    int landing_height;
    double score;
} PlaceState;

int board[BOARD_ROWS][BOARD_COLS] = {0};

typedef int Shape[BLOCK_SIZE][BLOCK_SIZE];

Shape shapes[7] = {
    // I
    {
        {0,0,0,0},
        {1,1,1,1},
        {0,0,0,0},
        {0,0,0,0}
    },
    // J
    {
        {2,0,0,0},
        {2,2,2,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // L
    {
        {0,0,3,0},
        {3,3,3,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // O
    {
        {4,4,0,0},
        {4,4,0,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // S
    {
        {0,5,5,0},
        {5,5,0,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // T
    {
        {0,6,0,0},
        {6,6,6,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // Z
    {
        {7,7,0,0},
        {0,7,7,0},
        {0,0,0,0},
        {0,0,0,0}
    }
};

SDL_Color colors[10] = {
    {0, 180, 190, 255},   // I
    {50, 90, 170, 255},     // J
    {230, 140, 50, 255},   // L
    {240, 220, 100, 255},   // O
    {120, 200, 100, 255},     // S
    {160, 110, 200, 255},   // T
    {220, 90, 100, 255},     // Z
    {0, 0, 0, 255},       //黑色
    {100, 100, 100, 100},     // sidebar和落下来的方块浅灰
    {225, 225, 225, 255}  //白色
};

Mix_Music *bgm = NULL;
Mix_Chunk *dropSound = NULL;
Mix_Chunk *clearSound = NULL;
Mix_Chunk *failureSound = NULL;

Shape* get_block(int index) {
    return &shapes[index];
}

void init(SDL_Window** win, SDL_Renderer** renderer) {
    SDL_Init(SDL_INIT_VIDEO);
    *win = SDL_CreateWindow("Tetris",
                            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            WINDOW_WIDTH + SIDE_BAR, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    *renderer = SDL_CreateRenderer(*win, -1, SDL_RENDERER_SOFTWARE);
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);  // 初始化音频系统
}

int isButtonClicked(Button *btn, int mouseX, int mouseY) {
    if (mouseX >= btn->rect.x && mouseX <= btn->rect.x + btn->rect.w &&
        mouseY >= btn->rect.y && mouseY <= btn->rect.y + btn->rect.h) {
        return 1;
    }
    return 0;
}

int show_start_window(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/ubuntu/UbuntuMono-RI.ttf", 48);
    TTF_Font* buttonFont = TTF_OpenFont("/usr/share/fonts/truetype/ubuntu/UbuntuMono-RI.ttf", 28);

    SDL_Color white = colors[9];
    SDL_Color gray = {100, 100, 100, 255};

    SDL_Surface *surface = TTF_RenderText_Blended(font, "Welcome to Tetris!", white);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_Rect dstrect;
    dstrect.w = surface->w;
    dstrect.h = surface->h;
    dstrect.x = (WINDOW_WIDTH + SIDE_BAR) / 2 - dstrect.w / 2;
    dstrect.y = 80;
    SDL_RenderCopy(renderer, texture, NULL, &dstrect);
    SDL_FreeSurface(surface);

    const char* modeLabels[] = {"Easy mode", "Normal mode", "Hard mode"};
    SDL_Rect modeRects[3];
    SDL_Texture* modeTextures[3];
    SDL_Color modeColors[3];
    int selected_mode = 1;

    for (int i = 0; i < 3; i++) {
        modeColors[i] = gray;
        surface = TTF_RenderText_Blended(buttonFont, modeLabels[i], modeColors[i]);
        modeTextures[i] = SDL_CreateTextureFromSurface(renderer, surface);
        modeRects[i].w = surface->w;
        modeRects[i].h = surface->h;
        modeRects[i].x = (WINDOW_WIDTH + SIDE_BAR) / 2 - modeRects[i].w / 2;
        modeRects[i].y = 300 + i * 60;
        SDL_FreeSurface(surface);
    }

    SDL_Color startColor = gray;
    SDL_Surface* startSurface = TTF_RenderText_Blended(buttonFont, "Start", startColor);
    SDL_Texture* startTexture = SDL_CreateTextureFromSurface(renderer, startSurface);
    SDL_Rect startRect = {
        .w = startSurface->w,
        .h = startSurface->h,
        .x = (WINDOW_WIDTH + SIDE_BAR) / 2 - startSurface->w / 2,
        .y = 400 + 4 * 60
    };
    SDL_FreeSurface(startSurface);

    SDL_Event e;
    int quit = 0;
    int mode_selected = 1;
    while (!quit) {
        int mx, my;
        SDL_GetMouseState(&mx, &my);

        for (int i = 0; i < 3; i++) {
            SDL_Color color = (mx >= modeRects[i].x && mx <= modeRects[i].x + modeRects[i].w &&
                               my >= modeRects[i].y && my <= modeRects[i].y + modeRects[i].h) || (mode_selected == i + 1)
                              ? white : gray;
            if (memcmp(&color, &modeColors[i], sizeof(SDL_Color)) != 0) {
                modeColors[i] = color;
                surface = TTF_RenderText_Blended(buttonFont, modeLabels[i], color);
                SDL_DestroyTexture(modeTextures[i]);
                modeTextures[i] = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_FreeSurface(surface);
            }
        }

        SDL_Color newStartColor = (mx >= startRect.x && mx <= startRect.x + startRect.w &&
                                   my >= startRect.y && my <= startRect.y + startRect.h) ? white : gray;
        if (memcmp(&newStartColor, &startColor, sizeof(SDL_Color)) != 0) {
            startColor = newStartColor;
            startSurface = TTF_RenderText_Blended(buttonFont, "Start", startColor);
            SDL_DestroyTexture(startTexture);
            startTexture = SDL_CreateTextureFromSurface(renderer, startSurface);
            SDL_FreeSurface(startSurface);
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, &dstrect);
        for (int i = 0; i < 3; i++) {
            SDL_RenderCopy(renderer, modeTextures[i], NULL, &modeRects[i]);
        }
        SDL_RenderCopy(renderer, startTexture, NULL, &startRect);
        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                exit(0);
            } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                for (int i = 0; i < 3; i++) {
                    Button tmp;
                    tmp.rect = modeRects[i];
                    if (isButtonClicked(&tmp, mx, my)) {
                        mode_selected = i + 1;
                    }
                }
                if (isButtonClicked(&(Button){.rect = startRect}, mx, my)) {
                    quit = 1;
                }
            }
        }
        SDL_Delay(50);
    }

    SDL_DestroyTexture(texture);
    for (int i = 0; i < 3; i++) SDL_DestroyTexture(modeTextures[i]);
    SDL_DestroyTexture(startTexture);
    TTF_CloseFont(font);
    TTF_CloseFont(buttonFont);

    return mode_selected;
}

void drawButton(SDL_Renderer* renderer, TTF_Font* font, Button btn) {
    // Draw background
    SDL_SetRenderDrawColor(renderer, btn.bgColor.r, btn.bgColor.g, btn.bgColor.b, btn.bgColor.a);
    SDL_RenderFillRect(renderer, &btn.rect);

    // Render label
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, btn.label, btn.textColor);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

    SDL_Rect textRect;
    textRect.w = textSurface->w;
    textRect.h = textSurface->h;
    textRect.x = btn.rect.x + (btn.rect.w - textRect.w) / 2;
    textRect.y = btn.rect.y + (btn.rect.h - textRect.h) / 2;

    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}

void initHelpButton(SDL_Renderer* renderer, Button *helpButton) {
    helpButton->rect.w = 100;
    helpButton->rect.h = 50;
    helpButton->rect.x = WINDOW_WIDTH + (SIDE_BAR - helpButton->rect.w) / 2;
    helpButton->rect.y = WINDOW_HEIGHT * 9 / 10 - helpButton->rect.h;
    helpButton->bgColor = colors[7];
    helpButton->textColor = colors[9];
    helpButton->label = "Help!";

    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/ubuntu/UbuntuMono-RI.ttf", 24);

    drawButton(renderer, font, *helpButton);
}

void init_sidebar(SDL_Renderer* renderer) {
    SDL_Rect sidebar = { WINDOW_WIDTH, 0, SIDE_BAR, WINDOW_HEIGHT };
    SDL_SetRenderDrawColor(renderer, colors[8].r, colors[8].g, colors[8].b, colors[8].a);
    SDL_RenderFillRect(renderer, &sidebar);
    SDL_RenderPresent(renderer);

    SDL_Init(SDL_INIT_VIDEO);
    if (TTF_Init() == -1) {
        printf("TTF_Init Error: %s\n", TTF_GetError());
        return;
    }
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/ubuntu/UbuntuMono-RI.ttf", 24);
    TTF_Font* font1 = TTF_OpenFont("/usr/share/fonts/truetype/ubuntu/UbuntuMono-RI.ttf", 16);
    if (!font) {
    printf("Failed to load font: %s\n", TTF_GetError());
    return;
    }

    SDL_Color color = colors[9];
    SDL_Surface* scoreSurface = TTF_RenderText_Blended(font, "Total Score: ", color);
    SDL_Surface* pieceSurface = TTF_RenderText_Blended(font, "Next Piece: ", color);
    SDL_Texture* scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface);
    SDL_Texture* pieceTexture = SDL_CreateTextureFromSurface(renderer, pieceSurface);
    SDL_Rect scoreRect, pieceRect;

    pieceRect.w = pieceSurface->w;
    pieceRect.h = pieceSurface->h;
    pieceRect.x = WINDOW_WIDTH + (SIDE_BAR - pieceRect.w) / 4;
    pieceRect.y = (WINDOW_HEIGHT - pieceRect.h) / 12;

    scoreRect.w = scoreSurface->w;
    scoreRect.h = scoreSurface->h;
    scoreRect.x = WINDOW_WIDTH + (SIDE_BAR - scoreRect.w) / 4;
    scoreRect.y = (WINDOW_HEIGHT - scoreRect.h) / 2;
    SDL_FreeSurface(scoreSurface);
    SDL_FreeSurface(pieceSurface);
    SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
    SDL_RenderCopy(renderer, pieceTexture, NULL, &pieceRect);

    char *instruction[] = {
    "Welcome to Tetris!",
    "Press'a' to rorate",
    "     '<-' to go left",
    "     '->' to go right",
    "     'down' to drop faster",
    "     'space' to stop",
    "     'h' to help",
    };
    int num_lines = sizeof(instruction) / sizeof(instruction[0]);

    for (int i = 0; i < num_lines; i++) {
        SDL_Surface* surface = TTF_RenderText_Solid(font1, instruction[i], color);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect instructionRect;
        instructionRect.w = surface->w;
        instructionRect.h = surface->h;
        instructionRect.x = WINDOW_WIDTH + (SIDE_BAR - scoreRect.w) / 4;
        instructionRect.y = (WINDOW_HEIGHT - scoreRect.h) / 2 + 100 + i * 25;
        SDL_RenderCopy(renderer, texture, NULL, &instructionRect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
}

void drawCell(SDL_Renderer* renderer, int x, int y, SDL_Color color) {
    SDL_Rect rect = { x * CELL_SIZE + 2, y * CELL_SIZE + 2, CELL_SIZE - 4, CELL_SIZE - 4 };
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

void drawShape(SDL_Renderer* renderer, Shape* shape, int startX, int startY, SDL_Color color) {
    for (int y = 0; y < BLOCK_SIZE; y++) {
        for (int x = 0; x < BLOCK_SIZE; x++) {
            if ((*shape)[y][x]) {
                drawCell(renderer, startX + x, startY + y, color);
            }
        }
    }
}

void drawBoard(SDL_Renderer* renderer) {
    SDL_Rect gameArea = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    SDL_SetRenderDrawColor(renderer, colors[7].r, colors[7].g, colors[7].b, colors[7].a);
    SDL_RenderFillRect(renderer, &gameArea);

    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; c++) {
            SDL_Color color = (board[r][c]) ? colors[board[r][c] - 1] : colors[7];
            drawCell(renderer, c, r, color);
        }
    }
}

int can_place(Shape* block, int board[BOARD_ROWS][BOARD_COLS], int row, int col) {
    for (int r = 0; r < BLOCK_SIZE; r++) {
        for (int c = 0; c < BLOCK_SIZE; c++) {
            if ((*block)[r][c]) {
                int br = row + r;
                int bc = col + c;
                if (br < 0 || br >= BOARD_ROWS || bc < 0 || bc >= BOARD_COLS) 
                    return 0;
                if (board[br][bc]) 
                    return 0;
            }
        }
    }
    return 1;
}

void rotate_shape(Shape* src, Shape* dst) {
    for (int r = 0; r < BLOCK_SIZE; r++) {
        for (int c = 0; c < BLOCK_SIZE; c++) {
            (*dst)[c][BLOCK_SIZE - 1 - r] = (*src)[r][c];
        }
    }
}

void copy_shape(Shape* src, Shape* dst) {
    for (int r = 0; r < BLOCK_SIZE; r++)
        for (int c = 0; c < BLOCK_SIZE; c++)
            (*dst)[r][c] = (*src)[r][c];
}

int find_landing_row(Shape *block, 
                    int board[BOARD_ROWS][BOARD_COLS],
                    int col) {
    int row = 0;
    while (row < BOARD_ROWS && can_place(block, board, row, col)) {
        row++;
    }
    return row - 1;
}

void place_block_at(Shape *block,
                    int board[BOARD_ROWS][BOARD_COLS],
                    int row,
                    int col) {
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            if ((*block)[i][j]) {
                board[row + i][col + j] = (*block)[i][j];
            }
        }
    }
}

int count_and_eliminate(int board[BOARD_ROWS][BOARD_COLS]) {
    int eliminated_lines = 0;
    for (int i = 0; i < BOARD_ROWS; i++) {
        int full_line = 1;
        for (int j = 0; j < BOARD_COLS; j++) {
            if (board[i][j] == 0) {
                full_line = 0;
                break;
            }
        }
        if (full_line) {
            eliminated_lines++;
            // 消行
            for (int k = i; k > 0; k--) {
                for (int j = 0; j < BOARD_COLS; j++) {
                    board[k][j] = board[k - 1][j];
                }
            }
            for (int j = 0; j < BOARD_COLS; j++) {
                board[0][j] = 0;
            }
        }
    }
    return eliminated_lines;
}

int score(int eliminated_lines) {
    int score;
    if (eliminated_lines == 0) {
        score = 0;
    }
    else if (eliminated_lines == 1) {
        score = 100;
    }
    else if (eliminated_lines == 2) {
        score = 300;
    }
    else if (eliminated_lines == 3) {
        score = 500;
    }
    else if (eliminated_lines == 4) {
        score = 800;
    }

    return score;
}

void updateScoreText(SDL_Renderer* renderer, int total_score)
{
    if (TTF_Init() == -1) {
        printf("TTF_Init Error: %s\n", TTF_GetError());
        return;
    }
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/ubuntu/UbuntuMono-RI.ttf", 24);
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return;
    }
    char scoreText[32];
    sprintf(scoreText, "%d", total_score);
    SDL_Color color = {colors[9].r, colors[9].g, colors[9].b};
    SDL_Surface* scoreSurface = TTF_RenderText_Blended(font, scoreText, color);
    SDL_Texture* scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface);
    SDL_Rect scoreRect;

    scoreRect.w = scoreSurface->w;
    scoreRect.h = scoreSurface->h;
    scoreRect.x = WINDOW_WIDTH + (SIDE_BAR - scoreRect.w) / 4;
    scoreRect.y = (WINDOW_HEIGHT - scoreRect.h) / 2 + 50;

    SDL_SetRenderDrawColor(renderer, colors[8].r, colors[8].g, colors[8].b, colors[8].a);
    SDL_RenderFillRect(renderer, &scoreRect);

    SDL_FreeSurface(scoreSurface);
    SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
}

//计算行高
void cal_line_height(int board[BOARD_ROWS][BOARD_COLS], int line_height[BOARD_COLS]) {
    for (int j = 0; j < BOARD_COLS; j++) {
        int height = 0;
        for (int i = 0; i < BOARD_ROWS; i++) {
            if (board[i][j]) {
                height = BOARD_ROWS - i;
                break;
            }
        }
        line_height[j] = height;
    }
}

//计算凹凸
int cal_concave(int board[BOARD_ROWS][BOARD_COLS], int line_height[BOARD_COLS]) {
    int total_bumpiness = 0;
    for (int j = 1; j < BOARD_COLS; j++) {
        int height_diff = abs(line_height[j] - line_height[j - 1]);
        total_bumpiness += height_diff;
    }

    //printf("总凹凸度: %d\n", total_bumpiness);
    return total_bumpiness;
}

//计算空洞
int cal_holes(int board[BOARD_ROWS][BOARD_COLS], int line_height[BOARD_COLS]) {
    int total_holes = 0;
    for (int j = 0; j < BOARD_COLS; j++) {
        for (int i = BOARD_ROWS - line_height[j] - 1; i < BOARD_ROWS; i++) {
            if (board[i][j] == 0) {
                total_holes++;
            }
        }
    }
    //printf("总空洞数: %d\n", total_holes);
    return total_holes;
}

//计算最大高度
int cal_max_height(int board[BOARD_ROWS][BOARD_COLS], int line_height[BOARD_COLS]) {
    int max_height = 0;
    for (int j = 0; j < BOARD_COLS; j++) {
        if (line_height[j] > max_height) {
            max_height = line_height[j];
        }
    }
    //printf("最大高度: %d\n", max_height);
    return max_height;
}

int cal_row_transitions(int board[BOARD_ROWS][BOARD_COLS]) {
    int transitions = 0;
    for (int i = 0; i < BOARD_ROWS; i++) {
        int last_cell = 1;
        for (int j = 0; j < BOARD_COLS; j++) {
            if (board[i][j] != last_cell) {
                transitions++;
            }
            last_cell = board[i][j];
        }
        if (last_cell == 0) transitions++;
    }
    return transitions;
}

int cal_column_transitions(int board[BOARD_ROWS][BOARD_COLS]) {
    int transitions = 0;
    for (int j = 0; j < BOARD_COLS; j++) {
        int last_cell = 1;
        for (int i = BOARD_ROWS-1; i >= 0; i--) {
            if (board[i][j] != last_cell) {
                transitions++;
            }
            last_cell = board[i][j];
        }
    }
    return transitions;
}

int cal_well_sums(int board[BOARD_ROWS][BOARD_COLS], int line_height[BOARD_COLS]) {
    int well_sums = 0;
    for (int j = 0; j < BOARD_COLS; j++) {
        int left_height = (j > 0) ? line_height[j-1] : BOARD_ROWS;
        int right_height = (j < BOARD_COLS-1) ? line_height[j+1] : BOARD_ROWS;
        int min_adjacent = (left_height < right_height) ? left_height : right_height;
        
        if (line_height[j] < min_adjacent) {
            well_sums += min_adjacent - line_height[j];
        }
    }
    return well_sums;
}

double evaluate_state_update(int board[BOARD_ROWS][BOARD_COLS],
    int line_height[BOARD_COLS],
    int landing_height,
    int eliminated_lines) {
    double score = 0.0;

    int holes = cal_holes(board, line_height);
    int row_transitions = cal_row_transitions(board);
    int col_transitions = cal_column_transitions(board);
    int well_sums = cal_well_sums(board, line_height);

    score += WEIGHT_LANDING_HEIGHT * landing_height;
    score += WEIGHT_HOLES * holes;
    score += WEIGHT_ROW_TRANSITIONS * row_transitions;
    score += WEIGHT_COLUMN_TRANSITIONS * col_transitions;
    score += WEIGHT_WELL_SUMS * well_sums;
    score += WEIGHT_ROWS_ELIMINATED * eliminated_lines;

    /*
    if (eliminated_lines == 1 && landing_height < 7) {
        score -= WEIGHT_ROWS_ELIMINATED * 6.3;
    }
*/
    return score;
}

void find_best_place(int board[BOARD_ROWS][BOARD_COLS], 
                     Shape *block, 
                     int line_height[BOARD_COLS], 
                     PlaceState *best_pos) {
    double best_score = -1e9;
    int best_col = 0;
    int best_angle = 0;

    Shape rotated;
    Shape temp_rotated;
    copy_shape(block, &rotated);
    int board_temp[BOARD_ROWS][BOARD_COLS];

    for (int angle = 0; angle < 4; angle++) {
        for (int col = -4; col < BOARD_COLS; col++) {
            if (can_place(&rotated, board, 0, col)) {
                memcpy(board_temp, board, sizeof(board_temp));

                int landing_row = find_landing_row(&rotated, board_temp, col);
                place_block_at(&rotated, board_temp, landing_row, col);
                int eliminated_lines = count_and_eliminate(board_temp);

                int temp_line_height[BOARD_COLS];
                cal_line_height(board_temp, temp_line_height);

                double score = evaluate_state_update(board_temp, temp_line_height,
                                                     BOARD_ROWS - landing_row, eliminated_lines);
                if (score > best_score) {
                    best_score = score;
                    best_col = col;
                    best_angle = angle;
                    //print_board(board_temp);
                    //printf("Score: %.2f, Col: %d, Angle: %d\n", score, col, angle);
                }
            }
        }
        rotate_shape(&rotated, &temp_rotated);
        copy_shape(&temp_rotated, &rotated);
    }
    //printf("%.2f\n", best_score);
    best_pos->col = best_col;
    best_pos->angle = best_angle;
}

int show_game_over_window() {
    SDL_Window *win = SDL_CreateWindow("Game Over", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 400, 200, SDL_WINDOW_SHOWN);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    Uint32 winID = SDL_GetWindowID(win);

    SDL_SetRenderDrawColor(ren, 225, 0, 0, 255);
    SDL_RenderClear(ren);

    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/ubuntu/UbuntuMono-RI.ttf", 36);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *surface = TTF_RenderText_Blended(font, "Your are dead haha!", white);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(ren, surface);

    SDL_Rect dstrect;
    dstrect.w = surface->w;
    dstrect.h = surface->h;
    dstrect.x = 200 - dstrect.w / 2;
    dstrect.y = 100 - dstrect.h / 2;
    SDL_RenderCopy(ren, texture, NULL, &dstrect);

    TTF_Font* Infofont = TTF_OpenFont("/usr/share/fonts/truetype/ubuntu/UbuntuMono-RI.ttf", 16);
    SDL_Surface *Infosurface = TTF_RenderText_Blended(Infofont, "'r' to restart", white);
    SDL_Texture *Infotexture = SDL_CreateTextureFromSurface(ren, Infosurface);

    SDL_Rect Infodstrect;
    Infodstrect.w = Infosurface->w;
    Infodstrect.h = Infosurface->h;
    Infodstrect.x = 200 - Infodstrect.w / 2;
    Infodstrect.y = 150 - Infodstrect.h / 2;
    SDL_RenderCopy(ren, Infotexture, NULL, &Infodstrect);
    SDL_RenderPresent(ren);

    failureSound = Mix_LoadWAV("assets/failureSound.wav");
    Mix_PlayChannel(-1, failureSound, 0);

    SDL_Event e;
    int quit = 0;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_CLOSE && e.window.windowID == winID) {
                    exit(0);
                }
            }
            else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_r) {
                    quit = 1;
                }
            }
        }
        SDL_Delay(50);
    }

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);

    return 1;
}

int game_loop()
{
    srand(time(NULL));

    SDL_Window* win;
    SDL_Renderer* renderer;

    TTF_Init();
    init(&win, &renderer);

    int mode = show_start_window(renderer);
    Uint32 delay;
    if (mode == EASY_MODE) {
         delay = INIT_DELAY + 100;
    }
    else if (mode == NORMAL_MODE) {
         delay = INIT_DELAY;
    }
    else if (mode == HARD_MODE) {
         delay = INIT_DELAY - 100;
    }

    init_sidebar(renderer);

    Button helpButton;
    initHelpButton(renderer, &helpButton);

    bgm = Mix_LoadMUS("assets/bgm.mp3");
    dropSound = Mix_LoadWAV("assets/dropSound.wav");
    clearSound = Mix_LoadWAV("assets/clearSound.wav");

    Mix_PlayMusic(bgm, -1);

    int total_score = 0;

    int index1 = rand() % 7;
    int index2 = rand() % 7;
    Shape* cur_block = get_block(index1);
    Shape* next_block = get_block(index2);
    drawShape(renderer, next_block, NEXT_X, NEXT_Y, colors[index2]);

    Shape cur_shape;
    copy_shape(cur_block, &cur_shape);

    int posX = 4; // 初始列
    int posY = 0; // 初始行

    int quit = 0;
    SDL_Event e;

    Uint32 last_tick = SDL_GetTicks();

    PlaceState best_pos;
    int line_height[BOARD_COLS] = {0};

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            }
            else if (e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.key.keysym.sym == SDLK_a) {
                    Shape rotated;
                    for (int i = 0; i < BLOCK_SIZE; i++)
                        for (int j = 0; j < BLOCK_SIZE; j++)
                            rotated[i][j] = 0;

                    rotate_shape(&cur_shape, &rotated);
                    if (can_place(&rotated, board, posY, posX)) {
                        copy_shape(&rotated, &cur_shape);
                    }
                }

                else if (e.key.keysym.sym == SDLK_LEFT) {
                    // 向左移动
                    if (can_place(&cur_shape, board, posY, posX - 1)) {
                        posX--;
                    }
                } 
                else if (e.key.keysym.sym == SDLK_RIGHT) {
                    // 向右移动
                    if (can_place(&cur_shape, board, posY, posX + 1)) {
                        posX++;
                    }
                } 
                else if (e.key.keysym.sym == SDLK_DOWN) {
                    if (can_place(&cur_shape, board, posY + 1, posX)) {
                        posY++;
                    }
                }
                else if (e.key.keysym.sym == SDLK_h || isButtonClicked(&helpButton, e.button.x, e.button.y)) {
                    find_best_place(board, &cur_shape, line_height, &best_pos);
                    //printf("best place: %d %d\n", best_pos.angle, best_pos.col);
                    for (int i = 0; i < best_pos.angle; i++) {
                        Shape rotated;
                        rotate_shape(&cur_shape, &rotated);
                        copy_shape(&rotated, &cur_shape);
                    }
                    posX += best_pos.col - 4;
                }
                //else if (isButtonClicked)
                else if (e.key.keysym.sym == SDLK_SPACE) {
                    //暂停游戏
                    int paused = 1;
                    while (paused) {
                        SDL_Event pause_event;
                        while (SDL_PollEvent(&pause_event)) {
                            if (pause_event.type == SDL_QUIT) {
                                quit = 1;
                                paused = 0;
                            } 
                            else if (pause_event.type == SDL_KEYDOWN && pause_event.key.keysym.sym == SDLK_SPACE) {
                                paused = 0; // 继续游戏
                            }
                        }
                    }
                }
                else if (e.key.keysym.sym == SDLK_q) {
                    quit = 1;
                }
                
            }
        }

        Uint32 now = SDL_GetTicks();
        if (now - last_tick > delay) {
            // 尝试向下移动方块
            if (can_place(&cur_shape, board, posY + 1, posX)) {
                posY++;
            } 
            else {
                place_block_at(&cur_shape, board, posY, posX);
                //print_board(board);
                Mix_PlayChannel(-1, dropSound, 0);
                int eliminated_lines = count_and_eliminate(board);
                if (eliminated_lines > 0)
                    Mix_PlayChannel(-1, clearSound, 0);
                cal_line_height(board, line_height);
                total_score += score(eliminated_lines);
                cur_block = next_block;

                index1 = index2;
                index2 = rand() % 7;
                next_block = get_block(index2);
                copy_shape(cur_block, &cur_shape);

                //更新sidebar
                drawShape(renderer, cur_block, NEXT_X, NEXT_Y, colors[8]);
                drawShape(renderer, next_block, NEXT_X, NEXT_Y, colors[index2]);

                updateScoreText(renderer, total_score);

                posX = 4;
                posY = 0;

                if (!can_place(&cur_shape, board, posY, posX)) {
                    printf("Game Over!\n");
                    quit = show_game_over_window();
                }
            }
            last_tick = now;
            /*
            if (delay > 100) {
                delay -= ACCELERATE_SPEED;
            }*/
        }

        // 绘制
        drawBoard(renderer);
        drawShape(renderer, &cur_shape, posX, posY, colors[index1]);
        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
}

int main(int argc, char* argv[]) {

    int quit_all = 0;
    while (!quit_all) {
        quit_all = game_loop();
        for (int i = 0; i < BOARD_ROWS; i++) {
            for (int j = 0; j < BOARD_COLS; j++) {
                board[i][j] = 0; // 重置游戏板
            }
        }
    }
    Mix_FreeMusic(bgm);
    Mix_FreeChunk(dropSound);
    Mix_FreeChunk(clearSound);
    Mix_FreeChunk(failureSound);
    Mix_CloseAudio();
    TTF_Quit();
    SDL_Quit();
    return 0;
}
