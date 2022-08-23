#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include "functions.h"

#define POSTLEN 4
#define NAMELIST_LENGTH 29

char* cpyncat (char* name, char* postfix) {
	char* united = malloc (sizeof(char) * (strlen(name) + POSTLEN));
	strcpy (united , name);
	strcat (united, postfix);
	return united;
}

bool isValidName (char* name) {
	int i;
	char* nameList[] = {"r0" , "r1" , "r2" , "r3" , "r4" , "r5" , "r6" , "r7" , "data" , 									   "string" , "struct" , "entry" , "extern" , "lea" , "sub" , "add" , 										   "cmp" , "mov" , "not" , "clr" , "inc" , "dec" , "jmp" , "bne" , 										   "get" , "prn" , "jsr" , "rts" , "hlt"};
	if (!(isalpha(name[0])))
		return false;

	for (i = 0; i < strlen(name) - 1; i++) {
		if (!(isalnum(name[i])))
			return false;
	}

	for (i = 0; i < NAMELIST_LENGTH; i++) {
		if (strcmp (nameList[i] , name) == 0)
			return false;
	}
	return true;
}

char* removeSpaces (char* str) {
	int end = strlen (str);
	int start = 0;
	char* returns;
	while (str[end - 1] == ' ' || str[end - 1] == '\n' || str[end - 1] == '\t') {
		str[end - 1] = '\0';
		end--;
	}
	returns = str;
	while (str[start] == ' ' || str[start] == '\t') {
		returns = &(str[start + 1]);
		start++;
	}
	return returns;
}

char* binTo32 (int binCode) {
    char str[] = "  ";
	char* base32;
	char code[] = "!@#$%^&*<>abcdefghijklmnopqrstuv";
	str[0] = code [binCode >> 5]; /*The 5 left bits*/
	str[1] = code [binCode & 31]; /*The 5 right bits*/
	str[2] = '\0';
	base32 = str;
	return base32;
}
