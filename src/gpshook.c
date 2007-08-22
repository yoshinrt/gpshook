/****************************************************************************/

#include <pspkernel.h>
#include <psputilsforkernel.h>
#include <pspctrl.h>
#include <pspaudio.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <psprtc.h>
#include <psppower.h>
#include "pspusbgps.h"

#include "dds.h"

PSP_MODULE_INFO( "ddsGPSHook", 0x1000, 1, 1 );
PSP_MAIN_THREAD_ATTR( 0 );

/****************************************************************************/

#define GPSHOOK_DIR				"ms0:/seplugins/gpshook"

#define THREAD_PRIORITY 		100
#define THREAD_PRIORITY_SOUND	5		// mp3prx が 6 なので
#define MAIN_THREAD_DELAY		10000	// 100Hz

/*** 機能選択 ***************************************************************/

#ifdef DEBUG
	// 家の中でオービス接近テストするときに定義
	//   /seplugins/gpshook/gpslog.dat を再生する
	#define OVIS_TEST
#endif

#ifndef NO_DEFAULT
	#define USE_GPS_LOG				// GPS ログ機能を使用
	#define USE_DYN_MALLOC			// sceKernelAllocPartitionMemory を使用
	#define USE_STATIC_OVIS_DAT		// ovis.dat を prx に内蔵
	#define USE_SOUND				// 音声メッセージ使用
#endif

/*** キーコンフィグ *********************************************************/

#define PAD_SHIFT		PSP_CTRL_CROSS
#define PAD_LOG_START	( PAD_SHIFT | PSP_CTRL_RTRIGGER )
#define PAD_LOG_STOP	( PAD_SHIFT | PSP_CTRL_LTRIGGER )
#define PAD_AP_START	( PAD_SHIFT | PSP_CTRL_CIRCLE )
#define PAD_AP_START_R	( PAD_SHIFT | PSP_CTRL_SQUARE )
#define PAD_AP_STOP		( PAD_SHIFT | PSP_CTRL_TRIANGLE )
#define PAD_DEBUG		( PAD_SHIFT | PSP_CTRL_START )

#define PAD_MASK ( \
	PSP_CTRL_UP			| \
	PSP_CTRL_DOWN		| \
	PSP_CTRL_LEFT		| \
	PSP_CTRL_RIGHT		| \
	PSP_CTRL_CIRCLE		| \
	PSP_CTRL_CROSS		| \
	PSP_CTRL_TRIANGLE	| \
	PSP_CTRL_SQUARE		| \
	PSP_CTRL_SELECT		| \
	PSP_CTRL_START		| \
	PSP_CTRL_LTRIGGER	| \
	PSP_CTRL_RTRIGGER )

/*** new type ***************************************************************/

struct SyscallHeader {
	void *unk;
	unsigned int basenum;
	unsigned int topnum;
	unsigned int size;
};

typedef int ( *SCE_USB_GPS_OPEN )( void );		// Open the GPS device
typedef int ( *SCE_USB_GPS_CLOSE )( void );		// Close the GPS device
typedef int ( *SCE_USB_GPS_GET_DATA )( gpsdata *buffer, satdata *satellites );	// Get data from GPS ( size of buffer = 0x30 u32 ? )

typedef int ( *SCE_CTRL_READ_BUFFER )( SceCtrlData *pad_data, int count );
typedef int ( *SCE_IO_OPEN )( const char *file, int flags, SceMode mode );

/*** gloval var *************************************************************/

BOOL g_bOvisWarning		SEC_BSS_BYTE;

#ifdef USE_GPS_LOG
	volatile int	g_fdLog = -1;
	#define IsLogOpened		( g_fdLog >= 0 )
#endif

UCHAR	g_cPlayedSound	SEC_BSS_BYTE;
enum {
	PLAYED_NONE,
	PLAYED_START,
	PLAYED_END,
};

/*** read ovis data *********************************************************/

#ifdef USE_STATIC_OVIS_DAT
	typedef struct {
		float	fLatitude, fLongitude;
	} OVIS_DATA_POS_t;
	
	#include "ovis_data.h"
	
	#define OVIS_DATA_LATI( n )	( g_OvisData_Pos[ n ].fLatitude )
	#define OVIS_DATA_LONG( n )	( g_OvisData_Pos[ n ].fLongitude )
	#define OVIS_DATA_DIR( n )	g_OvisData_uDir[ n ]
	#define OVIS_DATA_CNT( n )	g_OvisData_uCnt[ n ]
	
#else
	typedef struct {
		float	fLatitude, fLongitude;
		USHORT	uDirection;
		USHORT	uCnt;
	} OVIS_DATA_t;
	
	UINT		g_uOvisCnt;
	
	#ifdef USE_DYN_MALLOC
		#define MAX_OVIS_CNT	g_uOvisCnt
		OVIS_DATA_t	*g_OvisData;
	#else
		#define MAX_OVIS_CNT	560
		OVIS_DATA_t	g_OvisData[ MAX_OVIS_CNT ];
	#endif
	
	#define OVIS_DATA_LATI( n )	( g_OvisData[ n ].fLatitude )
	#define OVIS_DATA_LONG( n )	( g_OvisData[ n ].fLongitude )
	#define OVIS_DATA_DIR( n )	( g_OvisData[ n ].uDirection )
	#define OVIS_DATA_CNT( n )	( g_OvisData[ n ].uCnt )
	
#endif

#define CFG_PARAM( name, val )	UINT g_u ## name = val;
#include "config.h"

static int atoi( char *str ){
	int	iRet = 0;
	
	while( '0' <= *str && *str <= '9' ){
		iRet = *str - '0' + iRet * 10;
		++str;
	}
	return iRet;
}

// fgets の lowlevel IO 版

INLINE char *fgets_fd( char *pBuf, int iSize, int fd ){
	int		i = 0;
	UCHAR	*p = ( UCHAR *)pBuf;
	
	--iSize;
	
	while( sceIoRead( fd, &p[ i ], 1 ) > 0 ){
		if( p[ i++ ] == '\n' || i >= iSize ) break;
	}
	p[ i ] = '\0';
	
	return i ? pBuf : NULL;
}

INLINE BOOL ReadOvisData( void ){
	
	#define LINE_BUF_SIZE	32
	
	char	szBuf[ LINE_BUF_SIZE ];
	int		fdIn;
	
	DebugMsg( "ReadOvisData start.\n" );
	
	// config ファイル open
	if(( fdIn = sceIoOpen( "gpshook.cfg", PSP_O_RDONLY, 0777 )) >= 0 ){
		while( fgets_fd( szBuf, LINE_BUF_SIZE, fdIn )){
			
			#define CFG_PARAM( name, val )	if( !strncmp( szBuf, #name "=", sizeof( #name ))){ \
				g_u ## name = atoi( szBuf + sizeof( #name )); \
			}else
			#include "config.h"
			;
		}
		sceIoClose( fdIn );
	}
	DebugCmd(
		else DebugMsg( "gpshook.cfg open failed.\n" );
	)
	
	DebugMsg( "cfg: spd=%d dist=%d ETA=%d\n", g_uWarnSpeed, g_uWarnDistance, g_uWarnETA );
	
	#ifndef USE_STATIC_OVIS_DAT
		// オービスデータファイルopen
		if(( fdIn = sceIoOpen( "ovis.dat", PSP_O_RDONLY, 0777 )) < 0 ){
			g_uOvisCnt = 0;
			return( FALSE );
		}
		
		// ovis データ用メモリ確保
		sceIoRead( fdIn, &g_uOvisCnt, sizeof( g_uOvisCnt ));
		DebugMsg( "ovis cnt=%d\n", g_uOvisCnt );
		
		#ifdef USE_DYN_MALLOC
			SceUID MemID;
			if(( MemID = sceKernelAllocPartitionMemory( 2, "GpsOBuf", 0, sizeof( OVIS_DATA_t ) * MAX_OVIS_CNT, NULL )) < 0 ){
				DebugMsg( "malloc failed.\n" );
				sceIoClose( fdIn );
				g_uOvisCnt = 0;
				return( FALSE );
			}
			
			g_OvisData = ( OVIS_DATA_t *)sceKernelGetBlockHeadAddr( MemID );
		#endif
		
		// オービスデータリード
		sceIoRead( fdIn, g_OvisData, MAX_OVIS_CNT * sizeof( OVIS_DATA_t ));
		sceIoClose( fdIn );
		
		/*
		DebugCmd(
			UINT	u = 0;
			for( u = 0; u < g_uOvisCnt; ++u ){
				Kprintf( "%d:%d:%d\n",
					( int )( g_OvisData[ u ].fLatitude  * 1000000 ),
					( int )( g_OvisData[ u ].fLongitude * 1000000 ),
					g_OvisData[ u ].uDirection
				);
			}
		)
		*/
		
		DebugMsg( "ReadOvisData compl.\n" );
	#endif
	return TRUE;
}

/*** オービスが接近しているか? **********************************************/

#define LAT_M_DEG	110863.95F	// 緯度1度の距離
#define LNG_M_DEG	111195.10F	// 経度1度の距離 @ 0N

// 微精度 cos ( 27〜44度位のcos の近似値 )
// cos() と違って，ang の単位は[度]
INLINE float cos_dmy( float ang ){
	//     cos(30)     cos(44)-cos(30)
	return 0.866025404F - 0.146685603F / 14 * ( ang - 30 );
}

// WGS84->Tokyo 変換
/*
INLINE void ConvWGS84_Tokyo( float *fLat, float *fLong ){
	
	float	fLat2 = *fLat;
	float	fLong2 = *fLong;
	
	*fLat  = fLat2  + 0.00010696  * fLat2 - 0.000017467 * fLong2 - 0.0046020;
	*fLong = fLong2 + 0.000046047 * fLat2 + 0.000083049 * fLong2 - 0.010041;
}
*/

INLINE float Pow2( float f ){ return f * f; }

// 2点間が fDist[m] より近い?
INLINE BOOL IsDistanceNear( float fLat1, float fLong1, float fLat2, float fLong2, float fDist ){
	
	float fDistLat;
	if(( fDistLat = fabs( fLat1 - fLat2 ) * LAT_M_DEG ) >= fDist ) return FALSE;
	
	return (
		Pow2( fDistLat ) +
		Pow2(( fLong1 - fLong2 ) * LNG_M_DEG * cos_dmy( fLat1 ))
	) < Pow2( fDist );
}

INLINE BOOL IsOvisWarn( gpsdata *buffer ){
	
	UINT	u;
	BOOL	bRet = FALSE;
	float	fLat, fLong;
	
	UINT	uDir = 1 << (( UINT )( int )buffer->bearing * 16 / 360 );
	
	if( buffer->speed >= g_uWarnSpeed ){
		fLat  = buffer->latitude;
		fLong = buffer->longitude;
		// WGS84→Tokyo 変換
		//ConvWGS84_Tokyo( &fLat, &fLong );
		
		//DebugMsg( "IsOvisWarn:" );
		
		int	iDist = g_uWarnETA ? g_uWarnETA * ( UINT )( int )buffer->speed * 1000 / 3600 : g_uWarnDistance;
		
		for( u = 0; u < g_uOvisCnt; ++u ){
			if(
				( OVIS_DATA_DIR( u ) & uDir ) &&
				IsDistanceNear( fLat, fLong, OVIS_DATA_LATI( u ), OVIS_DATA_LONG( u ), iDist )
			){
				if( OVIS_DATA_CNT( u ) == 0 ){
					// 警告対象
					bRet = TRUE;
				}
				OVIS_DATA_CNT( u ) = 100;
			}else{
				// 警告対象ではない
				if( OVIS_DATA_CNT( u )) --OVIS_DATA_CNT( u );
			}
		}
	}
	
	//DebugMsg( "%d\n", bRet );
	return bRet;
}

/*** サウンド ***************************************************************/

#ifdef USE_SOUND

#define SOUND_BUF_SIZE			( 1 * 1024 )
#define SOUND_LOAD_SIZE			( SOUND_BUF_SIZE >> 3 )
#define SOUND_BUF_SAMPLES		( SOUND_BUF_SIZE >> 2 )
#define SOUND_BUF_SIZE_ALIGNED	PSP_AUDIO_SAMPLE_ALIGN( SOUND_BUF_SIZE + 1 )

#ifndef USE_DYN_MALLOC
	UCHAR	g_SoundBuf[ SOUND_BUF_SIZE_ALIGNED ];
#endif

INLINE int SoundPlay( char *szSndFile, UINT uSoundLoop ){
	
	BOOL	bRet = TRUE;
	UCHAR	*pBuf, *pBufLoad;
	UINT	*pBufAligned;
	int		fd;
	UINT	uSize, u;
	UINT	uSample;
	UINT	uLoop;
	
	DebugMsg( "SoundPlay thread started\n" );
	
	int thid = sceKernelGetThreadId();
	sceKernelChangeThreadPriority( thid, THREAD_PRIORITY_SOUND );
	
	DebugMsg( "Playing...\n" );
	
	// ch 確保
	int iCh = sceAudioChReserve(
		PSP_AUDIO_NEXT_CHANNEL,
		SOUND_BUF_SAMPLES,
		PSP_AUDIO_FORMAT_STEREO
	);
	
	DebugMsg( "iCh:%d\n", iCh );
	if( iCh < 0 ){
		bRet = FALSE;
		goto Exit;
	}
	
	#ifdef USE_DYN_MALLOC
		// malloc
		SceUID MemID;
		if(( MemID = sceKernelAllocPartitionMemory( 2, "GpsSBuf", 0, SOUND_BUF_SIZE_ALIGNED, NULL )) < 0 ){
			DebugMsg( "malloc failed.\n" );
			bRet = FALSE;
			goto ChRelease;
		}
		
		pBuf = sceKernelGetBlockHeadAddr( MemID );
	#else
		pBuf = g_SoundBuf;
	#endif
	
	pBufAligned = ( UINT *)PSP_AUDIO_SAMPLE_ALIGN(( UINT )pBuf );
	pBufLoad	= ( UCHAR *)pBufAligned + SOUND_BUF_SIZE - SOUND_LOAD_SIZE;
	
	// read wav
	DebugMsg( "reading wav\n" );
	
	if(( fd = sceIoOpen( szSndFile, PSP_O_RDONLY, 0777 )) < 0 ){
		DebugMsg( "fopen failed\n" );
		bRet = FALSE;
		goto FreeMem;
	}
	
	for( uLoop = 0; uLoop < uSoundLoop; ++uLoop ){
		sceIoLseek( fd, 0x3A, SEEK_SET );
		
		while(( uSize = sceIoRead( fd, pBufLoad, SOUND_LOAD_SIZE ))){
			for( u = 0; u < SOUND_LOAD_SIZE; ++u ){
				if( u < uSize ){
					uSample = ( pBufLoad[ u ] - 128 ) << 24;
					uSample |= uSample >> 16;
				}else{
					uSample = 0;
				}
				
				pBufAligned[ u * 2 ] =
				pBufAligned[ u * 2 + 1 ] = uSample;
			}
			// play sound
			sceAudioOutputBlocking( iCh, PSP_AUDIO_VOLUME_MAX, pBufAligned );
		}
	}
	sceIoClose( fd );
	
  FreeMem:
	#ifdef USE_DYN_MALLOC
		sceKernelFreePartitionMemory( MemID );
  ChRelease:
	#endif
	
	sceAudioChRelease( iCh );
	
  Exit:
	DebugMsg( "Playing done.\n" );
	
	sceKernelChangeThreadPriority( thid, THREAD_PRIORITY );
	
	return 0;
}
#else	// USE_SOUND
	#define SoundPlay( file, cnt )
#endif	// USE_SOUND

/*** GPS フック関数 *********************************************************/

//SCE_USB_GPS_OPEN		sceUsbGpsOpen_Real;
SCE_USB_GPS_CLOSE		sceUsbGpsClose_Real;
SCE_USB_GPS_GET_DATA	sceUsbGpsGetData_Real;

enum {
	LOG_DONE,				// 何もしない
	LOG_CLOSE_REQ,			// Log クローズ要求
	LOG_CLOSE_DONE,			// Log クローズ完了
	LOG_OPEN_REQ,			// オープン要求
};
volatile UCHAR	g_cReOpenLog	SEC_BSS_BYTE;	// レジューム時にログ再オープン

/*
int sceUsbGpsOpen_Hook( void ){
	DebugMsg( "USB GPS Open\n" );
	
	return sceUsbGpsOpen_Real();
}
*/

int sceUsbGpsClose_Hook( void ){
	#ifdef USE_GPS_LOG
		if( IsLogOpened ) sceIoClose( g_fdLog );
	#endif
	DebugMsg( "USB GPS Close\n" );
	return sceUsbGpsClose_Real();
}

#ifdef OVIS_TEST
	BOOL	g_bDebugPushButt;
	UINT	g_uDebugGpsLogProceed = 1;
	gpsdata	g_GpsTestData;
#endif

USHORT	g_uGpsLogCnt	SEC_BSS_WORD;

int sceUsbGpsGetData_Hook( gpsdata *buffer, satdata *satellites ){
	static UCHAR	cSec SEC_DATA_BYTE = 255;
	
	int iRet = sceUsbGpsGetData_Real( buffer, satellites );
	
	#if 0
		static int	iLngCnt = 100;
		if( g_bDebugPushButt ){
			buffer->hdop		= 1;
			buffer->valid		= 1;
			buffer->dimension	= 2;
			buffer->latitude	= 35 + ( 3 + 2.62 / 60 ) / 60;
			buffer->longitude	= 135 + ( 55 + 11.20 / 60 ) / 60 + iLngCnt / 3600.0;
			buffer->altitude	= 0;
			buffer->speed		= 120;
			buffer->bearing		= 245;
			
			if( --iLngCnt < -100 ) iLngCnt = 300;
		}
	#endif
	#ifdef OVIS_TEST
		if( g_bDebugPushButt ){
			buffer->hdop		= g_GpsTestData.hdop;
			buffer->valid		= g_GpsTestData.valid;
			buffer->dimension	= g_GpsTestData.dimension;
			buffer->latitude	= g_GpsTestData.latitude;
			buffer->longitude	= g_GpsTestData.longitude;
			buffer->altitude	= g_GpsTestData.altitude;
			buffer->speed		= g_GpsTestData.speed;
			buffer->bearing		= g_GpsTestData.bearing;
		}
	#endif
	
	if( cSec != buffer->second && buffer->valid ){
		// GPS ログ出力
		#ifdef USE_GPS_LOG
			if( IsLogOpened ){
				sceIoWrite( g_fdLog, buffer,     sizeof( gpsdata ));
				//sceIoWrite( g_fdLog, satellites, sizeof( satdata ));
			}
		#endif
		++g_uGpsLogCnt;
		
		/*
		DebugMsg( "location: %d %d %d : %dm cnt=%d\n",
			( int )( fLat * 1000000 ),
			( int )( fLong * 1000000 ),
			( int )buffer->bearing,
			g_uOvisCnt ? ( int )IsDistanceNear( fLat, fLong, g_OvisData[ 546 ].fLatitude, g_OvisData[ 546 ].fLongitude ) : 99999,
			g_uOvisCnt ? g_OvisData[ 546 ].uCnt : 99999
		);
		*/
		
		// オービスチェック
		g_bOvisWarning = IsOvisWarn( buffer );
		
		cSec = buffer->second;
	}
	
	return iRet;
}

/*** パッドフック関数 *******************************************************/

/*
SCE_CTRL_READ_BUFFER sceCtrlPeekBufferPositive_Real;
int sceCtrlPeekBufferPositive_Hook( SceCtrlData *pad_data, int count ){
	DebugMsg( "@PP:%d", count );
	return sceCtrlPeekBufferPositive_Real( pad_data, count );
}

SCE_CTRL_READ_BUFFER sceCtrlPeekBufferNegative_Real;
int sceCtrlPeekBufferNegative_Hook( SceCtrlData *pad_data, int count ){
	DebugMsg( "@PN:%d", count );
	return sceCtrlPeekBufferNegative_Real( pad_data, count );
}
*/

USHORT	g_uCtrlReadCnt		SEC_BSS_WORD;

UCHAR	g_cAutoPilotMode	SEC_BSS_BYTE;
enum {
	AUTO_PILOT_OPENING,
	AUTO_PILOT_ROUTE_START,
	AUTO_PILOT_ROUTING,
	AUTO_PILOT_NONE,
};

UCHAR	g_cNaviMode		SEC_BSS_BYTE;
enum {
	NAVI_NONE,
	NAVI_NORMAL,
	NAVI_REVERSE,
};

UCHAR	g_cNaviSpotNum	SEC_BSS_BYTE;
short	g_iNaviFolderID	SEC_BSS_WORD;
int		g_iNaviSpotID;

UCHAR	g_cShiftKeyMode	SEC_BSS_BYTE;
enum {
	SHIFT_KEY_NONE,
	SHIFT_KEY_REQ,
	SHIFT_KEY_CANCEL,
};

#define KEY_WAIT( n )	(( uCnt += ( n )) == g_uCtrlReadCnt )

SCE_CTRL_READ_BUFFER sceCtrlReadBufferPositive_Real;
int sceCtrlReadBufferPositive_Hook( SceCtrlData *pad_data, int count ){
	
	static UCHAR	cFolderID	SEC_BSS_BYTE;
	static UCHAR	cSpotID		SEC_BSS_BYTE;
	
	//DebugMsg( "@RP:%d", count );
	//DebugMsg( "%d %d %d\n", g_cAutoPilotMode, g_cNaviMode, g_uCtrlReadCnt );
	int iRet = sceCtrlReadBufferPositive_Real( pad_data, count );
	
	#ifdef DEBUG
		if( pad_data->Lx >= 128 ){
			g_uDebugGpsLogProceed = 1 << (( pad_data->Lx - 128 ) * 5 / 120 );
		}else{
			g_uDebugGpsLogProceed = 1;
		}
		
		pad_data->Lx = 127;
		pad_data->Ly = 127;
	#endif
	
	/*** シフトキーの処理 ***/
	// シフトキーが離された瞬間に，そのキーを押す
	if( !(( pad_data->Buttons & PAD_MASK ) || ( g_cShiftKeyMode == SHIFT_KEY_NONE ))){
		
		if( g_cShiftKeyMode == SHIFT_KEY_REQ ) pad_data->Buttons	= PAD_SHIFT;
		g_cShiftKeyMode		= SHIFT_KEY_NONE;
		
		return iRet;
	}
	
	if( pad_data->Buttons & PAD_SHIFT ){
		if( g_cShiftKeyMode != SHIFT_KEY_CANCEL && ( pad_data->Buttons & PAD_MASK ) == PAD_SHIFT ){
			// シフトキーだけ押されたら，離されたときに押すようにする
			g_cShiftKeyMode = SHIFT_KEY_REQ;
		}else{
			// シフトキー + 何か が押されたら，シフトキー自体の入力はしない
			g_cShiftKeyMode = SHIFT_KEY_CANCEL;
		}
		pad_data->Buttons = 0;
		return iRet;
	}
	
	/******/
	
	if( g_cAutoPilotMode == AUTO_PILOT_NONE ){
		return iRet;
	}
	//DebugMsg( ">%d<", g_uGpsLogCnt );
	//DebugMsg( "%c", pad_data->Buttons & PSP_CTRL_NOTE ? '#' : '.' );
	
	// ボタンが押されたら，または GPS 捕捉開始したら，自動キー入力停止
	if(
		(
			pad_data->Buttons & (
				PSP_CTRL_UP			|
				PSP_CTRL_DOWN		|
				PSP_CTRL_LEFT		|
				PSP_CTRL_RIGHT		|
				PSP_CTRL_CIRCLE		|
				PSP_CTRL_CROSS		|
			//	PSP_CTRL_TRIANGLE	|
			//	PSP_CTRL_SQUARE		|
				PSP_CTRL_SELECT		|
				PSP_CTRL_START		|
				PSP_CTRL_LTRIGGER	|
				PSP_CTRL_RTRIGGER
			)
		)
		|| ( g_cAutoPilotMode == AUTO_PILOT_OPENING && g_uGpsLogCnt >= 20 )
	){
		g_cAutoPilotMode	= AUTO_PILOT_NONE;
	}
	
	/************************************************************************/
	
	UINT	uCnt = 0;
	
	DebugMsg( "@%d:%d\n", g_cAutoPilotMode, g_uCtrlReadCnt );
	
	// オープニングのキー入力
	if( g_cAutoPilotMode == AUTO_PILOT_OPENING ){
		if     ( KEY_WAIT(   0 )) pad_data->Buttons = PSP_CTRL_START;
		else if( KEY_WAIT( 100 )) pad_data->Buttons = PSP_CTRL_CIRCLE;
		
		// ×連打
		else if( g_uCtrlReadCnt & 1 ) pad_data->Buttons |= PSP_CTRL_CROSS;
		
	}else if( g_cAutoPilotMode == AUTO_PILOT_ROUTE_START ){
		if( KEY_WAIT( 100 )){
			g_cAutoPilotMode	= AUTO_PILOT_ROUTING;
			//g_uCtrlReadCnt		= 0x80000000;
			g_uCtrlReadCnt		= 0x8000;
		}
	}else if( g_cAutoPilotMode == AUTO_PILOT_ROUTING ){
		
		// 既知の状態に戻す
		if     ( KEY_WAIT(  1 )) pad_data->Buttons = PSP_CTRL_CIRCLE;
		else if( KEY_WAIT( 50 )) pad_data->Buttons = PSP_CTRL_CIRCLE;
		else if( KEY_WAIT( 50 )) pad_data->Buttons = PSP_CTRL_CROSS;
		else if( KEY_WAIT( 50 )) pad_data->Buttons = PSP_CTRL_CROSS;
		else if( KEY_WAIT( 50 )) pad_data->Buttons = PSP_CTRL_CROSS;
		
		//メニュー
		else if( KEY_WAIT( 50 )) pad_data->Buttons = PSP_CTRL_CIRCLE;
		else if( KEY_WAIT( 30 )) pad_data->Buttons = PSP_CTRL_CIRCLE;
		
		//目的地
		else if( KEY_WAIT( 50 )) pad_data->Buttons = PSP_CTRL_LEFT;
		else if( KEY_WAIT( 20 )) pad_data->Buttons = PSP_CTRL_LEFT;
		else if( KEY_WAIT( 20 )){
			pad_data->Buttons = PSP_CTRL_CIRCLE;
			// フォルダ・スポットID ロード
			cFolderID		= g_iNaviFolderID;
			cSpotID			= g_iNaviSpotID;
			g_cPlayedSound	= PLAYED_NONE;
		}
		
		//フォルダ選択
		else if( KEY_WAIT( 50 )){
			if( cFolderID-- ){
				pad_data->Buttons	= PSP_CTRL_DOWN;
				g_uCtrlReadCnt		= uCnt - 20;
			}
		}
		
		else if( KEY_WAIT( 20 )) pad_data->Buttons = PSP_CTRL_CIRCLE;
		
		//スポット選択
		else if( KEY_WAIT( 20 )){
			if( cSpotID-- ){
				pad_data->Buttons	= PSP_CTRL_DOWN;
				g_uCtrlReadCnt		= uCnt - 20;
			}
		}
		
		// スポット確定・クイックスタート
		else if( KEY_WAIT( 20 ) || g_uCtrlReadCnt >= uCnt ){
			if( g_cPlayedSound == PLAYED_START ){
				// ○が受け付けられた
				g_cAutoPilotMode	= AUTO_PILOT_NONE;
				g_cPlayedSound		= PLAYED_NONE;
			}else{
				pad_data->Buttons	= PSP_CTRL_CIRCLE;
				g_uCtrlReadCnt = uCnt - 20;
			}
		}
	}
	++g_uCtrlReadCnt;
	
	return iRet;
}

/*
SCE_CTRL_READ_BUFFER sceCtrlReadBufferNegative_Real;
int sceCtrlReadBufferNegative_Hook( SceCtrlData *pad_data, int count ){
	DebugMsg( "@RN:%d", count );
	return sceCtrlReadBufferNegative_Real( pad_data, count );
}
*/

/*** fileIo フック関数 *********************************************************/

#define SET_SOUND_FLAG( name, id ) \
	if( strstr( file, name )){ g_cPlayedSound = id; } else

SCE_IO_OPEN sceIoOpen_Real;
int sceIoOpen_Hook( const char *file, int flags, SceMode mode ){
	
	int iRet = sceIoOpen_Real( file, flags, mode );
	
	SET_SOUND_FLAG( "B_43.vag",			PLAYED_END )	// 終了します
	SET_SOUND_FLAG( "start_guide.vag",	PLAYED_START )	// 案内開始
	;
	
	DebugMsg( "snd:%s\n", file );
	
	return iRet;
}

/*** API フック用 ***********************************************************/

INLINE u32 NIDByName( const char *name ){
	u8 digest[20];
	u32 nid;
	
	if( sceKernelUtilsSha1Digest(( u8 * ) name, strlen( name ), digest ) >= 0 ){
		nid = digest[0] | ( digest[1] << 8 ) | ( digest[2] << 16 ) | ( digest[3] << 24 );
		DebugMsg( "NID:%08X:%s\n", nid, name );
		return nid;
	}
	
	return 0;
}

INLINE u32 FindNID( char modname[27], u32 nid ){
	struct SceLibraryEntryTable *entry;
	void *entTab;
	int entLen;
	
	SceModule *tmpmod, *pMod = NULL;
	SceUID ids[100];
	int count = 0;
	int p;
	
	memset( ids, 0, 100 * sizeof( SceUID ));
	
	sceKernelGetModuleIdList( ids, 100 * sizeof( SceUID ), &count );
	
	for( p = 0; p < count; p++ ){
		tmpmod = sceKernelFindModuleByUID( ids[p] );
		
		//DebugMsg( "FindNID:modname:%s\n", tmpmod->modname );
		
		if( strcmp( tmpmod->modname, modname ) == 0 ){
			pMod = tmpmod;
		}
	}
	
	if( pMod != NULL ){
		int i = 0;
		
		entTab = pMod->ent_top;
		entLen = pMod->ent_size;
		while( i < entLen ){
			int count;
			int total;
			unsigned int *vars;
			
			entry = ( struct SceLibraryEntryTable * ) ( entTab + i );
			
			total = entry->stubcount + entry->vstubcount;
			vars = entry->entrytable;
			
			if( entry->stubcount > 0 ){
				for( count = 0; count < entry->stubcount; count++ ){
					if( vars[count] == nid ){
						DebugMsg( "FindNID:%X\n", vars[count+total] );
						return vars[count+total];
					}
				}
			}
			i += ( entry->len * 4 );
		}
	}
	
	DebugMsg( "FindNID:0\n" );
	return 0;
}

INLINE void *find_syscall_addr( u32 addr ){
	struct SyscallHeader *head;
	u32 *syscalls;
	void **ptr;
	int size;
	int i;
	
	asm( 
		"cfc0 %0, $12\n"
		: "=r"( ptr )
	 );
	
	if( !ptr ){
		DebugMsg( "find_syscall_addr:NULL\n" );
		return NULL;
	}
	
	head = ( struct SyscallHeader * ) *ptr;
	syscalls = ( u32* ) ( *ptr + 0x10 );
	size = ( head->size - 0x10 );
	
	for( i = 0; i < size; i++ ){
		if( syscalls[i] == addr ){
			DebugMsg( "find_syscall_addr:%X\n", ( u32 )&syscalls[i] );
			return &syscalls[i];
		}
	}
	
	DebugMsg( "find_syscall_addr:NULL\n" );
	return NULL;
}


INLINE void *apiHookAddr( u32 *addr, void *func ){
	if( !addr ){
		return NULL;
	}
	*addr = ( u32 ) func;
	sceKernelDcacheWritebackInvalidateRange( addr, sizeof( addr ));
	sceKernelIcacheInvalidateRange( addr, sizeof( addr ));
	
	DebugMsg( "apiHookAddr:%X\n", ( u32 )addr );
	return addr;
}

INLINE u32 PatchNID( char modname[27], const char *funcname, void *func ){
	u32 nidaddr = FindNID( modname, NIDByName( funcname ));
	
	if( nidaddr > 0x80000000 ){
		if( !apiHookAddr( find_syscall_addr( nidaddr ), func )){
			nidaddr = 0;
		}
	}
	
	DebugMsg( "PatchNID:%X\n", nidaddr );
	return nidaddr;
}

INLINE u32 PatchNID2( char modname[27], u32 uNID, void *func ){
	u32 nidaddr = FindNID( modname, uNID );
	
	if( nidaddr > 0x80000000 ){
		if( !apiHookAddr( find_syscall_addr( nidaddr ), func )){
			nidaddr = 0;
		}
	}
	
	DebugMsg( "PatchNID:%X\n", nidaddr );
	return nidaddr;
}

/*** ログファイル名生成 ******************************************************/

#ifdef USE_GPS_LOG

INLINE char *ItoA( char *szBuf, UINT uNum, UINT uWidth ){
	
	UINT	u;
	szBuf += uWidth;
	
	for( u = 1; u <= uWidth; ++u ){
		szBuf[ -u ] = uNum % 10 + '0';
		uNum /= 10;
	}
	return szBuf;
}

/* ↓こうしたいだけ．printf系を排除する事でバイナリサイズを抑える
sprintf(
	szBuf,
	"log%04d%02d%02d_%02d%02d%02d.dat",
	NowTime.year,	NowTime.month,		NowTime.day,
	NowTime.hour,	NowTime.minutes,	NowTime.seconds
);
*/
INLINE char *MakeLogFileName( char *szBuf ){
	pspTime	NowTime;
	
	sceRtcGetCurrentClockLocalTime( &NowTime );
	
	ItoA( szBuf +  3, NowTime.year,		2 );
	ItoA( szBuf +  5, NowTime.month,	2 );
	ItoA( szBuf +  7, NowTime.day,		2 );
	ItoA( szBuf + 10, NowTime.hour,		2 );
	ItoA( szBuf + 12, NowTime.minutes,	2 );
	ItoA( szBuf + 14, NowTime.seconds,	2 );
	
	return szBuf;
}

void ReOpenLog( void ){
	
	static char szBuf[]	SEC_DATA_BYTE = "log000000_000000.dat";
	
	MakeLogFileName( szBuf );
	
	if( IsLogOpened ) sceIoClose( g_fdLog );
	g_fdLog = sceIoOpen( szBuf, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777 );
	
	//DebugMsg( "ReOpen:%X:%s\n", ( UINT )g_fdLog, szBuf );
	g_uGpsLogCnt = 0;
	g_cReOpenLog = LOG_DONE;
}

/*** パワーステータス コールバック ******************************************/

int g_iPowCB_ID;

int PowerCallback( int unknown, int pwrflags, void *arg ){
	// Combine pwrflags and the above defined masks
	
	//DebugMsg( "Power:%08X\n", pwrflags );
	
	if( pwrflags & PSP_POWER_CB_POWER_SWITCH ){
		
		DebugMsg( "Power:suspend\n" );
		DebugCmd( pspDebugSioDisableKprintf(); )
		
		// サスペンドに入るので，ログを閉じる
		if( IsLogOpened ){
			g_cReOpenLog = LOG_CLOSE_REQ;
			// ↓意味ない?
			//while( g_cReOpenLog != LOG_CLOSE_DONE ) sceKernelDelayThread( MAIN_THREAD_DELAY );
		}
	}else if( pwrflags & PSP_POWER_CB_RESUME_COMPLETE ){
		// レジューム完了
		if( g_cReOpenLog == LOG_CLOSE_DONE ){
			g_cReOpenLog = LOG_OPEN_REQ;
			g_bOvisWarning = TRUE;
		}
		//DebugMsg( "Power:resume complete\n" );
	}
	
	scePowerRegisterCallback( 1, g_iPowCB_ID );
	
	return 0;
}

int PowerCallbackThread( SceSize args, void *argp ){
	g_iPowCB_ID = sceKernelCreateCallback( "PowCB", PowerCallback, NULL );
	DebugMsg( "PowCBThread:%d\n", g_iPowCB_ID );
	scePowerRegisterCallback( 1, g_iPowCB_ID );
	
	sceKernelSleepThreadCB();
	
	return 0;
}
#endif // USE_GPS_LOG

/*** ナビ開始 ***************************************************************/

#define MAX_FOLDER_NUM	50
#define MAX_SPOT_NUM	50

typedef struct {
	UINT	uIcon;
	short	szName[ 102 / 2 ];
	short	szComment[ 194 / 2 ];
	UINT	uLati;
	UINT	uLong;
} MAPLUS_SPOT;

typedef struct {
	short		szName[ 22 ];
	UINT		uNum;
	MAPLUS_SPOT	Spot[ MAX_SPOT_NUM ];
} MAPLUS_FOLDER;

typedef struct {
	UCHAR			cHeader[ 0x24 ];
	MAPLUS_FOLDER	Folder[ MAX_FOLDER_NUM ];
} MAPLUS_DATA;

#define MAPLUS_HEADER_SIZE			offsetof( MAPLUS_DATA, Folder )
#define MAPLUS_FOLDER_SIZE			sizeof( MAPLUS_FOLDER )
#define MAPLUS_FOLDER_HEADER_SIZE	offsetof( MAPLUS_FOLDER, uNum )

enum {
	NAVI_START_NORMAL,
	NAVI_START_FOLDER,
	NAVI_START_NEXT,
};

#define FAVORITE_DAT	"ms0:/PSP/SAVEDATA/ULJS00091FAVORITE/FAVORITE.DAT"

static void StartNavi( UCHAR cNaviMode, int iFolderID, int iSpotID ){
	int fd;
	
	// スポット数 get
	g_cNaviSpotNum = 0;
	
	if(( fd = sceIoOpen( FAVORITE_DAT, PSP_O_RDONLY, 0777 )) > 0 ){
		sceIoLseek(
			fd,
			MAPLUS_HEADER_SIZE + MAPLUS_FOLDER_HEADER_SIZE +
			MAPLUS_FOLDER_SIZE * g_iNaviFolderID,
			SEEK_SET
		);
		
		sceIoRead( fd, &g_cNaviSpotNum, sizeof( g_cNaviSpotNum ));
		sceIoClose( fd );
	}
	DebugCmd(
		else DebugMsg( "favo.dat open failed.\n" );
	)
	
	if( g_cNaviSpotNum ){
		SoundPlay( cNaviMode == NAVI_NORMAL ? "ap_start.wav" : "apr_start.wav", 1 );
		
		// folder/spot id の調整
		if(( g_iNaviFolderID = ( iFolderID % 50             )) < 0 ) g_iNaviFolderID += 50;
		if(( g_iNaviSpotID   = ( iSpotID   % g_cNaviSpotNum )) < 0 ) g_iNaviSpotID   += g_cNaviSpotNum;
		
		///
		g_uCtrlReadCnt		= 0;
		g_cAutoPilotMode	= AUTO_PILOT_ROUTE_START;
		g_cNaviMode			= cNaviMode;
	}
	
	DebugMsg(
		"Foler:%d Spot:%d Num:%d Mode:%d APmode:%d\n",
		g_iNaviFolderID,
		g_iNaviSpotID,
		g_cNaviSpotNum,
		g_cNaviMode,
		g_cAutoPilotMode
	);
}

/*** メイン *****************************************************************/

#define HOOK_API( mod, func )		func ## _Real = ( void* )PatchNID( mod, #func, func ## _Hook )
#define HOOK_API2( mod, func, nid )	func ## _Real = ( void* )PatchNID2( mod, nid, func ## _Hook )

//Keep our module running
int main_thread( SceSize args, void *argp ) {
	
	unsigned int paddata_old = 0;
	SceCtrlData	paddata;
	
	sceIoChdir( GPSHOOK_DIR );
	
	int	iFolderID, iKeyCnt = 0;
	
	// GPS モジュール ロード待ち
	while( !sceKernelFindModuleByName( "sceUSBGps_Driver" ))
		sceKernelDelayThread( MAIN_THREAD_DELAY );
	
	// GPS API フック開始
	DebugMsg( "USB GPS hook start\n" );
	
	//HOOK_API( "sceUSBGps_Driver", sceUsbGpsOpen );
	//HOOK_API( "sceUSBGps_Driver", sceUsbGpsClose );
	//HOOK_API( "sceUSBGps_Driver", sceUsbGpsGetData );
	HOOK_API2( "sceUSBGps_Driver", sceUsbGpsClose, 		0x6EED4811 );
	HOOK_API2( "sceUSBGps_Driver", sceUsbGpsGetData,	0x934EC2B2 );
	
	//HOOK_API( "sceController_Service", sceCtrlPeekBufferPositive );
	//HOOK_API( "sceController_Service", sceCtrlPeekBufferNegative );
	//HOOK_API( "sceController_Service", sceCtrlReadBufferPositive );
	//HOOK_API( "sceController_Service", sceCtrlReadBufferNegative );
	HOOK_API2( "sceController_Service", sceCtrlReadBufferPositive, 0x1F803938 );
	
	//HOOK_API( "sceIOFileManager", sceIoOpen );
	HOOK_API2( "sceIOFileManager", sceIoOpen, 0x109F50BC );
	
	// スタート画面が出るまで wait
	while( !g_uCtrlReadCnt ) sceKernelDelayThread( MAIN_THREAD_DELAY );
	
	// ovis データロード
	ReadOvisData();
	
	#ifdef USE_GPS_LOG
		int thid = sceKernelCreateThread( "GpsPow", PowerCallbackThread, THREAD_PRIORITY, 0x400, 0, NULL );
		if( thid >= 0 ) sceKernelStartThread( thid, 0, 0 );
	#endif
	
	while( 1 ){
		sceKernelDelayThread( MAIN_THREAD_DELAY );
		
		#ifdef USE_GPS_LOG
			// サスペンド時に log クローズ
			if( g_cReOpenLog == LOG_CLOSE_REQ ){
				DebugMsg( "@" );
				sceIoClose( g_fdLog );
				g_fdLog = -1;
				g_cReOpenLog = LOG_CLOSE_DONE;
			}
			
			// クラッシュ時のために，定期的に再オープン
			
			if(
				( IsLogOpened && g_uLogInterval && ( g_uGpsLogCnt >= g_uLogInterval * 60 )) ||
				( g_cReOpenLog == LOG_OPEN_REQ )
			){
				ReOpenLog();
			}
		#endif
		
		/*** オービス警告? **************************************************/
		
		if( g_bOvisWarning ){
			g_bOvisWarning = FALSE;
			SoundPlay( "warning.wav", g_uSoundLoop );
		}
		
		/*** auto pilot 処理 ************************************************/
		
		if( g_cPlayedSound == PLAYED_END ){
			g_cPlayedSound = PLAYED_NONE;
			
			DebugMsg( "Nav finished..." );
			
			if(
				( g_cNaviMode == NAVI_NORMAL  && ++g_iNaviSpotID < g_cNaviSpotNum ) ||
				( g_cNaviMode == NAVI_REVERSE && --g_iNaviSpotID >= 0 )
			){
				g_uCtrlReadCnt		= 0;
				g_cAutoPilotMode	= AUTO_PILOT_ROUTING;
				DebugMsg( "Starting next\n" );
			}else{
				g_cNaviMode = NAVI_NONE;
				DebugMsg( "cmpl.\n" );
			}
		}
		
		/*** gps ログロード (for debug) *************************************/
		
		#ifdef OVIS_TEST
			static UINT	uGpsCnt		= -1;
			static int	fdGpsTest	= -1;
			
			if( g_uGpsLogCnt != uGpsCnt && fdGpsTest > 0 ){
				uGpsCnt = g_uGpsLogCnt;
				int i = g_uDebugGpsLogProceed;
				while( i-- ) sceIoRead( fdGpsTest, &g_GpsTestData, sizeof( gpsdata ));
			}
		#endif
		
		/*** キー取得 *******************************************************/
		
		sceCtrlPeekBufferPositive( &paddata, 1 );
		paddata.Buttons &= PAD_MASK;
		
		if( paddata.Buttons != paddata_old ){
			//DebugMsg( "PAD:%08X\n", paddata.Buttons );
			
			// デバッグ動作起動
			#ifdef OVIS_TEST
				if( paddata.Buttons == PAD_DEBUG ){
					if( fdGpsTest < 0 ) fdGpsTest = sceIoOpen( "gpslog.dat", PSP_O_RDONLY, 0777 );
					
					DebugMsg( "log:%d fd:%d\n", g_cReOpenLog, g_fdLog );
					g_bDebugPushButt = TRUE;
				}else
			#endif
			
			// ログ取り関係
			#ifdef USE_GPS_LOG
				if( paddata.Buttons == PAD_LOG_START ){
					// ログ取り開始
					if( !IsLogOpened ) ReOpenLog();
					SoundPlay( "log_start.wav", 1 );
				}else if( paddata.Buttons == PAD_LOG_STOP ){
					// ログ取り終了
					if( IsLogOpened ){
						sceIoClose( g_fdLog );
						g_fdLog = -1;
						DebugMsg( "log close\n" );
					}
					SoundPlay( "log_stop.wav", 1 );
				}else
			#endif
			
			// オートパイロット関係
			if( paddata.Buttons == PAD_AP_START ){
				// AP 開始
				StartNavi( NAVI_NORMAL, iFolderID, iKeyCnt );
				
			}else if( paddata.Buttons == PAD_AP_START_R ){
				// AP リバースモード開始
				StartNavi( NAVI_REVERSE, iFolderID, iKeyCnt );
				
			}else if( paddata.Buttons == PAD_AP_STOP ){
				// AP終了
				g_cNaviMode = NAVI_NONE;
				SoundPlay( "ap_stop.wav", 1 );
				
			}else if( paddata.Buttons == PSP_CTRL_CIRCLE ){
				// folder 確定
				iFolderID = iKeyCnt;
				iKeyCnt = 0;
				DebugMsg( "CIRCLE\n" );
			}else if( paddata_old == PSP_CTRL_CROSS ){
				// folder 選択に戻る
				iKeyCnt = 0;
				
			}else if( paddata.Buttons == PSP_CTRL_DOWN ){
				++iKeyCnt;
				DebugMsg( "iKeyCnt:%d\n", iKeyCnt );
				
			}else if( paddata.Buttons == PSP_CTRL_UP ){
				--iKeyCnt;
				DebugMsg( "iKeyCnt:%d\n", iKeyCnt );
			}
			
			paddata_old = paddata.Buttons;
		}
	}
	return 0;
}

int module_start( SceSize args, void *argp ) __attribute__(( alias( "_start" )));
int _start( SceSize args, void *argp ){
	SceUID thid;
	
	DebugCmd(
		pspDebugSioInit();
		pspDebugSioSetBaud( 115200 );
		pspDebugSioInstallKprintf();
	)
	
	DebugMsg( "Loading gpshook\n" );
	
	thid = sceKernelCreateThread( "GpsMain", main_thread, THREAD_PRIORITY, 0x1000, 0, NULL );
	if( thid >= 0 ) sceKernelStartThread( thid, args, argp );
	
	DebugMsg( "exit main\n" );
	return 0;
}
