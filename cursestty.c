/*
 * Smart terminal output routine.
 * Uses curses to display the object.
 */

#include <stdarg.h>
#include <signal.h>
#include <curses.h>


#undef	FALSE
#undef	TRUE
#undef	OK

#include "lifesrc.h"


static	Bool	inputready;
static	int	statusline;
static	int	inputline;


static void	gotinput(int);


/*
 * Open the terminal and enable for detecting terminal input.
 * Returns TRUE if successful.
 */
Bool
ttyOpen(void)
{
	initscr();
	cbreak();
	noecho();
	signal(SIGINT, gotinput);

	/*
	 * Remember the lines used for printing results and reading input.
	 * These are the bottom lines of the screen.
	 */
	statusline = LINES - 1;
	inputline = LINES - 1;

	return TRUE;
}


static void
gotinput(int sigalNumber)
{
	signal(SIGINT, gotinput);
	inputready = TRUE;
}


/*
 * Close the terminal.
 */
void
ttyClose(void)
{
	refresh();
	endwin();
}


/*
 * Test to see if a keyboard character is ready.
 * Returns nonzero if so (and clears the ready flag).
 */
Bool
ttyCheck(void)
{
	Bool	result;

	result = inputready;
	inputready = FALSE;

	return result;
}


/*
 * Print a formatted string to the terminal.
 * The string length is limited to 256 characters.
 */
void
ttyPrintf(const char * fmt, ...)
{
	va_list		ap;
	static char	buf[256];

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);
	addstr(buf);
}


/*
 * Print a status line, similar to printf.
 * The string length is limited to 256 characters.
 */
void
ttyStatus(const char * fmt, ...)
{
	va_list 	ap;
	static char	buf[256];

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);

	move(statusline, 0);
	addstr(buf);
	clrtobot();
	move(0, 0);
	refresh();
}


void
ttyWrite(const char * cp, int len)
{
	while (len-- > 0)
	{
		addch(*cp);
		cp++;
	}
}


void
ttyHome(void)
{
	move(0, 0);
}


void
ttyEEop(void)
{
	clrtobot();
}


void
ttyFlush(void)
{
	refresh();
}


/*
 * Return a NULL terminated input line (without the final newline).
 * The specified string is printed as a prompt.
 * Returns TRUE if a string was read, or FALSE (and an empty buffer)
 * on end of file or error.
 */
Bool
ttyRead(const char * prompt, char * buf, int buflen)
{
	int	c;
	char *	cp;

	move(inputline, 0);
	addstr(prompt);
	clrtoeol();

	cp = buf;

	for (;;)
	{
		refresh();

		c = fgetc(stdin);

		switch (c)
		{
		case EOF:
			buf[0] = '\0';
			move(inputline, 0);
			clrtoeol();
			move(0, 0);
			refresh();

			return FALSE;

		default:
			*cp++ = c;
			addch(c);

			if (cp < buf + buflen - 1)
				break;

			/* fall through... */

		case '\n':
		case '\r':
			*cp = 0;
			move(inputline, 0);
			clrtoeol();
			move(0, 0);
			refresh();

			return TRUE;

		case '\b':
			if (cp == buf)
				break;

			--cp;
			addch('\b');
			clrtoeol();

			break;
		}
	}

	return FALSE;
}

/* END CODE */
