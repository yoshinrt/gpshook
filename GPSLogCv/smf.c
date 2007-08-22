/* smf.c - part of gpslogcv
mailto:imaizumi@nisiq.net
http://homepage1.nifty.com/gigo/

  Panasonic CN-HS400 .smf

History:
2005/05/10 initial 
*/

#define DEBUG 0

#if 0
-----
0x00000
　ヘッダ
0x000C0
  0フィル
0x000F0
  リンクリスト
0x00120
  0フィル
0x00160
　時刻と地点名。
　外部に出てこないファイル名?
0x001D0
  0フィル
0x00240
  軌跡情報 -　最大(北東)、最小(南西)、距離
  004CC027 0013D1FE 004CB510 0013B744
0x00260
  0x58バイトのファイルヘッダ
  ファイルヘッダで示されたのと同じだけのレコード(1レコード=0x18)
0x01A28(固定ではない。0x58+レコード数(この場合は0xFA(250))*0x18
  0x58バイトのファイルヘッダ
  ファイルヘッダで示されたのと同じだけのレコード(この場合は0xFA(250),1レコード=0x18)
0x31F0
  0x58バイトのファイルヘッダ
  ファイルヘッダで示されたのと同じだけのレコード(この場合は0x65,1レコード=0x18)
0x49c0
  0x58バイトのファイルヘッダが並ぶ。ここでは0x00-0x19
0x52b0

3っつのファイルを使ってリングバッファ構成を取っている。
1ファイルは約25Km分。
3ファイル目を書き始めた時点が2ファイルしか使えない最悪の場合で50Km分。
これが最低保証。
3ファイル目を使い切る時が最もよい場合で75km分記録されている。

#endif

#include <string.h>
#include <fcntl.h>
#include <io.h>
//#include <mbctype.h>
//#include <sys/utime.h>
//#include <sys/stat.h>

#include "gpslogcv.h"

#pragma pack(1)

typedef struct {
	long lon;
	long lat;
} smf_pos_t;

typedef struct {
	long	lon;
	long dmy1;	/* 常に0 */
	long	lat;
	long dmy2;	/* 常に0 */
	long dmy3;	/* 常に0 */
	long dmy4;	/* 常に0 */
} smf_logpos_t;

typedef struct {
	word id;
	word stab1;
	smf_logpos_t minloc;
	smf_logpos_t maxloc;
	smf_pos_t strtloc;
	smf_pos_t endloc;
	word stab2;
	char fname[10];
	char stab4[4];
	long recs;
} smf_dir_t;

#pragma pack()

static int n,rec,max,nmax;
static int lnk[21];
static smf_dir_t smfdir;

#if DEBUG==1
static smf_pos_t maxloc;
static smf_pos_t minloc;
static long Distance;
#endif

int in_SMF(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
	int i;
	char work[32];
	smf_logpos_t pos;
	
    if (nRec == 0) {
        if (_setmode( _fileno(rFile), _O_BINARY )== -1 ) {
            perror( "*Internal Error - can't set binary mode.\n" );
            return -1;	/* error with errno */
        }
        xfread(&work,32,1,rFile);
		if (strncmp(work,"SMRUNLOGver1.0",14)!=0) {
			XErr++;
			fprintf(stderr,"*Error - Missing header !\n");
			return -1;
		}

        xfread(&work,24,1,rFile);
#if DEBUG==1
		fprintf(stderr,"%s\n",work);
#endif
		strncpy(titleBuf,work,TTBUFLEN-1);
#if DEBUG==1
		xfseek(rFile,0x240,SEEK_SET);
        xfread(&maxloc,sizeof(maxloc),1,rFile);
		position->lat = (double)(dswab(maxloc.lat))/36000;
		position->lon = (double)(dswab(maxloc.lon))/36000;
		printf("Start:%8.5f %8.5f\n",position->lat, position->lon);
        xfread(&minloc,sizeof(minloc),1,rFile);
		position->lat = (double)(dswab(minloc.lat))/36000;
		position->lon = (double)(dswab(minloc.lon))/36000;
		printf("End  :%8.5f %8.5f\n",position->lat, position->lon);
        xfread(&Distance,sizeof(Distance),1,rFile);
		Distance = dswab(Distance);
		printf("Distance:%d\n",Distance);
#endif
		
		xfseek(rFile,0xF0,SEEK_SET);
        xfread(&work,sizeof(work),1,rFile);
		
		nmax = (unsigned char)work[1]-1;
		for (max = 0,i=4;i<4+nmax*8;i+=8) {	/* Max file number */
			if (max<work[i]) {
				max = work[i];
			}
		}
		for (i=4;i<4+nmax*8;i+=8) {	/* order list */
			lnk[work[i]+nmax-1-max]=(i-4)/8;
		}
		
		smfdir.recs = n = 0;
		rec = 0x7FFF;
	}
	if (rec>=smfdir.recs) {
		if (n < nmax) {
			xfseek(rFile,0x260+lnk[n++]*0x17c8,SEEK_SET);
			xfread(&smfdir,sizeof(smfdir),1,rFile);
			smfdir.recs = dswab(smfdir.recs);
			sprintf((char *)work,"k%03d.dat",max-nmax+n);
			if (strcmp(smfdir.fname,(char *)work)!=0) {
				fprintf(stderr,"*Error - Missing directory !\n");
				return -1;
			}
#if DEBUG==1
			printf("\n---\n%s - %d records\n",smfdir.fname,smfdir.recs);
			printf("Min  :%8.5f %8.5f\n",(double)dswab(smfdir.minloc.lat)/36000, (double)dswab(smfdir.minloc.lon)/36000);
			printf("Max  :%8.5f %8.5f\n",(double)dswab(smfdir.maxloc.lat)/36000, (double)dswab(smfdir.maxloc.lon)/36000);
			printf("Start:%8.5f %8.5f\n",(double)dswab(smfdir.strtloc.lat)/36000, (double)dswab(smfdir.strtloc.lon)/36000);
			printf("End  :%8.5f %8.5f\n\n",(double)dswab(smfdir.endloc.lat)/36000, (double)dswab(smfdir.endloc.lon)/36000);

#endif
			rec = 0;
			GPSLogp->thread = 1;
		} else
			return 0;
	}
	xfread(&pos,sizeof(pos),1,rFile);
	position->lat = (double)(dswab(pos.lat))/36000;
	position->lon = (double)(dswab(pos.lon))/36000;
#if DEBUG==1
	printf("%3d  :%8.5f %8.5f\n",rec, position->lat, position->lon);
	if (pos.dmy1 + pos.dmy2 + pos.dmy1 + pos.dmy2) {
		fprintf(stdout,"Unknown stab not zero. 0x%04lx\n",ftell(rFile));
	}
#endif
	rec++;
	return 1;
}
