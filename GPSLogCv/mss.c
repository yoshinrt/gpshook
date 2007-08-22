/* mss.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

 ProAtlas MapServerScript

History:
2001/03/18 V0.44 by masaaki@attglobal.net
2001/03/06 V0.43 by masaaki@attglobal.net
2001/03/01 V0.4  by masaaki@attglobal.net
2000/02/27 V0.3  by masaaki@attglobal.net
2000/02/21 V0.2
2000/02/21 V0.1 rev 0.01 by masaaki@attglobal.net
2000/02/19 V0.1
*/
#include <string.h>
#include <io.h>
#include <math.h>

#include "gpslogcv.h"

#define POSLEN 2000

/* MSS Options */
#define NoString       0x01 //0000 0001 = 1
#define CountPoints    0x02 //0000 0010 = 2
#define DispTime       0x04 //0000 0100 = 4
#define TextSizeSmall  0x08 //0000 1000 = 8
#define UniqueColor    0x40 //0100 0000
#define UniqueColorRGB 0x30 //0011 0000
#define UniqueColorRed 0x40 //0100 0000 = 64
#define UniqueColorGr  0x50 //0101 0000 = 80
#define UniqueColorBl  0x60 //0110 0000 = 96
#define DispPoints     0x80 //1000 0000 =128

static unsigned int npnt,nthr; // npnt = number of point   nthr = number of thread
static int noid; // noid = number of oid
static double lat,lon;
static struct tm start_recTime;
static struct tm end_recTime;

static char *rgb[] = {"255,0,0","0,255,0","0,0,255","0,0,0","255,255,255"} ;
#define RGB_red   0
#define RGB_Green 1
#define RGB_Blue  2
#define RGB_Black 3
#define RGB_White 4

static char *linestyle  ="line#0";
static char *linewidth  ="3";
static char *textoff    ="1000000"; //ProAtlas can't yet understand this parameter.
static char *textstyle  ="frame#2";
static char *textsize[] ={"","10"};
static char *pointstyle ="none"; 
static char *areastyle  ="fill";

#define CIRCLE_r    200
#define CIRCLE_rmax 6

static char *makAngStr(double loc)
{
	int sgn = (int)loc;
	int deg,min,sec;
	long IAng;
	
	IAng = (long)(fabs(loc)*36000+0.5);
	
	deg = (int)(IAng/36000);
	min = (int)((IAng%36000)/600);
	sec = (int)(IAng%600);
	
	if (sgn >0)
		sprintf(angbuf,"%3d/%02d/%02d.%d",deg,min,sec/10,sec%10);
	else
		sprintf(angbuf,"%4d/%02d/%02d.%d",-deg,min,sec/10,sec%10);
	
	return angbuf;
}

static void put_linergb(FILE *out,int ngname)
{
	int n;
	
	if ((MSSOpt & UniqueColor) == 0) {
		fprintf(out,"\tlinergb=\"%s\"",rgb[(ngname % 3)]);
	}
	else{
		n = MSSOpt & UniqueColorRGB;
		n = n >> 4;
		fprintf(out,"\tlinergb=\"%s\"",rgb[(n)]);
	}
}

static void put_textsize(FILE *out,int ntextsize)
{
	ntextsize = 0; //dummy
	if ((MSSOpt & TextSizeSmall) == 0) {
		fprintf(out,"\ttextsize=\"%s\"",textsize[0]);
	}
	else{
		fprintf(out,"\ttextsize=\"%s\"",textsize[1]);
	}
}


/* put mps header */
static void put_head(FILE *out,int maxthr)
{
	int ngname,ntextsize; /* added */
	unsigned int gidupper,lastgidupper,gidlower; //added
	
	fprintf(out,"mapserver script ver = \"1.0\";\n");
	fprintf(out,"message str=\"\";\n");
	fprintf(out,"theme str=\"%s\";\n",inNamep);
	lastgidupper = 0;
	if (maxthr <= 0xFF) {
		fprintf(out,"group\tgid=\"%d\" gname=\"tracklog\";\n",lastgidupper);
	}
	else {
		fprintf(out,"group\tgid=\"%d\" gname=\"tracklog%d\";\n",lastgidupper,(lastgidupper+1));
	}
	for(ngname=1;ngname <= maxthr;ngname++) {
		gidupper = ngname>>8;
		gidlower = ngname & 0xFF;
		if (lastgidupper != gidupper){
			lastgidupper++;
			if (lastgidupper == gidupper){
				fprintf(out,"group\tgid=\"%d\" gname=\"tracklog%d\";\n",gidupper,(gidupper+1));
			} else {
				fprintf(stderr,"Can't convert to the mss file!\n");
				XErr = -1;
				return;
			}
		}
		fprintf(out,"group\tgid=\"%d.%d\" gname=\"thread %d\"\n",gidupper,gidlower,ngname);
		if ((MSSOpt & DispPoints)==0){ //put header of line
			put_linergb(out,ngname);
			/*
			fprintf(out,"\tlinergb=\"%s\"",rgb[(ngname % 3)]);
			*/
			fprintf(out," linestyle=\"%s\" linewidth=\"%s\"\n",linestyle,linewidth);
			ntextsize = 0; //dummy
			put_textsize(out,ntextsize);
			fprintf(out," textoff=\"%s\" textstyle=\"%s\" pointstyle=\"%s\"\n",textoff,textstyle,pointstyle);
		}
		else{ //put header of point
			fprintf(out,"\tareargb=\"%s\" areastyle=\"%s\"",rgb[(RGB_red)],areastyle);
			fprintf(out," bodrrgb=\"%s\"\n",rgb[(RGB_red)]);
			fprintf(out,"\ttextsize=\"\" textoff=\"%s\" textstyle=\"%s\"\n",textoff,textstyle);
		}
		fprintf(out,";\n");

	}
}


static const char toMname[]="JAN\0FEB\0MAR\0APR\0MAY\0JUN\0JUL\0AUG\0SEP\0OCT\0NOV\0DEC";
static void put_timeofpoint(FILE *file){
	
	if ((MSSOpt & DispTime) > 0) {	
		fprintf(file,"start_time:%02d-%s-%02d %02d:%02d:%02d--->"
			,start_recTime.tm_mday,&toMname[start_recTime.tm_mon*4],start_recTime.tm_year%100
			,start_recTime.tm_hour,start_recTime.tm_min,start_recTime.tm_sec);
		
		fprintf(file,"end_time:%02d-%s-%02d %02d:%02d:%02d"
			,end_recTime.tm_mday,&toMname[end_recTime.tm_mon*4],end_recTime.tm_year%100
			,end_recTime.tm_hour,end_recTime.tm_min,end_recTime.tm_sec);
	}
}

static void put_line_trailer(FILE *file,char *ttbuf,int npnt)
{
	if ((MSSOpt & NoString) == 0) {
		if ((MSSOpt & CountPoints) > 0) {
			fprintf(file,"\"\nstr=\"%s(%4d)\"\nurl=\"\"\n",ttbuf,npnt);
			fprintf(file,"info=\"");
			put_timeofpoint(file);
			fprintf(file,"\"\n");
			fprintf(file,";\n");
		}
		else{
			fprintf(file,"\"\nstr=\"%s\"\nurl=\"\"\n",ttbuf);
			fprintf(file,"info=\"");
			put_timeofpoint(file);
			fprintf(file,"\"\n");
			fprintf(file,";\n");
		}
	} 
	else{
	/*
	fprintf(file,"\"\nstr=\"\"\nurl=\"\"\ninfo=\"\"\ntextsize=\"\";\n");
		*/
		if ((MSSOpt & CountPoints) > 0) {
			fprintf(file,"\"\nstr=\"\"\nurl=\"\"\ninfo=\"%s(%4d) ",ttbuf,npnt);
			put_timeofpoint(file);
			fprintf(file,"\"\n");
			fprintf(file,";\n");
		}
		else{
			fprintf(file,"\"\nstr=\"\"\nurl=\"\"\ninfo=\"%s ",ttbuf);
			put_timeofpoint(file);
			fprintf(file,"\"\n");
			fprintf(file,";\n");
		}
	}
}

static char temp[9];
static FILE *tFile;
static char tnambuf[_MAX_DIR+_MAX_FNAME+_MAX_EXT];
static unsigned int gidupper,gidlower;
static char toStatus[][4]={"N/A","Lost","2D","3D","DGPS"};

void out_MSS(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
	word r,rmax;
	
	if (nRec == 0) {
		char *t,*td;
		nthr = npnt = 0;
		noid = 0; /* added */
		/* making temporary file */
		if ((td = getenv("TEMP")) == NULL)
			td = getenv("TMP");
		strcpy(temp,"GLCXXXXXX");
		if ((t = mktemp(temp)) == NULL) {
			fprintf(stderr,"can't mktemp.\n");
			XErr++;
			return;
		}
		sprintf(tnambuf,"%s\\%s",td,t);
		if ((tFile = fopen(tnambuf,"w+b")) == NULL) {
			fprintf(stderr,"can't open temporary file - '%s'.\n",tnambuf);
			return;
		}
		(void) ftell(out);	/* dummy */
	} else if ((position->lat == lat) && (position->lon == lon) && (GPSLogp->thread == 0))
		return;	/* skip meaningless point */
	lat = position->lat;	/* save last point */
	lon = position->lon;
	
	if ((MSSOpt & DispPoints)==0){ //put line
		if ( (GPSLogp->thread) || ((npnt % (POSLEN+1)) == POSLEN) ) { 
			/* new section required */
			if (npnt)	/* need trailer ? ---> Yes*/
			{
				put_line_trailer(tFile,ttbuf,npnt);
			}
			npnt = 0;	/* clear number of point in section */
			start_recTime = *(GPSLogp->recTime);
			if (GPSLogp->thread) {
				/* new thread */
				++nthr;
				strncpy(ttbuf,titleBuf,TTBUFLEN-1);
				titleBuf[0] = '\0';
			}
			fprintf(tFile,"line oid=\"%d\"",++noid);
			if ((npnt % (POSLEN+1)) == POSLEN){ /* tie last point and new point */
			
			}
			gidupper = nthr>>8;
			gidlower = nthr & 0xFF;
			if (gidupper <= 0xFF){
				fprintf(tFile," gid=\"%d.%d\"",gidupper,gidlower);
				fputs(" pos=\">",tFile);
			} else {
				fprintf(stderr,"thread is over 0xFFFF.Can't convert to the mss file!\n");
				XErr = -1;
				return;
			}
			
		} else 
			fprintf(tFile,",");	/* data is continued */
	
		if ((npnt++ % 3) == 0)	/* need new line ? */
			fprintf(tFile,"\n\t");
		end_recTime = *(GPSLogp->recTime);
	
		fprintf(tFile,"%s",makAngStr(position->lat)+1);	/* data */
		fprintf(tFile,",%s",makAngStr(position->lon));
	}
	else{  //put point
		if (GPSLogp->thread) {
			/* new thread */
			++nthr;  /* nthr >=1 */
			strncpy(ttbuf,titleBuf,TTBUFLEN-1);
			titleBuf[0] = '\0';
			gidupper = nthr>>8;
			gidlower = nthr & 0xFF;
			if (gidupper <= 0xFF){
			} 
			else {
				fprintf(stderr,"thread is over 0xFFFF.Can't convert to the mss file!\n");
				XErr = -1;
				return;
			}
		}
		fprintf(tFile,"circle oid=\"%d\"",++noid);
		fprintf(tFile," gid=\"%d.%d\"",gidupper,gidlower);
		fputs(" pos=\"",tFile);
		fprintf(tFile,"%s",makAngStr(position->lat)+1);	/* data */
		fprintf(tFile,",%s",makAngStr(position->lon));
		fputs("\"",tFile);
		r = CIRCLE_r;
		rmax = CIRCLE_rmax;
		fprintf(tFile," r=\"%d\" rmax=\"%d\"\nstr=\"\"\n",r,rmax);
		if ((GPSLogp->rcvStat == 1)||(GPSLogp->rcvStat == 2))
			fprintf(tFile,"areargb=\"%s\" bodrrgb=\"%s\"\n",rgb[(RGB_Black)],rgb[(RGB_Black)]);
		if(GPSLogp->rcvStat == 4) 
		/* in_NME() does support ? */
			fprintf(tFile,"areargb=\"%s\" bodrrgb=\"%s\"\n",rgb[(RGB_Blue)],rgb[(RGB_Blue)]);
		fprintf(tFile,"info=\"");
		fprintf(tFile,"Time(JST):%02d:%02d:%02d",
			GPSLogp->recTime->tm_hour,GPSLogp->recTime->tm_min,GPSLogp->recTime->tm_sec
			);
		if (GPSLogp->speed >= 0) 
			fprintf(tFile,"---Speed:%6.1fkm/h",GPSLogp->speed);
		if (GPSLogp->rcvStat > 0) {
			fprintf(tFile,"---Satellite:%s",&toStatus[GPSLogp->rcvStat]);
			if (GPSLogp->satdatp)
				fprintf(tFile,"(%02d/%02d)",
					GPSLogp->satdatp->usedSat,GPSLogp->satdatp->numSat);
		}
		fprintf(tFile,"\"\n;\n");

	}
	
	GPSLogp->thread = 0;
	nRec++;
}

int cls_MSS(FILE *out)
{
	char *p;
	
	if (nthr > 0xFFFF){ //check max thread
		fprintf(stderr,"thread is over 0xFFFF.Can't convert to the mss file!\n");
		return XErr = -1;
	}
	
	put_head(out,nthr);	/* put header */
	rewind(tFile);
	while((p = xfgets(tFile))!=NULL) {	/* copy temp. */
		if (fputs(p,out)==EOF) {
			fprintf(stderr,"*Error - Can't copy temp file!\n");
			fclose(tFile);
			return -1;	/* error with errno */
		}
	}
	fclose(tFile);
	unlink(tnambuf);
	if (npnt*((MSSOpt & DispPoints)==0)) // need last trailer ? ---> Yes
		put_line_trailer(out,ttbuf,npnt);
	fprintf(out,"disp all;\nmapserver end;\n");	/* finish */
	
	if (out != stdout) {
		fclose(out);
		out = NULL;
	}
	return 0;
}
