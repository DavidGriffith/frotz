/*
 * ux_pic.c
 *
 * Unix interface, stubs for picture functions
 *
 */

/*
 * os_draw_picture
 *
 * Display a picture at the given coordinates. Top left is (1,1).
 *
 */

void os_draw_picture (int picture, int y, int x)
{

    /* Not Implemented */

}/* os_draw_picture */

/*
 * os_peek_colour
 *
 * Return the colour of the pixel below the cursor. This is used
 * by V6 games to print text on top of pictures. The coulor need
 * not be in the standard set of Z-machine colours. To handle
 * this situation, Frotz extends the colour scheme: Values above
 * 15 (and below 256) may be used by the interface to refer to
 * non-standard colours. Of course, os_set_colour must be able to
 * deal with these colours. Interfaces which refer to characters
 * instead of pixels might return the current background colour
 * instead.
 *
 */

int os_peek_colour (void)
{

    /* Not Implemented */
    return 0;

}/* os_peek_colour */

/*
 * os_picture_data
 *
 * Return true if the given picture is available. If so, write the
 * width and height of the picture into the appropriate variables.
 * Only when picture 0 is asked for, write the number of available
 * pictures and the release number instead.
 *
 */

int os_picture_data (int picture, int *height, int *width)
{

    /* Not implemented */
    return 0;

}/* os_picture_data */
