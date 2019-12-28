#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include "dialog.h"


DIALOG_CHOICE *new_choice(char *text, DIALOG *next_dialog)
{
	DIALOG_CHOICE *dc = malloc(sizeof(DIALOG_CHOICE));
	if (text != NULL)
		dc->text = strdup(text);
	else 
		dc->text = NULL;

	dc->next_dialog = next_dialog;
	return dc;
}
void free_choice(DIALOG_CHOICE *dc)
{
	if (dc->text != NULL)
		free(dc->text);
	free(dc);
}
DIALOG *new_dialog(char *head, char *data, DIALOG_CHOICE **choices, short n_choices)
{
	DIALOG *d = (DIALOG *)malloc(sizeof(DIALOG));

	d->choices = choices;
	d->n_choices = n_choices;
	d->win = newwin(DIALOG_BASE_Y, DIALOG_BASE_X, 0, 0);
	d->win_data = derwin(d->win, DIALOG_BASE_Y - 6, DIALOG_BASE_X - 2, 3, 1);
	d->head = strdup(head);
	d->data = strdup(data);
	d->color = 0;
	d->color_head = 0;
	d->color_data = 0;
	d->color_choices = 0;
	d->color_choices_focused = 0;

	return d;
}

void destroy_dialog(DIALOG *d)
{
	delwin(d->win_data);
	delwin(d->win);
	free(d->head);
	free(d->data);
	for (int i = 0; i < d->n_choices; i++)
		free_choice(d->choices[i]);
	free(d);
}

void resize_dialog(DIALOG *d, int y, int x)
{
	wresize(d->win, y, x);
	wresize(d->win_data, y - 6, x - 2);
	refresh();
}

void show_dialog(DIALOG *d)
{

	wattron(d->win, d->color);
	for (int i = 0; i < getmaxy(d->win); i++)
		for (int j = 0; j < getmaxx(d->win); j++)
			mvwaddch(d->win, i, j, ' ');
	box(d->win, 0, 0);
	mvwhline(d->win, 2, 1, ACS_HLINE, getmaxx(d->win) - 2);
	mvwhline(d->win, getmaxy(d->win) - 3, 1, ACS_HLINE, getmaxx(d->win) - 2);
	wattroff(d->win, d->color);

	wattron(d->win, d->color_head);
	mvwaddstr(d->win, 1, (getmaxx(d->win) - strlen(d->head)) / 2, d->head);
	wattroff(d->win, d->color_head);

	wbkgd(d->win_data, d->color_data);
	wattron(d->win_data, d->color_data);

	wmove(d->win_data, 0, 0);
	waddstr(d->win_data, d->data);
	wrefresh(d->win);

	wattroff(d->win_data, d->color_data);
}

void move_dialog(DIALOG *d, int y, int x)
{
	mvwin(d->win, y, x);
	mvwin(d->win_data, y + 3, x + 1);
}

int start_dialog(DIALOG *d)
{
	if (!d)
		return -1;
	if (d->n_choices < 1)
		return -1;

	show_dialog(d);
	
	int c = 0, choice_number;
	int *pos = calloc(d->n_choices, sizeof(int));
	int focus = 0;
	attr_t a_focused = (d->color_choices_focused == 0) ? A_REVERSE : d->color_choices_focused;

	int total_len_pos = 0;
	for (int i = 0; i < d->n_choices; i++)
		total_len_pos += strlen(d->choices[i]->text) + ((i + 1) == d->n_choices ? 0 : 2);
	total_len_pos = pos[0] = (getmaxx(d->win) - total_len_pos) / 2;

	for (int i = 1; i < d->n_choices; i++)
		pos[i] = pos[i - 1] + strlen(d->choices[i - 1]->text) + 2;

	keypad(d->win, true);
	wattron(d->win, d->color_choices);
	do	
	{
		switch(c)
		{
			case KEY_LEFT:
				focus -= (focus == 0) ? 0 : 1;
				break;
			case KEY_RIGHT:
				focus += (focus == (d->n_choices - 1)) ? 0 : 1;
				break;
		}
		if (c == 10) {
			DIALOG *next = d->choices[focus]->next_dialog;	
			if (next == NULL) {
				choice_number = focus;
				break;
			}
			start_dialog(next);
			show_dialog(d);
			wrefresh(d->win);
		}

		if (d->n_choices == 1) {
			wattron(d->win, a_focused);
			mvwaddstr(d->win, getmaxy(d->win) - 2, pos[0], d->choices[0]->text);
			wattroff(d->win, a_focused);
		}
		else {
			for (int i = 0; i < d->n_choices; i++)
			{
				if (i == focus)
					wattron(d->win, a_focused);
				else
					wattron(d->win, d->color_choices);
				mvwprintw(d->win, getmaxy(d->win) - 2, pos[i], d->choices[i]->text);

				wattroff(d->win, a_focused);
				wattroff(d->win, d->color_choices);
			}
		}

		wrefresh(d->win);
	} while ((c = wgetch(d->win)));
	wattroff(d->win, d->color_choices);
	free(pos);

	return choice_number;
}

void dialog_copy_styles(DIALOG *d1, DIALOG *d2)
{
	d2->color = d1->color;
	d2->color_head = d1->color_head;
	d2->color_data = d1->color_data;
	d2->color_choices = d1->color_choices;
	d2->color_choices_focused = d1->color_choices_focused;
}

chtype *chtype_str_color(chtype *str, int color_pair)
{
	for (int i = 0; str[i] != '\0'; i++)
		str[i] |= color_pair;
	return str;
}
