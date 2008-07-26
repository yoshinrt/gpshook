/* gpslogcv.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

  History:
  2005/05/13 V0.37a Remove enbug about .fx
  2005/05/10 V0.37 Support .SMF read.
  2005/05/08 V0.36 Support .XML read.
  2005/05/07 V0.35a Fix .PMF check sum.
@2005/05/06 V0.35 Support Panasonic Car Navigation HS400 .PMF.
  2003/05/02 V0.34o FIX argv[0] option problem.
  2003/04/13 V0.34m ignore time check at thread top.
  2003/04/09 V0.34l -IWGS84,TOKYO option, in out same type allow w/-OR, nme terminate record.
  2002/05/01 V0.34g ignore abnomal when waypoints.
  2001/12/08 V0.33c WonderWitch Waypoint WWP support
  2001/11/12 V0.33b -k noKanji option
  2001/11/08 V0.33a Force last point output
  2001/11/01 V0.33 out_STF
  2001/06/07 V0.31d .tdf in .fx for garlog 02
  2001/05/09 V0.31b gar2ws .fx(1.04b7) i/o support
  2001/05/04 V0.30a fix -XT -XA bug.
  2001/05/03 V0.30 Support Mapfan V route,.rml
  2001/04/13 V0.29b Support Waypoint/Userpoint
  2001/03/20 V0.28d DAT_STA Fix:nyp,
  2001/03/19 V0.28c fix DAT_SPD no effect without DAT_HED
  2001/03/15 V0.28b satellite,receiver information added.(nme,ips)
  2001/03/10 V0.28a .TDF decode bug FIX.
  2001/03/10 V0.28 add .TDF
  2001/03/08 V0.27d MapServerScript enhance
  2001/03/05 V0.27c MapServerScript enhance
  2001/01/22 V0.27b remove .csv enbug
  2001/01/17 V0.27 skip function implemented.
  2001/01/15 V0.26d fix CSVG time bug.
  2000/12/14 V0.26c .pot datum
  2000/11/26 V0.26b some adavance for sgl
  2000/11/16 V0.26a add magnetic declination(by Ed_Williams@compuserve.com) out_nme()
  2000/11/15 V0.26 add in/out_gdn()
             for MSC5, "forcedos d:\bin\c6\binr\cl -DDOS /Ox /Fegdn2nme *.c setargv /link /NOE"
  2000/11/06 V0.25 add out_nme()
  2000/07/19 V0.24b fix west, south hemisphere
  2000/07/17 V0.24a fix sgl definition JST->UTC, change .nme default to DAT_TKY
  2000/07/02 V0.24 add sgl(SONY GPS Log)
  2000/05/09 V0.23 add VCD(kashmir data file)
  2000/04/20 V0.22 change DAT_POT def.(JST->UTC,remove DAT_TIM,DAT_TKY)
  2000/02/23 V0.19a fix temp file template error.
  2000/02/22 V0.19 Alps Map Server Script,IPS8000 support(Not tested yet.)
  2000/02/19 V0.18 Garmin Map Source MPS support
  2000/01/27 V0.17d fix fixed IPS5000 nyp log bug.
  1999/10/25 V0.17c fix nyp bug.
  1999/10/25 V0.17  add in_stf()
  1999/08/31 V0.16
  1999/08/31 V0.15 add options
  1999/08/27 V0.13 Fix no TIME record pot problem.
  1999/08/23 V0.12 add in_pot()
  1999/08/16 V0.11
  1999/08/11 V0.10 add out_nyp()
  1999/08/08 V0.07
  1999/08/06 V0.05 add in_MTK(), out_POT():add alt,date,time, add -U
  1999/08/05 V0.04 add out_POT() (requester:MasaakiSATO)
  1999/08/05 V0.03 add in_LOG() (requester:MasaakiSATO)
  1999/08/05 V0.02 add in_IPS() (requester:MasaakiSATO)
*/
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <math.h>
#include <sys/types.h>
#include <sys/utime.h>

#define MAIN
#include "gpslogcv.h"

static Molodensky_param_t mTokyo = {
    "WGS84toTokyo",
        -739.845,
        -0.10037483e-4,
        128.0,
        -481.0,
        -664.0
};

static Molodensky_param_t mWGS = {
    "TokyoToWGS84",
        739.845,
        0.10037483e-4,
        -128.0,
        481.0,
        664.0
};

static int in_ovr_datum;	/* over_ride input datum */

static int out_datum;       /* default output datum */
static int out_angfmt;      /* default output angfmt */

static int  in_idx;         /*  input type index */
static int out_idx;         /* output type index */

/* flags */
static int timeStamp;       /* time stamp */
static int outFormat;       /* 0=time stamp */
static int noPause=0;
static int noKanji=0;
static int overWriteFlag = 1;
static int ignoreFlag;

/*=====================================================================*/
/* Spec of in_XXX */
/* File handle */
/* get Datum,Datum parameter,angle format */
/* get lat,lon,alt,t,thread */
/* Conversion Routines */
/* datum,datum parameter,angle format */
/* Spec of out_XXX */
/* put Datum,Datum parameter,angle format */
/* get lat,lon,alt,t,thread */

/* === Helper === */

char *strtokX(char *str1,char *delim)
{
    char *p;

    if (XErr)
        return NULL;

    if ((p = strtok(str1,delim))!=NULL)
        return(p);
    fprintf(stderr,"*Error - illegal format(Near Line:%lu)!\n",line);
    exit(1);
//  return NULL;
}

static char mnames[]="   JanFebMarAprMayJunJulAugSepOctNovDec";

int mname2m(char *mname)
{
    char *p,buf[4];
    int mon;

    if (XErr)
        return -1;

    p = mname;
    buf[0]=(char)toupper(*p) ;p++;
    buf[1]=(char)tolower(*p) ;p++;
    buf[2]=(char)tolower(*p) ;
    buf[3]='\0';
    mon = (strstr(mnames,buf)-&mnames[0])/3 -1;
    if (mon >=0 )
        return mon;
    fprintf(stderr,"*Error - illegal Date format(Line:%d)!\n",line);
    XErr++;
    return -1;
}

/* cordinate,dist,direction */

/* Datum conversion */
#if 1
void Molodensky(cordinate_t *position, Molodensky_param_t *datum)
{
	double a, c, ee, f, rn, rm;
	double coslat, coslon, sinlat, sinlon, esnlt2;
	
	sinlat = sin(position->lat);
	coslat = cos(position->lat);
	sinlon = sin(position->lon);
	coslon = cos(position->lon);
	
	a = 6378137.0 - datum->da;
	f =	0.00335281066474 - datum->df;
	c = (1.0 - f);
	ee = f*(2.0 - f);
	
	esnlt2 = sqrt(1.0 - ee * sinlat*sinlat);
	
	rn = a / esnlt2;
	rm = (a * (1.0 - ee)) / (esnlt2*esnlt2*esnlt2);
	
	position->lat +=
		( datum->dz * coslat 
		+ (  datum->da * ee * coslat / esnlt2
		   - datum->dx * coslon
		   - datum->dy * sinlon
		   + datum->df * ((rm /c) + (rn * c)) * coslat
		   ) * sinlat
		)
		/ (rm + position->alt);
	
	position->lon += ( (datum->dy * coslon) - (datum->dx * sinlon) ) / ((rn + position->alt) * coslat);
	
	position->alt += 
		 (datum->dx * coslon
		+ datum->dy * sinlon) * coslat
		+(datum->dz 
		+ datum->df * c * rn * sinlat) * sinlat
		- (datum->da * esnlt2);
}
#else
static void Molodensky(cordinate_t *position, Molodensky_param_t *datum)
{
       double a, b, e, f, rn, rm;
       double coslat, coslon, sinlat, sinlon, esnlt2;

       sinlat = sin(position->lat);
       coslat = cos(position->lat);
       sinlon = sin(position->lon);
       coslon = cos(position->lon);

       a = 6378137.0 - datum->da;
       f =  0.00335281066474 - datum->df;
       b = (1.0 - f) * a;
       e = sqrt(f*(2.0 - f));

       rn = a / sqrt(1.0 - pow(e * sinlat, 2));
       rm = (a * (1.0 - (e * e))) / pow (1.0 - pow(e * sinlat, 2), 1.5);
       esnlt2 = sqrt(1 - e*e * sinlat*sinlat);
       rn = a / esnlt2;
       rm = (a * (1.0 - (e * e))) / (esnlt2*esnlt2*esnlt2);

       position->lat += ( datum->dz * coslat - datum->dx * sinlat * coslon - datum->dy * sinlat * sinlon
           + datum->da * rn * e * e * sinlat * coslat / a
           + datum->df * ((rm * (a/b)) + (rn * (b/a))) * sinlat * coslat)
           / (rm + position->alt);

       position->lon += ( (datum->dy * coslon) - (datum->dx * sinlon) ) / ((rn + position->alt) * coslat);

       position->alt += datum->dx * coslat * coslon + datum->dy * coslat * sinlon + datum->dz * sinlat - (datum->da * a / rn) + datum->df * b / a * rn * sinlat * sinlat;
}
#endif

static double   calc_dist(cordinate_t *pos0rad, cordinate_t *pos, double rpol, double requ){
    double  a,b;

    a=rpol*(pos->lat - pos0rad->lat);
    b=requ*cos((pos->lat + pos0rad->lat)/2)*(pos->lon - pos0rad->lon);
    return(sqrt(a*a+b*b));
}

static double calc_heading(cordinate_t *pos0rad, cordinate_t *pos)
{
    int i;
    double dlon,a,b,c;

    dlon = pos->lon - pos0rad->lon;
    a = sin((pos->lat - pos0rad->lat)/2);
    b = sin(dlon/2);
    a = a*a + cos(pos->lat) * cos(pos0rad->lat) * b*b;
    b = 2 * atan2(sqrt(a), sqrt(1 - a));
    c = (sin(pos->lat) - sin(pos0rad->lat) * cos(b)) / (sin(b) * cos(pos0rad->lat));
    i = (int)c;
    if (i!=0)
        c = i;
    a = acos(c);
    if (errno)
        return 0;
    if (sin(dlon) < 0) {
        a = 2*PI - a;
    }
    return a;
}

/* file and dir */
char *xfgets( FILE *stream )
{
    line++;
    return fgets( lbuf, LBUFLEN, stream );
}
#ifdef DBG_FMAP
#include <malloc.h>
void initbmap(FILE *inFile)
{
    if ((sizeof(unsigned char) != 1 ) || (sizeof(unsigned short int) != 2)||
        (sizeof(unsigned long) != 4)) {
        printf("DBG_FMAP - Illegal dword size.\n" );
        exit(EXIT_FAILURE);
    }
    if ((fsiz = _filelength( fileno(inFile) )) == -1L)
        exit(EXIT_FAILURE);
    if ((map = malloc(fsiz+1))==NULL)
        exit(EXIT_FAILURE);
    memset( map, 0, fsiz );
    *(map+fsiz) = 0xff;
}

void dmpmap()
{
    int i,j=0;
    unsigned char a,*p;

    fprintf(stderr,"** Unreaded area **\n");
    fsiz++;
    for (i=0,p=map,a=*p;i<fsiz;i++,p++) {
        if (a != *p) {
            if (a == 0)
                fprintf(stderr,"%08x-%08x(%08x): %d\n",j,i-1,i-j,a);
            a = *p;
            j = i;
        }
    }
}
#endif

static void xexit(int code)
{
#ifdef DBG_FMAP
    dmpmap();
#endif
    if (noPause == 0){
        fprintf(stderr,"\n-- Press any key to exit --");
        if (getchar()!='\n')
            fputc('\n',stderr);
    }
    exit(code);
}

/*long xgetFsize(FILE *f)
{
long cur,siz;

  cur = ftell(f);
  fseek(f,0,SEEK_END);
  siz = ftell(f);
  fseek(f,cur,SEEK_SET);
  return siz;
}*/

/**/
#include <direct.h>
#ifdef WIN32
#include <mbstring.h>

static struct _finddata_t c_file;
static long hFile = -1L;

static int findfile(char **p)
{
    char *q;

    do {
        if (hFile == -1L) {
            if ( _getcwd( current, _MAX_DIR ) == NULL ) {
                fprintf(stderr,"*Error - can't getcwd !\n");
                xexit(EXIT_FAILURE);
            }
            if ((q = (char *)_mbsrchr((unsigned char *)*p,0x5c))!= NULL) {
                *q = '\0';
                if (_chdir(*p) != 0) {
                    fprintf(stderr,"*Error - can't chdir -- '%s' !\n",p);
                    return 1;
                }
                *q++ = '\\';
            } else {
                q = *p;
            }
            hFile = _findfirst(q, &c_file );
            if (hFile == -1L) {
                fprintf(stderr,"*Error - can't find -- '%s' !\n",*p);
                return 1;
            }
        } else {
            if ( _findnext( hFile, &c_file ) != 0 ) {
                _findclose( hFile );
                hFile = -1L;
                if (_chdir(current) != 0) {
                    fprintf(stderr,"*Error - can't chdir -- '%s' !\n",current);
                    xexit(EXIT_FAILURE);
                }
                return 1;
            }
        }
    } while (c_file.attrib == _A_SUBDIR);
    *p = c_file.name;
    return 0;
}
#else
static char *last=NULL;

int findfile(char **p)
{
    if (*p == last)
        return 1;
    last = *p;
    return 0;
}
#endif

#define DAT_CSG  (DAT_DMS|DAT_ALT|DAT_TIM|DAT_JST|DAT_TKY|DAT_DST|DAT_SPD|DAT_HED)
#define DAT_CSV  (DAT_DMS|DAT_ALT|DAT_TIM)
#define DAT_GDB  (                                DAT_WGS)
#define DAT_GDN  (                DAT_TIM)
#define DAT_TRK  (        DAT_ALT|DAT_TIM)
#define DAT_FX   (                DAT_TIM        |DAT_WGS)
#define DAT_WPT  (0)
#define DAT_WWP  (                                DAT_WGS)
#define DAT_TXT  (                DAT_TIM)
#define DAT_IPS  (        DAT_ALT|DAT_TIM|                        DAT_SPD|DAT_HED|DAT_STA|DAT_SAT)
#define DAT_LOG  (DAT_DMS        |DAT_TIM|DAT_JST|DAT_TKY)
#define DAT_NME  (DAT_DMM        |DAT_TIM        /*|DAT_TKY*/        |DAT_SPD|DAT_HED|DAT_STA|DAT_SAT)
#define DAT_MPS  (                                DAT_WGS)
#define DAT_MSS  (DAT_DMS        |DAT_TIM|DAT_JST        |DAT_TKY|DAT_SPD        |DAT_STA|DAT_SAT)  //added DAT_TIM etc
#define DAT_MTK  (DAT_DMS                        |DAT_TKY)
#define DAT_NYP  (                DAT_TIM        |DAT_TKY|DAT_DST|DAT_SPD|DAT_HED|DAT_STA)
#define DAT_NUP  (                DAT_TIM|DAT_JST|DAT_TKY)
#define DAT_PMF  (                                DAT_TKY)
#define DAT_RMF  (                                DAT_TKY)
#define DAT_SGL  (                DAT_TIM        |DAT_TKY        |DAT_SPD|DAT_HED)
#define DAT_STF (DAT_DMM|DAT_ALT|DAT_TIM|         DAT_WGS|        DAT_SPD|DAT_HED|DAT_STA)
/* DAT_HED is calculated from speed vector. */
#define DAT_SMF  (                                DAT_TKY)
#define DAT_POT  (DAT_DMS                                )
#define DAT_VCD  (DAT_DEG                       |DAT_TKY)
#define DAT_ZPO  (DAT_DEG                       |DAT_TKY)
#define DAT_TDF  (        DAT_ALT|DAT_TIM       |DAT_WGS)
#define DAT_PSP  (DAT_ALT|DAT_TIM|DAT_WGS|DAT_DST|DAT_SPD|DAT_HED)

typedef struct {
    char *name;
    int datTyp;         /* indicate format bitmap */
    char *deflt;
    int (*func)();
    char subTyp;
    char *description;
} fnTable_t;

static fnTable_t inFunc[] = {
    { "CSG", DAT_CSG, "TRK",in_CSG,'\0',"CSV - Gtrex"       },
    { "CSV", DAT_CSG, "TRK",in_CSG, 'g',"CSV - Gtrex"       },
    { "CSV", DAT_CSV, "TRK",in_CSV,'\0',"CSV - kashmir"     },
#if 1
    { "DRS",       0, "DRS",in_DRS,'\0',"CN-HS400 .DRS"		},
#endif
    { "GDB", DAT_GDB, "TRK",in_GDB,'\0',"MapSource GDB"     },
    { "GDN", DAT_GDN, "TRK",in_GDN,'\0',"Gardown track"     },
    { "TRK", DAT_TRK, "TXT",in_TRK,'\0',"PCX5 .TRK"         },
    { "WPT", DAT_WPT, "WWP",in_TRK,'\0',"PCX5 .WPT"         },
    { "TXT", DAT_TXT, "TRK",in_TXT,'\0',"Waypoint+ TEXT .TXT"},
    { "IPS", DAT_IPS, "TRK",in_IPS,'\0',"IPS/ETAK GPS"      },
    { "LOG", DAT_LOG, "TRK",in_LOG,'\0',"Atlas DriveLog"    },
    { "MTK", DAT_MTK, "TRK",in_MTK,'\0',"MapFanII DriveLog" },
    { "TRK", DAT_MTK, "TRK",in_MTK, 'm',"MapFanII DriveLog" },
    { "RML", DAT_MTK, "POTr",in_RML,'\0',"MapFan V Route"  },
    { "RMF", DAT_RMF, "POTr",in_RMF,'\0',"CN-HS400 Route .RMF"  },
    { "MPS", DAT_MPS, "TRK",in_MPS,'\0',"MapSource MPS"     },
    { "NME", DAT_NME, "TRK",in_NME,'\0',"NMEA"              },
    { "NNN", DAT_NYP, "TRK",in_NNN,'\0',"Navin' You DriveLog"},
    { "NYP", DAT_NYP, "TRK",in_NYP,'\0',"Navin' You Package"},
    { "NYP", DAT_NUP, "PMF",in_NUP, 'u',"Navin' You UserPoint"},
#if 0
    { "NYP", DAT_NYP, "POT",in_NRP, 'r',"Navin' You Route"},
#endif
    { "PMF", DAT_PMF, "POTw",in_PMF,'\0',"Portable Map File"		},
    { "POT", DAT_POT, "TRK",in_POT,'\0',"POT format standard"   },
#ifndef DOS
    { "SGL", DAT_SGL, "TRK",in_SGL,'\0',"GTrex GPS Log"},
#if 0
    { "rdc",       0, "rdc",in_rdc,'\0',"reduction"},
#endif
#endif
    { "SMF", DAT_SMF, "TRK",in_SMF,'\0',"CN-HS400 Track .SMF"		},
    { "STF", DAT_STF, "TRK",in_STF,'\0',"GARMIN Simple Text Format"},
    { "VCD", DAT_VCD, "TRK",in_VCD,'\0',"Kashmir data"  },
    { "ZPO", DAT_ZPO, "TRK",in_ZPO,'\0',"Z[Zi]II"       },
    { "FX\0", DAT_FX, "TRK",in_FX,'\0',"gar2ws log"     },
    { "TDF", DAT_TDF, "TRK",in_TDF,'\0',"diffed"        },
    { "WWP", DAT_WWP, "WPT",in_WWP,'\0',"WWlogger Waypoint"     },
    { "XML", 0, "PMF",in_XML,'\0',"XML"     },
//   ,{ "DMY",     0, "TRK",in_DMY,'\0',"dummy"         }
    { "PSP", DAT_PSP, "NME",in_PSP,'\0',"PSP-290 raw data" },
};

typedef struct {
    char *name;
    int datTyp;         /* indicate format bitmap */
    void (*func)();
    int  (*cls_func)(FILE *out);
    char subTyp;
    char category;
    char *description;
} outTable_t;

static outTable_t outFunc[] = {
    { "TRK", DAT_TRK, out_TRK, fclose,'\0','t',"PCX5 .TRK"          },
    { "TXT", DAT_TXT, out_TXT, fclose,'\0','t',"Waypoint+ TEXT .TXT" },
    { "IPS", DAT_IPS, out_IPS, fclose,'\0','t',"IPS GPS" },
    { "LOG", DAT_LOG, out_LOG, fclose,'\0','t',"Atlas DriveLog"     },
    { "LST", DAT_LST, out_LST,cls_LST,'\0','t',"Statistical List"   },
    { "MSS", DAT_MSS, out_MSS,cls_MSS,'\0','w',"ProAtlas MapServerScript"   },
    { "MPS", DAT_MSS, out_MSS,cls_MSS, 'a','w',"ProAtlas MapServerScript"   },
    { "MTK", DAT_MTK, out_MTK, fclose,'\0','t',"MapFanII DriveLog"  },
    { "GDN", DAT_GDN, out_GDN, fclose,'\0','t',"Gardwon track"      },
    { "NME", DAT_NME, out_NME, fclose,'\0','t',"NMEA"               },
    { "TRK", DAT_MTK, out_MTK, fclose, 'm','t',"MapFanII DriveLog"  },
    { "NYP", DAT_NYP, out_NYP,cls_NYP,'\0','t',"Navin' You Package" },
    { "NYP", DAT_NUP, out_NUP,cls_NUP, 'u','w',"Navin' You UserPoint"   },
    { "PMF", DAT_PMF, out_PMF, fclose,'\0','w',"Portable Map File"  },
    { "POT", DAT_POT, out_POTT, fclose,'\0','t',"POT track format"  },
    { "POT", DAT_POT, out_POTR, fclose,'r','r',"POT route format"   },
    { "POT", DAT_POT, out_POTW, fclose,'w','w',"POT waypoint format"    },
#ifndef DOS
    { "SGL", DAT_SGL, out_SGL,cls_SGL,'\0','t',"GTrex GPS Log"  },
#if 0
    { "rdc",       0, out_rdc,cls_rdc,'\0','t',"reduction"  },
#endif
#endif
    { "VCD", DAT_VCD, out_VCD, fclose,'\0','w',"Kashmir data"   },
    { "CSG", DAT_CSG, out_CSG, fclose,'\0','t',"CSG - Gtrex CSV"        },
    { "CSV", DAT_CSG, out_CSG, fclose, 'g','t',"CSV - Gtrex"            },
    { "CSV", DAT_CSV, out_CSV, fclose,'\0','t',"CSV - kashmir"          },
    { "TDF", DAT_TDF, out_TDF,cls_TDF,'\0','t',"TDF - diffed"           },
    { "WPT", DAT_WPT, out_WPT, fclose,'\0','w',"WPT - PCX5 .WPT"        },
    { "WWP", DAT_WWP, out_WWP,cls_FXW,'\0','w',"WWP - WWlogger WPT"     },
    { "FX",  DAT_FX,  out_FX ,cls_FXL,'\0','t',"gar2ws .fx"     },
    { "STF", DAT_STF, out_STF, fclose,'\0','t',"GARMIN Simple Text Format" }
};

static void usage(void)
{
#if 0
    fprintf(stderr,"\nUsage: %s2%s [options] files [files ...]\n",inExt[cvType],outExt[cvType]);
    fprintf(stderr,"     : %s2%s [options] <infile >outfile\n",inExt[cvType],outExt[cvType]);
#endif
    fprintf(stderr," options:\n"\
        "  -Ityp Specify Input File Type\n"\
        "  -Ttyp Specify Output File Type\n"\
        "  typ:CSV i/o,kshmir .CSV File\n"\
        "      CSG,CSVg i/o,Gtrex .CSV File\n"\
        "      GDB i  ,MapSource .GDB File\n"\
        "      GDN i/o,Gardown7 File\n"\
        "      IPS i/o,IPS/ETAK GPS File\n"\
        "      LOG i/o,Pro atlas log .LOG File\n"\
        "      LST   o,list File\n"\
        "      MPS i  ,MapSource .MPS File\n"\
        "      MSS,MPSa  /o,ProAtlas MapServerScript File\n"\
        "      MTK,TRKm i/o,MapFan log File\n"\
        "      NME i/o ,NMEA Capture File\n"\
        "      NNN i  ,Files in Navin' You DriveLog\\(no extension)\n"\
        "      NYP i/o,Navin' You DriveLog/UserPoint File\n"\
        "      PMF i/o,Potable Map File\n"\
        "      POT i/o,POT standard(Track,Route,Point) File\n"\
        "      RMF i  ,Route Map File\n"\
        "      RML i  ,MapFan Route File\n");
#ifndef DOS
    fprintf(stderr,
        "      SGL i/o,GTrex GPS Log File\n"
    );
#endif
    fprintf(stderr,
        "      STF i/o,Garmin Simple Text Out Capture File\n"\
        "      TDF i/o,Trk Diffed File\n"\
        "      TRK i/o,PCX5 .TRK File\n"\
        "      TXT i/o,Waypoint+ comma delimited .TXT File\n"\
        "      VCD i/o,Kashmir .VCD File\n"\
        "      WPT i/o,PCX5 .WPT File\n"\
        "      WWP i/o,WS logger .WWP File\n"\
        "      XML i  ,its-moNavi .XML File\n"\
        "      ZPO i  ,Z[zi:]II .ZPO File\n"\
        "      PSP i  ,PSP-290 raw data\n"\
        "  -IWGS84 Over ride input datum WGS84.\n"\
        "  -ITOKYO Over ride input datum TOKYO.\n"\
        "  -AB  Allow Bad NMEA Checksum.\n"\
        "  -AN  Allow No NMEA Checksum.\n"\
        "  -D   DEG format.\n"\
        "  -DM  DM format.\n"\
        "  -DMS DMS format.\n"\
        "  -DT  Datum Tokyo.\n"\
        "  -DW  Datum WGS84.\n"\
        "  -HTs Header Title string.\n"\
        "  -HMs Header Map name string.\n"\
        "  -RRn Reduction Resolution n[meter].\n"\
        "  -RPn Reduction Points n.\n"\
        "  -SDn Skip Distance under n[meter].\n"\
        "  -STn Skip Time Difference under n[sec].\n"\
        "  -SHn Skip Speed over n[m/sec].\n"\
        "  -SLn Skip Speed under n[m/sec].\n"\
        "  -SA  Skip Abnormal data.\n"\
        "  -F Filter, output to stdout.\n"\
        "  -K noKanji\n"\
        "  -L List\n"\
        "  -M        Merge\n"\
        "  -MSoption MapServer Script option[bit flag]."\
        "  -N noPause\n"\
        "  -O Overwrite file.\n"\
        "      N:Never(default)\n"\
        "      Q:Query\n"\
        "      R:Rename\n"\
        "      F:Force\n"\
        "  -Ppath output to path.\n"\
        "  -U   Set File timestamp as last record.\n"\
/*      "  -V   Verbose.\n"\*/
        "  -XA  eXclude Altitude data from input.\n"\
        "  -XT  eXclude Time data from input.\n");
    xexit(EXIT_FAILURE);
}

void xfseek( FILE *stream, long offset, int origin )
{
    if (fseek(stream, offset, origin ) != 0) {
        fprintf(stderr,"\n*Error - can't seek !\n");
        if (stream == stdout)
            fprintf(stderr,"Remove -f option and try again !\n");
        xexit(EXIT_FAILURE);
    }
}

void xfread( void *buffer, size_t size, size_t count, FILE *stream )
{
#ifdef DBG_FMAP
    {
        unsigned char *p,*q;
        for (p = map+ftell(stream),
            q = p+(size*count); p<q ; p++) (*p)++;
    }
#endif
    if (fread(buffer, size, count, stream)!= count) {
        fprintf(stderr,"\n*Error - can't read !\n");
        xexit(EXIT_FAILURE);
    }
}

void xfwrite( void *buffer, size_t size, size_t count, FILE *stream )
{
    if (fwrite(buffer, size, count, stream)!= count) {
        fprintf(stderr,"*Error - can't write !\n\n");
        xexit(EXIT_FAILURE);
    }
}

int round(double d)
{
    if (d < 0)
        d-=0.5;
    else
        d+=0.5;
    return (int)d;
}

dword dswab(dword big)
{
    dword little;
    char *p,*q;

    p =((char *)&big) +3;
    q =(char *)&little;

    *q++ = *p--;
    *q++ = *p--;
    *q++ = *p--;
    *q = *p;
    return little;
}

static FILE *rFile,*wFile;
static char inType[8],outType[8];
char outName[_MAX_DIR+_MAX_FNAME+_MAX_EXT+1+3];

static int mak_outName(int iIdx,int oIdx)
{
    char *p,*q;

    if ((p = strrchr(outName,'.'))!=NULL) {
        if (stricmp(p,".fx")==0) {
            *p = '\0';
            p = strrchr(outName,'.');
        }
    }
    if (p == NULL) {
        for(p = outName;*p;)
            p++;
    }
    *p++ = '.';
    for (q = outFunc[oIdx-1].name;*q;++q){
        if (isupper(*p))
            *p++ = (char)toupper(*q);
        else
            *p++ = (char)tolower(*q);
    }
    if (stricmp(p-4,".wwp")==0) {
        strcpy(p,".fx");
        p+=3;
    }
    *p = '\0';

    if (_access(outName, 0) == -1) {
        fprintf(stderr,"%s(%s)->%s(%s)\n",
            inNamep,inFunc[iIdx-1].description,outName,outFunc[oIdx-1].description);
        return 1;
    }

    if (overWriteFlag != 2){
        fprintf(stderr,"%s(%s)->%s(%s)\n",
            inNamep,inFunc[iIdx-1].description,outName,outFunc[oIdx-1].description);
    }
    if (overWriteFlag == 0) {/* never */
        fprintf(stderr,"*File %s already exists. Skipped.\n",outName);
        return 0;
    }
    if (overWriteFlag == 1) {/* query */
        char buf[32];

        fprintf(stderr,"*File %s already exists. over write ? (Y/N/All Yes/Rename all) ",outName);
        fgets(buf,32,stdin);

        switch(toupper(buf[0])) {
        case 'R':
            overWriteFlag = 2;
            break;
        case 'A':
            overWriteFlag = 3;
            /* no break */
        case 'Y':
            break;
        case 'N':
        default:
            return 0;
        }/* end switch */
    }/* end if */

    if (overWriteFlag == 2) {
        int j= 0;
        p = strrchr(outName,'.');
        if (stricmp(p,".fx")==0) {
            *p = '\0';
            p = strrchr(outName,'.');
        }
#ifdef DOS
        if (p-outName > 8)
            p = outName+8;
        if ((*(p-2) == '_') && isdigit(*(p-1)) ) {
            j = *(p-1) - '0';
            p-=2;
        }
        if (p-outName > 6)
            p = outName+6;
        do {
            if (j > 9) {
                fprintf(stderr,"*Can't make new Name, Skip.\n");
                return 0;
            }
            sprintf(p,"_%d.%s",j++,outFunc[oIdx-1].name);
        } while(_access(outName, 0) != -1);
#else
        if ((*(p-3) == '_') && (isdigit(*(p-2))) && (isdigit(*(p-1))))
            j = atoi(p-=3);
        do {
            if (j > 99) {
                fprintf(stderr,"*Can't make new Name, Skip.\n");
                return 0;
            }
            if (stricmp(outFunc[oIdx-1].name,"wwp")==0)
                sprintf(p,"_%02d.%s.fx",j++,outFunc[oIdx-1].name);
            else
                sprintf(p,"_%02d.%s",j++,outFunc[oIdx-1].name);
        } while(_access(outName, 0) != -1);
#endif
        fprintf(stderr,"%s(%s)->%s(%s)\n",
            inNamep,inFunc[iIdx-1].description,outName,outFunc[oIdx-1].description);
    } else {
        if (_access(outName, 2) == -1) {
            fprintf(stderr,"*File '%s' is write protected. Skip.\n",outName);
            return 0;
        }
        if (_stricmp(inNamep, outName)==0) {
            fprintf(stderr,"*Output file name '%s' is same as input file. Skip.\n",outName);
            return 0;
        }
    }
    return 1;
}

#define TOKYO_RPOL  6356078.963
#define TOKYO_REQU  6377397.155

#define WGS84_RPOL  6356752.314
#define WGS84_REQU  6378137.0

static cnvData_t GPSLogBuf;

static int optFlag;
static int rdcFlag;
static double minSpd = 0.0;
static double maxSpd = 29979246.0;

double minLat = -90;
double maxLat = 90;
double minLon = -180;
double maxLon = 180;

static int cvt(int iIdx, int oIdx)
{
    cordinate_t pos0,pos,pos0rad,posrad;
    time_t t0,t1;
    struct tm atime0,atime;
    struct _utimbuf modTimBuf;
    int datTyp;
    int inStat;
    int skipped = 0;

    if (((wFile != stdout) && (_stricmp(inFunc[iIdx-1].name,outFunc[oIdx-1].name)==0)) && (overWriteFlag != 2)) {
	        fprintf(stderr,"*Error - input file '%s' extension is same as output.\n",inNamep);
		    return -1;
    }

    pos0.lat = pos0.lon = 999;
    pos0rad = posrad = pos0;
    atime.tm_isdst = 0;

    memset(&GPSLogBuf,0,sizeof(GPSLogBuf));
    GPSLogBuf.datTyp = inFunc[iIdx-1].datTyp;   /* indicate format bitmap */
    GPSLogBuf.thread = 1;
    GPSLogBuf.recTime = &atime; /* recoded time, if null, no time data */

    nRec = 0;
    readRec = 0;
    nEntry = 0;
    line = 0;
    XErr = 0;
    errno = 0;
    t0 = t1 = 0;
    ttbuf[0] = '\0';

    for (;;) {
        if ((inStat = (*inFunc[iIdx-1].func)(rFile, &pos, &GPSLogBuf))>0) {
            if (XErr || errno)
                break;
			if (in_ovr_datum) {	/* over_ride_input datum anyway */
				GPSLogBuf.datTyp &= ~DAT_DTM_MSK;
				GPSLogBuf.datTyp |= in_ovr_datum;
			}
            if (noKanji) {
                unsigned char *u;
                for (u = (unsigned char *)titleBuf; *u; u++) {
                    if (_ismbblead(*u))
                        break;
                }
                if (*u != '\0')
                    titleBuf[0] = '\0';
            }
            GPSLogBuf.datTyp &= ~ignoreFlag;
            if (pos.lat > 180.0)
                pos.lat -= 360.0;
            if (pos.lon > 180.0)
                pos.lon -= 360.0;
            if (listFlag) {
                if (nRec++ == 0)
                    fprintf(stdout,"\nFile: %s\n",inNamep);
                continue;
            }
            if (nRec == 0) {
                if ((GPSLogBuf.datTyp & DAT_TIM)==0) {
                    t1 = time(&t1);
                    if (GPSLogBuf.outTyp & DAT_JST)
                        GPSLogBuf.recTime = localtime(&t1);
                    else
                        GPSLogBuf.recTime = gmtime(&t1);
                }
                GPSLogBuf.outTyp = outFunc[oIdx-1].datTyp | ((out_angfmt) ? out_angfmt:(GPSLogBuf.datTyp & DAT_ANGFMT_MSK));
                if ((GPSLogBuf.outTyp & DAT_ANGFMT_MSK) == 0)
                    GPSLogBuf.outTyp |= DAT_DMS;

                if ((GPSLogBuf.outTyp & DAT_DTM_MSK) == 0) {/* no specific datum ? */
                    if (GPSLogBuf.datTyp & DAT_OTH) {
                        if (GPSLogBuf.mpp == NULL) {
                            fprintf(stderr,"*Error - No datum conversion parameter !\n");
                            XErr++;
                            break;
                        }
                        GPSLogBuf.outTyp |= DAT_WGS;
                    } else
                        GPSLogBuf.outTyp |= ((out_datum) ? out_datum:(GPSLogBuf.datTyp & DAT_DTM_MSK));
                }
                if (((GPSLogBuf.datTyp & DAT_DTM_MSK) == DAT_TKY)&&(GPSLogBuf.mpp == NULL))
                    GPSLogBuf.mpp = &mWGS;
            }
            datTyp = (GPSLogBuf.datTyp ^ (GPSLogBuf.outTyp|optFlag)) & (GPSLogBuf.outTyp|optFlag);

            if ((datTyp & DAT_DTM_MSK) && ((GPSLogBuf.datTyp & DAT_ALT) == 0))
                pos.alt = 0;

            if (GPSLogBuf.datTyp & DAT_TIM)
                t1 = mktime(GPSLogBuf.recTime);

            if (datTyp & CNV_RAD) { /* radian convert needed */
                posrad.lat = pos.lat*TORAD;
                posrad.lon = pos.lon*TORAD;
                posrad.alt = pos.alt;

                if (datTyp & (DAT_WGS|DAT_OTH))
                    Molodensky(&posrad, GPSLogBuf.mpp); /* to WGS84 */

                if (datTyp & DAT_TKY)
                    Molodensky(&posrad, &mTokyo); /* to Tokyo datum */

                if (datTyp & (DAT_DST|DAT_SPD|DAT_TIM)) {
                    if (nRec != 0) {
                        if (GPSLogBuf.outTyp & DAT_TKY)
                            GPSLogBuf.dist = calc_dist(&pos0rad,&posrad,TOKYO_RPOL,TOKYO_REQU);
                        else
                            GPSLogBuf.dist = calc_dist(&pos0rad,&posrad,WGS84_RPOL,WGS84_REQU);
                        if (datTyp & DAT_HED) {
                            if (fabs(GPSLogBuf.dist)>0)
                                GPSLogBuf.head = calc_heading(&pos0rad,&posrad)/TORAD;
                        }
                        if (datTyp & (DAT_SPD|DAT_TIM)) {
                            if (datTyp & DAT_TIM) {
                                t1 = t0 + (long)((GPSLogBuf.dist*3.6)/simSpeed);
                                if (t1 == t0)
                                    t1++;
                                *(GPSLogBuf.recTime) = *gmtime(&t1);
                            } else {
                                if (t1-t0)
                                    GPSLogBuf.speed = GPSLogBuf.dist*3.6/(t1-t0);
                            }
                        }
                    }
                }
                pos.alt = posrad.alt;
                pos.lat = posrad.lat/TORAD;
                pos.lon = posrad.lon/TORAD;
            }
            skipped = 1;
            if (outFunc[oIdx-1].category != 'w') {  /* skip abnomal if category is track. */
                if (((GPSLogBuf.thread == 0) || (GPSLogBuf.datTyp & DAT_SPD))
                    && (optFlag & DAT_SPD) && ((GPSLogBuf.speed < minSpd) || (GPSLogBuf.speed > maxSpd)) )
                    continue;
                if (((GPSLogBuf.thread == 0) || (GPSLogBuf.datTyp & DAT_DST))
                    && (optFlag & DAT_DST) && (GPSLogBuf.dist < minDist))
                    continue;
                if( pos.lat < minLat || maxLat < pos.lat || pos.lon < minLon || maxLon < pos.lon )
                    continue;
//                if ( ((optFlag|GPSLogBuf.datTyp) & DAT_TIM) && ((t1-t0 < minTime)&&(GPSLogBuf.thread == 0)))
//                    continue;
            }
        }
        if ((inStat > 0) || ((inStat<=0)&&(skipped!=0)/* Force write to last point */)) {
            pos0 = pos;
            pos0rad = posrad;
            atime0 = atime;
            t0 = t1;
            if ((GPSLogBuf.datTyp ^ GPSLogBuf.outTyp) & DAT_JST) {
                if ((t1 = mktime(&atime))==-1) {
                    fprintf(stderr,"*Error - can't mktime.\n");
                    break;
                }
                if (GPSLogBuf.datTyp & DAT_JST)
                    atime = *gmtime(&t1);   /* JST -> UTC */
                else {
                    t1 -= timezone;
                    atime = *localtime(&t1);/* UTC -> JST */
                }
            }
            if (wFile == NULL) {
                if (path != NULL) {
                    if ( _getcwd( current, _MAX_DIR ) == NULL ) {
                        fprintf(stderr,"*Error - can't getcwd !\n");
                        xexit(EXIT_FAILURE);
                    }
                    if (_chdir(path) != 0) {
                        fprintf(stderr,"*Error - can't chdir -- '%s' !\n",path);
                        xexit(EXIT_FAILURE);
                    }
                }
                if (XErr || errno)
                    break;
                if (mak_outName(iIdx,oIdx) != 1)
                    return 0;
                errno = 0;
                if ((wFile = fopen(outName,"w")) == NULL) {
                    fprintf(stderr,"*Error - can't open file -- '%s' !\n",outName);
                    xexit(EXIT_FAILURE);
                }
                if (path != NULL) {
                    if (_chdir(current) != 0) {
                        fprintf(stderr,"*Error - can't chdir -- '%s' !\n",path);
                        xexit(EXIT_FAILURE);
                    }
                }
            }
            skipped = 0;
            (*outFunc[oIdx-1].func)(wFile, &pos, &GPSLogBuf);
            if (XErr || errno)
                break;
        }
        if (inStat <= 0)
            break;
    }

    if ((wFile!=NULL) && (wFile!=stdout)) {
        (*outFunc[oIdx-1].cls_func)(wFile);
        wFile = NULL;
        if (timeStamp) {
            modTimBuf.actime = modTimBuf.modtime =  mktime(&atime) - _timezone;
            if( _utime(outName, &modTimBuf ) == -1 )
                fprintf(stderr,"*Error - can't change time stamp !\n" );
        }
    }
    if (((_stricmp(outType,"lst")==0) || (_stricmp(outType,"mss")==0)) && (wFile!=NULL))
        (*outFunc[oIdx-1].cls_func)(wFile);

    if (errno) {
        fprintf(stderr,"line:%ld(nRec:%d) - ",line,nRec);
        perror("Aborted.");
    }
    if (XErr || errno)
        return -1;

    if (nRec)
        fprintf(stderr,"%d record(s) Complete.\n",nRec);
    else if (newType == NULL)
        fprintf(stderr,"Nothing to process !\n");
    return EXIT_SUCCESS;
}

static int getInIdx()
{
    int i;
    char subTyp = '\0';

    if (strlen(inType) > 3)
        subTyp = (char)tolower(inType[3]);
    inType[3] = '\0';

    for (i=1;i<=(sizeof(inFunc)/sizeof(fnTable_t));i++)
        if ((_stricmp(inType,inFunc[i-1].name) == 0) && (subTyp == inFunc[i-1].subTyp))
            return i;
        return 0;
}

static int getOutIdx()
{
    int i;
    char subTyp = '\0';

    if (strlen(outType) > 3)
        subTyp = (char)tolower(outType[3]);
    outType[3] = '\0';

    for (i = sizeof(outFunc)/sizeof(outTable_t);i;--i)
        if ((_stricmp(outType,outFunc[i-1].name) == 0) && (subTyp == outFunc[i-1].subTyp))
            return i;
        return 0;
}

static void chkOpt(char p[])
{
    char c;

    switch(toupper(p[1])) {
    case 'A':   /* NMEA checksum */
        switch(toupper(p[2])) {
        case 'B':
            csumFlag = -1;
            break;
        case 'N':
            csumFlag = 1;
            break;
        default:
            usage();
        }
        break;
    case 'G':   /* GPS Type */
        if (_stricmp(&p[2],"ips")==0)
            GPSTyp = GPS_IPS;
        else if (_stricmp(&p[2],"garmin")==0)
            GPSTyp = GPS_GRM;
        else if (_stricmp(&p[2],"mitsumi")==0)
            GPSTyp = GPS_MTM;
        else
            usage();
        break;
    case 'R':   /* skip */
        c = p[2];
        switch(toupper(c)) {
        case 'H':
            rdcRes = atoi(&p[3]);
			rdcFlag |= RDC_RES;
            optFlag |= DAT_ADJ;
            break;
        case 'S':
            rdcNum = atoi(&p[3]);
			rdcFlag |= RDC_NUM;
            optFlag |= DAT_ADJ;
            break;
		}
		break;
    case 'S':   /* skip */
        c = p[2];
        switch(toupper(c)) {
        case 'D':
            optFlag |= DAT_DST;
            minDist = atoi(&p[3]);
            break;
        case 'H':
        case 'S':
            optFlag |= DAT_SPD;
            maxSpd = atoi(&p[3]);
            break;
        case 'L':
            optFlag |= DAT_SPD;
            minSpd = atoi(&p[3]);
            break;
        case 'T':
            optFlag |= DAT_TIM;
            minTime = atoi(&p[3]);
            break;
        case 'A':   /* abnormal */
            optFlag |= DAT_ADJ;
            maxSpd = 1000;  /* 1000m/s */
            minTime = 1;
#if 1
            minLat = 30;
            maxLat = 50;
            minLon = 120;
            maxLon = 150;
#endif
            break;
        default:
            usage();
            break;
        }
        break;
    case 'D':
        c = p[2];
        switch(toupper(c)) {
        case 'W':
            out_datum = DAT_WGS;
            break;
        case 'T':
            out_datum = DAT_TKY;
            break;
        case '\0':
            out_angfmt = DAT_DEG;
            break;
        case 'M':
            if (p[3] == '\0') {
                out_angfmt = DAT_DMM;
                break;
            } else if (toupper(p[3]) == 'S') {
                break;
            }
            /* no break */
        default:
            usage();
        }
        break;
    case 'F':   /* output to stdout */
        wFile = stdout;
        break;
    case 'H':
        switch(toupper(p[2])) {
        case 'T':
            strncpy(titleBuf,&p[3],TTBUFLEN-1);
            break;
        case 'M':
            strncpy(mapTtlBuf,&p[3],MTBUFLEN-1);
            break;
        default:
            usage();
            break;
        }
        break;
    case 'I':   /* set input type */
		if (stricmp(&p[2],"WGS84")==0) {
			in_ovr_datum = DAT_WGS;
			break;
		}
		if (stricmp(&p[2],"TOKYO")==0) {
			in_ovr_datum = DAT_TKY;
			break;
		}
        strncpy(inType,&p[2],7);
        if ((in_idx = getInIdx())==0)
            usage();
        break;
    case 'T':   /* set to type */
        strncpy(outType,&p[2],7);
        if ((out_idx = getOutIdx())==0)
            usage();
        break;
    case 'K':
        noKanji++;
        break;
    case 'L':
        listFlag++;
        break;
#ifdef WIN32
    case 'M':
        c = p[2];
        switch(toupper(c)) {
        case '\0':
            mergeFlag++;
            break;
        case 'S':
            MSSOpt = atoi(&p[3]);
            if ((MSSOpt<0) || (MSSOpt>0xFF))
                usage();
            break;
        default:
            usage();
        }
        break;
#endif
    case 'N':
        noPause++;
        break;
    case 'O':   /* over write */
        c = p[2];
        switch(toupper(c)) {
        case '0':   /* Never */
        case '1':   /* Query */
        case '2':   /* Rename */
        case '3':   /* Force */
            overWriteFlag = c -'0';
            break;
        case 'N':
            overWriteFlag = 0;
            break;
        case 'Q':
            overWriteFlag = 1;
            break;
        case 'R':
            overWriteFlag = 2;
            break;
        case 'F':
            overWriteFlag = 3;
            break;
        default:
            usage();
        }
        break;
    case 'P':
        if (_chdir(&p[2]) != 0) {
            fprintf(stderr,"*Error - can't chdir -- '%s' !\n",&p[2]);
            xexit(EXIT_FAILURE);
        } else {
            path = &p[2];
        }
        break;
    case 'U':
        timeStamp++;
        break;
#if 0
    case 'V':
        verbose++;
        break;
#endif
    case 'X':   /* exclued */
        c = p[2];
        switch(toupper(c)) {
        case 'A':   /* alt */
            ignoreFlag |= DAT_ALT;
            break;
        case 'T':   /* Time */
            ignoreFlag |= DAT_TIM;
            break;
        default:
            usage();
        }
        break;
    default:
        usage();
    }
}

#if 1
char argvBuf[] = "nme2nyp-itokyo-D.exe";
#endif

void main(int argc, char *argv[], char *envp[])
{
    int i,j;
    int inIdx,outIdx;
    char *p,*q;

    rFile = wFile = NULL;
    optFlag = mergeFlag = 0;
    minTime = 1;
    minDist = 0;
    simSpeed = 30.0;

    ignoreFlag = 0;

    out_angfmt = DAT_DMS;

    if ((p = strrchr(argv[0],'\\')) == NULL)
        p = argv[0];
    else
        ++p;

#if 0
	p = argvBuf;
#endif

    if ((q = strrchr(p,'.')) != NULL)
        *q = '\0';

    if ((q = strchr(p,'2')) != NULL) {
        i = ((q-p)<8)?(q-p):7;
        strncpy(inType,p,i);
        inType[i] = '\0';
        strncpy(outType,++q,7);
        q = outType;
        while(isalpha(*q)) q++;
        *q = '\0';
    }
	p=strchr(p,'-');
    while(p!=NULL) {
		if ((q = strchr(p+1,'-'))!=NULL)
			*q = '\0';
        chkOpt(p);
		p = q;
	}

    fprintf(stderr,"%s - GPS Log File converter V0.39 07-Jul-2005\n",argv[0]);
    fprintf(stderr,"Program by imaizumi@nisiq.net\n");
    fprintf(stderr,"MapServerScript by masaaki@attglobal.net\n\n");

    if ((p = strrchr(argv[0],'\\')) != NULL) {
        *p ='\0';
        if ((p = strrchr(argv[0],'\\')) != NULL) {
            if ((stricmp(++p,"GPSLog")==0) && (stricmp(outType,"sgl")==0)) {
                path = argv[0];
            }
        }
    }
    for(i=1;i<argc;i++) {
        if (argv[i][0] == '-')
            chkOpt(argv[i]);
        else
            break;
    }

	while(*envp != NULL) {
		//	printf("%s\n",*envp++);
		if (strncmp("PROMPT=",*envp++,7)==0) {
			noPause = -1;	// we need prompt.
			break;
		}
	}

    if (in_idx == 0)
        in_idx = getInIdx();
    if (out_idx == 0)
        out_idx = getOutIdx();

    if (i>=argc) {
        rFile = stdin;
        inNamep = "stdin";
        wFile = stdout;
    }
    if (wFile == stdout)
        strcpy(outName,"stdout");
    do {
        inIdx = (in_idx) ? in_idx:1;

        if (rFile != stdin) {
            if ((j = findfile(&argv[i])) != 0) {
                i+=j;
                continue;
            }
            inNamep = argv[i];
            strncpy(outName,argv[i],_MAX_DIR+_MAX_FNAME+_MAX_EXT);
            if ((p = strrchr(outName,'.'))==NULL){
                if ((p = strrchr(outName,'\\'))==NULL)
                    p = outName;
                while(isdigit(*p))*p++;
                if (*p != '\0'){
                    fprintf(stderr,"*Error - no file type -- '%s' !\n",argv[i]);
                    continue;
                }
                strcpy(p+1,"NNN");
            }
            strncpy(inType,++p,7);
            /* For AccessIt! Log .PDB requester:san. */
            if (strcmp(inType,"PDB")==0) {
                int c;
                dword pt;
                if (rFile == NULL) {
                    if ((rFile = fopen(argv[i],"rb")) == NULL) {
                        fprintf(stderr,"*Error - can't open file -- '%s' !\n",argv[i]);
                        continue;
                    }
                }
                xfseek(rFile,0x4El,SEEK_SET);
                xfread(&pt,sizeof(dword),1,rFile);
                pt = dswab(pt)+12;
                xfseek(rFile,pt,SEEK_SET);
                while((c=getc(rFile)) != EOF) {
                    if (c == '@') {
                        strcpy(inType,"stf");
                        break;
                    } else if (c == '$') {
                        strcpy(inType,"nme");
                        break;
                    }
                }
                if ((inIdx = in_idx = getInIdx())==0) {
                    fprintf(stderr,"*Error - can't determine file type -- '%s' !\n",argv[i]);
                    continue;
                }
                ungetc(c,rFile);
            }
            /* End of AccessIt! Log process */
            if (in_idx == 0) {
                int k;
                if ((k=getInIdx())!=0)
                    inIdx = k;
                else {
                    fprintf(stderr,"* can't determine file type -- '%s' skip!\n",argv[i]);
                    continue;
                }
            }
        }

        if (out_idx == 0) {
            strcpy(outType,inFunc[inIdx-1].deflt);
            outIdx = getOutIdx();
        } else
            outIdx = out_idx;

        if (rFile == NULL) {
            if ((rFile = fopen(argv[i],"r+")) == NULL) {
				if ((rFile = fopen(argv[i],"r")) == NULL) {
					fprintf(stderr,"*Error - can't open file -- '%s' !\n",argv[i]);
					continue;
				}
			}
        }
#ifdef DBG_FMAP
        initbmap(rFile);
#endif
        newType = NULL;
        for (;;) {
            cvt(inIdx,outIdx);
            if (newType == NULL)
                break;
            strcpy(inType,newType);
            newType = NULL;
            if ((inIdx = getInIdx())==0) {
                fprintf(stderr,"*Error - can't determine file type -- '%s' !\n",argv[i]);
                break;
            }
            if (out_idx == 0) {
                strcpy(outType,inFunc[inIdx-1].deflt);
                outIdx = getOutIdx();
            }
        }
        if (rFile != stdin) {
            nFile++;
            fclose(rFile);
            rFile = NULL;
            fprintf(stderr,"---\n");
        }
    } while(i<argc);

    xexit(EXIT_SUCCESS);
}
