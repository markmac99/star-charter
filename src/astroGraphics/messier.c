// messier.c
// 
// -------------------------------------------------
// Copyright 2015-2020 Dominic Ford
//
// This file is part of StarCharter.
//
// StarCharter is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// StarCharter is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with StarCharter.  If not, see <http://www.gnu.org/licenses/>.
// -------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <gsl/gsl_math.h>

#include "astroGraphics/messier.h"
#include "coreUtils/asciiDouble.h"
#include "coreUtils/errorReport.h"
#include "mathsTools/projection.h"
#include "settings/chart_config.h"
#include "vectorGraphics/lineDraw.h"
#include "vectorGraphics/cairo_page.h"

//! plot_messier_objects - Draw Messier objects onto a star chart
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param page - A <cairo_page> structure defining the cairo drawing context.

void plot_messier_objects(chart_config *s, cairo_page *page) {
    FILE *file;
    char line[FNAME_LENGTH];

    // Open data file listing the positions of the Messier objects
    file = fopen(SRCDIR "../data/deepSky/messier/messier.dat", "r");
    if (file == NULL) stch_fatal(__FILE__, __LINE__, "Could not open Messier catalogue");

    // Loop over the lines of the data file
    while ((!feof(file)) && (!ferror(file))) {
        int messier_num, ra_hrs, dec_deg, dec_south;
        double ra_min, dec_min;
        double ra, dec, mag, x, y;

        file_readline(file, line);

        // Ignore comment lines
        if (line[0] != '[') continue;

        messier_num = get_digit(line[4]) * 100 + get_digit(line[5]) * 10 + get_digit(line[6]);
        mag = get_float(line + 18, NULL);
        ra_hrs = get_digit(line[32]) * 10 + get_digit(line[33]);
        ra_min = get_digit(line[34]) * 10 + get_digit(line[35]) + 0.1 * get_digit(line[37]);
        dec_deg = get_digit(line[40]) * 10 + get_digit(line[41]);
        dec_min = get_digit(line[42]) * 10 + get_digit(line[43]) + 0.1 * get_digit(line[45]);
        dec_south = (line[39] == '-');

        ra = (ra_hrs + ra_min / 60.) / 12. * M_PI;
        dec = (dec_deg + dec_min / 60.) / 180. * M_PI;
        if (dec_south) dec *= -1;

        // Project RA and Dec of object into physical coordinates on the star chart
        plane_project(&x, &y, s, ra, dec, 0);

        // Reject this object if it falls outside the plot area
        if ((x < s->x_min) || (x > s->x_max) || (y < s->y_min) || (y > s->y_max)) continue;

        // Draw a cross showing the position of this object
        double x_canvas, y_canvas;
        double pt = 1. / 72; // 1 pt
        double point_size = 0.7 * 0.75 * 3 * pt;
        fetch_canvas_coordinates(&x_canvas, &y_canvas, x, y, s);
        cairo_set_source_rgb(s->cairo_draw, s->messier_col.red, s->messier_col.grn, s->messier_col.blu);
        cairo_set_line_width(s->cairo_draw, 1.2);
        cairo_new_path(s->cairo_draw);
        cairo_move_to(s->cairo_draw, x_canvas - s->dpi * point_size, y_canvas - s->dpi * point_size);
        cairo_line_to(s->cairo_draw, x_canvas + s->dpi * point_size, y_canvas + s->dpi * point_size);
        cairo_move_to(s->cairo_draw, x_canvas - s->dpi * point_size, y_canvas + s->dpi * point_size);
        cairo_line_to(s->cairo_draw, x_canvas + s->dpi * point_size, y_canvas - s->dpi * point_size);
        cairo_stroke(s->cairo_draw);

        // Write a text label for this object
        const int multiple_labels = s->messier_names && s->messier_mag_labels;
        const double horizontal_offset = 1.1 * s->mm;

        if (s->messier_names) {
            snprintf(temp_err_string, FNAME_LENGTH, "M%d", messier_num);
            chart_label_buffer(page, s, s->messier_col, temp_err_string,
                               (label_position[2]){{x, y, horizontal_offset, -1, 0},
                                                   {x, y, -horizontal_offset, 1, 0}}, 2,
                               multiple_labels, 0, 1.4, 0, 0, mag);
        }
        if (s->messier_mag_labels) {
            snprintf(temp_err_string, FNAME_LENGTH, "mag %.1f", mag);
            chart_label_buffer(page, s, s->messier_col, temp_err_string,
                               (label_position[2]){{x, y, horizontal_offset, -1, 0},
                                                   {x, y, -horizontal_offset, 1, 0}}, 2,
                               multiple_labels, 0, 1.4, 0, 0, mag);
        }
    }

    // Close data file listing Messier objects
    fclose(file);
}
