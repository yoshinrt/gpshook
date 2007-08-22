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

/* �T�E���h �@�킲�Ƃɔ����ɈႤ */
static pmf_snd_t snd_tbl[] = {
	{ 0x005a,PMF_MSG,"�����ӂ�������" },	/* 1 */
	{ 0x00c1,PMF_MSG,"�H����" },			/* 3 */
	{ 0x00dc,PMF_MSG,"�H�ʒ���" },			/* 14 */
	{ 0x00dd,PMF_MSG,"�i������" },			/* 15 */
	{ 0x00de,PMF_MSG,"���̑���" },			/* 16 */
	{ 0x00bf,PMF_MSG,"�댯�ł�" },			/* 2 */
	{ 0x00db,PMF_MSG,"��������" },			/* 13 */
	{ 0x00e6,PMF_MSG,"�}�J�[�u����" },		/* 21 */
	{ 0x00c2,PMF_MSG,"�a�؂��܂�" },		/* 4 */
	{ 0x00e8,PMF_MSG,"�X�s�[�h����" },		/* 22 */
	{ 0x00e9,PMF_MSG,"�Ԑ�����" },			/* 23 */
	{ 0x00ea,PMF_MSG,"�Ό��Ԓ���" },		/* 24 */
	{ 0x00e4,PMF_MSG,"�V�[�g�x���g����" },	/* 19 */
	{ 0x00d4,PMF_MSG,"�g�C������" },		/* 6 */
	{ 0x00d6,PMF_MSG,"���ԏꂠ��" },		/* 8 */
	{ 0x00d7,PMF_MSG,"�K�\�����X�^���h����" },	 /* 9 */
	{ 0x00d8,PMF_MSG,"���[����������" },	/* 10 */
	{ 0x00d9,PMF_MSG,"���i������" },		/* 11 */
	{ 0x00da,PMF_MSG,"���������X����" },	/* 12 */
	{ 0x00d5,PMF_MSG,"���򂠂�" },			/* 7 */
	{ 0x00df,PMF_MSG,"�����" },			/* 17 */
	{ 0x00e0,PMF_MSG,"�F�B�̉Ƃ���" },		/* 18 */
	{ 0x00e5,PMF_MSG,"��i�����ꂢ" },		/* 20 */
	{ 0x00ec,PMF_MSG,"��������" },			/* 26 */
	{ 0x00eb,PMF_MSG,"�o�^�n�_�ڋ�" },		/* 25 */

	{ 0x0004,PMF_SND,"�x�����P" },			/* 0 */
	{ 0x0000,PMF_SND,"�x�����Q" },			/* 0 */
	{ 0x01b5,PMF_SND,"�x�����P" },			/* 27 */
	{ 0x01b6,PMF_SND,"�x�����Q" },			/* 28 */
	{ 0x01b7,PMF_SND,"�x�����R" },			/* 29 */
	{ 0x01b7,PMF_SND,"�A���[���P" },			
	{ 0x01b7,PMF_SND,"�A���[���Q" },			
	{ 0x01b7,PMF_SND,"�A���[���R" },
	{ 0x01b7,PMF_SND,"�A���[���S" },			
	{ 0x01b7,PMF_SND,"�A���[���T" },
	{ 0x01b9,PMF_SND,"�]�E�̖���" },		/* 31 */
	{ 0x01bb,PMF_SND,"�l�R�̖���" },		/* 33 */
	{ 0x01cc,PMF_SND,"�C�k�̖���" },		/* 49 */
	{ 0x01bd,PMF_SND,"�E�V�̖���" },		/* 35 */
	{ 0x01be,PMF_SND,"�E�O�C�X�̖���" },	/* 36 */
	{ 0x01cd,PMF_SND,"�ɂ�Ƃ�̖���" },	/* 50 */
	{ 0x01c9,PMF_SND,"�Ђ悱�̖���" },	/* 46 */
	{ 0x01c1,PMF_SND,"�Z�~�̖���" },		/* 39 */
	{ 0x01d1,PMF_SND,"�n�̑��鉹" },		/* 54 */
	{ 0x01ce,PMF_SND,"�΂���" },			/* 51 */
	{ 0x01d0,PMF_SND,"���芅��" },			/* 53 */
	{ 0x01cf,PMF_SND,"��{����" },			/* 52 */
	{ 0x01b8,PMF_SND,"�����̏�" },			/* 30 */
	{ 0x01d2,PMF_SND,"����̂����炬" },	/* 55 */
	{ 0x01bf,PMF_SND,"���ɔ�э��މ�" },	/* 37 */
	{ 0x01c0,PMF_SND,"���㌀" },			/* 38 */
	{ 0x01c3,PMF_SND,"����" },				/* 41 */
	{ 0x01d4,PMF_SND,"���������~" },		/* 57 */
	{ 0x01ca,PMF_SND,"���q�I�h�V" },		/* 47 */
	{ 0x01ca,PMF_SND,"�������" },
	{ 0x01ba,PMF_SND,"�D�J�̉�" },			/* 32 */
	{ 0x01ba,PMF_SND,"�n�[�v�̉�" },		
	{ 0x01ba,PMF_SND,"�Ղ̉�" },			
	{ 0x01ba,PMF_SND,"�d�b�̃x��" },		
	{ 0x01ba,PMF_SND,"�e�P�̉�" },			

	{ 0x01d5,PMF_MUS,"�`���C��" },			/* 58 */
	{ 0x01c5,PMF_MUS,"�t�@���t�@�[���P" },	/* 42 */
	{ 0x01c6,PMF_MUS,"�t�@���t�@�[���Q" },	/* 43 */
	{ 0x01c7,PMF_MUS,"�I���S�[���P" },		/* 44 */
	{ 0x01c8,PMF_MUS,"�I���S�[���Q" },		/* 45 */
	{ 0x01c2,PMF_MUS,"�s�A�m�P" },			/* 40 */
	{ 0x01cb,PMF_MUS,"�s�A�m�Q" },			/* 48 */
	{ 0x01d3,PMF_MUS,"�s�A�m�R" },			/* 56 */
	{ 0x01d3,PMF_MUS,"�s�A�m�S" },			
	{ 0x01c5,PMF_MUS,"�G���N�g�[���P" },	
	{ 0x01c6,PMF_MUS,"�G���N�g�[���Q" },	
	{ 0x00c4,PMF_MUS,"�}���{" },			/* 5 */
	{ 0x01c5,PMF_MUS,"���ꖯ�w" },	
	{ 0x01c6,PMF_MUS,"�R�[���X" },	
	{ 0xffff,PMF_MUS,"" },	

#if 0	/* HS400�ɂ͂Ȃ� */
	{ 0x0215,PMF_MSG,"�E�܃��[��" },		/* 59 */
	{ 0x0217,PMF_MSG,"���܃��[��" },		/* 60 */
	{ 0x021d,PMF_MSG,"�ڈ�" },				/* 61 */
	{ 0x0229,PMF_MSG,"�K�\�����⋋" },		/* 62 */
	{ 0x023d,PMF_MSG,"�Ɩ��I��" }			/* 63 */
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
		fprintf(out,"�o�^�|�C���g\n");
	}
	for (p = titleBuf;*p;p++) {
		if (_ismbclegal(*p *256 + *(p+1))) {
			--c; ++p;
		}
	}
	if ((strstr(titleBuf,"H�V�X�e��")!=NULL)
		|| (strstr(titleBuf,"�g�V�X�e��")!=NULL)
		|| (strstr(titleBuf,"���[�_")!=NULL)
		|| (strstr(titleBuf,"�I�[�r�X")!=NULL)
		|| (strstr(titleBuf,"���d��")!=NULL)
		|| (strstr(titleBuf,"���[�v�R�C��")!=NULL)
		|| (strstr(titleBuf,"�˂��ݎ��")!=NULL)
		) {
		sndTyp = 0x00e8;
		sndOn = 1;
		if ((titleBuf[0] == 'R')||(strstr(titleBuf,"��")==titleBuf))
			sndDistance = 300;
		else if ((strstr(titleBuf,"��")==titleBuf)||(strstr(titleBuf,"��")==titleBuf)||(strstr(titleBuf,"��")==titleBuf))
			sndDistance = 400;
		else
			sndDistance = 500;
		sndName = "�X�s�[�h����";
		if (GPSLogp->datTyp & DAT_HED) {
			sndDirection = (((int)(GPSLogp->head+202.5))%360)/45;
			sndDirOn = 1;
		}
		if (strstr(titleBuf,"�˂��ݎ��")!=NULL)
			pmfIcon = 21;	/* �˂��� */
		else
			pmfIcon = 9;	/* �J���� */
	}

    if (GPSLogp->datTyp & DAT_TIM) {
		sprintf(timbuf,"%04d%02d%02d%02d%02d%02d"
		,GPSLogp->recTime->tm_year+1900,GPSLogp->recTime->tm_mon+1,GPSLogp->recTime->tm_mday
    	,GPSLogp->recTime->tm_hour,GPSLogp->recTime->tm_min,GPSLogp->recTime->tm_sec);
	} else {
		timbuf[0] = '\0';
	}
	sprintf(pmfbuf,"#%02d\t#%02d\t#%02d\t#%s\t#%s\t#%04x\t#%04x\t#%04x\t#%04x\t#%04X\t#%02d\t#%02X\t#%-20s\t#%02d\t#%02d\t#%04d\t#%s\t#%s\t#%s\t#%s\t#%s\t#%s\t#%02d\t#%-13s\t"
		,pmfIcon	/* �E�}�[�N */
		,0	/* �W�������I���H */
		,0	/* �W�������I���H */
		,ang2pmfDMS(position->lon,lonbuf)	/* ���o 13545326 */
		,ang2pmfDMS(position->lat,latbuf)	/* �k�� 03441188 */
		,0	/* �W�������R�[�h�H */
		,0	/* �W���������R�[�h�H */
		,0	/* �s�� */
		,0	/* �s�� */
		,sndTyp	/* �E�T�E���h�̎�� */
		,sndOn	/* �E�T�E���h�̗L�� */
		,c //strlen(titleBuf)	/* �E���̂̕�����(0x20-�����̕�����?? */
		,titleBuf	/* �E���� */
		,sndDirOn	/* �E���ʎw��L�� */
		,sndDirection	/* �E�T�E���h���� 315-405, 0-90, 45-135, 90-180, 135-225, 180-270, 225-315, 270-360 */
		,sndDistance	/* �E�T�E���h���� 50, 100, 200, 300, 400, 500 */
		,sndName	/* �E�T�E���h���� */
		,addrBuf	/* �E�o�^�n�_�Z�� */
		,""	/* �E���� */
		,""	/* �E����2�s�� */
		,""	/* �ǂ݂����� */
    	,timbuf	/* �o�^���� 20050501191415 */
		,1	/* �n�_���̏��� */
		,telBuf/* �E�d�b�ԍ� */
	);
	fprintf(out,"%s",pmfbuf);
#if 1
	sum = ~sum;	/* ����ŏo�͂����ڈ󂪂��ɂ킴�Ɣ��]�����ċ��킹�� */
#endif
	fprintf(out,"#%02X\t",sum&0xff);	/* �`�F�b�N�T�� */
	fprintf(out,"#%04X\t",0xffff);	/* ?? */
	fprintf(out,"#%02d",0);	/* ?? */
	fprintf(out,"\n");
    nRec++;
}

/* �}�[�N �@�킲�Ƃɔ����ɈႤ */
/* 00:�Ԃ��� */
/* 01:���� */
/* 02:�p�[�L���O */
/* 03:�i���֎~ */
/* 04:�j�� */
/* 05:���� */
/* 06:�T�b�J�[�{�[�� */
/* 07:�n�[�g */
/* 08:�r�� */
/* 09:�J���� */
/* 10:GS */
/* 11: */
/* 12:�Ԃ��s�� */
/* 13:�h */
/* 14:���� */
/* 15:�L�����v */
/* 16:�� */
/* 17:�R���r�j */
/* 18:�т����� */
/* 19:������ */
/* 20:�e�j�X */
/* 21:�l�Y�~ */
/* 22:���X�g���� */
/* 23:�R�[�q�[ */
/* 24:�� */
/* 25:��s */
/* 26:�s�A�m */
/* 27:�_���x�� */
/* 28:���P */
/* 29:���Q */
/* 30:���R */
/* 31:���s�� */

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
				case  2:	/* �W�������I���H */
					break;
				case  3:	/* �W�������I���H */
					break;
				case  4:	/* ���o 13545326 */
					l=atol(p);
					d = (l%1000);l/=1000;
					d = d/600 + (l%100);l/=100;
					position->lon = d/60 + l;
					break;
				case  5:	/* �k�� 03441188 */
					l=atol(p);
					d = (l%1000);l/=1000;
					d = d/600 + (l%100);l/=100;
					position->lat = d/60 + l;
					break;
				case  6:	/* �W�������R�[�h�H */
					break;
				case  7:	/* �W���������R�[�h�H */
					break;
				case  8:	/* �s�� */
					break;
				case  9:	/* �s�� */
					break;
				case 10:	/* �E�T�E���h�̎�� 0xffff */
					break;
				case 11:	/* �E�T�E���h�̗L�� */
					break;
				case 12:	/* �E���̂̕�����(0x20-�����̕�����?? */
					break;
				case 13:	/* �E���� */
					strTABcpy(titleBuf,p);
					break;
				case 14:	/* �E���ʎw��L�� */
					break;
				case 15:	/* �E�T�E���h���� 315-45, 0-90, 45-135, 90-180, 135-225, 180-270, 225-315, 270-360 */
					break;
				case 16:	/* �E�T�E���h���� 50, 100, 200, 300, 400, 500 */
					break;
				case 17:	/* �E�T�E���h���� */
					break;
				case 18:	/* �E�o�^�n�_�Z�� */
					strTABcpy(addrBuf,p);
					break;
				case 19:	/* �E���� */
					break;
				case 20:	/* �E����2�s�� */
					break;
				case 21:	/* �ǂ݂����� */
					break;
				case 22:	/* �o�^���� 20050501191415 */
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
				case 23:	/* �n�_���̏��� */
					break;
				case 24:
					if (atoi(p))
						strTABcpy(telBuf,p);	/* �E�d�b�ԍ� */
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
