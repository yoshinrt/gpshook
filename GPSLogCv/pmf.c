/* pmf.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

 Panasonic Car Navigation

History:
  2005/07/07 fix no-time bug
  2005/05/06 initial release
*/
#include <string.h>
#include <locale.h>
#include <mbstring.h>
#include <ctype.h>
#include "gpslogcv.h"

#define PMF_SND 1
#define PMF_MSG 2
#define PMF_MUS 3

typedef struct {
	int snd;
	int typ;
	char *msg;
} pmf_snd_t;

/* サウンド 機種ごとに微妙に違う */
static pmf_snd_t snd_tbl[] = {
	{ 0x005a,PMF_MSG,"ご注意ください" },	/* 1 */
	{ 0x00c1,PMF_MSG,"工事中" },			/* 3 */
	{ 0x00dc,PMF_MSG,"路面注意" },			/* 14 */
	{ 0x00dd,PMF_MSG,"段差あり" },			/* 15 */
	{ 0x00de,PMF_MSG,"事故多し" },			/* 16 */
	{ 0x00bf,PMF_MSG,"危険です" },			/* 2 */
	{ 0x00db,PMF_MSG,"合流注意" },			/* 13 */
	{ 0x00e6,PMF_MSG,"急カーブ注意" },		/* 21 */
	{ 0x00c2,PMF_MSG,"渋滞します" },		/* 4 */
	{ 0x00e8,PMF_MSG,"スピード注意" },		/* 22 */
	{ 0x00e9,PMF_MSG,"車線注意" },			/* 23 */
	{ 0x00ea,PMF_MSG,"対向車注意" },		/* 24 */
	{ 0x00e4,PMF_MSG,"シートベルト注意" },	/* 19 */
	{ 0x00d4,PMF_MSG,"トイレあり" },		/* 6 */
	{ 0x00d6,PMF_MSG,"駐車場あり" },		/* 8 */
	{ 0x00d7,PMF_MSG,"ガソリンスタンドあり" },	 /* 9 */
	{ 0x00d8,PMF_MSG,"ラーメン屋あり" },	/* 10 */
	{ 0x00d9,PMF_MSG,"寿司屋あり" },		/* 11 */
	{ 0x00da,PMF_MSG,"おいしい店あり" },	/* 12 */
	{ 0x00d5,PMF_MSG,"温泉あり" },			/* 7 */
	{ 0x00df,PMF_MSG,"自宅あり" },			/* 17 */
	{ 0x00e0,PMF_MSG,"友達の家あり" },		/* 18 */
	{ 0x00e5,PMF_MSG,"夜景がきれい" },		/* 20 */
	{ 0x00ec,PMF_MSG,"ここだよ" },			/* 26 */
	{ 0x00eb,PMF_MSG,"登録地点接近" },		/* 25 */

	{ 0x0004,PMF_SND,"ベル音１" },			/* 0 */
	{ 0x0000,PMF_SND,"ベル音２" },			/* 0 */
	{ 0x01b5,PMF_SND,"警告音１" },			/* 27 */
	{ 0x01b6,PMF_SND,"警告音２" },			/* 28 */
	{ 0x01b7,PMF_SND,"警告音３" },			/* 29 */
	{ 0x01b7,PMF_SND,"アラーム１" },			
	{ 0x01b7,PMF_SND,"アラーム２" },			
	{ 0x01b7,PMF_SND,"アラーム３" },
	{ 0x01b7,PMF_SND,"アラーム４" },			
	{ 0x01b7,PMF_SND,"アラーム５" },
	{ 0x01b9,PMF_SND,"ゾウの鳴き声" },		/* 31 */
	{ 0x01bb,PMF_SND,"ネコの鳴き声" },		/* 33 */
	{ 0x01cc,PMF_SND,"イヌの鳴き声" },		/* 49 */
	{ 0x01bd,PMF_SND,"ウシの鳴き声" },		/* 35 */
	{ 0x01be,PMF_SND,"ウグイスの鳴き声" },	/* 36 */
	{ 0x01cd,PMF_SND,"にわとりの鳴き声" },	/* 50 */
	{ 0x01c9,PMF_SND,"ひよこの鳴き声" },	/* 46 */
	{ 0x01c1,PMF_SND,"セミの鳴き声" },		/* 39 */
	{ 0x01d1,PMF_SND,"馬の走る音" },		/* 54 */
	{ 0x01ce,PMF_SND,"笑い声" },			/* 51 */
	{ 0x01d0,PMF_SND,"拍手喝采" },			/* 53 */
	{ 0x01cf,PMF_SND,"一本締め" },			/* 52 */
	{ 0x01b8,PMF_SND,"お寺の鐘" },			/* 30 */
	{ 0x01d2,PMF_SND,"小川のせせらぎ" },	/* 55 */
	{ 0x01bf,PMF_SND,"水に飛び込む音" },	/* 37 */
	{ 0x01c0,PMF_SND,"時代劇" },			/* 38 */
	{ 0x01c3,PMF_SND,"風鈴" },				/* 41 */
	{ 0x01d4,PMF_SND,"お化け屋敷" },		/* 57 */
	{ 0x01ca,PMF_SND,"獅子オドシ" },		/* 47 */
	{ 0x01ca,PMF_SND,"くしゃみ" },
	{ 0x01ba,PMF_SND,"汽笛の音" },			/* 32 */
	{ 0x01ba,PMF_SND,"ハープの音" },		
	{ 0x01ba,PMF_SND,"琴の音" },			
	{ 0x01ba,PMF_SND,"電話のベル" },		
	{ 0x01ba,PMF_SND,"Ｆ１の音" },			

	{ 0x01d5,PMF_MUS,"チャイム" },			/* 58 */
	{ 0x01c5,PMF_MUS,"ファンファーレ１" },	/* 42 */
	{ 0x01c6,PMF_MUS,"ファンファーレ２" },	/* 43 */
	{ 0x01c7,PMF_MUS,"オルゴール１" },		/* 44 */
	{ 0x01c8,PMF_MUS,"オルゴール２" },		/* 45 */
	{ 0x01c2,PMF_MUS,"ピアノ１" },			/* 40 */
	{ 0x01cb,PMF_MUS,"ピアノ２" },			/* 48 */
	{ 0x01d3,PMF_MUS,"ピアノ３" },			/* 56 */
	{ 0x01d3,PMF_MUS,"ピアノ４" },			
	{ 0x01c5,PMF_MUS,"エレクトーン１" },	
	{ 0x01c6,PMF_MUS,"エレクトーン２" },	
	{ 0x00c4,PMF_MUS,"マンボ" },			/* 5 */
	{ 0x01c5,PMF_MUS,"沖縄民謡" },	
	{ 0x01c6,PMF_MUS,"コーラス" },	
	{ 0xffff,PMF_MUS,"" },	

#if 0	/* HS400にはない */
	{ 0x0215,PMF_MSG,"右折レーン" },		/* 59 */
	{ 0x0217,PMF_MSG,"左折レーン" },		/* 60 */
	{ 0x021d,PMF_MSG,"目印" },				/* 61 */
	{ 0x0229,PMF_MSG,"ガソリン補給" },		/* 62 */
	{ 0x023d,PMF_MSG,"業務終了" }			/* 63 */
#endif
};

static int sum;

static char *ang2pmfDMS(double angle, char *buf)
{
    int deg,min,sec,sec10;
    unsigned long IAng;

    IAng = (unsigned long)((angle+0.5/36000.0) * ONEDEG);
    deg = (int)(IAng/ONEDEG);
    IAng = (IAng%ONEDEG)*60;
    min = (int)(IAng/ONEDEG);
    IAng = (IAng%ONEDEG)*60;
    sec = (int)(IAng/ONEDEG);
    IAng = (IAng%ONEDEG)/(ONEDEG/1000UL);
	sec10 = (int)IAng/100;
    sprintf(buf,"%03d%02d%02d%d",deg,min,sec,sec10);
	sum += deg+min+sec+sec10;
    return buf;
}

static char pmfbuf[512];
static char latbuf[16];
static char lonbuf[16];
static char timbuf[16];


//-inypu -tpmf -of allicon.nyp 

void out_PMF(FILE *out, cordinate_t *position, cnvData_t *GPSLogp)
{
	char *p;
	unsigned	sndTyp = 0xffff;
	int			sndOn = 0;
	int			sndDistance = 0;
	char		*sndName = "";
	int			sndDirOn = 0;
	int			sndDirection = 0;
	int			pmfIcon = nypicon2pmf(icon);

	int c = 20;

	sum = 0;

    if (nRec == 0) {
		fprintf(out,"登録ポイント\n");
	}
	for (p = titleBuf;*p;p++) {
		if (_ismbclegal(*p *256 + *(p+1))) {
			--c; ++p;
		}
	}
	if ((strstr(titleBuf,"Hシステム")!=NULL)
		|| (strstr(titleBuf,"Ｈシステム")!=NULL)
		|| (strstr(titleBuf,"レーダ")!=NULL)
		|| (strstr(titleBuf,"オービス")!=NULL)
		|| (strstr(titleBuf,"光電式")!=NULL)
		|| (strstr(titleBuf,"ループコイル")!=NULL)
		|| (strstr(titleBuf,"ねずみ取り")!=NULL)
		) {
		sndTyp = 0x00e8;
		sndOn = 1;
		if ((titleBuf[0] == 'R')||(strstr(titleBuf,"県")==titleBuf))
			sndDistance = 300;
		else if ((strstr(titleBuf,"首")==titleBuf)||(strstr(titleBuf,"環")==titleBuf)||(strstr(titleBuf,"阪")==titleBuf))
			sndDistance = 400;
		else
			sndDistance = 500;
		sndName = "スピード注意";
		if (GPSLogp->datTyp & DAT_HED) {
			sndDirection = (((int)(GPSLogp->head+202.5))%360)/45;
			sndDirOn = 1;
		}
		if (strstr(titleBuf,"ねずみ取り")!=NULL)
			pmfIcon = 21;	/* ねずみ */
		else
			pmfIcon = 9;	/* カメラ */
	}

    if (GPSLogp->datTyp & DAT_TIM) {
		sprintf(timbuf,"%04d%02d%02d%02d%02d%02d"
		,GPSLogp->recTime->tm_year+1900,GPSLogp->recTime->tm_mon+1,GPSLogp->recTime->tm_mday
    	,GPSLogp->recTime->tm_hour,GPSLogp->recTime->tm_min,GPSLogp->recTime->tm_sec);
	} else {
		timbuf[0] = '\0';
	}
	sprintf(pmfbuf,"#%02d\t#%02d\t#%02d\t#%s\t#%s\t#%04x\t#%04x\t#%04x\t#%04x\t#%04X\t#%02d\t#%02X\t#%-20s\t#%02d\t#%02d\t#%04d\t#%s\t#%s\t#%s\t#%s\t#%s\t#%s\t#%02d\t#%-13s\t"
		,pmfIcon	/* ・マーク */
		,0	/* ジャンル選択？ */
		,0	/* ジャンル選択？ */
		,ang2pmfDMS(position->lon,lonbuf)	/* 東経 13545326 */
		,ang2pmfDMS(position->lat,latbuf)	/* 北緯 03441188 */
		,0	/* ジャンルコード？ */
		,0	/* ジャンル内コード？ */
		,0	/* 不明 */
		,0	/* 不明 */
		,sndTyp	/* ・サウンドの種類 */
		,sndOn	/* ・サウンドの有無 */
		,c //strlen(titleBuf)	/* ・名称の文字数(0x20-漢字の文字数?? */
		,titleBuf	/* ・名称 */
		,sndDirOn	/* ・方位指定有無 */
		,sndDirection	/* ・サウンド方向 315-405, 0-90, 45-135, 90-180, 135-225, 180-270, 225-315, 270-360 */
		,sndDistance	/* ・サウンド距離 50, 100, 200, 300, 400, 500 */
		,sndName	/* ・サウンド名称 */
		,addrBuf	/* ・登録地点住所 */
		,""	/* ・メモ */
		,""	/* ・メモ2行目 */
		,""	/* 読みあげる */
    	,timbuf	/* 登録日時 20050501191415 */
		,1	/* 地点名称飾り */
		,telBuf/* ・電話番号 */
	);
	fprintf(out,"%s",pmfbuf);
#if 1
	sum = ~sum;	/* これで出力した目印がわりにわざと反転させて狂わせる */
#endif
	fprintf(out,"#%02X\t",sum&0xff);	/* チェックサム */
	fprintf(out,"#%04X\t",0xffff);	/* ?? */
	fprintf(out,"#%02d",0);	/* ?? */
	fprintf(out,"\n");
    nRec++;
}

/* マーク 機種ごとに微妙に違う */
/* 00:赤い家 */
/* 01:青い家 */
/* 02:パーキング */
/* 03:進入禁止 */
/* 04:男性 */
/* 05:女性 */
/* 06:サッカーボール */
/* 07:ハート */
/* 08:ビル */
/* 09:カメラ */
/* 10:GS */
/* 11: */
/* 12:赤いピン */
/* 13:宿 */
/* 14:温泉 */
/* 15:キャンプ */
/* 16:魚 */
/* 17:コンビニ */
/* 18:びっくり */
/* 19:自動車 */
/* 20:テニス */
/* 21:ネズミ */
/* 22:レストラン */
/* 23:コーヒー */
/* 24:樹 */
/* 25:銀行 */
/* 26:ピアノ */
/* 27:ダンベル */
/* 28:☆１ */
/* 29:☆２ */
/* 30:☆３ */
/* 31:青いピン */

static char *strTABcpy(char *dst, char *src)
{
	char *p = dst;
	unsigned char *q = (unsigned char *)src;

	while(*q > '\t')
		*p++ = *q++;
	*p = '\0';
	return dst;
}

//-of allicon.pmf 

int in_PMF(FILE *rFile, cordinate_t *position, cnvData_t *GPSLogp)
/* return value -1:Error, 0:EOF, 1:OK */
{
	char *p;
	double d;
	long l;
	int n;

	titleBuf[0] = '\0';
	addrBuf[0] = '\0';
	telBuf[0] = '\0';
	GPSLogp->datTyp &= ~(DAT_TIM|DAT_JST);

	while((p=xfgets(rFile))!=NULL) {
		n = 0;
		while(*p) {
			if (*p++ == '#') {
				++n;
				switch(n) {
				case  1:
					icon = atoi(p);
					break;
				case  2:	/* ジャンル選択？ */
					break;
				case  3:	/* ジャンル選択？ */
					break;
				case  4:	/* 東経 13545326 */
					l=atol(p);
					d = (l%1000);l/=1000;
					d = d/600 + (l%100);l/=100;
					position->lon = d/60 + l;
					break;
				case  5:	/* 北緯 03441188 */
					l=atol(p);
					d = (l%1000);l/=1000;
					d = d/600 + (l%100);l/=100;
					position->lat = d/60 + l;
					break;
				case  6:	/* ジャンルコード？ */
					break;
				case  7:	/* ジャンル内コード？ */
					break;
				case  8:	/* 不明 */
					break;
				case  9:	/* 不明 */
					break;
				case 10:	/* ・サウンドの種類 0xffff */
					break;
				case 11:	/* ・サウンドの有無 */
					break;
				case 12:	/* ・名称の文字数(0x20-漢字の文字数?? */
					break;
				case 13:	/* ・名称 */
					strTABcpy(titleBuf,p);
					break;
				case 14:	/* ・方位指定有無 */
					break;
				case 15:	/* ・サウンド方向 315-45, 0-90, 45-135, 90-180, 135-225, 180-270, 225-315, 270-360 */
					break;
				case 16:	/* ・サウンド距離 50, 100, 200, 300, 400, 500 */
					break;
				case 17:	/* ・サウンド名称 */
					break;
				case 18:	/* ・登録地点住所 */
					strTABcpy(addrBuf,p);
					break;
				case 19:	/* ・メモ */
					break;
				case 20:	/* ・メモ2行目 */
					break;
				case 21:	/* 読みあげる */
					break;
				case 22:	/* 登録日時 20050501191415 */
					if (strlen(p) < 14)
						break;
					GPSLogp->recTime->tm_year = (*p++ -'0')*1000;
					GPSLogp->recTime->tm_year += (*p++ -'0')*100;
					GPSLogp->recTime->tm_year += (*p++ -'0')*10;
					GPSLogp->recTime->tm_year += (*p++ -'0');
					GPSLogp->recTime->tm_year -= 1900;
					GPSLogp->recTime->tm_mon = (*p++ -'0')*10;
					GPSLogp->recTime->tm_mon += (*p++ -'0');
					GPSLogp->recTime->tm_mon--;
					GPSLogp->recTime->tm_mday = (*p++ -'0')*10;
					GPSLogp->recTime->tm_mday += (*p++ -'0');
					
					GPSLogp->recTime->tm_hour = (*p++ -'0')*10;
					GPSLogp->recTime->tm_hour += (*p++ -'0');
					GPSLogp->recTime->tm_min = (*p++ -'0')*10;
					GPSLogp->recTime->tm_min += (*p++ -'0');
					GPSLogp->recTime->tm_sec = (*p++ -'0')*10;
					GPSLogp->recTime->tm_sec += (*p++ -'0');
					if (mktime(GPSLogp->recTime) != -1)
	                    GPSLogp->datTyp |= DAT_TIM|DAT_JST;
					break;
				case 23:	/* 地点名称飾り */
					break;
				case 24:
					if (atoi(p))
						strTABcpy(telBuf,p);	/* ・電話番号 */
					break;
				default:
					break;
				}
			}
		}
		if (n)
			return 1;
	}
	return 0;	/* return EOF */
}
