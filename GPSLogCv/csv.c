/* csv.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

 CSV

History:
2001/01/22 V0.27b fix can't get hour.(enbug0.26d) 
2001/01/17 V0.27 auto select bug fix.
2001/01/15 V0.26d fix CSVG time bug.
2000/11/16 V0.26a add heading
2000/07/02 double quotation bug fix.
2000/03/13 add in_CSV(),out_CSV(),in_CSG
2000/03/09 initial out_CSG()
*/
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "gpslogcv.h"

static double distSum,distSum0;

int in_CSG(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
/* return value -1:Error, 0:EOF, 1:OK */
{
	char *p,*q;

	while((p=xfgets(rFile))!=NULL) {
		if (atoi(p) == 0)
			continue;
		p = strchr(p,',')+1;
		p = strchr(p,'/');GPSLogp->recTime->tm_year = atoi(p-4)-1900;
		GPSLogp->recTime->tm_mon = atoi(++p)-1;
		p = strchr(p,'/')+1;GPSLogp->recTime->tm_mday = atoi(p);
		p = strchr(p,':')-2;
		if (!isdigit(*p))
			p++;
		GPSLogp->recTime->tm_hour = atoi(p);
		p = strchr(p,':')+1;GPSLogp->recTime->tm_min = atoi(p);
		p = strchr(p,':')+1;GPSLogp->recTime->tm_sec = atoi(p);
		if ((p = strchr(p,','))==NULL) {
			GPSLogp->thread = 1;
			continue;
		}
		distSum = atof(++p);
		GPSLogp->dist = (distSum - distSum0)*1000;
		distSum0 = distSum;
		p = strchr(p,',')+1;GPSLogp->speed = atof(p);
		p = strchr(p,',')+1;GPSLogp->head = atof(p);
		p = strchr(p,',')+1;q = strchr(p,',');
		if (*p == '"') {
			p++;
			if (*(q-1) == '"')
				*(q-1) = '\0';
		}
		*q =  '\0';
		if ((/*GPSLogp->thread = */strncmp(ttbuf,p,TTBUFLEN-1))!=0){
			strncpy(titleBuf,p,TTBUFLEN-1);
			strncpy(ttbuf,p,TTBUFLEN-1);
		}
		p = strchr(++q,',');
		if (p++ == q)
			continue;
		p = strchr(p,',')+1;position->lat = atof(p)/TORAD;
		p = strchr(p,',')+1;position->lon = atof(p)/TORAD;
		p = strchr(p,',')+1;position->alt = atof(p);
		return 1;
	}
	return 0;	/* return EOF */
}


static char *makAngStrJ(double loc, char *buf, char *plus, char *minus)
{
	int deg,min,sec;
	long IAng;

	strncpy(buf,(loc > 0) ? plus:minus,4);
	IAng = (long)(fabs(loc)*3600+0.5);

	deg = (int)(IAng/3600);
	min = (int)((IAng%3600)/60.0);
	sec = (int)(IAng%60);
	sprintf(buf+4,"%3d%��%02d��%02d��",deg,min,sec);
	return buf;
}

static char latJBuf[32],lonJBuf[32];
static double lat,lon;

void out_CSG(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
	if (nRec == 0) {
		fprintf(out,"\"�n�_�ԍ�\",\"�}�[�N\",\"���t\",\"����\",\"�ݐϋ���(Km)\",\"����(Km/h)\",\"�i�s����(�x)\",\"�t�ߖ���\",\"�ܓx\",\"�o�x\",\"�ܓx(���W�A��)\",\"�o�x(���W�A��)\",\"�W��(m)\"\n");
		distSum = 0;
		lat = lon = 0;
	}
	lat = position->lat;	/* save last point */
	lon = position->lon;
	distSum += GPSLogp->dist;

	if (GPSLogp->thread)
		fprintf(out,"%d,0,\"%d/%02d/%02d\",\"%02d:%02d:%02d\"\n"
			,++nRec
			,GPSLogp->recTime->tm_year+1900,GPSLogp->recTime->tm_mon+1,GPSLogp->recTime->tm_mday
			,GPSLogp->recTime->tm_hour,GPSLogp->recTime->tm_min,GPSLogp->recTime->tm_sec
		);
	fprintf(out,"%d,0,\"%d/%02d/%02d\",\"%02d:%02d:%02d\",%.3f,%d,%.3f,\"%s\",\"%s\",\"%s\",%11.9f,%11.9f,%d\n"
		,++nRec
		,GPSLogp->recTime->tm_year+1900,GPSLogp->recTime->tm_mon+1,GPSLogp->recTime->tm_mday
		,GPSLogp->recTime->tm_hour,GPSLogp->recTime->tm_min,GPSLogp->recTime->tm_sec
		,distSum/1000
		,(int)(GPSLogp->speed+.5)
		,(GPSLogp->head <= 180) ? GPSLogp->head : GPSLogp->head - 360.0
		,titleBuf
		,makAngStrJ(position->lat, latJBuf, "�k��","���")
		,makAngStrJ(position->lon, lonJBuf, "���o","���o")
		,position->lat*TORAD
		,position->lon*TORAD
		,((GPSLogp->datTyp & DAT_ALT) && (position->alt > -9997))? round(position->alt):0 );

	GPSLogp->thread = 0;
}

/* kashmir */

static double angStr2AngY(char *str)
{
	char *p,*q;
	int sgn = 0;
	long l;
	double angle = 0.0;

	for (p = q = str;*q;q++) {
		if (*q == '.')
			break;
	}
	angle =	atof(q+=3)/36000.0;
	l = *--q -'0';
	l += (*--q -'0')*10;
	angle += (double)l/60.0;

	if (*p == '-') {
		--sgn;
		p++;
	}
	angle += atol(p);

	if (sgn <0)
		angle = -angle;
	return angle;
}

int in_CSV(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
/* return value -1:Error, 0:EOF, 1:OK */
{
	char *p,*q;

	while((p=xfgets(rFile))!=NULL) {
		switch(*p) {
		case ';':
			continue;
		case 'H':	 /* check header */
			(void)strtokX(p,",");
			if ( (p = strtokX(NULL,",")) != NULL ) {
				if ((q = strtokX(NULL," "))!=NULL)
					*q =  '\0';
				strncpy(titleBuf,p,TTBUFLEN-1);
				titleBuf[0] = '\0';
			}
			GPSLogp->thread = 1;
			break;
		case 'T':
			(void)strtokX(p,",");
			(void)strtokX(NULL,",");
			position->lat = angStr2AngY(strtokX(NULL,","));
			position->lon = angStr2AngY(strtokX(NULL,","));
			position->alt = atof(strtokX(NULL,","));
			if (position->alt <= -9998.9)
				GPSLogp->datTyp &= ~DAT_ALT;
			GPSLogp->recTime->tm_year = atoi(strtokX(NULL,"-"))-1900;
			GPSLogp->recTime->tm_mon = atoi(strtokX(NULL,"-"))-1;
			GPSLogp->recTime->tm_mday = atoi(strtokX(NULL,","));
			GPSLogp->recTime->tm_hour = atoi(strtokX(NULL,":"));
			GPSLogp->recTime->tm_min = atoi(strtokX(NULL,":"));
			GPSLogp->recTime->tm_sec = atoi(strtokX(NULL,","));
			if (_strnicmp(p=strtokX(NULL,","),"Tokyo",5)==0 ) {
				GPSLogp->datTyp |= DAT_TKY;
			} else if (_strnicmp(p,"WGS84",5)==0 ) {
				GPSLogp->datTyp |= DAT_WGS;
			} else if (_strnicmp(p,"WGS 84",6)==0 ) {
				GPSLogp->datTyp |= DAT_WGS;
			} else {
				GPSLogp->datTyp |= DAT_OTH;
			}
			if ((p=strtokX(NULL,","))!=NULL) {
				if ((GPSLogp->head = atof(p))>360)
					GPSLogp->datTyp |= DAT_HED;
			}
			return 1;
			break;	/* never executed */
		default:
			GPSLogp->thread = 1;
			if ((nRec == 0) && (strstr(p,"�n�_�ԍ�")!=NULL)) {
				xfseek(rFile,0L,SEEK_SET);
				newType = "csvg";
				return XErr = -2;
			}
			break;
		}	/* end switch */
	}
	return 0;	/* return EOF */
}

static int numhed,numpnt;

static char *makAngStrY(double loc, char *buf)
{
	int sgn = (int)loc;
	int deg,min,sec;
	long IAng;

	IAng = (long)(fabs(loc)*36000+0.5);

	deg = (int)(IAng/36000);
	min = (int)((IAng%36000)/600);
	sec = (int)(IAng%600);

	if (sgn >0)
		sprintf(buf,"%3d.%02d%03d",deg,min,sec);
	else
		sprintf(buf,"%4d.%02d%03d",-deg,min,sec);

	return buf;
}

void out_CSV(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
	if (nRec == 0) {
		fprintf(out,";���,����,�ܓx,�o�x,�W��,���t,����,���n�n,����,�p,���,�A�C�R��\n");
		numhed = 0;
		lat = lon = -9999.9;
	}
	if (GPSLogp->thread) {
		numpnt = 1;
		fprintf(out,"\n;�w�b�_,����,���F(R),���F(G),���F(B),����,���X�^�C��\n");
		fprintf(out,"H%d,%s,255,0,0,2,1\n",++numhed,titleBuf);
		fprintf(out,";���,����,�ܓx,�o�x,�W��,���t,����,���n�n,����,�p\n");
	} else if ((position->lat == lat) && (position->lon == lon) && (GPSLogp->thread == 0))
		return;	/* skip meaningless point */
	lat = position->lat;	/* save last point */
	lon = position->lon;
	distSum += GPSLogp->dist;

	/*T1,No. 1,36.15110,137.41000,1566.0,1997-02-26,13:23:44,Tokyo,-9999.9,-9999.9*/
	fprintf(out,"T%d,No. %d,%s,%s,%6.1f,%d-%02d-%02d,%02d:%02d:%02d,%s,%7.1f,%7.1f\n"
		,numhed
		,numpnt++
		,makAngStrY(lat,latJBuf)+1
		,makAngStrY(lon,lonJBuf)
		,(GPSLogp->datTyp & DAT_ALT)? position->alt:-9999.9
		,GPSLogp->recTime->tm_year+1900,GPSLogp->recTime->tm_mon+1,GPSLogp->recTime->tm_mday
		,GPSLogp->recTime->tm_hour,GPSLogp->recTime->tm_min,GPSLogp->recTime->tm_sec
		,((GPSLogp->outTyp & DAT_DTM_MSK) & DAT_OTH ) ? GPSLogp->mpp->datum_name: ((GPSLogp->outTyp & DAT_TKY)?"Tokyo":"WGS84")
		,(GPSLogp->datTyp & DAT_HED) ? GPSLogp->head:-9999.9
		,-9999.9 );

	GPSLogp->thread = 0;
	nRec++;
}
