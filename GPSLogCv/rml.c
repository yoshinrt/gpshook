/* rml.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

 Increment-P MapFan V .rml

History:
  2001/11/08 V0.33a use thread.
  2001/05/03 initial release
*/
#include <string.h>
#include <locale.h>
#include "gpslogcv.h"

/* === in rml === */
/* no thread,alitude,time */

#ifdef VCPP
#pragma warning( disable: 4100)
#endif

static unsigned char inBuf[256];

wchar_t utf8getc(unsigned char **p)
{
	unsigned char c1 = *(*p)++;
	wchar_t code;
	if((c1 & 0x80) == 0)
		code = c1;
	else{
		unsigned char c2 = *(*p)++;
		if((c1 & 0xE0) == 0xC0){
			code = (wchar_t)(((c1 & 0x1F) << 6) |  (c2 & 0x3F));
		}else{
			if((c1 & 0xF0) == 0xE0){
				unsigned int c3 = *(*p)++;
				code = (wchar_t)(((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F));
			}else{
				code= 0;
			}
		}
	}
	return(code);
}

static char * yfgets(FILE *in)
{
	unsigned char *p = NULL;
	int c;

	for(;;) {
		c = getc(in);
		if (c==EOF)
			return NULL;
		if (p == NULL) {
			if (c =='<')
				p = inBuf;
		} else {
			if (c == '>') {
				*p = '\0';
				line++;
				return (char *)inBuf;
			}
			*p++ = (char) c;
			if (p-inBuf > 255)
				p = NULL;
		}
	}
}

int in_RML(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
	char *p,*q;
	wchar_t c,*mbp,mbbuf[32];

	while((p = yfgets(rFile))!=NULL) {
		if (line == 1) {
			if (strcmp((char *)p,"?xml version=\"1.0\"?")) {
				fprintf(stderr,"*Error - Unexpected format !\n");
				return XErr = -1;
			}
			continue;
		}
		if ((q = strstr(p,"\"Name\""))!= NULL) {
			if ((q = strstr(p,"value=\""))!= NULL) {
				q += 7;
				mbp = mbbuf;
				setlocale(LC_CTYPE,"jpn");
				while((c = utf8getc(&(unsigned char *)q))!=0) {
					if (c == 0x22)
						break;
					*mbp++ = c;
//					wctomb(titleBuf,c);
				}
				*mbp = '\0';
				wcstombs(titleBuf,mbbuf,TTBUFLEN-1);
			}
		}
		if ((q = strstr(p,"node longitude=\""))== NULL) {
			GPSLogp->thread = 1;	/* V0.33a */
			continue;
		}
		position->lon = (double)atol(q+16)/(256*3600);
		if ((q = strstr(q,"latitude=\""))== NULL)
			continue;
		position->lat = (double)atol(q+10)/(256*3600);	
		return 1;
	}
	return 0;
}

#ifdef VCPP
#pragma warning( default: 4100)
#endif