/* mps.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

  MapSource .MPS

History:
  2000/02/05 V0.18 beta2 
  2000/02/04 V0.18 initial 
*/
#include <string.h>
#include <fcntl.h>
#include <io.h>

#include "gpslogcv.h"

//#define DBG_MPS

#pragma pack(1)
typedef struct {
	byte dmy_0;
	long dmy1;
	long numTrk;
} TrkHead;

typedef struct {
	long lat;
	long lon;
	byte altExist;
	double alt; /* altitude in meters  */
	/*float dpth;*/ /* depth in meters ? */
	byte valDate; /* valid date */
	time_t t;
	byte dmy_1;
	long dmy3;
	long dmy4;
} TrkPoint;
#pragma pack()

#ifdef DBG_MPS
static TrkPoint trkRec0;
#endif
static TrkPoint trkRec;
static TrkHead trkHed;
static dword reclen;
static byte rectyp;
static char *magicStr = "MsRcd";
static char buf[32];

int in_MPS(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
#ifdef DBG_MPS
	int i = 0;
	int j = 0;
#endif
	char *p;
	
    if (nRec == 0) {
        if (_setmode( _fileno(rFile), _O_BINARY )== -1 ) {
            perror( "*Internal Error - can't set binary mode.\n" );
            return -1;	/* error with errno */
        }
		xfread(buf,sizeof(char),6,rFile);
		if (strncmp(buf,magicStr,6) != 0) {
			fprintf(stderr,"%s - *Error - MPS ID Mismatch.\n",inNamep);
			return XErr = -1;
		}
		rectyp = '\0';
	}
	while(rectyp != 'T'){
		xfread(&reclen,sizeof(reclen),1,rFile);
		xfread(&rectyp,sizeof(rectyp),1,rFile);
#ifdef DBG_MPS
		fprintf(stderr,"%02d,%06lx, %04x, '%c'\n",i+=1,ftell(rFile)-sizeof(reclen)-sizeof(rectyp),reclen,rectyp);
#endif
		if (rectyp == 'V')
			return 0;
		if (rectyp != 'T') {
			xfseek(rFile,reclen,SEEK_CUR);
			continue;
		}
		for(p = titleBuf; (*p++ = (char)fgetc(rFile))!=0 ; );
		reclen -= p-titleBuf;
		xfread(&trkHed,sizeof(trkHed),1,rFile);
		reclen -= sizeof(trkHed);
#ifdef DBG_MPS
		fprintf(stderr,"%s\n",titleBuf);
#endif
		GPSLogp->thread = 1;
	}
#ifdef DBG_MPS
	fprintf(stderr,"%08lx, ",ftell(rFile));
#endif
	xfread(&trkRec,sizeof(trkRec),1,rFile);
	position->lat = (double)trkRec.lat*(360.0/4294967296.0);
	position->lon = (double)trkRec.lon*(360.0/4294967296.0);
	if (trkRec.altExist) {
		GPSLogp->datTyp |= DAT_ALT;
		position->alt = trkRec.alt;
	}
	if (trkRec.t) {
		GPSLogp->datTyp |= DAT_TIM;
		*GPSLogp->recTime = *gmtime(&trkRec.t);
	}
#ifdef DBG_MPS
	if (   (trkRec.altExist != trkRec0.altExist)
		|| (trkRec.valDate != trkRec0.valDate)
		|| (trkRec.dmy_1 != trkRec0.dmy_1)
		|| (trkRec.dmy3 != trkRec0.dmy3)
		|| (trkRec.dmy4 != trkRec0.dmy4) )
		trkRec0 = trkRec;

	fprintf(stderr,"  %02d,%06lx, %04x,%8.5f,%9.5f\n",j+=1,ftell(rFile),trkRec.t,(double)trkRec.lat*(360.0/4294967296.0),(double)trkRec.lon*(360.0/4294967296.0));
#endif
	reclen -= sizeof(trkRec);
	if (reclen <= 0)
		rectyp = '\0';
    return 1;
}
