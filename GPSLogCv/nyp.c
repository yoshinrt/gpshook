/* nyp.c - part of gpslogcv
mailto:imaizumi@nisiq.net
http://homepage1.nifty.com/gigo/

  SONY Navin'You .NYP

History:
  2005/07/07 V0.39 UserPoint no-Time bug fix
  2001/03/30 V0.29a support in UserPoint
  2001/03/20 V0.28d rcvStat
  2000/02/21 V0.18 export xfwrite
  2000/01/28 V0.17d bug fix.
  1999/10/28 V0.17c Fspeed is Km/h, not m/h 
  1999/10/26 V0.17b Thread bug fix
  1999/08/23 V0.12 
  1999/08/12 V0.11 
  1999/08/06 V0.07 many..
  1999/08/06 V0.05 add Title output
*/
#include <math.h>
#include <string.h>
#include <ctype.h>
#ifdef WIN32
#include <mbstring.h>
#endif
#include <fcntl.h>
#include <io.h>
#include <direct.h>

#include "gpslogcv.h"
#include "nyp.h"

//#define DEBUG

#if 1
#define TTSTRDFLT "AÅ`B" 
#define MTSTRDFLT "SuperëSçëî≈III" 
#else
#define TTSTRDFLT "%dAÅ`%dB" 
#define MTSTRDFLT "SuperëSçëî≈ÇS ägí£FMêÍóp" 
#endif

#define LATMIN (20*ASCALE)
#define LATMAX (48*ASCALE)
#define LONMIN (120*ASCALE)
#define LONMAX (150*ASCALE)

#define NEXT_LOG 0x01
#define WN_SAME_TIME 0x02
#define IL_TIME 0x10
#define IL_TIME_DIFF 0x20
#define IL_LAT 0x40
#define IL_LAT_DIFF 0x80
#define IL_LON 0x100
#define IL_LON_DIFF 0x200

static NYPInfo0 info0;
static NYPInfo1 info1;
static char mtbuf[MTBUFLEN];

static NYPRecord rec;

/* === NYP out === */
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

static char DMSBuf[4][32];
static int bufidx = 0;

static char *IAng2DMS(dword IAng)
{
    int deg,min,sec;
	
    bufidx = (++bufidx)%4;
    deg = (int)(IAng/ASCALE);
    IAng = (IAng%ASCALE)*60;
    min = (int)(IAng/ASCALE);
    IAng = (IAng%ASCALE)*60;
    sec = (int)(IAng/ASCALE);
    IAng = (IAng%ASCALE)/(ASCALE/1000);
    sprintf(DMSBuf[bufidx],"%d%02d%02d.%03ld",deg,min,sec,IAng);
    return DMSBuf[bufidx];
}

static unsigned long dirPos,sectionPos,sectionLen;
static dword numRec;

static void getHeader(FILE *rFile)
{
    xfseek(rFile,sectionPos,SEEK_SET);
    xfread(&info0,sizeof(info0),1,rFile);
    info0.ttlen = dswab(info0.ttlen);     /* map title length */
    xfread(titleBuf,sizeof(char),(size_t)info0.ttlen,rFile);
    titleBuf[info0.ttlen]='\0';
	
    xfread(&info1,sizeof(info1),1,rFile);
    info1.day = dswab(info1.day);
    info1.hour= dswab(info1.hour);
    info1.min = dswab(info1.min);
    info1.sec = dswab(info1.sec);
    info1.dist = dswab(info1.dist);
    info1.date = dswab(info1.date) + _timezone; /* convert to UTC */
    info1.lonStart = dswab(info1.lonStart);
    info1.latStart = dswab(info1.latStart);
    info1.lonEnd = dswab(info1.lonEnd);
    info1.latEnd = dswab(info1.latEnd);
    info1.size = dswab(info1.size);
    info1.mtlen = dswab(info1.mtlen);     /* map title length */
    xfread(mapTtlBuf,sizeof(char),(size_t)info1.mtlen,rFile);
    mapTtlBuf[info1.mtlen]='\0';
    xfread(&numRec,sizeof(numRec),1,rFile);
    numRec = dswab(numRec);      /* number of records */
    readRec = 0;
}

static void listHeader(FILE *msgOut)
{
    struct tm *tmp;
    time_t t;
	
    fprintf(msgOut,"---\n%s(%ld records) on %s\n",titleBuf,numRec,mapTtlBuf);
    if ( (tmp = localtime(&info1.date))!=NULL )
        fprintf(msgOut,"Date: %s",ctime(&info1.date));
    fprintf(msgOut,"N%s E%s -> N%s E%s\n",
        IAng2DMS(info1.latStart),IAng2DMS(info1.lonStart),
        IAng2DMS(info1.latEnd),IAng2DMS(info1.lonEnd));
    fprintf(msgOut,"Mileage:%3.1fKm, Running time:%dday(s) %02dh%02dm%02ds\n"
        ,(float)info1.dist/1000.0,info1.day,info1.hour,info1.min,info1.sec);
    t = (((info1.day*24)+info1.hour)*60+info1.min)*60+info1.sec;
    fprintf(msgOut,"Average Speed:");
    if (t != 0)
        fprintf(msgOut,"%3.1fKm/h\n",((float)info1.dist*3.6)/(float)t);
    else
        fprintf(msgOut,"N/A\n");
}

int adjFlag;

static int monadj[] = {
    /*  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 */
	31,31,28,31,30,31,30,31,31,30,31,30
};

static time_t adj(time_t te,time_t te0,int adj)
{
    int d;
    time_t dadj1;
    struct tm *tmp;
	
    if (adj == -1)
        return te0;
    if (te == -1)
        return te0;
	
    tmp = gmtime(&te);
    if (tmp == NULL)
        return te0;
	
    if (tmp->tm_mon == 2)
        monadj[2] = (tmp->tm_year%4 == 0)? 29:28;
    d = monadj[(tmp->tm_mon+1)%12] - monadj[tmp->tm_mon];
    if ((d > 0) && (tmp->tm_mday <= d)) { /* twoMeanDate, select one */
        if (adj < 2) {
            dadj1 = te - monadj[tmp->tm_mon-1]*86400L;
            if (dadj1 >= te0)
                return dadj1;
        }
    }
    dadj1 = te - (long)monadj[tmp->tm_mon]*86400L;
    if (dadj1 < te0)
        return 0;
    return dadj1;
}

static time_t t;
static time_t tRec0;
static dword ILat0,ILon0;

static chkRec(cnvData_t *GPSLogp)
{
    int s = 0;
    time_t tRec;
    dword ILat,ILon;
	
    tRec = dswab(rec.IrecTime);
	
	ILat = dswab(rec.ILat);
    ILon = dswab(rec.ILon);

    if (tRec > 0) {
		
        if (GPSLogp->thread == 0) {
#if 0
            if ( (tRec-tRec0)<0 )
                s |= IL_TIME_DIFF;  /* mark next record */
#endif
            if ( (ILat < LATMIN) || (ILat > LATMAX) )
                s |= IL_LAT;  /* mark next record */
            if ( (ILon < LONMIN) || (ILon > LONMAX) )
                s |= IL_LON;  /* mark next record */
#if 0
            if ( tRec == tRec0 ) {
                s |= WN_SAME_TIME;  /* mark next record */
            } else {
                if ( labs(ILat-ILat0)/(tRec-tRec0) > 0x1a300)
                    s |= IL_LAT_DIFF;  /* mark next record */
                if ( labs(ILon-ILon0)/(tRec-tRec0) > 0x1a300)
                    s |= IL_LON_DIFF;  /* mark next record */
            }
#endif
        }
    } else {
        s |= IL_TIME;  /* mark next record */
    }
    if (s == 0) {
        tRec0 = tRec;
        ILat0 = ILat;
        ILon0 = ILon;
    }
#if 0
    if (s)
        fprintf(stderr,"%04x\n",s);
#endif
    return s;
}

//#define DEBUG
#ifdef DEBUG
static word LastStat;
#endif

static cvt_NNN(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
    dword spd;
    time_t lt1,lt2,st1,st2;

    if (readRec == 0) {
        long curPos;
		
        xfseek(rFile,sectionPos,SEEK_END);
        getHeader(rFile);
        tRec0 = t = 0;
        adjFlag = -1;
        curPos = ftell(rFile);
        fseek(rFile,sectionPos+sectionLen-sizeof(NYPRecord),SEEK_SET);
        xfread(&rec,sizeof(rec),1,rFile);
        if (((GPSTyp == GPS_UNK) && (info1.date!=0) && (info1.date+3600*24*10 < (time_t)dswab(rec.IrecTime))) 
            || (GPSTyp == GPS_IPS)){
            adjFlag = 0;
            fseek(rFile,curPos,SEEK_SET);
            while(readRec < numRec) {
                xfread(&rec,sizeof(rec),1,rFile);readRec++;
                if (chkRec(GPSLogp)==0)
                    break;
            }
            if (readRec >= numRec) 
                return XErr = -1;

            st1 = lt1 = adj(dswab(rec.IrecTime),0,1);
            st2 = lt2 = adj(dswab(rec.IrecTime),0,2);
            if (st1 == st2) {
                t = st1;
            } else {
                while(readRec < numRec) {
                    xfread(&rec,sizeof(rec),1,rFile);readRec++;
                    if (chkRec(GPSLogp)!=0)
                        continue;
					
                    if (lt1)
                        lt1 = adj(dswab(rec.IrecTime),lt1,0);
                    if (lt2)
                        lt2 = adj(dswab(rec.IrecTime),lt2,0);
                }
                if (info1.date < lt2)
                    lt2 = 0;
                if ((lt1==0) && (lt2==0)) {
                    fprintf(stderr,"* can't IPS adjust.\n");
                    adjFlag = -1;
                } else {
                    if (lt2 == 0)
                        t = st1;
                    else if (lt1 != 0) {
                        lt1 = (info1.date > lt1) ? info1.date-lt1:lt1-info1.date;
                        lt2 = (info1.date > lt2) ? info1.date-lt2:lt2-info1.date;
                        t = (lt1<=lt2)? st1:st2;
                    }
                }
            }
        }
        readRec = 0;
        fseek(rFile,curPos,SEEK_SET);
    }
	
    while(readRec < numRec) {
        xfread(&rec,sizeof(rec),1,rFile);readRec++;
        if (chkRec(GPSLogp)!=0)
            continue;
		if (adjFlag == -1)
	        t = dswab(rec.IrecTime);
		else
	        t = adj(dswab(rec.IrecTime),t,adjFlag);
        *GPSLogp->recTime = *localtime(&t); /* use local time but result is UTC */
        position->lat = (double)dswab(rec.ILat)/ASCALE;
        position->lon = (double)dswab(rec.ILon)/ASCALE;
        spd = dswab(rec.Fspeed);
        GPSLogp->speed = (double)*((float *)&spd)/1000;      
        GPSLogp->head = (double)dswab(rec.IAng)/ASCALE;
		GPSLogp->rcvStat = (char)(wswab(rec.Ssat)&0xf);
		if (GPSLogp->rcvStat)
			GPSLogp->rcvStat++;
#ifdef DEBUG
		if ((GPSLogp->recTime->tm_min >= 0) && (GPSLogp->recTime->tm_sec >= 0))
			printf("%02d:%02d:%02d %02x,%6d,%6d,%7f\n",GPSLogp->recTime->tm_hour,GPSLogp->recTime->tm_min,GPSLogp->recTime->tm_sec,wswab(rec.Ssat),(short)wswab(rec.LatAdj),(short)wswab(rec.LonAdj),GPSLogp->speed);
//		if (rec.Ssat != LastStat)
//			fprintf(stderr,"%d:%02x,%04x,%04x\n",readRec,wswab(rec.Ssat),wswab(rec.LatAdj),wswab(rec.LonAdj));
//		LastStat = rec.Ssat;
#endif
        return 1;
    }
    if (listFlag)
        listHeader(stdout);
    readRec = 0;
    return 0;
}

int in_NNN(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
    if (readRec == 0) {
        if (_setmode( _fileno(rFile), _O_BINARY )== -1 ) {
            perror( "can't set binary mode." );
            return -1;	/* error with errno */
        }
        xfseek(rFile,sectionPos = 0L,SEEK_END);
        sectionLen = ftell(rFile);
    }
    return cvt_NNN(rFile, position, GPSLogp);
}

static NYPDirDesc DirDesc;

static NYPDirEnt DirEnt;

typedef struct NYPDirEntList_t {
    unsigned long dirPos;
    time_t  date;
} NYPDirEntList;

#define MAXDIR 2048
static NYPDirEntList DirEntList[MAXDIR];

static int logTimeCmp(const void *e0,const void *e1)
{
    return (((NYPDirEntList *)e0)->date - ((NYPDirEntList *)e1)->date);
}

static int numEntry  = 0;
static int nypTyp  = 0;

int in_NUP(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
	char *p, *q;
	dword u;
	
	if (readRec++ == 0) {
		if (_setmode( _fileno(rFile), _O_TEXT)== -1 ) {
			perror( "can't set text mode." );
            return -1;	/* error with errno */
		}
	}

	telBuf[0] = '\0';
    GPSLogp->datTyp &= ~(DAT_TIM|DAT_JST);

	while((p=xfgets(rFile))!=NULL) {
		if (strstr(p,"eoNPOIs@NRA")!=NULL) {
			minTime = INT_MIN;
			return 1;
		} else if (*(p+1) == '$') {
			dword lat = 0;
			dword lon = 0;
			if (strchr(p,';')==NULL)
				return XErr = -1;
			p+=2;
			do {
				u = (isalpha(*p))? toupper(*p)-'A'+10:*p-'0';
				lat <<= 2;
				lon <<= 2;
				if (u&8)
					lat |= 2;
				if (u&4)
					lon |= 2;
				if (u&2)
					lat |= 1;
				if (u&1)
					lon |= 1;
				p++;
			} while(isxdigit(*p));
			position->lat = (double)lat / ASCALE;
			position->lon = (double)lon / ASCALE;
		} else if (strstr(p," TTL")!=NULL) {
			if ((p=strchr(p,'{'))!=NULL) {
				strcpy(titleBuf,p+1);
				if ((p=strrchr(titleBuf,'}'))!=NULL) {
					*p = '\0';
				}
			}
		} else if (strstr(p," TEL")!=NULL) {
			if ((p=strchr(p,'='))!=NULL) {
				strcpy(telBuf,p+2);
				if ((p=strrchr(telBuf,';'))!=NULL) {
					*p = '\0';
				}
			}
		} else if (strstr(p," ADDR")!=NULL) {
			if ((p=strchr(p,'{'))!=NULL) {
				strcpy(addrBuf,p+1);
				if ((p=strrchr(addrBuf,'}'))!=NULL) {
					*p = '\0';
				}
			}
		} else if (strstr(p,"_JST")!=NULL) {
			GPSLogp->recTime->tm_year = atoi(strtokX(p,"/"))-1900;
			GPSLogp->recTime->tm_mon = atoi(strtokX(NULL,"/"))-1;
			GPSLogp->recTime->tm_mday = atoi(strtokX(NULL,"_"));
			GPSLogp->recTime->tm_hour = atoi(strtokX(NULL,":"));
			GPSLogp->recTime->tm_min = atoi(strtokX(NULL,":"));
			GPSLogp->recTime->tm_sec = atoi(strtokX(NULL,"_"));
            GPSLogp->datTyp |= DAT_TIM|DAT_JST;
		} else if ((q=strstr(p," = "))!=NULL) {
			/* ignore */
		} else if (*p == ' ') {
			icon=atoi(p);
			if ((p=strchr(p,'.'))!=NULL) {
				icon += atoi(++p)*256;
				if ((p=strchr(p,'.'))!=NULL) {
					icon += atoi(++p)*65536;
				}
			}
#if 0 /* dbg */
			{
				int i;

				for (i = 0;icontable[i].icon_type; i++) {
				if (icontable[i].icon_type == icon)
					printf("%s\n",icontable[i].icon_name);
				}
			}
#endif
		}
	}
	return 0;
}

void out_NUP(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
	dword lat = (dword)(position->lat * ASCALE);
	dword lon = (dword)(position->lon * ASCALE);
	char iso[18];
	int i;

	iso[17] = 0;
	iso[0] = '$';

	for (i = 16; i ; --i) {
		char c = 0;
		if (lat & 2)
			c |= 8;
		if (lon & 2)
			c |= 4;
		if (lat & 1)
			c |= 2;
		if (lon & 1)
			c |= 1;
		iso[i] = (char)((c < 10) ? c + '0':c+'A'-10);
		lat >>= 2;
		lon >>= 2;
	}
	fprintf(out,"NPOIs100@NRA/R1;\n");
	fprintf(out," %d/%d/%d_%02d:%02d:%02d_JST;\n"
	  ,GPSLogp->recTime->tm_year+1900,GPSLogp->recTime->tm_mon+1,GPSLogp->recTime->tm_mday
	  ,GPSLogp->recTime->tm_hour,GPSLogp->recTime->tm_min,GPSLogp->recTime->tm_sec);
	fprintf(out," 255;\n");
	fprintf(out," %s;\n",iso);
	fprintf(out," TTL = {%s};\n",titleBuf);
	fprintf(out," CHAR = SJIS;\n");
	fprintf(out," ADDR = {%s};\n",addrBuf);
	if (telBuf[0])
		fprintf(out," TEL = %s;\n",telBuf);

	fprintf(out," HPGL = {};\n");
	fprintf(out," MAIL = {};\n");
	fprintf(out," LNKF = {};\n");
	fprintf(out," COMM = {};\n");
	fprintf(out,"eoNPOIs@NRA\n");
	GPSLogp->thread = 0;
	minTime = INT_MIN; 
	nRec++;
}

int cls_NUP(FILE *out)
{
	putc(0x1a,out);
	return fclose(out);
}

int in_NYP(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
{
    for (;;){
        if (readRec == 0) {
            if (nEntry == 0) {
                int i;
				
				{
					char *p;
					if ((p=xfgets(rFile))!=NULL) {
						if ((strstr(p,"NPOIs"))!=NULL) {
							xfseek(rFile,0L,SEEK_SET);
							newType = "nypu";
							return XErr = -2;
						}
					}
				}

                if (_setmode( _fileno(rFile), _O_BINARY )== -1 ) {
                    perror( "can't set binary mode." );
                    return -1;	/* error with errno */
                }
                GPSLogp->datTyp &= ~DAT_DST;
				
                DirDesc.IDString[0] = '\0';
                if (fseek(rFile,- (long)sizeof(DirDesc),SEEK_END) == 0)
                    xfread(&DirDesc,sizeof(DirDesc),1,rFile);
                if (strncmp(&DirDesc.IDString[0],"  SONY ITC NYP",14)!=0) {
                    fprintf(stderr,"\n*Error - Unknown format file !\n");
                    return XErr = -1;
                }
                dirPos = dswab(DirDesc.DirPosBigI);
                xfseek(rFile,dirPos,SEEK_SET);
                i = 0;
                for (;;){
                    time_t t0=0xffffffff,t1=0;
                    xfread(&DirEnt,sizeof(DirEnt),1,rFile);
                    sectionPos = dswab(DirEnt.SectionPosBigI);
                    sectionLen = dswab(DirEnt.SectionLenBigI);
                    if (sectionLen == 0L)
                        break;
                    if (DirEnt.Unknown[2] != 0x2121)
                        continue;
                    if (DirEnt.Unknown[1] == 0x3144)
						nypTyp = 1;
                    else if (DirEnt.Unknown[1] == 0x3155) {
						xfseek(rFile,0L,SEEK_SET);
						newType = "NYPu";
	                    dirPos = ftell(rFile);
						continue;
					} else {
						fprintf(stderr,"Warning: ignore unknown data Type.\n");
                        continue;
					}
                    DirEntList[i].dirPos = dirPos;
                    dirPos = ftell(rFile);
                    getHeader(rFile);
                    DirEntList[i].date = info1.date;
                    if (info1.date < t0) {
                        strcpy(ttbuf,titleBuf);
                        t0 = info1.date;
                    }
                    if (t1 < info1.date) {
                        strcpy(mtbuf,titleBuf);
                        t1 = info1.date;
                    }
                    xfseek(rFile,dirPos,SEEK_SET);
                    if (++i >= MAXDIR){
                        fprintf(stderr,"\n*Error - Too many section in file !\n");
                        return XErr = -1;
                    }
                }
                qsort(DirEntList,i,sizeof(NYPDirEntList), logTimeCmp);
                numEntry = i;
#ifdef WIN32
                if (mergeFlag) {
                    char *p;
                    if ((p = (char *)_mbsstr((const unsigned char *)ttbuf,(const unsigned char *)"Å`"))==NULL)
                        strcat(ttbuf,"Å`");
                    else
                        *(p+2) = '\0';
                    if ((p = (char *)_mbsstr((const unsigned char *)mtbuf,(const unsigned char *)"Å`"))==NULL)
                        strcat(ttbuf,p+2);
                    else
                        strcat(ttbuf,mtbuf);
                }
#endif
            }
			
            if (nEntry<numEntry){
                xfseek(rFile,DirEntList[nEntry].dirPos,SEEK_SET);
                xfread(&DirEnt,sizeof(DirEnt),1,rFile);
                sectionPos = dswab(DirEnt.SectionPosBigI);
                sectionLen = dswab(DirEnt.SectionLenBigI);
                GPSLogp->thread = 1;
                nEntry++;
            } else {
                nEntry = numEntry = 0;
                return 0;
            }
        }
        if (cvt_NNN(rFile, position, GPSLogp))
            break;
    }
    return 1;
}

static dword fn;

static void mkDrvLogDirEnt()
{
    info0.si = wswab(1);
    info0.fn = dswab(++fn);
    info0.ttlen = dswab((dword)strlen(ttbuf));
    info1.day = dswab(info1.day);
    info1.hour= dswab(info1.hour);
    info1.min = dswab(info1.min);
    info1.sec = dswab(info1.sec);
    info1.dist = dswab(info1.dist);
    info1.date = dswab(info1.date);
	/*  info1.lonStart = dswab(info1.lonStart);
    info1.latStart = dswab(info1.latStart);
    info1.lonEnd = dswab(info1.lonEnd);
    info1.latEnd = dswab(info1.latEnd);
	*/
    info1.size = dswab(info1.size);
    info1.mtlen = dswab((dword)strlen(mtbuf));
}

static time_t tStart;
static word nSection;

void out_NNN(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
    float ftemp;
	
    rec.ILon = dswab((dword)(position->lon*ASCALE));
    rec.ILat = dswab((dword)(position->lat*ASCALE));
    rec.IrecTime = dswab(t = mktime(GPSLogp->recTime)/*- _timezone*/ );
	
    if (mergeFlag==0){
        if (titleBuf[0])
            strcpy(ttbuf,titleBuf);
    }
    if (mapTtlBuf[0])
        strcpy(mtbuf,mapTtlBuf);
    titleBuf[0] = mapTtlBuf[0] = '\0';
	
    if (nRec == 0) {
        tStart = t;
		
        info1.dist = 0;
        info1.lonStart = rec.ILon;
        info1.latStart = rec.ILat;
		
        rec.LonAdj = 0;
        rec.LatAdj = 0;
		rec.Ssat = wswab(2);

        if (_setmode( _fileno(out), _O_BINARY )== -1 ) {
            perror( "can't set binary mode." );
            XErr++;
        }
        dirPos = ftell(out);
        /* Header length must be fixed at this point !!! */
        if (ttbuf[0]=='\0')
            sprintf(ttbuf,TTSTRDFLT,fn,fn);/*"%dAÅ`%dB"*/
        if (mtbuf[0]=='\0')
            strncpy(mtbuf,MTSTRDFLT,MTBUFLEN-1);/*"SuperëSçëî≈ÇS ägí£FMêÍóp"*/
        if (nSection >= MAXDIR){
            fprintf(stderr,"\n*Error - Too many section in file !\n");
            XErr++;
            return;
        }
        DirEntList[nSection].dirPos = ftell(out);
        xfseek(out,sizeof(NYPInfo0)+strlen(ttbuf)+sizeof(nRec)+sizeof(NYPInfo1)+strlen(mtbuf),SEEK_CUR);
    }
    rec.IAng = dswab((dword)(GPSLogp->head*ASCALE));
    info1.dist += (dword)GPSLogp->dist;
    ftemp = (float)(GPSLogp->speed*1000);
    rec.Fspeed = dswab(*((dword *)(&ftemp)) );

	if (GPSLogp->datTyp & DAT_STA)
		if (GPSLogp->rcvStat)
			rec.Ssat = wswab((word)(GPSLogp->rcvStat-1));
		else
			rec.Ssat = 0;
	else
		rec.Ssat = wswab(2);
	
    xfwrite(&rec,sizeof(rec),1,out);
    nRec++;
}

static void close_section(FILE *out)
{
    int n;
	
    info1.lonEnd = rec.ILon;
    info1.latEnd = rec.ILat;
    info1.date = t - _timezone;
	
    t = t-tStart;
    info1.day = t/(3600*24); t = t%(3600*24);
    info1.hour = t/3600; t = t%3600;
    info1.min = t/60;
    info1.sec = t%60;
    info1.size = ftell(out)-dirPos;
	
    mkDrvLogDirEnt();
    xfseek(out,DirEntList[nSection++].dirPos,SEEK_SET);
    xfwrite(&info0,sizeof(info0),1,out);
    xfwrite(ttbuf,strlen(ttbuf),1,out);
    xfwrite(&info1,sizeof(info1),1,out);
    xfwrite(mtbuf,strlen(mtbuf),1,out);
	
    n = dswab(nRec);
    xfwrite(&n,sizeof(n),1,out);
    xfseek(out,0L,SEEK_END);
	/*  sectionLen = ftell(out)-sectionPos; */
}

#define MAXDIRBUF 512
static NYPDirEnt DirBuf[512];

void out_NYP(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
    if (ftell(out)==0L) {
        xfseek(out,sizeof(char),SEEK_SET);
        nSection = 0;
    } else if (titleBuf[0]) {   /* new title */
        if ((nRec) && (mergeFlag==0)){
            close_section(out);
            ttbuf[0] = mtbuf[0]= '\0';
            fprintf(stderr,"%d+",nRec);
            nRec = 0;
        }
    }
    out_NNN(out, position, GPSLogp);
    GPSLogp->thread = 0;
}

int cls_NYP(FILE *out)
{
    int i;
	
    close_section(out);
	
    memset(&DirEnt,0,sizeof(DirEnt));
    DirEnt.Unknown[1] = 0x3144;
    DirEnt.Unknown[2] = 0x2121;
    DirEntList[nSection].dirPos = ftell(out);
	
    for (i = 0 ; i <nSection ; i++) {
        DirEnt.SectionPosBigI = dswab(DirEntList[i].dirPos);
        DirEnt.SectionLenBigI = dswab(DirEntList[i+1].dirPos-DirEntList[i].dirPos);
        xfwrite(&DirEnt,sizeof(DirEnt),1,out);  /* add section directory */ 
    }
    /* add dir Desc */
    memset(&DirDesc,0,sizeof(DirDesc));
    DirDesc.Unknown1 = wswab(0x010);
    DirDesc.Unknown3 = wswab(0x100);
    strncpy(&DirDesc.IDString[0],"  SONY ITC NYP",14);
    DirDesc.numSect = wswab(nSection);
    DirDesc.DirPosBigI = dswab(DirEntList[i].dirPos);
    xfwrite(&DirDesc,sizeof(DirDesc),1,out);
    xfseek(out,0L,SEEK_SET);
    fputc(0x1A,out);
    xfseek(out,0L,SEEK_END);
    fclose(out);
    return 0;
}
