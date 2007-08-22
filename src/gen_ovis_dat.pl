#!/usr/bin/perl -w

#ovis*.pot から ovis_data.h を生成するツール

open( fpIn,  "<$ARGV[0]" );
open( fpOut, ">$ARGV[1]" );

printf( fpOut "OVIS_DATA_POS_t g_OvisData_Pos[] = {\n" );

$DirP = 0;

while( <fpIn> ){
	
	next if( /^;/ );
	
	if( m|/kei=(\d+)'(\d+)'(\d+)'(\d+)/ido=(\d+)'(\d+)'(\d+)'(\d+)/deg=(\d+)| ){
		$Lat  = $5 + ( $6 + ( $7 + $8 / 10.0 ) / 60.0 ) / 60.0;
		$Long = $1 + ( $2 + ( $3 + $4 / 10.0 ) / 60.0 ) / 60.0;
		
		$Dir  = 0xF00FF00F << int( $9 * 16 / 360 );
		$Dir = ( $Dir | ( $Dir >> 16 )) & 0xFFFF;
		
		# TOKYO->WGS84 変換
		$Lat2  = $Lat  - 0.00010695  * $Lat + 0.000017464 * $Long + 0.0046017;
		$Long2 = $Long - 0.000046038 * $Lat - 0.000083043 * $Long + 0.010040;
		
		if( defined( $LatP )){
			if( $Lat2 == $LatP && $Long2 == $LongP ){
				# 一致したので，ovis の方向 bitmap の OR を取る
				$Dir |= $DirP;
			}else{
				# 一致しないので，直前のヤツを print
				&PrintOvis();
			}
		}
		$LatP  = $Lat2;
		$LongP = $Long2;
		$DirP  = $Dir;
	}
}

&PrintOvis();

print( fpOut <<"EOF" );
};

static const USHORT g_OvisData_uDir[] = {
$Dirs
};

static const UINT g_uOvisCnt = $Cnt;

static UCHAR g_OvisData_uCnt[ $Cnt ] SEC_BSS_BYTE;
EOF

sub PrintOvis {
	printf( fpOut "\t{ %.15lf, %.15lf },\n", $LatP, $LongP );
	$Dirs .= sprintf( "\t0x%04X,\n", $DirP );
	++$Cnt;
}
