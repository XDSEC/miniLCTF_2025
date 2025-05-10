#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>

#define WIDTH 16
#define HEIGHT WIDTH
#define TICK_TIME 200000000
#define POINTS 10
#define MAX_LENGTH POINTS

#define EATING "GOT POINT!"
#define PADDING "          "
#define WALL "HIT WALL!"

bool running = true;

enum modes : bool { FIXED, RANDOM };
enum skins : bool { CLASSIC, NUMERIC };
enum directions { UP, DOWN, LEFT, RIGHT };

struct Point {
    size_t x;
    size_t y;
    uint8_t value;
};

static int point_compar_pos(const void *_pa, const void *_pb) {
    const struct Point *pa = _pa;
    const struct Point *pb = _pb;
    if (pa->x - pb->x != 0) {
        return pa->x - pb->x;
    }
    return pa->y - pb->y;
}

struct Snake {
    enum directions dir;
    size_t length;
    struct Point body[MAX_LENGTH + 3];
    size_t point_count;
    struct Point points[POINTS];
};

struct {
    enum modes mode;
    enum skins skin;
    unsigned int seed;
} config;

bool hit_wall = false;
int ate_point_idx = -1;
WINDOW *window;

void *search(const void *key, const void *base, size_t nmemb, size_t size,
             int (*compar)(const void *, const void *)) {
    const void *ptr;
    for (size_t i = 0; i < nmemb; ++i) {
        ptr = base + i * size;
        if (compar(ptr, key) == 0) {
            return (void *)ptr;
        }
    }
    return NULL;
}

void _read_uint(unsigned int *pnum) {
    if (scanf("%u", pnum) <= 0) {
        puts("Invalid input.");
        _exit(EXIT_FAILURE);
    }
}

void init(void) { setbuf(stdout, NULL); }

void banner(void) {
    puts("███╗   ███╗██╗███╗   ██╗██╗███████╗███╗   ██╗ █████╗ ██╗  "
         "██╗███████╗\n"
         "████╗ ████║██║████╗  ██║██║██╔════╝████╗  ██║██╔══██╗██║ "
         "██╔╝██╔════╝\n"
         "██╔████╔██║██║██╔██╗ ██║██║███████╗██╔██╗ ██║███████║█████╔╝ █████╗\n"
         "██║╚██╔╝██║██║██║╚██╗██║██║╚════██║██║╚██╗██║██╔══██║██╔═██╗ ██╔══╝\n"
         "██║ ╚═╝ ██║██║██║ ╚████║██║███████║██║ ╚████║██║  ██║██║  "
         "██╗███████╗\n"
         "╚═╝     ╚═╝╚═╝╚═╝  ╚═══╝╚═╝╚══════╝╚═╝  ╚═══╝╚═╝  ╚═╝╚═╝  "
         "╚═╝╚══════╝");
}

void settings(void) {
    unsigned int choice;
    printf("Welcome to Minisnake speedrun game!\nControls:\n  Moving: WASD\n  "
           "Quit: Q\n\n1. Random seed mode\n2. Fixed "
           "seed mode\nPlease select your mode: ");
    _read_uint(&choice);
    switch (choice) {
    case 1:
        config.mode = RANDOM;
        break;
    case 2:
        config.mode = FIXED;
        break;
    default:
        puts("Invalid choice.");
        _exit(EXIT_FAILURE);
    }
    if (config.mode == RANDOM) {
        config.seed = arc4random();
    } else if (config.mode == FIXED) {
        printf("Please input the seed: ");
        _read_uint(&choice);
        config.seed = choice;
    }
    printf(
        "1. Classic skin\n2. Numeric skin\nPlease select your snake's skin: ");
    _read_uint(&choice);
    switch (choice) {
    case 1:
        config.skin = CLASSIC;
        break;
    case 2:
        config.skin = NUMERIC;
        break;
    default:
        puts("Invalid choice.");
        _exit(EXIT_FAILURE);
    }
}

void admin_env(void) {
    unsigned int choice;
    puts("ADMIN ENV\n\n1. Modify seed\n2. Modify skin\n3. Maintenance mode");
    _read_uint(&choice);
    switch (choice) {
    case 1:
        printf("Please input the new seed: ");
        _read_uint(&choice);
        config.seed = choice;
        break;
    case 2:
        printf("Please input the new skin type: ");
        _read_uint(&choice);
        switch (choice) {
        case 1:
            config.skin = CLASSIC;
            break;
        case 2:
            config.skin = NUMERIC;
            break;
        default:
            puts("Invalid choice.");
            _exit(EXIT_FAILURE);
        }
        break;
    case 3:
        puts("Entering system maintenance mode...");
        system("/bin/sh");
        break;
    default:
        puts("Invalid choice.");
        _exit(EXIT_FAILURE);
    }
}

void create(uint8_t map[HEIGHT][WIDTH], struct Snake *snake) {
    int x, y;
    for (int i = 0; i < POINTS; ++i) {
        do {
            x = random() % WIDTH;
            y = random() % HEIGHT;
        } while (search(&(struct Point){.x = x, .y = y}, snake->points, i,
                        sizeof(struct Point), point_compar_pos) ||
                 search(&(struct Point){.x = x, .y = y}, snake->body,
                        snake->length, sizeof(struct Point), point_compar_pos));
        switch (config.skin) {
        case CLASSIC:
            snake->points[i] = (struct Point){.x = x, .y = y, .value = '#'};
            break;
        case NUMERIC:
            snake->points[i] = (struct Point){
                .x = x, .y = y, .value = random() % (UINT8_MAX - 1) + 1};
            break;
        }
        ++snake->point_count;
    }
}

int control_handler(void *arg) {
    struct Snake *snake = arg;
    while (running) {
        int ch = getch();
        switch (ch) {
        case 'w':
        case 'W':
            snake->dir = UP;
            break;
        case 's':
        case 'S':
            snake->dir = DOWN;
            break;
        case 'a':
        case 'A':
            snake->dir = LEFT;
            break;
        case 'd':
        case 'D':
            snake->dir = RIGHT;
            break;
        case 'q':
        case 'Q':
            running = false;
            break;
        }
    }
    return thrd_error;
}

int events_handler(void *arg) {
    struct Snake *snake = arg;
    while (running) {
        if (ate_point_idx != -1) {
            snake->body[snake->length++] = snake->points[ate_point_idx];
            snake->points[ate_point_idx].x = UINT8_MAX;
            snake->points[ate_point_idx].y = UINT8_MAX;
            --snake->point_count;
            ate_point_idx = -1;
            move(HEIGHT / 2, WIDTH * 2 + 2);
            addstr(EATING);
            refresh();
            thrd_sleep(
                &(struct timespec){.tv_nsec = TICK_TIME * 2, .tv_sec = 0},
                NULL);
            move(HEIGHT / 2, WIDTH * 2 + 2);
            addstr(PADDING);
            refresh();
            move(0, 0);
        }
        if (hit_wall) {
            move(HEIGHT / 2, WIDTH * 2 + 2);
            addstr(WALL);
            refresh();
            running = false;
            thrd_sleep(
                &(struct timespec){.tv_nsec = TICK_TIME * 2, .tv_sec = 0},
                NULL);
        }
        thrd_sleep(&(struct timespec){.tv_nsec = TICK_TIME, .tv_sec = 0}, NULL);
    }
    return thrd_success;
}

void draw(const uint8_t map[HEIGHT][WIDTH], struct Point *snake_pos) {
    char pos_str[8];
    memset(pos_str, '0', sizeof(pos_str));
    wclear(window);
    wborder(window, '|', '|', '-', '-', '*', '*', '*', '*');
    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            if (map[i][j]) {
                if (config.skin == NUMERIC) {
                    mvwprintw(window, i + 1, j * 2 + 1, "%02x", map[i][j]);
                }
                if (config.skin == CLASSIC) {
                    mvwprintw(window, i + 1, j * 2 + 1, "%1$c%1$c", map[i][j]);
                }
            }
        }
    }
    if (!hit_wall) {
        move(HEIGHT / 2 - 2, WIDTH * 2 + 2);
        snprintf(pos_str, sizeof(pos_str), "%zu %zu    ", snake_pos->x,
                 snake_pos->y);
        addstr(pos_str);
        refresh();
    }
    wrefresh(window);
}

void snake_move(struct Snake *snake) {
    for (int i = snake->length - 1; i > 0; --i) {
        snake->body[i].x = snake->body[i - 1].x;
        snake->body[i].y = snake->body[i - 1].y;
    }
    switch (snake->dir) {
    case UP:
        --(snake->body[0].y);
        break;
    case DOWN:
        ++(snake->body[0].y);
        break;
    case LEFT:
        --(snake->body[0].x);
        break;
    case RIGHT:
        ++(snake->body[0].x);
        break;
    }
}

void emit_event(struct Snake *snake) {
    size_t x = snake->body[0].x;
    size_t y = snake->body[0].y;
    struct Point *pptr;
    if (x == -1 || y == -1 || x == WIDTH || y == HEIGHT) {
        hit_wall = true;
    } else {
        hit_wall = false;
    }
    if ((pptr = search(&(struct Point){.x = x, .y = y}, &snake->body[1],
                       snake->length - 1, sizeof(struct Point),
                       point_compar_pos)) != NULL) {
        hit_wall = true;
    }
    if ((pptr = search(&(struct Point){.x = x, .y = y}, snake->points, POINTS,
                       sizeof(struct Point), point_compar_pos)) != NULL) {
        ate_point_idx = pptr - snake->points;
    }
    if (snake->point_count == 0) {
        running = false;
    }
}

void set_map(uint8_t map[HEIGHT][WIDTH], struct Snake *snake) {
    memset(map, 0, HEIGHT * WIDTH);
    for (int i = hit_wall ? 1 : 0; i < snake->length; ++i) {
        map[snake->body[i].y][snake->body[i].x] = snake->body[i].value;
    }
    for (int i = 0; i < POINTS; ++i) {
        size_t x = snake->points[i].x;
        size_t y = snake->points[i].y;
        if (x == UINT8_MAX || y == UINT8_MAX) {
            continue;
        }
        map[y][x] = snake->points[i].value;
    }
}

size_t game(void) {
    struct {
        thrd_t control_thrd;
        thrd_t events_thrd;
        struct Snake snake;
        struct timespec time_sleep;
        uint8_t map[HEIGHT][WIDTH];
    } game = {.snake = {.body = {{.x = 5, .y = 3},
                                 {.x = 4, .y = 3},
                                 {.x = 3, .y = 3}},
                        .dir = RIGHT,
                        .length = 3},
              .time_sleep = {.tv_nsec = TICK_TIME, .tv_sec = 0}};
    srandom(config.seed);
    if (config.skin == CLASSIC) {
        game.snake.body[0].value = game.snake.body[1].value =
            game.snake.body[2].value = '#';
    } else if (config.skin == NUMERIC) {
        game.snake.body[0].value = random() % (UINT8_MAX - 1) + 1;
        game.snake.body[1].value = random() % (UINT8_MAX - 1) + 1;
        game.snake.body[2].value = random() % (UINT8_MAX - 1) + 1;
    }
    create(game.map, &game.snake);
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    window = newwin(HEIGHT + 2, WIDTH * 2 + 2, 0, 0);
    thrd_create(&game.control_thrd, control_handler, &game.snake);
    thrd_create(&game.events_thrd, events_handler, &game.snake);
    int i = 0;
    while (running) {
        snake_move(&game.snake);

        emit_event(&game.snake);

        set_map(game.map, &game.snake);

        draw(game.map, &game.snake.body[0]);

        thrd_sleep(&game.time_sleep, NULL);
    }
    thrd_join(game.events_thrd, NULL);
    curs_set(0);
    endwin();
    return game.snake.point_count;
}

void finishup(double duration, size_t point_count) {
    printf("\033cYou ate %zu points in %.3lf seconds.\n", point_count,
           duration);
}

int main(void) {
    init();
    banner();
    settings();
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    int ate_pts = POINTS - game();
    clock_gettime(CLOCK_MONOTONIC, &end);
    finishup((end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9,
             ate_pts);
    return 0;
}

// gcc minisnake.c -o minisnake -lncurses
