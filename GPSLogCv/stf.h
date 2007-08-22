/* stf.h - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

 Garmin GPS Simple Text Output Format log .STF

History:
 1999/10/25 V0.01 add in_STF() (requester:Mio)
*/
/* @991023150446N3554281E13938046G035+00040W0092N0126U0000 */

typedef struct {
	char start;						/* Always '@' */
	char year[2];					/* Last two digits of UTC year*/
	char month[2];					/* UTC month, "01".."12" */ 
	char day[2];					/* UTC day of month, "01".."31" */
	char hour[2];					/* UTC hour, "00".."23" */
	char minute[2];					/* UTC minute, "00".."59" */
	char second[2];					/* UTC second, "00".."59" */
	char latitude_hemisphere;		/* 'N' or 'S'*/
	char latitude_position_d[2];	/* WGS84 ddmmmmm, with an implied decimal after the 4th digit */
	char latitude_position_m[5];	/* WGS84 ddmmmmm, with an implied decimal after the 4th digit */
	char longitude_hemishpere;		/* 'E' or 'W' */
	char longitude_position_d[3];	/* WGS84 dddmmmmm with an implied decimal after the 5th digit */
	char longitude_position_m[5];	/* WGS84 dddmmmmm with an implied decimal after the 5th digit */
	char position_status;			/* 'd' if current 2D differential GPS position */
									/* 'D' if current 3D differential GPS position */
									/* 'g' if current 2D GPS position */
									/* 'G' if current 3D GPS position */
									/* 'S' if simulated position */
									/* '_' if invalid position */
	char horizontal_posn_error[3];	/* EPH in meters */
	char altitude_sign;				/* '+' or '-' */
	char altitude[5];				/* Height above or below mean sea level in meters */
	char EW_velocity_direction;		/* 'E' or 'W' */
	char EW_velocity_magnitude[4];	/* ("1234" = 123.4 m/s) */
	char NS_velocity_direction;		/* 'N' or 'S' */
	char NS_velocity_magnitude[4];	/* ("1234" = 123.4 m/s) */
	char UD_velocity_direction;		/* 'U' (up) or 'D' (down) */
	char UD_velocity_magnitude[4];	/* ("1234" = 12.34 m/s) */
/*	char sentence_end[2];*/			/* CR+LF */
} stof_t;

