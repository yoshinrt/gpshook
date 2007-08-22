/* nme.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

 GPS NMEA format log .NME

History:
 2002/06/26 V0.34i fix GGA altitude bug.
 2002/06/18 V0.34h $PNTS support.
 2001/03/18 V0.28c Satellite information Bug Fix
 2001/03/15 V0.28b Satellite information/Receiver Status 
 2000/11/16 V0.26a add mag. decl.
 2000/11/06 V0.26 add mag. decl.
 2000/11/06 V0.25 add out_nme()
 1999/09/05 V0.16 add ignore bad/no checksum for some GPS.
*/
#include <ctype.h>
#include <string.h>

#include "gpslogcv.h"
#ifdef DOS
#undef MAGFIELD
#endif
#ifdef MAGFIELD
#include "magfield/magfield.h"
#endif
#define RMC_TIM 1
#define RMC_STA 2
#define RMC_LAT 3
#define RMC_LAS 4
#define RMC_LON 5
#define RMC_LOS 6
#define RMC_SPD 7
#define RMC_HED 8
#define RMC_DAT 9
#define RMC_MAG 10

/* add by Nori-chan */
#define PNT_VER 1
#define PNT_TYP 2
#define PNT_DAY 3
#define PNT_MON 4
#define PNT_YER 5
#define PNT_TIM 6
#define PNT_LAT 7
#define PNT_LAS 8
#define PNT_LON 9
#define PNT_LOS 10
#define PNT_HED 11
#define PNT_SPD 12
#define PNT_MRK 13
#define PNT_MES 14
#define PNT_GRP 15
#define PNT_STA 16

#define GGA_TIM 1
#define GGA_LAT 2
#define GGA_LAS 3
#define GGA_LON 4
#define GGA_LOS 5
#define GGA_STA 6

#define GGA_AUNT  8
#define GGA_ALT	  9
#define GGA_GUNT 10
#define GGA_GOID 11

#define GSA_MOD	  2

#define notToKm 1.852

/* === in NMEA === */
/*

  $GPRMC,110244,A,3537.0033,N,13902.5826,E,54.330,221.6,010298,7.0,W*57
*/

#define MAXPARAM 20
static unsigned char pcnt;
static char *param[MAXPARAM+1];	   /* parameter pointer for NMEA sentence */

static void angStrNME2Ang(char *p,double *dbl)
{
	char *q;
	int a;
	double d;
	
	if ((q = strchr(p,'.'))==NULL)
		return;
	d = atof(q-=2)/60.0;
	if (!isdigit(*p) && (p<q)) p++;
	for (a=0;p<q;)
		a = a*10 + *p++ - '0';
	d += a;
	*dbl = d;
}

/* separate parameter from sentence */
static unsigned char sep(char *buf)
{
	register char i,*p;
	
	for(i = -1,p=buf; i <MAXPARAM ;) {
		while(*p == ' ') p++;	/* skip leading blank */
		param[++i] = p;
		if ((p = strchr(p,','))!=NULL) {
			*p++ = '\0';			/* set EOS*/
		} else {
		/*			if (i<0)
			return 0; never */
			if ((p = strchr(param[i],'*'))!=NULL)
				*p = '\0';
			return i;
		}
	}
	return i;
}

static int numGSV;
static char usedSat[13];
static Satdat_t nmeaSat;
static Satdat_t nmeaSatTmp;

int in_NME(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
	char *p;
	unsigned char *q,csum;
	int is_data = 0;
	
	if (nRec == 0)
		pcnt = 0;
	
	if (pcnt == 0xff)
		return 0;

	for (;;) {
		if (pcnt == 0) {
			if ((p=xfgets(rFile))==NULL) {
				if (is_data) {
					pcnt = 0xff;
					return 1;
				}
				return 0;
			}
			if ((p = strchr(lbuf,'$')) == NULL)
				continue;
			q = (unsigned char *)++p;
			for(csum = 0; (*q >= ' ') && (*q != '*') ;)
				csum ^= *q++;
			if (*q == '*') {
				if (csum != (( *(q+1) - (( *(q+1) >'9')? 'A'-10:'0'))<<4)
					+ (*(q+2) - ((*(q+2)>'9')? 'A'-10:'0')) ) {
					if (csumFlag != -1) {
						fprintf(stderr,"* checksum error(%02x expected) at line %d. skipped.\n",csum,line);
						continue;
					}
				}
			} else {
				if (csumFlag == 0) {
					fprintf(stderr,"* Missing checksum at line %d. skipped.\n",line);
					continue;
				}
			}
			pcnt = sep(p);
		}
		if ( (strcmp(param[0],"GPRMC") == 0) && (pcnt >= 9) ) {
			if (is_data)
				return 1;
			if (*param[2] != 'A') {
				GPSLogp->thread = 1;
				pcnt = 0;
				continue;
			}
			angStrNME2Ang(param[RMC_LAT],&(position->lat));
			if (*param[RMC_LAS] == 'S')
				position->lat = -position->lat;
			angStrNME2Ang(param[RMC_LON],&(position->lon));
			if (*param[RMC_LOS] == 'W')
				position->lon = -position->lon;
			p = param[RMC_DAT]; /* 010298 */
			GPSLogp->recTime->tm_mday = (*p++ -'0')*10;
			GPSLogp->recTime->tm_mday += (*p++ -'0');
			GPSLogp->recTime->tm_mon = (*p++ -'0')*10;
			GPSLogp->recTime->tm_mon += (*p++ -'0')-1;
			GPSLogp->recTime->tm_year = (*p++ -'0')*10;
			GPSLogp->recTime->tm_year += (*p++ -'0');
			if (GPSLogp->recTime->tm_year < 70)
				GPSLogp->recTime->tm_year += 100;
			p = param[RMC_TIM];	/* 110244 */
			GPSLogp->recTime->tm_hour = (*p++ -'0')*10;
			GPSLogp->recTime->tm_hour += (*p++ -'0');
			GPSLogp->recTime->tm_min = (*p++ -'0')*10;
			GPSLogp->recTime->tm_min += (*p++ -'0');
			GPSLogp->recTime->tm_sec = (*p++ -'0')*10;
			GPSLogp->recTime->tm_sec += (*p++ -'0');
			
			GPSLogp->head = atof(param[RMC_HED]);
			GPSLogp->speed = atof(param[RMC_SPD])*notToKm;
			is_data++;
		} else if ( (param[0]==lbuf+1) && (strcmp(param[0],"PNTS") == 0) && (pcnt >= PNT_STA) ) {
			pcnt = 0;
			if (strchr("SE01",*param[PNT_TYP]) == NULL) {
				continue;
			}
			if (*param[PNT_STA] != '1') {
				GPSLogp->thread = 1;
				continue;
			}
			if (*param[PNT_TYP] == 'S')
				GPSLogp->thread = 1;

			GPSLogp->datTyp |= DAT_TKY;
			angStrNME2Ang(param[PNT_LAT],&(position->lat));
			if (*param[PNT_LAS] == 'S')
				position->lat = -position->lat;
			angStrNME2Ang(param[PNT_LON],&(position->lon));
			if (*param[PNT_LOS] == 'W')
				position->lon = -position->lon;
			GPSLogp->recTime->tm_mday = atoi(param[PNT_DAY]);
			GPSLogp->recTime->tm_mon = atoi(param[PNT_MON])-1;
			GPSLogp->recTime->tm_year = atoi(param[PNT_YER])-1900;
			p = param[PNT_TIM];	/* 110244 */
			GPSLogp->recTime->tm_hour = (*p++ -'0')*10;
			GPSLogp->recTime->tm_hour += (*p++ -'0');
			GPSLogp->recTime->tm_min = (*p++ -'0')*10;
			GPSLogp->recTime->tm_min += (*p++ -'0');
			GPSLogp->recTime->tm_sec = (*p++ -'0')*10;
			GPSLogp->recTime->tm_sec += (*p++ -'0');
			
			GPSLogp->head = atof(param[PNT_HED])/64*360;
			GPSLogp->speed = atof(param[PNT_SPD])/*notToKm*/;
			return 1;
		} else if ( (strcmp(param[0],"GPGGA") == 0) && (pcnt >= 10) ) {
			position->alt = atof(param[GGA_ALT]);
			if (nRec == 0)
				GPSLogp->datTyp |= DAT_ALT;
		} else if ( (strcmp(param[0],"GPGSA") == 0) && (pcnt >= 15) ) {
			int i,j;
			GPSLogp->rcvStat = (char) atoi(param[GSA_MOD]);
			for (i=3,j=0;i<15;++i)
				if ((usedSat[j] = (char) atoi(param[i]))!=0)
					++j;
			nmeaSatTmp.usedSat = j;
			usedSat[j] = 0;
		} else if ( (strcmp(param[0],"GPGSV") == 0) && (pcnt >= 7) ) {
			int i;
			if ( (i = atoi(param[2])) <= numGSV)
				nmeaSatTmp.numSat = 0;
			numGSV = i;
			for (i=4;i<=pcnt;) {
				if (param[i][0]=='\0')
					break;
				nmeaSatTmp.satprm[nmeaSatTmp.numSat].sta = 0;
				nmeaSatTmp.satprm[nmeaSatTmp.numSat].serial = (byte)atoi(param[i++]);
				nmeaSatTmp.satprm[nmeaSatTmp.numSat].angle =  atoi(param[i++]);
				nmeaSatTmp.satprm[nmeaSatTmp.numSat].adimath =  atoi(param[i++]);
				nmeaSatTmp.satprm[nmeaSatTmp.numSat++].lvl =  atoi(param[i++]);
			}
			if (numGSV == atoi(param[1])) {/* last msg */
				int j;
				for (i = 0; i < nmeaSatTmp.numSat ; i++) {
					for (j = 0;usedSat[j];++j) {
						if (usedSat[j] == nmeaSatTmp.satprm[i].serial) {
							nmeaSatTmp.satprm[i].sta = 5;
							break;
						}
					}
				}
				nmeaSat = nmeaSatTmp;
				GPSLogp->satdatp = &nmeaSat;
			}
		} else if ( (strcmp(param[0],"PGRMM") == 0) && (pcnt >= 1) ) {
			if (strcmp(param[1],"WGS 84") == 0)
				GPSLogp->datTyp &= ~DAT_TKY;
			else if (strcmp(param[1],"Tokyo") == 0)
				GPSLogp->datTyp |= DAT_TKY;
			else {
				fprintf(stderr,"*Error - unknown datum:'%s' !\n",param[1]); /* Unknown */
				return XErr = -1;
			}
		}
		pcnt = 0;
	}
}

void put_nme(FILE *out,char *buf)
{
	unsigned char sum = 0;
	char *p;
	
	p = buf;
	for (;;) {
		putc(*p++,out);
		if( *p == '*')
			break;
		sum ^= *p;
	}
	fprintf(out,"*%02X\n",sum);
}

/* 0.0 - 180.0 */
static char *ang2nmeDM(double angle,char *buf)
{
	int deg,min;
	unsigned long IAng;
	
	IAng = (unsigned long)((angle+0.5/6000000.0) * ONEDEG);
	deg = (int)(IAng/ONEDEG);
	IAng = (IAng%ONEDEG)*60;
	min = (int)(IAng/ONEDEG);
	IAng = (IAng%ONEDEG)/(ONEDEG/10000UL);
	sprintf(buf,"%d%02d.%04ld",deg,min,IAng);
	return buf;
}

void out_NME(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
	char buf[128];
	char latbuf[11];
	char lonbuf[12];
	char varbuf[7];
	char latRef = 'N';
	char lonRef = 'E';
	double lat = position->lat;
	double lon = position->lon;
	double spd = GPSLogp->speed/notToKm;
	
	strcpy(varbuf,",");
	if (nRec == 0) {
		if (GPSLogp->outTyp & DAT_TKY)
			put_nme(out,"$PGRMM,Tokyo*");
		else if (GPSLogp->outTyp & DAT_WGS)
			put_nme(out,"$PGRMM,WGS 84*");
	}
	if (GPSLogp->thread) {
		put_nme(out,"$GPRMC,,V,,,,,,,,*");
		if (GPSLogp->datTyp & DAT_ALT)
			put_nme(out,"$GPGGA,,,,,,,0,,,,,,*");
		GPSLogp->thread = 0;
	}
#ifdef MAGFIELD
	{
		char varRef = 'E';
		double var;
		double field[6];
		int yy = GPSLogp->recTime->tm_year%100;
		int model;
		
		if (yy < 50)
			model = 6;	/*WMM2000,7 for IGRF2000*/
		//		else if (yy < 90)
		//			model = 2;	/*WMM85*/
		else if (yy < 95)
			model = 3;	/*WMM90,1 for IGRF90*/
		else
			model = 4;	/*WMM95,5 for IGRF90*/
		var=rad_to_deg(
			SGMagVar(deg_to_rad(lat),deg_to_rad(lon),position->alt/1000.,
			yymmdd_to_julian_days(yy,GPSLogp->recTime->tm_mon+1,GPSLogp->recTime->tm_mday),model,field));
		if (var < 0) {
			varRef = 'W';
			var = -var;
		}
		sprintf(varbuf,"%d.%d,%c",(int)var,(int) ( (var-(int)var)*10 ),varRef );
	}
#else
	{
		double var;
		double df = lat-37.0;
		double dl = lon-138.0;
		var = 7.0 + 22.82/60.0+21.01/60.0*df - 7.36/60.0*dl - 0.197/60.0*df*df + 0.587/60.0*df*dl - 0.961/60.0*dl*dl;
		if ((var < 12.0) && (var > 4.0)) {
			var += 5.0/60.0;	/* for round */
			sprintf(varbuf,"%d.%d,W",(int)var,(int) ( (var-(int)var)*10.0 ));
		}
	}
#endif
	if (lat < 0) {
		latRef = 'S';
		lat = -lat;
	}
	if (lon < 0) {
		lonRef = 'W';
		lon = -lon;
	}
	
	/* $GPRMC,025325,A,3540.207,N,13944.534,E,85.0,131.9,060500,,,E*4D */
	sprintf(buf,"$GPRMC,%02d%02d%02d,A,%s,%c,%s,%c,%d.%d,%d.%d,%02d%02d%02d,%s*"
		,GPSLogp->recTime->tm_hour,GPSLogp->recTime->tm_min,GPSLogp->recTime->tm_sec
		,ang2nmeDM(lat,latbuf),latRef
		,ang2nmeDM(lon,lonbuf),lonRef
		,(int)spd,(int)((spd - (int)spd)*10)
		,(int)GPSLogp->head,(int)((GPSLogp->head - (int)GPSLogp->head)*10)
		,GPSLogp->recTime->tm_mday,GPSLogp->recTime->tm_mon+1,GPSLogp->recTime->tm_year%100
		,varbuf
		);
	put_nme(out,buf);
	/* $GPGGA,025332,3540.188,N,13944.583,E,2,03,7.8HDOP,90,M,0,M,23,ID*6A */
	if (GPSLogp->datTyp & DAT_ALT) {
		sprintf(buf,"$GPGGA,%02d%02d%02d,%s,%c,%s,%c,1,04,,%.1f,M,,M,,*"
			,GPSLogp->recTime->tm_hour,GPSLogp->recTime->tm_min,GPSLogp->recTime->tm_sec
			,latbuf,latRef
			,lonbuf,lonRef
			,position->alt
			);
		put_nme(out,buf);
	}
	nRec++;
}
