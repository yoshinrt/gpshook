/* mtk.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

 Increment-P MapFan II .TRK

History:
 1999/08/12 V0.11
 1999/08/05 V0.04 add in_mtk()
*/
#include "gpslogcv.h"

/* === in MTK out === */
/* no thread,alitude,time */

#ifdef VCPP
#pragma warning( disable: 4100)
#endif

int in_MTK(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
	char *p;

	while((p = xfgets(rFile))!=NULL) {
		if (line == 1) {
			if (atoi(p)!=16) {
				fprintf(stderr,"*Error - Magic Number not found !\n");
				return XErr = -1;
			}
			continue;
		}
		position->lat = (double)atol(p)/(256*3600);
		while(*p)
			if (*p++ == ',')
				break;
		position->lon = (double)atol(p)/(256*3600);	
		return 1;
	}
	return 0;
}

void out_MTK(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
	if (nRec == 0)
		fprintf(out,"16\n");
	fprintf(out,"%8.0f,%9.0f\n",position->lat*256*3600,position->lon*256*3600);
	nRec++;
}
#ifdef VCPP
#pragma warning( default: 4100)
#endif