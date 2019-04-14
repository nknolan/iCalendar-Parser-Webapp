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

/** Function to remove the trailing /r/n from a string
 *@pre str exists, is not null
 *@post str trailing characters have been removed
 *@return none
 *@param str
**/
void chop(char * str);

void parseDateTime(DateTime dt, char * body);

Property * parseProperty(char * name, char * body);

char * unfoldLine(char buffer[][90], int * currLinePtr, int size);

#endif
