/*
ovis*.pot から ovis.dat を生成するツール
sizeof( short ) == 2, リトルエンディアンの CPU 専用
エラー処理はもろもろ省略
*/

#include <stdio.h>

typedef unsigned short	USHORT;
typedef unsigned		UINT;

typedef struct {
	float	fLatitude, fLongitude;
	short	iDirection;
	USHORT	uCnt;
} OVIS_DATA_t;

OVIS_DATA_t *g_pOvisData;

char	g_szBuf[ 1024 ];

#define MAX_OVIS_NUM	2000

int main( int argc, char **argv ){
	
	UINT	uCnt = 0;
	int		iLatD, iLatM, iLatS, iLatS2;
	int		iLngD, iLngM, iLngS, iLngS2;
	int		iDir;
	
	double	dLat, dLong;
	
	FILE *fpIn  = fopen( argv[ 1 ], "r" );
	FILE *fpOut;
	
	g_pOvisData = ( OVIS_DATA_t *)malloc( MAX_OVIS_NUM * sizeof( OVIS_DATA_t ));
	
	memset( g_pOvisData, 0, MAX_OVIS_NUM * sizeof( OVIS_DATA_t ));
	
	while( fgets( g_szBuf, 1024 , fpIn )){
		if(
			sscanf( g_szBuf,
				"%*[^/]/%*[^/]"
				"/kei=%d'%d'%d'%d"
				"/ido=%d'%d'%d'%d"
				"/deg=%d",
				&iLngD, &iLngM, &iLngS, &iLngS2,
				&iLatD, &iLatM, &iLatS, &iLatS2,
				&iDir
			) == 9
		){
			dLat  = iLatD + ( iLatM + ( iLatS + iLatS2 / 10.0 ) / 60.0 ) / 60.0;
			dLong = iLngD + ( iLngM + ( iLngS + iLngS2 / 10.0 ) / 60.0 ) / 60.0;
			
			g_pOvisData[ uCnt ].fLatitude	= dLat  - 0.00010695  * dLat + 0.000017464 * dLong + 0.0046017;
			g_pOvisData[ uCnt ].fLongitude	= dLong - 0.000046038 * dLat - 0.000083043 * dLong + 0.010040;
			g_pOvisData[ uCnt ].iDirection	= iDir;
			
			++uCnt;
		}
	}
	
	fpOut = fopen( "ovis.dat", "wb" );
	fwrite( &uCnt, 1, sizeof( uCnt ), fpOut );
	fwrite( g_pOvisData, uCnt, sizeof( OVIS_DATA_t ), fpOut );
}
