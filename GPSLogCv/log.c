/* log.c - part of gpslogcv.c
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

 AlpsMap Pro Atlas 2000 .LOG

History:
1999/08/12 V0.11 support pro atlas 2000 Header
                 "#3 format [ Time:UTC  Location:WGS84 ]" 
1999/08/05 V0.03 add in_LOG() (requester:MasaakiSATO)
*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "gpslogcv.h"

/* === in LOG out === */
/* no thread,alitude */

static char *xstrchr(char *p,char dlm)
{
	char *q;

	if (XErr)
		return p;
	if ((q = strchr(p,dlm))==NULL) {
		XErr++;
		return p;
	}
	return q+1;
}

int in_LOG(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
	char *p,*q,c;

	while((p = xfgets(rFile))!=NULL) {
		if (*p++ == '#') {
			if (*p++ != '3')
				continue;
			if ((q = strstr(p,"Time:"))!=NULL)
				if (strncmp(q+5,"UTC",3)==0)
					GPSLogp->datTyp &= ~DAT_JST;
			if ((q = strstr(q,"Location:"))!=NULL)
				if (strncmp(q+9,"WGS84",5)==0)
					GPSLogp->datTyp &= ~DAT_TKY;
			continue;
		}
		GPSLogp->recTime->tm_year = atoi(p); p = xstrchr(p,'/');
		if (GPSLogp->recTime->tm_year < 70)
			GPSLogp->recTime->tm_year += 100;

		GPSLogp->recTime->tm_mon = atoi(p)-1; p = xstrchr(p,'/');
		GPSLogp->recTime->tm_mday =	atoi(p); p = xstrchr(p,' ');
		GPSLogp->recTime->tm_hour = atoi(p); p = xstrchr(p,':');
		GPSLogp->recTime->tm_min = atoi(p); p = xstrchr(p,':');
		GPSLogp->recTime->tm_sec = atoi(p); p = xstrchr(p,' ');
		c = *p++;
		position->lat = atoi(p); p = xstrchr(p,':');
		position->lat += atof(p)/60; p = xstrchr(p,':');
		position->lat += atof(p)/3600; p = xstrchr(p,' ');
		if (c != 'N')
			position->lat = -position->lat;
		c = *p++;
		position->lon = atoi(p); p = xstrchr(p,':');
		position->lon += atof(p)/60; p = xstrchr(p,':');
		position->lon += atof(p)/3600;
		if (c != 'E')
			position->lon = -position->lon;
		return 1;
	}
	return 0;
}

static char *ang2DMSLog(double angle)
{
	int deg,min,sec;
	unsigned long IAng;

	IAng = (unsigned long)((angle+0.5/36000.0) * ONEDEG);
	deg = (int)(IAng/ONEDEG);
	IAng = (IAng%ONEDEG)*60;
	min = (int)(IAng/ONEDEG);
	IAng = (IAng%ONEDEG)*60;
	sec = (int)(IAng/ONEDEG);
	IAng = (IAng%ONEDEG)/(ONEDEG/10UL);
	sprintf(angbuf,"%03d:%02d:%02d.%1d",deg,min,sec,(int)IAng);
	return angbuf;
}

void out_LOG(FILE *out, cordinate_t *posi, cnvData_t *GPSLogp)
{
	char latRef = 'N';
	char lonRef = 'E';
	double lat = posi->lat;
	double lon = posi->lon;

	fprintf(out,"%02d/%02d/%02d %02d:%02d:%02d"
	  ,GPSLogp->recTime->tm_year%100,GPSLogp->recTime->tm_mon+1,GPSLogp->recTime->tm_mday
	  ,GPSLogp->recTime->tm_hour,GPSLogp->recTime->tm_min,GPSLogp->recTime->tm_sec);

	if (lat < 0) {
		latRef = 'S';
		lat = -lat;
	}
	fprintf(out," %C%s",latRef,ang2DMSLog(lat)+1);

	if (lon < 0) {
		lonRef = 'W';
		lon = -lon;
	}
	fprintf(out," %C%s\n",lonRef,ang2DMSLog(lon));
	nRec++;
}
