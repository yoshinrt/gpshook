/* stf.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

 Garmin GPS Simple Text Output Format log .STF

History:
 2001/11/01 V0.33 add out_STF() (requester:suzuki-y@pluto.dti.ne.jp)
 2001/03/20 V0.28d add rcvStat
 1999/10/25 V0.01 add in_STF() (requester:Mio)
*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include <fcntl.h>
#include <io.h>

#include "gpslogcv.h"

/* === in STF === */

#include "stf.h"

static	char *pstatstr = "_S__gdG_D";

int in_STF(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
	char *q;
	stof_t *s;
	int x,y;

	while((s = (stof_t *)xfgets(rFile))!=NULL) {
		if (s->start != '@')
			continue;
		if (strlen((char *)s) < sizeof(stof_t))
			continue;

		if ((q=strchr(pstatstr,s->position_status)) == NULL){
			fprintf(stderr,"*Warning - illegal status format.\n");
			GPSLogp->thread = 1;
			continue;
		} else if ((GPSLogp->rcvStat = (char)((q-pstatstr)/2))<2) {
			GPSLogp->thread = 1;
			continue;
		}

#if 0
		*(q = (char *)s + sizeof(stof_t)) = '\0';
		x = atoi(&s->UD_velocity_magnitude[0]);
		GPSLogp->speed = x*x*x/1000000.0;
		x = atoi(&s->NS_velocity_magnitude[0]);
		GPSLogp->speed += x*x*x/1000.0;
		x = atoi(&s->EW_velocity_magnitude[0]);
		GPSLogp->speed += x*x*x/1000.0;
		GPSLogp->speed = exp(log(GPSLogp->speed)/3)*3.6;	/* km/h */
#else
		y = atoi(&s->NS_velocity_magnitude[0]);
		x = atoi(&s->EW_velocity_magnitude[0]);
		GPSLogp->speed = sqrt((x*x + y*y)/100.0)*3.6;	/* km/h */
#endif
		if (s->EW_velocity_direction == 'W')
			x = -x;
		if (s->NS_velocity_direction == 'S')
			y = -y;
		/* if (GPSLogp->speed > 6) */
		GPSLogp->head = fmod(atan2((double)x,(double)y)/TORAD + 360,360);
		position->alt = atoi(&s->altitude_sign);

		x = atoi(q = &s->longitude_position_m[0]);*q='\0';
		position->lon = atoi(&s->longitude_position_d[0])+(double)x/60000.0;
		if (s->longitude_hemishpere == 'W')
			position->lon = -position->lon;

		s->longitude_hemishpere = '\0';
		x = atoi(q = &s->latitude_position_m[0]);*q='\0';
		position->lat = atoi(&s->latitude_position_d[0])+(double)x/60000.0;
		if (s->latitude_hemisphere == 'S')
			position->lat = -position->lat;

		q = &s->latitude_hemisphere;
		GPSLogp->recTime->tm_sec = atoi(q-=2);*q='\0';
		GPSLogp->recTime->tm_min = atoi(q-=2);*q='\0';
		GPSLogp->recTime->tm_hour = atoi(q-=2);*q='\0';
		GPSLogp->recTime->tm_mday = atoi(q-=2);*q='\0';
		GPSLogp->recTime->tm_mon = atoi(q-=2)-1;*q='\0';
		GPSLogp->recTime->tm_year = atoi(q-=2);
		if (GPSLogp->recTime->tm_year < 70)
			GPSLogp->recTime->tm_year += 100;

		return 1;
	}
	return 0;
}

static char stfbuf[80];
static 	time_t tim0;
static double alt0;

static char *ang2trkDM(double angle, char *p)
{
	int deg,min;
	unsigned long IAng;

	IAng = (unsigned long)((angle+0.5/600000.0) * ONEDEG);
	deg = (int)(IAng/ONEDEG);
	IAng = (IAng%ONEDEG)*60;
	min = (int)(IAng/ONEDEG);
	IAng = (IAng%ONEDEG)/(ONEDEG/1000UL);
	sprintf(p,"%03d%02d%03ld",deg,min,IAng);
	return p;
}


void out_STF(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
	time_t tim;
	double xSpd = cos(GPSLogp->head*TORAD)*GPSLogp->speed*1000/3600*10;
	double ySpd = sin(GPSLogp->head*TORAD)*GPSLogp->speed*1000/3600*10;
	double zSpd;

	char latbuf[12];
	char lonbuf[12];
	
    if (nRec == 0) {
        if (_setmode( _fileno(out), _O_BINARY )== -1 ) {
            perror( "can't set binary mode." );
            XErr++;
			return;
        }
	}
	tim = mktime(GPSLogp->recTime);
	if (GPSLogp->thread == 0)
		zSpd = (position->alt - alt0)/(tim - tim0) *100;
	else
		zSpd = 0;

	tim0 = tim;
	alt0 = position->alt;
	/*@010505082108N3502224E13856167G009+00035E0077N0009U0000*/

	if (GPSLogp->rcvStat==0) {
//		sprintf(stfbuf+13,"__________________________________________\012\015");
		if (position->alt <= -9999)
			GPSLogp->rcvStat = 2;
		else
			GPSLogp->rcvStat = 3;
	}
	sprintf(stfbuf,"@%02d%02d%02d%02d%02d%02d%c%s%c%s%c%03d%c%05d%c%04d%c%04d%c%04d\012\015",
		GPSLogp->recTime->tm_year%100,
		GPSLogp->recTime->tm_mon+1,
		GPSLogp->recTime->tm_mday,
		GPSLogp->recTime->tm_hour,
		GPSLogp->recTime->tm_min,
		GPSLogp->recTime->tm_sec
		,(position->lat > 0) ? 'N':'S'
		,ang2trkDM(fabs(position->lat),latbuf)+1
		,(position->lon > 0) ? 'E':'W'
		,ang2trkDM(fabs(position->lon),lonbuf)
		,*(pstatstr + GPSLogp->rcvStat*2)
		,1		
		,(position->alt<0) ? '-':'+'
		,(int)(fabs(position->alt)+.5)
		,(xSpd>0) ? 'E':'W'
		,(int)(fabs(xSpd)+.5)
		,(ySpd>0) ? 'N':'S'
		,(int)(fabs(ySpd)+.5)
		,(zSpd>0) ? 'U':'D'
		,(int)(fabs(zSpd)+.5)
	);
	GPSLogp->thread = 0;
	fputs(stfbuf,out);
	nRec++;
}
