/* ips.h */
/* Header for IPS format */
/* imaizumi@nisiq.net */
/* V1.04 01/05/18 */
/* V1.03 01/03/15 */
/* V1.02 00/02/22 */
/* V1.01 99/07/25 */
/* V1.00 98/03/10 */

#define IPS_ID_POS		 0
#define IPS_ID_LEN		 6

#define IPS_HGR3_ID "SM0020"	/* HGR3? */
#define IPS_HGR1_ID "SM0120"	/* HGR1? */
#define IPS_ETGPS_ID "SMATC0"	/* ETAK ET-GPS1 12 */
#define IPS_8000_ID "SONY99"	/* IPS8000? 16 */
#define IPS_5000_ID "SONY81"	/* IPS5000 8 */
#define IPS_3000_1_ID "SONY80"	/* IPS3000 8 */
#define IPS_3000_0_ID "SONY73"	/* IPS3000 8 */

#define IPS_UTC_DATE_POS 6	/* "YYMMDD" */
#define IPS_UTC_DATE_LEN 6
#define IPS_UTC_WEEK_POS 12	/* "W" a-g,A-G,0-6 Sun->Sat */
#define IPS_UTC_WEEK_LEN 1
#define IPS_UTC_TIME_POS 13	/* HHMMSS */
#define IPS_UTC_TIME_LEN 6

#define IPS_TIME_STAMP_LEN 13

#define IPS_LAT_DIR_POS 19	/* "N" */
#define IPS_LAT_DIR_LEN 1
#define IPS_LAT_VAL_POS 20	/* "DDMMSSS" */
#define IPS_LAT_VAL_LEN 7

#define IPS_LON_DIR_POS 27	/* "E" */
#define IPS_LON_DIR_LEN 1
#define IPS_LON_VAL_POS 28	/* "DDDMMSSS" */
#define IPS_LON_VAL_LEN 8

#define IPS_HGH_SGN_POS 36	/* "+" */
#define IPS_HGH_SGN_LEN	 1
#define IPS_HGH_VAL_POS 37	/* "NNNN" */
#define IPS_HGH_VAL_LEN	 4

#define IPS_SPEED_POS 41	/* "NNN" [km/h] */
#define IPS_SPEED_LEN  3

#define IPS_COURSE_POS 44	/* "AAA" 0->360 N->E->S->W */
#define IPS_COURSE_LEN	3

#define IPS_MES_DATE_POS 47	/* "YYMMDD" */
#define IPS_MES_DATE_LEN 6
#define IPS_MES_WEEK_POS 53	/* "W" */
#define IPS_MES_WEEK_LEN 1
#define IPS_MES_TIME_POS 54	/* "HHMMSS" */
#define IPS_MES_TIME_LEN 6

#define IPS_DOP_POS 60		/* "D" 3D:HDOP, 2D:PDOP */
#define IPS_DOP_LEN	 1

#define IPS_MODE_POS 61		/* "M" 3:2D 4< 3D */
#define IPS_MODE_LEN  1

#define IPS_DATUM_POS 62	/* "D" 'A':WGS84, 'B':TOKYO */
#define IPS_DATUM_LEN  1

/*-------------------------*/
#define IPS_SAT_SER_POS 63
#define IPS_SAT_ANG_POS 64
#define IPS_SAT_ADI_POS 65
#define IPS_SAT_STA_POS 66
#define IPS_SAT_LVL_POS 67

#define IPS_SAT_LEN 5

/*--------------------------*/
/* Use add IPS_SAT_LEN*sats */
#define IPS_OSC_STA_OFS 63	/* "N" */
#define IPS_OSC_STA_LEN 1

#define IPS_STB_OFS 64		/* IPS3000/5000/5200/HGR3:'D',IPS8000/HGR1:'i' */
#define IPS_STB_LEN 1

#define IPS_LAT_MLD 65		/* MOST LOWER DIGIT */
#define IPS_LON_MLD 66

#define IPS_FMT_OFS 66		/* "N" ALPHA:DMS, NUMERIC:DMD */
#define IPS_FMT_LEN 1

#define IPS_PAR_OFS 67		/* "N" O:ODD,E:EVEN */
#define IPS_PAR_LEN 1
