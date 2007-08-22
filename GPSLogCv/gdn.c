/* gdn.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

 Gardwon track .GDN

History:
2000/11/15 V0.26 initial release
*/
#include <math.h>
#include <string.h>

#include "gpslogcv.h"

/* === in TRK out === */
static double angStr2Ang(char *str)
{
	double angle = 0.0;

	if (XErr)
		return angle;

	angle =	atof(str+1);
	angle += atof(strtokX(NULL," "))/60.0;
	switch(*str) {
	case 'S':
	case 'W':
		angle = -angle;
		break;
	case '+':
	case 'N':
	case 'E':
		break;
	default:
		XErr++;
		angle = 0.0;
		break;
	}
	if (XErr) {
		fprintf(stderr,"*Error - illegal Cordinate Format(Line:%lu)!\n",line);
		angle = 0;
	}
	return angle;
}

int in_GDN(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
/* global val :datum,mParam */
/* call lib: fgets,_strnicmp,atof */
/* call strtokX,mname2m */
/* return value -1:Error, 0:EOF, 1:OK */
{
	char *p;

	while((p=xfgets(rFile))!=NULL) {
		switch(*p) {
		case 'D':	 /* check Datum */
			if (_strnicmp(p+6,"Tokyo",5)==0 ) {
				GPSLogp->datTyp |= DAT_TKY;
			} else if (_strnicmp(p+5,"WGS84",5)==0 ) {
				GPSLogp->datTyp |= DAT_WGS;
			} else if (_strnicmp(p+5,"WGS 84",6)==0 ) {
				GPSLogp->datTyp |= DAT_WGS;
			} else {
				GPSLogp->datTyp |= DAT_OTH;
			}
			/* no break */
		case 'N':	 /* New */
			GPSLogp->thread = 1;
			break;
		case 'T':
			position->lat = angStr2Ang(strtokX(p+3," "));
			position->lon = angStr2Ang(strtokX(NULL," "));
			strtokX(NULL," ");	/* skip day of week */
			GPSLogp->recTime->tm_mon = mname2m(strtokX(NULL," "));
			GPSLogp->recTime->tm_mday = atoi(strtokX(NULL," "));
			GPSLogp->recTime->tm_hour = atoi(strtokX(NULL,":"));
			GPSLogp->recTime->tm_min = atoi(strtokX(NULL,":"));
			GPSLogp->recTime->tm_sec = atoi(strtokX(NULL," "));
			GPSLogp->recTime->tm_year = atoi(strtokX(NULL," "))-1900;
			return 1;
			break;	/* never executed */
		default:
			GPSLogp->thread = 1;
			break;
		}	/* end switch */
	}	/* end while */
	return 0;	/* return EOF */
}

/* 0.0 - 180.0 */
static char *ang2trkDM(double angle)
{
	int deg,min;
	unsigned long IAng;

	IAng = (unsigned long)((angle+0.5/6000000.0) * ONEDEG);
	deg = (int)(IAng/ONEDEG);
	IAng = (IAng%ONEDEG)*60;
	min = (int)(IAng/ONEDEG);
	IAng = (IAng%ONEDEG)/(ONEDEG/10000UL);
	sprintf(angbuf,"%03d %02d.%04ld",deg,min,IAng);
	return angbuf;
}

static const char toMname[]="Jan\0Feb\0Mar\0Apr\0May\0Jun\0Jul\0Aug\0Sep\0Oct\0Nov\0Dec";
static const char toDname[]="Sun\0Mon\0Tue\0Wed\0Thu\0Fri\0Sat";

void out_GDN(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
	char latRef = 'N';
	char lonRef = 'E';
	double lat = position->lat;
	double lon = position->lon;

	if (nRec == 0) {
		fprintf(out,"\nD  95\t%s\n",(GPSLogp->outTyp & DAT_TKY) ?"Tokyo":"WGS 84");
		fprintf(out,"\nF  0\tLat Long\n");
	}
	if (GPSLogp->thread) {
		fprintf(out,"N New Track Start\n");
		GPSLogp->thread = 0;
	}
	if (lat < 0) {
		latRef = 'S';
		lat = -lat;
	}
	if (lon < 0) {
		lonRef = 'W';
		lon = -lon;
	}
	fprintf(out,"T  %C%s ",latRef,ang2trkDM(lat)+1);
	fprintf(out,"%C%s",lonRef,ang2trkDM(lon));
	(void) mktime(GPSLogp->recTime);
	fprintf(out," %s %s %02d %02d:%02d:%02d %d\n"
	  ,&toDname[GPSLogp->recTime->tm_wday*4],&toMname[GPSLogp->recTime->tm_mon*4],GPSLogp->recTime->tm_mday
	  ,GPSLogp->recTime->tm_hour,GPSLogp->recTime->tm_min,GPSLogp->recTime->tm_sec,GPSLogp->recTime->tm_year+1900);
	nRec++;
}

