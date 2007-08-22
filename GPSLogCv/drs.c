/* drs.c - part of gpslogcv
mailto:imaizumi@nisiq.net
http://homepage1.nifty.com/gigo/

  Panasonic CN-HS400 .drs

History:
2005/05/04 initial 
*/
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <mbctype.h>
#include <sys/utime.h>
#include <sys/stat.h>

#include "gpslogcv.h"

typedef struct {
	dword ofs; //?? 0x00A0
	dword data_size;
	dword num_entry;
	dword size1; // ?? data_size +0xA0 
	dword size2; // ?? data_size +0xA0 +size2 = file size
	dword rev; //?? thread ?
	dword distance; // m
	dword seconds; // Œo‰ßŽžŠÔ
	byte c0;	// BCD
	byte y0;	// BCD
	byte m0;	// BCD
	byte d0;	// BCD
	byte h0;	// BCD
	byte min0;	// BCD
	byte s0;	// BCD
	byte stab0;
	byte c1;	// BCD
	byte y1;	// BCD
	byte m1;	// BCD
	byte d1;	// BCD
	byte h1;	// BCD
	byte min1;	// BCD
	byte s1;	// BCD
	byte stab1;
	word stab2[8];
	char strt[32];
	char end[32];
	char firm[8];
	word stab3[12];
} DRS_HEADER_T;

DRS_HEADER_T headerbuff;

#pragma pack(1)
typedef struct {

//	dword t;
	byte a[52];
#if 0
	short int i;
	char byte[10];
	double d[2];
	double a;
	double b;
	double c;
	double d;
	double e;
	long f;
	long g;
#endif
} DRS_T;
#pragma pack()

DRS_T drsbuff;

#if 0
static word wswab(word big)
{
    word little;
    char *p,*q;
	
    p =((char *)&big)+1;
    q =(char *)&little;
	
    *q++ = *p--;
    *q = *p;
    return little;
}
#endif

static time_t drs_time(byte *p)
{
	static struct tm tmbuf;

	tmbuf.tm_year = (((*p>>4) & 0x0F)*10) + (*p & 0x0F);p++;
	tmbuf.tm_year = tmbuf.tm_year*100 + (((*p>>4) & 0x0F)*10) + (*p & 0x0F) - 1900;p++ ;
	tmbuf.tm_mon = (((*p>>4) & 0x0F)*10) + (*p & 0x0F);p++;
	tmbuf.tm_mday = (((*p>>4) & 0x0F)*10) + (*p & 0x0F);p++;
	tmbuf.tm_hour = (((*p>>4) & 0x0F)*10) + (*p & 0x0F);p++;
	tmbuf.tm_min = (((*p>>4) & 0x0F)*10) + (*p & 0x0F);p++;
	tmbuf.tm_sec = (((*p>>4) & 0x0F)*10) + (*p & 0x0F);p++;
	return mktime(&tmbuf);
}

static dword chkbuf[52];

int in_DRS(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
	time_t t;
	dword n;

    if (nRec == 0) {
        if (_setmode( _fileno(rFile), _O_BINARY )== -1 ) {
            perror( "*Internal Error - can't set binary mode.\n" );
            return -1;	/* error with errno */
        }
	}
	if ((sizeof(DRS_T) != 52) || (sizeof(DRS_HEADER_T) != 0xA0))
		perror( "*Internal Error - struct size.\n" );

	fread(&headerbuff,1,sizeof(DRS_HEADER_T),rFile);

	printf("Start:%s\n",headerbuff.strt);
	printf("End  :%s\n",headerbuff.end);
	headerbuff.ofs = dswab(headerbuff.ofs);
	headerbuff.data_size = dswab(headerbuff.data_size);
	headerbuff.num_entry = dswab(headerbuff.num_entry);
	printf("Number of Points:%d\n",headerbuff.num_entry);
	headerbuff.size1 = dswab(headerbuff.size1);
	headerbuff.size2 = dswab(headerbuff.size2);
	headerbuff.rev = dswab(headerbuff.rev);
	headerbuff.distance = dswab(headerbuff.distance);
	printf("Distance:%ld(m)\n",headerbuff.distance);
	headerbuff.seconds = dswab(headerbuff.seconds);
	printf("Elapsed:%ld(sec)\n",headerbuff.seconds);
	t = drs_time(&headerbuff.c1)-drs_time(&headerbuff.c0);
	n = 0;
	do {
		int i;
		fread(&drsbuff,1,sizeof(DRS_T),rFile);
#if 0
		printf("%d",n/52);
		printf(",%d",drsbuff.a[0]*256+drsbuff.a[1]);
		printf(",%d",drsbuff.a[2]);
		printf(",%d",drsbuff.a[3]);
		printf(",%d",drsbuff.a[4]);
		printf(",%d",drsbuff.a[5]);
		printf(",%d",drsbuff.a[6]);
		printf(",%d",drsbuff.a[7]);
		printf(",%d",drsbuff.a[8]);
		printf(",%d",drsbuff.a[9]);
		printf(",%d",drsbuff.a[10]);
		printf(",%d",drsbuff.a[11]);
		printf(",%d",drsbuff.a[12]*256+drsbuff.a[13]);
		printf(",%d",drsbuff.a[14]*256+drsbuff.a[15]);
		printf(",%d",drsbuff.a[16]*256+drsbuff.a[17]);
		printf(",%d",drsbuff.a[18]*256+drsbuff.a[19]);
		printf(",%d",drsbuff.a[20]*256+drsbuff.a[21]);
		printf(",%d",drsbuff.a[22]*256+drsbuff.a[23]);
		printf(",%d",drsbuff.a[24]*256+drsbuff.a[25]);
		printf(",%d",drsbuff.a[26]*256+drsbuff.a[27]);
		printf(",%d",drsbuff.a[30]*256+drsbuff.a[31]);
		printf(",%d",drsbuff.a[32]*256+drsbuff.a[33]);
		printf(",%d",drsbuff.a[34]*256+drsbuff.a[35]);
		printf(",%d",drsbuff.a[36]*256+drsbuff.a[37]);
		printf(",%d",drsbuff.a[38]*256+drsbuff.a[39]);
		printf(",%d",drsbuff.a[48]*256+drsbuff.a[49]);
		printf(",%d\n",drsbuff.a[50]*256+drsbuff.a[51]);
#else
		if (n==0) {
			for(i=0;i<52;i++)
				printf("%02x",i);
			printf("\n");
		}
		for(i=0;i<52;i++) {
//			drsbuff.a[i] = dswab(drsbuff.a[i]);
//			printf(",%d",drsbuff.a[i]);
			if ((chkbuf[i] == drsbuff.a[i] )&&(n!=0)) {
				printf("==");
			} else {
				printf("%02x",drsbuff.a[i]);
//					printf("%4d, %2d, %8d, 0x%08x, %d\n",n/52,i,drsbuff.a[i],drsbuff.a[i],drsbuff.a[i]-chkbuf[i]);
				chkbuf[i] = drsbuff.a[i];
			}
		}
		printf("\n");
#endif
	} while(++n < headerbuff.num_entry);
	ftell(rFile);
    return 0;
}
