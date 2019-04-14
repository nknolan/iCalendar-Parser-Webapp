#ifndef PARSER_UTILS_H
#define PARSER_UTILS_H

/*
 * @file CalendarParserUtilities.h
 * @author Nicholas Nolan
 * 0914780
 * @date February 2019
 * @brief Header for the additional functions in CalendarParser.c
 */

#include "CalendarParser.h"

void chop(char * str);

DateTime parseDateTime(char * body);

Property * parseProperty(char * name, char * body);

char * unfoldLine(char buffer[][90], int * currLinePtr, int size);

void writeProperty(ListIterator propItr, FILE * fp);

#endif
