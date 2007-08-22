/* reduction.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

 reduction log

History:
2004/03/18 V0.35
���Ă̓_���o�b�t�@���Ďn�_�ƏI�_�����߂�B

�n�_-�I�_�Œ����������B
���̒�������ł�����Ă���_��L���ɂ��A�V���ȏI�_/�n�_�Ƃ���B
�L���_���ő�_���ɒB����A�������͍ł�����Ă���_�̋��������e�l�������
�܂ōċA����B

�X���b�h���l������Ƃ�₱�����Ȃ�̂łƂ肠������������B

�{���͍��x���A���ʂł��邱�Ƃ��l������ׂ����������܂ł���K�v�͖������Ƃɂ���B

�o�b�t�@�ƍċA�Ń��������ʂɏ����..DOS�łł͓��삵�Ȃ����낤..

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

/* �o�b�t�@�ɋl�ߍ��� */

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

/* reduction���s�� */

int cls_rdc(FILE *out)
{
	int i, maxrec;
	double sheeta, inclination, ofs, x, y;
	double  diff,maxdiff=0;
	double 	latincl, latofs, latdiff;
	double 	lonincl, lonofs, londiff;
	
	/* �X�������߂� */
	/* �����傾������? */
	y = xybuff[nRec-1].position.lat - xybuff[0].position.lat;
	x = xybuff[nRec-1].position.lon - xybuff[0].position.lon;

	if ((y != 0) && (x != 0))
		inclination = y/x;
		cos_sheeta = cos(atan2(x,y));
		/* �ؕЂ����߂�B*/
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

/* reduction ���ʂ�f���o�� */

int in_rdc(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
}

#endif