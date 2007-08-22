/* lst.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

 GPSLogCv statistical list .LST

History:
1999/09/01 V0.16 initial release
1999/09/01 V0.15 initial release
*/
#include <float.h>

#include "gpslogcv.h"

/* === LST out === */

typedef struct logStat {
	long   nPoint;
	struct tm tStart;
	struct tm tEnd;
	cordinate_t startPos;
	cordinate_t endPos;
	cordinate_t minLatPos;
	cordinate_t maxLatPos;
	cordinate_t minLonPos;
	cordinate_t maxLonPos;
	cordinate_t minAltPos;
	cordinate_t maxAltPos;
	double maxSpeed;
	cordinate_t maxSpeedPos;
} logStat_t;

void iniStat(logStat_t *stat)
 {
	stat->nPoint = 0;
	stat->minLatPos.lat =  DBL_MAX;
	stat->maxLatPos.lat = -DBL_MAX;
	stat->minLonPos.lon =  DBL_MAX;
	stat->maxLonPos.lon = -DBL_MAX;
	stat->minAltPos.alt =  DBL_MAX;
	stat->maxAltPos.alt = -DBL_MAX;
	stat->maxSpeed = -DBL_MAX;
}

static word nThread;
static logStat_t totalStat,threadStat;

void prt_result(logStat_t *stat, FILE *out)
{
	fprintf(out,"Number of points:%ld\n",stat->nPoint);
	fprintf(out,"Minimum Lat Pos:%f %f %f\n",stat->minLatPos.lat, stat->minLatPos.lon, stat->minLatPos.alt);
	fprintf(out,"Maximum Lat Pos:%f %f %f\n",stat->maxLatPos.lat, stat->maxLatPos.lon, stat->maxLatPos.alt);
	fprintf(out,"Minimum Lon Pos:%f %f %f\n",stat->minLonPos.lat, stat->minLonPos.lon, stat->minLonPos.alt);
	fprintf(out,"Maximum Lon Pos:%f %f %f\n",stat->maxLonPos.lat, stat->maxLonPos.lon, stat->maxLonPos.alt);
	fprintf(out,"Minimum Alt Pos:%f %f %f\n",stat->minAltPos.lat, stat->minAltPos.lon, stat->minAltPos.alt);
	fprintf(out,"Maximum Alt Pos:%f %f %f\n",stat->maxAltPos.lat, stat->maxAltPos.lon, stat->maxAltPos.alt);
	if (stat->nPoint > 1)
		fprintf(out,"Maximum Speed %f km/h at Pos:%f %f %f\n",stat->maxSpeed*3.6, stat->maxSpeedPos.lat, stat->maxSpeedPos.lon, stat->maxSpeedPos.alt);
}


void cumStat(void)
{
	totalStat.nPoint += threadStat.nPoint;
	if (threadStat.minLatPos.lat < totalStat.minLatPos.lat)
		totalStat.minLatPos = threadStat.minLatPos;
	if (threadStat.maxLatPos.lat > totalStat.maxLatPos.lat)
		totalStat.maxLatPos = threadStat.maxLatPos;
	if (threadStat.minLonPos.lon < totalStat.minLonPos.lon)
		totalStat.minLonPos = threadStat.minLonPos;
	if (threadStat.maxLonPos.lon > totalStat.maxLonPos.lon)
		totalStat.maxLonPos = threadStat.maxLonPos;
	if (threadStat.minAltPos.alt < totalStat.minAltPos.alt)
		totalStat.minAltPos = threadStat.minAltPos;
	if (threadStat.maxAltPos.alt > totalStat.maxAltPos.alt)
		totalStat.maxAltPos = threadStat.maxAltPos;
	if (threadStat.maxSpeed > totalStat.maxSpeed) {
		totalStat.maxSpeed = threadStat.maxSpeed;
		totalStat.maxSpeedPos = threadStat.maxSpeedPos;
	}
}

void out_LST(FILE *out, cordinate_t *pos, cnvData_t *GPSLogp)
{
    if (nRec == 0) {
		nThread = 0;
		iniStat(&totalStat);
		totalStat.tStart = *(GPSLogp->recTime);
		totalStat.startPos = *pos;
		threadStat.nPoint = 0;
	}
	if (GPSLogp->thread != 0) {
		if (threadStat.nPoint != 0) {
			nThread++;
		    /* cumulative */
			prt_result(&threadStat,out);
			cumStat();
		}
		iniStat(&threadStat);
		threadStat.tStart = *(GPSLogp->recTime);
	}
	threadStat.nPoint++;
	if (pos->lat < threadStat.minLatPos.lat)
		threadStat.minLatPos = *pos;
	if (pos->lat > threadStat.maxLatPos.lat)
		threadStat.maxLatPos = *pos;
	if (pos->lon < threadStat.minLonPos.lon)
		threadStat.minLonPos = *pos;
	if (pos->lon > threadStat.maxLonPos.lon)
		threadStat.maxLonPos = *pos;
	if (pos->alt < threadStat.minAltPos.alt)
		threadStat.minAltPos = *pos;
	if (pos->alt > threadStat.maxAltPos.alt)
		threadStat.maxAltPos = *pos;
	if ((GPSLogp->thread == 0)&&(GPSLogp->speed > threadStat.maxSpeed)) {
		threadStat.maxSpeed = GPSLogp->speed;
		threadStat.maxSpeedPos = *pos;
	}
	threadStat.tEnd = *(GPSLogp->recTime);

	GPSLogp->thread = 0;
	nRec++;
}

int cls_LST(FILE *out)
{
	if (threadStat.nPoint != 0) {
		nThread++;
		/* cumulative */
		prt_result(&threadStat,out);
		cumStat();
	}
	fprintf(out,"-- Total:%d threads --\n",nThread);
	prt_result(&totalStat,out);

	if (out != stdout)
		return fclose(out);
	return 0;
}
