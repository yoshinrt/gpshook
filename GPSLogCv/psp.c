/* psp.c - part of gpslogcv
mailto:hoge@hoge.com
http://hoge.com/

  SONY PSP-290 raw data

History:
  2007/04/04 V0.01 create
*/
#include <fcntl.h>
#include <io.h>
#include <float.h>
#include "gpslogcv.h"

typedef unsigned	u32;
#include "pspusbgps.h"

//#define DEBUG

int in_PSP( FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp ){
	
	static gpsdata GPS_Data;
	//static satdata tmp;
	
	if( readRec == 0 ){
		if ( _setmode( _fileno( rFile ), _O_BINARY )== -1 ){
			perror( "can't set binary mode." );
			return -1;	/* error with errno */
		}
	}
	
	do{
		if( fread( &GPS_Data, 1, sizeof( gpsdata ), rFile ) < sizeof( gpsdata )){
			// EOF‚ç‚µ‚¢
			return 0;
		}
		
		if( !(
			!_isnan( GPS_Data.latitude ) &&
			!_isnan( GPS_Data.longitude ) &&
			20  <= GPS_Data.latitude  && GPS_Data.latitude  <= 60 &&
			120 <= GPS_Data.longitude && GPS_Data.longitude <= 150
		)) GPS_Data.valid = 0;
	} while( GPS_Data.valid == 0 );
	
	position->lat	= GPS_Data.latitude;
	position->lon	= GPS_Data.longitude;
	position->alt	= GPS_Data.altitude;
	
	GPSLogp->speed	= GPS_Data.speed;
	GPSLogp->head	= GPS_Data.bearing;
	
	GPSLogp->recTime->tm_year	= GPS_Data.year - 1900;
	GPSLogp->recTime->tm_mon	= GPS_Data.month - 1;
	GPSLogp->recTime->tm_mday	= GPS_Data.date;
	GPSLogp->recTime->tm_hour	= GPS_Data.hour;
	GPSLogp->recTime->tm_min	= GPS_Data.minute;
	GPSLogp->recTime->tm_sec	= GPS_Data.second;
	GPSLogp->recTime->tm_wday	= 0;
	GPSLogp->recTime->tm_yday	= 0;
	GPSLogp->recTime->tm_isdst	= 0;
	
	//fread( &tmp, 1, sizeof( tmp ), rFile );
	
	return 1;
}
