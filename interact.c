/*
 * Life search program - user interactions module.
 * Author: David I. Bell.
 */

#include "lifesrc.h"

#define	VERSION	"3.8"


/*
 * Local data.
 */
static	Bool	noWait;		/* don't wait for commands after loading */
static	Bool	setAll;		/* set all cells from initial file */
static	Bool	isLife;		/* whether the rules are for standard Life */
static	char	ruleString[20];	/* rule string for printouts */
static	long	foundCount;	/* number of objects found */
static	char *	initFile;	/* file containing initial cells */
static	char *	loadFile;	/* file to load state from */


/*
 * Local procedures
 */
static	void		usage(void);
static	void		getSetting(const char *);
static	void		getBackup(const char *);
static	void		getClear(const char *);
static	void		getExclude(const char *);
static	void		getFreeze(const char *);
static	void		excludeCone(int, int, int);
static	void		freezeCell(int, int);
static	Status		loadState(const char *);
static	Status		readFile(const char *);
static	Bool		confirm(const char *);
static	Bool		setRules(const char *);
static	long		getNum(const char **, int);
static	const char *	getStr(const char *, const char *);


/*
 * Table of addresses of parameters which are loaded and saved.
 * Changing this table may invalidate old dump files, unless new
 * parameters are added at the end and default to zero.
 * When changed incompatibly, the dump file version should be incremented.
 * The table is ended with a NULL pointer.
 */
static	int *	paramTable[] =
{
	&curStatus,
	&rowMax, &colMax, &genMax, &rowTrans, &colTrans,
	&rowSym, &colSym, &pointSym, &fwdSym, &bwdSym,
	&flipRows, &flipCols, &flipQuads,
	&parent, &allObjects, &nearCols, &maxCount,
	&useRow, &useCol, &colCells, &colWidth, &follow,
	&orderWide, &orderGens, &orderMiddle, &followGens,
	NULL
};


int
main(int argc, char ** argv)
{
	const char *	str;

	if (--argc <= 0)
	{
		usage();
		exit(1);
	}

	argv++;

	if (!setRules("3/23"))
		fatal("Cannot set Life rules!");

	/*
	 * Set a couple of defaults.
	 */
	viewFreq = 10 * VIEW_MULT;
	colMax = 75;

	/*
	 * Collect the command line options.
	 */
	while (argc-- > 0)
	{
		str = *argv++;

		if (*str++ != '-')
		{
			usage();
			exit(1);
		}

		switch (*str++)
		{
			case 'q':
				/*
				 * Don't output.
				 */
				quiet = TRUE;
				break;

			case 'r':
				/*
				 * Set number of rows.
				 */
				rowMax = atoi(str);
				break;

			case 'c':
				/*
				 * Set number of columns.
				 */
				colMax = atoi(str);
				break;

			case 'g':
				/*
				 * Set number of generations.
				 */
				genMax = atoi(str);
				break;

			case 't':
				/*
				 * Set row or column translations.
				 */
				switch (*str++)
				{
					case 'r':
						rowTrans = atoi(str);
						break;

					case 'c':
						colTrans = atoi(str);
						break;

					default:
						fatal("Bad translate");
				}

				break;

			case 'f':
				/*
				 * Flip cells around an axis.
				 */
				switch (*str++)
				{
					case 'r':
						flipRows = 1;

						if (*str)
							flipRows = atoi(str);

						break;

					case 'c':
						flipCols = 1;

						if (*str)
							flipCols = atoi(str);

						break;

					case 'q':
						flipQuads = TRUE;
						break;

					case 'g':
						followGens = TRUE;
						break;

					case '\0':
						follow = TRUE;
						break;

					default:
						fatal("Bad flip");
				}

				break;

			case 's':
				/*
				 * Set symmetry.
				 */
				switch (*str++)
				{
					case 'r':
						rowSym = 1;

						if (*str)
							rowSym = atoi(str);

						break;

					case 'c':
						colSym = 1;

						if (*str)
							colSym = atoi(str);

						break;

					case 'p':
						pointSym = TRUE;
						break;

					case 'f':
						fwdSym = TRUE;
						break;

					case 'b':
						bwdSym = TRUE;
						break;

					default:
						fatal("Bad symmetry");
				}

				break;

			case 'n':
				/*
				 * Set near cells.
				 */
				switch (*str++)
				{
					case 'c':
						nearCols = atoi(str);
						break;

					default:
						fatal("Bad near");
				}

				break;

			case 'w':
				/*
				 * Set max width of ON cells.
				 */
				switch (*str++)
				{
					case 'c':
						colWidth = atoi(str);
						break;

					default:
						fatal("Bad width");
				}

				break;

			case 'u':
				/*
				 * Force use of row or column.
				 */
				switch (*str++)
				{
					case 'r':
						useRow = atoi(str);
						break;

					case 'c':
						useCol = atoi(str);
						break;

					default:
						fatal("Bad use");
				}

				break;

			case 'd':
				/*
				 * Get dump frequency.
				 */
				dumpFreq = atol(str) * DUMP_MULT;
				dumpFile = DUMP_FILE;

				if ((argc > 0) && (**argv != '-'))
				{
					argc--;
					dumpFile = *argv++;
				}

				break;

			case 'v':
				/*
				 * Set view frequency.
				 */
				viewFreq = atol(str) * VIEW_MULT;
				break;

			case 'l':
				/*
				 * Load file.
				 */
				if (*str == 'n')
					noWait = TRUE;

				if ((argc <= 0) || (**argv == '-'))
					fatal("Missing load file name");

				loadFile = *argv++;
				argc--;
				break;

			case 'i':
				/*
				 * Read initial file.
				 */
				if (*str == 'd')
				{
					setAll = TRUE;
					setDeep = TRUE;
				}
				else if (*str != 'n')
					setAll = TRUE;

				if ((argc <= 0) || (**argv == '-'))
					fatal("Missing initial file name");

				initFile = *argv++;
				argc--;
				break;

			case 'o':
				/*
				 * Set output columns or file name.
				 */
				if ((*str == '\0') || isDigit(*str))
				{
					/*
					 * Output file name
					 */
					outputCols = atol(str);

					if ((argc <= 0) || (**argv == '-'))
						fatal("Missing output file name");

					outputFile = *argv++;
					argc--;
					break;
				}

				/*
				 * An ordering option.
				 */
				while (*str)
				{
					switch (*str++)
					{
						case 'w':
							orderWide = TRUE;
							break;

						case 'g':
							orderGens = TRUE;
							break;

						case 'm':
							orderMiddle = TRUE;
							break;

						default:
							fatal("Bad ordering option");
					}
				}

				break;

			case 'm':
				/*
				 * Set maximum cell count.
				 */
				switch (*str++)
				{
					case 'c':
						colCells = atoi(str);
						break;

					case 't':
						maxCount = atoi(str);
						break;

					default:
						fatal("Bad maximum");
				}

				break;

			case 'p':
				/*
				 * Find parents only.
				 */
				parent = TRUE;
				break;

			case 'a':
				/*
				 * Find all objects.
				 */
				allObjects = TRUE;
				break;

			case 'D':
				/*
				 * Turn on debugging output.
				 */
				debug = TRUE;
				break;

			case 'R':
				/*
				 * Set rules.
				 */
				if (!setRules(str))
					fatal("Bad rule string");

				break;

			default:
				ttyClose();

				fprintf(stderr, "Unknown option -%c\n",
					str[-1]);

				exit(1);
		}
	}

	if (parent &&
		(rowTrans || colTrans || flipQuads || flipRows || flipCols))
	{
		fatal("Cannot specify translations or flips with -p");
	}

	if ((pointSym != 0) + (rowSym || colSym) + (fwdSym || bwdSym) > 1)
		fatal("Conflicting symmetries specified");

	if ((fwdSym || bwdSym || flipQuads) && (rowMax != colMax))
		fatal("Rows must equal cols with -sf, -sb, or -fq");

	if ((rowTrans || colTrans) + (flipQuads != 0) > 1)
		fatal("Conflicting translation or flipping specified");

	if ((rowTrans && flipRows) || (colTrans && flipCols))
		fatal("Conflicting translation or flipping specified");

	if ((useRow < 0) || (useRow > rowMax))
		fatal("Bad row for -ur");

	if ((useCol < 0) || (useCol > colMax))
		fatal("Bad column for -uc");

	if (!ttyOpen())
		fatal("Cannot initialize terminal");

	/*
	 * Check for loading state from file or reading initial
	 * object from file.
	 */
	if (loadFile)
	{
		if (loadState(loadFile) != OK)
		{
			ttyClose();
			exit(1);
		}
	}
	else
	{
		initCells();

		if (initFile)
		{
			if (readFile(initFile) != OK)
			{
				ttyClose();
				exit(1);
			}

			baseSet = nextSet;
		}
	}

	/*
	 * If we are looking for parents, then set the current generation
	 * to the last one so that it can be input easily.  Then get the
	 * commands to initialize the cells, unless we were told to not wait.
	 */
	if (parent)
		curGen = genMax - 1;

	if (noWait && !quiet)
		printGen(0);
	else
		getCommands();

	inited = TRUE;

	/*
	 * Initial commands are complete, now look for the object.
	 */
	while (TRUE)
	{
		if (curStatus == OK)
			curStatus = search();

		if ((curStatus == FOUND) && useRow &&
			(rowInfo[useRow].onCount == 0))
		{
			curStatus = OK;
			continue;
		}

		if ((curStatus == FOUND) && !allObjects && subPeriods())
		{
			curStatus = OK;
			continue;
		}

		if (dumpFreq)
		{
			dumpcount = 0;
			dumpState(dumpFile);
		}

		quitOk = (curStatus == NOT_EXIST);

		curGen = 0;

		if (outputFile == NULL)
		{
			getCommands();
			continue;
		}

		/*
		 * Here if results are going to a file.
		 */
		if (curStatus == FOUND)
		{
			curStatus = OK;

			if (!quiet)
			{
				printGen(0);
				ttyStatus("Object %ld found.\n", ++foundCount);
			}

			writeGen(outputFile, TRUE);
			continue;
		}

		if (foundCount == 0)
			fatal("No objects found");

		ttyClose();

		if (!quiet)
			printf("Search completed, file \"%s\" contains %ld object%s\n",
				outputFile, foundCount, (foundCount == 1) ? "" : "s");

		exit(0);
	}
}


/*
 * Get one or more user commands.
 * Commands are ended by a blank line.
 */
void
getCommands(void)
{
	const char *	cp;
	const char *	cmd;
	char		buf[LINE_SIZE];

	dumpcount = 0;
	viewCount = 0;
	printGen(curGen);

	while (TRUE)
	{
		if (!ttyRead("> ", buf, LINE_SIZE))
		{
			ttyClose();
			exit(0);
		}

		cp = buf;

		while (isBlank(*cp))
			cp++;

		cmd = cp;

		if (*cp)
			cp++;

		while (isBlank(*cp))
			cp++;

		switch (*cmd)
		{
			case 'p':
				/*
				 * Print previous generation.
			 	 */
				printGen((curGen + genMax - 1) % genMax);
				break;

			case 'n':
				/*
				 * Print next generation.
				 */
				printGen((curGen + 1) % genMax);
				break;

			case 's':
				/*
				 * Add a cell setting.
				 */
				getSetting(cp);
				break;

			case 'b':
				/*
				 * Back up the search.
				 */
				getBackup(cp);
				break;

			case 'c':
				/*
				 * Clear an area.
				 */
				getClear(cp);
				break;

			case 'v':
				/*
				 * Set viewing frequency.
				 */
				viewFreq = atol(cp) * VIEW_MULT;
				printGen(curGen);
				break;

			case 'w':
				/*
				 * Write generation to a file.
				 */
				writeGen(cp, FALSE);
				break;

			case 'd':
				/*
				 * Dump state to a file.
				 */
				dumpState(cp);
				break;

			case 'N':
				/*
				 * Find next object.
				 */
				if (curStatus == FOUND)
					curStatus = OK;

				return;

			case 'q':
			case 'Q':
				/*
				 * Quit program.
				 */
				if (quitOk || confirm("Really quit? "))
				{
					ttyClose();
					exit(0);
				}

				break;

			case 'x':
				/*
				 * Exclude cells from the search.
				 */
				getExclude(cp);
				break;

			case 'f':
				/*
				 * Free state of cells.
				 */
				getFreeze(cp);
				break;
	
			case '\n':
			case '\0':
				/*
				 * Return from commands to the search.
				 */
				return;

			default:
				/*
				 * If a digit, set that cell.
				 */
				if (isDigit(*cmd))
				{
					getSetting(cmd);
					break;
				}

				ttyStatus("Unknown command\n");
				break;
		}
	}
}


/*
 * Get a cell to be set in the current generation.
 * The state of the cell is defaulted to ON.
 * Warning: Use of this routine invalidates backing up over
 * the setting, so that the setting is permanent.
 */
static void
getSetting(const char * cp)
{
	int	row;
	int	col;
	State	state;

	cp = getStr(cp, "Cell to set (row col [state]): ");

	if (*cp == '\0')
		return;

	row = getNum(&cp, -1);

	if (*cp == ',')
		cp++;

	col = getNum(&cp, -1);

	if (*cp == ',')
		cp++;

	state = getNum(&cp, 1);

	while (isBlank(*cp))
		cp++;

	if (*cp != '\0')
	{
		ttyStatus("Bad input line format\n");

		return;
	}

	if ((row <= 0) || (row > rowMax) || (col <= 0) || (col > colMax) ||
		((state != 0) && (state != 1)))
	{
		ttyStatus("Illegal cell value\n");

		return;
	}

	if (proceed(findCell(row, col, curGen), state, FALSE) != OK)
	{
		ttyStatus("Inconsistent state for cell\n");

		return;
	}

	baseSet = nextSet;
	printGen(curGen);
}


/*
 * Backup the search to the nth latest free choice.
 * Notice: This skips examinination of some of the possibilities, thus
 * maybe missing a solution.  Therefore this should only be used when it
 * is obvious that the current search state is useless.
 */
static void
getBackup(const char * cp)
{
	Cell *	cell;
	State	state;
	int	count;
	int	blanksToo;

	blanksToo = TRUE;
#if 0
	/*
	 * This doesn't work!
	 */
	blanksToo = FALSE;

	if (*cp == 'b')
	{
		blanksToo = TRUE;
		cp++;
	}
#endif
	count = getNum(&cp, 0);

	if ((count <= 0) || *cp)
	{
		ttyStatus("Must back up at least one cell\n");

		return;
	}

	while (count > 0)
	{
		cell = backup();

		if (cell == NULL_CELL)
		{
			printGen(curGen);
			ttyStatus("Backed up over all possibilities\n");

			return;
		}

		state = 1 - cell->state;

		if (blanksToo || (state == ON))
			count--;

		cell->state = UNK;

		if (go(cell, state, FALSE) != OK)
		{
			printGen(curGen);
			ttyStatus("Backed up over all possibilities\n");

			return;
		}
	}

	printGen(curGen);
}


/*
 * Clear all remaining unknown cells in the current generation or all
 * generations, or else just the specified rectangular area.  If
 * clearing the whole area, then confirmation is required.
 */
static void
getClear(const char * cp)
{
	int	beggen;
	int	begRow;
	int	begCol;
	int	endGen;
	int	endRow;
	int	endCol;
	int	gen;
	int	row;
	int	col;
	Cell *	cell;

	/*
	 * Assume we are doing just this generation, but if the 'cg'
	 * command was given, then clear in all generations.
	 */
	beggen = curGen;
	endGen = curGen;

	if (*cp == 'g')
	{
		cp++;
		beggen = 0;
		endGen = genMax - 1;
	}

	while (isBlank(*cp))
		cp++;

	/*
	 * Get the coordinates.
	 */
	if (*cp)
	{
		begRow = getNum(&cp, -1);
		begCol = getNum(&cp, -1);
		endRow = getNum(&cp, -1);
		endCol = getNum(&cp, -1);
	}
	else
	{
		if (!confirm("Clear all unknown cells ?"))
			return;

		begRow = 1;
		begCol = 1;
		endRow = rowMax;
		endCol = colMax;
	}

	if ((begRow < 1) || (begRow > endRow) || (endRow > rowMax) ||
		(begCol < 1) || (begCol > endCol) || (endCol > colMax))
	{
		ttyStatus("Illegal clear coordinates");

		return;
	}

	for (row = begRow; row <= endRow; row++)
	{
		for (col = begCol; col <= endCol; col++)
		{
			for (gen = beggen; gen <= endGen; gen++)
			{
				cell = findCell(row, col, gen);

				if (cell->state != UNK)
					continue;

				if (proceed(cell, OFF, FALSE) != OK)
				{
					ttyStatus("Inconsistent state for cell\n");

					return;
				}
			}
		}
	}

	baseSet = nextSet;
	printGen(curGen);
}


/*
 * Exclude cells in a rectangular area from searching.
 * This simply means that such cells will not be selected for setting.
 */
static void
getExclude(const char * cp)
{
	int	begRow;
	int	begCol;
	int	endRow;
	int	endCol;
	int	row;
	int	col;

	while (isBlank(*cp))
		cp++;

	if (*cp == '\0')
	{
		ttyStatus("Coordinates needed for exclusion");

		return;
	}

	begRow = getNum(&cp, -1);
	begCol = getNum(&cp, -1);
	endRow = begRow;
	endCol = begCol;

	while (isBlank(*cp))
		cp++;

	if (*cp)
	{
		endRow = getNum(&cp, -1);
		endCol = getNum(&cp, -1);
	}

	if ((begRow < 1) || (begRow > endRow) || (endRow > rowMax) ||
		(begCol < 1) || (begCol > endCol) || (endCol > colMax))
	{
		ttyStatus("Illegal exclusion coordinates");

		return;
	}

	for (row = begRow; row <= endRow; row++)
	{
		for (col = begCol; col <= endCol; col++)
			excludeCone(row, col, curGen);
	}

	printGen(curGen);
}


/*
 * Exclude all cells within the previous light cone centered at the
 * specified cell from searching.
 */
static void
excludeCone(int row, int col, int gen)
{
	int	tGen;
	int	tRow;
	int	tCol;
	int	dist;

	for (tGen = genMax; tGen >= gen; tGen--)
	{
		dist = tGen - gen;

		for (tRow = row - dist; tRow <= row + dist; tRow++)
		{
			for (tCol = col - dist; tCol <= col + dist; tCol++)
			{
				findCell(tRow, tCol, tGen)->choose = FALSE;
			}
		}
	}
}


/*
 * Freeze cells in a rectangular area so that their states in all
 * generations are the same.
 */
static void
getFreeze(const char * cp)
{
	int	begRow;
	int	begCol;
	int	endRow;
	int	endCol;
	int	row;
	int	col;

	while (isBlank(*cp))
		cp++;

	if (*cp == '\0')
	{
		ttyStatus("Coordinates needed for freezing");

		return;
	}

	begRow = getNum(&cp, -1);
	begCol = getNum(&cp, -1);
	endRow = begRow;
	endCol = begCol;

	while (isBlank(*cp))
		cp++;

	if (*cp)
	{
		endRow = getNum(&cp, -1);
		endCol = getNum(&cp, -1);
	}

	if ((begRow < 1) || (begRow > endRow) || (endRow > rowMax) ||
		(begCol < 1) || (begCol > endCol) || (endCol > colMax))
	{
		ttyStatus("Illegal freeze coordinates");

		return;
	}

	for (row = begRow; row <= endRow; row++)
	{
		for (col = begCol; col <= endCol; col++)
			freezeCell(row, col);
	}

	printGen(curGen);
}


/*
 * Freeze all generations of the specified cell.
 * A frozen cell can be ON or OFF, but must be the same in all generations.
 * This routine marks them as frozen, and also inserts all the cells of
 * the generation into the same loop so that they will be forced
 * to have the same state.
 */
void
freezeCell(int row, int col)
{
	int	gen;
	Cell *	cell0;
	Cell *	cell;

	cell0 = findCell(row, col, 0);

	for (gen = 0; gen < genMax; gen++)
	{
		cell = findCell(row, col, gen);

		cell->frozen = TRUE;

		loopCells(cell0, cell);
	}
}


/*
 * Print out the current status of the specified generation.
 * This also sets the current generation.
 */
void
printGen(int gen)
{
	int		row;
	int		col;
	int		count;
	const Cell *	cell;
	const char *	msg;

	curGen = gen;

	switch (curStatus)
	{
		case NOT_EXIST:	msg = "No such object"; break;
		case FOUND:	msg = "Found object"; break;
		default:	msg = ""; break;
	}

	count = 0;

	for (row = 1; row <= rowMax; row++)
	{
		for (col = 1; col <= colMax; col++)
		{
			count += (findCell(row, col, gen)->state == ON);
		}
	}

	ttyHome();
	ttyEEop();

	if (isLife)
	{
		ttyPrintf("%s (gen %d, cells %d)", msg, gen, count);
	}
	else
	{
		ttyPrintf("%s (rule %s, gen %d, cells %d)",
			msg, ruleString, gen, count);
	}

	ttyPrintf(" -r%d -c%d -g%d", rowMax, colMax, genMax);

	if (rowTrans)
		ttyPrintf(" -tr%d", rowTrans);

	if (colTrans)
		ttyPrintf(" -tc%d", colTrans);

	if (flipRows == 1)
		ttyPrintf(" -fr");

	if (flipRows > 1)
		ttyPrintf(" -fr%d", flipRows);

	if (flipCols == 1)
		ttyPrintf(" -fc");

	if (flipCols > 1)
		ttyPrintf(" -fc%d", flipCols);

	if (flipQuads)
		ttyPrintf(" -fq");

	if (rowSym == 1)
		ttyPrintf(" -sr");

	if (rowSym > 1)
		ttyPrintf(" -sr%d", rowSym);

	if (colSym == 1)
		ttyPrintf(" -sc");

	if (colSym > 1)
		ttyPrintf(" -sc%d", colSym);

	if (pointSym)
		ttyPrintf(" -sp");

	if (fwdSym)
		ttyPrintf(" -sf");

	if (bwdSym)
		ttyPrintf(" -sb");

	if (orderGens || orderWide || orderMiddle)
	{
		ttyPrintf(" -o");

		if (orderGens)
			ttyPrintf("g");

		if (orderWide)
			ttyPrintf("w");

		if (orderMiddle)
			ttyPrintf("m");
	}

	if (follow)
		ttyPrintf(" -f");

	if (followGens)
		ttyPrintf(" -fg");

	if (parent)
		ttyPrintf(" -p");

	if (allObjects)
		ttyPrintf(" -a");

	if (useRow)
		ttyPrintf(" -ur%d", useRow);

	if (useCol)
		ttyPrintf(" -uc%d", useCol);

	if (nearCols)
		ttyPrintf(" -nc%d", nearCols);

	if (maxCount)
		ttyPrintf(" -mt%d", maxCount);

	if (colCells)
		ttyPrintf(" -mc%d", colCells);

	if (colWidth)
		ttyPrintf(" -wc%d", colWidth);

	if (viewFreq)
		ttyPrintf(" -v%ld", viewFreq / VIEW_MULT);

	if (dumpFreq)
		ttyPrintf(" -d%ld %s", dumpFreq / DUMP_MULT, dumpFile);

	if (outputFile)
	{
		if (outputCols)
			ttyPrintf(" -o%d %s", outputCols, outputFile);
		else
			ttyPrintf(" -o %s", outputFile);

		if (foundCount)
			ttyPrintf(" [%d]", foundCount);
	}

	ttyPrintf("\n");

	for (row = 1; row <= rowMax; row++)
	{
		for (col = 1; col <= colMax; col++)
		{
			cell = findCell(row, col, gen);

			switch (cell->state)
			{
				case OFF:
					msg = ". ";
					break;

				case ON:
					msg = "O ";
					break;

				case UNK:
					msg = "? ";

					if (cell->frozen)
						msg = "+ ";

					if (!cell->choose)
						msg = "X ";

					break;
			}

			/*
			 * If wide output, print only one character,
			 * else print both characters.
			 */
			ttyWrite(msg, (colMax < 40) + 1);
		}

		ttyWrite("\n", 1);
	}

	ttyHome();
	ttyFlush();
}


/*
 * Write the current generation to the specified file.
 * Empty rows and columns are not written.
 * If no file is specified, it is asked for.
 * Filename of "." means write to stdout.
 */
void
writeGen(const char * file, Bool append)
{
	FILE *		fp;
	const Cell *	cell;
	int		row;
	int		col;
	int		ch;
	int		minRow;
	int		maxRow;
	int		minCol;
	int		maxCol;

	file = getStr(file, "Write object to file: ");

	if (*file == '\0')
		return;

	fp = stdout;

	if (strcmp(file, "."))
		fp = fopen(file, append ? "a" : "w");

	if (fp == NULL)
	{
		ttyStatus("Cannot create \"%s\"\n", file);

		return;
	}

	/*
	 * First find the minimum bounds on the object.
	 */
	minRow = rowMax;
	minCol = colMax;
	maxRow = 1;
	maxCol = 1;

	for (row = 1; row <= rowMax; row++)
	{
		for (col = 1; col <= colMax; col++)
		{
			cell = findCell(row, col, curGen);

			if (cell->state == OFF)
				continue;

			if (row < minRow)
				minRow = row;

			if (row > maxRow)
				maxRow = row;

			if (col < minCol)
				minCol = col;

			if (col > maxCol)
				maxCol = col;
		}
	}

	if (minRow > maxRow)
	{
		minRow = 1;
		maxRow = 1;
		minCol = 1;
		maxCol = 1;
	}

	if (fp == stdout)
		fprintf(fp, "#\n");

	/*
	 * Now write out the bounded area.
	 */
	for (row = minRow; row <= maxRow; row++)
	{
		for (col = minCol; col <= maxCol; col++)
		{
			cell = findCell(row, col, curGen);

			switch (cell->state)
			{
				case OFF:	ch = '.'; break;
				case ON:	ch = '*'; break;
				case UNK:	ch =
						(cell->choose ? '?' : 'X');
						break;
				default:
					ttyStatus("Bad cell state");
					fclose(fp);

					return;
			}

			fputc(ch, fp);
		}

		fputc('\n', fp);
	}

	if (append)
		fprintf(fp, "\n");

	if ((fp != stdout) && fclose(fp))
	{
		ttyStatus("Error writing \"%s\"\n", file);

		return;
	}

	if (fp != stdout)
		ttyStatus("\"%s\" written\n", file);

	quitOk = TRUE;
}


/*
 * Dump the current state of the search in the specified file.
 * If no file is specified, it is asked for.
 */
void
dumpState(const char * file)
{
	FILE *		fp;
	Cell **		set;
	const Cell *	cell;
	int		row;
	int		col;
	int		gen;
	int **		param;

	file = getStr(file, "Dump state to file: ");

	if (*file == '\0')
		return;

	fp = fopen(file, "w");

	if (fp == NULL)
	{
		ttyStatus("Cannot create \"%s\"\n", file);

		return;
	}

	/*
	 * Dump out the version so we can detect incompatible formats.
	 */
	fprintf(fp, "V %d\n", DUMP_VERSION);

	/*
	 * Dump out the life rule if it is not the normal one.
	 */
	if (!isLife)
		fprintf(fp, "R %s\n", ruleString);

	/*
	 * Dump out the parameter values.
	 */
	fprintf(fp, "P");

	for (param = paramTable; *param; param++)
		fprintf(fp, " %d", **param);

	fprintf(fp, "\n");

	/*
	 * Dump out those cells which have a setting.
	 */
	set = setTable;

	while (set != nextSet)
	{
		cell = *set++;

		fprintf(fp, "S %d %d %d %d %d\n", cell->row, cell->col,
			cell->gen, cell->state, cell->free);
	}

	/*
	 * Dump out those cells which are being excluded from the search.
	 */
	for (row = 1; row <= rowMax; row++)
		for (col = 1; col < colMax; col++)
			for (gen = 0; gen < genMax; gen++)
	{
		cell = findCell(row, col, gen);

		if (cell->choose)
			continue;

		fprintf(fp, "X %d %d %d\n", row, col, gen);
	}

	/*
	 * Dump out those cells in generation 0 which are frozen.
	 * It isn't necessary to remember frozen cells in other
	 * generations since they will be copied from generation 0.
	 */
	for (row = 1; row <= rowMax; row++)
		for (col = 1; col < colMax; col++)
	{
		cell = findCell(row, col, 0);

		if (cell->frozen)
			fprintf(fp, "F %d %d\n", row, col);
	}

	/*
	 * Finish up with the setting offsets and the final line.
	 */
	fprintf(fp, "T %d %d\n", baseSet - setTable, nextSet - setTable);
	fprintf(fp, "E\n");

	if (fclose(fp))
	{
		ttyStatus("Error writing \"%s\"\n", file);

		return;
	}

	ttyStatus("State dumped to \"%s\"\n", file);
	quitOk = TRUE;
}


/*
 * Load a previously dumped state from a file.
 * Warning: Almost no checks are made for validity of the state.
 * Returns OK on success, ERROR on failure.
 */
static Status
loadState(const char * file)
{
	FILE *		fp;
	const char *	cp;
	int		row;
	int		col;
	int		gen;
	int		len;
	State		state;
	Bool		free;
	Cell *		cell;
	int **		param;
	char		buf[LINE_SIZE];

	file = getStr(file, "Load state from file: ");

	if (*file == '\0')
		return OK;

	fp = fopen(file, "r");

	if (fp == NULL)
	{
		ttyStatus("Cannot open state file \"%s\"\n", file);

		return ERROR;
	}

	buf[0] = '\0';
	fgets(buf, LINE_SIZE, fp);

	if (buf[0] != 'V')
	{
		ttyStatus("Missing version line in file \"%s\"\n", file);
		fclose(fp);

		return ERROR;
	}

	cp = &buf[1];

	if (getNum(&cp, 0) != DUMP_VERSION)
	{
		ttyStatus("Unknown version in state file \"%s\"\n", file);
		fclose(fp);

		return ERROR;
	}

	fgets(buf, LINE_SIZE, fp);

	/*
	 * Set the life rules if they were specified.
	 * This line is optional.
	 */
	if (buf[0] == 'R')
	{
		len = strlen(buf) - 1;

		if (buf[len] == '\n')
			buf[len] = '\0';

		cp = &buf[1];

		while (isBlank(*cp))
			cp++;

		if (!setRules(cp))
		{
			ttyStatus("Bad Life rules in state file\n");
			fclose(fp);

			return ERROR;
		}

		fgets(buf, LINE_SIZE, fp);
	}

	/*
	 * Load up all of the parameters from the parameter line.
	 * If parameters are missing at the end, they are defaulted to zero.
	 */
	if (buf[0] != 'P')
	{
		ttyStatus("Missing parameter line in state file\n");
		fclose(fp);

		return ERROR;
	}

	cp = &buf[1];

	for (param = paramTable; *param; param++)
		**param = getNum(&cp, 0);

	/*
	 * Initialize the cells.
	 */
	initCells();

	/*
	 * Handle cells which have been set.
	 */
	newSet = setTable;

	for (;;)
	{
		buf[0] = '\0';
		fgets(buf, LINE_SIZE, fp);

		if (buf[0] != 'S')
			break;

		cp = &buf[1];
		row = getNum(&cp, 0);
		col = getNum(&cp, 0);
		gen = getNum(&cp, 0);
		state = getNum(&cp, 0);
		free = getNum(&cp, 0);

		cell = findCell(row, col, gen);

		if (setCell(cell, state, free) != OK)
		{
			ttyStatus(
				"Inconsistently setting cell at r%d c%d g%d \n",
				row, col, gen);

			fclose(fp);

			return ERROR;
		}
	}

	/*
	 * Handle non-choosing cells.
	 */
	while (buf[0] == 'X')
	{
		cp = &buf[1];
		row = getNum(&cp, 0);
		col = getNum(&cp, 0);
		gen = getNum(&cp, 0);

		findCell(row, col, gen)->choose = FALSE;

		buf[0] = '\0';
		fgets(buf, LINE_SIZE, fp);
	}

	/*
	 * Handle frozen cells.
	 */
	while (buf[0] == 'F')
	{
		cp = &buf[1];
		row = getNum(&cp, 0);
		col = getNum(&cp, 0);

		freezeCell(row, col);

		buf[0] = '\0';
		fgets(buf, LINE_SIZE, fp);
	}

	if (buf[0] != 'T')
	{
		ttyStatus("Missing table line in state file\n");
		fclose(fp);

		return ERROR;
	}

	cp = &buf[1];
	baseSet = &setTable[getNum(&cp, 0)];
	nextSet = &setTable[getNum(&cp, 0)];

	fgets(buf, LINE_SIZE, fp);

	if (buf[0] != 'E')
	{
		ttyStatus("Missing end of file line in state file\n");
		fclose(fp);

		return ERROR;
	}

	if (fclose(fp))
	{
		ttyStatus("Error reading \"%s\"\n", file);

		return ERROR;
	}

	ttyStatus("State loaded from \"%s\"\n", file);
	quitOk = TRUE;

	return OK;
}


/*
 * Read a file containing initial settings for either gen 0 or the last gen.
 * If setAll is TRUE, both the ON and the OFF cells will be set.
 * If setDeep is TRUE, then OFF cells will be set deeply (in all generations).
 * Returns OK on success, ERROR on error.
 */
static Status
readFile(const char * file)
{
	FILE *		fp;
	const char *	cp;
	char		ch;
	int		row;
	int		col;
	int		activeGen;
	int		minGen;
	int		maxGen;
	int		gen;
	State		state;
	char		buf[LINE_SIZE];

	file = getStr(file, "Read initial object from file: ");

	if (*file == '\0')
		return OK;

	fp = fopen(file, "r");

	if (fp == NULL)
	{
		ttyStatus("Cannot open \"%s\"\n", file);

		return ERROR;
	}

	activeGen = (parent ? (genMax - 1) : 0);
	row = 0;

	while (fgets(buf, LINE_SIZE, fp))
	{
		row++;
		cp = buf;
		col = 0;

		while (*cp && (*cp != '\n'))
		{
			minGen = activeGen;
			maxGen = activeGen;

			col++;
			ch = *cp++;

			/*
			 * Check for out of range coordinates.
			 * OFF and UNK cells are allowed for convenience.
			 */
			if ((row > rowMax) || (col > colMax))
			{
				if ((ch == '.') || (ch == ' ') ||
					(ch == ':') || (ch == '?'))
				{
					continue;
				}

				fatal("File sets cells beyond defined area");
			}


			/*
			 * OK, handle the character.
			 */
			switch (ch)
			{
				case '?':
					continue;

				case 'x':
				case 'X':
					excludeCone(row, col, activeGen);
					continue;

				case '+':
					freezeCell(row, col);
					continue;

				case '.':
				case ' ':
					if (!setAll)
						continue;

					if (setDeep)
					{
						minGen = 0;
						maxGen = genMax;
					}

					state = OFF;
					break;

				case ':':
					minGen = 0;
					maxGen = genMax;
					state = OFF;
					break;

				case 'O':
				case 'o':
				case '*':
					state = ON;
					break;

				default:
					ttyStatus("Bad file format in line %d\n",
						row);
					fclose(fp);

					return ERROR;
			}

			for (gen = minGen; gen <= maxGen; gen++)
			{
				if (proceed(findCell(row, col, gen),
					state, FALSE) != OK)
				{
					ttyStatus(
					"Inconsistent state for cell %d %d\n",
						row, col);

					fclose(fp);

					return ERROR;
				}
			}
		}
	}

	if (fclose(fp))
	{
		ttyStatus("Error reading \"%s\"\n", file);

		return ERROR;
	}

	return OK;
}


/*
 * Check a string for being NULL, and if so, ask the user to specify a
 * value for it.  Returned string may be static and thus is overwritten
 * for each call.  Leading spaces in the string are skipped over.
 */
static const char *
getStr(const char * str, const char * prompt)
{
	static char	buf[LINE_SIZE];

	if ((str == NULL) || (*str == '\0'))
	{
		if (!ttyRead(prompt, buf, LINE_SIZE))
		{
			buf[0] = '\0';

			return buf;
		}

		str = buf;
	}

	while (isBlank(*str))
		str++;

	return str;
}


/*
 * Confirm an action by prompting with the specified string and reading
 * an answer.  Entering 'y' or 'Y' indicates TRUE, everything else FALSE.
 */
static Bool
confirm(const char * prompt)
{
	int	ch;

	ch = *getStr(NULL, prompt);

	if ((ch == 'y') || (ch == 'Y'))
		return TRUE;

	return FALSE;
}


/*
 * Read a number from a string, eating any leading or trailing blanks.
 * Returns the value, and indirectly updates the string pointer.
 * Returns specified default if no number was found.
 */
static long
getNum(const char ** cpp, int defnum)
{
	const char *	cp;
	long		num;
	Bool		isNeg;

	isNeg = FALSE;
	cp = *cpp;

	while (isBlank(*cp))
		cp++;

	if (*cp == '-')
	{
		cp++;
		isNeg = TRUE;
	}

	if (!isDigit(*cp))
	{
		*cpp = cp;

		return defnum;
	}

	num = 0;

	while (isDigit(*cp))
		num = num * 10 + (*cp++ - '0');

	if (isNeg)
		num = -num;

	while (isBlank(*cp))
		cp++;

	*cpp = cp;

	return num;
}


/*
 * Parse a string and set the Life rules from it.
 * Returns TRUE on success, or FALSE on an error.
 * The rules can be "mmm,nnn",  "mmm/nnn", "Bmmm,Snnn", "Bmmm/Snnn",
 * or a hex number in the Wolfram encoding.
 */
static Bool
setRules(const char * cp)
{
	char *		cpTemp;
	int		i;
	unsigned int	bits;

	for (i = 0; i < 9; i++)
	{
		bornRules[i] = OFF;
		liveRules[i] = OFF;
	}

	if (*cp == '\0')
		return FALSE;

	/*
	 * See if the string contains a comma or a slash.
	 * If not, then assume Wolfram's hex format.
	 */
	if ((strchr(cp, ',') == NULL) && (strchr(cp, '/') == NULL))
	{
		bits = 0;

		for (; *cp; cp++)
		{
			bits <<= 4;

			if ((*cp >= '0') && (*cp <= '9'))
				bits += *cp - '0';
			else if ((*cp >= 'a') && (*cp <= 'f'))
				bits += *cp - 'a' + 10;
			else if ((*cp >= 'A') && (*cp <= 'F'))
				bits += *cp - 'A' + 10;
			else
				return FALSE;
		}

		if (i & ~0x3ff)
			return FALSE;

		for (i = 0; i < 9; i++)
		{
			if (bits & 0x01)
				bornRules[i] = ON;

			if (bits & 0x02)
				liveRules[i] = ON;

			bits >>= 2;
		}
	}
	else
	{
		/*
		 * It is in normal born/survive format.
		 */
		if ((*cp == 'b') || (*cp == 'B'))
			cp++;

		while ((*cp >= '0') && (*cp <= '8'))
			bornRules[*cp++ - '0'] = ON;

		if ((*cp != ',') && (*cp != '/'))
			return FALSE;

		cp++;

		if ((*cp == 's') || (*cp == 'S'))
			cp++;

		while ((*cp >= '0') && (*cp <= '8'))
			liveRules[*cp++ - '0'] = ON;

		if (*cp)
			return FALSE;
	}

	/*
	 * Construct the rule string for printouts and see if this
	 * is the normal Life rule.
	 */
	cpTemp = ruleString;

	*cpTemp++ = 'B';

	for (i = 0; i < 9; i++)
	{
		if (bornRules[i] == ON)
			*cpTemp++ = '0' + i;
	}

	*cpTemp++ = '/';
	*cpTemp++ = 'S';

	for (i = 0; i < 9; i++)
	{
		if (liveRules[i] == ON)
			*cpTemp++ = '0' + i;
	}

	*cpTemp = '\0';

	isLife = (strcmp(ruleString, "B3/S23") == 0);

	return TRUE;
}


/*
 * Print out a fatal message and exit.
 * The terminal is closed before the message is printed.
 * A newline is added after the supplied message.
 */
void
fatal(const char * msg)
{
	ttyClose();

	fprintf(stderr, "%s\n", msg);

	exit(1);
}


/*
 * Print usage text.
 */
static void
usage(void)
{
	const char * const *	cpp;

	static const char * const text[] =
	{
	"",
	"lifesrc -r# -c# -g# [other options]",
	"lifesrc -l[n] file -v# -o# file -d# file",
	"",
	"   -r   Number of rows",
	"   -c   Number of columns",
	"   -g   Number of generations",
	"   -tr  Translate rows between last and first generation",
	"   -tc  Translate columns between last and first generation",
	"   -fr  Flip rows between last and first generation",
	"   -fc  Flip columns between last and first generation",
	"   -fq  Flip quadrants between last and first generation",
	"   -sr  Enforce symmetry on rows",
	"   -sc  Enforce symmetry on columns",
	"   -sp  Enforce symmetry around central point",
	"   -sf  Enforce symmetry on forward diagonal",
	"   -sb  Enforce symmetry on backward diagonal",
	"   -nc  Near N cells of live cells in previous columns for generation 0",
	"   -wc  Maximum width of live cells in each column for generation 0",
	"   -mt  Maximum total live cells for generation 0",
	"   -mc  Maximum live cells in any column for generation 0",
	"   -ur  Force using at least one ON cell in the given row for generation 0",
	"   -uc  Force using at least one ON cell in the given column for generation 0",
	"   -f   First follow the average location of the previous column's cells",
	"   -fg  First follow settings of previous or next generation",
	"   -ow  Set search order to find wide objects first",
	"   -og  Set search order to examine all gens in a column before next column",
	"   -om  Set search order to examine from middle column outwards",
	"   -p   Only look for parents of last generation",
	"   -a   Find all objects (even those with subPeriods)",
	"   -v   View object every N thousand searches",
	"   -d   Dump status to file every N thousand searches",
	"   -l   Load status from file",
	"   -ln  Load status without entering command mode",
	"   -i   Read initial object setting both ON and OFF cells",
	"   -in  Read initial object from file setting only ON cells",
	"   -id  Read initial object setting OFF cells deeply (all gens)",
	"   -o   Output objects to file (appending) every N columns",
	"   -R   Use Life rules specified by born,live values",
	NULL
	};

	fprintf(stderr,
		"Program to search for Life oscillators or spaceships (version %s)\n",
		VERSION);

	for (cpp = text; *cpp; cpp++)
		fprintf(stderr, "%s\n", *cpp);
}

/* END CODE */
