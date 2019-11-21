/*******************************************************************************
 * include(s):
 ******************************************************************************/

#include <time.h>

/********************************************************************************
 * pTime
 *******************************************************************************/
char *pTime(char *inStr)
{
    /* declare some variables and initialize them */
    struct tm *tim;
    time_t t;

    t = time((time_t *) & t);
    tim = localtime((time_t *) & t);

    if (inStr == (char *) NULL) {
	return inStr;
    }

    sprintf(inStr, "%4d%02d%02dT%02d%02d%02d-", tim->tm_year + 1900,
	    tim->tm_mon + 1, tim->tm_mday, tim->tm_hour, tim->tm_min,
	    tim->tm_sec);

    return inStr;
}
