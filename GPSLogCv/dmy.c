/* dmy.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

 Dummy Data Generator for DEBUG

History:
*/
#include <string.h>
#include "gpslogcv.h"

#ifdef VCPP
#pragma warning( disable: 4100)
#endif

static double lat0,lat1,lon0,lon1,latdif,londif;
static int latstep,lonstep,nLat,nLon,dir;

double dmstof(char *p)
{
	double a;
	int d,m,s;

	a = atof(p);
	d = (int)a;
	a -= d;
	a *= 100;
	m = (int)a;
	a -= m;
	a *= 1000;
	s = (int)a;
	return d+m/60.0+s/36000.0;
}

int in_DMY(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
	char *p;

	for(;;) {
		if (nRec == 0) {
			nLat = nLon = latstep = lonstep = 0;
		}
		if (++nLat > latstep) {
			if (++nLon > lonstep) {
				if ((p=xfgets(rFile))==NULL)
					break;
				if (*p == ';') {
					char *q = titleBuf;
					while(*p && (*p!='\n'))
						*q++ = *++p;
					*--q = '\0';
					continue;
				}
				lat0 = dmstof(strtokX(p,","));		//35;
				lat1 = dmstof(strtokX(NULL,","));		//81;
				lon0 = dmstof(strtokX(NULL,","));		//-17;
				lon1 = dmstof(strtokX(NULL,","));		//71;
				latdif = dmstof(strtokX(NULL,","));	//
				londif = dmstof(strtokX(NULL,","));	//2; //20.0/3600.0;
				if ((p = strtokX(NULL,","))!=NULL)
					simSpeed = atof(p);	//30000.0;
				else
					XErr = 0;
				if (latdif != 0)
					latstep = (int)((lat1-lat0)/latdif);
				else
					latstep = 0;
				if (londif != 0)
					lonstep = (int)((lon1-lon0)/londif);
				else
					lonstep = 0;
				dir = 0;
				nLat = nLon = 0;
				GPSLogp->thread = 1;
			} else {
				dir = ~dir;
				nLat = 0;
			}
		}
		
		if (dir == 0)
			position->lat = lat0+nLat*latdif;
		else
			position->lat = lat1-nLat*latdif;
		
		position->lon = lon0+londif*nLon;
		return 1;
	}
	return 0;
}
#ifdef VCPP
#pragma warning( default: 4100)
#endif