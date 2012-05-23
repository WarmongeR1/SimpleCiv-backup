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
#include <math.h>

#include "../../game/world/definitions.h"
#include "../graph/graph.h"
#include "unit_common_info.h"
#include "unit.h"
#include "../player/player.h"

#include "../city/city.h"

Unit * createUnit(World * world, unsigned int r, unsigned int c, unsigned char unit_id, Player * player)
{
    // Cell (r,c) already have a unit.
    if(getNeighbour( getMapCell(world -> map, r, c), EDGE_CELL_UNIT ) != NULL)
    {
        return NULL;
    }

    // Allocate some memory.
    Unit * unit = malloc(sizeof(Unit));
    UnitCommonInfo * info = (UnitCommonInfo *) daGetByIndex(world -> units_info, unit_id);
    Node * node = createGraph(NODE_UNIT, unit);

    // Copying basic info.
    unit -> unit_id = unit_id;
    unit -> health = info -> max_health;
    unit -> moves = info -> max_moves;

    // Adding owner info.
    unit -> owner = player;
    listPrepend(player -> units, unit);

    // Adding coordinates.
    unit -> r = r;
    unit -> c = c;
    addEdge(getMapCell(world -> map, r, c), node, EDGE_CELL_UNIT);

    return unit;
}

void destroyUnitNodeData(unsigned char type, void * data)
{
    free(data);
}

void destroyUnit(World * world, Unit * unit)
{
    // TODO Awful. Maybe rewrite?
    Player * player = unit -> owner;

    // Removing unit from map.
    Node * n = getMapCell(world -> map, unit -> r, unit -> c);
    Edge * edge;
    for(int i = 0; i < n -> edges -> length; i++)
    {
        edge = n -> edges -> data[i];
        if(edge -> type == EDGE_CELL_UNIT)
        {
            break;
        }
    }

    // Removing node with unit.
    destroyNode(edge -> target, &destroyUnitNodeData);

    // Removing pointer to him from map.
    daRemoveByPointer(n -> edges, edge, &free);

    // Removing pointer to him from owner's list.
    if(player != NULL)
    {
        listDeleteByPointer(player -> units, unit, NULL);
    }
}

void unitsFight(World * world, Unit ** unit1, Unit ** unit2)
{
    // Getting units' info.
    UnitCommonInfo * u1 = (UnitCommonInfo *) daGetByIndex(world -> units_info, (* unit1) -> unit_id);
    UnitCommonInfo * u2 = (UnitCommonInfo *) daGetByIndex(world -> units_info, (* unit2) -> unit_id);

    // Calculating max damage.
    float damage1 = (float) (* unit1) -> health / u1 -> max_health * u1 -> max_damage;
    float damage2 = (float) (* unit2) -> health / u2 -> max_health * u2 -> max_damage;

    // Random damage between BALANCE_UNIT_DAMAGE_MIN% and
    // BALANCE_UNIT_DAMAGE_MAX% of damage1/damage2.
    int delta = BALANCE_UNIT_DAMAGE_MAX - BALANCE_UNIT_DAMAGE_MIN;
    damage1 *= (float) (rand() % delta + BALANCE_UNIT_DAMAGE_MIN) / 100.0f;
    damage2 *= (float) (rand() % delta + BALANCE_UNIT_DAMAGE_MIN) / 100.0f;

    // Checking there're still alive or not.
    if( (* unit1) -> health <= ceil(damage2) )
    {
        // Player2 gets drop.
        if((* unit2) -> owner != NULL)
        {
            (* unit2) -> owner -> gold += u1 -> gold_drop;
        }
        // Destroy unit.
        destroyUnit(world, * unit1);
        * unit1 = NULL;
    }
    else
    {
        (* unit1) -> health -= ceil(damage2);
    }

    if( (* unit2) -> health <= ceil(damage1) )
    {
        // Player1 gets drop.
        if((* unit1) -> owner != NULL)
        {
            (* unit1) -> owner -> gold += u2 -> gold_drop;
        }
        // Destroy unit.
        destroyUnit(world, * unit2);
        * unit2 = NULL;
    }
    else
    {
        (* unit2) -> health -= ceil(damage1);
    }
}

void developUnit(void * data, DynArray * info)
{
    Unit * unit = (Unit *) data;
    UnitCommonInfo * unit_info = (UnitCommonInfo *) daGetByIndex(info, unit -> unit_id);

    // Unit needs money.
    if(unit -> owner -> gold <= BALANCE_UNIT_SALARY)
    {
        // No money, no honey.
        unit -> health -= ceil(BALANCE_UNIT_HEALTH_DELTA * unit_info -> max_health);
        if(unit -> health < 0)
        {
            // But he can't die from hunger.
            unit -> health = ceil((float) BALANCE_UNIT_HEALTH_DELTA * unit_info -> max_health);
        }
    }
    else
    {
        // Pay salary.
        unit -> owner -> gold -= BALANCE_UNIT_SALARY;
        // Heals unit.
        if(unit -> moves == unit_info -> max_moves)
        {
            if(unit -> health + BALANCE_UNIT_SALARY * unit_info -> max_health < unit_info -> max_health)
            {
                unit -> health = unit -> health + BALANCE_UNIT_SALARY * unit_info -> max_health;
            }
            else
            {
                unit -> health = unit_info -> max_health;
            }
        }
    }

    unit -> moves = unit_info -> max_moves;
}

void makeUnitNeutral(void * data)
{
    Unit * u = (Unit *) data;

    u -> owner = NULL;
}




int unitMoveShip(Unit * unit,UnitCommonInfo * u_info, Node * destination)
{
    // Unit is a ship?
    if(u_info -> privileges != NULL && iaSearchForData(u_info -> privileges, UNIT_PRVL_CAN_FLOAT) != -1)
    {
        // Cannot go to the land, if there is no city.
        if( ((Cell *) destination -> data) -> territory != CELL_TYPE_WATER && getNeighbour(destination, EDGE_CELL_CITY) == NULL )
        {
            return 3;
        }
    }
}


int unitMoveAnotherUnit(World * world, Unit * unit,UnitCommonInfo * u_info, Node * destination)
{
    // There is another unit?
    if(getNeighbour(destination, EDGE_CELL_UNIT) != NULL)
    {
        Node * neighbour = getNeighbour(destination, EDGE_CELL_UNIT);
        Unit * another_unit = (Unit *) neighbour -> data;
        // Fight!
        if(another_unit -> owner != unit -> owner)
        {
            unit -> moves = 0;
            unitsFight(world, &unit, &another_unit);
            return 1;
        }
        return 3;
    }
}



int unitMoveCity(World * world, Unit * unit, Node * destination)
{
    Node * neighbour = getNeighbour(destination, EDGE_CELL_CITY);
    City * city = (City *) neighbour -> data;
    // Capture!
    if(city -> owner != unit -> owner)
    {
        unit -> moves = 0;
        Player * prev_owner = city -> owner;
        Player * new_owner = unit -> owner;

        // Change owner.
        city -> owner = new_owner;
        // Remove city from old owner's cities list.
        listDeleteByPointer(prev_owner -> cities, city, NULL);
        // Add to new owner.
        listPrepend(new_owner -> cities, city);

        // If prev_owner doesn't have cities any more, destroy him.
        if(prev_owner -> cities -> length == 0)
        {
            // Make all units neutral.
            listForEach(prev_owner -> units, &makeUnitNeutral);
            // Delete player.
            listDeleteByPointer(world -> players, prev_owner, &destroyPlayer);
            // Decrement players' count.
            world -> properties -> players_count -= 1;
            // It is last player.
            if(world -> properties -> players_count == 1)
            {
                return 2;
            }
        }
        unit -> r = city -> r;
        unit -> c = city -> c;
        return 0;
    }
}

void unitMoveMotion(World * world, Node * current_cell, Node * destination, int direction, Unit * unit, Edge * edge)
{
        // Motion.
    unit -> moves--;
    switch(direction)
    {
        case EDGE_CELL_TOP:    unit -> r--; break;
        case EDGE_CELL_BOTTOM: unit -> r++; break;
        case EDGE_CELL_RIGHT:  unit -> c++; break;
        case EDGE_CELL_LEFT:   unit -> c--; break;
    }

    if(unit -> r >= world -> properties -> map_r)
    {
        unit -> r %= world -> properties -> map_r;
    }
    else if(unit -> r < 0)
    {
        unit -> r += world -> properties -> map_r;
    }

    if(unit -> c >= world -> properties -> map_c)
    {
        unit -> c %= world -> properties -> map_c;
    }
    else if(unit -> c < 0)
    {
        unit -> c += world -> properties -> map_c;
    }

    daRemoveByPointer(current_cell -> edges, edge, NULL);
    daPrepend(destination -> edges, edge);

    revealFogRadius(unit -> owner -> fog, unit -> r, unit -> c, BALANCE_UNIT_VIEW_RADIUS);
}


int moveUnit(World * world, Node * current_cell, int direction)
{
    // Getting edge to unit.
    Edge * edge;
    for(int i = 0; i < current_cell -> edges -> length; i++)
    {
        edge = current_cell -> edges -> data[i];
        if(edge -> type == EDGE_CELL_UNIT)
        {
            break;
        }
    }

    // Getting unit, his info and destination.
    Unit * unit = (Unit *) edge -> target -> data;
    UnitCommonInfo * u_info = (UnitCommonInfo *) daGetByIndex(world -> units_info, unit -> unit_id);
    Node * destination = getNeighbour(current_cell, direction);

    // Unit cannot go any more.
    if(unit -> moves == 0)
    {
        return 3;
    }

    // There is another unit?
    if (getNeighbour(destination, EDGE_CELL_UNIT) != NULL)
    {
        if (unitMoveAnotherUnit(world, unit , u_info, destination) == 3) return 3;
        else return 1;
    }

    // There is a city?
    if(getNeighbour(destination, EDGE_CELL_CITY) != NULL)
    {
        if (unitMoveCity(world, unit, destination) == 2) return 2;
        else return 1;
    }

    if(u_info -> privileges != NULL && iaSearchForData(u_info -> privileges, UNIT_PRVL_CAN_FLOAT) != -1)
    {
        if (unitMoveShip(unit, u_info, destination) == 3)
            return 3;
    }

    // Cannot go to the water.
    if(((Cell *) destination -> data) -> territory == CELL_TYPE_WATER)
    {
        // Not a ship.
        if(u_info -> privileges == NULL || iaSearchForData(u_info -> privileges, UNIT_PRVL_CAN_FLOAT) == -1)
        {
            return 3;
        }
    }

    unitMoveMotion(world, current_cell, destination, direction, unit, edge);



    return 0;
}

