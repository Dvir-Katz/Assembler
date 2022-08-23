#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "functions.h"

#define MAX_LENGTH 81 /*extra place for the \0*/
#define MACRO_LENGTH 31 /*extra place for the \0*/

FILE* preProcessing (FILE*);
void pass1 (FILE*);
void pass2 ();

typedef struct k
{
    char name[MACRO_LENGTH];
	struct k *nextMacro;
    struct n *nextLine;
}macroName;

typedef struct n
{
    char line[MAX_LENGTH];
    struct n *nextLine;
}macroLine;

typedef struct l
{
    char name[MACRO_LENGTH];
	char type[7]; /*The longest name of type is string, struct and extern which have 6 letters*/
	int address;
    struct l *nextLabel; 
}label;

typedef struct c
{
	char label[MACRO_LENGTH];
	int isStruct;
	int binCode;
	int address;
    struct c *nextLine; 
}code;

typedef struct e
{
	char name[MACRO_LENGTH];
	int address;
	struct e *nextLine;
}entry;

typedef struct x
{
	char label[MACRO_LENGTH];
	int address;
	struct x *nextLine;
}externy;

macroLine *col;
macroName *macroHead;
macroName *macroRow;
macroName *currMacroRow;

label *labelHead;
label *labelRow;
label *currLabelRow;

code *codeHead;
code *codeRow;
code *currCodeRow;

entry *entryHead;
entry *entryRow;
entry *currEntryRow;

externy *externHead;
externy *externRow;
externy *currExternRow;

char* fullFileName = NULL;
char* preProcessingFileName = NULL;
char* entryFileName = NULL;
char* externFileName = NULL;
char* finalFileName = NULL;
bool errorFlag = false;
bool externFlag = false;
bool entryFlag = false;
int IC = 0;
int DC = 0;

int main (int argc, char **argv)
{
	int i;
	FILE *myFile1;
	FILE *myFile2;
	FILE *myFile3;
	FILE *myFile4;
	for (i = 1; i < argc; i++) {
		fullFileName = cpyncat (argv[i], ".as\0");
		if ((myFile1 = fopen (fullFileName , "r")) == NULL) {
			printf("Error 1: can not open %s\n", fullFileName);
			exit(1);
		}
		free (fullFileName);

		preProcessingFileName = cpyncat (argv[i], ".am\0");
		myFile1 = preProcessing (myFile1);
		free (preProcessingFileName);

		if (! (errorFlag)) {
			rewind (myFile1);
			pass1 (myFile1);
		}
		fclose(myFile1);

		if (! (errorFlag))
			pass2();

		if (! (errorFlag)) {
			finalFileName = cpyncat (argv[i], ".ob\0");
			if ((myFile2 = fopen (finalFileName , "w")) == NULL) {
				printf("Error 1: can not open %s\n" ,  finalFileName);
				exit(1);
			}
			fprintf (myFile2 , "%d %d\n" , IC , DC);
			for (codeRow = codeHead; codeRow != currCodeRow; codeRow = codeRow -> nextLine) {
				fprintf (myFile2 , "%c%c %c%c\n" , binTo32(codeRow -> address)[0] , binTo32(codeRow -> 								 address)[1] , binTo32(codeRow -> binCode)[0] , binTo32(codeRow -> binCode)[1]);
			}
			free (finalFileName);
			fclose(myFile2);

			if (entryFlag) {
				entryFileName = cpyncat (argv[i], ".ent\0");
				if ((myFile3 = fopen (entryFileName , "w")) == NULL) {
					printf("Error 1: can not open %s\n" ,  entryFileName);
					exit (1);
				}
				for (entryRow = entryHead; entryRow != currEntryRow; entryRow = entryRow -> nextLine) {
				fprintf (myFile3 , "%s %c%c\n" , entryRow -> name , binTo32(entryRow -> address)[0] ,
																		   binTo32(entryRow -> address)[1]);
				}
			}
			free (entryFileName);
			fclose(myFile3);

			if (externFlag) {
				externFileName = cpyncat (argv[i], ".ext\0");
				if ((myFile4 = fopen (externFileName , "w")) == NULL) {
					printf("Error 1: can not open %s\n" ,  externFileName);
					exit(1);
				}
				for (externRow = externHead; externRow != currExternRow; externRow = externRow -> nextLine){
				fprintf (myFile4 , "%s %c%c\n" , externRow -> label , binTo32(externRow -> address)[0] ,
																		  binTo32(externRow -> address)[1]);
				}
			}
			free (externFileName);
			fclose(myFile4);
		}
		for (macroRow = macroHead; macroRow != currMacroRow; macroRow = macroRow -> nextMacro) {
			while (strcmp (col -> line , "") != 0) {
				col = col -> nextLine;
				free (col);
			}
			free (macroRow);
		}
		free (currMacroRow);

		for (labelRow = labelHead; labelRow != currLabelRow; labelRow = labelRow -> nextLabel)
			free (labelRow);
		free (currLabelRow);

		for (codeRow = codeHead; codeRow != currCodeRow; codeRow = codeRow -> nextLine)
			free (codeRow);
		free (currCodeRow);

		for (entryRow = entryHead; entryRow != currEntryRow; entryRow = entryRow -> nextLine)
			free (entryRow);
		free (currEntryRow);

		for (externRow = externHead; externRow != currExternRow; externRow = externRow -> nextLine)
			free (externRow);
		free (currExternRow);
	}
	return 1;
}

FILE* preProcessing (FILE *file)
{
	FILE* returnFile;
	bool macroWritten;
	char currLine[MAX_LENGTH];
	char* word;

	currMacroRow = macroHead = malloc(sizeof(macroName));
	col = malloc(sizeof(macroLine));

	if ((returnFile = fopen (preProcessingFileName , "w+")) == NULL) {
		printf("Error 1: can not open %s\n" ,  preProcessingFileName);
		exit(1);
	}

	while (fgets(currLine, MAX_LENGTH + 1, file) != NULL) {
		if (strlen(currLine) >= MAX_LENGTH) {
			printf ("Error 2: line is longer then maximum valid length\n");
			errorFlag = true;
		}
		macroWritten = false;
		for (macroRow = macroHead; macroRow != currMacroRow; macroRow = macroRow -> nextMacro) {
			if (strcmp (currLine ,macroRow -> name) == 0) {
				col = &(*macroRow -> nextLine);
				while (strcmp (col -> line , "") != 0) {
					fprintf (returnFile , "%s" , col -> line);
					col = col -> nextLine;
				}
				macroWritten = true;
				break;
			}
		}

		word = strtok (currLine , " ");
		if (strcmp (word, "macro") == 0) {
			strcpy (currMacroRow -> name , strtok (NULL, " "));
			if (strlen (currMacroRow -> name) >= MACRO_LENGTH) {
			printf ("Error 3: macro's name is longer then maximum valid length\n");
			errorFlag = true;
			}
			if (strtok (NULL, currLine) != NULL) {
			printf ("Error 4: macro has more then one name\n");
			errorFlag = true;
			}

			for (macroRow = macroHead; macroRow != currMacroRow; macroRow = macroRow -> nextMacro) {
				if (strcmp (currMacroRow -> name ,macroRow -> name) == 0) {
					printf ("Error 5: more then one macro with the same name\n");
					errorFlag = true;
				}
			}
			if (!(isValidName (currMacroRow -> name))) {
				printf ("Error 6: macro's name isn't valid\n");
				errorFlag = true;
			}
			currMacroRow -> nextLine = malloc (sizeof(macroLine));
 			col = &(*currMacroRow -> nextLine);
			while (fgets(currLine, MAX_LENGTH, file) != NULL) {
				if (strcmp (currLine , "endmacro\n") == 0) {
					currMacroRow -> nextMacro = malloc (sizeof(macroName));
					currMacroRow = &(*currMacroRow -> nextMacro);
					break;
				}
				strcpy (col -> line , currLine);
				col -> nextLine = malloc (sizeof(macroLine));
				col = &(*col -> nextLine);
			}
			continue;
		}

		if (!(macroWritten)) {
			fprintf (returnFile , "%s" , word);
			word = strtok(NULL, currLine);
			while (word != NULL) {
				fprintf (returnFile , " %s" , word);
				word = strtok(NULL, currLine);
			}
		}
	}
return returnFile;
}

void pass1 (FILE *file)
{
	char currLine[MAX_LENGTH];
	char* word;
	int address = 100;
	int lineNumber = 0;
	int com;
	int reg;
	int i;
	const int NUM_OF_COMMANDS = 16;
	const int NUM_OF_REGISTERS = 8;
	const int EIGHT_BITS = 256;
	const int TEN_BITS = 1024;
	bool isLabel;
	bool currErrorFlag;
	bool commandFlag;
	bool firstOpIsNumber;
	bool firstOpIsRegister;
	bool firstOpIsStruct;
	bool secOpIsNumber;
	bool secOpIsRegister;

	char* comList[] = {"mov" , "cmp" , "add" , "sub" , "not" , "clr" , "lea" , "inc" , "dec" , "jmp" , 						   "bne" , "get" , "prn" , "jsr" , "rts" , "hlt"};
	char* regList[] = {"r0" , "r1" , "r2" , "r3" , "r4" , "r5" , "r6" , "r7"};

	currLabelRow = labelHead = malloc(sizeof(label));
	currCodeRow = codeHead = malloc(sizeof(code));
	currEntryRow = entryHead = malloc(sizeof(entry));
	currExternRow = externHead = malloc(sizeof(externy));

	while (fgets(currLine, MAX_LENGTH + 1, file) != NULL) {
		lineNumber++;
		isLabel = false;
		commandFlag = false;
		currErrorFlag = false;
		firstOpIsNumber = false;
		firstOpIsRegister = false;
		firstOpIsStruct = false;
		secOpIsNumber = false;
		secOpIsRegister = false;

		if (currLine[0] == ';') 
			continue;

		word = strtok (currLine , " ");
		/*lables*/
		if (word[strlen(word) - 1] == ':') {
			isLabel = true;
			word[strlen(word) - 1] = '\0';

			if (strlen (word) >= MACRO_LENGTH) {
			printf ("Error 3 in line %d: label's name is longer then maximum valid length\n" , lineNumber);
			errorFlag = true;
			currErrorFlag = true;
			}
			for (macroRow = macroHead; macroRow != currMacroRow; macroRow = macroRow -> nextMacro) {
				if (strcmp (word ,macroRow -> name) == 0) {
					printf ("Error 5 in line %d: label and macro with the same name\n" , lineNumber);
					errorFlag = true;
					currErrorFlag = true;
				}
			}			
			for (labelRow = labelHead; labelRow != currLabelRow; labelRow = labelRow -> nextLabel) {
				if (strcmp (word ,labelRow -> name) == 0) {
					printf ("Error 5 in line %d: more then one label with the same name\n" , lineNumber);
					errorFlag = true;
					currErrorFlag = true;
				}
			}
			if (!(isValidName (word))) {
				printf ("Error 6 in line %d: label's name isn't valid\n" , lineNumber);
				errorFlag = true;
				currErrorFlag = true;
			}
			if (! (currErrorFlag)) {
				strcpy (currLabelRow -> name , word);
				currLabelRow -> nextLabel = malloc(sizeof(label));
				currLabelRow -> address = address;
			}
			word = strtok(NULL, " ");
			if (word == NULL) {
				printf ("Error 11 in line %d: empty label\n" , lineNumber);
				errorFlag = true;
				continue;
			}
		} /*End of labels case*/
		currCodeRow -> address = address;
		address++;
		if (word == NULL)
			continue;

		/*Commands*/
		word = removeSpaces (word);
		for (com = 0; com < NUM_OF_COMMANDS; com++) {
			if (strcmp(word , comList[com]) == 0) {
				commandFlag = true;
				IC++;
				currCodeRow -> binCode = com << 6; /*The location of the command in the binary code*/
				if (com == 14 || com == 15) { /*rts or hlt*/
					if (strtok(NULL, currLine) != NULL) {
						printf ("Error 7 in line %d: too much operands\n" , lineNumber);
						errorFlag = true;
						currErrorFlag = true;
					}
					break;
				}
				word = strtok(NULL, ",");
				if (word == NULL) {
					printf ("Error 8 in line %d: not enough operands\n" , lineNumber);
					errorFlag = true;
					currErrorFlag = true;
					break;
				}
				word = removeSpaces (word);
				/*Addressing method 0 - number*/
				if (word[0] == '#') {
					firstOpIsNumber = true;
					word[0] = ' ';
					for (i = 1; i < strlen(word) - 1; i++) {
						if (! (isdigit(word[i]) || (i == 1 && (word[i] == '+' || word[i] == '-')))) {
        					printf ("Error 9 in line %d: number isn't valid\n" , lineNumber);
							errorFlag = true;
							currErrorFlag = true;
							break;
						}
					}
					if (com != 0 && com != 1 && com != 2 && com != 3 && com != 12) { /*Other commands 																	mustn't have number as their first operand*/
						printf ("Error 10 in line %d: the addressing method isn't valid\n" , lineNumber);
						errorFlag = true;
						currErrorFlag = true;
					}
					if (com == 12) { /*This command must have only one operand*/
							if (strtok (NULL, currLine) != NULL) {
								printf ("Error 7 in line %d: too much operands\n" , lineNumber);
								errorFlag = true;
								currErrorFlag = true;
							}
							if (! (currErrorFlag)) {
								/*The code of numbers is 0 so there is no need to update the binary code*/
								currCodeRow -> nextLine = malloc(sizeof(code));
								if (word[1] != '-')
									currCodeRow -> nextLine -> binCode = atoi(word) << 2;
								else
									currCodeRow -> nextLine -> binCode = (EIGHT_BITS + atoi(word)) << 2;
								currCodeRow -> nextLine -> address = address;
								address++;
								IC++;
								currCodeRow -> nextLine -> nextLine = malloc(sizeof(code));
								currCodeRow = currCodeRow -> nextLine -> nextLine;
							}
							break;
					}
					if (! (currErrorFlag)) {
						/*The code of numbers is 0 so there is no need to update the binary code*/
						currCodeRow -> nextLine = malloc(sizeof(code));
						currCodeRow -> nextLine -> binCode = atoi(word);
						currCodeRow -> nextLine -> address = address;
						address++;
						IC++;
					}
				}
				/*Addressing method 3 - register*/
				for (reg = 0; reg < NUM_OF_REGISTERS; reg++) {
					if (strcmp(word , regList[reg]) == 0) {
						firstOpIsRegister = true;
						if (com == 6) { /*The command is lea which mustn't have register as its first 																									   operand*/
							printf ("Error 10 in line %d: the addressing method isn't valid\n", lineNumber);
							errorFlag = true;
							currErrorFlag = true;
						}
						if (com != 0 && com != 1 && com != 2 && com != 3) { /*other commands must have only 																							   one operand*/
							if (strtok (NULL, currLine) != NULL) {
								printf ("Error 7 in line %d: too much operands\n" , lineNumber);
								errorFlag = true;
								currErrorFlag = true;
							}
							if (! (currErrorFlag)) {
								currCodeRow -> binCode += (3 << 2); /*3 is the code of registers and it 								   puts this code in the location of the target operand in the binary code*/
								currCodeRow -> nextLine = malloc(sizeof(code));
								currCodeRow -> nextLine -> binCode = reg << 2; /*The location of register 																				  when it's the target operand*/
								currCodeRow -> nextLine -> address = address;
								address++;
								IC++;
								currCodeRow -> nextLine -> nextLine = malloc(sizeof(code));
								currCodeRow = currCodeRow -> nextLine -> nextLine;
							}
							break;
						}
						else if (! (currErrorFlag)) { /*Those commands have also a source operand*/
							currCodeRow -> binCode += (3 << 4); /*3 is the code of registers and it puts 											this code in the location of the source operand in the binary code*/
							currCodeRow -> nextLine = malloc(sizeof(code));
							currCodeRow -> nextLine -> binCode = reg << 6; /*The location of register when 																					   it's the source operand*/
							currCodeRow -> nextLine -> address = address;
							address++;
							IC++;
						}
					}
				}
				/*Addressing method 1 - label*/
				if (!(firstOpIsNumber || firstOpIsRegister)) {
					/*Addressing method 2 - struct*/
					if (word[strlen (word) - 2] == '.' && (word[strlen (word) - 1] == '1' || 																			  word[strlen (word) - 1] == '2')) {
						firstOpIsStruct = true;
						if (com != 0 && com != 1 && com != 2 && com != 3 && com != 6) { /*other commands 																					must have only one operand*/
							if (strtok (NULL, currLine) != NULL) {
								printf ("Error 7 in line %d: too much operands\n" , lineNumber);
								errorFlag = true;
								currErrorFlag = true;
							}
							if (! (currErrorFlag)) {
								currCodeRow -> binCode += (2 << 2); /*2 is the code of structs and it puts 											this code in the location of the target operand in the binary code*/
								currCodeRow -> nextLine = malloc(sizeof(code));
								strcpy (currCodeRow -> nextLine -> label , word);
								currCodeRow -> nextLine -> isStruct = 1;
								currCodeRow -> nextLine -> label [strlen (word) - 2] = '\0'; /*now there is 													  just the name of the struct without the ".1" or ".2"*/
								currCodeRow -> nextLine -> address = address;
								address++;
								IC++;

								currCodeRow -> nextLine -> nextLine = malloc(sizeof(code));
								currCodeRow -> nextLine -> nextLine -> binCode = (word[strlen (word)-1]-'0')
																									   << 2;
															/*The location of the field number of a struct*/
								currCodeRow -> nextLine -> nextLine -> address = address;
								address++;
								IC++;
								currCodeRow -> nextLine -> nextLine -> nextLine = malloc(sizeof(code));
								currCodeRow = currCodeRow -> nextLine -> nextLine -> nextLine;
							}
							break;
						}
						else if (! (currErrorFlag)) { /*Those commands have also a source operand*/
							currCodeRow -> binCode += (2 << 4); /*2 is the code of structs and it puts this 											 code in the location of the source operand in the binary code*/
							currCodeRow -> nextLine = malloc(sizeof(code));
							strcpy (currCodeRow -> nextLine -> label , word);
							currCodeRow -> nextLine -> isStruct = 1;
							currCodeRow -> nextLine -> label [strlen (word) - 2] = '\0'; /*now there is 													  just the name of the struct without the ".1" or ".2"*/
							currCodeRow -> nextLine -> address = address;
							address++;
							IC++;

							currCodeRow -> nextLine -> nextLine = malloc(sizeof(code));
							currCodeRow -> nextLine -> nextLine -> binCode = (word[strlen (word)-1] - '0') 																										   << 2;
															/*The location of the field number of a struct*/
							currCodeRow -> nextLine -> nextLine -> address = address;
							address++;
							IC++;
						}
					} /*end of struct case*/
					else if (com != 0 && com != 1 && com != 2 && com != 3 && com != 6) { /*other commands 																					must have only one operand*/
						if (strtok (NULL, currLine) != NULL) {
							printf ("Error 7 in line %d: too much operands\n" , lineNumber);
							errorFlag = true;
							currErrorFlag = true;
						}
						if (! (currErrorFlag)) {
							currCodeRow -> binCode += (1 << 2); /*1 is the code of lables and it puts this 												 code in the location of the target operand in the binary code*/
							currCodeRow -> nextLine = malloc(sizeof(code));
							strcpy (currCodeRow -> nextLine -> label , word);
							currCodeRow -> nextLine -> address = address;
							address++;
							IC++;
							currCodeRow -> nextLine -> nextLine = malloc(sizeof(code));
							currCodeRow = currCodeRow -> nextLine -> nextLine;
						}
						break;
					}
					else if (! (currErrorFlag)) { /*These commands have also a source operand*/
						currCodeRow -> binCode += (1 << 4); /*1 is the code of lables and it puts this code 												  in the location of the source operand in the binary code*/
						currCodeRow -> nextLine = malloc(sizeof(code));
						strcpy (currCodeRow -> nextLine -> label , word);
						currCodeRow -> nextLine -> address = address;
						address++;
						IC++;
					}
				} /*End of first operand case*/

				/*second operand*/
				if (com == 0 || com == 1 || com == 2 || com == 3 || com == 6) { /*These commands have 2 																								  operands*/
    				word = strtok(NULL , ",");
					if (word == NULL) {
						printf ("Error 8 in line %d: not enough operands\n" , lineNumber);
						errorFlag = true;
						currErrorFlag = true;
						break;
					}
					word = removeSpaces (word);
					/*Second operand with addressing method 0 - number*/
					if (word[0] == '#') {
						secOpIsNumber = true;
						word[0] = ' ';
						for (i = 1; i < strlen(word) - 1; i++) {
							if (! (isdigit(word[i]) || (i == 1 && (word[i] == '+' || word[i] == '-')))) {
    	    					printf ("Error 9 in line %d: number isn't valid\n" , lineNumber);
								errorFlag = true;
								currErrorFlag = true;
								break;
							}
						}
						if (com != 1) { /*Other commands mustn't have number as their second operand*/
							printf ("Error 10 in line %d: the addressing method isn't valid\n", lineNumber);
							errorFlag = true;
							currErrorFlag = true;
						}
						if (! (currErrorFlag)) {
							/*The code of numbers is 0 so there is no need to update the binary code*/
							if (firstOpIsStruct) {
								currCodeRow -> nextLine -> nextLine -> nextLine = malloc(sizeof(code));
								if (word[0] != '-')
									currCodeRow -> nextLine -> nextLine -> nextLine -> binCode = atoi(word) 																									   << 2;
								else
									currCodeRow -> nextLine -> nextLine -> nextLine -> binCode =
																		   (EIGHT_BITS + (atoi(word))) << 2;
								currCodeRow -> nextLine -> nextLine -> nextLine -> address = address;
								address++;
								IC++;
								currCodeRow -> nextLine -> nextLine -> nextLine -> nextLine
																					 = malloc(sizeof(code));
								currCodeRow = currCodeRow -> nextLine -> nextLine -> nextLine -> nextLine;
							}
							else {
								currCodeRow -> nextLine -> nextLine = malloc(sizeof(code));
								currCodeRow -> nextLine -> nextLine -> binCode = atoi(word) << 2;
								currCodeRow -> nextLine -> nextLine -> address = address;
								address++;
								IC++;
								currCodeRow -> nextLine -> nextLine -> nextLine = malloc(sizeof(code));
								currCodeRow = currCodeRow -> nextLine -> nextLine -> nextLine;
							}
						}
					}
					/*Second operand with addressing method 3 - register*/
					for (reg = 0; reg < NUM_OF_REGISTERS; reg++) {
						if (strcmp(word , regList[reg]) == 0) {
							secOpIsRegister = true;
							currCodeRow -> binCode += (3 << 2); /*3 is the code of registers and it puts 											this code in the location of the target operand in the binary code*/
							if (firstOpIsStruct) {
								currCodeRow -> nextLine -> nextLine -> nextLine = malloc(sizeof(code));
								currCodeRow -> nextLine -> nextLine -> nextLine -> binCode = reg << 2;
												   /*The location of register when it's the target operand*/
								currCodeRow -> nextLine -> nextLine -> nextLine -> address = address;
								address++;
								IC++;
								currCodeRow -> nextLine -> nextLine -> nextLine -> nextLine
																					 = malloc(sizeof(code));
								currCodeRow = currCodeRow -> nextLine -> nextLine -> nextLine -> nextLine;
							}
							else if (firstOpIsRegister) {
								currCodeRow -> nextLine -> binCode += (reg << 2); /*The location of 																	 register when it's the target operand*/
								currCodeRow -> nextLine -> nextLine = malloc(sizeof(code));
								currCodeRow = currCodeRow -> nextLine -> nextLine;
							}
							else {
								currCodeRow -> nextLine -> nextLine = malloc(sizeof(code));
								currCodeRow -> nextLine -> nextLine -> binCode = reg << 2; /*The location 																	  of register when it's the target operand*/
								currCodeRow -> nextLine -> nextLine -> address = address;
								address++;
								IC++;
								currCodeRow -> nextLine -> nextLine -> nextLine = malloc(sizeof(code));
								currCodeRow = currCodeRow -> nextLine -> nextLine -> nextLine;
							}
						}
					}
					/*Second operand with addressing method 1 - label*/
					if (!(secOpIsNumber || secOpIsRegister)) {
						/*Second operand with addressing method 2 - struct*/
						if (word[strlen (word) - 2] == '.' && (word[strlen (word) - 1] == '1' || 																			  word[strlen (word) - 1] == '2')) {
							currCodeRow -> binCode += (2 << 2); /*2 is the code of structs and it puts 											this code in the location of the target operand in the binary code*/
							if (firstOpIsStruct) {
								currCodeRow -> nextLine -> nextLine -> nextLine = malloc(sizeof(code));
								strcpy (currCodeRow -> nextLine -> nextLine -> nextLine -> label , word);
								currCodeRow -> nextLine -> nextLine -> nextLine -> isStruct = 1;
								currCodeRow -> nextLine -> nextLine -> nextLine ->
																		   label [strlen (word) - 2] = '\0';
									   /*now there is just the name of the struct without the ".1" or ".2"*/
								currCodeRow -> nextLine -> nextLine -> nextLine -> address = address;
								address++;
								IC++;

								currCodeRow -> nextLine -> nextLine -> nextLine -> nextLine 
																					 = malloc(sizeof(code));
								currCodeRow -> nextLine -> nextLine -> nextLine -> nextLine -> binCode = 
																	   (word[strlen (word) - 1] - '0') << 2;
															/*The location of the field number of a struct*/
								currCodeRow -> nextLine -> nextLine -> nextLine -> nextLine ->
																						  address = address;
								address++;
								IC++;
								currCodeRow -> nextLine -> nextLine -> nextLine -> nextLine -> nextLine
																					 = malloc(sizeof(code));
								currCodeRow = currCodeRow -> nextLine -> nextLine -> nextLine -> nextLine
																								-> nextLine;
							}
							else {
								currCodeRow -> nextLine -> nextLine = malloc(sizeof(code));
								strcpy (currCodeRow -> nextLine -> nextLine -> label , word);
								currCodeRow -> nextLine -> nextLine -> isStruct = 1;
								currCodeRow -> nextLine -> nextLine -> label [strlen (word) - 2] = '\0';
								   	   /*now there is just the name of the struct without the ".1" or ".2"*/
								currCodeRow -> nextLine -> nextLine -> address = address;
								address++;
								IC++;

								currCodeRow -> nextLine -> nextLine -> nextLine = malloc(sizeof(code));
								currCodeRow -> nextLine -> nextLine -> nextLine -> binCode = 
																	   (word[strlen (word) - 1] - '0') << 2;
															/*The location of the field number of a struct*/
								currCodeRow -> nextLine -> nextLine -> nextLine -> address = address;
								address++;
								IC++;
								currCodeRow -> nextLine -> nextLine -> nextLine -> nextLine
																					 = malloc(sizeof(code));
								currCodeRow = currCodeRow -> nextLine -> nextLine -> nextLine -> nextLine;
							}
						} /*End of struct case*/
						else {
							currCodeRow -> binCode += (1 << 2); /*1 is the code of lables and it puts this 												 code in the location of the target operand in the binary code*/
							if (firstOpIsStruct) {
								currCodeRow -> nextLine -> nextLine -> nextLine = malloc(sizeof(code));
								strcpy (currCodeRow -> nextLine -> nextLine -> nextLine -> label , word);
								currCodeRow -> nextLine -> nextLine -> nextLine -> address = address;
								address++;
								IC++;
								currCodeRow -> nextLine -> nextLine -> nextLine -> nextLine
																					 = malloc(sizeof(code));
								currCodeRow = currCodeRow -> nextLine -> nextLine -> nextLine -> nextLine;
							}
							else {
								currCodeRow -> nextLine -> nextLine = malloc(sizeof(code));
								strcpy (currCodeRow -> nextLine -> nextLine -> label , word);
								currCodeRow -> nextLine -> nextLine -> address = address;
								address++;
								IC++;
								currCodeRow -> nextLine -> nextLine -> nextLine = malloc(sizeof(code));
								currCodeRow = currCodeRow -> nextLine -> nextLine -> nextLine;
							}
						}
					} /*End of label case*/
				} /*End of second operand case*/
			} /*End of if*/
		} /*End of commands case*/
		
		/*Data instructions*/
		/*Instruction 1 - .data*/
		if (strcmp(word , ".data") == 0) {
			word = strtok(NULL, ",");
			if (word == NULL) {
				printf ("Error 8 in line %d: not enough operands\n" , lineNumber);
				errorFlag = true;
				continue;
			}
			while (word != NULL) {
				word = removeSpaces (word);
				for (i = 0; i < strlen(word); i++) {
					if (! (isdigit(word[i]) || (i == 0 && (word[i] == '+' || word[i] == '-')))) {
        				printf ("Error 9 in line %d: number isn't valid\n" , lineNumber);
						errorFlag = true;
						currErrorFlag = true;
						break;
					}
				}
				if (! (currErrorFlag)) {
					if (word[0] != '-')
						currCodeRow -> binCode = atoi(word);
					else
						currCodeRow -> binCode = TEN_BITS + atoi(word);
					currCodeRow -> nextLine = malloc(sizeof(code));
					currCodeRow = currCodeRow -> nextLine;
					currCodeRow -> address = address;
					DC++;
					address++;
					word = strtok(NULL, ",");
				}
				else
					break;
			}
			address--;
		}
		/*Instruction 2 - .string*/
		else if (strcmp(word , ".string") == 0) {
			word = strtok(NULL, ",");
			if (word == NULL) {
				printf ("Error 8 in line %d: not enough operands\n" , lineNumber);
				errorFlag = true;
				continue;
			}
			word = removeSpaces (word);

			if (word[0] != '"' || word[strlen(word) - 1] != '"') {
	       		printf ("Error 12 in line %d: string isn't valid\n" , lineNumber);
				errorFlag = true;
				continue;
			}
			word[0] = ' ';
			word[strlen(word) - 1] = ' ';
			word = removeSpaces (word);
			for (i = 0; i < strlen(word); i++) {
				currCodeRow -> binCode = word[i];
				currCodeRow -> nextLine = malloc(sizeof(code));
				currCodeRow = currCodeRow -> nextLine;
				currCodeRow -> address = address;
				DC++;
				address++;
			}
			currCodeRow -> binCode = 0; /*The value of '\0' which is the last character of the string*/
			currCodeRow -> nextLine = malloc(sizeof(code));
			currCodeRow = currCodeRow -> nextLine;
			currCodeRow -> address = address;
			DC++;
		}
		/*Instruction 3 - .struct*/
		else if (strcmp(word , ".struct") == 0) {
			word = strtok(NULL, ",");
			if (word == NULL) {
				printf ("Error 8 in line %d: not enough operands\n" , lineNumber);
				errorFlag = true;
				continue;
			}
			word = removeSpaces (word);
			for (i = 0; i < strlen(word); i++) {
				if (! (isdigit(word[i]) || (i == 0 && (word[i] == '+' || word[i] == '-')))) {
        			printf ("Error 9 in line %d: number isn't valid\n" , lineNumber);
					errorFlag = true;
					currErrorFlag = true;
					break;
				}
			}
			if (! (currErrorFlag)) {
				if (word[0] != '-')
					currCodeRow -> binCode = atoi(word);
				else
					currCodeRow -> binCode = TEN_BITS + atoi(word);
				currCodeRow -> nextLine = malloc(sizeof(code));
				currCodeRow = currCodeRow -> nextLine;
				currCodeRow -> address = address;
				DC++;
				address++;
			}
			word = strtok(NULL, ",");
			if (word == NULL) {
				printf ("Error 8 in line %d: not enough operands\n" , lineNumber);
				errorFlag = true;
				currErrorFlag = true;
				continue;
			}
			word = removeSpaces (word);

			if (word[0] != '"' || word[strlen(word) - 1] != '"') {
	       		printf ("Error 12 in line %d: string isn't valid\n" , lineNumber);
				errorFlag = true;
				continue;
			}
			word[0] = ' ';
			word[strlen(word) - 1] = ' ';
			word = removeSpaces (word);
			if (! (currErrorFlag)) {
				for (i = 0; i < strlen(word); i++) {
					currCodeRow -> binCode = word[i];
					currCodeRow -> nextLine = malloc(sizeof(code));
					currCodeRow = currCodeRow -> nextLine;
					currCodeRow -> address = address;
					DC++;
					address++;
				}
				currCodeRow -> binCode = 0; /*The value of '\0' which is the last character of the string*/
				currCodeRow -> nextLine = malloc(sizeof(code));
				currCodeRow = currCodeRow -> nextLine;
				currCodeRow -> address = address;
				DC++;
				if (isLabel)
					strcpy (currLabelRow -> type , "struct");
			}
		}
		/*Instruction 4 - .extern*/
		else if (strcmp(word , ".extern") == 0) {
			externFlag = true;
			address--;
			word = strtok(NULL, ",");
			if (word == NULL) {
				printf ("Error 13 in line %d: extern instruction without label to import\n" , lineNumber);
				errorFlag = true;
				continue;
			}
			if (isLabel) {
				printf ("Error 14 in line %d: a label statement and an external label in the same line\n" , 																								lineNumber);
				errorFlag = true;
				continue;
			}
			word = removeSpaces(word);
			if (strlen (word) >= MACRO_LENGTH) {
			printf ("Error 3 in line %d: label's name is longer then maximum valid length\n" , lineNumber);
			errorFlag = true;
			currErrorFlag = true;
			}
			for (macroRow = macroHead; macroRow != currMacroRow; macroRow = macroRow -> nextMacro) {
				if (strcmp (word ,macroRow -> name) == 0) {
					printf ("Error 5 in line %d: label and macro with the same name\n" , lineNumber);
					errorFlag = true;
					currErrorFlag = true;
					break;
				}
			}	
			for (labelRow = labelHead; labelRow != currLabelRow; labelRow = labelRow -> nextLabel) {
				if (strcmp (word ,labelRow -> name) == 0) {
					printf ("Error 5 in line %d: more then one label with the same name\n" , lineNumber);
					errorFlag = true;
					currErrorFlag = true;
					break;
				}
			}
			if (!(isValidName (word))) {
				printf ("Error 6 in line %d: label's name isn't valid\n" , lineNumber);
				errorFlag = true;
				continue;
			}
			if (strtok (NULL, currLine) != NULL) {
				printf ("Error 4 in line %d: label has more then one name\n" , lineNumber);
				errorFlag = true;
				continue;
			}
			if (! (currErrorFlag)) {
				isLabel = true;
				strcpy (currLabelRow -> name , word);
				currLabelRow -> nextLabel = malloc(sizeof(label));
				currLabelRow -> address = 0; /*external label's address is 0*/
				strcpy (currLabelRow -> type , "extern");
			}
		}
		/*Instruction 5 - .entry*/
		else if (strcmp(word , ".entry") == 0) {
			entryFlag = true;
			address--;
			word = strtok(NULL, ",");
			if (word == NULL) {
				printf ("Error 13 in line %d: entry instruction without label to export\n" , lineNumber);
				errorFlag = true;
				continue;
			}
			if (isLabel) {
				printf ("Error 14 in line %d: a label statement and an entry statement in the same line\n" ,
																								lineNumber);
				errorFlag = true;
				continue;
			}
			word = removeSpaces(word);
			if (strtok (NULL, currLine) != NULL) {
				printf ("Error 4 in line %d: entry's label has more then one name\n" , lineNumber);
				errorFlag = true;
				continue;
			}
			strcpy (currEntryRow -> name , word);
			currEntryRow -> nextLine = malloc(sizeof(entry));
			currEntryRow = currEntryRow -> nextLine;
		}

		else if (! (commandFlag)) {
			printf ("Error 15 in line %d: name of command doesn't exist\n" , lineNumber);
			errorFlag = true;
		}

		if (isLabel)
			currLabelRow = currLabelRow -> nextLabel;
	}
}

void pass2 () {
	bool definedLabel;
	for (codeRow = codeHead; codeRow != currCodeRow; codeRow = codeRow -> nextLine) {
		if (! (strcmp (codeRow -> label , "") == 0)) {
			definedLabel = false;
			for (labelRow = labelHead; labelRow != currLabelRow; labelRow = labelRow -> nextLabel) {
				if (strcmp (codeRow -> label , labelRow -> name) == 0) {
					definedLabel = true;
					if (((codeRow -> isStruct) == 0) && (strcmp (labelRow -> type , "struct") == 0)) {
						printf ("Error 16: %s is defined as a struct but the reference to it isn't as a struct\n" , codeRow -> label);
						errorFlag = true;
					}
					if (((codeRow -> isStruct) != 0) && ((strcmp (labelRow -> type , "struct") != 0))) {
						printf ("Error 17: %s isn't defined as a struct but the reference to it is as a struct\n" , codeRow -> label);
						errorFlag = true;
					}
					if (! (errorFlag) && (strcmp (labelRow -> type , "extern") != 0))
						codeRow -> binCode += ((labelRow -> address) << 2) + 2; /*The location of the 															  address of a not extern label in the binary code*/
					else if (! (errorFlag)) {
						codeRow -> binCode += ((labelRow -> address) << 2) + 1; /*The location of the 																 address of an extern label in the binary code*/
						currExternRow -> address = codeRow -> address;
						strcpy (currExternRow -> label , labelRow -> name);
						currExternRow -> nextLine = malloc(sizeof(externy));
						currExternRow = currExternRow -> nextLine;
					}
					break;
				}
			}
			if (! (definedLabel)) {
				printf ("Error 18: %s wasn't define\n" , codeRow -> label);
				errorFlag = true;
			}
		}
	}
	for (entryRow = entryHead; entryRow != currEntryRow; entryRow = entryRow -> nextLine) {
		definedLabel = false;
		for (labelRow = labelHead; labelRow != currLabelRow; labelRow = labelRow -> nextLabel) {
			if (strcmp (entryRow -> name , labelRow -> name) == 0) {
				entryRow -> address = labelRow -> address;
				definedLabel = true;
				break;
			}
		}
		if (! (definedLabel)) {
			printf ("Error 18: %s wasn't define\n" , entryRow -> name);
			errorFlag = true;
		}
	}
}
