/*

    SimpleCiv is simple clone of Civilization game, using ncurses library.
    Copyright (C) 2012 by K. Mingulov, A. Sapronov.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/

#include <stdlib.h>
#include <string.h>

#include "../../game/world/definitions.h"
#include "../../game/world/world_properties.h"
#include "../unit/unit_common_info.h"
#include "../technology/technology_parse_info.h"
#include "../technology/technology.h"
#include "string_functions.h"
#include "expat_handlers.h"

/*
    Auxiliary arrays for comfortable view of elementStart() function.
    xml_tags[] — array of xml tags of our xml files.
    xml_states[] — array of XMLParserData's states.
    xml_parents[i] — state, which required for state xml_states[i].
*/
const char xml_tags[][16] = {"map", "rows", "columns", "players", "count",
    "names", "cities", "unit", "id", "name", "char", "health", "damage",
    "moves", "hiring_turns", "gold_drop", "privileges", "resources",
    "technology", "id", "name", "turns", "provides", "units", "technologies",
    "requires", "resources"};

const int xml_states[] = {XML_MAP, XML_MAP_ROWS, XML_MAP_COLUMNS, XML_PLAYERS,
    XML_PLAYERS_COUNT, XML_PLAYERS_NAMES, XML_PLAYERS_CITIES, XML_UNIT,
    XML_UNIT_ID, XML_UNIT_NAME, XML_UNIT_CHAR, XML_UNIT_HEALTH, XML_UNIT_DAMAGE,
    XML_UNIT_MOVES, XML_UNIT_HIRING_TURNS, XML_UNIT_GOLD_DROP,
    XML_UNIT_PRIVILEGES, XML_UNIT_RESOURCES, XML_TECH, XML_TECH_ID,
    XML_TECH_NAME, XML_TECH_TURNS, XML_TECH_PROVIDES, XML_TECH_PROVIDES_UNITS,
    XML_TECH_PROVIDES_TECHS, XML_TECH_REQUIRES, XML_TECH_REQUIRES_RESOURCES};

const int xml_parents[] = {XML_NONE, XML_MAP, XML_MAP, XML_NONE, XML_PLAYERS,
    XML_PLAYERS, XML_PLAYERS, XML_NONE, XML_UNIT, XML_UNIT, XML_UNIT, XML_UNIT,
    XML_UNIT, XML_UNIT, XML_UNIT, XML_UNIT, XML_UNIT, XML_UNIT, XML_NONE,
    XML_TECH, XML_TECH, XML_TECH, XML_TECH, XML_TECH_PROVIDES,
    XML_TECH_PROVIDES, XML_TECH, XML_TECH_REQUIRES};

/*
    Auxiliary arrays for resources convertation from their names to their
    defined (in game/world/definitions.h) constants.
*/
const char xml_res_names[][16] = {"bronze", "iron", "coal", "gunpowder",
    "horses", "mushrooms"};

const int xml_res_values[] = {CELL_RES_BRONZE, CELL_RES_IRON, CELL_RES_COAL,
    CELL_RES_GUNPOWDER, CELL_RES_HORSES, CELL_RES_MUSHROOMS};

int resourcesConvertation(const char * str)
{
    for(int i = 0; i < CELL_RES_COUNT; i++)
    {
        if(strcmp(str, xml_res_names[i]) == 0)
        {
            return xml_res_values[i];
        }
    }

    return 0;
}

/*
    Auxiliary arrays for privileges convertations from their names to their
    definied (in game/world/definitions.h) constants.
*/
const char xml_prvl_names[][16] = {"build_city", "build_mine", "can_float",
    "chop_trees"};

const int xml_prvl_values[] = {UNIT_PRVL_BUILD_CITY, UNIT_PRVL_BUILD_MINE,
    UNIT_PRVL_CAN_FLOAT, UNIT_PRVL_CHOP_TREES};

int privilegesConvertation(const char * str)
{
    for(int i = 0; i < UNIT_PRVL_COUNT; i++)
    {
        if(strcmp(str, xml_prvl_names[i]) == 0)
        {
            return xml_prvl_values[i];
        }
    }

    return 0;
}

/*
    Parsing functions (expat handlers).
*/
void elementStart(void * data, const char * name, const char ** attr)
{
    XMLParserData * p_data = (XMLParserData *) data;

    for(int i = 0; i < XML_STATES; i++)
    {
        // Check for tag and required state.
        if(strcmp(name, xml_tags[i]) == 0 && p_data -> state == xml_parents[i])
        {
            p_data -> state = xml_states[i];
            break;
        }
    }
}

void elementEnd(void * data, const char * name)
{
    XMLParserData * p_data = (XMLParserData *) data;

    // Set state to parent.
    for(int i = 0; i < XML_STATES; i++)
    {
        if(xml_states[i] == p_data -> state)
        {
            p_data -> state = xml_parents[i];
            break;
        }
    }
}

void elementContent(void * data, const char * s, int len)
{
    // Copy string.
    char * temp = malloc(sizeof(char) * (len + 1));
    memcpy(temp, s, sizeof(char) * len);
    temp[len] = '\0';

    // Remove spaces.
    strTrimSpaces(temp);

    // If it is non-zero length string, check for parser state.
    if(strlen(temp) != 0)
    {
        // Parser data.
        XMLParserData * p_data = (XMLParserData *) data;
        void * temp_data = NULL;

        switch(p_data -> state)
        {
            case XML_MAP_ROWS:
                ((WorldProperties *) p_data -> data) -> map_r = atoi(temp);
            break;

            case XML_MAP_COLUMNS:
                ((WorldProperties *) p_data -> data) -> map_c = atoi(temp);
            break;

            case XML_PLAYERS_COUNT:
                ((WorldProperties *) p_data -> data) -> players_count = atoi(temp);
            break;

            case XML_PLAYERS_NAMES:
                ((WorldProperties *) p_data -> data) -> player_names = strSplit(',', temp);
            break;

            case XML_PLAYERS_CITIES:
                ((WorldProperties *) p_data -> data) -> player_cities = strSplit(',', temp);
            break;

            case XML_UNIT_ID:
                // Here we do one _bad_ thing: we ignore unit id, what we read.
                // It won't matter, if all ids of units in units.xml file go
                // in ascending from 0 to (some number) without any missing
                // numbers between.
                // TODO Fix this. Somewhen.
                temp_data = createUnitCommonInfo();
                daPrepend(p_data -> data, temp_data);
            break;

            case XML_UNIT_NAME:
                temp_data = malloc(sizeof(char) * (strlen(temp) + 1));
                ((UnitCommonInfo *) daGetLast(p_data -> data)) -> name = strcpy(temp_data, temp);
            break;

            case XML_UNIT_CHAR:
                ((UnitCommonInfo *) daGetLast(p_data -> data)) -> c = temp[0];
            break;

            case XML_UNIT_HEALTH:
                ((UnitCommonInfo *) daGetLast(p_data -> data)) -> max_health = atoi(temp);
            break;

            case XML_UNIT_DAMAGE:
                ((UnitCommonInfo *) daGetLast(p_data -> data)) -> max_damage = atoi(temp);
            break;

            case XML_UNIT_MOVES:
                if(TEST_MODE)
                {
                    ((UnitCommonInfo *) daGetLast(p_data -> data)) -> max_moves = 9000;
                }
                else
                {
                    ((UnitCommonInfo *) daGetLast(p_data -> data)) -> max_moves = atoi(temp);
                }
            break;

            case XML_UNIT_HIRING_TURNS:
                if(TEST_MODE)
                {
                    ((UnitCommonInfo *) daGetLast(p_data -> data)) -> hiring_turns = 1;
                }
                else
                {
                    ((UnitCommonInfo *) daGetLast(p_data -> data)) -> hiring_turns = atoi(temp);
                }
            break;

            case XML_UNIT_GOLD_DROP:
                ((UnitCommonInfo *) daGetLast(p_data -> data)) -> gold_drop = atoi(temp);
            break;

            case XML_UNIT_PRIVILEGES:
                ((UnitCommonInfo *) daGetLast(p_data -> data)) -> privileges = strSplitAndConvert(',', temp, &privilegesConvertation);
            break;

            case XML_UNIT_RESOURCES:
                ((UnitCommonInfo *) daGetLast(p_data -> data)) -> resources = strSplitAndConvert(',', temp, &resourcesConvertation);
            break;

            case XML_TECH_ID:
                // Same note as for XML_UNIT_ID.
                // TODO Fix this. Somewhen.
                temp_data = createTechnologyParseInfo();
                daPrepend(p_data -> data, temp_data);
                ((Technology *) ((TechnologyParseInfo *) daGetLast(p_data -> data)) -> tech_in_tree -> data) -> id = ((DynArray *) p_data -> data) -> length - 1;
            break;

            case XML_TECH_NAME:
                temp_data = malloc(sizeof(char) * (strlen(temp) + 1));
                ((Technology *) ((TechnologyParseInfo *) daGetLast(p_data -> data)) -> tech_in_tree -> data) -> name = strcpy(temp_data, temp);
            break;

            case XML_TECH_TURNS:
                ((Technology *) ((TechnologyParseInfo *) daGetLast(p_data -> data)) -> tech_in_tree -> data) -> turns = atoi(temp);
            break;

            case XML_TECH_PROVIDES_UNITS:
                ((Technology *) ((TechnologyParseInfo *) daGetLast(p_data -> data)) -> tech_in_tree -> data) -> provides_units = strSplitToInts(',', temp);
            break;

            case XML_TECH_PROVIDES_TECHS:
                ((TechnologyParseInfo *) daGetLast(p_data -> data)) -> provides_technologies = strSplitToInts(',', temp);
            break;

            case XML_TECH_REQUIRES_RESOURCES:
                ((Technology *) ((TechnologyParseInfo *) daGetLast(p_data -> data)) -> tech_in_tree -> data) -> requires_resources = strSplitAndConvert(',', temp, &resourcesConvertation);
            break;
        }
    }

    free(temp);
}
