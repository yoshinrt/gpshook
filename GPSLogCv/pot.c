/* pot.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

This module support "kashmir as is .POT" only.

History:
2002/12/19 V0.34j W,S conversion, altitude check bug fix. time check improved.
2002/04/18 V0.34g support name of point.
2002/04/18 V0.34f support POTW.
2001/11/12 V0.33b add /ide= for POT Route.
2001/11/08 V0.33a ignore thread/duplicate for POTR.
2001/03/05 V0.27c contracted form of "Altitude"
2000/12/12 V0.26c "Tokyo","Coo"
2000/04/20 V0.22 use DAT_TIM for enable /pdate=,/ptime=
1999/08/31 V0.14 support /pdate=,/ptime=
1999/08/23 V0.12 add in_POT();
1999/08/05 V0.05 add alt,date,time
1999/08/05 V0.04 add out_POT() (requester:MasaakiSATO)
*/
#include <ctype.h>
#include <string.h>

#include "gpslogcv.h"

/* === in EXT POT out === */

static int adr;

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

static double angStrPot2Ang(char *p, int type)
{
	int minus;
    double d;

	while (isspace(*p))
		p++;
    switch(type) {
    case DAT_DMS:
		if ((minus = ('-' == *p))!=0)
			p++;
		d = atoi(p);p = xstrchr(p,'\'');
        d += atof(p)/60; p = xstrchr(p,'\'');
        d += atof(p)/3600; p = xstrchr(p,'\'');
        d += atof(p)/36000;
        break;
    default:
    case DAT_DEG:
    case DAT_DMM:
        XErr++;
        fprintf(stderr,"*Error - illegal angle format !\n");
        return 0;
    }
    return minus ? -d:d;
}

int in_POT(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
    char *p,*q;

    texBuf[0] = idBuf[0] = '\0';

    while((p=xfgets(rFile))!=NULL) {
        if (line == 1 ) {
            adr = -1;
            position->alt = -9999;
            GPSLogp->recTime->tm_hour = 25;
            GPSLogp->datTyp |= DAT_DMS;
            GPSLogp->datTyp |= DAT_TKY;
        }
		GPSLogp->datTyp &= ~DAT_HED;
        switch(toupper(*p)) {
        case 'D':    /* check Datum */
            if (_strnicmp(strtokX(p,"="),"Dat",3) == 0) {
                p = strtokX(NULL,"/");
                (void)strtokX(NULL,"/");    /* skip */
                mParam.da = atof(strtokX(NULL,"/"));
                mParam.df = atof(strtokX(NULL,"/"))/10000.0;
                mParam.dx = atof(strtokX(NULL,"/"));
                mParam.dy = atof(strtokX(NULL,"/"));
                mParam.dz = atof(strtokX(NULL,"/"));
                GPSLogp->mpp = &mParam;
                GPSLogp->datTyp &= ~DAT_DTM_MSK;
                if (_strnicmp(p,"Tokyo_mean",10)==0 ) {
                    GPSLogp->datTyp |= DAT_TKY;
                    mParam.da = -mParam.da;
                    mParam.df *= -10000.0;
                } else if (_strnicmp(p,"Tokyo",5)==0 ) {
                    GPSLogp->datTyp |= DAT_TKY;
                } else if (_strnicmp(p,"WGS84",5)==0 ) {
                    GPSLogp->datTyp |= DAT_WGS;
                } else {
                    GPSLogp->datTyp |= DAT_OTH;
                    strncpy(datumbuf,p,31);
                }
            }
            break;
        case 'C':
            if (_strnicmp(strtokX(p,"="),"coo",3) == 0) {
                if (_stricmp(strtokX(NULL," "),"LAT")!=0 )
                    XErr++;
                if (_stricmp(strtokX(NULL," "),"LON")!=0 )
                    XErr++;
                if (_stricmp(strtokX(NULL," \n"),"DMS")!=0 )
                    XErr++;
                if (XErr) {
                    fprintf(stderr,"*Error - illegal foramt(LAT LON DMS expected)!\n");
                    return -1;
                }
            }
            break;
        case 'O':
            if (_strnicmp(p,"ORD=POI",7) == 0) {
                while((p = xstrchr(q=p,'/'))!=q) {
                    if (_strnicmp(p,"tex",3)==0) {
                        char *q;
                        if ((q = xstrchr(p+3,'='))!=NULL) {
                            p = q;
                            q = texBuf;
                            while(((unsigned)*p >= ' ') && (*p !='/'))
                                *q++ = *p++;
                            *q = '\0';
							strcpy(titleBuf,texBuf);
                        }
                    } else if (_strnicmp(p,"ide",3)==0) {
                        char *q;
                        if ((q = xstrchr(p+3,'='))!=NULL) {
                            p = q;
                            q = idBuf;
                            while(((unsigned)*p >= ' ') && (*p !='/'))
                                *q++ = *p++;
                            *q = '\0';
                        }
                    } else if (_strnicmp(p,"ido=",4)==0) {
                        position->lat = angStrPot2Ang(p+4,GPSLogp->datTyp & DAT_ANGFMT_MSK);
                    } else if (_strnicmp(p,"kei",3)==0) {
                        position->lon = angStrPot2Ang(xstrchr(p+3,'='),GPSLogp->datTyp & DAT_ANGFMT_MSK);
                    } else if (_strnicmp(p,"alt",3)==0) {
                        position->alt = atof(xstrchr(p+3,'='));
                    } else if (_strnicmp(p,"pda",3)==0) {
                        GPSLogp->recTime->tm_mday = atoi(p=xstrchr(p+3,'='));p = xstrchr(p,'-');
                        GPSLogp->recTime->tm_mon = mname2m(p);p = xstrchr(p,'-');
                        GPSLogp->recTime->tm_year = atoi(p);
                        if (GPSLogp->recTime->tm_year < 70)
                            GPSLogp->recTime->tm_year += 100;
                        GPSLogp->recTime->tm_hour = 0;  /* for missing pti */
                        GPSLogp->recTime->tm_min = 0;
                        GPSLogp->recTime->tm_sec = 0;
                    } else if (_strnicmp(p,"pti",3)==0) {
                        GPSLogp->recTime->tm_hour = atoi(p=xstrchr(p+3,'='));p = xstrchr(p,':');
                        GPSLogp->recTime->tm_min = atoi(p);p = xstrchr(p,':');
                        GPSLogp->recTime->tm_sec = atoi(p);
                    } else if (_strnicmp(p,"adr",3)==0) {
                        int adr1;
                        adr1 = atoi(xstrchr(p+3,'='));
                        if (adr != adr1) {
                            GPSLogp->thread = 1;
                            adr = adr1;
                        }
                    } else if (_strnicmp(p,"deg",3)==0) {
						GPSLogp->datTyp |= DAT_HED;
						GPSLogp->head = atoi(p=xstrchr(p+3,'='));
                    }
                    if (XErr)
                        return -1;
                }
                XErr = 0;
                /* check integrity */
                if (nRec == 0) {
                    if (position->alt > -9999)
                        GPSLogp->datTyp |= DAT_ALT;
                    if ((GPSLogp->recTime->tm_hour < 25) && (GPSLogp->recTime->tm_year>=90))
                        GPSLogp->datTyp |= DAT_TIM;
                }
                return 1;
            }
            break;
        default:
            break;
        }   /* end switch */
    }   /* end while */
    return 0;   /* return EOF */
}

static char *ang2potDMS(double angle)
{
    int sgn,deg,min,sec;
    unsigned long IAng;

    if (angle<0) {
        sgn = -1;
        angle = -angle;
    } else {
        sgn = 1;
    }
    IAng = (unsigned long) ((angle+0.5/36000.0) * ONEDEG);  /* round */
    deg = (int)(IAng/ONEDEG);
    IAng = (IAng%ONEDEG)*60;
    min = (int)(IAng/ONEDEG);
    IAng = (IAng%ONEDEG)*60;
    sec = (int)(IAng/ONEDEG);
    IAng = (IAng%ONEDEG)/(ONEDEG/10);
    sprintf(angbuf,"%04d'%02d'%02d'%d",deg*sgn,min,sec,(int)IAng);
    if (sgn == -1)
        return angbuf;
    else
        return &angbuf[1];
}

/*
ORD=POI/tex=No. 4/ido=037'43'21'7
/kei=140'04'07'8/adr=1
/alt=+840/col=12/pda=06-JUL-99/pti=22:59:05
*/
static const char toMname[]="JAN\0FEB\0MAR\0APR\0MAY\0JUN\0JUL\0AUG\0SEP\0OCT\0NOV\0DEC";
static double lat,lon;

static void _out_POT(FILE *out, cordinate_t *position, cnvData_t *GPSLogp, char typ)
{
    if (nRec == 0) {
        adr = 0;
        lat = lon = 999.9;
        fprintf(out,(GPSLogp->outTyp & DAT_TKY) ?
            "Dat=Tokyo mean/Bessel 1841/739.845/0.10037483/-128/481/664\n"
            :
            "Dat=WGS84/WGS84/0/0/0/0/0\n"
        );
        /* fprintf(out,"coordinate=LAT LON DMS\n"); */
    }
    if (typ == 'T') {
        if (GPSLogp->thread)
            adr++;
        fprintf(out,"ORD=POI/tex=/ido=%s",ang2potDMS(position->lat));
        fprintf(out,"/kei=%s/adr=%d",ang2potDMS(position->lon),adr);
    } else {
        char idebuf[16];
        if ((lat == position->lat) && (lon == position->lon) && (typ != 'W'))
            return;
        lat = position->lat;
        lon = position->lon;

        if (texBuf[0] == '\0') {
            if (descBuf[0] != '\0') {
                strcpy(texBuf,descBuf);
            } else if (titleBuf[0] != '\0') {
                strcpy(texBuf,titleBuf);
			} else {
                sprintf(texBuf,"%06d",nRec+1);
            }
        }

        if (idBuf[0] != '\0')
            strncpy(idebuf,idBuf,15);
        else
            sprintf(idebuf,"%06d",nRec+1);

        fprintf(out,"ORD=POI/tex=%s/ide=%s",texBuf,idebuf);
        fprintf(out,"/ido=%s",ang2potDMS(position->lat));
        fprintf(out,"/kei=%s",ang2potDMS(position->lon));
        if (typ == 'R')
            fprintf(out,"/adr=%d",adr);
    }

    if (GPSLogp->datTyp & DAT_ALT)
        fprintf(out,"/alt=%+d",round(position->alt));
    fprintf(out,"/col=14");
    if (GPSLogp->datTyp & DAT_TIM)
        fprintf(out,"/pda=%02d-%s-%02d/pti=%02d:%02d:%02d"
          ,GPSLogp->recTime->tm_mday,&toMname[GPSLogp->recTime->tm_mon*4],GPSLogp->recTime->tm_year%100
          ,GPSLogp->recTime->tm_hour,GPSLogp->recTime->tm_min,GPSLogp->recTime->tm_sec);
    fprintf(out,"\n");
    texBuf[0] = titleBuf[0] = '\0';
    GPSLogp->thread = 0;
    nRec++;
}

void out_POTT(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
    _out_POT(out, position, GPSLogp, 'T');
}
void out_POTR(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
    _out_POT(out, position, GPSLogp, 'R');
}
void out_POTW(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
    _out_POT(out, position, GPSLogp, 'W');
}
