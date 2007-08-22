/* xml.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

 This module support ItsMoNavi .XML.

History:
2005/05/07 V0.35a initial release
*/
#include <ctype.h>
#include <math.h>
#include <string.h>

#include "gpslogcv.h"

#if 0
static int adr;

static char *xstrchr(char *p,char dlm)
{
    char *q;

    if (XErr)
        return p;
    if ((q = strchr(p,dlm))==NULL) {
        XErr++;
        return p;
    }
    return q+1;
}

static double angStrPot2Ang(char *p, int type)
{
	int minus;
    double d;

	while (isspace(*p))
		p++;
    switch(type) {
    case DAT_DMS:
		if ((minus = ('-' == *p))!=0)
			p++;
		d = atoi(p);p = xstrchr(p,'\'');
        d += atof(p)/60; p = xstrchr(p,'\'');
        d += atof(p)/3600; p = xstrchr(p,'\'');
        d += atof(p)/36000;
        break;
    default:
    case DAT_DEG:
    case DAT_DMM:
        XErr++;
        fprintf(stderr,"*Error - illegal angle format !\n");
        return 0;
    }
    return minus ? -d:d;
}
#endif

static char sepbuf[8][512];

static char *sep(int lvl, char *key)
{
	char *q,*r;
	char *src = &sepbuf[lvl++][0];
	char *s = &sepbuf[lvl][0];

	sprintf(s,"<%s",key);
	if ((q = strstr(src,s))!=NULL) {
		while(*q)
			if (*q++ == '>')
				break;
		sprintf(s,"</%s>",key);
		if ((r = strstr(q,s))!=NULL) {
			for(; q < r ; *s++ = *q++);
		}
	}
	*s = '\0';
	return &sepbuf[lvl][0];
}

int in_XML(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
    char *p;
	int lvl;

    while((p=xfgets(rFile))!=NULL) {
		if (strstr(p,"<gpoi>")==NULL)
			continue;
		titleBuf[0] = '\0';
		position->lon = position->lat = 999;
		GPSLogp->head = 0;
        GPSLogp->datTyp &= ~DAT_HED;

		strcpy(&sepbuf[lvl=0][0],p);
		sep(0,"format");
		if (_stricmp(sep(1,"datum"), "tokyo") == 0)
            GPSLogp->datTyp |= DAT_TKY;
		if (_stricmp(sep(1,"unit"), "degree") == 0)
            GPSLogp->datTyp |= DAT_DEG;
			
		lvl = 0;
		sep(lvl++,"gpoi");
		sep(lvl++, "poi");
		if ((p = strstr(sep(lvl  ,  "note"),"deg="))!=NULL) {
			GPSLogp->head = atoi(p+4);
            GPSLogp->datTyp |= DAT_HED;
		}
		sep(lvl++,  "name");
		strcpy(titleBuf, sep(lvl--,   "nb"));
		sep(lvl++,  "point");
		sep(lvl++,   "pos");
		position->lat = atof(sep(lvl  ,    "lat"));
		position->lon = atof(sep(lvl--,    "lon"));
		sep(lvl=1, "address");
		sep(++lvl,  "address-text");
		sep(lvl=1, "category");
		sep(lvl++, "linkfile");
		sep(lvl++,  "image");
		sep(lvl  ,   "comment");
		if ((titleBuf[0] != '\0') && (position->lat != 999) && (position->lon != 999))
			return 1;
	}
	return 0;
}
