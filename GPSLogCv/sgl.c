/* sgl.c - part of gpslogcv
mailto:imaizumi@nisiq.net
http://homepage1.nifty.com/gigo/

  SONY GTrex .sgl

History:
  2000/11/26 V0.26b fill empty title with source filename.
  2000/07/02 V0.24 initial release
*/
#ifdef WIN32
#include <math.h>
#include <string.h>
#include <mbstring.h>
#include <fcntl.h>
#include <io.h>
#include <direct.h>

#include "gpslogcv.h"
#include "nyp.h"

//#define DEBUG

#pragma pack(1)

typedef struct SGLEntry_t {
	int	unknown0;
	int	unknown1;
	int	unknown2;
	int	unknown3;
	_int64 t0;
	_int64 t1;
	int dist;
	_int64 numrec;
	int stab0;
} SGLEntry;

typedef struct SGLHead_t {
	SGLEntry ent0;
	_int64	 numrec;
	int		 stab0;
	int		 stab1;
	SGLEntry ent1;
} SGLHead;

/* 32 byte record  */
typedef struct SGLRecord_t {
	short int	recTyp;
	short int	subTyp;
	dword  ILon;	/* long	 dswab(value)/ASCALE Deg. */
	dword  ILat;	/* long	 dswab(value)/ASCALE Deg. */
	dword  stab1;	/* float dswab(value) ???		  */
	dword  speed;	/* long	 dswab(value)		 m/h  */
	time_t IrecTime;/* time_t dswab(value) Sec. UTC   */
	dword  IAng;	/* long	 dswab(value)/ASCALE Deg. */
	dword  stab2;	/* float dswab(value) ???	      */
} SGLRecord;
#pragma pack()

#define UNKNOWN1 0x00464F78
#define UNKNOWN2 0x00464608

#define USERNAME "Default User"
#define TITLE	 "GPSÉçÉO%06d"

char headBuf[0x70];
SGLHead *hp = (SGLHead *)&headBuf[0];

long fsiz;
time_t t;

static SGLRecord rec;
static _int64 numRec;
static double distSum;

#ifdef DEBUG
static SGLRecord rec0;
#endif

int in_SGL(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
	if (readRec == 0) {
		int i;
		char usrName[32];

		if (_setmode( _fileno(rFile), _O_BINARY )== -1 ) {
			perror( "can't set binary mode." );
			return -1;	/* error with errno */
		}

		fseek(rFile,0,SEEK_END);
		if (ftell(rFile) < 0x70) {
			fprintf(stderr, "too short." );
			return -1;	/* error */
		}

		fseek(rFile,0,SEEK_SET);
		xfread(headBuf,sizeof(headBuf),1,rFile);

#ifdef DEBUG
		t = (time_t)((double)hp->ent0.t0/1590728.62 - 2938117119054.8 + .5);
		*GPSLogp->recTime = *localtime(&t);

		t = (time_t)((double)hp->ent0.t1/1590728.62 - 2938117119054.8 + .5);
		*GPSLogp->recTime = *localtime(&t);
#endif
		if (memcmp(&hp->ent0,&hp->ent1,sizeof(SGLEntry)) !=0 ) {
			fprintf(stderr, "Illegal header !" );
			return -1;	/* error */
		}
		/* 0x70 */

		xfread(&i,sizeof(int),1,rFile);
		xfread(usrName,i,1,rFile);
		xfread(&i,sizeof(int),1,rFile);
		xfread(titleBuf,i,1,rFile);

		numRec = hp->numrec;
	}
	while (readRec < numRec) {
		xfread(&rec,sizeof(rec),1,rFile);
		readRec++;
		if (((rec.recTyp & 0x4) == 0) || (rec.stab1!=0) || (rec.stab2!=0) ){
			GPSLogp->thread++;
			continue;
		}
/*		0:			(0,45,
		4:	 valid	(0,
		80: Mark	(0,a00e,
*/
#ifdef DEBUG
		if (rec0.recTyp != rec.recTyp)
			printf("rec.recTyp diff\n");
		if (rec0.subTyp != rec.subTyp)
			printf("rec.subTyp diff\n");
		if ((rec0.stab1 != rec.stab1) ||
			(rec0.stab2 != rec.stab2))
			printf("stab diff\n");
		rec0 = rec;
#endif
		position->lat = (double)rec.ILat/ASCALE;
		position->lon = (double)rec.ILon/ASCALE;
		GPSLogp->head = (double)rec.IAng/ASCALE;
		GPSLogp->speed = (double)rec.speed/1000;
		{
			struct tm *tmp;
			if ((tmp = localtime(&rec.IrecTime))!=NULL) /* use local time but result is UTC */
				*GPSLogp->recTime = *tmp;
			else
				return -1;
		}

#if 0
		printf("%d,0,%d/%d/%d,%02d:%02d:%02d,%d,%d,%d,%s,%s,%s,%11.9f,%11.9f,%d\n"
			,readRec
			,GPSLogp->recTime->tm_year+1900,GPSLogp->recTime->tm_mon+1,GPSLogp->recTime->tm_mday
			,GPSLogp->recTime->tm_hour,GPSLogp->recTime->tm_min,GPSLogp->recTime->tm_sec
			,0/*(int)(distSum/1000+.5)*/
			,(int)(GPSLogp->speed+.5)
			,(int)(GPSLogp->head+.5)
			,titleBuf
			,makAngStrJ(position->lat, latJBuf, "ñkà‹","ìÏà‹")
			,makAngStrJ(position->lon, lonJBuf, "ìååo","êºåo")
			,position->lat*TORAD
			,position->lon*TORAD
			,((GPSLogp->datTyp & DAT_ALT) && (position->alt > -9997))? round(position->alt):0 );
#endif
		return 1;
	}
	return 0;
}

static dword wrtRec;

#define TMULT 1590728.62
#define TOFFS 2938117151454.8

void out_SGL(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
	if (nRec == 0) {
		int i;

		if (_setmode( _fileno(out), _O_BINARY )== -1 ) {
			perror( "can't set binary mode." );
			XErr++;
		}
		xfseek(out,0x70,SEEK_SET);
		i = strlen(USERNAME)+1;
		xfwrite(&i,sizeof(i),1,out);
		xfwrite(USERNAME,i,1,out);
		if (titleBuf[0]=='\0') {
			char c;
			char *p = inNamep;
			char *q = titleBuf;
			while((c = *p++)!='\0') {
				if (c == '.')
					c = '_';
				*q++ = c;
			}
		}
		i = strlen(titleBuf)+1;
		xfwrite(&i,sizeof(i),1,out);
		xfwrite(titleBuf,i,1,out);
		memset( hp, 0, sizeof(SGLHead) );
		hp->ent0.t0 = (_int64)(((double)mktime(GPSLogp->recTime) + TOFFS)*TMULT);
		distSum = 0;
		wrtRec = 0;
	}
	distSum += GPSLogp->dist;

	memset( &rec, 0, sizeof(rec) );

	rec.IrecTime = t = mktime(GPSLogp->recTime) ;	/* time_t Sec. UTC */
	if (GPSLogp->thread) {
		rec.recTyp = 0;
		rec.subTyp = 0x45;
		xfwrite(&rec,sizeof(rec),1,out);
		wrtRec++;
	}
	rec.recTyp = 4;
	rec.subTyp = 0;
	rec.ILat = (dword)(position->lat*ASCALE);
	rec.ILon = (dword)(position->lon*ASCALE);
	rec.stab1 = 0;
	rec.speed = (dword)(GPSLogp->speed*1000);	/* m/h	*/
	rec.IAng = (dword)(GPSLogp->head*ASCALE);	/* long	 value/ASCALE Deg. */
	rec.stab2 = 0;
	xfwrite(&rec,sizeof(rec),1,out);
	wrtRec++;

	GPSLogp->thread = 0;
	nRec++;
}

int cls_SGL(FILE *out)
{
	hp->ent0.unknown1 = UNKNOWN1;
	hp->ent0.unknown2 = UNKNOWN2;
	hp->ent0.t1 = (_int64)(((double)t + TOFFS)*TMULT);
	hp->ent0.numrec = wrtRec;
	hp->ent0.dist = (int)distSum;
	memcpy(&hp->ent1,&hp->ent0,sizeof(SGLEntry));
	hp->numrec = wrtRec;
	hp->stab0 = 0;
	hp->stab1 = 0;
	xfseek(out,0x0,SEEK_SET);
	xfwrite(hp,sizeof(SGLHead),1,out);
	xfseek(out,0,SEEK_END);
	fclose(out);
	return 0;
}
#else	/* DOS */
#include "gpslogcv.h"
int cls_SGL(FILE *out)
{
	fclose(out);
}
void out_SGL(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
	fprintf(stderr,"Error - in_SGL not supported on DOS Binary\n");
	exit(1);
}
int in_SGL(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
	fprintf(stderr,"Error - out_SGL not supported on DOS Binary\n");
	exit(1);
}
#endif
