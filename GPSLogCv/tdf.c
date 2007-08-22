/* tdf.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

 Track Diffed Format file.

History:
2005/05/23 V0.37b TDF 2.0 0xff terminate.
2003/08/03 V0.34n check illegal time record.
2002/06/26 V0.34i fix zero time diff record bug.
2001/09/14 V0.01a optimize a little.
2001/03/09 V0.01 initial release.
*//* -f -OR -DT *.dif */
/* -tdif -an -of *.* */
#include <fcntl.h>
#include <io.h>
#include <direct.h>
#include <math.h>
#include <string.h>
#include <sys/utime.h>
#include <sys/stat.h>

#include "gpslogcv.h"

#define BITS_PAR_SEC 5
#define TOINT ((1<<BITS_PAR_SEC)*3600)

#define MAGIC_TDF 0xdf
#define TDF_VERSION 0x01

unsigned char id[] = { MAGIC_TDF, TDF_VERSION }; 

#define ALT_INVALID -32768

static char bbuf[13];

static int		alt0;
static int		lat0;
static int		lon0;
static time_t	tim0;

static time_t	tim1;

static int		zcnt;
static int		ver;

int in_TDF(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
	if (nRec == 0) {
		if (_setmode( _fileno(rFile), _O_BINARY )== -1 ) {
			perror( "*Internal Error - can't set binary mode.\n" );
			return -1;	/* error with errno */
		}
		xfread(bbuf,sizeof(char),2,rFile);
		if ((byte)bbuf[0] != MAGIC_TDF) {
			fprintf(stderr,"Illegal format. \n");
			return -1;
		}
		switch(ver = bbuf[1]){
		case 0:
		case 1:
		case 2:
			break;
		default:
			fprintf(stderr,"Illegal version. \n");
			return -1;
		}
	}
	if (zcnt == 0) {
#ifdef DEBUG
		if (nRec > 0x7fffffff)
			printf("%5d:%08x, ",nRec,ftell(rFile));
#endif
		if (fread(bbuf,1,1,rFile) < 1)
			return 0;
		if ((unsigned char)bbuf[0]==0xff) {	// terminate code
			struct _utimbuf modTimBuf;
			struct _stat statbuf;
			long rSize = ftell(rFile)-1;
			if (_fstat(_fileno(rFile),&statbuf) == -1) {
				perror( "*Internal Error - can't get _stat.\n" );
				return -1;	/* error with errno */
			}
			if (_chsize(_fileno(rFile),rSize)!=0)
				perror( "*Internal Error - can't trucate.\n" );
			xfseek(rFile,0L,SEEK_SET);
			xfread(bbuf,sizeof(char),4,rFile);
			if (memcmp(bbuf,"#!ws",4) == 0) { // .fx
				xfseek(rFile,0x6cl,SEEK_SET);
				rSize -= 0x80;
				xfwrite((void *)&rSize,sizeof(rSize),1,rFile);
			}
			modTimBuf.actime = modTimBuf.modtime = statbuf.st_mtime;
			if( _futime(_fileno(rFile), &modTimBuf ) == -1 )
				perror( "*Internal Error - can't set time stamp.\n" );
			return 0;
		}

		if (bbuf[0]&0x3)
			xfread(&bbuf[1],(bbuf[0]&0x3),1,rFile);
	}
	/* -- */
	switch(bbuf[0]&0x3) {
	case 0x3:
		if (bbuf[0]&4) {
			/* 4byte tttt t111 nnnn nnnn eeee eeee aaaa aane, for time */
			lat0 = lat0 + ((bbuf[1] << 1) | ((bbuf[3]>>1)&1));
			lon0 = lon0 + ((bbuf[2] << 1) | (bbuf[3]&1));
			alt0 = alt0 + (bbuf[3]>>2);
			tim0 = tim0 + ((bbuf[0]>>3)&0x1f) + 2;
		} else if (bbuf[0]&8) {
			/* 4byte neeT 1011 */
			/*       nnnn nnnn nnnn nnnn nnnn nnnn  full */
			/*       eeee eeee eeee eeee eeee eeee */
			/*       aaaa aaaa aaaa aaaa full */
			/*       tttt tttt tttt tttt tttt tttt tttt tttt for air */
#ifndef DEBUG
			xfread(&bbuf[4],9,1,rFile);
#endif
			lat0 = (*((int *)(&bbuf[0])) >> 7);
			lon0 = ((*((int *)(&bbuf[3])) >> 6)&~3)|((bbuf[0]>>5)&3);
			alt0 = *((short *)(&bbuf[7]));
			tim0 = *((time_t *)(&bbuf[9]));
			if (bbuf[0]&0x10)
				GPSLogp->thread = 1;
		} else {
			/* 4byte nnee 0011 nnnn nnnn eeee eeee aaaa aaat, for air */
			lat0 = lat0 + ((bbuf[1] << 2) | ((bbuf[0]>>6)&3));
			lon0 = lon0 + ((bbuf[2] << 2) | ((bbuf[0]>>4)&3));
			alt0 = alt0 + (bbuf[3]>>1);
			tim0 = tim0 + (bbuf[3]&1) + 1;
		}
		break;
	case 0x2:
		lat0 = lat0 + bbuf[1];
		lon0 = lon0 + bbuf[2];
		tim0 = tim0 + ((bbuf[0]>>2)&1) + 1;
		alt0 = alt0 + (bbuf[0]>>3);
		break;
	case 0x1:
		tim0 = tim0 + ((bbuf[0]>>2)&1) + 1;
		lat0 = lat0 + (bbuf[0]>>3);
		lon0 = lon0 + (bbuf[1]>>3);
		bbuf[1] <<= 5;
		alt0 = alt0 + (bbuf[1]>>5);
		break;
	case 0x0:
		if (zcnt==0)
			zcnt = ((bbuf[0]>>3)&0x1f)+1;
		tim0 += ((bbuf[0]>>2)&1)+1;
		--zcnt;
		break;
	}
	if (alt0 == ALT_INVALID)
		GPSLogp->datTyp &= ~DAT_ALT;

	position->lat = (double)lat0/TOINT;
	position->lon = (double)lon0/TOINT;
	position->alt = (double)alt0;

	if (tim0 < 0) {
		fprintf(stderr,"Illegal time detected(%d). \n",nRec);
		tim0 = 0;
	}

	if (tim1-tim0 == 604799L)	/* quick hack for wn_days problem */
		tim0 += 604800L;
	tim1 = tim0;

	if (ver == 0)
		*GPSLogp->recTime = *localtime(&tim0);
	else
		*GPSLogp->recTime = *gmtime(&tim0);

	return 1;
}

//#define DEBUG 1

static short	lastalt;
static int		lastlat;
static int		lastlon;
static time_t	lasttim;
static	byte	lasttdf;
static int rep;
static int f,c1,c11,c2,c3,c4,c41;

void put0Rec(FILE *out)
{
	int k;
	byte zbuf;

#ifdef DEBUG
	tim0 += (lasttdf+1) * rep;
	if (nRec > 0x7fffffff)
		printf("%5d:%08x, ",nRec,ftell(out));
#endif
	/* kkkk kt00 */
	while (rep) {
		c11++;
		k = ((rep> (1<<5)) ? (1<<5):rep) -1;
		zbuf = (byte)(((k&0x1f)<<3)|((lasttdf&1)<<2)/*| 0*/);
		xfwrite(&zbuf,1,1,out);
		rep -= k+1;
	}
}

/* for air plane */
#define ANG_BIT_4_00  9/*9*/
#define ALT_BIT_4_00  6/*6*/
/* for repeat */
#define ANG_BIT_4_01  9/*8*/
#define ALT_BIT_4_01  6/*5*/
/* for long period */
#define ANG_BIT_4_1x  8/*8*/
#define ALT_BIT_4_1x  5/*5*/
//#define TIM_VAL_4_1x  32/*32*/
#define TIM_VAL_4_1x  31 /*v 2.0 31*/

#define ANG_BIT_3  7/*7*/
#define ALT_BIT_3  4/*4*/
#define ANG_BIT_2  4/*4*/
#define ALT_BIT_2  2/*2*/

#define REC1 0
#define REC2 1
#define REC3 2
#define REC4 3
#define REC4_1x 7
#define FULL 13

void out_TDF(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
	byte type,i,j,k;
	short a,alt,altdif;
	int l,lat,lon,latdif,londif;
	time_t tim;
	unsigned long timdif;
	
	if (nRec++ == 0) {
		if (_setmode( _fileno(out), _O_BINARY )== -1 ) {
			perror( "can't set binary mode." );
			XErr++;
		}
		xfwrite(&id[0],sizeof(id),1,out);
		lasttim = 0;
		f = c1 = c11 = c2 = c3 = c4 = c41 = 0;
		rep = 0;
	}
	
	lat = round(position->lat * TOINT);
	lon = round(position->lon * TOINT);
	tim = mktime(GPSLogp->recTime) - timezone ;	/* time_t Sec. UTC */
	
	latdif = lat - lastlat;
	londif = lon - lastlon;
	timdif = tim - lasttim;
	
	l = ((latdif<0)? -latdif:latdif)|((londif<0)? -londif:londif);
	for(k=0;l;k++)
		l >>= 1;

	if ((GPSLogp->datTyp & DAT_ALT) == 0)
		alt = (short)ALT_INVALID;
	else {
		alt = (short)position->alt;
		if ((int)alt != (int)position->alt)
			alt = (short)((alt & 0x8000) | 0x7fff);
		else
			alt = (short)(alt+(position->alt - alt)*2.0);	/* ROUND */
	}
	altdif = (short)(alt - lastalt);
	a = (short)((altdif >= 0)? altdif:-altdif);
	for (j=0;a;j++)
		a >>= 1;

	timdif--;

//	if (nRec == 1304)
//		printf("nRec:%d\n",nRec);

	if ((k > ANG_BIT_4_00) || (j > ALT_BIT_4_00) || (timdif > TIM_VAL_4_1x) || (GPSLogp->thread!=0)) {
		/* full */
		type = FULL;
	} else if (timdif > 1)  {
		if ((k <= ANG_BIT_4_1x) && (j <= ALT_BIT_4_1x)) {
			/* time */
			type = REC4_1x;
		} else {
			type = FULL;
		}
	} else if ((k > ANG_BIT_3) || (j > ALT_BIT_3)) {
		/* 4byte */
		type = REC4;
	} else if ((k > ANG_BIT_2) || (j > ALT_BIT_2)) {
		/* 3byte */
		type = REC3;
	} else if (k+j) {
		/* 2byte */
		type = REC2;
	} else {
		type = REC1;
	}
	i = 4;
	switch(type) {
	case FULL:
		/* 4byte neeT 1011 */
		/*       nnnn nnnn nnnn nnnn nnnn nnnn  full */
		/*       eeee eeee eeee eeee eeee eeee */
		/*       aaaa aaaa aaaa aaaa full */
		/*       tttt tttt tttt tttt tttt tttt tttt tttt for air */
		f++;
		bbuf[0] = (byte)(((lat&1)<< 7) | (lon&3) << 5 | ((GPSLogp->thread&1)<<4) | 11);
		*((int *)(&bbuf[1])) = lat >> 1;
		*((int *)(&bbuf[4])) = lon >> 2;
		*((short *)(&bbuf[7])) = (short)alt;
		*((time_t *)(&bbuf[9])) = tim;
		GPSLogp->thread = 0;
		i = 13;
		break;
	case REC4:
		c4++;
		/* 4byte nnee 0011 nnnn nnnn eeee eeee aaaa aaat, for air */
		bbuf[0] = (byte)( ((latdif&3)<< 6) | ((londif&3)<<4) | 3);
		bbuf[1] = (byte)(latdif >> 2);
		bbuf[2] = (byte)(londif >> 2);
		bbuf[3] = (byte)((altdif << 1)|(timdif&1));
		break;
	case REC4_1x:
		c41++;
		/* 4byte tttt t111 nnnn nnnn eeee eeee aaaa aane, for time */
		bbuf[0] = (byte)((--timdif<<3) | 7);
		bbuf[1] = (byte)(latdif >> 1);
		bbuf[2] = (byte)(londif >> 1);
		bbuf[3] = (byte)( (altdif<<2) | ((latdif&1)<<1) | (londif&1) );
		break;
	case REC3:
		c3++;
		/* 3byte aaaa at10 nnnn nnnn eeee eeee */
		bbuf[0] = (byte)((altdif<<3)|(timdif<<2)|2);
		bbuf[1] = (byte)latdif;
		bbuf[2] = (byte)londif;
		i = 3;
		break;
	case REC2:
		c2++;
		/* 2byte nnnn nt01 eeee eaaa */
		bbuf[0] = (byte)((latdif<<3)|(timdif<<2)|1) ;
		bbuf[1] = (byte)((londif<<3)|(altdif &7));
		i = 2;
		break;
	case REC1:
		c1++;
		if ((rep != 0) && (lasttdf != timdif))
			put0Rec(out);
		rep++;
		lasttdf = (byte)timdif;
		lasttim = tim;
		return;
	}
	if (rep) 
		put0Rec(out);

#ifdef DEBUG
		if (nRec > 0x7fffffff)
			printf("%5d:%08x ,",nRec,ftell(out));
#endif
	xfwrite(bbuf,i,1,out);
#ifdef DEBUG
	/* -- cut -- */
	switch(bbuf[0]&0x3) {
	case 0x3:
		if (bbuf[0]&4) {
			/* 4byte tttt t111 nnnn nnnn eeee eeee aaaa aane, for time */
			lat0 = lat0 + ((bbuf[1] << 1) | ((bbuf[3]>>1)&1));
			lon0 = lon0 + ((bbuf[2] << 1) | (bbuf[3]&1));
			alt0 = alt0 + (bbuf[3]>>2);
			tim0 = tim0 + ((bbuf[0]>>3)&0x1f) + 2;
		} else if (bbuf[0]&8) {
			/* 4byte neeT 1011 */
			/*       nnnn nnnn nnnn nnnn nnnn nnnn  full */
			/*       eeee eeee eeee eeee eeee eeee */
			/*       aaaa aaaa aaaa aaaa full */
			/*       tttt tttt tttt tttt tttt tttt tttt tttt for air */
#ifndef DEBUG
			xfread(&bbuf[4],9,1,out);
#endif
			lat0 = (*((int *)(&bbuf[0])) >> 7);
			lon0 = ((*((int *)(&bbuf[3])) >> 6)&~3)|((bbuf[0]>>5)&3);
			alt0 = *((short *)(&bbuf[7]));
			tim0 = *((time_t *)(&bbuf[9]));
			GPSLogp->thread = (bbuf[0] >> 4)&1;
		} else {
			/* 4byte nnee 0011 nnnn nnnn eeee eeee aaaa aaat, for air */
			lat0 = lat0 + ((bbuf[1] << 2) | ((bbuf[0]>>6)&3));
			lon0 = lon0 + ((bbuf[2] << 2) | ((bbuf[0]>>4)&3));
			alt0 = alt0 + (bbuf[3]>>1);
			tim0 = tim0 + (bbuf[3]&1) + 1;
		}
		break;
	case 0x2:
		lat0 = lat0 + bbuf[1];
		lon0 = lon0 + bbuf[2];
		tim0 = tim0 + ((bbuf[0]>>2)&1) + 1;
		alt0 = alt0 + (bbuf[0]>>3);
		break;
	case 0x1:
		tim0 = tim0 + ((bbuf[0]>>2)&1) + 1;
		lat0 = lat0 + (bbuf[0]>>3);
		lon0 = lon0 + (bbuf[1]>>3);
		bbuf[1] <<= 5;
		alt0 = alt0 + (bbuf[1]>>5);
		break;
	case 0x0:
		if (zcnt==0)
			zcnt = ((bbuf[0]>>3)&0x1f)+1;
		tim0 += ((bbuf[0]>>2)&1)+1;
		--zcnt;
		break;
	}
	if (tim != tim0)
		printf("et");
	if (lat != lat0)
		printf("ela");
	if (lon != lon0)
		printf("elo");
	if (alt != alt0)
		printf("ea");
	GPSLogp->thread = 0;
#endif	
	lastlat = lat;
	lastlon = lon;
	lastalt = alt;
	lasttim = tim;
}

int cls_TDF(FILE *out)
{
	int count = c1+c2+c3+c4+c41+f;
	if (rep)
		put0Rec(out);
	printf("f13:%6d(%5.1f%%)\t",f,(f*100.0/count));
	printf("c4 :%6d(%5.1f%%)\t",c4,(c4*100.0/count));
	printf("c41:%6d(%5.1f%%)\n",c41,(c41*100.0/count));
	printf("c3 :%6d(%5.1f%%)\t",c3,(c3*100.0/count));
	printf("c2 :%6d(%5.1f%%)\t",c2,(c2*100.0/count));
	printf("c1 :%6d(%5.1f%%),",c1,(c1*100.0/count));
	printf("(c11:%d)\n",c11);
	return fclose(out);
}
