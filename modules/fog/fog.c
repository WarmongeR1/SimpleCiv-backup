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

#include "fog.h"
#include "../int_array/int_array.h"
#include <math.h>

FogOfWar * createFog(int r, int c)
{
    FogOfWar * fog = malloc(sizeof(FogOfWar));
    fog -> rows = daCreate();
    fog -> max_r = r;
    fog -> max_c = c;
    for(int i = 0; i < r; i++)
    {
        // iaLengthCreate() creates array with length c and this array is
        // already null'd (each element == 0). So there is no need in it's
        // initialization.
        IntArray * row = iaLengthCreate(c);
        // Reveal all fog in test mode.
        if(TEST_MODE)
        {
            for(int j = 0; j < c; j++)
            {
                iaSetByIndex(row, j, 1);
            }
        }
        // Prepend row.
        daPrepend(fog -> rows, row);
    }
    return fog;
}

void destroyFog(FogOfWar * fog)
{
    for(int i = 0; i < fog -> rows -> length; i++)
    {
        iaDestroy(daGetByIndex(fog -> rows, i));
    }

    daDestroy(fog -> rows, NULL);
    free(fog);
}

static int normalize(int x, int b)
{
    // Check for bounds.
    while(x < 0)
    {
        x += b;
    }

    while(x >= b)
    {
        x -= b;
    }

    return x;
}

void revealFogCell(FogOfWar * fog, int r, int c)
{
    // Check for bounds.
    r = normalize(r, fog -> max_r);
    c = normalize(c, fog -> max_c);

    // Set as visited.
    iaSetByIndex(daGetByIndex(fog -> rows, r), c, 1);
}

void revealFogRadius(FogOfWar * fog, int center_r, int center_c, int rad)
{
    for (int r = center_r - rad; r <= center_r + rad; r++)
    {
        for (int c = center_c - rad; c <= center_c + rad; c++)
        {
            int dr = abs(center_r - r);
            int dc = abs(center_c - c);
            if(round( sqrt (dr * dr + dc * dc) ) <= rad)
            {
                revealFogCell(fog, normalize(r, fog -> max_r), normalize(c, fog -> max_c));
            }
        }
    }
}

int isKnownCell(FogOfWar * fog, int r, int c)
{
    // Check for bounds.
    r = normalize(r, fog -> max_r);
    c = normalize(c, fog -> max_c);

    if(iaGetByIndex(daGetByIndex(fog -> rows, r), c))
    {
        return 1;
    }
    return 0;
}
