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
#include <unistd.h>

#include "CalendarParser.h"
#include "CalendarParserUtilities.h"
#include "LinkedListAPI.h"

void createUserCalendar(char * fileName, char * calendarJSON, char * eventJSON, char * buffer) {
    chdir("uploads/");
    Calendar * cal = JSONtoCalendar(calendarJSON);
    Event * newEvent = JSONtoEvent(eventJSON);
    addEvent(cal, newEvent);
    ICalErrorCode error;
    error = writeCalendar(fileName, cal);
    deleteCalendar(cal);
    if(error == OK) {
        strcpy(buffer, "Calendar created successfully");
    } else {
        strcpy(buffer, "Error writing calendar");
    }
}

void getCalendarJSON(char * fileName, char * buffer) {
    chdir("uploads/");
    ICalErrorCode error;
    Calendar * cal;
    error = createCalendar(fileName, &cal);
    if(error != OK) {
        //error
        char * message = printError(error);
        strcpy(buffer, "{\"error\":\"");
        strcat(buffer, message);
        strcat(buffer, "\"}");
        free(message);
    } else {
        //valid
        char * output = calendarToJSON(cal);
        deleteCalendar(cal);
        strcpy(buffer, output);
    }
}

void getEventListJSON(char * fileName, char * buffer) {
    chdir("uploads/");
    ICalErrorCode error;
    Calendar * cal;
    char hack[500];
    strcpy(hack, fileName);
    error = createCalendar(hack, &cal);
    if(error != OK) {
        //error
        char * message = printError(error);
        strcpy(buffer, "{\"error\":\"");
        strcat(buffer, message);
        strcat(buffer, "\"}");
        free(message);
    } else {
        //valid
        char * output = eventListToJSON(cal->events);
        deleteCalendar(cal);
        strcpy(buffer, output);
    }
}

void getAlarmListJSON(char * fileName, char * eventID, char * buffer) {
    chdir("uploads/");
    ICalErrorCode error;
    Calendar * cal;
    error = createCalendar(fileName, &cal);

    char copyID[20];
    strncpy(copyID, eventID, 15);
    char * end = copyID + 6;
    int eventNum = atoi(end);

    if(error != OK) {
        //error
        char * message = printError(error);
        strcpy(buffer, "{\"error\":\"");
        strcat(buffer, message);
        strcat(buffer, "\"}");
        free(message);
    } else {
        //valid
        ListIterator eventItr = createIterator(cal->events);
        Event * e = nextElement(&eventItr);
        for(int i = 0; i <= eventNum; i++) {
            if(i == eventNum) {
                break;
            } else {
                e = nextElement(&eventItr);
            }
        }
        char * output = alarmListToJSON(e->alarms);
        deleteCalendar(cal);
        strcpy(buffer, output);
    }
}

void getOptionalPropertiesJSON(char * fileName, char * eventID, char * buffer) {
    chdir("uploads/");
    ICalErrorCode error;
    Calendar * cal;
    error = createCalendar(fileName, &cal);

    char copyID[20];
    strncpy(copyID, eventID, 19);
    char * end = copyID + 10;
    int eventNum = atoi(end);

    if(error != OK) {
        //error
        char * message = printError(error);
        strcpy(buffer, "{\"error\":\"");
        strcat(buffer, message);
        strcat(buffer, "\"}");
        free(message);
    } else {
        //valid
        ListIterator eventItr = createIterator(cal->events);
        Event * e = nextElement(&eventItr);
        for(int i = 0; i <= eventNum; i++) {
            if(i == eventNum) {
                break;
            } else {
                e = nextElement(&eventItr);
            }
        }
        char * output = propertyListToJSON(e->properties);
        deleteCalendar(cal);
        strcpy(buffer, output);
    }
}

void addEventToCalendar(char * fileName, char * eventString, char * buffer) {
    chdir("uploads/");
    ICalErrorCode error;
    Calendar * cal;
    error = createCalendar(fileName, &cal);
    if(error != OK) {
        //error
        char * message = printError(error);
        strcpy(buffer, "{\"error\":\"");
        strcat(buffer, message);
        strcat(buffer, "\"}");
        free(message);
    } else {
        //valid
        Event * newEvent = JSONtoEvent(eventString);
        addEvent(cal, newEvent);
        error = writeCalendar(fileName, cal);
        if(error != OK) {
            strcpy(buffer, "Failed to add event");
        } else {
            strcpy(buffer, "Success");
        }
        deleteCalendar(cal);
    }
}

ICalErrorCode createCalendar(char* fileName, Calendar** obj) {
    //chdir("../../uploads/");
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
        printf("no such file\n");
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
    fclose(fp);

    while(currentLine < numLines) {
        char * fullLine = unfoldLine(buffer, currLinePtr, size);
        strncpy(name, fullLine, 1000);

        if((currentLine == 0) && strcmp(fullLine, "BEGIN:VCALENDAR\r\n") != 0) {
            error = INV_CAL;
            break;
        } else if((currentLine == numLines - 1) && strcmp(fullLine, "END:VCALENDAR\r\n") != 0) {
            error = INV_CAL;
            break;
        }

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
            if((body == NULL || strlen(body) <= 2) && error == OK) {
                error = INV_PRODID;
            } else if(duplicateID > 0 && error == OK) {
                error = DUP_PRODID;
            } else {
                chop(body);
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
                            } else if(strncmp(name, "BEGIN:", 6) == 0) {
                                free(fullLine);
                                return INV_ALARM;
                            } else if(strncmp(fullLine, "END:", 4) == 0) {
                                free(fullLine);
                                return INV_ALARM;
                            } else if(strncmp(name, "TRIGGER", 7) == 0) {
                                char * trigger = (char*)malloc(sizeof(char) * (strlen(body) + 1));
                                strcpy(trigger, body);
                                chop(trigger);
                                newAlarm->trigger = trigger;
                            } else if(strncmp(name, "ACTION", 6) == 0) {
                                chop(body);
                                strncpy(newAlarm->action, body, 199);
                            } else {
                                Property * newProperty = parseProperty(name, body);
                                if(newProperty->propName == NULL && error == OK) {
                                    error = INV_ALARM;
                                } else if((strlen(newProperty->propName) == 0 || strlen(newProperty->propDescr) == 0) && error == OK) {
                                    error = INV_ALARM;
                                }
                                insertBack(newAlarm->properties, newProperty);
                            }

                            free(fullLine);
                        }

                        if((strlen(newAlarm->action) == 0 || newAlarm->trigger == NULL) && error == OK) {
                            error = INV_ALARM;
                        } else if(strlen(newAlarm->trigger) == 0 && error == OK) {
                            error = INV_ALARM;
                        }
                        insertBack(newEvent->alarms, newAlarm);
                    }//end of alarm branch
                    //Other Event components
                    if(strncmp(fullLine, "END:VEVENT", 10) == 0) {
                        free(fullLine);
                        break;//end of event entry
                    } else if(strncmp(fullLine, "BEGIN:VEVENT", 12) == 0) {
                        free(fullLine);
                        return INV_EVENT;
                    } else if(strncmp(fullLine, "END:", 4) == 0 && error == OK) {
                        error = INV_EVENT;
                        free(fullLine);
                        break;
                    } else if(strncmp(name, "UID", 3) == 0) {
                        strncpy(newEvent->UID, body, 999);
                        chop(newEvent->UID);
                    } else if(strncmp(name, "DTSTAMP", 7) == 0) {
                        chop(body);
                        for(int i = 0; i < strlen(body); i++) {
                            if(((i == 8 && body[i] != 'T') || (i == 15 && body[i] != 'Z' && isalnum(body[i]))) && error == OK) {
                                error = INV_DT;
                            }
                            if((i != 8 && i < 15) && !isdigit(body[i]) && error == OK) {
                                error = INV_DT;
                            }
                        }
                        if(error == OK) {
                            newEvent->creationDateTime = parseDateTime(body);
                            creationFlag = 1;
                        }
                    } else if(strncmp(name, "DTSTART", 7) == 0) {
                        chop(body);
                        for(int i = 0; i < strlen(body); i++) {
                            if(((i == 8 && body[i] != 'T') || (i == 15 && body[i] != 'Z' && isalnum(body[i]))) && error == OK) {
                                error = INV_DT;
                            }
                            if((i != 8 && i < 15) && !isdigit(body[i]) && error == OK) {
                                error = INV_DT;
                            }
                        }
                        if(error == OK) {
                            newEvent->startDateTime = parseDateTime(body);
                            startFlag = 1;
                        }
                    } else {
                        Property * newProperty = parseProperty(name, body);
                        if(newProperty->propName == NULL && error == OK) {
                            error = INV_EVENT;
                        } else if((strlen(newProperty->propName) == 0 || strlen(newProperty->propDescr) == 0) && error == OK) {
                            error = INV_EVENT;
                        }
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
    return error;
}

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
    strcat(str, "\nVersion: ");
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
            strcat(message, "all good!");
            break;
        case INV_FILE:
            strcat(message, "invalid file");
            break;
        case INV_CAL:
            strcat(message, "invalid calendar");
            break;
        case INV_VER:
            strcat(message, "invalid version");
            break;
        case DUP_VER:
            strcat(message, "duplicated version");
            break;
        case INV_PRODID:
            strcat(message, "invalid product id");
            break;
        case DUP_PRODID:
            strcat(message, "duplicated product id");
            break;
        case INV_EVENT:
            strcat(message, "invalid event");
            break;
        case INV_DT:
            strcat(message, "invalid date/time");
            break;
        case INV_ALARM:
            strcat(message, "invalid alarm");
            break;
        case WRITE_ERROR:
            strcat(message, "write error");
            break;
        case OTHER_ERROR:
            strcpy(message, "other error");
            break;
        default:
            //really shouldn't happen but reason not to
            strcat(message, "unknown");
            break;
    }

    return message;
}

ICalErrorCode writeCalendar(char* fileName, const Calendar* obj) {
    if(fileName == NULL || strlen(fileName) == 0) {
        return WRITE_ERROR;
    }
    int fileNameLength = strlen(fileName);
    char * extension = &fileName[fileNameLength - 4];
    const char colon = ':';
    if(strcmp(extension, ".ics") != 0 || fileNameLength <= 4) {
        return WRITE_ERROR;
    }
    //Calendar * calendar = obj;
    FILE * fp = fopen(fileName, "w");
    char version[5];
    fwrite("BEGIN:VCALENDAR\r\nVERSION:", 25, sizeof(char), fp);
    sprintf(version, "%0.1lf", obj->version);
    fwrite(version, strlen(version), sizeof(char), fp);
    fwrite("\r\nPRODID:", 9, sizeof(char), fp);
    fwrite(obj->prodID, strlen(obj->prodID), sizeof(char), fp);
    fwrite("\r\n", 2, sizeof(char), fp);
    if(getLength(obj->properties) > 0) {
        ListIterator propItr = createIterator(obj->properties);
        writeProperty(propItr, fp);
    }
    if(getLength(obj->events) > 0) {
        ListIterator eventItr = createIterator(obj->events);
        Event * e = (Event*)nextElement(&eventItr);
        while(e != NULL) {//events
            DateTime * created = &e->creationDateTime;
            DateTime * start = &e->startDateTime;
            fwrite("BEGIN:VEVENT\r\n", 14, sizeof(char), fp);
            //evt UID, creation, start time
            if(strchr(e->UID, colon) == NULL) {
                fwrite("UID:", 4, sizeof(char), fp);
            } else {
                fwrite("UID;", 4, sizeof(char), fp);
            }
            fwrite(e->UID, strlen(e->UID), sizeof(char), fp);
            fwrite("\r\nDTSTAMP:", 10, sizeof(char), fp);
            fwrite(created->date, 8, sizeof(char), fp);
            fwrite("T", 1, sizeof(char), fp);
            fwrite(created->time, 6, sizeof(char), fp);
            if(created->UTC == true) {
                fwrite("Z\r\n", 3, sizeof(char), fp);
            } else {
                fwrite("\r\n", 2, sizeof(char), fp);
            }
            fwrite("DTSTART:", 8, sizeof(char), fp);
            fwrite(start->date, 8, sizeof(char), fp);
            fwrite("T", 1, sizeof(char), fp);
            fwrite(start->time, 6, sizeof(char), fp);
            if(start->UTC == true) {
                fwrite("Z\r\n", 3, sizeof(char), fp);
            } else {
                fwrite("\r\n", 2, sizeof(char), fp);
            }
            if(getLength(e->properties) > 0) {//properties
                ListIterator propItr = createIterator(e->properties);
                writeProperty(propItr, fp);
            }
            if(getLength(e->alarms) > 0) {//alarms
                //action, trigger
                ListIterator alarmItr = createIterator(e->alarms);
                Alarm * a = (Alarm*)nextElement(&alarmItr);
                while(a != NULL) {
                    fwrite("BEGIN:VALARM\r\nACTION", 20, sizeof(char), fp);
                    if(strchr(a->action, colon) == NULL) {
                        fwrite(":", 1, sizeof(char), fp);
                    } else {
                        fwrite(";", 1, sizeof(char), fp);
                    }
                    fwrite(a->action, strlen(a->action), sizeof(char), fp);
                    fwrite("\r\nTRIGGER", 9, sizeof(char), fp);
                    if(strchr(a->trigger, colon) == NULL) {
                        fwrite(":", 1, sizeof(char), fp);
                    } else {
                        fwrite(";", 1, sizeof(char), fp);
                    }
                    fwrite(a->trigger, strlen(a->trigger), sizeof(char), fp);
                    fwrite("\r\n", 2, sizeof(char), fp);
                    if(getLength(a->properties) > 0) {//properties
                        ListIterator propItr = createIterator(a->properties);
                        writeProperty(propItr, fp);
                    }
                    fwrite("END:VALARM\r\n", 12, sizeof(char), fp);
                    a = nextElement(&alarmItr);
                }
            }
            fwrite("END:VEVENT\r\n", 12, sizeof(char), fp);
            e = nextElement(&eventItr);
        }
    }
    fwrite("END:VCALENDAR\r\n", 15, sizeof(char), fp);
    fclose(fp);
    return OK;
}

ICalErrorCode validateCalendar(const Calendar* obj) {
    //ICalErrorCode error = OK;
    const char eventProperties[37][20] = {
    "GEO","CLASS","DESCRIPTION","LAST-MODIFIED","LOCATION","ORGANIZER","PRIORITY","SEQUENCE","STATUS",
    "SUMMARY","TRANSP","URL","RECURRENCE-ID","CREATED",
    "ATTACH","CATEGORIES","COMMENT","PERCENT-COMPLETE","RESOURCES","COMPLETED",
    "DTEND","DUE","DTSTART","DURATION","FREEBUSY","TZID","TZNAME",
    "TZOFFSETFROM","TZOFFSETTO","TZURL","ATTENDEE","CONTACT",
    "RELATED-TO","EXDATE","RDATE","RRULE","TRIGGER"};
    int eventDuplications[13] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
    const char alarmProperties[8][20] = {
    "REPEAT","DURATION","ATTACH","CREATED","DTSTAMP","LAST-MODIFIED","SEQUENCE","REPEAT"};
    int alarmDuplications[8] = {0,0,0,0,0,0,0,0};
    int calendarDuplications[2] = {0,0};
    char propertyName[30];
    if(obj == NULL) {
        return INV_CAL;
    }
    if(obj->events == NULL || obj->properties == NULL) {
        return INV_CAL;
    } else if(getLength(obj->events) == 0) {
        return INV_CAL;
    } else if(strlen(obj->prodID) < 1) {
        return INV_CAL;
    }
    //must also return INV_CAL if it has a property it shouldn't, one that viol-
    //ates the # of occurrences rule, properties with empty values, etc.
    ListIterator calPropItr = createIterator(obj->properties);
    Property * cp = nextElement(&calPropItr);
    while(cp != NULL) {
        strncpy(propertyName, cp->propName, 9);
        if(strcmp(propertyName, "CALSCALE") || strcmp(propertyName, "METHOD")) {
            return INV_CAL;
        }
        if(!strcmp(propertyName, "CALSCALE")) {
            calendarDuplications[0]++;
        } else if(!strcmp(propertyName, "METHOD")) {
            calendarDuplications[1]++;
        }
        if(strlen(cp->propDescr) == 0) {
            return INV_CAL;
        }
        cp = nextElement(&calPropItr);
    }
    if(calendarDuplications[0] > 1 || calendarDuplications[1] > 1) {
        return INV_CAL;
    }
    //Events
    ListIterator eventItr = createIterator(obj->events);
    Event * e = nextElement(&eventItr);
    while(e != NULL) {
        if(e->properties == NULL || e->alarms == NULL) {
            return INV_EVENT;
        } else if(strlen(e->UID) == 0) {
            return INV_EVENT;
        }
        //Event properties
        ListIterator eventPropItr = createIterator(e->properties);
        Property * ep = nextElement(&eventPropItr);
        while(ep != NULL) {
            for(int i = 0; i < 13; i++) {
                eventDuplications[i] = 0;
            }
            strcpy(propertyName, ep->propName);
            bool epMatch = false;
            for(int i = 0; i < 37; i++) {
                if(strcmp(propertyName, eventProperties[i]) == 0) {
                    epMatch = true;
                    if(i < 13) {
                        eventDuplications[i]++;
                    }
                }
                if(strlen(ep->propDescr) == 0) {
                    return INV_EVENT;
                }
            }
            if(!epMatch) {
                return INV_EVENT;
            }
            ep = nextElement(&eventPropItr);
        }
        for(int i = 0; i < 13; i++) {
            if(eventDuplications[i] > 1) {
                return INV_EVENT;
            }
        }
        //DateTime components of the Event
        if(strlen(e->creationDateTime.date) != 8 || strlen(e->startDateTime.date) != 8) {
            return INV_DT;
        } else if(strlen(e->creationDateTime.time) != 6 || strlen(e->startDateTime.time) != 6) {
            return INV_DT;
        }
        //Alarms
        ListIterator alarmItr = createIterator(e->alarms);
        Alarm * a = nextElement(&alarmItr);
        while(a != NULL) {
            if(a->properties == NULL || a->trigger == NULL) {
                return INV_ALARM;
            } else if(strlen(a->action) == 0 || strlen(a->trigger) == 0) {
                return INV_ALARM;
            }
            //Alarm properties
            ListIterator alarmPropItr = createIterator(a->properties);
            Property * ap = nextElement(&alarmPropItr);
            while(ap != NULL) {
                for(int i = 0; i < 8; i++) {
                    alarmDuplications[i] = 0;
                }
                strcpy(propertyName, ap->propName);
                bool apMatch = false;
                for(int i = 0; i < 8; i++) {
                    if(strcmp(propertyName, alarmProperties[i]) == 0) {
                        apMatch = true;
                    }
                    alarmDuplications[i]++;
                    if(strlen(ap->propDescr) == 0) {
                        return INV_ALARM;
                    }
                }
                if(!apMatch) {
                    return INV_ALARM;
                }
                ap = nextElement(&alarmPropItr);
            }
            if(alarmDuplications[0] != alarmDuplications[1]) {
                return INV_ALARM;
            } else if (alarmDuplications[0] > 1 || alarmDuplications[2] > 1) {
                return INV_ALARM;
            }

            if(!strcmp(a->action, "AUDIO")) {
                if(alarmDuplications[2] != 1) {
                    return INV_ALARM;
                }

            }
            a = nextElement(&alarmItr);
        }
        e = nextElement(&eventItr);
    }
    return OK;
}

/* JSON functions!
 * some are optional and defined in CalendarParserUtilities.h
 */

char* dtToJSON(DateTime prop) {
    char * str = malloc(sizeof(char) * 49);
    strcpy(str, "{\"date\":\"");
    strcat(str, prop.date);
    strcat(str, "\",\"time\":\"");
    strcat(str, prop.time);
    if(prop.UTC == true) {
        strcat(str, "\",\"isUTC\":true}");
    } else {
        strcat(str, "\",\"isUTC\":false}");
    }
    return str;
}
char* eventToJSON(const Event* event) {
    if(event == NULL) {
        char * empty = malloc(sizeof(char) * 3);
        strcpy(empty, "{}");
        return empty;
    }
    int size = 110;
    char * start = dtToJSON(event->startDateTime);
    char * summary;
    char numProps[5];
    char numAlarms[5];
    bool isSummary = false;
    sprintf(numProps, "%d", getLength(event->properties) + 3);
    sprintf(numAlarms, "%d", getLength(event->alarms));
    if(getLength(event->properties) > 0) {
        ListIterator propItr = createIterator(event->properties);
        Property * p = (Property*)nextElement(&propItr);
        while(p != NULL) {
            if(strncmp(p->propName, "SUMMARY", 7) == 0) {
                isSummary = true;
                summary = malloc(sizeof(char) * (strlen(p->propDescr) + 5));
                strcpy(summary, p->propDescr);
                summary[strlen(p->propDescr)] = '\0';
                size += strlen(p->propDescr);
                break;
            } else {
                p = nextElement(&propItr);
            }
        }
    }
    char * str = malloc(sizeof(char) * size);
    strcpy(str, "{\"startDT\":");
    strcat(str, start);
    strcat(str, ",\"numProps\":");
    strcat(str, numProps);
    strcat(str, ",\"numAlarms\":");
    strcat(str, numAlarms);
    strcat(str, ",\"summary\":\"");
    if(isSummary) {
        strcat(str, summary);
        free(summary);
    }
    strcat(str, "\"}");
    free(start);
    return str;
}
char* eventListToJSON(const List* eventList) {
    int size = 3;
    char * str = (char*)malloc(sizeof(char) * size);
    strcpy(str, "[");
    if(eventList == NULL || eventList->length == 0) {
        strcat(str, "]");
        return str;
    }
    ListIterator eventItr = createIterator((List*)eventList);
    Event * e = (Event*)nextElement(&eventItr);
    while(e != NULL) {
        char * eventJSON = eventToJSON(e);
        size += (strlen(eventJSON) + 1);
        str = (char*)realloc(str, sizeof(char) * size);
        strcat(str, eventJSON);
        free(eventJSON);//TODO:if it's crashing weirdly, check here first
        e = nextElement(&eventItr);
        if(e != NULL) {
            strcat(str, ",");
        }
    }
    strcat(str, "]");
    return str;
}
char* calendarToJSON(const Calendar* cal) {
    if(cal == NULL) {
        char * empty = malloc(sizeof(char) * 3);
        strcpy(empty, "{}");
        return empty;
    }
    //char version[5];
    char props[5];
    char events[5];
    char id[1001];
    strncpy(id, cal->prodID, strlen(cal->prodID));
    id[strlen(cal->prodID)] = '\0';
    int numProps = 2 + getLength(cal->properties);
    sprintf(props, "%d", numProps);
    int numEvents = getLength(cal->events);
    sprintf(events, "%d", numEvents);
    int size = 1065;
    char * str = malloc(sizeof(char) * size);
    strcpy(str, "{\"version\":");
    strcat(str, "2");
    strcat(str, ",\"prodID\":\"");
    strcat(str, id);
    strcat(str, "\",\"numProps\":");
    strcat(str, props);
    strcat(str, ",\"numEvents\":");
    strcat(str, events);
    strcat(str, "}");
    return str;
}
Calendar* JSONtoCalendar(const char* str) {
    if(str == NULL) {
        return NULL;
    }
    const char colon = ':';
    Calendar * newCalendar = (Calendar*)malloc(sizeof(Calendar));
    newCalendar->properties = initializeList(&printProperty, &deleteProperty, &compareProperties);
    newCalendar->events = initializeList(&printEvent, &deleteEvent, &compareEvents);
    char * versionStr = strchr(str, colon) + 1;
    char * idStr = strchr(versionStr, colon) + 2;
    if(versionStr[0] == '2') {
        newCalendar->version = 2.0;
    } else if(versionStr[0] == '3') {
        newCalendar->version = 3.0;
    } else if(versionStr[0] == '4') {
        newCalendar->version = 4.0;
    } else if(versionStr[0] == '1'){
        newCalendar->version = 1.0;
    } else {
        newCalendar->version = 2.0;
    }
    strcpy(newCalendar->prodID, idStr);
    newCalendar->prodID[strlen(newCalendar->prodID) - 2] = '\0';
    return newCalendar;
}
Event* JSONtoEvent(const char* str) {
    if(str == NULL) {
        return NULL;
    }
    Event * newEvent = (Event*)malloc(sizeof(Event));
    newEvent->properties = initializeList(&printProperty, &deleteProperty, &compareProperties);
    newEvent->alarms = initializeList(&printAlarm, &deleteAlarm, &compareAlarms);
    char * JSON = malloc(sizeof(char) * (strlen(str) + 2));
    strcpy(JSON, str);
    int length = 0;
    const char colon = ':';
    char * ptr = strchr(JSON, colon);
    ptr++;
    //createDT - date
    ptr = strchr(ptr, colon);
    ptr+=2;
    strncpy(newEvent->creationDateTime.date, ptr, 8);
    newEvent->creationDateTime.date[8] = '\0';
    //Time
    ptr = strchr(ptr, colon);
    ptr += 2;
    strncpy(newEvent->creationDateTime.time, ptr, 6);
    //UTC
    ptr = strchr(ptr, colon);
    ptr+=2;
    if(ptr[0] == 'f') {
        newEvent->creationDateTime.UTC = false;
    } else {
        newEvent->creationDateTime.UTC = true;
    }
    //startDT - date
    ptr = strchr(ptr, colon);
    ptr++;
    ptr = strchr(ptr, colon);
    ptr+=2;
    strncpy(newEvent->startDateTime.date, ptr, 8);
    newEvent->startDateTime.date[8] = '\0';
    //Time
    ptr = strchr(ptr, colon);
    ptr += 2;
    strncpy(newEvent->startDateTime.time, ptr, 6);
    newEvent->startDateTime.time[6] = '\0';
    //UTC
    ptr = strchr(ptr, colon);
    ptr+=2;
    newEvent->startDateTime.UTC = false;
    if(ptr[0] == 'f') {
        newEvent->startDateTime.UTC = false;
    } else {
        newEvent->startDateTime.UTC = true;
    }
    //UID
    ptr = strchr(ptr, colon);
    ptr+=2;
    length = strchr(ptr, '"') - ptr;
    strncpy(newEvent->UID, ptr, length);
    newEvent->UID[length] = '\0';
    //Summary (if present)
    ptr = strchr(ptr, colon);
    ptr+=2;
    if(ptr[0] != '"') {
        //Summary property is present
        //Hours spent debugging this block of code: 9
        char summaryValue[2000];
        strncpy(summaryValue, ptr, strlen(ptr) - 2);
        summaryValue[strlen(ptr)-2] = '\0';
        //printf("%s\n", summaryValue);
        Property * summary = parseProperty("SUMMARY", summaryValue);
        insertBack(newEvent->properties, summary);
    }

    return newEvent;
}

void addEvent(Calendar* cal, Event* toBeAdded) {
    if(cal == NULL || toBeAdded == NULL) {
        return;
    } else {
        insertBack(cal->events, toBeAdded);
    }
}

char * alarmToJSON(const Alarm * alarm) {
    if(alarm == NULL) {
        char * empty = malloc(sizeof(char) * 3);
        strcpy(empty, "{}");
        return empty;
    }
    char numProps[5];
    int size = 50 + strlen(alarm->action) + strlen(alarm->trigger);
    sprintf(numProps, "%d", getLength(alarm->properties) + 3);
    char * str = malloc(sizeof(char) * size);
    strcpy(str, "{\"action\":\"");
    strcat(str, alarm->action);
    strcat(str, "\",\"trigger\":\"");
    strcat(str, alarm->trigger);
    strcat(str, "\",\"numProps\":");
    strcat(str, numProps);
    strcat(str, "}");
    return str;
}
char * alarmListToJSON(const List * alarmList) {
    int size = 3;
    char * str = (char*)malloc(sizeof(char) * size);
    strcpy(str, "[");
    if(alarmList == NULL || alarmList->length == 0) {
        strcat(str, "]");
        return str;
    }
    ListIterator alarmItr = createIterator((List*)alarmList);
    Alarm * a = (Alarm*)nextElement(&alarmItr);
    while(a != NULL) {
        char * alarmJSON = alarmToJSON(a);
        size += (strlen(alarmJSON) + 1);
        str = (char*)realloc(str, sizeof(char) * size);
        strcat(str, alarmJSON);
        free(alarmJSON);
        a = nextElement(&alarmItr);
        if(a != NULL) {
            strcat(str, ",");
        }
    }
    strcat(str, "]");
    return str;
}

char * propertyToJSON(const Property * property) {
    if(property == NULL) {
        char * empty = malloc(sizeof(char) * 3);
        strcpy(empty, "{}");
        return empty;
    }
    int size = 30 + strlen(property->propName) + strlen(property->propDescr);
    char * str = malloc(sizeof(char) * size);
    strcpy(str, "{\"name\":\"");
    strcat(str, property->propName);
    strcat(str, "\",\"description\":\"");
    strcat(str, property->propDescr);
    strcat(str, "\"}");
    return str;
}

char * propertyListToJSON(const List * propertyList) {
    int size = 3;
    char * str = (char*)malloc(sizeof(char) * size);
    strcpy(str, "[");
    if(propertyList == NULL || propertyList->length == 0) {
        strcat(str, "]");
        return str;
    }
    ListIterator propItr = createIterator((List*)propertyList);
    Property * p = (Property*)nextElement(&propItr);
    while(p != NULL) {
        char * propertyJSON = propertyToJSON(p);
        size += (strlen(propertyJSON) + 1);
        str = (char*)realloc(str, sizeof(char) * size);
        strcat(str, propertyJSON);
        free(propertyJSON);
        p = nextElement(&propItr);
        if(p != NULL) {
            strcat(str, ",");
        }
    }
    strcat(str, "]");
    return str;
}

/* helper functions!
 * the compare functions aren't super useful though
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
    sprintf(numProperties, "\n# of properties: %d\n", getLength(toPrint->properties));
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
    strcat(str, "\nTRIGGER");
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

//Updates a DateTime object
DateTime parseDateTime(char * body) {
    DateTime dt;
    const char zed = 'Z';
    const char tee = 'T';
    char * timeptr = strchr(body, tee);
    strncpy(dt.date, body, 8);
    dt.date[8] = '\0';
    strncpy(dt.time, timeptr + 1, 7);
    dt.time[6] = '\0';
    if(body[strlen(body) - 1] == zed) {
        dt.UTC = true;
    } else {
        dt.UTC = false;
    }
    return dt;
}

//Creates and returns a Property object
Property * parseProperty(char * name, char * body) {
    int len = strlen(body) + 1;
    Property * newProperty = (Property*)malloc(sizeof(Property) + len);

    strncpy(newProperty->propName, name, 199);
    strncpy(newProperty->propDescr, body, len);
    chop(newProperty->propDescr);

    return newProperty;
}

//Writes a property to a file
void writeProperty(ListIterator propItr, FILE * fp) {
    const char colon = ':';
    Property * p = (Property*)nextElement(&propItr);
    while(p != NULL) {
        fwrite(p->propName, strlen(p->propName), sizeof(char), fp);
        if(strchr(p->propDescr, colon) == NULL) {
            fwrite(":", 1, sizeof(char), fp);
        } else {
            fwrite(";", 1, sizeof(char), fp);
        }
        fwrite(p->propDescr, strlen(p->propDescr), sizeof(char), fp);
        fwrite("\r\n", 2, sizeof(char), fp);
        p = nextElement(&propItr);
    }
}
