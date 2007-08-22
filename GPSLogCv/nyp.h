/* nyp.h - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

  SONY Navin'You .NYP

History:
1999/10/28 V0.17c Wrong description, Fspeed Km/h -> m/h 
1999/08/06 V0.08 
*/
/* NNN Format
  --- Start --
  NYPInfo0
  TITLE
  NYPInfo
  MAPNAME
  records
     .
  records
  --- End ----
  Same as "Section" of NYP File.

  DriveLog.idx
  --------
  NYPIdx
  --------
  NYPInfo0
  TITLE
  NYPInfo
  MAPNAME
  --------
     .
  --------
  NYPInfo0
  TITLE
  NYPInfo
  MAPNAME
  --------
*/
/* NYP Format
  0x1A
  --- Section -- <-+
  NYPInfo0         |
  TITLE            |
  NYPInfo          |
  MAPNAME          |
  records          |
     .             |
  records          |
  ---------------  |
      .            |
  -- Section - <-+ |
  NYPInfo0       | |
  TITLE          | |
  NYPInfo        | |
  MAPNAME        | |
  records        | |
     .           | |
  records        | |
  +-> ---------  | |
  |   NYPDirEnt--+ |
  |   .            |
  |   NYPDirEnt    |
  +-- NYPDirDesc --+
*/
#define ASCALE 0xB60B60 /* 0xfffffff / 360 32*7*17*3133 */

#pragma pack(1)

typedef struct NYPIdx_t {
    dword   unknown1;
    dword   maxFnumber;
    dword   numFile;
} NYPIdx;

/* directory descriptor */
typedef struct NYPDirDesc_t {
	word  Unknown[3];
	word  numSect;
	dword  DirPosBigI;
	word  Unknown1;
	byte  Unknown2[18];
	word  Unknown3;
	char  IDString[14];
} NYPDirDesc;

typedef struct NYPDirEnt_t {
	word   Unknown[3];
	dword  SectionPosBigI;
	dword  SectionLenBigI;
	word   Unkonwn1;
} NYPDirEnt;

typedef struct NYPInfo0_t {
    word    si;
    dword   fn;		/* source fn.NNN */
    dword   ttlen;
} NYPInfo0;

/* title has variable length */

typedef struct NYPInfo_t {
    dword   day;    /* Running time */
    dword   hour;
    dword   min;
    dword   sec;

    dword   dist;   /* Mileage */
    time_t  date;	/* JST */
    byte    stab;	/* 01 */
    dword   lonStart;
    dword   latStart;
    dword   lonEnd;
    dword   latEnd;
    dword   size;
    dword   mtlen;
} NYPInfo1;

/* mapname has variable length */

/* 26 byte record  */
typedef struct NYPRecord_t {
    dword  ILon;    /* long  dswab(value)/ASCALE Deg. */
    dword  ILat;    /* long  dswab(value)/ASCALE Deg. */
    dword  IAng;    /* long  dswab(value)/ASCALE Deg. */
    dword  Fspeed;  /* float dswab(value)        m/h  */
    time_t IrecTime;/* time_t dswab(value) Sec. UTC */
    word   LonAdj;  /* swab(value) map matching offset */
    word   LatAdj;  /* swab(value) map matching offset */
    word   Ssat;    /* swab(value) number of satelite */
} NYPRecord;
#pragma pack()
