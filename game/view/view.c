#include <stdlib.h>

#include "../../modules/graph/graph.h"
#include "../../modules/player/player.h"
#include "../../modules/city/city.h"
#include "../../modules/unit/unit.h"
#include "../../modules/technology/technology.h"
#include "../world/definitions.h"
#include "view.h"

View * createView(World * world)
{
    View * result = malloc(sizeof(View));

    // Initialize ncurses.
    initscr();
    raw();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    start_color();
    attroff(A_BOLD);;
    attron(COLOR_PAIR(0));

    // Color pairs. 0 — default, 1-5 — territories' colors.
    init_pair(0, COLOR_WHITE, COLOR_BLACK);
    init_pair(CELL_TYPE_WATER, COLOR_BLUE,  COLOR_BLACK);
    init_pair(CELL_TYPE_GRASS, COLOR_GREEN, COLOR_BLACK);
    init_pair(CELL_TYPE_HILL, COLOR_WHITE, COLOR_BLACK);
    init_pair(CELL_TYPE_TREE, COLOR_GREEN, COLOR_BLACK);
    init_pair(CELL_TYPE_MOUNTAIN, COLOR_WHITE, COLOR_BLACK);

    getmaxyx(stdscr, result -> rows, result -> columns);

    result -> sidebar = 0.8 * result -> columns;

    // Min size of sidepanel is 20 chars.
    if(result -> sidebar > result -> columns - 20)
    {
        result -> sidebar = result -> columns - 20;
    }

    result -> cur_r = result -> rows / 2;
    result -> cur_c = result -> sidebar / 2;
    result -> map_r = result -> cur_r - 1;
    result -> map_c = result -> cur_c - 1;

    result -> current_cell = getCell(world -> graph_map, result -> map_r, result -> map_c);

    result -> chooser = NULL;

    return result;
}

void destroyView(View * view)
{
    endwin();
    free(view);
}

ViewChooser * createTechChooser(World * world)
{
    ViewChooser * chooser = malloc(sizeof(ViewChooser));

    chooser -> ids = iaCreate();
    chooser -> current = -1;
    chooser -> start_r = -1;

    Player * player = (Player *) world -> graph_players -> data;

    int count = 0;
    for(int i = 0; i < player -> available_techs -> length; i++)
    {
        int value = iaGetByIndex(player -> available_techs, i);
        if(value == TECH_AVAILABLE)
        {
            Technology * t = (Technology *) ((Node *) daGetByIndex(world -> techs_info, i)) -> data;
            // Checking for resources.
            if(t -> requires_resources == NULL)
            {
                // Nothing requires. Great.
                count++;
                iaPrepend(chooser -> ids, t -> id);
            }
            else
            {
                // Checking for each resource.
                char okay = 1;
                for(int j = 0; j < t -> requires_resources -> length; j++)
                {
                    // Getting resource id.
                    int id = iaGetByIndex(t -> requires_resources, j);
                    // Does player have this resources?
                    if(iaGetByIndex(player -> resources, id) == 0)
                    {
                        // Sad but true.
                        okay = 0;
                        break;
                    }
                }
                // You're lucky man.
                if(okay == 1)
                {
                    count++;
                    iaPrepend(chooser -> ids, t -> id);
                }
            }
            // It is current research?
            if(t -> id == player -> research -> id)
            {
                chooser -> current = count - 1;
            }
        }
    }

    return chooser;
}

ViewChooser * createUnitChooser(World * world)
{
    ViewChooser * chooser = malloc(sizeof(ViewChooser));

    chooser -> ids = iaCreate();

    return chooser;
}

void destroyChooser(ViewChooser * chooser)
{
    iaDestroy(chooser -> ids);
    free(chooser);
}

void drawGeneralView(World * world, View * view)
{
    erase();

    // Copying rows, columns and sidebar.
    int r = view -> rows;
    int c = view -> columns;
    int s = view -> sidebar;

    // Drawing main interface.
    drawBox(0, 0, r, c);
    drawVertLine(0, r, s);
    drawHorizLine(s, c - s, SIDEBAR_CELL_BLOCK);
    drawHorizLine(s, c - s, r - 3);

    // Some static strings.
    putInMiddle(r - 4, s + 1, c - s - 2, "Press h for help");
}

void drawHelpView(World * world, View * view)
{
    erase();

    // Drawing main interface.
    drawBox(0, 0, view -> rows, view -> columns);

    // Drawing help info.
    int line = 2; int column = 20;

    // Map window.
    attron(A_BOLD); mvprintw(line++, 3, "Map:"); attroff(A_BOLD);
    mvprintw(line,   3,      "Arrow keys");
    mvprintw(line++, column, "Move unit/cursor.");
    mvprintw(line,   3,      "Enter");
    mvprintw(line++, column, "End turn.");
    mvprintw(line,   3,      "Space");
    mvprintw(line++, column, "Choose unit to move or open city hiring dialog.");
    mvprintw(line,   3,      "c");
    mvprintw(line++, column, "Create city (if settler is selected).");
    mvprintw(line,   3,      "t");
    mvprintw(line++, column, "Open research dialog.");
    mvprintw(line,   3,      "h");
    mvprintw(line++, column, "Show this help.");
    mvprintw(line,   3,      "q");
    mvprintw(line++, column, "Quit game.");

    // Hiring dialog.
    line++;
    attron(A_BOLD); mvprintw(line++, 3, "Hiring dialog:"); attroff(A_BOLD);
    mvprintw(line,   3,      "Up/down keys");
    mvprintw(line++, column, "Choose unit.");
    mvprintw(line,   3,      "Enter");
    mvprintw(line++, column, "Start hiring (will terminate current hiring).");
    mvprintw(line,   3,      "q");
    mvprintw(line++, column, "Quit to map.");

    // Research dialog.
    line++;
    attron(A_BOLD); mvprintw(line++, 3, "Research dialog:"); attroff(A_BOLD);
    mvprintw(line,   3,      "Up/down keys");
    mvprintw(line++, column, "Choose technology.");
    mvprintw(line,   3,      "Enter");
    mvprintw(line++, column, "Start research (will terminate current research).");
    mvprintw(line,   3,      "q");
    mvprintw(line++, column, "Quit to map.");

    // One hint.
    line++;
    mvprintw(line,   3,      "Press q to hide this help.");

    // That's all.
    line++;
    move(line, 3);
}

void drawTechView(World * world, View * view)
{
    erase();

    // Drawing interface.
    drawBox(0, 0, view -> rows, view -> columns);

    // Drawing player name and other info.
    int line = 2;
    Player * player = (Player *) world -> graph_players -> data;
    attron(A_BOLD); mvprintw(line++, 3, "%s's researches", player -> name); attroff(A_BOLD);
    mvprintw(line++, 3, "%d gold, %d gold spents on reasearches every turn", player -> gold, player -> research -> delta);

    // Drawing space.
    line++;

    // Drawing research.
    attron(A_BOLD); mvprintw(line++, 3, "Current research:"); attroff(A_BOLD);
    if(player -> research -> id == -1)
    {
        mvprintw(line++, 3, "No research");
    }
    else
    {
        // Getting technology.
        Node * n = (Node *) daGetByIndex(world -> techs_info, player -> research -> id);
        Technology * t = (Technology *) n -> data;
        // Printing it.
        mvprintw(line++, 3, "%s (%d/%d turns)", t -> name, player -> research -> turns, t -> turns);
        // If player doesn't have enought money…
        if(player -> gold < player -> research -> delta)
        {
            mvprintw(line++, 3, "(Your research is suspended: you don't have enough gold!)");
        }
    }

    // Drawing space.
    line++;

    // Drawing available technologies.
    attron(A_BOLD); mvprintw(line++, 3, "Available for researching:"); attroff(A_BOLD);
    // Remembering start_r line.
    view -> chooser -> start_r = line;
    mvprintw(line++, 3, "[ ] Do not explore anything");
    // Getting ids.
    IntArray * ids = view -> chooser -> ids;
    for(int i = 0; i < ids -> length; i++)
    {
        Technology * t = (Technology *) ((Node *) daGetByIndex(world -> techs_info, iaGetByIndex(ids, i))) -> data;
        mvprintw(line++, 3, "[ ] %s (%d turns)", t -> name, t -> turns);
    }

    // Drawing researched technologies.
    line++;
    attron(A_BOLD); mvprintw(line++, 3, "Already researched:"); attroff(A_BOLD);
    for(int i = 0; i < player -> available_techs -> length; i++)
    {
        int value = iaGetByIndex(player -> available_techs, i);
        if(value == TECH_RESEARCHED)
        {
            Technology * t = (Technology *) ((Node *) daGetByIndex(world -> techs_info, i)) -> data;
            mvprintw(line++, 3, "%s", t -> name);
        }
    }

    mvaddch(view -> chooser -> start_r + view -> chooser -> current + 1, 4, '*');
    move(view -> chooser -> start_r + view -> chooser -> current + 1, 4);
}

void drawCityView(World * world, View * view)
{
    erase();

    // Drawing interface.
    drawBox(0, 0, view -> rows, view -> columns);

    // Drawing player name and other info.
    Player * player = (Player *) world -> graph_players -> data;
    City * city = (City * ) getNeighbour(view -> current_cell, EDGE_CELL_CITY) -> data;
    attron(A_BOLD); mvprintw(2, 3, "%s's city %s", player -> name, city -> name); attroff(A_BOLD);

    // Drawing available units.
    attron(A_BOLD); mvprintw(4, 3, "Available for hiring:"); attroff(A_BOLD);
    int line = 5; int start_r = line; int count = 0;
    for(int i = 0; i < player -> available_units -> length; i++)
    {
        int value = iaGetByIndex(player -> available_units, i);
        if(value == UNIT_AVAILABLE)
        {
            //~ Unit * u = (Unit *) ((Node *) daGetByIndex(world -> units_info, i)) -> data;
            //~ // Checking for hiring.
            //~ if(u -> requires_resources == NULL)
            //~ {
                //~ // Nothing requires. Great.
                //~ count++;
                //~ mvprintw(line++, 3, "[ ] %s (%d turns)", u -> name, u -> turns);
            //~ }
            //~ else
            //~ {
                //~ // Checking for each resource.
                //~ char okay = 1;
                //~ for(int j = 0; j < u -> requires_resources -> length; j++)
                //~ {
                    //~ // Getting resource id.
                    //~ int id = iaGetByIndex(u -> requires_resources, j);
                    //~ // Does player have this resources?
                    //~ if(iaGetByIndex(player -> resources, id) == 0)
                    //~ {
                        //~ // Sad but true.
                        //~ okay = 0;
                        //~ break;
                    //~ }
                //~ }
                //~ // You're lucky man.
                //~ if(okay == 1)
                //~ {
                    //~ count++;
                    //~ mvprintw(line++, 3, "[ ] %s (%d turns)", u -> name, u -> turns);
                //~ }
            //~ }
        }
    }

    if(count == 0)
    {
        mvprintw(line++, 3, "No units");
    }
    move(start_r, 4);
}

void drawPlayerInfo(World * world, View * view)
{
    int s = view -> sidebar;
    int len = view -> columns - view -> sidebar - 2;

    clearBlock(SIDEBAR_PLAYER_BLOCK + 1, s + 1, 7, len);

    // Player.
    Player * player = (Player *) world -> graph_players -> data;

    // Player info.
    attron(A_BOLD); putInMiddle(SIDEBAR_PLAYER_BLOCK + 1, s + 1, len, "%s", player -> name); attroff(A_BOLD);
    // Researching.
    if(player -> research -> id == -1)
    {
        mvprintw(SIDEBAR_PLAYER_BLOCK + 2, s + 1, "No researches");
    }
    else
    {
        // Getting technology.
        Node * n = (Node *) daGetByIndex(world -> techs_info, player -> research -> id);
        Technology * t = (Technology *) n -> data;
        // Printing it.
        putInLeft(SIDEBAR_PLAYER_BLOCK + 2, s + 1, len, "%d/%d %s", player -> research -> turns, t -> turns, t -> name);
    }
    // Gold.
    mvprintw(SIDEBAR_PLAYER_BLOCK + 3, s + 1, "Gold");
    putInRight(SIDEBAR_PLAYER_BLOCK + 3, s + 1, len, "%d-%d", player -> gold, player -> research -> delta);
    // Resources.
    const char res_names[][10] = {"", "Bronze", "Iron", "Coal", "Gunpowder", "Horses"};
    for(int i = 1; i <= CELL_RES_COUNT - 1; i++)
    {
        mvprintw(SIDEBAR_PLAYER_BLOCK + 3 + i, s + 1, res_names[i]);
        putInRight(SIDEBAR_PLAYER_BLOCK + 3 + i, s + 1, len, "%d", iaGetByIndex(player -> resources, i));
    }
}

void drawCellInfo(World * world, View * view)
{
    int s = view -> sidebar;
    int r = view -> rows;
    int len = view -> columns - view -> sidebar - 2;

    clearBlock(SIDEBAR_CELL_BLOCK + 1, s + 1, 11, len);

    // Cell.
    Cell * c = (Cell *) view -> current_cell -> data;
    attron(A_BOLD);

    // Type of territory.
    switch(c -> territory)
    {
        case CELL_TYPE_WATER:    mvprintw(SIDEBAR_CELL_BLOCK + 1, s + 1, "Water    "); break;
        case CELL_TYPE_GRASS:    mvprintw(SIDEBAR_CELL_BLOCK + 1, s + 1, "Grass    "); break;
        case CELL_TYPE_TREE:     mvprintw(SIDEBAR_CELL_BLOCK + 1, s + 1, "Forest   "); break;
        case CELL_TYPE_HILL:     mvprintw(SIDEBAR_CELL_BLOCK + 1, s + 1, "Hill     "); break;
        case CELL_TYPE_MOUNTAIN: mvprintw(SIDEBAR_CELL_BLOCK + 1, s + 1, "Mountains"); break;
    }

    // Resources.
    switch(c -> resources)
    {
        case CELL_RES_NONE:      mvprintw(SIDEBAR_CELL_BLOCK + 2, s + 1, "            "); break;
        case CELL_RES_BRONZE:    mvprintw(SIDEBAR_CELL_BLOCK + 2, s + 1, "Bronze      "); break;
        case CELL_RES_IRON:      mvprintw(SIDEBAR_CELL_BLOCK + 2, s + 1, "Iron        "); break;
        case CELL_RES_COAL:      mvprintw(SIDEBAR_CELL_BLOCK + 2, s + 1, "Coal        "); break;
        case CELL_RES_GUNPOWDER: mvprintw(SIDEBAR_CELL_BLOCK + 2, s + 1, "Gunpowder   "); break;
        case CELL_RES_HORSES:    mvprintw(SIDEBAR_CELL_BLOCK + 2, s + 1, "Horses      "); break;
        case CELL_RES_MUSHROOMS: mvprintw(SIDEBAR_CELL_BLOCK + 2, s + 1, "Mushrooms :O"); break;
    }

    attroff(A_BOLD);

    // City.
    Node * city = getNeighbour(view -> current_cell, EDGE_CELL_CITY);
    if(city != NULL)
    {
        City * c = (City *) city -> data;
        attron(A_BOLD); mvprintw(SIDEBAR_CELL_BLOCK + 3, s + 1, "City:  "); attroff(A_BOLD);
        putInLeft(SIDEBAR_CELL_BLOCK + 4, s + 2, len - 2, c -> name);
        putInLeft(SIDEBAR_CELL_BLOCK + 5, s + 2, len - 2, c -> owner -> name);
        mvprintw(SIDEBAR_CELL_BLOCK + 6, s + 2, "People");
        putInRight(SIDEBAR_CELL_BLOCK + 6, s + 2, len - 2, "%d", c -> population);
    }

    // Unit.
    Node * unit = getNeighbour(view -> current_cell, EDGE_CELL_UNIT);
    if(unit != NULL)
    {
        Unit * u = (Unit *) unit -> data;
        UnitCommonInfo * u_info = (UnitCommonInfo *) daGetByIndex(world -> units_info, u -> unit_id);
        attron(A_BOLD); mvprintw(SIDEBAR_CELL_BLOCK + 7, s + 1, "Unit:  "); attroff(A_BOLD);
        putInLeft(SIDEBAR_CELL_BLOCK + 8, s + 2, len - 2, u_info -> name);
        putInLeft(SIDEBAR_CELL_BLOCK + 9, s + 2, len - 2, u -> owner -> name);
        mvprintw(SIDEBAR_CELL_BLOCK + 10, s + 2, "HP");
        putInRight(SIDEBAR_CELL_BLOCK + 10, s + 2, len - 2, "%d/%d", u -> health, u_info -> max_health);
        mvprintw(SIDEBAR_CELL_BLOCK + 11, s + 2, "Moves");
        putInRight(SIDEBAR_CELL_BLOCK + 11, s + 2, len - 2, "%d/%d", u -> moves, u_info -> max_moves);
    }

    // Other info.
    clearBlock(r - 2, s + 1, 1, len);
    putInMiddle(r - 2, s + 1, len, "(%d,%d)", view -> map_r, view -> map_c);
}

void drawNode(World * world, Node * current)
{
    if(getNeighbour(current, EDGE_CELL_CITY) != NULL)
    {
        addch('M');
    }
    else if(getNeighbour(current, EDGE_CELL_UNIT) != NULL)
    {
        // Getting unit's char.
        Unit * unit = (Unit *) getNeighbour(current, EDGE_CELL_UNIT) -> data;
        UnitCommonInfo * u_info = (UnitCommonInfo *) daGetByIndex(world -> units_info, unit -> unit_id);
        addch(u_info -> c);
    }
    else
    {
        unsigned char type = ((Cell *) current -> data) -> territory;
        attron(COLOR_PAIR(type));
        switch ( type )
        {
            case CELL_TYPE_WATER    : addch('.'); break;
            case CELL_TYPE_GRASS    : addch('_'); break;
            case CELL_TYPE_HILL     : addch('-'); break;
            case CELL_TYPE_TREE     : addch('T'); break;
            case CELL_TYPE_MOUNTAIN : addch('^'); break;
            default                 : addch('E'); break;
        }
        attroff(COLOR_PAIR(type));
    }
}

void drawMap(World * world, View * view)
{
    int start_row = 1,    end_row = view -> rows - 2;
    int start_column = 1, end_column = view -> sidebar - 1;

    Node * current = ((Player *) world -> graph_players -> data) -> graph_map;
    Node * line = current;

    for(int i = start_row; i <= end_row; i++)
    {
        for(int j = start_column; j <= end_column; j++)
        {
            move(i, j);
            drawNode(world, current);
            current = getNeighbour(current, EDGE_CELL_RIGHT);
        }
        line = getNeighbour(line, EDGE_CELL_BOTTOM);
        current = line;
    }
    move(view->cur_r,view->cur_c);
}

int viewProcess(World * world, View * view, List * list)
{
    Player * player = (Player *) world -> graph_players -> data;

    if(list != NULL)
    {
        ListElement * le = list -> head;
        for (int i = 0; i < list -> length; i++)
        {
            Message * message = (Message *) le -> data;
            switch(message -> type)
            {
                case VIEW_ESCAPE:
                    listDestroy(list, &destroyMessage);
                    return 0;
                break;

                case VIEW_REDRAW_ALL:
                    drawGeneralView(world, view);
                    drawPlayerInfo(world, view);
                    drawCellInfo(world, view);
                    drawMap(world, view);
                    move(view -> cur_r, view -> cur_c);
                break;

                case VIEW_REDRAW_INFO:
                    drawPlayerInfo(world, view);
                    drawCellInfo(world, view);
                    move(view -> cur_r, view -> cur_c);
                break;

                case VIEW_REDRAW_MAP:
                    drawMap(world, view);
                break;

                case VIEW_MOVE_CURSOR_TOP:
                    view -> current_cell = getNeighbour(view -> current_cell, EDGE_CELL_TOP);
                    view -> map_r--; view -> map_r = view -> map_r < 0 ? view -> map_r + world -> properties -> map_r : view -> map_r;
                    if(view -> cur_r > 5)
                    {
                        view -> cur_r--;
                    }
                    else
                    {
                        player -> graph_map = getNeighbour(player -> graph_map, EDGE_CELL_TOP);
                        drawMap(world, view);
                    }
                    drawCellInfo(world, view);
                    move(view -> cur_r, view -> cur_c);
                break;

                case VIEW_MOVE_CURSOR_BOTTOM:
                    view -> current_cell = getNeighbour(view -> current_cell, EDGE_CELL_BOTTOM);
                    view -> map_r = (view -> map_r + 1) % world -> properties -> map_r;
                    if(view -> cur_r < view -> rows - 5)
                    {
                        view -> cur_r++;
                    }
                    else
                    {
                        player -> graph_map = getNeighbour(player -> graph_map, EDGE_CELL_BOTTOM);
                        drawMap(world, view);
                    }
                    drawCellInfo(world, view);
                    move(view -> cur_r, view -> cur_c);
                break;

                case VIEW_MOVE_CURSOR_RIGHT:
                    view -> current_cell = getNeighbour(view -> current_cell, EDGE_CELL_RIGHT);
                    view -> map_c = (view -> map_c + 1) % world -> properties -> map_c;
                    if(view -> cur_c < view -> sidebar - 5)
                    {
                        view -> cur_c++;
                    }
                    else
                    {
                        player -> graph_map = getNeighbour(player -> graph_map, EDGE_CELL_RIGHT);
                        drawMap(world, view);
                    }
                    drawCellInfo(world, view);
                    move(view -> cur_r, view -> cur_c);
                break;

                case VIEW_MOVE_CURSOR_LEFT:
                    view -> current_cell = getNeighbour(view -> current_cell, EDGE_CELL_LEFT);
                    view -> map_c--; view -> map_c = view -> map_c < 0 ? view -> map_c + world -> properties -> map_c : view -> map_c;
                    if(view -> cur_c > 5)
                    {
                        view -> cur_c--;
                        move(view -> cur_r, view -> cur_c);
                    }
                    else
                    {
                        player -> graph_map = getNeighbour(player -> graph_map, EDGE_CELL_LEFT);
                        drawMap(world, view);
                    }
                    drawCellInfo(world, view);
                    move(view -> cur_r, view -> cur_c);
                break;

                case VIEW_REDRAW_TECH_DIALOG:
                    drawTechView(world, view);
                break;

                case VIEW_MOVE_TECH_CURSOR_TOP:
                    if(view -> chooser -> current > -1)
                    {
                        addch(' ');
                        (view -> chooser -> current)--;
                        mvaddch(view -> chooser -> start_r + view -> chooser -> current + 1, 4, '*');
                        move(view -> chooser -> start_r + view -> chooser -> current + 1, 4);
                    }
                break;

                case VIEW_MOVE_TECH_CURSOR_BOTTOM:
                    if(view -> chooser -> current < view -> chooser -> ids -> length - 1)
                    {
                        addch(' ');
                        (view -> chooser -> current)++;
                        mvaddch(view -> chooser -> start_r + view -> chooser -> current + 1, 4, '*');
                        move(view -> chooser -> start_r + view -> chooser -> current + 1, 4);
                    }
                break;

                case VIEW_REDRAW_CITY_DIALOG:
                    drawCityView(world, view);
                break;

                case VIEW_REDRAW_HELP:
                    drawHelpView(world, view);
                break;
            }
            le = le -> next;
        }
        listDestroy(list, &destroyMessage);
    }

    // No need in terminating.
    return 1;
}
