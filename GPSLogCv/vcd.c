/* vcd.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

 Kashmir VCD

History:
 2000/05/09 V0.23
*/
#include "gpslogcv.h"
#include <ctype.h>
#include <string.h>

/* === in MTK out === */
/* no thread,alitude,time */

int in_VCD(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
    char *p;
    unsigned char *q;

    while((p = xfgets(rFile))!=NULL) {
        if (!isdigit(*p) && (*p != '-')) {
            GPSLogp->thread = 1;
            for(q = (unsigned char *)p; *q++ >= ' '; );
			*--q = '\0';
            if (*p == '[')
                strcpy(titleBuf,p);
            else if (*p == ';')
                strcpy(mapTtlBuf,p);
            continue;
        } 
        position->lon = atof(p);
        for (;*p;++p)
            if (isspace(*p))
                break;
        position->lat = atof(p);    
        return 1;
    }
    return 0;
}

void out_VCD(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
    if (GPSLogp->thread) {
        fprintf(out,"\n");
        if (mapTtlBuf[0] != '\0') {
            fprintf(out,"%s\n",mapTtlBuf);
            mapTtlBuf[0] = '\0';
        }
        if (titleBuf[0] != '\0') {
            fprintf(out,"%s\n",titleBuf);
            titleBuf[0] = '\0';
        }
        GPSLogp->thread = 0;
    }
    fprintf(out,"%.7f %.7f\n",position->lon,position->lat);
    nRec++;
}
