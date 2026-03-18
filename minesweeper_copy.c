#include <stdio.h>
#include <stdlib.h>
#include <time.h> 
#include <string.h>
#include <ctype.h>
#include <math.h>

#pragma message("usage of the file: difficulty rows cols \n")

#ifdef __MINGW32__
    #include <ncurses/ncurses.h>
#else
    #include <ncurses.h>
#endif

#define X_OFF 5
#define Y_OFF 5

// according to r/minesweeper, the hardest difficulty is 20% mines  \
                               the medium  difficulty is 15% mines  \
                               the easy    difficulty is 10% mines

typedef enum GameState {
    WIN     = 0,
    LOSE    = 1,
    IN_GAME = 2,
    QUIT    = 3
} GameState;

typedef enum Difficulty {
    EASY   = 10,
    MEDIUM = 15,
    HARD   = 20
} Difficulty;

typedef enum Flag {
    ACTIVE,
    UNACTIVE
} Flag;

typedef struct Board{
    char** grid;
    int rows;
    int cols;
} Board;

typedef struct Game {
    Difficulty diff;
    GameState state;
    Board board;       
    Board visible_board;
    Flag flag;
    int c_row;
    int c_col;
} Game;

int main(int, char* []);
Board init_board(int ,int, char);
Game init_game(char*, int ,int);
void print_board(Board, int, int);
char** create_board(Board, char);
void free_board(Board);
char** set_mines(Board, Difficulty);
char** set_numbers(Board);
int randomNum(int);
void init_ui(void);
void end_ui(void);
void draw_game(Game);
void toggle_flag(Game*);
void reveal_cell(Game*, int, int);
void check_win(Game*);
void game_loop(Game*);

int main(int argc, char* argv[]) {

    if (argc != 4) {
        fprintf(stderr, "Usage: %s difficulty rows cols\n", argv[0]);
        return 1;
    }

    Game game = init_game(argv[1], atoi(argv[2]),atoi(argv[3]));

    init_ui();
    game_loop(&game);
    end_ui();

    switch(game.state) {
        case WIN:
            fprintf(stdout, "GG wellplayed you won this time\n"); break;
        case LOSE:
            fprintf(stdout, "You dont even know how to solve a minesweeper?\n"); break;   
        case QUIT:
            fprintf(stdout, "Yeah go ahead and quit you big quitter\n"); break;
        case IN_GAME:
            fprintf(stdout, "How did you get here?\n"); break;
    }

    free_board(game.board);
    free_board(game.visible_board);

    return 0;

}

void game_loop(Game* game) {
    int ch;

    while (game -> state == IN_GAME) {
        draw_game(*game);

        ch = getch();

        switch (ch) {
            case KEY_UP:
            case 'w':
            case 'W':
                if (game->c_row > 0) game->c_row--;
                break;

            case KEY_DOWN:
            case 's':
            case 'S':
                if (game->c_row < game->board.rows - 1) game->c_row++;
                break;

            case KEY_LEFT:
            case 'a':
            case 'A':
                if (game->c_col > 0) game->c_col--;
                break;

            case KEY_RIGHT:
            case 'd':
            case 'D':
                if (game->c_col < game->board.cols - 1) game->c_col++;
                break;

            case 'f':
            case 'F':
                toggle_flag(game);
                break;

            case ' ':
            case '\n':
                reveal_cell(game, game->c_row, game->c_col);
                check_win(game);
                break;

            case 'q':
            case 'Q':
                game->state = QUIT;
                break;
        }
    }

    draw_game(*game);
}

void draw_game(Game game) {
    clear();
    mvprintw(0, 0, "minesweeper");
    mvprintw(1, 0, "press q|Q to quit");
    mvprintw(2, 0, "controlls:");
    mvprintw(3, 0, "use 'AWSD' to move the curser, 'f' to toggle flag, 'space' to reveal cell/put flag ('X')");    
    mvprintw(4, 0, "Currently in: %s", game.flag == ACTIVE ? "FLAG" : "REVEAL");
    
    print_board(game.visible_board, X_OFF, Y_OFF);

    mvchgat(game.c_row+ X_OFF, game.c_col + Y_OFF, 1, A_REVERSE | A_DIM, 0, NULL);
}

void toggle_flag(Game* game) {
    game -> flag ^= 1;
}

void check_win(Game* game) {
    if (game->state == LOSE) {
        return;
    }

    for (int row = 0; row < game -> board.rows; row++) {
        for (int col = 0; col < game -> board.cols; col++) {
            if ('0' <= game -> board.grid[row][col] && game -> board.grid[row][col] < '9' && game -> visible_board.grid[row][col] == '#') {
                game -> state = IN_GAME;
                return;
            }
        }
    }

    game -> state = WIN;
}

Game init_game(char* difficulty, int rows ,int cols) {
    Game game; 

    Difficulty diff;
    if (strcmp(difficulty, "easy") == 0 || strcmp(difficulty, "e") == 0) {
        game.diff= EASY;
    } else if (strcmp(difficulty, "medium") == 0 || strcmp(difficulty, "m") == 0) {
        game.diff = MEDIUM;
    } else if (strcmp(difficulty, "hard") == 0 || strcmp(difficulty, "h") == 0) {
        game.diff = HARD;
    } else {
        fprintf(stderr, "Usage: difficulty rows cols\n");
        exit(1);
    }

    game.board = init_board(rows , cols, '0');
    game.board.grid = set_mines(game.board, game.diff);
    game.board.grid = set_numbers(game.board);

    game.visible_board = init_board(rows, cols, '#');
    game.c_row = 0;
    game.c_col = 0;
    game.state = IN_GAME;
    game.flag = UNACTIVE;

    return game;
}

Board init_board(int rows, int cols, char sign){
    Board board;

    board.rows = rows;
    board.cols = cols;

    if (board.rows <= 0 || board.cols <= 0) {
        fprintf(stderr, "Rows and columns must be positive integers.\n");
        exit(1);
    }

    board.grid = create_board(board, sign);

    return board;
}

void reveal_cell(Game* game, int c_row, int c_col) {
    if (game -> flag == UNACTIVE) {
        if (game -> board.grid[c_row][c_col] == '*') {
            game -> visible_board.grid[c_row][c_col] = '*';
            game -> state = LOSE;
        } else {
            game -> visible_board.grid[c_row][c_col] = game -> board.grid[c_row][c_col];
        }
    } 
    if (game -> flag == ACTIVE) {
        if (game -> visible_board.grid[c_row][c_col] == '#') {
            game -> visible_board.grid[c_row][c_col] = 'X';
        } else if (game -> visible_board.grid[c_row][c_col] == 'X') {
            game -> visible_board.grid[c_row][c_col] = '#';
        }
    }
}

void init_ui(void) {

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);
}

void end_ui(void) {
    endwin();
}

void print_board(Board board, int x_off, int y_off) {

    for (int row = 0; row < board.rows; row++) {
        for (int col = 0; col < board.cols; col++) {
            char cell = board.grid[row][col];
            char shown = (cell == '0') ? '.' : cell;

            mvaddch(y_off + row, x_off + col, shown);
        }
    }

    refresh();
}

char** create_board(Board board, char sign) {
    char** grid = (char**)malloc(board.rows * sizeof(char*));

    for (int row = 0; row < board.rows; row++) {
        grid[row] = (char*)malloc(board.cols * sizeof(char));

        for (int col = 0; col < board.cols; col++) {
            grid[row][col] = sign;
        }
    }
    return grid;
}

char** set_mines(Board board, Difficulty diff) {
    int total_cells = board.rows * board.cols;
    int total_mines = ceil((float)(total_cells * diff) / 100);  

    int row = 0;
    int col = 0;

    int mines_col = total_mines / board.cols;
    int mines_placed = 0;
    for (; col < board.cols; col++)
    {
        mines_placed = 0;
        while (mines_placed != mines_col) {

            row = randomNum(board.rows);
            while (board.grid[row][col] == '*') {
                row = randomNum(board.rows);
            }

            board.grid[row][col] = '*';
            mines_placed += 1;
        }
    }

    mines_placed = 0;
    while ( (mines_col * board.cols) + mines_placed < total_mines) {
        row = randomNum(board.rows);
        col = randomNum(board.cols);

        if (board.grid[row][col] != '*') {
            board.grid[row][col] = '*';
            mines_placed++;
        }
    }

    return board.grid;
}

void free_board(Board board) {
    for (int row = 0; row < board.rows; row++) {
        free(board.grid[row]);
    }
    free(board.grid);
}

int randomNum(int max) {
    static int seed = 0;
    if (!seed) {
        seed = 1;
        srand((unsigned)time(0));
    }
    return (rand() % (max));
}

char** set_numbers(Board board) {
    for (int row = 0; row < board.rows; row++) {
        for (int col = 0; col < board.cols; col++) {
            if (board.grid[row][col] == '*') {

                if (row - 1 >= 0 && col - 1 >= 0 && board.grid[row - 1][col - 1] != '*') {
                    board.grid[row - 1][col - 1] += 1;
                }
                if (row - 1 >= 0 && board.grid[row - 1][col] != '*') {
                    board.grid[row - 1][col] += 1;
                }
                if (row - 1 >= 0 && col + 1 < board.cols && board.grid[row - 1][col + 1] != '*') {
                    board.grid[row - 1][col + 1] += 1;
                }
                if (col - 1 >= 0 && board.grid[row][col - 1] != '*') {
                    board.grid[row][col - 1] += 1;
                }
                if (col + 1 < board.cols && board.grid[row][col + 1] != '*') {
                    board.grid[row][col + 1] += 1;
                }
                if (row + 1 < board.rows && col - 1 >= 0 && board.grid[row + 1][col - 1] != '*') {
                    board.grid[row + 1][col - 1] += 1;
                }
                if (row + 1 < board.rows && board.grid[row + 1][col] != '*') {
                    board.grid[row + 1][col] += 1;
                }
                if (row + 1 < board.rows && col + 1 < board.cols && board.grid[row + 1][col + 1] != '*') {
                    board.grid[row + 1][col + 1] += 1;
                }
            } 
        }
    }

    return board.grid;
}