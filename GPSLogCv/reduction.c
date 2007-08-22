/* reduction.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

 reduction log

History:
2004/03/18 V0.35
総ての点をバッファして始点と終点を求める。

始点-終点で直線を引く。
その直線から最も離れている点を有効にし、新たな終点/始点とする。
有効点が最大点数に達する、もしくは最も離れている点の距離が許容値を下回る
まで再帰する。

スレッドを考慮するとややこしくなるのでとりあえず無視する。

本来は高度も、球面であることも考慮するべきだがそこまでする必要は無いことにする。

バッファと再帰でメモリを大量に消費する..DOS版では動作しないだろう..

*/
#include <math.h>
#include "gpslogcv.h"

typedef struct {
	cordinate_t position;
	cnvData_t	GPSLog;
	double maxdiff;
	int idxmax;
} xyrad_t;

#if 0
static xyrad_t *xybuff;

/* バッファに詰め込む */

void out_rdc(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
    if (nRec == 0) {
		xybuff = malloc(sizeof(xyrad_t)*32768);	/* allocate buffer */
    } else if (nRec < 32768) {
		xybuff[nRec].position = *position;
		xybuff[nRec].GPSLog = *GPSLogp;
	    nRec++;
	} else {
		fprintf(stderr,"Reduction buffer overflow.\n");
		XErr++;
	}
}

/* reductionを行う */

int cls_rdc(FILE *out)
{
	int i, maxrec;
	double sheeta, inclination, ofs, x, y;
	double  diff,maxdiff=0;
	double 	latincl, latofs, latdiff;
	double 	lonincl, lonofs, londiff;
	
	/* 傾きを求める */
	/* 無限大だったら? */
	y = xybuff[nRec-1].position.lat - xybuff[0].position.lat;
	x = xybuff[nRec-1].position.lon - xybuff[0].position.lon;

	if ((y != 0) && (x != 0))
		inclination = y/x;
		cos_sheeta = cos(atan2(x,y));
		/* 切片を求める。*/
		lonofs = xybuff[0].position.lat- xybuff[0].position.lon*inclination;
	}

	if (x == 0)
	for(i=0;i<nRec;i++) {
		diff = (xybuff[i].position.lon*inclination + lonofs - latincl*(xybuff[i].position.lon))*cos_sheeta;
		if (maxdiff < diff) {
			maxdiff = diff;
			maxrec = i;
		}
	}
}

/* reduction 結果を吐き出す */

int in_rdc(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
}

#endif