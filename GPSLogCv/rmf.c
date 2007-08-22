/* rmf.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

 Panasonic Car Navigation
 Route Points

History:
  2005/06/06 initial release
*/
#include <string.h>
#include "gpslogcv.h"

static char *strTABcpy(char *dst, char *src)
{
	char *p = dst;
	unsigned char *q = (unsigned char *)src;

	while(*q > '\t')
		*p++ = *q++;
	*p = '\0';
	return dst;
}

static int n = -1;
static char *p;

#ifdef VCPP
#pragma warning( disable: 4100)
#endif

int in_RMF(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
/* return value -1:Error, 0:EOF, 1:OK */
{
	double d;
	long l;

	titleBuf[0] = '\0';

	if ((n < 0) && ((p = xfgets(rFile))!=NULL))
		if (strncmp(p,"登録ルート",10)==0)
			if ((p=xfgets(rFile))!=NULL)
				n++;

	if (n >= 0) {
		while(*p) {
			if (*p++ == '#') {
				++n;
				switch(n) {
				case  1:	// ルート名称
					icon = atoi(p);
					break;
				case  2:	// ?
					break;
				case  3:	// 経由地数
					break;
				case  4:	// 700:始点, 701:終点, 702-706:経由
				case 20:
				case 36:
				case 52:
				case 68:
				case 84:
				case 100:
					break;
				case  5:	/* 東経 13545326 */
				case 21:
				case 37:
				case 53:
				case 69:
				case 85:
				case 101:
					l=atol(p);
					d = (l%1000);l/=1000;
					d = d/600 + (l%100);l/=100;
					position->lon = d/60 + l;
					break;
				case  6:	/* 北緯 03441188 */
				case 22:
				case 38:
				case 54:
				case 70:
				case 86:
				case 102:
					l=atol(p);
					d = (l%1000);l/=1000;
					d = d/600 + (l%100);l/=100;
					position->lat = d/60 + l;
					break;
				case  7:	// 経由地名
				case 23:
				case 39:
				case 55:
				case 71:
				case 87:
				case 103:
					strTABcpy(titleBuf,p);
					break;
				case  8:	// ?
				case 24:
				case 40:
				case 56:
				case 72:
				case 88:
				case 104:
					break;
				case  9:	// ?
				case 25:
				case 41:
				case 57:
				case 73:
				case 89:
				case 105:
					break;
				case 10:	// ?
				case 26:
				case 42:
				case 58:
				case 74:
				case 90:
				case 106:
					break;
				case 11:	// ?
				case 27:
				case 43:
				case 59:
				case 75:
				case 91:
				case 107:
					break;
				case 12:	// ?
				case 28:
				case 44:
				case 60:
				case 76:
				case 92:
				case 108:
					break;
				case 13:	//
				case 29:
				case 45:
				case 61:
				case 77:
				case 93:
				case 109:
					break;
				case 14:	//
				case 30:
				case 46:
				case 62:
				case 78:
				case 94:
				case 110:
					break;
				case 15:	//
				case 31:
				case 47:
				case 63:
				case 79:
				case 95:
				case 111:
					if (titleBuf[0] != '\0')
						return 1;
					break;
				case 16:	//
				case 32:
				case 48:
				case 64:
				case 80:
				case 96:
					break;
				case 17:	//
				case 33:
				case 49:
				case 65:
				case 81:
				case 97:
					break;
				case 18:	//
				case 34:
				case 50:
				case 66:
				case 82:
				case 98:
					break;
				case 19:	//
				case 35:
				case 51:
				case 67:
				case 83:
				case 99:
					break;
				case 112:
					n = -1;
					return 0;	/* return EOF */
				default:
					break;
				}
			}
		}
	}
	n = -1;
	fprintf(stderr,"*Error - Unexpected format !\n");
	return XErr = -1;
}

#ifdef VCPP
#pragma warning( default: 4100)
#endif