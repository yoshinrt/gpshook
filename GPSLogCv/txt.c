/* txt.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

 Waypoint+ .TXT

History:
2000/12/13 V0.26c df/10000.0
1999/08/12 V0.11
*/
#include <string.h>
#include <math.h>

#include "gpslogcv.h"

/* === in WAYPOINT+ comma delimited TEXT out === */
/*
Datum,WGS84,WGS84,0,0,0,0,0
Datum,Tokyo mean,Bessel 1841,739.845,0.10037483,-128,481,664

012345678901234567890123456789012345678901234567890123456789
TP,D, 34.757604003, 137.7385836840,07/05/1997,09:33:04,1
TP,DM, 34.454562402, 137.4431502104,07/05/1997,09:33:04,1
TP,DMS, 36.014155393, 138.5920911510,07/04/1997,17:41:21,0

Datum,WGS84,WGS84,0,0,0,0,0
Datum,Tokyo mean,Bessel 1841,739.845,0.10037483,-128,481,664
TP,D, 35.150913833, 135.8208856667,04/10/1999,03:46:19,1
TP,DM, 35.090592140, 135.4927339000,04/10/1999,03:46:19,1
TP,DMS, 35.090328966, 135.4915188330,04/10/1999,03:46:19,1
*/

static double angStrTxt2Ang(char *p,int typ)
{
	int sgn = 1;
	double d,x;

	if ((d = atof(p)) < 0) {
		sgn = -1;
		d = -d;
	}
	/* convert to DEG */
	switch(typ){
	case DAT_DEG:
		break;
	case DAT_DMM:/*DM*/
		d = floor(d)+(d-floor(d))*5.0/3.0;
		break;
	case DAT_DMS:/*DMS*/
		x = (d-floor(d))*100.0;
		d = floor(d) + floor(x)/60.0 + (x-floor(x))/36.0;
		break;
	default:
		XErr++;
		fprintf(stderr,"*Error - illegal angle format !\n");
		break;
	}
	return d*sgn;
}

int in_TXT(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
/* global val :lbuf,datum,mParam */
/* call lib: fgets,_strnicmp,atoi */
/* call strtokX */
/* return value -1:Error, 0:EOF, 1:OK */
{
	char *p;

	while((p=xfgets(rFile))!=NULL) {
		switch(*p) {
		case 'D':	 /* check Datum */
			if (_strnicmp(strtokX(p,","),"Datum",5) == 0) {
				p = strtokX(NULL,",");
				if (_strnicmp(p,"Tokyo",5)==0 ) {
					GPSLogp->datTyp |= DAT_TKY;
				} else if (_strnicmp(p,"WGS84",5)==0 ) {
					GPSLogp->datTyp |= DAT_WGS;
				} else {
					GPSLogp->datTyp |= DAT_OTH;
					strncpy(datumbuf,p,31);
				}
				(void)strtokX(NULL,",");	/* skip */
				mParam.da = atof(strtokX(NULL,","));
				mParam.df = atof(strtokX(NULL,","))/10000.0;
				mParam.dx = atof(strtokX(NULL,","));
				mParam.dy = atof(strtokX(NULL,","));
				mParam.dz = atof(strtokX(NULL,","));
				GPSLogp->mpp = &mParam;
			}
			break;
		case 'T':
			if (_strnicmp(strtokX(p,","),"TP",2) == 0) {
				p = strtokX(NULL,",");
				if (_stricmp(p,"D")==0 ) {
					GPSLogp->datTyp |= DAT_DEG;
				} else if (_stricmp(p,"DM")==0 ) {
					GPSLogp->datTyp |= DAT_DMM;
				} else if (_stricmp(p,"DMS")==0 ) {
					GPSLogp->datTyp |= DAT_DMS;
				} else {
					fprintf(stderr,"*Error - illegal modes('D' or 'DM' or 'DMS' expected)!\n");
					return (XErr = -1);
				}
				position->lat = angStrTxt2Ang(strtokX(NULL,","),GPSLogp->datTyp & DAT_ANGFMT_MSK);
				position->lon = angStrTxt2Ang(strtokX(NULL,","),GPSLogp->datTyp & DAT_ANGFMT_MSK);
				GPSLogp->recTime->tm_mon = atoi(strtokX(NULL,",/"))-1;
				GPSLogp->recTime->tm_mday = atoi(strtokX(NULL,"/"));
				GPSLogp->recTime->tm_year = atoi(strtokX(NULL,"/,"))-1900;
				GPSLogp->recTime->tm_hour = atoi(strtokX(NULL,":"));
				GPSLogp->recTime->tm_min = atoi(strtokX(NULL,":"));
				GPSLogp->recTime->tm_sec = atoi(strtokX(NULL,":,"));
				GPSLogp->thread = (GPSLogp->thread)?1:atoi(strtokX(NULL,",\n"));
				return 1;
			}
			break;
		default:
			break;
		}	/* end switch */
	}	/* end while */
	return 0;	/* return EOF */
}

static char *typstrway[]= {
	"D","DM","DMS"
};

/* -180.0 to +180.0 */
static char *ang2txtDM(double angle)
{
	int sgn,deg;
	unsigned long IAng,scale = 10000000L;

	if (angle<0) {
		sgn = -1;
		angle = -angle;
	} else {
		sgn = 1;
	}
	IAng = (unsigned long) ((angle+0.5/600000000.0) * scale);	/* round */
	deg = (int)(IAng/scale);
	IAng = (IAng%scale)*60;
	sprintf(angbuf,"%4d.%02d%07ld",deg*sgn,(int)(IAng/scale),IAng%scale);
	return angbuf;
}

static char *ang2txtDMS(double angle)
{
	int sgn,deg,min,sec;
	unsigned long IAng;

	if (angle<0) {
		sgn = -1;
		angle = -angle;
	} else {
		sgn = 1;
	}
	IAng = (unsigned long) ((angle+0.5/360000000.0) * ONEDEG);	/* round */
	deg = (int)(IAng/ONEDEG);
	IAng = (IAng%ONEDEG)*60;
	min = (int)(IAng/ONEDEG);
	IAng = (IAng%ONEDEG)*60;
	sec = (int)(IAng/ONEDEG);
	IAng = (IAng%ONEDEG)/(ONEDEG/100000UL);
	sprintf(angbuf,"%4d.%02d%02d%05ld",deg*sgn,min,sec,IAng);
	return angbuf;
}

void out_TXT(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
	if (nRec == 0) {
		fprintf(out,(GPSLogp->outTyp & DAT_TKY) ?
			"Datum,Tokyo mean,Bessel 1841,739.845,0.10037483,-128,481,664\n":
			"Datum,WGS84,WGS84,0,0,0,0,0\n");
	}
	fprintf(out,"TP,%s",typstrway[(GPSLogp->outTyp & DAT_ANGFMT_MSK)-1]);
	switch(GPSLogp->outTyp & DAT_ANGFMT_MSK) {
	case DAT_DEG:
		fprintf(out,",%13.9f,%15.10f",position->lat,position->lon);
		break;
	case DAT_DMM:
		fprintf(out,",%s",ang2txtDM(position->lat)+1);
		fprintf(out,",%s0",ang2txtDM(position->lon));
		break;
	default:
	case DAT_DMS:
		fprintf(out,",%s",ang2txtDMS(position->lat)+1);
		fprintf(out,",%s0",ang2txtDMS(position->lon));
		break;
	}
	fprintf(out,",%02d/%02d/%04d,%02d:%02d:%02d,%d\n"
		,GPSLogp->recTime->tm_mon+1,GPSLogp->recTime->tm_mday,GPSLogp->recTime->tm_year+1900
		,GPSLogp->recTime->tm_hour,GPSLogp->recTime->tm_min,GPSLogp->recTime->tm_sec,(GPSLogp->thread)?1:0);
	GPSLogp->thread = 0;
	nRec++;
}
