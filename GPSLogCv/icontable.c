/* icontablef.c - part of gpslogcv
 mailto:imaizumi@nisiq.net
 http://homepage1.nifty.com/gigo/

 navin'you 5.0
 Panasonic Car Navigation HS400

History:
  2005/05/06 initial release
*/
#include "gpslogcv.h"

#define NO_PMF_ICON 12 /* �Ԃ��s�� */

icon_table_t icontable[] = {
	{ 0x0000fe,"�ėp"				, 31 },	/* ���s�� */
	{ 0x0001ff,"�j�̎q"				,  4 },	/* �j�� */
	{ 0x0002ff,"���̎q"				,  5 },	/* ���� */
	{ 0x0103ff,"�Z��"				,  0 },	/* �Ԃ��� */
	{ 0x0003ff,"��"					,  1 },	/* ���� */
	{ 0x0004ff,"�C�̎v���o"			, 29 },	/* ���Q */
	{ 0x0005ff,"�R�̎v���o"			, 30 },	/* ���R */
	{ 0x0009ff,"�v���C�X�|�b�g"		,  7 },	/* �n�[�g */
	{ 0x0109ff,"����"				, 12 }, /**/
	{ 0x0209ff,"�V���n"				, 12 },	/**/
	{ 0x0309ff,"�L�����v��"			, 15 },	/* �L�����v */
	{ 0x0409ff,"����"				, 14 },	/* ���� */
	{ 0x0509ff,"�J���I�P�{�b�N�X"	, 12 }, /**/
	{ 0x0609ff,"�Q�[���Z���^�["		, 12 }, /**/
	{ 0x040409,"�X�|�[�c�X�|�b�g"	,  6 },	/* �T�b�J�[�{�[�� */
	{ 0x000aff,"�S���t��"			, 12 },	/**/
	{ 0x010aff,"�X�L�[��"			, 12 },	/**/
	{ 0x020aff,"�v�[��"				, 12 },	/**/
	{ 0x030aff,"�e�j�X�R�[�g"		, 20 },	/* �e�j�X */
	{ 0x070309,"�����Ԋ֘A�T�[�r�X"	, 19 },	/* ������ */
	{ 0x000bff,"�J�[�p�i�X"			, 12 },	/**/
	{ 0x010bff,"�J�[�f�B�[���["		, 12 }, /**/
	{ 0x020bff,"���ԏ�"				, 02 },	/* �p�[�L���O */
	{ 0x030bff,"�K�\�����X�^���h"	, 10 },	/* �K�\�����X�^���h */
	{ 0x0c0209,"�i���X"				, 23 },	/* �R�[�q�[ */
	{ 0x0d0209,"�o�[����"			, 12 },	/**/
	{ 0x0a0209,"�t�@�[�X�g�t�[�h"	, 12 }, /**/
	{ 0x0006ff,"���[�����X"			, 12 },	/**/
	{ 0x060209,"�O�����X�|�b�g"		, 22 },	/* ���X�g���� */
	{ 0x010209,"���{�����E�a�H"		, 12 },	/**/
	{ 0x070209,"���ؗ����X"			, 12 },	/**/
	{ 0x020209,"�C�^���A�����X"		, 12 },	/**/
	{ 0x030209,"�t�����X�����X"		, 12 },	/**/
	{ 0x080209,"�ē��E�؍�����"		, 12 },	/**/
	{ 0x0b0209,"�t�@�~���[���X�g����",12 }, /**/
	{ 0x0008ff,"�h���{��"			, 13 },	/* �h */
	{ 0x000cff,"���X"				, 12 },	/**/
	{ 0x010cff,"�R���r�j�G���X�X�g�A",17 },	/* �R���r�j */
	{ 0x000dff,"�a�@"				, 12 },	/**/
	{ 0x010dff,"��s"				, 25 },	/* ��s */
	{ 0x000eff,"�w�Z"				, 11 },	/* ���̗��������� */
	{ 0x010eff,"���"				,  8 },	/* �r�� */
	{ 0x0007ff,"MPS�t�@�C��"		, 12 },	/**/
	{ 0x0010ff,"Smart Capture"		,  9 },	/* �J���� */
	{ 0x000000,""					, 12 }	/* --- */
};

/* pmf -> nyp */
/* �Y�����Ȃ��̂�nyp�ėp�ɂ���B */
#if 0
03:�i���֎~
12:�Ԃ��s��
16:��
18:�т�����
21:�l�Y�~
24:��
26:�s�A�m
27:�_���x��
28:���P
#endif

int nypicon2pmf(long n)
{
	int i;

	for (i = 0;i < sizeof(icontable)/sizeof(icon_table_t); i++) {
		if (icontable[i].icon_type == n)
			return icontable[i].icon_type_pmf;
	}
	return NO_PMF_ICON;
}

int nypiconidx(int n)
{
	int i;

	for (i = 0;i < sizeof(icontable)/sizeof(icon_table_t); i++) {
		if (icontable[i].icon_type == n)
			return i;
	}
	return 0;
}