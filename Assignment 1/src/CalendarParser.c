/*
 * @file CalendarParser.c
 * @author Nicholas Nolan
 * 0914780
 * @date February 2019
 * @brief iCalendar parser file
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CalendarParser.h"
#include "CalendarParserUtilities.h"
#include "LinkedListAPI.h"

ICalErrorCode createCalendar(char* fileName, Calendar** obj) {
    if(fileName == NULL || strlen(fileName) == 0) {
        return INV_FILE;
    }
    int fileNameLength = strlen(fileName);
    char * extension = &fileName[fileNameLength - 4];
    if(strcmp(extension, ".ics") != 0 || fileNameLength <= 4) {
        return INV_FILE;
    }
    ICalErrorCode error = OK;
    Calendar * newCalendar = malloc(sizeof(Calendar));
    newCalendar->properties = initializeList(&printProperty, &deleteProperty, &compareProperties);
    newCalendar->events = initializeList(&printEvent, &deleteEvent, &compareEvents);

    char line[90];
    FILE * fp = fopen(fileName, "r");
    if(fp == NULL) {
        deleteCalendar(newCalendar);
        return INV_FILE;
    }

    //Reads the entire iCal file into static memory because that's easier
    char currChar = 0;
    int numLines = 0;
    while((currChar = fgetc(fp)) != EOF) {
        if(currChar == '\n') {
            numLines++;
        }
    }
    char buffer[numLines + 1][90];//There's an OB1 bug in unfoldLine();
    strcpy(buffer[numLines], "0");//This was a 30-second fix
    int size = ftell(fp);
    rewind(fp);
    int j = 0;
    while(fgets(line, 90, fp) != NULL) {
        if(strlen(line) > 80) {
            error = INV_FILE;
        }
        strcpy(buffer[j], line);
        j++;
    }
    if(strcmp(buffer[0], "BEGIN:VCALENDAR\r\n") != 0 || strcmp(buffer[numLines - 1], "END:VCALENDAR\r\n") != 0) {
        deleteCalendar(newCalendar);
        fclose(fp);
        return INV_CAL;
    }

    //Some additional variables
    char name[1001];
    char * body;//only a pointer to part of fullLine

    int currentLine = 0;
    int * currLinePtr = &currentLine;
    int duplicateVersion = 0;
    int duplicateID = 0;
    int creationFlag = 0;
    int startFlag = 0;

    const char colon = ':';
    const char semicolon = ';';
    char * delimeter;

    float versionNumber = 0.0;

    while(currentLine < numLines) {
        char * fullLine = unfoldLine(buffer, currLinePtr, size);
        strncpy(name, fullLine, 1000);

        //Tokenize before and after first : or ;
        delimeter = strchr(name, semicolon);
        if(delimeter != NULL) {
            body = delimeter + 1;
            *(delimeter) = '\0';
        } else {
            delimeter = strchr(name, colon);
            if(delimeter != NULL) {
                body = delimeter + 1;
                *(delimeter) = '\0';
            }
        }

        //Get version number
        if(strncmp(fullLine, ";", 1) == 0) {
            ;//ignore full-line comments
        } else if(strcmp(name, "VERSION") == 0) {
            versionNumber = atof(body);
            if(versionNumber == 0 && error == OK) {//Returns zero on failure
                error = INV_VER;
            } else if(duplicateVersion > 0 && error == OK) {
                error = DUP_VER;
            } else {
                newCalendar->version = versionNumber;
                duplicateVersion++;
            }
        //Get Product ID
        } else if(strcmp(name, "PRODID") == 0) {
            if((body == NULL || strlen(body) == 0) && error == OK) {
                error = INV_PRODID;
            } else {
                strcpy(newCalendar->prodID, body);
                duplicateID++;
            }
            //Parse Events, Alarms, etc.
        } else if(strncmp(name, "BEGIN", 5) == 0) {
            if(strncmp(fullLine, "BEGIN:VCALENDAR", 15) == 0) {
                ;
            //Parse as Event
            } else if(strncmp(fullLine, "BEGIN:VEVENT", 12) == 0) {
                Event * newEvent= (Event*)malloc(sizeof(Event));
                newEvent->properties = initializeList(&printProperty, &deleteProperty, &compareProperties);
                newEvent->alarms = initializeList(&printAlarm, &deleteAlarm, &compareAlarms);
                while(strncmp(fullLine, "END:VEVENT", 10) != 0) {
                    currentLine++;
                    char * fullLine = unfoldLine(buffer, currLinePtr, size);
                    strncpy(name, fullLine, 1000);

                    //Tokenize before and after first : or ;
                    delimeter = strchr(name, semicolon);
                    if(delimeter != NULL) {
                        body = delimeter + 1;
                        *(delimeter) = '\0';
                    } else {
                        delimeter = strchr(name, colon);
                        if(delimeter != NULL) {
                            body = delimeter + 1;
                            *(delimeter) = '\0';
                        }
                    }

                    //Parse as Alarm
                    if(strncmp(fullLine, "BEGIN:VALARM", 12) == 0) {
                        Alarm * newAlarm = (Alarm*)malloc(sizeof(Alarm));
                        newAlarm->properties = initializeList(&printProperty, &deleteProperty, &compareProperties);

                        while(strncmp(fullLine, "END:VALARM", 10) != 0) {
                            currentLine++;
                            char * fullLine = unfoldLine(buffer, currLinePtr, size);
                            strncpy(name, fullLine, 1000);

                            //Tokenize
                            delimeter = strchr(name, semicolon);
                            if(delimeter != NULL) {
                                body = delimeter + 1;
                                *(delimeter) = '\0';
                            } else {
                                delimeter = strchr(name, colon);
                                if(delimeter != NULL) {
                                    body = delimeter + 1;
                                    *(delimeter) = '\0';
                                }
                            }

                            if(strncmp(fullLine, "END:VALARM", 10) == 0) {
                                free(fullLine);
                                break;
                            } else if((strncmp(fullLine, "END:VEVENT", 10) == 0 || strncmp(fullLine, "END:VCALENDAR", 13) == 0) && error == OK) {
                                error = INV_ALARM;
                                free(fullLine);
                                break;
                            }else if(strncmp(name, "TRIGGER", 7) == 0) {
                                char * trigger = (char*)malloc(sizeof(char) * (strlen(body) + 1));
                                strcpy(trigger, body);
                                newAlarm->trigger = trigger;
                            } else if(strncmp(name, "ACTION", 6) == 0) {
                                strncpy(newAlarm->action, body, 199);
                            } else {
                                Property * newProperty = parseProperty(name, body);
                                insertBack(newAlarm->properties, newProperty);
                            }

                            free(fullLine);
                        }

                        if((strlen(newAlarm->action) == 0 || newAlarm->trigger == NULL) && error == OK) {
                            error = INV_ALARM;
                        }
                        insertBack(newEvent->alarms, newAlarm);
                    }//end of alarm branch

                    //Other Event components
                    if(strncmp(fullLine, "END:VEVENT", 10) == 0) {
                        free(fullLine);
                        break;//end of event entry
                    } else if(strncmp(fullLine, "END:VCALENDAR", 13) == 0 && error == OK) {
                        error = INV_EVENT;
                        free(fullLine);
                        break;
                    } else if(strncmp(name, "UID", 3) == 0) {
                        strncpy(newEvent->UID, body, 999);
                    } else if(strncmp(name, "DTSTAMP", 7) == 0) {
                        for(int i = 0; i < strlen(body); i++) {
                            if(((i == 8 && body[i] != 'T') || (i == 15 && body[i] != 'Z' && isalnum(body[i]))) && error == OK) {
                                error = INV_DT;
                            }
                            if((i != 8 && i < 15) && !isdigit(body[i]) && error == OK) {
                                error = INV_DT;
                            }
                        }
                        if(error == OK) {
                            parseDateTime(newEvent->creationDateTime, body);
                            creationFlag = 1;
                        }
                    } else if(strncmp(name, "DTSTART", 7) == 0) {
                        for(int i = 0; i < strlen(body); i++) {
                            if(((i == 8 && body[i] != 'T') || (i == 15 && body[i] != 'Z' && isalnum(body[i]))) && error == OK) {
                                error = INV_DT;
                            }
                            if((i != 8 && i < 15) && !isdigit(body[i]) && error == OK) {
                                error = INV_DT;
                            }
                        }
                        if(error == OK) {
                            parseDateTime(newEvent->startDateTime, body);
                            startFlag = 1;
                        }
                    } else {
                        Property * newProperty = parseProperty(name, body);
                        if(strncmp(newProperty->propDescr, "VALARM", 6) == 0) {
                            deleteProperty(newProperty);
                        } else {
                            insertBack(newEvent->properties, newProperty);
                        }
                    }

                    free(fullLine);
                }

                if((strlen(newEvent->UID) == 0 || creationFlag == 0 || startFlag == 0) && error == OK) {
                    error = INV_EVENT;
                }
                creationFlag = 0;
                startFlag = 0;
                insertBack(newCalendar->events, newEvent);
            }//end of event branch

        //Make sure the last line is END:VCALENDAR
        } else if(strncmp(fullLine, "END:VCALENDAR", 13) == 0) {
            if(currentLine != numLines - 1) {
                error = INV_CAL;
            }
            //Parse as a Property of the calendar
        } else {
            Property * newProperty = parseProperty(name, body);
            insertBack(newCalendar->properties, newProperty);
        }

        free(fullLine);
        currentLine++;
    }

    if((duplicateVersion != 1 || duplicateID != 1 || getLength(newCalendar->events) == 0) && error == OK) {
        error = INV_CAL;
    }

    if(error != OK) {
        deleteCalendar(newCalendar);
        *obj = NULL;
    } else {
        *obj = newCalendar;
    }

    fclose(fp);
    return error;
}

//deletes the events list, the properties list, then frees the calendar
void deleteCalendar(Calendar* obj) {
    if(obj == NULL) {
        return;
    } else {
        freeList(obj->properties);//not null
        ListIterator eventItr = createIterator(obj->events);
        Event * e = (Event*)nextElement(&eventItr);
        while(e != NULL) {
            freeList(e->alarms);
            e = nextElement(&eventItr);
        }
        freeList(obj->events);//not null
        free(obj);
    }
}

char* printCalendar(const Calendar* obj) {
    if(obj == NULL) {
        return NULL;
    }
    int maxLength = 1000;
    char * str = (char*)malloc(sizeof(char) * maxLength);
    strcpy(str, "Calendar Data:\nID:");
    strcat(str, obj->prodID);
    strcat(str, "Version: ");
    char version[3];
    sprintf(version, "%f", obj->version);
    strcat(str, version);
    if(getLength(obj->properties) != 0) {
        strcat(str, "\n- Properties -\n");
        ListIterator propertyItr = createIterator(obj->properties);
        Property * p = (Property*)nextElement(&propertyItr);
        while(p != NULL) {
            char * propertyString = printProperty(p);
            if(strlen(str) + strlen(propertyString) > maxLength) {
                maxLength += 1000;
                str = (char*)realloc(str, sizeof(char) * maxLength);
            }
            strcat(str, propertyString);
            if(str[strlen(str) - 1] != '\n') {
                strcat(str, "\n");
            }
            p = nextElement(&propertyItr);
            free(propertyString);
        }
    }
    if(getLength(obj->events) != 0) {
        ListIterator eventItr = createIterator(obj->events);
        Event * e = (Event*)nextElement(&eventItr);
        while(e != NULL) {
            strcat(str, "\n- Event -\n");
            char * eventString = printEvent(e);
            if(strlen(str) + strlen(eventString) > maxLength) {
                maxLength += 1000;
                str = (char*)realloc(str, sizeof(char) * maxLength);
            }
            strcat(str, eventString);
            if(str[strlen(str) - 1] != '\n') {
                strcat(str, "\n");
            }
            ListIterator alarmItr = createIterator(e->alarms);
            Alarm * a = (Alarm*)nextElement(&alarmItr);
            while(a != NULL) {
                strcat(str, "\n- Alarm -\n");
                char * alarmString = printAlarm(a);
                if(strlen(str) + strlen(alarmString) > maxLength) {
                    maxLength += 1000;
                    str = (char*)realloc(str, sizeof(char) * maxLength);
                }
                strcat(str, alarmString);
                if(str[strlen(str) - 1] != '\n') {
                    strcat(str, "\n");
                }
                a = nextElement(&alarmItr);
                free(alarmString);
            }
            e = nextElement(&eventItr);
            free(eventString);
        }
    }

    return str;
}

char* printError(ICalErrorCode err) {
    char * message = malloc(sizeof(char) * 45);
    strcpy(message, "iCalendar error: ");
    switch(err) {
        case OK:
            strcat(message, "all good!\n");
            break;
        case INV_FILE:
            strcat(message, "invalid file\n");
            break;
        case INV_CAL:
            strcat(message, "invalid calendar\n");
            break;
        case INV_VER:
            strcat(message, "invalid version\n");
            break;
        case DUP_VER:
            strcat(message, "duplicated version\n");
            break;
        case INV_PRODID:
            strcat(message, "invalid product id\n");
            break;
        case DUP_PRODID:
            strcat(message, "duplicated product id\n");
            break;
        case INV_EVENT:
            strcat(message, "invalid event\n");
            break;
        case INV_DT:
            strcat(message, "invalid date/time\n");
            break;
        case INV_ALARM:
            strcat(message, "invalid alarm\n");
            break;
        case WRITE_ERROR:
            strcat(message, "write error\n");
            break;
        case OTHER_ERROR:
            strcpy(message, "no iCalendar errors.\n");
            break;
        default:
            //really shouldn't happen but reason not to
            strcat(message, "unknown\n");
            break;
    }

    return message;
}

ICalErrorCode writeCalendar(char* fileName, const Calendar* obj) {
    if(fileName == NULL || strlen(fileName) == 0) {
        return INV_FILE;
    }
    return OK;
}

ICalErrorCode validateCalendar(const Calendar* obj) {
    if(obj == NULL) {
        return INV_CAL;
    }
    return OK;
}

/* helper functions!
 * every compare() function returns zero
 * since they're not actually used right now
 * TODO: unstub the print functions
 */

//EVENTS
void deleteEvent(void* toBeDeleted) {
    if(toBeDeleted == NULL) {
        return;
    }
    Event * temp = (Event*)toBeDeleted;
    freeList(temp->properties);
    free(temp);
}

int compareEvents(const void* first, const void* second) {
    if(first == NULL || second == NULL) {
        return 0;
    }
    Event * tempFirst = (Event*)first;
    Event * tempSecond = (Event*)second;
    int uidComparison = strcmp(tempFirst->UID, tempSecond->UID);
    int creationComparison = compareDates(&tempFirst->creationDateTime, &tempSecond->creationDateTime);
    int startComparison = compareDates(&tempFirst->startDateTime, &tempSecond->startDateTime);
    int numAlarms = getLength(tempFirst->alarms) - getLength(tempSecond->alarms);
    int numProperties = getLength(tempFirst->properties) - getLength(tempSecond->properties);
    if(uidComparison != 0) {
        return uidComparison;
    } else if(creationComparison != 0) {
        return creationComparison;
    } else if(startComparison != 0) {
        return startComparison;
    } else if(numAlarms != 0) {
        return numAlarms;
    } else if(numProperties != 0) {
        return numProperties;
    }

    //TODO: full comparison of alarms and properties
    return 0;
}

char* printEvent(void* toBePrinted) {
    if(toBePrinted == NULL) {
        return NULL;
    }
    Event * toPrint = (Event*)toBePrinted;
    char * str = malloc(sizeof(char) * 1000);
    char numProperties[30];
    strcpy(str, "UID:");
    strcat(str, toPrint->UID);
    sprintf(numProperties, "# of properties: %d\n", getLength(toPrint->properties));
    strcat(str, numProperties);
    return str;
}

//ALARMS
void deleteAlarm(void* toBeDeleted) {
    if(toBeDeleted == NULL) {
        return;
    }
    Alarm * temp = (Alarm*)toBeDeleted;
    free(temp->trigger);
    freeList(temp->properties);//list empty but not null
    free(temp);
}

int compareAlarms(const void* first, const void* second) {
    if(first == NULL || second == NULL) {
        return 0;
    }
    Alarm * tempFirst = (Alarm*)first;
    Alarm * tempSecond = (Alarm*)second;
    int actionComparison = strcmp(tempFirst->action, tempSecond->action);
    int triggerComparison = strcmp(tempFirst->trigger, tempSecond->trigger);
    int numProperties = getLength(tempFirst->properties) - getLength(tempSecond->properties);
    int propertyComparison = 0;
    if(actionComparison != 0) {
        return actionComparison;
    } else if(triggerComparison != 0) {
        return triggerComparison;
    } else if (numProperties != 0) {
        return numProperties;
    } else {
        ListIterator firstProperties = createIterator(tempFirst->properties);
        ListIterator secondProperties = createIterator(tempSecond->properties);
        Property * firstProperty = nextElement(&firstProperties);
        Property * secondProperty = nextElement(&secondProperties);
        while(firstProperty != NULL && secondProperty != NULL) {
            propertyComparison = compareProperties(firstProperty, secondProperty);
            if(propertyComparison != 0) {
                return propertyComparison;
            }
            firstProperty = nextElement(&firstProperties);
            secondProperty = nextElement(&secondProperties);
        }
    }

    return 0;
}

char* printAlarm(void* toBePrinted) {
    if(toBePrinted == NULL) {
        return NULL;
    }
    Alarm * toPrint = (Alarm*)toBePrinted;
    int len = strlen(toPrint->action) + strlen(toPrint->trigger) + 60;
    const char colon = ':';
    char * str = malloc(sizeof(char) * len);
    char properties[25];
    sprintf(properties, "# of properties: %d\n", getLength(toPrint->properties));
    strcpy(str, "ACTION:");
    strcat(str, toPrint->action);
    strcat(str, "TRIGGER");
    if(strchr(toPrint->trigger, colon) == NULL) {
        strcat(str, ":");
    } else {
        strcat(str, ";");
    }
    strcat(str, toPrint->trigger);
    strcat(str, properties);
    return str;
}

//PROPERTIES
void deleteProperty(void* toBeDeleted) {
    if(toBeDeleted == NULL) {
        return;
    }
    free(toBeDeleted);
}
int compareProperties(const void* first, const void* second) {
    if(first == NULL || second == NULL) {
        return 0;
    }
    Property * tempFirst = (Property*)first;
    Property * tempSecond = (Property*)second;

    if(strcmp(tempFirst->propName, tempSecond->propName) != 0) {
        return strcmp(tempFirst->propName, tempSecond->propName);
    } else {
        return strcmp(tempFirst->propDescr, tempSecond->propDescr);
    }
}

char* printProperty(void* toBePrinted) {
    if(toBePrinted == NULL) {
        return NULL;
    }
    Property * toPrint = (Property*)toBePrinted;
    int len = strlen(toPrint->propName) + strlen(toPrint->propDescr) + 15;
    const char colon = ':';
    char * str = malloc(sizeof(char) * len);
    str[0] = '\0';
    strcat(str, toPrint->propName);
    if(strchr(toPrint->propDescr, colon) == NULL) {
        strcat(str, ":");
    } else {
        strcat(str, ";");
    }
    strcat(str, toPrint->propDescr);
    return str;
}

//DATES
void deleteDate(void* toBeDeleted) {
    if(toBeDeleted == NULL) {
        return;
    }
    free(toBeDeleted);
}

int compareDates(const void* first, const void* second) {
    if(first == NULL || second == NULL) {
        return 0;
    }
    DateTime *tempFirst = (DateTime*)first;
    DateTime *tempSecond = (DateTime*)second;
    if(tempFirst->date - tempSecond->date != 0) {
        return tempFirst->date - tempSecond->date;
    } else if(tempFirst->UTC == tempSecond->UTC) {
        return tempFirst->time - tempSecond->time;
    } else {
        return 1;//If one isn't UTC comparison is nontrivial
    }
    return 0;
}

char* printDate(void* toBePrinted) {
    if(toBePrinted == NULL) {
        return NULL;
    }
    char * str = malloc(sizeof(char) * 20);
    DateTime *toPrint = (DateTime*)toBePrinted;
    strcpy(str, toPrint->date);
    strcat(str, toPrint->time);
    if(toPrint->UTC == true) {
        strcat(str, "Z");
    }
    strcat(str, "\r\n");
    return str;
}

// Additional functions used internally by libcal.so
// Included via CalendarParserUtilities.h

//Removes the trailing characters from strings
void chop(char * str) {
    if(str == NULL) {
        return;
    }
    int len = strlen(str);

    if(str[len-1] == '\n') {
        str[len-1] = '\0';
    }

    if(str[len-2] == '\r') {
        str[len-2] = '\0';
    }
}

//Gets line from ical file, tacks on subsequent lines that have been folded
char * unfoldLine(char buffer[][90], int * currLinePtr, int size) {
    char * line = (char*)malloc(sizeof(char) * size);
    line[0] = '\0';
    strcpy(line, buffer[*currLinePtr]);

    //Automatically skip comments in the middle of folded lines
    while(strncmp(buffer[*currLinePtr + 1], ";", 1) == 0) {
        *currLinePtr = *currLinePtr + 1;
        while(strncmp(buffer[*currLinePtr + 1], " ", 1) == 0 || strncmp(buffer[*currLinePtr + 1], "\t", 1) == 0) {
            *currLinePtr = *currLinePtr + 1;
            chop(line);
            strcat(line, buffer[*currLinePtr] + 1);
        }
    }
    while(strncmp(buffer[*currLinePtr + 1], " ", 1) == 0 || strncmp(buffer[*currLinePtr + 1], "\t", 1) == 0) {
        *currLinePtr = *currLinePtr + 1;
        chop(line);
        strcat(line, buffer[*currLinePtr] + 1);
    }

    return line;
}

//Creates and returns a DateTime object
void parseDateTime(DateTime dt, char * body) {
    strncpy(dt.date, body, 8);
    strncpy(dt.time, body + 9, 6);
    if(body[16] == 'Z') {
        dt.UTC = true;
    } else {
        dt.UTC = false;
    }
}

//Creates and returns a Property object
Property * parseProperty(char * name, char * body) {
    int len = strlen(body) + 1;
    Property * newProperty = (Property*)malloc(sizeof(Property) + len);

    strncpy(newProperty->propName, name, 199);
    strncpy(newProperty->propDescr, body, len);

    return newProperty;
}
