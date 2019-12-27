#define DIALOG_BASE_Y 12
#define DIALOG_BASE_X 50
#define DIALOG_ERR -1
#define DIALOG_CHOICE_ESC -2

typedef struct dialog DIALOG;
typedef struct dialog_choice DIALOG_CHOICE;

struct dialog {
	char *head, *data;
	short n_choices;
	WINDOW *win, *win_data;
	int color, color_head, color_data, color_choices, color_choices_focused;
	DIALOG_CHOICE **choices;
};
struct dialog_choice {
	char *text;
	DIALOG *next_dialog;
};

DIALOG *new_dialog(char *head, char *data, DIALOG_CHOICE **choices, short n_choices);
void destroy_dialog(DIALOG *d);
DIALOG_CHOICE *new_choice(char *text, DIALOG *next_dialog);
void resize_dialog(DIALOG *d, int y, int x);
void show_dialog(DIALOG *d);
void move_dialog(DIALOG *d, int y, int c);
int start_dialog(DIALOG *d);
void dialog_copy_styles(DIALOG *d1, DIALOG *d2);
