/* ips.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

 SONY/ETAK GPS SONY Format log .IPS

History:
 2001/08/22 V0.31j FIX HGR bug
 2001/07/12 V0.31h FIX HGR3 bug
 2001/03/19 V0.28c add out_IPS()
 2001/03/15 V0.28b Satellite information/Receiver Status 
 2000/02/22 V0.03 IPS8000 support.(not tested yet)
 1999/08/05 V0.02 add in_IPS() (requester:MasaakiSATO)
*/
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "gpslogcv.h"

/* === in IPS === */
static double strAngIPS2Ang(char *p,int typ,char *q)
{
    long l;
    double d;

    /* convert to DEG */
    l = atol(p+1);
    switch(typ){
    case DAT_DMM:/*DM*/
        d = (l%100000);l/=100000;
        d += d/60000 +l;
		d += (double)(*q - '0')/600000;
        break;
    case DAT_DMS:/*DMS*/
        d = (l%1000);l/=1000;
        d = d/600 + (l%100);l/=100;
        d = d/60 + l;
		d += (double)(*q - 'A')/360000;
        break;
    case DAT_DEG:
    default:
        XErr++;
        fprintf(stderr,"*Error - illegal angle format !\n");
        d = 0.0;
        break;
    }
    switch(*p) {
    case 'S':
    case 'W':
        d = -d;
        break;
    case 'N':
    case 'E':
        break;
    default:
        break;
    }
    return d;
}

#include "ips.h"

static Satdat_t ipsSat;

int in_IPS(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
    char *p,*q,*r;
    int i,sum,numSatCol;

    while((p = xfgets(rFile))!=NULL) {
        if (*p != 'S')
            continue;

        if (strncmp(p,"SM",2)==0) {
			if (*(p+2) == '0')
	            numSatCol = 16;
			else
	            numSatCol = 12;
		} else if (strncmp(p,"SONY",4)==0) {
			if (strncmp(p+4,"99",2) == 0)
	            numSatCol = 16;
			else
	            numSatCol = 8;
		} else {
             fprintf(stderr,"Warning: Missing IPS Header!\n");
			 continue;
		}
        r = p+numSatCol*IPS_SAT_LEN;

        for(sum=0,q=p;(q<r+IPS_PAR_OFS) && *q;)
            sum += *q++;
        if (*q != ((sum&1)?'O':'E')) {
            fprintf(stderr,"Warning: IPS Data parity error !\n");
            continue;
        }
        if (islower(*(p+IPS_LAT_DIR_POS))) {
            GPSLogp->thread = 1;
            continue;
        }
        GPSLogp->datTyp &= ~DAT_ANGFMT_MSK;
        GPSLogp->datTyp |= (isalpha(*(r+IPS_FMT_OFS))) ? DAT_DMS:DAT_DMM;

		q = p + IPS_SAT_SER_POS;
		ipsSat.usedSat = 0;
		for (i=-1;i<numSatCol-1;) {
			if (*q < 'A')
				break;
			ipsSat.satprm[++i].serial = (byte)(islower(*q) ? *q++ -'a'+25:*q++ -'A'+1);
			ipsSat.satprm[i].angle = islower(*q) ? ('a'- *q++)*10 : (*q++ - 'A')*10; 
			ipsSat.satprm[i].adimath = islower(*q) ? ('a' - *q++ -1)*10 : (*q++ - 'A')*10;
			if ((ipsSat.satprm[i].sta = (byte)(*q++ - 'A')) >= 5)
				ipsSat.usedSat++;
			ipsSat.satprm[i].lvl = (*q++ - 'D')*4;	/* minimum usable lvl='D' */
		}
		ipsSat.numSat = i+1;
		GPSLogp->satdatp = &ipsSat;

        GPSLogp->datTyp &= ~DAT_DTM_MSK;
        switch(*(p+IPS_DATUM_POS)){
        case 'A':
            GPSLogp->datTyp |= DAT_WGS;
            break;
        case 'B':
            GPSLogp->datTyp |= DAT_TKY;
            break;
        default:
            /* Unknown */
            fprintf(stderr,"*Error - unknown datum code:'%c' !\n",*(p+IPS_DATUM_POS));
            return XErr = -1;
        }

		GPSLogp->rcvStat = (char) (*(p+IPS_MODE_POS)-'0'-1);

        *(q = p+IPS_DOP_POS) = '\0';
        GPSLogp->recTime->tm_sec = atoi(q-=2);*q='\0';
        GPSLogp->recTime->tm_min = atoi(q-=2);*q='\0';
        GPSLogp->recTime->tm_hour = atoi(q-2);

        *(q = p+IPS_MES_WEEK_POS) = '\0';
        GPSLogp->recTime->tm_mday = atoi(q-=2);*q='\0';
        GPSLogp->recTime->tm_mon = atoi(q-=2)-1;*q='\0';
        GPSLogp->recTime->tm_year = atoi(q-=2);*q='\0';
        if (GPSLogp->recTime->tm_year < 70)
            GPSLogp->recTime->tm_year += 100;

        GPSLogp->head = atoi(q = p+IPS_COURSE_POS);*q='\0';
        GPSLogp->speed = atoi(q = p+IPS_SPEED_POS);*q='\0';

        position->alt = atoi(q = p+IPS_HGH_SGN_POS);*q='\0';
        position->lon = strAngIPS2Ang(q = p+IPS_LON_DIR_POS,GPSLogp->datTyp & DAT_ANGFMT_MSK,r+IPS_LON_MLD);*q='\0';
        position->lat = strAngIPS2Ang(p+IPS_LAT_DIR_POS,GPSLogp->datTyp & DAT_ANGFMT_MSK,r+IPS_LAT_MLD);
        return 1;
    }
    return 0;
}

static char *ang2trkDM(double angle, char *p)
{
	int deg,min;
	unsigned long IAng;

	IAng = (unsigned long)((angle+0.5/600000.0) * ONEDEG);
	deg = (int)(IAng/ONEDEG);
	IAng = (IAng%ONEDEG)*60;
	min = (int)(IAng/ONEDEG);
	IAng = (IAng%ONEDEG)/(ONEDEG/10000UL);
	sprintf(angbuf,"%03d%02d%04ld",deg,min,IAng);
	*p = angbuf[8];
	return angbuf;
}

static char *ang2trkDMS(double angle, char *p)
{
	int deg,min,sec;
	unsigned long IAng;

	IAng = (unsigned long)((angle+0.5/360000.0) * ONEDEG);
	deg = (int)(IAng/ONEDEG);
	IAng = (IAng%ONEDEG)*60;
	min = (int)(IAng/ONEDEG);
	IAng = (IAng%ONEDEG)*60;
	sec = (int)(IAng/ONEDEG);
	IAng = (IAng%ONEDEG)/(ONEDEG/100UL);
	sprintf(angbuf,"%03d%02d%02d%02d",deg,min,sec,(int)IAng);
	*p = (char)(angbuf[8]+'A'-'0');
	return angbuf;
}

static char ipsbuf[255];

void out_IPS(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
	char *p;
	char latRef = 'N';
	char lonRef = 'E';
	double lat = position->lat;
	double lon = position->lon;
	int i;
	byte sum = 0;
	
	if (lat < 0) {
		latRef = 'S';
		lat = -lat;
	}
	if (lon < 0) {
		lonRef = 'W';
		lon = -lon;
	}

	sprintf(ipsbuf,"%s%02d%02d%02d%c%02d%02d%02d",
		"SONY81",
		GPSLogp->recTime->tm_year%100,
		GPSLogp->recTime->tm_mon+1,
		GPSLogp->recTime->tm_mday,
		(char)(GPSLogp->recTime->tm_wday+'0'),
		GPSLogp->recTime->tm_hour,
		GPSLogp->recTime->tm_min,
		GPSLogp->recTime->tm_sec);
	
	switch(GPSLogp->outTyp & DAT_ANGFMT_MSK) {
	case DAT_DMM:
		sprintf(&ipsbuf[IPS_LAT_DIR_POS],"%c%s",latRef,ang2trkDM(lat,&ipsbuf[IPS_LAT_MLD+IPS_SAT_LEN*8])+1);
		sprintf(&ipsbuf[IPS_LON_DIR_POS],"%c%s",lonRef,ang2trkDM(lon,&ipsbuf[IPS_LON_MLD+IPS_SAT_LEN*8]));
		break;
	default:
	case DAT_DMS:
		sprintf(&ipsbuf[IPS_LAT_DIR_POS],"%c%s",latRef,ang2trkDMS(lat,&ipsbuf[IPS_LAT_MLD+IPS_SAT_LEN*8])+1);
		sprintf(&ipsbuf[IPS_LON_DIR_POS],"%c%s",lonRef,ang2trkDMS(lon,&ipsbuf[IPS_LON_MLD+IPS_SAT_LEN*8]));
		break;
	}
	sprintf(&ipsbuf[IPS_HGH_SGN_POS],"%c%04d%03d%03d",
		(position->alt<0) ? '-':'+',
		(int)(fabs(position->alt)+.5),
		(int)(GPSLogp->speed+.5),
		(int)(GPSLogp->head+.5));
	
	memcpy(&ipsbuf[IPS_MES_DATE_POS],&ipsbuf[IPS_UTC_DATE_POS],IPS_TIME_STAMP_LEN);
	
	sprintf(&ipsbuf[IPS_DOP_POS],"%c%c%c",
		(char)('A'/*+dop*/),
		(char)((GPSLogp->datTyp & DAT_STA) ? GPSLogp->rcvStat+'1':'3'),
		(char)((GPSLogp->datTyp & DAT_TKY) ? 'B':'A') );
	
	i = 0;
	p = &ipsbuf[IPS_SAT_SER_POS];
	if (GPSLogp->satdatp!=NULL) {
		int j,v;
		SatParm_t *satp;
		int maxUnused = 8-GPSLogp->satdatp->usedSat;
		for (j=0;j<GPSLogp->satdatp->numSat;++j) {
			satp = &GPSLogp->satdatp->satprm[j];
			if (satp->sta != 5)
				if (--maxUnused < 0)
					continue;
			*p++ = (char)((satp->serial>24) ? 'a'+satp->serial-25:'A'+satp->serial-1);
			*p++ = (char)((satp->angle <90) ? (satp->angle+5)/10 + 'A':18-(satp->angle+5)/10 + 'a');
			v = (int)satp->adimath;
			if (v < 0)
				v += 360;
			v += 5;
            if (v == 10) v = 9;
            if (v == 190) v = 189;
            v /= 10;
            *p++ = (char)((v < 18) ? v+'A': 35-v+'a');
			*p++ = (char)(satp->sta + 'A');
			*p++ = (char)((satp->lvl<12) ? 'A':satp->lvl/4+'D');
			++i;
		}
	}
	for (;i<8;++i,p+=IPS_SAT_LEN)
		sprintf(p,"___AA");

	*p++ = 'A';	/* OSC */
	*p++ = 'D';
	p++;p++;
	*p = '\0';
	for (p = ipsbuf;*p;)
		sum = (byte)(sum + (byte)*p++);
	*p++ = (char)((sum&1)?'O':'E');	/* PAR */
//	*p++ = 0x0d;
	*p++ = 0x0a;
	*p = '\0';
	if (GPSLogp->thread) {
		if (nRec!=0) {
			ipsbuf[IPS_LAT_DIR_POS] = (char)tolower(ipsbuf[IPS_LAT_DIR_POS]);
			fputs(ipsbuf,out);
			ipsbuf[IPS_LAT_DIR_POS] = (char)toupper(ipsbuf[IPS_LAT_DIR_POS]);
		}
		GPSLogp->thread = 0;
	}
	fputs(ipsbuf,out);
	nRec++;
}
