#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/select.h>
#include <time.h>

#define FLOOR_CHAR ' '
#define SNAKE_CHAR 'o'
#define BORDER_CHAR_W '#'
#define BORDER_CHAR_H '#'
#define FRUIT_CHAR 'x'

typedef struct snake
{
    int x;
    int y;
    struct snake* next;
} snake;

typedef struct fruit
{
    int x;
    int y;
} fruit;

void clear_screen()
{
    printf("\e[1;1H\e[2J");
}

void clear_board(int h, int w, char BOARD[h][w])
{
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            BOARD[i][j] = FLOOR_CHAR;
        }
    }
}

void draw_board(int h, int w, char BOARD[h][w], int score)
{
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            if (i == 0 || i == h - 1)
            {
                printf("%c", BORDER_CHAR_W);
            }
            else if (j == 0 || j == w - 1)
            {
                printf("%c", BORDER_CHAR_H);
            }
            else
            {
                printf("%c", BOARD[i][j]);
            }
        }
        printf("\n");
    }
    printf("Score: %i\n", score);
}

void prepare_terminal(struct termios* original)
{
    struct termios newConfig;
    tcgetattr(STDIN_FILENO, original);
    newConfig = *original;

    newConfig.c_lflag &= ~(ICANON | ECHO);
    newConfig.c_cc[VMIN] = 1;
    newConfig.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &newConfig);
}

void restore_terminal(struct termios* original)
{
    tcsetattr(STDIN_FILENO, TCSANOW, original);
}

int kbhit()
{
    fd_set set;
    struct timeval timeout;

    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    return select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout) > 0;
}

void get_key(char* key)
{
    read(STDIN_FILENO, key, 1);
    tcflush(STDIN_FILENO, TCIFLUSH);
}

void set_directions(int* directions, char key)
{
    switch(key)
    {
        case 'a':
            if (directions[1] == 0)
            {
                directions[0] = 1;
                directions[1] = 0;
                directions[2] = 0;
                directions[3] = 0;
            }
            break;
        case 'd':
            if (directions[0] == 0)
            {
                directions[1] = 1;
                directions[0] = 0;
                directions[2] = 0;
                directions[3] = 0;
            }
            break;
        case 'w':
            if (directions[3] == 0)
            {
                directions[2] = 1;
                directions[0] = 0;
                directions[1] = 0;
                directions[3] = 0;
            }
            break;
        case 's':
            if (directions[2] == 0)
            {
                directions[3] = 1;
                directions[0] = 0;
                directions[1] = 0;
                directions[2] = 0;
            }
            break;
    }
}

void update_snake_pos(snake* s, int rows, int cols, int* directions)
{
    int prevx = s->x;
    int prevy = s->y;

    if (directions[0] == 1) // left
    {
        s->x--;
    }
    else if (directions[1] == 1) // right
    {
        s->x++;
    }
    else if (directions[2] == 1) // up
    {
        s->y--;
    }
    else if (directions[3] == 1) // down
    {
        s->y++;
    }

    snake* current = s->next;
    while(current)
    {
        int tempx = current->x;
        int tempy = current->y;
        current->x = prevx;
        current->y = prevy;
        prevx = tempx;
        prevy = tempy;
        current = current->next;
    }

    if (s->y >= rows - 1) s->y = 1;
    if (s->x >= cols - 1) s->x = 1;
    if (s->y < 1) s->y = rows - 1;
    if (s->x < 1) s->x = cols - 1;
}

int snake_hit(snake* s)
{
    snake* current = s->next;
    while(current)
    {
        if (s->x == current->x && s->y == current->y)
        {
            return 1;
        }
        current = current->next;
    }
    return 0;
}

void update_fruit(fruit* f, int rows, int cols)
{
    f->x = (rand() % (cols - 2)) + 1;
    f->y = (rand() % (rows - 2)) + 1;
}

void free_snake(snake* s)
{
    if (s == NULL)
    {
        return;
    }
    free_snake(s->next);
    free(s);
}

int main(void)
{
    srand(time(NULL));
    struct termios originalConfig;

    // Use curses lib...?
    prepare_terminal(&originalConfig);

    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    
    // get terminal size
    // int rows = w.ws_row;
    // int cols = w.ws_col;
    
    int rows = 20;
    int cols = 30;
    int score = 0;

    fruit* f = malloc(sizeof(fruit));
    snake* s = malloc(sizeof(snake));
    snake* t = s;
    update_fruit(f, rows, cols);
    s->x = 1;
    s->y = 1;
    char BOARD[rows][cols];
    char key;
    int directions[4] = {0};
    while(1)
    {
        if (kbhit())
        {
            get_key(&key);
            set_directions((int*) directions, key);
        }

        if (s->y == f->y && s->x == f->x)
        {
            score++;
            snake* new_body = malloc(sizeof(snake));
            new_body->next = NULL;
            new_body->x = t->x;
            new_body->y = t->y;
            t->next = new_body;
            t = new_body;
            update_fruit(f, rows, cols);
        }

        clear_screen();
        clear_board(rows, cols, BOARD);
        update_snake_pos(s, rows, cols, (int*) directions);
        if (snake_hit(s))
        {
            clear_screen();
            break;
        }
        snake* aux = s;
        while(aux != NULL)
        {
            BOARD[aux->y][aux->x] = SNAKE_CHAR;
            aux = aux->next;
        }
        BOARD[f->y][f->x] = FRUIT_CHAR;
        
        draw_board(rows, cols, BOARD, score);
        usleep(100 * 1000);
    }
    printf("Game Over\n");
    printf("Final Score: %i\n", score);
    restore_terminal(&originalConfig);
    free_snake(s);
    free(f);
    return 0;
}