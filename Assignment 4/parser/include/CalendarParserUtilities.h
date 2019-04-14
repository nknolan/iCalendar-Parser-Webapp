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

char * alarmListToJSON(const List * alarmList);

char * alarmToJSON(const Alarm * alarm);

char * propertyListToJSON(const List * propertyList);

char * propertyToJSON(const Property * property);

char * unfoldLine(char buffer[][90], int * currLinePtr, int size);

DateTime parseDateTime(char * body);

Property * parseProperty(char * name, char * body);

void addEventToCalendar(char * fileName, char * eventString, char * buffer);

void createUserCalendar(char * fileName, char * calendarJSON, char * eventJSON, char * buffer);

void chop(char * str);

void getAlarmListJSON(char * fileName, char * eventID, char * buffer);

void getCalendarJSON(char * fileName, char * buffer);

void getEventListJSON(char * fileName, char * buffer);

void getOptionalPropertiesJSON(char * fileName, char * eventID, char * buffer);

void writeProperty(ListIterator propItr, FILE * fp);

#endif