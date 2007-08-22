/* fx.c - part of gpslogcv
mailto:imaizumi@nisiq.net
http://homepage1.nifty.com/gigo/

  gar2ws .fx
  
History:
2002/03/06 fix .log.fx 8byte record
2001/12/07 wwp 
2001/06/07 garlog 02 
2001/05/13 gar2ws 1.04 
2001/05/04 initial 
*/
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <mbctype.h>
#include <sys/utime.h>
#include <sys/stat.h>

#include "gpslogcv.h"

/* L001 - Link Protocol 1 */
enum {
	Pid_Command_Data	= 10,
	Pid_Xfer_Cmplt		= 12,
	Pid_Date_Time_Data	= 14,
	Pid_Position_Data	= 17,
	Pid_Prx_Wpt_Data	= 19,
	Pid_Records			= 27,
	Pid_Rte_Hdr			= 29,
	Pid_Rte_Wpt_Data	= 30,
	Pid_Almanac_Data	= 31,
	Pid_Trk_Data		= 34,
	Pid_Wpt_Data		= 35,
	Pid_Pvt_Data		= 51,
	Pid_Rte_Link_Data	= 98,
	Pid_Trk_Hdr			= 99
};

/* packet offset */
enum {
    P_Type,                             /* 0    */
    P_len,                              /* 1    */
    P_lat0, P_lat1, P_lat2, P_lat3,     /* 2-5  */
    P_lon0, P_lon1, P_lon2, P_lon3,     /* 6-9  */
    P_tim0, P_tim1, P_tim2, P_tim3,     /* 10-13*/
    P_alt0, P_alt1, P_alt2, P_alt3,     /* 14-17*/
    P_dpt0, P_dpt1, P_dpt2, P_dpt3,     /* 18-21*/
    P_301_trk,                          /* 22   */
    P_stb1,
    P_stb2,
    P_stb3,
};

#pragma pack(1)
typedef struct
{
	long lat; /* latitude in semicircles */
	long lon; /* longitude in semicircles */
} Semicircle_Type;

//7.5.16. D300_Trk_Point_Type
// Example products: all products unless otherwise noted.
typedef struct
{
	Semicircle_Type posn;	// position
	time_t time;			// time
	boolean new_trk;		// new track segment?
} D300_Trk_Point_Type;
//The "time" member provides a timestamp for the track log point.  This
// time is expressed as the number of seconds since 12:00 AM on January 1st,
// 1990.  When true, the "new_trk" member indicates that the track log
// point marks the beginning of a new track log segment.
typedef struct
{
    Semicircle_Type posn;   // position
    time_t time;          // time */
    float alt;              // altitude in meters
    float dpth;             // depth in meters
    boolean new_trk;        // new track segment?
} D301_Trk_Point_Type;
//
// The "time" member provides a timestamp for the track log point.  This
// time is expressed as the number of seconds since UTC 12:00 AM on
// December 31 st , 1989.
//
// The `alt' and `dpth' members may or may not be supported on a given
// unit.  A value of 1.0e25 in either of these fields indicates that this
// parameter is not supported or is unknown for this track point.
//
// When true, the "new_trk" member indicates that the track log point marks
// the beginning of a new track log segment.
//
#pragma pack()

/* Track Log Record */
#define TRK_POINT_SIZE_D300	15
#define TRK_POINT_SIZE_D301	26

//#define DEBUG

#define BYTE byte
/* record size macro */
#define GET_SIZE(rec) (rec[1]&0x7f)

static int useType;

#ifdef DEBUG
static int dif;
static int zero[20];

static byte p_2nd[32];
static int irecs[32];
static int orecs[32];
#endif

size_t yfread(BYTE trkBuf[],BYTE rBuf[],size_t size, size_t nobj, FILE /* far */ *file)
{
	unsigned u;

	if (file != NULL) {
		if (useType) {
#ifdef DEBUG
			rBuf[1] = (BYTE)(size-1);
#endif
			return fread(trkBuf,size,nobj,file);	/* V1.03はそのまま*/
		}
		fread(&rBuf[1],sizeof(BYTE),1,file);
		fread(&rBuf[2],sizeof(BYTE),GET_SIZE(rBuf),file);
	}
	/* V1.04形式のレコードをV1.03形式に展開する。 */
	if (GET_SIZE(rBuf) > 0x14) {	/* Hdr on V1.04 */
		memcpy(&trkBuf[1],&rBuf[1],rBuf[1]+1);
		trkBuf[0] = Pid_Trk_Hdr;
		return nobj;
	}
	trkBuf[0] = Pid_Trk_Data;
	if (GET_SIZE(rBuf) <= 5)
		u = 0x0C;
	else
		u = 0x14;
	switch(GET_SIZE(rBuf)) {
	case 0x0a:
		rBuf[12] = rBuf[13] = rBuf[14] = rBuf[15] = 0xff;
		/* no break */
	case 0x0e:
		trkBuf[P_dpt3] = 0x69;	/* dpt */
		trkBuf[P_dpt2] = 0x04;
		trkBuf[P_dpt1] = 0x59;
		trkBuf[P_dpt0] = 0x51;
		trkBuf[P_alt3] = rBuf[11];	/* alt */
		trkBuf[P_alt2] = rBuf[10];
		trkBuf[P_alt1] = rBuf[9];
		trkBuf[P_alt0] = rBuf[8];
		trkBuf[P_tim3] = rBuf[15];
		trkBuf[P_tim2] = rBuf[14];
		trkBuf[P_tim1] = rBuf[13];
		trkBuf[P_tim0] = rBuf[12];
		trkBuf[P_lon3] = rBuf[7];
		trkBuf[P_lon2] = rBuf[6];
		trkBuf[P_lon1] = rBuf[5];
		trkBuf[P_lon0] = 0x00;
		trkBuf[P_lat3] = rBuf[4];
		trkBuf[P_lat2] = rBuf[3];
		trkBuf[P_lat1] = rBuf[2];
		trkBuf[P_lon0] = 0x00;
		break;
	case 0x07:
		trkBuf[P_alt3] ^= rBuf[8];	/* alt */
		/* no break */
	case 0x06:
		trkBuf[P_alt2] ^= rBuf[7];
		trkBuf[P_alt1] ^= rBuf[6];
		trkBuf[P_alt0] ^= rBuf[5];
		/* no break */
	case 0x03:
		trkBuf[P_tim0] ^= rBuf[4];	/* lower byte of time */
		trkBuf[P_lon1] ^= rBuf[3];	/* lower 2nd of posn.lon */
		trkBuf[P_lon0] = 0; 
		trkBuf[P_lat1] ^= rBuf[2];	/* lower 2nd of posn.lat */
		trkBuf[P_lat0] = 0;
		break;
	case 0x09:
		trkBuf[P_alt3] ^= rBuf[10];	/* alt */
        /* no break */                  /* [2002/03/05] */
    case 0x08:                          /* [2002/03/05] */
		trkBuf[P_alt2] ^= rBuf[9];
		trkBuf[P_alt1] ^= rBuf[8];
		trkBuf[P_alt0] ^= rBuf[7];
		/* no break */
	case 0x05:	/* diffed D300 */
		trkBuf[P_tim0] ^= rBuf[6];	/* lower byte of time */
		trkBuf[P_lon1] ^= rBuf[5];	/* lower word of posn.lon */
		trkBuf[P_lon0] ^= rBuf[4];
		trkBuf[P_lat1] ^= rBuf[3];	/* lower word of posn.lat */
		trkBuf[P_lat0] ^= rBuf[2];
		break;
	case 0x10:
		rBuf[1] += 4;
		rBuf[P_dpt3] = 0x69;	/* dpt */
		rBuf[P_dpt2] = 0x04;
		rBuf[P_dpt1] = 0x59;
		rBuf[P_dpt0] = 0x51;
		/* no break; */
	case 0x0c:
	case 0x14:
	default:
		u = GET_SIZE(rBuf);
		memcpy(&trkBuf[2],&rBuf[2],u);
		break;
	}
	trkBuf[1] = (BYTE) ++u;
	trkBuf[++u] = (BYTE)((rBuf[1]&0x80) ? 1:0);
	if (trkBuf[1] == 0x15) {
		trkBuf[++u] = 0xff;
		trkBuf[++u] = 0xff;
		trkBuf[++u] = 0xff;
		trkBuf[1] = 0x18;
	}
	return nobj;
}

static const BYTE altinvalid[4] = { 0x51, 0x59, 0x04, 0x69 };
static byte p_1st[32];
static byte trackdat[32];

BYTE skipped;
long lastTime = -2;
#define bwrite(b,s,n) fwrite(b,s,n,out)
#define WORD unsigned short

/*	p_1st(V1.04) <- trkRec(V1.03) */
void yfwrite(BYTE savBuf[], BYTE rcvBuf[],FILE *out)
{
	int i,j;

	/* input savBuf: last packet, rcvBuf: received packet */
	/* output savBuf: compacted record */
	if (rcvBuf[0] == Pid_Trk_Hdr) {
		/*	データサイズ24byteのパケットへ拡張する */
		WORD size;				/* [2001/03/05] */

		savBuf[0] = rcvBuf[0];
		savBuf[1] = 24;
		savBuf[2] = rcvBuf[2];
		savBuf[3] = rcvBuf[3];

		size = (WORD)rcvBuf[1];
		if( size >= 24 ){
			for(i=4; i<=24	; i++) savBuf[i] = rcvBuf[i];
		}else{
			for(i=4; i<=24	; i++) savBuf[i] = 0x20;			/* 空白で初期化 */
			for(i=1; i<=size; i++) savBuf[i+1] = rcvBuf[i+1];
		}
		savBuf[25] = 0x00;
		skipped |= 2;			/* ヘッダ有り */
		return;
	}
	if (*((long *)&rcvBuf[P_tim0]) < lastTime) {
		skipped |= 1;			/* スキップデータ有り */
		return;
	} else {
		lastTime = *((long *)&rcvBuf[P_tim0]);
		if (skipped & 2) {		/* ヘッダのflush必要? */
			bwrite(&savBuf[1],sizeof(BYTE),trackdat[1]+1);
			skipped &= ~2;
		}
	}
	if (rcvBuf[1] == 0x0D)
		j = savBuf[1] = 0x0C;	/* D300 */
	else
		j = savBuf[1] = 0x14;	/* D301 */
	if (skipped) {			/* スキップしたデータがある時は強制的にnew_trk=1 */
		rcvBuf[j+2] = 1;
		skipped = 0;
	}

	for (i = 2; i < savBuf[1]+2;i++)
		savBuf[i] ^= rcvBuf[i];
	if (((savBuf[4] | savBuf[5] | savBuf[8] | savBuf[9]	| savBuf[11] | savBuf[12] | savBuf[13]) == 0) /*&& (rcvBuf[j+2]==0)*/){
		if ((savBuf[1] == 0x0C) || ((savBuf[18] | savBuf[19] | savBuf[20] | savBuf[21])==0)) {
			/* make diffed record */
			if ((rcvBuf[P_lat0] == 0) && (rcvBuf[P_lon0] == 0)) {
				savBuf[1] = 3;
				savBuf[2] = savBuf[P_lat1];
				savBuf[3] = savBuf[P_lon1];
				savBuf[4] = savBuf[P_tim0];
			} else {
				savBuf[1] = 5;
				/* savBuf[2] = savBuf[2]; */
				/* savBuf[3] = savBuf[3]; */
				savBuf[4] = savBuf[P_lon0];
				savBuf[5] = savBuf[P_lon1];
				savBuf[6] = savBuf[P_tim0];
			}
			if (rcvBuf[1] != 0x0d) {
				savBuf[savBuf[1]++ +2] = savBuf[P_alt0];
				savBuf[savBuf[1]++ +2] = savBuf[P_alt1];
				savBuf[savBuf[1]++ +2] = savBuf[P_alt2];
				if (savBuf[P_alt3] != 0)
					savBuf[savBuf[1]++ +2] = savBuf[P_alt3];
			}
		}
	} else if ((rcvBuf[1] == 0x18)&&(rcvBuf[P_lat0] == 0)&&(rcvBuf[P_lon0] == 0)&&( *((long *)&rcvBuf[P_dpt0]) == *((long *)altinvalid))) {
		savBuf[1] = 0x0a;				/* saved Track */
		savBuf[2] = rcvBuf[P_lat1];
		savBuf[3] = rcvBuf[P_lat2];
		savBuf[4] = rcvBuf[P_lat3];
		savBuf[5] = rcvBuf[P_lon1];
		savBuf[6] = rcvBuf[P_lon2];
		savBuf[7] = rcvBuf[P_lon3];
		savBuf[8] = rcvBuf[P_alt0];
		savBuf[9] = rcvBuf[P_alt1];
		savBuf[10] = rcvBuf[P_alt2];
		savBuf[11] = rcvBuf[P_alt3];
		if ((rcvBuf[P_tim0] != 0xff)||(rcvBuf[P_tim1] != 0xff)||(rcvBuf[P_tim2] !=0xff)||(rcvBuf[P_tim3] != 0xff)) {
			savBuf[1] = 0x0e;	/* ACTIVE Track */
			savBuf[12] = rcvBuf[P_tim0];
			savBuf[13] = rcvBuf[P_tim1];
			savBuf[14] = rcvBuf[P_tim2];
			savBuf[15] = rcvBuf[P_tim3];
		} else {
			savBuf[1] = 0x0a;
		}
	} else {
		if (rcvBuf[1] == 0x18)
			rcvBuf[1] = 0x15;
		memcpy(&savBuf[1],&rcvBuf[1],rcvBuf[1]--);
	}
	if (rcvBuf[j+2])
		savBuf[1] |= 0x80;
	bwrite(&savBuf[1],sizeof(BYTE),GET_SIZE(savBuf)+1);	/* 差分データ書き出し */
	memcpy(&savBuf[1],&rcvBuf[1],GET_SIZE(rcvBuf)+1);	/* 最終データを保存 */
}

static size_t size = TRK_POINT_SIZE_D300;
static unsigned short record;

static char wsname[17];
static byte lastdat[32];

int in_FX(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
	D301_Trk_Point_Type *tp;
	time_t tg;
	
    if (nRec == 0) {
        if (_setmode( _fileno(rFile), _O_BINARY )== -1 ) {
            perror( "*Internal Error - can't set binary mode.\n" );
            return -1;	/* error with errno */
        }
		xfread(p_1st,sizeof(char),4,rFile);
		if (strncmp((char *)p_1st,"#!ws",4) != 0) {
			fprintf(stderr,"%s - *Error - WS ID Mismatch.\n",inNamep);
			return XErr = -1;
		} else {
			short t[2];
			long rSize;
			struct _utimbuf modTimBuf;
			struct _stat statbuf;
			struct tm atime;
			xfseek(rFile,0x6cl,SEEK_SET);
			xfread((void *)&rSize,sizeof(rSize),1,rFile);
			if (rSize == 0L) {
				fprintf(stderr,"%s - *Error - Empty file.\n",inNamep);
				return XErr = -1;
			}
			if ( (rSize+=0x80) != _filelength(_fileno(rFile))) {
				if (_chsize(_fileno(rFile),rSize)!=0) {
					perror( "*Internal Error - can't trucate.\n" );
					return -1;	/* error with errno */
				}
			}
			fseek(rFile,0x74,SEEK_SET);
			fread((void *)t,sizeof(short),2,rFile);
			atime.tm_sec = (t[0]&31)*2; t[0] >>= 5;
			atime.tm_min = t[0]&63; t[0] >>= 6;
			atime.tm_hour = t[0]&31;
			atime.tm_mday = t[1]&31; t[1] >>= 5;
			atime.tm_mon = (t[1]&15)-1; t[1] >>= 4;
			atime.tm_year = t[1] + 2000 - 1900;
			atime.tm_isdst = 0;
			modTimBuf.actime = modTimBuf.modtime = mktime(&atime) /*- _timezone*/;
			/*atime = *localtime(&modTimBuf.modtime);*/
			if (_fstat(_fileno(rFile),&statbuf) == -1) {
				perror( "*Internal Error - can't get _stat.\n" );
				return -1;	/* error with errno */
			}
			if (statbuf.st_mtime != modTimBuf.modtime) {
				if( _futime(_fileno(rFile), &modTimBuf ) == -1 ) {
					perror( "*Internal Error - can't set time stamp.\n" );
					return -1;	/* error with errno */
				}
			}
		}
		xfseek(rFile,0x80l,SEEK_SET);
		xfread(p_1st,sizeof(char),4,rFile);
		xfread(&record,sizeof(char),2,rFile);
		if (strncmp((char *)p_1st,"#TRK",4) == 0) {
			useType = 1;
			xfread(p_1st,sizeof(byte),1,rFile);
			if (p_1st[0] == Pid_Trk_Hdr)
				size = TRK_POINT_SIZE_D301;
			else
				size = TRK_POINT_SIZE_D300;
		} else if (strncmp((char *)p_1st,"#DTK",4) == 0) {
			useType = 0;
		} else {
			xfseek(rFile,0x40l,SEEK_SET);
			xfread(wsname,1,16,rFile);
			wsname[16] = '\0';
			if ((newType = strchr(wsname,'.'))!=NULL) {
				++newType;
				xfseek(rFile,0x80l,SEEK_SET);
				return XErr = -2;
			}
#if 0
			if (p_1st[0]==MAGIC_TDF) {
				xfseek(rFile,0x80l,SEEK_SET);
				newType = "tdf";
				return XErr = -2;
			}
#endif
			fprintf(stderr,"%s - *Error - Unknown file Type.\n",inNamep);
			return XErr = -1;
		}
		memset(lastdat,0,sizeof(lastdat));
	}
	while((record--)&&(!feof(rFile))) {
		yfread(trackdat,p_1st,sizeof(byte),size,rFile);
#ifdef DEBUG
		irecs[p_1st[1]&0x7f]++;
#endif
		if (trackdat[0] == Pid_Trk_Hdr) {
			memcpy(titleBuf,&trackdat[4],trackdat[1]-2);
			continue;
		}
		tp = (D301_Trk_Point_Type *)&trackdat[2];
		GPSLogp->thread = trackdat[trackdat[1]+1];
		switch(trackdat[1]) {
		case 0x18:	/* D301 */
			if (tp->alt < 1E25)
				GPSLogp->datTyp |= DAT_ALT;
			position->alt = tp->alt;
			if (tp->dpth < 0.99999E25)
				position->alt += tp->dpth;
			GPSLogp->thread = trackdat[trackdat[1]-2];
			/* no break */
		case 0x0D:	/* D300 */
			position->lat = (double)tp->posn.lat/0x7FFFFFFF*180.0;
			position->lon = (double)tp->posn.lon/0x7FFFFFFF*180.0;
			tg = tp->time;
			tg += 631033200; /*631119600 1990-Jan-01 00:00:00 */
			*GPSLogp->recTime = *localtime(&tg);
#ifdef DEBUG
//			useType = 0;
//			if (useType == 0 ){
			memcpy(p_1st,trackdat,trackdat[1]+3);
			memcpy(p_2nd,lastdat,trackdat[1]+3);
			yfwrite(p_2nd,p_1st);
			yfread(lastdat,p_2nd,sizeof(byte),size,NULL);
			if ((lastdat[0] == Pid_Trk_Data) && (lastdat[1]>0x15))
				trackdat[0x17] = trackdat[0x18] = trackdat[0x19] = 0xff;

			if (memcmp(trackdat,lastdat,trackdat[1]+2)!=0) {
				int i;
				fprintf(stderr,"conversion error.\n");
				for (i = 0 ; i < lastdat[1]+1; i++)
					if (lastdat[i] != trackdat[i])
						lastdat[i] = trackdat[i];
			}
			orecs[p_2nd[1]&0x7f]++;
//			}
#endif
			return 1;
		default:
			fprintf(stderr,"%02x at 0x%lx\n",p_1st[1],ftell(rFile));
			break;
		}
	}
#ifdef DEBUG
	{
		int i,rec=0,sum=0;
		for (i=0;i<32;i++) {
			if (orecs[i]) {
				rec += orecs[i];
				sum += orecs[i]*i;
				fprintf(stderr,"0x%02x: %4d\n",i,orecs[i]);
			}
		}
		fprintf(stderr,"total record %d, size:%4d(0x%04x) byte\n",rec,sum,sum);
		memset(orecs,0,sizeof(orecs));
	}
#endif
    return 0;
}

static long l3round(long l)
{
	if (l<0)
		l-=0x80;
	else
		l+=0x80;
	return l & ~0xff;
}
static unsigned short wRec;

void out_FX(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
	D301_Trk_Point_Type *tp = (D301_Trk_Point_Type *)&p_1st[2];

	if (nRec == 0) {
		if (_setmode( _fileno(out), _O_BINARY )== -1 ) {
			perror( "can't set binary mode." );
			XErr++;
		}
		wRec = 0;
		xfseek(out,0x86L,SEEK_SET);
		if (GPSLogp->datTyp & DAT_ALT) {
			if (GPSLogp->thread && (titleBuf[0]=='\0')) {
				if (tp->time != -1)
					strcpy(titleBuf,"ACTIVE LOG");
				else {
					time_t t = time(NULL);
					strftime(titleBuf,10,"%d-%b-%y", localtime(&t));
				}
			}
		}
		memset(trackdat,0,sizeof(trackdat));
	}
	if (GPSLogp->thread && titleBuf[0]) {
		p_1st[0] = Pid_Trk_Hdr;
		p_1st[2] = 0x01;
		p_1st[3] = 0xff;
		memset(&p_1st[4],' ',21);
		strcpy((char *)&p_1st[4],titleBuf);
		p_1st[25] = '\0';
		p_1st[1] = 24;
		yfwrite(trackdat,p_1st,out);
		wRec++;
		titleBuf[0] = '\0';
	}
	p_1st[0] = Pid_Trk_Data;
	tp->posn.lat = (long)(position->lat*0x7FFFFFFF/180.0);
	tp->posn.lon = (long)(position->lon*0x7FFFFFFF/180.0);
	if (GPSLogp->datTyp & DAT_TIM)
		tp->time = mktime(GPSLogp->recTime) - 631033200;
	else
		tp->time = -1;
	if (GPSLogp->datTyp & DAT_ALT) {
		tp->alt = (float)position->alt;
		p_1st[1] = 0x18;
		tp->posn.lat = l3round(tp->posn.lat);
		tp->posn.lon = l3round(tp->posn.lon);
		memcpy((BYTE *)&(tp->dpth),altinvalid,sizeof altinvalid);
		tp->new_trk = (BYTE) (GPSLogp->thread ? 1:0);
	}else {
		p_1st[1] = 0x0d;
		p_1st[p_1st[1]+1] = (BYTE) (GPSLogp->thread ? 1:0);
	}

	yfwrite(trackdat,p_1st,out);
	wRec++;

	GPSLogp->thread = 0;
	nRec++;
}

extern char outName[];
#define MAXFNAME		16
#define MAXFINFO		24

static int cls_FX(FILE *out, char *ext, char *desc, byte *idhdr, int n)
{
	long lpos;
	int i,b = -1;
	char *p,buf[MAXFINFO];
    struct tm *tmp;
    time_t t;

	lpos = ftell(out)-0x80;		/* get file size */
	xfseek(out,0L,SEEK_SET);
	xfwrite("#!ws",4,1,out);	/* write header */
	for ( i = 4 ;i < 0x40 ; i+=sizeof b)
		xfwrite(&b,sizeof b,1,out);

	memset(buf,0, MAXFNAME);			/* file name */
	strncpy(buf,outName,8);			/* easy.. todo: check validity */
	if ( (p = strchr(buf,'.'))!= NULL)
		strcpy(p,ext);
	else
		strcat(buf,ext);
	xfwrite(buf,1, MAXFNAME,out);
	memset(buf,0,MAXFINFO);			/* description */
	strcpy(buf,desc);
	strncpy(&buf[12],outName,11);
	xfwrite(buf,1,MAXFINFO,out);
	*((long *)&buf[0]) = 0L;	/* physical location *INVALID* */
	*((long *)&buf[4]) = lpos;	/* size */
	*((word *)&buf[8]) = (BYTE) ((lpos+127)/128);	/* blocks */
	*((word *)&buf[0xa]) = 0x6;	/* attribute r/w */
	t = time(&t);				/* time */
	tmp = localtime(&t);
    *((word *)&buf[0xc]) = (word)(((tmp->tm_hour &  31) << 11) | ((tmp->tm_min &  63) <<  5) | ((tmp->tm_sec &  31) >>  1));
    *((word *)&buf[0xe]) = (word)((((tmp->tm_year - 100) & 127) << 9) | (((tmp->tm_mon + 1)  &  15) << 5) | ((tmp->tm_mday  &  31) << 0));
	*((long *)&buf[0x10]) = 0;	/* handler */
	*((long *)&buf[0x14]) = -1;	/* resource position */
	xfwrite(buf,1,24,out);		/* save rest of fent structure */
	/*--*/
	xfwrite(idhdr,n,1,out);
	xfseek(out,0,SEEK_END);
	fclose(out);
	return 0;
}

#pragma pack(1)
typedef struct {
	char	  magic[4];
	short int count;
} FXHDR;
#pragma pack()


int cls_FXL(FILE *out)
{
	FXHDR hdr;
	memcpy(&hdr.magic[0],"#DTK",4);
	hdr.count = wRec;
	return cls_FX(out,".log","GARMINTRK ",(byte *)"#DTK",sizeof(hdr));
}

#pragma pack(1)
typedef struct {
	char			ident[6];
	long			lat;
	long			lon;
	unsigned long	unused;
	char			cmnt[40];
	float			dst;
	unsigned char	smbl;
} D101_Wpt_Type;

typedef struct {
	char	  magic[2];
	short int version;
} WPHDR;
#pragma pack()

static struct tm tmbuf;

int in_WWP(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
	D101_Wpt_Type d101;
	struct tm *tmp;
	char *p;

	if (nRec == 0) {
		WPHDR hdr;

		if (_setmode( _fileno(rFile), _O_BINARY )== -1 ) {
			perror( "*Internal Error - can't set binary mode.\n" );
			return -1;	/* error with errno */
		}
		if (!(fread((char *)&hdr, sizeof(hdr),1,rFile) == 1 && hdr.magic[0] == 'W' && hdr.magic[1] == 'P' && hdr.version == 1)){
			fprintf(stderr,"Illegal format. \n");
			return -1;
		}
	}
	if (fread((char *)&d101, sizeof(d101),1,rFile) != 1)
		return 0;
	memcpy(idBuf,&d101.ident[0],6);
	idBuf[6] = '\0';
	position->lat = d101.lat * (180.0/0x7FFFFFFF);
	position->lon = d101.lon * (180.0/0x7FFFFFFF);
	if ((p =strstr(&d101.cmnt[0],"alt="))!=NULL) {
		position->alt = atof(p+4);
		GPSLogp->datTyp |= DAT_ALT;
	}
	descBuf[0] = '\0';
	if ((p =strstr(&d101.cmnt[0],"des="))!=NULL)
		strcpy(descBuf,p+4);
	texBuf[0] = '\0';
	if ((p =strstr(&d101.cmnt[0],"tex="))!=NULL)
		strcpy(texBuf,p+4);

	d101.unused += 631033200;
	if ((tmp =gmtime((time_t *)&d101.unused))!=NULL) {
		GPSLogp->recTime = tmp;
		GPSLogp->datTyp |= DAT_TIM;
	}
	else if ((d101.cmnt[4] == '/') && (atoi(&d101.cmnt[0])>1970)) {
//		GPSLogp->recTime = &tmbuf;
		GPSLogp->recTime->tm_year = atoi(strtokX(&d101.cmnt[0],"/"))-1900;
		GPSLogp->recTime->tm_mon = atoi(strtokX(NULL,"/"));
		GPSLogp->recTime->tm_mday = atoi(strtokX(NULL," "));
		GPSLogp->recTime->tm_hour = atoi(strtokX(NULL,":"));
		GPSLogp->recTime->tm_min = atoi(strtokX(NULL,":"));
		GPSLogp->recTime->tm_sec = atoi(strtokX(NULL," "));
		GPSLogp->datTyp |= DAT_TIM|DAT_JST;
		if ((p=strtok(NULL,""))!=NULL)
			strcpy(titleBuf,p);
	}
	return 1;
}

void out_WWP(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
	char *p, *q;
	D101_Wpt_Type d101;

	if (nRec == 0) {
		WPHDR hdr;

		if (_setmode( _fileno(out), _O_BINARY )== -1 ) {
			perror( "can't set binary mode." );
			XErr++;
		}
		xfseek(out,0x80L,SEEK_SET);

		hdr.magic[0] = 'W'; hdr.magic[1] = 'P'; hdr.version = 1;
		xfwrite((char *)&hdr, sizeof(hdr),1,out);
	}
	if (idBuf[0] == '\0') {
		sprintf(&d101.ident[0],"%06d",nRec);
	} else {
		strncpy(&d101.ident[0],idBuf,6);
	}
	d101.lat = (long)(position->lat / (180.0/0x7FFFFFFF));
	d101.lon = (long)(position->lon / (180.0/0x7FFFFFFF));
	/* mktimeはローカル時刻のつもりで変換する */
	d101.unused = mktime(GPSLogp->recTime)  - timezone - 631033200;
	memset(&d101.cmnt[0],' ',40);
	if (GPSLogp->datTyp & DAT_ALT)
		sprintf(&d101.cmnt[0],"alt=%d",(int)round(position->alt));
	p = NULL;
	if (descBuf[0]!='\0') {
		p = descBuf;
	} else if (texBuf[0]!='\0') {
		p = texBuf;
	}
	if (p!=NULL) {	/* ポイント名がある */
		int l=6;
		strcat(d101.cmnt," des=");
		for(q=d101.cmnt;*q;q++) {};	/* EOLを探す */
		do {
			if (_ismbblead(*p)) {	/* 2バイトコードの先頭? */
				if (((--l)<1) || !_ismbbtrail(*(p+1)) )	/* 空きが無いもしくは無効なコード */
					break;
				*q++ = *p++;
			}
			*q++ = *p++;
		} while(--l);
	}

	d101.cmnt[39]='\0';

	d101.smbl = 0;
	d101.dst = 300;
	xfwrite((char *)&d101, sizeof(d101),1,out);
	++nRec;
}

int cls_FXW(FILE *out)
{
	WPHDR hdr;

	if (titleBuf[0]=='\0')
		strcpy(titleBuf,"LOGGERWAYPNT");

	hdr.magic[0] = 'W'; hdr.magic[1] = 'P'; hdr.version = 1;
	return cls_FX(out,".wwp",titleBuf,(byte *)&hdr,sizeof hdr);
}
