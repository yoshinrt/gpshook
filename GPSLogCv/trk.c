/* trk.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

 Garmin PCX5 .TRK

History:
2002/05/01 V0.34g support name of point for waypoints.
2002/04/18 V0.34f Fix Waypoint alt bug.
2001/03/28 V0.29 support .WPT
1998/08/23 V0.12 in_TRK(); -9999 means no valid alitude data 
1999/08/06 V0.11
1999/08/06 V0.05 add Title output
*/
#include <math.h>
#include <string.h>
#include <ctype.h>

#include "gpslogcv.h"

/* === in TRK out === */
static double angStr2Ang(char *str,int type)
{
    char *p,*q;
    long l;
    double angle = 0.0;

    if (XErr)
        return angle;

    for (p = q = str+1;*q;q++)
        if (*q == '.')
            break;
    switch(type) {
    case DAT_DEG:
        angle = atof(p);
        break;
    case DAT_DMM:
        angle = atof(q-=2)/60.0;
        for(l=0;(p<q) && *p;)
            l = l*10 + *p++ - '0';
        angle += l;
        break;
    case DAT_DMS:
        angle = atof(q-=2)/3600.0;
        l = *--q -'0';
        l += (*--q -'0')*10;
        angle += (double)l/60.0;
        for(l=0;(p<q) && *p;)
            l = l*10 + *p++ - '0';
        angle += l;
        break;
    default:
        XErr++;
        fprintf(stderr,"*Error - illegal angle format !\n");
        break;
    }
    switch(*str) {
    case '-':
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

/*
U  LAT LON DEG
T  N35.5406968 E139.4345175 08-JUL-97 14:44:17 -9999
U  LAT LON DM
T  N3532.63909 E13925.88437 08-JUL-97 14:44:17 -9999
U  LAT LON DMS
T  N344527.374 E1374418.901 05-JUL-97 09:33:04 -9999
*/

static int nzero = 1;
static char tbuf0[TTBUFLEN];

int in_TRK(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
/* global val :datum,mParam */
/* call lib: fgets,_strnicmp,atof */
/* call strtokX,mname2m */
/* return value -1:Error, 0:EOF, 1:OK */
{
    char *p,tbuf[TTBUFLEN];

    while((p=xfgets(rFile))!=NULL) {
        switch(*p) {
        case 'H':    /* check header */
            if ((*(p+2) != ' ') && (titleBuf[0]=='\0')) {
                strncpy(titleBuf,p+2,TTBUFLEN-1);
                p = titleBuf+strlen(p+2);
                if (*--p == '\n')
                    *p = '\0';
           } else if ((_strnicmp(p,"H  MAP:",7) == 0) && (mapTtlBuf[0]=='\0')) {
                strncpy(mapTtlBuf,p+7,MTBUFLEN-1);
                p = mapTtlBuf+strlen(p+7);
                if (*--p == '\n')
                    *p = '\0';
            } else
            GPSLogp->thread = 1;
            break;
        case 'M':    /* check Datum */
            if (_strnicmp(p,"M  G ",5) == 0) {
                (void)strtokX(p+10," ");
                mParam.da = atof(strtokX(NULL," "));
                mParam.df = atof(strtokX(NULL," "));
                mParam.dx = atof(strtokX(NULL," "));
                mParam.dy = atof(strtokX(NULL," "));
                mParam.dz = atof(strtokX(NULL," "));
                GPSLogp->mpp = &mParam;
                if (_strnicmp(p+5,"Tokyo",5)==0 ) {
                    GPSLogp->datTyp |= DAT_TKY;
                } else if (_strnicmp(p+5,"WGS84",5)==0 ) {
                    GPSLogp->datTyp |= DAT_WGS;
                } else if (_strnicmp(p+5,"WGS 84",6)==0 ) {
                    GPSLogp->datTyp |= DAT_WGS;
                } else {
                    GPSLogp->datTyp |= DAT_OTH;
                    strncpy(datumbuf,p+5,31);
                }
            }
            GPSLogp->thread = 1;
            break;
        case 'U':    /* check Format */
            if (_strnicmp(p,"U  LAT LON ",11) == 0) {
                if (_strnicmp(p+11,"DMS",3) == 0) {
                    GPSLogp->datTyp |= DAT_DMS;
                } else if (_strnicmp(p+11,"DM",2) == 0) {
                    GPSLogp->datTyp |= DAT_DMM;
                } else if (_strnicmp(p+11,"DEG",3) == 0) {
                    GPSLogp->datTyp |= DAT_DEG;
                } else {
                    fprintf(stderr,"*Error - illegal modes('DEG or DMS or DM' expected)!\n");
                    return (XErr = -1);
                }
            }
            GPSLogp->thread = 1;
            break;
        case 'W':
            strncpy(idBuf,strtokX(p+3," "),TTBUFLEN-1); /* skip idnt */
            position->lat = angStr2Ang(strtokX(NULL," "),GPSLogp->datTyp & DAT_ANGFMT_MSK);
            position->lon = angStr2Ang(strtokX(NULL," "),GPSLogp->datTyp & DAT_ANGFMT_MSK);
            GPSLogp->recTime->tm_mday = atoi(strtokX(NULL,"-"));
            GPSLogp->recTime->tm_mon = mname2m(strtokX(NULL,"-"));
            GPSLogp->recTime->tm_year = atoi(strtokX(NULL," "));
            if (GPSLogp->recTime->tm_year < 70)
                GPSLogp->recTime->tm_year += 100;
            GPSLogp->recTime->tm_hour = atoi(strtokX(NULL,":"));
            GPSLogp->recTime->tm_min = atoi(strtokX(NULL,":"));
            GPSLogp->recTime->tm_sec = atoi(strtokX(NULL," "));
            position->alt = atof(strtokX(NULL," "));
            if (position->alt <= -9998.9)
                GPSLogp->datTyp &= ~DAT_ALT;
            else
                GPSLogp->datTyp |= DAT_ALT;

            if (mktime(GPSLogp->recTime)>=0)
                GPSLogp->datTyp |= DAT_TIM;
            else
                GPSLogp->datTyp &= ~DAT_TIM;

            if ((p = strtok(NULL," "))!=NULL) {
                strncpy(descBuf,p,TTBUFLEN-1);
                if ((p = strchr(tbuf,'\n'))!=NULL)
                    *p = '\0';
            } else
                descBuf[0] = '\0';
//          if (strcmp(tbuf0,tbuf) != 0) {
//              GPSLogp->thread = 1;
//              strcpy(titleBuf,tbuf);
//              strcpy(tbuf0,tbuf);
//          }
            minTime = INT_MIN;
            return 1;
            break;  /* never executed */
        case 'T':
            position->lat = angStr2Ang(strtokX(p+3," "),GPSLogp->datTyp & DAT_ANGFMT_MSK);
            position->lon = angStr2Ang(strtokX(NULL," "),GPSLogp->datTyp & DAT_ANGFMT_MSK);
            GPSLogp->recTime->tm_mday = atoi(strtokX(NULL,"-"));
            GPSLogp->recTime->tm_mon = mname2m(strtokX(NULL,"-"));
            GPSLogp->recTime->tm_year = atoi(strtokX(NULL," "));
            if (GPSLogp->recTime->tm_year < 70)
                GPSLogp->recTime->tm_year += 100;
            GPSLogp->recTime->tm_hour = atoi(strtokX(NULL,":"));
            GPSLogp->recTime->tm_min = atoi(strtokX(NULL,":"));
            GPSLogp->recTime->tm_sec = atoi(strtokX(NULL," "));
            position->alt = atof(strtokX(NULL," "));
            if (position->alt <= -9998.9)
                GPSLogp->datTyp &= ~DAT_ALT;
#if 0
            if (position->alt == 0) {
                if (nzero++) {
                    GPSLogp->thread = 1;
//                  continue;
                }
            } else {
                nzero = 1;
            }
#endif
            return 1;
            break;  /* never executed */
        default:
            GPSLogp->thread = 1;
            if ((nRec == 0) && (atoi(p)==16)) {
                xfseek(rFile,0L,SEEK_SET);
                newType = "trkm";
                return XErr = -2;
            }
            break;
        }   /* end switch */
    }   /* end while */
    return 0;   /* return EOF */
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
    IAng = (IAng%ONEDEG)/(ONEDEG/100000UL);
    sprintf(angbuf,"%03d%02d.%05ld",deg,min,IAng);
    return angbuf;
}

static char *ang2trkDMS(double angle)
{
    int deg,min,sec;
    unsigned long IAng;

    IAng = (unsigned long)((angle+0.5/3600000.0) * ONEDEG);
    deg = (int)(IAng/ONEDEG);
    IAng = (IAng%ONEDEG)*60;
    min = (int)(IAng/ONEDEG);
    IAng = (IAng%ONEDEG)*60;
    sec = (int)(IAng/ONEDEG);
    IAng = (IAng%ONEDEG)/(ONEDEG/1000UL);
    sprintf(angbuf,"%03d%02d%02d.%03d",deg,min,sec,(int)IAng);
    return angbuf;
}

static const char *typstrpcx[]= {
    "DEG","DM","DMS"
};

static const char toMname[]="JAN\0FEB\0MAR\0APR\0MAY\0JUN\0JUL\0AUG\0SEP\0OCT\0NOV\0DEC";

void out_PCX(FILE *out, cordinate_t *position, cnvData_t *GPSLogp, char typ)
{
    char latRef = 'N';
    char lonRef = 'E';
    double lat = position->lat;
    double lon = position->lon;

    if (nRec == 0) {
        fprintf(out,"\n");
        fprintf(out,"H  SOFTWARE NAME & VERSION\n");
        fprintf(out,"I  PCX5 "PCXVER"\n");
        fprintf(out,"\n");
        fprintf(out,"H  R DATUM                IDX DA            DF            DX            DY            DZ\n");
        fprintf(out,"M  G %s\n",
                (GPSLogp->outTyp & DAT_TKY) ?
                    "Tokyo                116 +7.398450e+02 +1.003748e-05 -1.280000e+02 +4.810000e+02 +6.640000e+02\n"
                    :
                    "WGS 84               121 +0.000000e+00 +0.000000e+00 +0.000000e+00 +0.000000e+00 +0.000000e+00\n");
        fprintf(out,"H  COORDINATE SYSTEM\n");
        fprintf(out,"U  LAT LON %s\n",typstrpcx[(GPSLogp->outTyp & DAT_ANGFMT_MSK)-1]);
        if (typ == 'W')
            fprintf(out,"\nH  IDNT   LATITUDE    LONGITUDE    DATE      TIME     ALT   DESCRIPTION\n");
    }
    if (typ == 'T') {
        if (GPSLogp->thread) {
            if (mapTtlBuf[0] != '\0') {
                fprintf(out,"\nH  MAP:%s\n",mapTtlBuf);
                mapTtlBuf[0] = '\0';
            }
            if (titleBuf[0] != '\0') {
                fprintf(out,"\nH %s\n",titleBuf);
                titleBuf[0] = '\0';
            }
            fprintf(out,"\nH  LATITUDE    LONGITUDE    DATE      TIME     ALT    ;track\n");
        }
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
    if ((GPSLogp->datTyp & DAT_ALT) == 0)
        position->alt = -9999.0;

    fprintf(out,"%c  ",typ);
    if (typ == 'W') {
        if (idBuf[0] == '\0')
            fprintf(out,"%06d ",nRec);
        else {
            int i;
            for (i=0;i<6;i++) {
                if (isalnum(idBuf[i]))
                    putc(idBuf[i],out);
                else
                    putc(' ',out);
            }
            putc(' ',out);
        }
    }
    switch(GPSLogp->outTyp & DAT_ANGFMT_MSK) {
    case DAT_DEG:
        fprintf(out,"%C%010.7f %C%011.7f",latRef,lat,lonRef,lon);
        break;
    case DAT_DMM:
        fprintf(out,"%C%s ",latRef,ang2trkDM(lat)+1);
        fprintf(out,"%C%s",lonRef,ang2trkDM(lon));
        break;
    default:
    case DAT_DMS:
        fprintf(out,"%C%s ",latRef,ang2trkDMS(lat)+1);
        fprintf(out,"%C%s",lonRef,ang2trkDMS(lon));
        break;
    }
	if (GPSLogp->datTyp & DAT_TIM) {
	    fprintf(out," %02d-%s-%02d %02d:%02d:%02d"
		  ,GPSLogp->recTime->tm_mday,&toMname[GPSLogp->recTime->tm_mon*4],GPSLogp->recTime->tm_year%100
		  ,GPSLogp->recTime->tm_hour,GPSLogp->recTime->tm_min,GPSLogp->recTime->tm_sec);
	} else {
	    fprintf(out," 01-JAN-80 00:00:00");
	}
    fprintf(out," %5.0f ",position->alt);

    if (typ == 'W') {
        if (descBuf[0]!='\0')
            fprintf(out,"%s",descBuf);
        else if (texBuf[0]!='\0')
            fprintf(out,"%s",texBuf);
		else
            fprintf(out,"%s",titleBuf);
    }
    putc('\n',out);
    nRec++;
}

void out_TRK(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
    out_PCX(out, position, GPSLogp, 'T');
}
void out_WPT(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
    out_PCX(out, position, GPSLogp, 'W');
    minTime = INT_MIN;
}

