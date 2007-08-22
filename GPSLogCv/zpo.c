/* zpo.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

 Zenrin Z[Zi]II .ZPO

History:
1999/08/12 V0.11
1999/08/06 V0.05 add Title input
*/
#include <string.h>

#include <fcntl.h>
#include <io.h>

#include "gpslogcv.h"

/* === ZPO in === */
int in_ZPO(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
	char *lp = NULL;
	char *q;
	int seq;

	if (line == 0) {
		if (_setmode( _fileno(rFile), _O_BINARY )== -1 ) {
			perror( "can't set binary mode." );
			return -1;	/* error with errno */
		}
	}
	for(;;) {
		if (lp == NULL)
			if ((lp = xfgets(rFile))==NULL)
				break;
		q = lp;
		lp = NULL;
		if (_strnicmp(q,"Title=",6)==0) {
			char *p;
			for(p = q+6,q = titleBuf;((unsigned char)*p>0x1F) && (q-titleBuf <TTBUFLEN-1);)
				*q++ = *p++;
			*q = '\0';
			GPSLogp->thread = 1;
			continue;
		}
		if (*q++ != 'x')
			continue;
		if ((seq = atoi(q))== 0)
			continue;
		if ((q=strchr(q,'='))==NULL)
			continue;
		position->lon = atof(++q);
		if ((lp=xfgets(rFile))==NULL)
			break;
		q = lp;
		if (*q++ != 'y')
			continue;
		if (atoi(q)!=seq)
			continue;
		if ((q=strchr(q,'='))==NULL)
			continue;
		position->lat = atof(++q);

		if (XErr || errno)
			return -1;
		return 1;
	}
	return 0;
}
