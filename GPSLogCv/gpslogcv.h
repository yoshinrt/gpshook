/* gpslogcv.h - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

History:
2002/05/01 V0.34g
2001/05/07 V0.31
2001/04/13 V0.29b
2000/11/15 V0.26
2000/10/06 V0.25
2000/07/02 V0.24
2000/02/21 V0.18
1999/08/23 V0.16
1999/08/23 V0.12 add in_POT();
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

#ifdef _MSC_VER
#if _MSC_VER == 1200
#define VCPP
#endif
#endif

#ifdef MAIN
#define EXTERN
#else
#define EXTERN extern
#endif

/*#define DBG_FMAP*//* check all of file contents readed */

typedef unsigned char boolean;	/* 8 */
typedef unsigned char byte;	/* 8 */
typedef unsigned short int word;	/* 16 */
typedef unsigned long dword;	/* 32 */

/* definitions for DOS MSC6.00A */
#ifdef DOS
#define _O_BINARY O_BINARY
#define _access(path,mode) access(path,mode)
#define _chdir(path) chdir(path)
#define _getcwd(buf,len) getcwd(buf,len)
#define _setmode(handle,mode) setmode(handle,mode)
#define _fileno(file) fileno(file)
#define _strnicmp(p,q,n) strnicmp(p,q,n)
#define _stricmp(p,q) stricmp(p,q)
#define _timezone (9*3600)
#define _utime(path,tim) utime(path,tim)
#define _utimbuf utimbuf
#endif

/* definitions */
#ifndef PCXVER
#define PCXVER "2.09"
#endif

/*#define ANG_FMT_DEG 1
#define ANG_FMT_DMM 2
#define ANG_FMT_DMS 3
*/
#define ONEDEG 0xA4CB80UL /* = 360 * 3600 * 1000 * 3  = FFFFA500 */

#define PI 3.14159265358979
#define TORAD	(PI/180.0)

/* types */
typedef struct {
   double lat;
   double lon;
   double alt;
} cordinate_t;

typedef struct {
	char *datum_name;
	double da;
	double df;
	double dx;
	double dy;
	double dz;
} Molodensky_param_t;

typedef struct {
	byte sta;			/* 0:SCAN,1:LOCK,2:Ready,3:HOLD,4:Sick,5:Used */
	byte serial;		/* sat. ID */
	double angle;		/* elevation angle */
	double adimath;		/* direction */
	int lvl;			/* signal level */
} SatParm_t;

typedef struct {
	int numSat;			/* number of sat. */
	int usedSat;		/* number of used sat. */
	SatParm_t satprm[16];	/* param. */
} Satdat_t;

typedef struct {
	int datTyp;			/* indicate format bitmap */
	int outTyp;			/* indicate output format bitmap */
	int	thread;			/* indicate thread top */
	double dist;		/* [m] distance, if minus, no data */
	double speed;		/* [km/h] speed, if minus, no data */
	double head;		/* direction 0-360, if minus, no data */
	double dop;			/* dop , if minus, no data */
	struct tm *recTime;	/* recoded time, if null, no time data */
	char rcvStat;		/* reciever status, 0=N/A,1=Lost,2=2D,3=3D,4=DGPS */
	Molodensky_param_t *mpp;	/* to WGS84 */
#if 0
	char *titlep;		/* pointer to title, if null, no data */
	char *mpttlp;		/* pointer to map title, if null, no data */
#endif
	Satdat_t *satdatp;	/* pointer to satelite data, if null, no data */
} cnvData_t;

typedef struct {
	long icon_type;
	char *icon_name;
	int icon_type_pmf;
} icon_table_t;

EXTERN Molodensky_param_t mParam;	/* Datum param */
EXTERN char datumbuf[32];

EXTERN unsigned long line; /* input line number */

EXTERN int nFile;			/* processed file */
EXTERN int nRec;			/* processed record */
EXTERN dword readRec;
EXTERN int nEntry;

EXTERN int XErr;			/* Error flag */

EXTERN double dist,head;

EXTERN char *path;
EXTERN char *inNamep;

EXTERN char *newType;

/* angle string common work */
EXTERN char angbuf[32];
/* common line buffer */
#define LBUFLEN 1024
EXTERN char lbuf[LBUFLEN+1];	/* line buffer */

/* title, map name buf */
#define TTBUFLEN 128
#define MTBUFLEN 128
EXTERN char addrBuf[TTBUFLEN];
EXTERN char titleBuf[TTBUFLEN];
EXTERN char telBuf[TTBUFLEN];
EXTERN char ttbuf[TTBUFLEN];
EXTERN char mapTtlBuf[MTBUFLEN];
EXTERN char descBuf[TTBUFLEN];
EXTERN char idBuf[TTBUFLEN];
EXTERN char texBuf[TTBUFLEN];
EXTERN long icon;
EXTERN icon_table_t icontable[];
/* save current path */
EXTERN char current[_MAX_DIR];

EXTERN double minDist;		/* minimum dist */
EXTERN double simSpeed;		/* simulation speed */
EXTERN double rdcRes;		/* resolution */
EXTERN double rdcNum;		/* number of points */
EXTERN int minTime;
EXTERN int mergeFlag;
EXTERN int listFlag;
EXTERN int csumFlag;
EXTERN int verbose;
EXTERN int GPSTyp;
EXTERN int MSSOpt;

/* helper */
#ifdef DBG_FMAP
EXTERN long fsiz;
EXTERN unsigned char *map;
#endif

extern char *strtokX(char *str1,char *delim);
extern int mname2m(char *mname);
extern char *xfgets(FILE *stream );

#define DAT_DEG 0x0001
#define DAT_DMM 0x0002
#define DAT_DMS 0x0003

#define DAT_ANGFMT_MSK 3

#define DAT_TIM 0x0004
#define DAT_ALT 0x0008
#define DAT_DOP 0x0010
#define DAT_STA 0x0020	/* number of satelite 2D,3D,DGPS*/
#define DAT_SAT 0x0040	/* detailed satelite */
#define DAT_TIT 0x0080

#define DAT_WGS 0x0100
#define DAT_TKY 0x0200
#define DAT_OTH 0x0400
#define DAT_DTM_MSK (DAT_WGS|DAT_TKY|DAT_OTH)

/* RAD Conversion and calculate required */
#define DAT_DST 0x0800
#define DAT_SPD 0x1000
#define DAT_HED 0x2000

#define DAT_ADJ 0x4000	/* for skip/adjust */

/* Conversion required */
#define DAT_JST 0x8000

#define CNV_RAD (DAT_DST|DAT_SPD|DAT_HED|DAT_DTM_MSK|DAT_ADJ|DAT_TIM)

#define DAT_LST 0x1000	/* dummy */

/* GPS Type for adjust */

#define GPS_UNK 0x00
#define GPS_IPS 0x01
#define GPS_GRM 0x02
#define GPS_MTM 0x04	/* Navin'You = Mitsumi */

/* Reduction */
#define RDC_RES 0x01
#define RDC_NUM 0x02

/* input functions */
extern int in_CSV(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_CSG(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_DRS(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_FX(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_GDB(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_GDN(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_TRK(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_TXT(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_IPS(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_LOG(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_MTK(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_PMF(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_RML(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_RMF(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_MPS(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_NME(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_NNN(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_NUP(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_NRP(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_NYP(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_STF(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_SMF(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_SGL(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_POT(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_VCD(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_ZPO(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_TDF(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_WWP(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_XML(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_DMY(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);

extern int in_rdc(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);
extern int in_PSP(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp);

/*extern int in_WPT(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp); use in_TRK */
/* output functions */
extern void out_CSV(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_CSG(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_TRK(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_TXT(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_IPS(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_LOG(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_LST(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_MTK(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_MSS(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_GDN(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_NME(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_NNN(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_NYP(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_NUP(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_SGL(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_STF(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_VCD(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_PMF(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_POTT(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_POTR(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_POTW(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_TDF(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_WPT(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_WWP(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);
extern void out_FX(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);

extern void out_rdc(FILE *wFile, cordinate_t *position,cnvData_t *GPSLogp);

/* index related function */
extern int cls_SGL(FILE *out);
extern int cls_NYP(FILE *out);
extern int cls_NUP(FILE *out);
extern int cls_LST(FILE *out);
extern int cls_MSS(FILE *out);
extern int cls_TDF(FILE *out);
extern int cls_FXL(FILE *out);
extern int cls_FXW(FILE *out);

extern int cls_rdc(FILE *out);

/* file I/O */
extern void xfseek( FILE *stream, long offset, int origin );
extern void xfread( void *buffer, size_t size, size_t count, FILE *stream );
extern void xfwrite( void *buffer, size_t size, size_t count, FILE *stream );
extern int round(double d);
extern dword dswab(dword big);

extern int nypicon2pmf(long n);
