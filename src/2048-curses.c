#define _GNU_SOURCE
#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include "dialog.h"

#define CALC_WIN_X(block_x) (block_x * 6 + (block_x + 1) * 2)
#define CALC_WIN_Y(block_y) (block_y * 3 + block_y + 1)
#define EXIT_KEY 'q'
#define CP_BACKGROUND 13
#define CP_GRID 14
#define CP_EMPTY 15
#define CP_SCORE 16
#define CP_OTHER 17
#define CP_DIALOG 18
#define CP_DIALOG_HEAD 19
#define CP_DIALOG_BODY 20
#define CP_DIALOG_CHOICES 21
#define CP_DIALOG_CHOICES_F 22

/* Structs */
struct cp_t
{
	short f, b;
};

/* Enums */
enum signals
{
	sig_block_not_spawned,
	sig_game_over,
	sig_block_spawned,
	sig_can_sum,
	sig_block_zero,
	sig_exit,
	sig_err
};
enum directions
{
	dir_none,
	dir_up,
	dir_down,
	dir_left,
	dir_right
};

/* Global variables */
int BLOCK_ARRAY_Y, BLOCK_ARRAY_X, MAIN_WIN_X, MAIN_WIN_Y;
int **block_array;
chtype grid;
chtype **win_array;	// note: only test. Can be useful when I add the help window.
WINDOW *main_win;
WINDOW *main_win_frame;
int **block_array_old;
int score, score_old;
enum signals sig;
int winner = 0;

/* Functions */
void initpairs(void)
{
	struct cp_t tmp[12] = {
		{0, 253}, 	// 2
		{0, 246}, 	// 4
		{15, 208}, 	// 8
		{15, 202}, 	// 16
		{0, 9}, 	// 32
		{0, 1}, 	// 64
		{0, 227}, 	// 128
		{0, 220},	// 256
		{0, 10}, 	// 512
		{0, 6}, 	// 1024
		{0, 1},		// 2048
		{0, 1}		// 4096+
	};

	for (int i = 0; i < 12; i++)
		init_pair(i + 1, tmp[i].f, tmp[i].b);
	init_pair(CP_GRID, 0, 236);
	init_pair(CP_EMPTY, COLOR_WHITE, COLOR_BLACK);
	init_pair(CP_SCORE, 46, 236);
	init_pair(CP_OTHER, 15, 236);
	init_pair(CP_BACKGROUND, COLOR_RED, 17);
	init_pair(CP_DIALOG, 15, 236);
	init_pair(CP_DIALOG_HEAD, 46, 236);
	init_pair(CP_DIALOG_BODY, COLOR_WHITE, 236);
	init_pair(CP_DIALOG_CHOICES, 15, 236);
	init_pair(CP_DIALOG_CHOICES_F, 15, COLOR_RED);
}
void initArrays(void)
{
	//MAIN_WIN_X = BLOCK_ARRAY_X * 6 + (BLOCK_ARRAY_X + 1) * 2;
	//MAIN_WIN_Y = BLOCK_ARRAY_Y * 3 + BLOCK_ARRAY_Y + 1;
	MAIN_WIN_X = CALC_WIN_X(BLOCK_ARRAY_X);
	MAIN_WIN_Y = CALC_WIN_Y(BLOCK_ARRAY_Y);

	block_array = (int **)calloc(BLOCK_ARRAY_Y, sizeof(int *));
	block_array_old = (int **)calloc(BLOCK_ARRAY_Y, sizeof(int *));
	for (int i = 0; i < BLOCK_ARRAY_Y; i++)
	{
		block_array[i] = (int *)calloc(BLOCK_ARRAY_X, sizeof(int *));
		block_array_old[i] = (int *)calloc(BLOCK_ARRAY_X, sizeof(int *));
	}

	win_array = (chtype **)malloc(MAIN_WIN_Y * sizeof(chtype *) + 1);
	for (int i = 0; i < MAIN_WIN_Y; i++)
		win_array[i] = (chtype *)malloc(MAIN_WIN_X * sizeof(chtype) + 1);


	grid = ' ' | COLOR_PAIR(CP_GRID);

	for (int i = 0; i < BLOCK_ARRAY_Y; i++)
		for (int j = 0; j < BLOCK_ARRAY_X; j++)
			block_array[i][j] = 0;

	for (int i = 0; i < MAIN_WIN_Y; i++)
	{
		for (int j = 0; j < MAIN_WIN_X; j++)
		{
			if (i == 0 || (i % 4) == 0)
				win_array[i][j] = grid;
			else if (j == 0 || (j % 8) == 0 || ((j - 1) % 8) == 0)
				win_array[i][j] = grid;
			else
				win_array[i][j] = ' ' | COLOR_PAIR(CP_EMPTY);
		}
	}
}
int get_block_color(int value)
{
	if (value == 0)
		return COLOR_BLACK;
	int a = 1, i;
	for (i = 1; a < 4096; i++)
	{
		a *= 2;
		if (a == value)
			return i;
	}
	return 12;
}
void add_block(int block_value, int block_array_y, int block_array_x)
{
	block_array[block_array_y][block_array_x] = block_value;

	int CP = get_block_color(block_value);
	int win_y = 1, win_x = 2;
	int i, j;

	for (i = 0; i < block_array_y; i++)
		win_y += 4;
	for (i = 0; i < block_array_x; i++)
		win_x += 8;

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 6; j++)
		{
			//win_array[win_y + i][win_x + j] = ' ' | COLOR_PAIR(CP);
			mvwaddch(main_win, win_y + i, win_x + j, ' ' | COLOR_PAIR(CP));
		}
	}

	if (block_value != 0)
	{
		char *buf;
		asprintf(&buf, "%i", block_value);

		for (i = 0; buf[i] != '\0'; i++);
		for (j = 0; j < i; j++)
		{
			//win_array[win_y + 1][win_x + 3 - i / 2 + j] = buf[j] | COLOR_PAIR(CP);
			mvwaddch(main_win, win_y + 1, win_x + 3 - i / 2 + j, buf[j] | COLOR_PAIR(CP));
		}
		free(buf);
	}
}

_Bool can_spawn_block()
{
	for (int i = 0; i < BLOCK_ARRAY_Y; i++)
		for (int j = 0; j < BLOCK_ARRAY_X; j++)
			if (block_array[i][j] == 0)
				return true;
	return false;
}

enum signals rand_add_block(void)
{
	if (!can_spawn_block())
	{
		return sig_block_not_spawned;	
	}

	FILE *f = fopen("/dev/urandom", "r");
	int rand_y, rand_x, rand_TwoOrFour;

	while (true)
	{
		rand_y = fgetc(f) % BLOCK_ARRAY_Y;
		rand_x = fgetc(f) % BLOCK_ARRAY_X;

		if (block_array[rand_y][rand_x] != 0)
			continue;

		rand_TwoOrFour = ((fgetc(f) % 2) + 1) * 2;

		add_block(rand_TwoOrFour, rand_y, rand_x);
		break;
	}
	fclose(f);
	
	return sig_block_spawned;
}
void move_block(int from_y, int from_x, int to_y, int to_x)
{
	add_block(block_array[from_y][from_x], to_y, to_x);
	add_block(0, from_y, from_x);
}
enum signals get_action(int from_y, int from_x, int to_y, int to_x)
{
	if (((0 <= to_y) && (to_y < BLOCK_ARRAY_Y)) && ((0 <= to_x) && (to_x < BLOCK_ARRAY_X)))
	{
		if (block_array[to_y][to_x] == 0)
			return sig_block_zero;
		else if (block_array[to_y][to_x] == block_array[from_y][from_x])
			return sig_can_sum;
		return sig_err;
	}
	return sig_err;
}

_Bool lower_then(int a, int b)
{
	return (a < b) ? 1 : 0;
}

_Bool higher_then(int a, int b)
{
	return (a > b) ? 1 : 0;
}

void print_block_array_values(int y, int x)
{
	init_pair(20, COLOR_GREEN, COLOR_BLACK);
	mvprintw(y++, x, "[DEBUG]");
	for (int i = 0; i < BLOCK_ARRAY_Y; i++)
	{
		for (int j = 0; j < BLOCK_ARRAY_X; j++)
		{
			if (block_array[i][j] != 0)
				attron(COLOR_PAIR(20));
			mvprintw(y++, x, "block_array[%i][%i]=%3i", i, j, block_array[i][j]);
			attroff(COLOR_PAIR(20));
		}
	}
	refresh();
}

void move_blocks(enum directions dir)
{
	int i = 0, j = 0, i1 = BLOCK_ARRAY_Y, j1 = BLOCK_ARRAY_X, pmi = 1, pmj = 1, si = 0, sj = 0, j2, i2, anim_limit;
	
	switch (dir)
	{
	case dir_up:
		si = -1;
		i = 1;
		anim_limit = BLOCK_ARRAY_Y - 1;
		break;
	case dir_down:
		si = 1;
		i = BLOCK_ARRAY_Y - 2;
		i1 = -1;
		pmi = -1;
		anim_limit = BLOCK_ARRAY_Y - 1;
		break;
	case dir_left:
		sj = -1;
		j = 1;
		anim_limit = BLOCK_ARRAY_X - 1;
		break;
	case dir_right:
		j = BLOCK_ARRAY_X - 2;
		j1 = -1;
		sj = 1;
		pmj = -1;
		anim_limit = BLOCK_ARRAY_X - 1;
		break;
	default:
		return;
	}

	/* phase 0: moving, phase 1: summing, phase 2: moving */
	for (int phase = 0; phase < 3; phase++)
	{
		int si1 = si, sj1 = sj;
		for (int anim = 0; (phase == 1) ? anim < 1 : anim < anim_limit; anim++)
		{
			for (i2 = i; (i2 < i1) ? lower_then(i2, i1) : higher_then(i2, i1); i2 += pmi)
			{
				for (j2 = j; (j2 < j1) ? lower_then(j2, j1) : higher_then(j2, j1); j2 += pmj)
				{
					if (block_array[i2][j2] != 0)
					{
						enum signals sig = get_action(i2, j2, i2 + si, j2 + sj);
						if (phase == 0 || phase == 2)
						{
							if (sig == sig_block_zero)
							{
								add_block(block_array[i2][j2], i2 + si, j2 + sj);
								add_block(0, i2, j2);
							}
						}
						else
						{
							if (sig == sig_can_sum)
							{
								score_old = score;
								score += block_array[i2][j2] * 2;
								add_block(block_array[i2][j2] * 2, i2 + si, j2 + sj);
								add_block(0, i2, j2);
								if (block_array[i2 + si][j2 + sj] == 2048)
									winner++;
							}
						}
					}
				}	
			}
			wrefresh(main_win);
			napms(20);
		}
	}

}
enum directions choose_dir(int input)
{
	switch (input)
	{
		case KEY_UP: 
			return dir_up;
		case KEY_DOWN: 
			return dir_down;
		case KEY_LEFT: 
			return dir_left;
		case KEY_RIGHT: 
			return dir_right;
		default: 
			return dir_none;
	}
}

_Bool matrix_eq(int **m1, int **m2)
{
	for (int i = 0; i < BLOCK_ARRAY_Y; i++)
		for (int j = 0; j < BLOCK_ARRAY_X; j++)
			if (m1[i][j] != m2[i][j])
				return 0;
	return 1;
}

_Bool check_pos(int block_array_y, int block_array_x)
{
	if (block_array_y >= 0 && block_array_y < BLOCK_ARRAY_Y && block_array_x >= 0 && block_array_x < BLOCK_ARRAY_X)
		return 1;
	return 0;
}

_Bool possible_move(void)
{
	for (int block_y = 0; block_y < BLOCK_ARRAY_Y; block_y++)
	{
		for (int block_x = 0; block_x < BLOCK_ARRAY_X; block_x++)
		{
			for (int i = -1; i < 2; i++)
			{
				if (i == 0)
					continue;
				if (check_pos(block_y + i, block_x))
					if (block_array[block_y][block_x] == block_array[block_y + i][block_x])
						return 1;
				if (check_pos(block_y, block_x + i))
					if (block_array[block_y][block_x] == block_array[block_y][block_x + i])
						return 1;
			}
		}
	}
	return 0;
}

static inline void do_undo(void)
{
	for (int block_y = 0; block_y < BLOCK_ARRAY_Y; block_y++)
	{
		for (int block_x = 0; block_x < BLOCK_ARRAY_X; block_x++)
		{
			add_block(block_array_old[block_y][block_x], block_y, block_x);
		}
	}
}

void free_all(void)
{
	int i;
	for (i = 0; i < BLOCK_ARRAY_Y; i++)
	{
		free(block_array[i]);
		free(block_array_old[i]);
	}
	free(block_array);
	free(block_array_old);
	
	for (i = 0; i < MAIN_WIN_Y; i++)
		free(win_array[i]);
	free(win_array);
}

void col_mvwprintw(short cp, WINDOW *win, int y, int x, char *str)
{
	wattron(win, COLOR_PAIR(cp));
	mvwprintw(win, y, x, "%s", str);
	wattroff(win, COLOR_PAIR(cp));
}

void update_score(void)
{
	char *buf;
	asprintf(&buf, "%i", score);
	col_mvwprintw(CP_OTHER, main_win_frame, 0, MAIN_WIN_X	- strlen("Score: ") - strlen(buf), "Score: ");
	col_mvwprintw(CP_SCORE, main_win_frame, 0, MAIN_WIN_X - strlen(buf), buf);
	free(buf);
}
void set_dialog_color(DIALOG *d)
{
	d->color = COLOR_PAIR(CP_DIALOG);
	d->color_head = COLOR_PAIR(CP_DIALOG_HEAD);
	d->color_data = COLOR_PAIR(CP_DIALOG_BODY);
	d->color_choices = COLOR_PAIR(CP_DIALOG_CHOICES);
	d->color_choices_focused = COLOR_PAIR(CP_DIALOG_CHOICES_F);
}

void start_help(int help_win_y, int help_win_x, int help_pos_y, int help_pos_x)
{
	if (help_win_y <= 0 || help_win_x <= 0) 
	{
		help_win_y = DIALOG_BASE_Y;
		help_win_x = DIALOG_BASE_X;
	}

	DIALOG_CHOICE **about_choices = malloc(1 * sizeof(DIALOG_CHOICE *));
	about_choices[0] = new_choice("<Ok>", NULL);

	DIALOG *about = new_dialog(
			"2048 - About",
			"- This vresion of 2048 was made by TheRetikGM.\n"
			"- If you have any comments, please contact me on theretikgm@gmail.com",
			about_choices, 1);

	DIALOG_CHOICE **help_choices = malloc(2 * sizeof(DIALOG_CHOICE *));
	help_choices[0] = new_choice("<Ok>", NULL);
	help_choices[1] = new_choice("<About>", about);

	DIALOG *help = new_dialog(
			"2048 - HELP",
			"Options:\n"
			"- Use arrow keys to move the blocks\n"
			"- Press 'u' for undo\n"
			"- Press 'r' to restart\n"
			"- Press 'q' to quit",
			help_choices, 2);

	set_dialog_color(help);
	dialog_copy_styles(help, about);

	resize_dialog(help, help_win_y, help_win_x);
	resize_dialog(about, help_win_y, help_win_x);
	move_dialog(help, help_pos_y, help_pos_x);
	move_dialog(about, help_pos_y, help_pos_x);

	start_dialog(help);

	destroy_dialog(about);
	destroy_dialog(help);
	free(help_choices);
	free(about_choices);
}
void start_dialog_exit(int height, int width, int base_y, int base_x)
{
	DIALOG_CHOICE **exit_choices = (DIALOG_CHOICE **)malloc(2 * sizeof(DIALOG_CHOICE *));
	exit_choices[0] = new_choice("<Yes>", NULL);
	exit_choices[1] = new_choice("<No>", NULL);

	DIALOG *exit = new_dialog(
			"Exit",
			"Are you sure?",
			exit_choices, 2);
	set_dialog_color(exit);
	resize_dialog(exit, height, width);
	move_dialog(exit, base_y, base_x);
	
	if (start_dialog(exit) == 0)
		sig = sig_exit;

	destroy_dialog(exit);
	free(exit_choices);
}
void start_win_dialog(int height, int width, int base_y, int base_x)
{
	DIALOG_CHOICE **win_choices = malloc(sizeof(DIALOG_CHOICE *));
	win_choices[0] = new_choice("<Ok>", NULL);

	DIALOG *win = new_dialog(
			"You win!",
			"You have created a block with value 2048!\n"
			"Congratulation!",
			win_choices, 1);
	set_dialog_color(win);
	resize_dialog(win, height, width);
	move_dialog(win, base_y, base_x);
	start_dialog(win);

	destroy_dialog(win);
	free(win_choices);
}
void refresh_game(void)
{
	bkgd(COLOR_PAIR(CP_BACKGROUND));
	refresh();
	wbkgd(main_win_frame, COLOR_PAIR(CP_BACKGROUND));

	for (int i = 0; i < MAIN_WIN_Y; i++)
		for(int j = 0; j < MAIN_WIN_X; j++)
			mvwaddch(main_win, i, j, win_array[i][j]);
	for (int i = 0; i < BLOCK_ARRAY_Y; i++)
		for (int j = 0; j < BLOCK_ARRAY_X; j++)
			add_block(block_array[i][j], i, j);

	col_mvwprintw(CP_OTHER, main_win_frame, 0, 0, " 2048 ");

	char under_text[7];
	sprintf(under_text, "F1 or %c", EXIT_KEY);
	col_mvwprintw(CP_OTHER, main_win_frame, MAIN_WIN_Y + 3, (MAIN_WIN_X - 7) / 2, under_text);
	wrefresh(main_win_frame);
	wrefresh(main_win);
}

int main(int argc, char **argv)
{
	int maxx, maxy;
	int input = 0;

	int help_win_y = DIALOG_BASE_Y;
	int help_win_x = DIALOG_BASE_X;

	enum directions dir;
	sig = sig_err;

	if (argv[1] == NULL || argv[2] == NULL)
		BLOCK_ARRAY_Y = BLOCK_ARRAY_X = 4;
	else {
		BLOCK_ARRAY_Y = atoi(argv[1]);
		BLOCK_ARRAY_X = atoi(argv[2]);
	}

	/* Start of curses */
	initscr();
	noecho();
	curs_set(false);

	if (!has_colors())
	{
		endwin();
		printf("[ERROR] Your terminal does not support needed colors.");
		return -1;
	}

	getmaxyx(stdscr, maxy, maxx);

	if (CALC_WIN_X(BLOCK_ARRAY_X) >= maxx || CALC_WIN_Y(BLOCK_ARRAY_Y) >= (maxy - 4)) {
		endwin();
		return -2;
	}

	initArrays();
	
	start_color();
	initpairs();

	main_win_frame = newwin(MAIN_WIN_Y + 4, MAIN_WIN_X, (maxy - MAIN_WIN_Y) / 2, (maxx - MAIN_WIN_X) / 2);
	main_win = derwin(main_win_frame, MAIN_WIN_Y, MAIN_WIN_X, 2, 0);

	refresh();
	refresh_game();

	keypad(main_win, true);
	do
	{
		dir = choose_dir(input);
		
		if (dir == dir_none && input != 0) {
			flushinp();

			if (input == 'u')
			{
				if (!matrix_eq(block_array_old, block_array))
				{
					do_undo();
					score = score_old;
					update_score();
				       	wrefresh(main_win);
					wrefresh(main_win_frame);
				}
			}
			else if (input == KEY_F(1))
			{
				start_help(help_win_y, help_win_x, (maxy - help_win_y) / 2, (maxx - help_win_x) / 2);
				refresh_game();
			}
			else if (input == EXIT_KEY)
			{
				start_dialog_exit(8, 18, (maxy - 8) / 2, (maxx - 18) / 2);
				refresh_game();
			}

			if (sig != sig_exit)
				continue;
			break;
		}
		for (int y = 0; y < BLOCK_ARRAY_Y; y++)
			for (int x = 0; x < BLOCK_ARRAY_X; x++)
				block_array_old[y][x] = block_array[y][x];

		move_blocks(dir);
		update_score();

		if (winner == 1)
		{
			wrefresh(main_win_frame);
			start_win_dialog(11, 38, (maxy - 7) / 2, (maxx - 38) / 2);
			refresh_game();
			winner = 2;
		}
		if (!can_spawn_block() && !possible_move())
		{
			sig = sig_game_over;
			break;
		}
		
		if (!matrix_eq(block_array_old, block_array) || input == 0)
			rand_add_block();

		wrefresh(main_win);
		wrefresh(main_win_frame);
		flushinp();
	} while ((input = wgetch(main_win)));

	if (sig == sig_game_over)
	{
		mvprintw(0, 0, "Game Over!");
		refresh();
	}

	delwin(main_win);
	delwin(main_win_frame);
	endwin();
	/* End of curses */

	free_all();

	return 0;
}
