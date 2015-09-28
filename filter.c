/*********************************************************************
* 	�X�y�N�g�����A�i���C�U�[ �v���O�C��  for AviUtl
* 
* 2003
* 	07/05:	����J�n
* 	07/12:	�Ƃ肠�����������̂��ł���
* 
*********************************************************************/
#include <windows.h>
#include <math.h>
#include "filter.h"
#include "fft.h"
#include "resource.h"

//----------------------------
//	�v���g�^�C�v
//----------------------------
static void wm_filter_init(HWND hwnd,FILTER *fp);
static void wm_filter_exit(FILTER *fp);
static void wm_paint(HWND hwnd);
static void disp_mono(HWND hwnd,int rate,int channel);
static void disp_stereo(HWND,int,int);
static void disp_left(HWND,int,int);
static void disp_right(HWND,int,int);
static void show_menu(FILTER *fp,LPARAM lparam);
static void set_color(FILTER *fp,COLORREF *rgb);


//----------------------------
//	�O���[�o���ϐ�
//----------------------------
HDC     memdc;	// ���z�f�o�C�X�R���e�L�X�g
HBITMAP hbit;	// ���z�r�b�g�}�b�v�n���h��
HDC     bgdc;
HBITMAP bgbit;
HBRUSH  hbrush;



#define  MONORAL     0
#define  STEREO      1
#define  LEFT        2
#define  RIGHT       3

#define KEY_FFT_BLOCK    "FFTBlock"
#define DEF_FFT_BLOCK    1024
#define KEY_DISPMODE     "Mode"
#define DEF_DISPMODE     MONORAL
#define KEY_COLOR_MONO   "ColorMONO"
#define KEY_COLOR_LEFT   "ColorLEFT"
#define KEY_COLOR_RIGHT  "ColorRIGHT"
#define DEF_COLOR_MONO   RGB(255,255,255)
#define DEF_COLOR_LEFT   RGB(255,255,128)
#define DEF_COLOR_RIGHT  RGB(128,255,255)

#define NMAX       4096
#define NMAXSQRT   64


// FFT
double  f[2][NMAX];
int     fft_n = DEF_FFT_BLOCK;
int     ip[NMAXSQRT + 2] = {0};
double  w[NMAX * 5/4];


// �ݒ�
int      mode      = DEF_DISPMODE;
COLORREF rgb_mono  = DEF_COLOR_MONO;
COLORREF rgb_left  = DEF_COLOR_LEFT;
COLORREF rgb_right = DEF_COLOR_RIGHT;

void (*disp[4])(HWND,int,int) = { disp_mono,disp_stereo,disp_left,disp_right };


#define GRAFX 256
#define GRAFY 160
#define _X  10
#define _Y  10
#define X_  20
#define Y_  40
#define CFGWND_X  GRAFX+_X+X_
#define CFGWND_Y  GRAFY+_Y+Y_

//----------------------------
//	FILTER_DLL�\����
//----------------------------
char filter_name[] = "�X�y�N�g���� �A�i���C�U�[";
char filter_info[] = "�X�y�N�g�����A�i���C�U�[ ver0.01 by MakKi";
#define track_N 0
#if track_N
TCHAR *track_name[]   = { "" };	// �g���b�N�o�[�̖��O
int   track_default[] = { 0 };	// �g���b�N�o�[�̏����l
int   track_s[]       = { 0 };	// �g���b�N�o�[�̉����l
int   track_e[]       = { 0 };	// �g���b�N�o�[�̏���l
#endif
#define check_N 0
#if check_N
TCHAR *check_name[]   = { 0 };	// �`�F�b�N�{�b�N�X
int   check_default[] = { 0 };	// �f�t�H���g
#endif


FILTER_DLL filter = {
	FILTER_FLAG_ALWAYS_ACTIVE |
	FILTER_FLAG_DISP_FILTER	|
	FILTER_FLAG_PRIORITY_LOWEST |
	FILTER_FLAG_REDRAW |
	FILTER_FLAG_WINDOW_SIZE |
	FILTER_FLAG_AUDIO_FILTER |
	FILTER_FLAG_EX_INFORMATION,
	CFGWND_X,CFGWND_Y,	// �ݒ�E�C���h�E�̃T�C�Y
	filter_name,		// �t�B���^�̖��O
	track_N,        	// �g���b�N�o�[�̐�
#if track_N
	track_name,     	// �g���b�N�o�[�̖��O�S
	track_default,  	// �g���b�N�o�[�̏����l�S
	track_s,track_e,	// �g���b�N�o�[�̐��l�̉������
#else
	NULL,NULL,NULL,NULL,
#endif
	check_N,      	// �`�F�b�N�{�b�N�X�̐�
#if check_N
	check_name,   	// �`�F�b�N�{�b�N�X�̖��O�S
	check_default,	// �`�F�b�N�{�b�N�X�̏����l�S
#else
	NULL,NULL,
#endif
	func_proc,   	// �t�B���^�����֐�
	NULL,NULL,   	// �J�n��,�I�����ɌĂ΂��֐�
	NULL,        	// �ݒ肪�ύX���ꂽ�Ƃ��ɌĂ΂��֐�
	func_WndProc,	// �ݒ�E�B���h�E�v���V�[�W��
	NULL,NULL,   	// �V�X�e���Ŏg�p
	NULL,NULL,     	// �g���f�[�^�̈�
	filter_info,	// �t�B���^���
	NULL,			// �Z�[�u�J�n���O�ɌĂ΂��֐�
	NULL,			// �Z�[�u�I�����ɌĂ΂��֐�
	NULL,NULL,NULL,	// �V�X�e���Ŏg�p
	NULL,			// �g���̈揉���l
};

/*********************************************************************
*	DLL Export
*********************************************************************/
EXTERN_C FILTER_DLL __declspec(dllexport) * __stdcall GetFilterTable( void )
{
	return &filter;
}

/*====================================================================
*	�t�B���^�����֐�
*===================================================================*/
BOOL func_proc( FILTER *fp,FILTER_PROC_INFO *fpip )
{
	FILE_INFO fi;
	int ch,n,i;
	int s;
	int sample;
	short *audiop = NULL;

	if(!fp->exfunc->is_filter_window_disp(fp))
		return FALSE;
	if(!fp->exfunc->is_editing(fpip->editp))
		return FALSE;


	if(!fp->exfunc->get_file_info(fpip->editp,&fi))
			return FALSE;	// FILE_INFO�擾���s

	if(!(fi.flag & FILE_INFO_FLAG_AUDIO))
		return FALSE;	// �����Ȃ�


	// �I�[�f�B�I�f�[�^�擾�iftt_n�T���v���j
	audiop = (short *)malloc(fi.audio_rate * fi.audio_ch * sizeof(short));
	for(n=0,sample=0;sample<fft_n;n++){
		s = fp->exfunc->get_audio_filtered(fpip->editp,fpip->frame+n,audiop);

		for(i=0;i<s&&sample+i<fft_n;i++){
			for(ch=0;ch<fi.audio_ch;ch++){
				f[ch][sample+i] = audiop[fi.audio_ch*i+ch];
			}
		}

		sample += s;

		if(fpip->frame+n>=fpip->frame_n){	// �ŏI�t���[��
			for(;sample<fft_n;sample++){
				for(ch=0;ch<fi.audio_ch;ch++)
					f[ch][sample] = 0;
			}
			break;
		}
	}
	free(audiop);

	// �t�[���G�ϊ�
	for(ch=0;ch<fi.audio_ch&&ch<2;ch++)
		rdft(fft_n,1,f[ch],ip,w);

	// �\��
	disp[mode](fp->hwnd,fi.audio_rate,ch);

	return TRUE;
}

/*====================================================================
*	�ݒ�E�B���h�E�v���V�[�W��
*===================================================================*/
BOOL func_WndProc( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void *editp, FILTER *fp )
{

	switch(message){
		case WM_FILTER_INIT:
			wm_filter_init(hwnd,fp);
			break;

		case WM_FILTER_EXIT:
			wm_filter_exit(fp);
			break;

		case WM_PAINT:
			wm_paint(hwnd);
			break;

		case WM_FILTER_FILE_OPEN:
		case WM_FILTER_FILE_CLOSE:
			BitBlt(memdc,0,0,CFGWND_X,CFGWND_Y,bgdc,0,0,SRCCOPY);
			InvalidateRect(hwnd,NULL,TRUE);
			break;

		//----------------------------------------------- ���j���[����

		case WM_RBUTTONDOWN:	// �R���e�L�X�g���j���[�\��
			show_menu(fp,lparam);
			break;

		case WM_COMMAND:
			switch(LOWORD(wparam)){
				//----------------------------------------- �\�����[�h
				case IDC_MO:
					mode = MONORAL;
					break;
				case IDC_ST:
					mode = STEREO;
					break;
				case IDC_L:
					mode = LEFT;
					break;
				case IDC_R:
					mode = RIGHT;
					break;
				//--------------------------------------------- �\���F
				case IDC_COLOR_MO:
					set_color(fp,&rgb_mono);
					break;
				case IDC_COLOR_L:
					set_color(fp,&rgb_left);
					break;
				case IDC_COLOR_R:
					set_color(fp,&rgb_right);
					break;
				//-------------------------------------------- FFT�ݒ�
				case IDC_FFT256:
					fft_n = 256;
					break;
				case IDC_FFT512:
					fft_n = 512;
					break;
				case IDC_FFT1024:
					fft_n = 1024;
					break;
				case IDC_FFT2048:
					fft_n = 2048;
					break;
				case IDC_FFT4096:
					fft_n = 4096;
					break;

				default:
					return FALSE;
			}
			return TRUE;

		//------------------------------------- ���C���E�B���h�E�֑���
		case WM_KEYUP:
		case WM_KEYDOWN:
		case WM_MOUSEWHEEL:
			SendMessage(GetWindow(hwnd, GW_OWNER), message, wparam, lparam);
			break;
	}

	return FALSE;
}


/*--------------------------------------------------------------------
* 	wm_filter_init()
*-------------------------------------------------------------------*/
static void wm_filter_init(HWND hwnd,FILTER* fp)
{
	HPEN   hrpen;
	HPEN   hwpen;
	HDC    hdc;
	HFONT  hfont;

	// �ݒ��ǂݍ���
	mode   = fp->exfunc->ini_load_int(fp,KEY_DISPMODE,DEF_DISPMODE);
	fft_n  = fp->exfunc->ini_load_int(fp,KEY_FFT_BLOCK,DEF_FFT_BLOCK);
	rgb_mono  = fp->exfunc->ini_load_int(fp,KEY_COLOR_MONO,DEF_COLOR_MONO);
	rgb_left  = fp->exfunc->ini_load_int(fp,KEY_COLOR_LEFT,DEF_COLOR_LEFT);
	rgb_right = fp->exfunc->ini_load_int(fp,KEY_COLOR_RIGHT,DEF_COLOR_RIGHT);

	hdc = GetDC(hwnd);
	memdc = CreateCompatibleDC(hdc);
	hbit = CreateCompatibleBitmap(hdc,CFGWND_X,CFGWND_Y);
	SelectObject(memdc,hbit);
	bgdc  = CreateCompatibleDC(hdc);
	bgbit = CreateCompatibleBitmap(hdc,CFGWND_X,CFGWND_Y);
	SelectObject(bgdc,bgbit);

	// �w�i�����

	hbrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
	SelectObject(bgdc,hbrush);
	PatBlt(bgdc,0,0,CFGWND_X,CFGWND_Y,PATCOPY);

	// line
	SetROP2(bgdc,R2_MERGENOTPEN);
	MoveToEx(bgdc,CFGWND_X-X_,CFGWND_Y-Y_,NULL);
	hwpen = CreatePen(PS_DOT,1,RGB(127,127,127));
	SelectObject(bgdc,hwpen);
	LineTo(bgdc,CFGWND_X-X_,_Y);
	LineTo(bgdc,_X,_Y);
	MoveToEx(bgdc,_X,_Y+40,NULL);  LineTo(bgdc,CFGWND_X-X_,_Y+40);
	MoveToEx(bgdc,_X,_Y+80,NULL);  LineTo(bgdc,CFGWND_X-X_,_Y+80);
	MoveToEx(bgdc,_X,_Y+120,NULL); LineTo(bgdc,CFGWND_X-X_,_Y+120);
	MoveToEx(bgdc,GRAFX*1/4+_X,_Y,NULL); LineTo(bgdc,GRAFX*1/4+_X,CFGWND_Y-Y_);
	MoveToEx(bgdc,GRAFX*2/4+_X,_Y,NULL); LineTo(bgdc,GRAFX*2/4+_X,CFGWND_Y-Y_);
	MoveToEx(bgdc,GRAFX*3/4+_X,_Y,NULL); LineTo(bgdc,GRAFX*3/4+_X,CFGWND_Y-Y_);

	// red line
	SetROP2(bgdc,R2_COPYPEN);
	hrpen = CreatePen(PS_SOLID,2,RGB(255,0,0));
	SelectObject(bgdc,hrpen);
	MoveToEx(bgdc,_Y,_Y,NULL);
	LineTo(bgdc,_Y,CFGWND_Y-Y_+1);
	LineTo(bgdc,CFGWND_X-X_,CFGWND_Y-Y_+1);

	DeleteObject(hrpen);
	DeleteObject(hwpen);

	BitBlt(hdc,0,0,CFGWND_X,CFGWND_Y,bgdc,0,0,SRCCOPY);
	BitBlt(memdc,0,0,CFGWND_X,CFGWND_Y,bgdc,0,0,SRCCOPY);

	// �X�y�N�g�����\���p�̃t�H���g��ݒ�
	hfont = GetStockObject(DEFAULT_GUI_FONT);
	SelectObject(memdc,hfont);
	SetTextColor(memdc,RGB(0,255,0));
	SetBkColor(memdc,RGB(0,0,0));
	SetTextAlign(memdc,TA_CENTER);

	ReleaseDC(hwnd,hdc);
}

/*--------------------------------------------------------------------
* 	wm_filter_exit()
*-------------------------------------------------------------------*/
static void wm_filter_exit(FILTER *fp)
{
	// �ݒ��ۑ�
	fp->exfunc->ini_save_int(fp,KEY_DISPMODE,mode);
	fp->exfunc->ini_save_int(fp,KEY_FFT_BLOCK,fft_n);
	fp->exfunc->ini_save_int(fp,KEY_COLOR_MONO,(long)rgb_mono);
	fp->exfunc->ini_save_int(fp,KEY_COLOR_LEFT,(long)rgb_left);
	fp->exfunc->ini_save_int(fp,KEY_COLOR_RIGHT,(long)rgb_right);

	// �f�o�C�X�R���e�L�X�g�Ƃ��J��
	DeleteObject(hbit);
	DeleteDC(memdc);
	DeleteObject(bgbit);
	DeleteDC(bgdc);
}

/*--------------------------------------------------------------------
* 	wm_paint()
*-------------------------------------------------------------------*/
static void wm_paint(HWND hwnd)
{
	HDC hdc;
	PAINTSTRUCT ps;

	hdc = BeginPaint(hwnd,&ps);

 // ���z�C���[�W�̈ꕔ���R�s�[
	BitBlt(hdc,
		ps.rcPaint.left, ps.rcPaint.top,
		ps.rcPaint.right - ps.rcPaint.left,	// ��
		ps.rcPaint.bottom - ps.rcPaint.top,	// ����
		memdc,
		ps.rcPaint.left, ps.rcPaint.top,
		SRCCOPY );

	EndPaint(hwnd,&ps);
}

/*--------------------------------------------------------------------
* 	disp_mono()
*-------------------------------------------------------------------*/
static void disp_mono(HWND hwnd,int rate,int channel)
{
	int  i,j,ch;
	HDC  hdc;
	char str[10];
	int  K = fft_n / GRAFX;
	HPEN hpen;


	// �w�i�œh��Ԃ�
	BitBlt(memdc,0,0,CFGWND_X,CFGWND_Y,bgdc,0,0,SRCCOPY);

	if(channel < 1) return;


	// ���g���\��
	TextOut(memdc,_X+1,CFGWND_Y-Y_+3,"0",1);
	itoa(rate/4,str,10);
	TextOut(memdc,GRAFX*2/4+_X+1,CFGWND_Y-Y_+3,str,lstrlen(str));
	itoa(rate/2,str,10);
	TextOut(memdc,CFGWND_X-X_-4,CFGWND_Y-Y_+3,str,lstrlen(str));

	// �X�y�N�g����
	hpen = CreatePen(PS_SOLID,1,rgb_mono);
	SelectObject(memdc,hpen);

	MoveToEx(memdc,_X,CFGWND_Y-Y_,NULL);
	for(i=0;i<GRAFX;i++){
		double temp = 0;
		for(j=0;j<K;j++)
			for(ch=0;ch<channel;ch++)
				temp += f[ch][i*K+j];
		temp = temp / K;
		temp = (abs(temp)<=0)? 0 : 20*log10(abs(temp)) * 3/2;	// �f�V�x���\��
		temp = (temp<40) ? 0 : temp-40;
		LineTo(memdc,_X+i+1,CFGWND_Y-Y_-temp);
	}
	DeleteObject(hpen);

	// �E�B���h�E�ɃR�s�[
	hdc = GetDC(hwnd);
	BitBlt(hdc,0,0,CFGWND_X,CFGWND_Y,memdc,0,0,SRCCOPY);
	ReleaseDC(hwnd,hdc);
}
/*--------------------------------------------------------------------
* 	disp_stereo()
*-------------------------------------------------------------------*/
static void disp_stereo(HWND hwnd,int rate,int channel)
{
	int  i,j,ch;
	HDC  hdc;
	char str[10];
	int  K = fft_n / GRAFX;
	HPEN hpen;

	if(channel<2){
		disp_mono(hwnd,rate,channel);
		return;
	}

	// �w�i�œh��Ԃ�
	BitBlt(memdc,0,0,CFGWND_X,CFGWND_Y,bgdc,0,0,SRCCOPY);

	// ���g���\��
	TextOut(memdc,_X+1,CFGWND_Y-Y_+3,"0",1);
	itoa(rate/4,str,10);
	TextOut(memdc,GRAFX*2/4+_X+1,CFGWND_Y-Y_+3,str,lstrlen(str));
	itoa(rate/2,str,10);
	TextOut(memdc,CFGWND_X-X_-4,CFGWND_Y-Y_+3,str,lstrlen(str));


	// ��
	hpen = CreatePen(PS_SOLID,1,rgb_left);
	SelectObject(memdc,hpen);
	MoveToEx(memdc,_X,CFGWND_Y-Y_,NULL);
	for(i=0;i<GRAFX;i++){
		double temp = 0;
		for(j=0;j<K;j++)
			temp += f[0][i*K+j];
		temp = temp / K;
		temp = (abs(temp)<=0)? 0 : 20*log10(abs(temp)) * 3/2;
		temp = (temp<40) ? 0 : temp-40;
		LineTo(memdc,_X+i+1,CFGWND_Y-Y_-temp);
	}
	DeleteObject(hpen);

	// �E
	hpen = CreatePen(PS_SOLID,1,rgb_right);
	SelectObject(memdc,hpen);
	MoveToEx(memdc,_X,CFGWND_Y-Y_,NULL);
	for(i=0;i<GRAFX;i++){
		double temp = 0;
		for(j=0;j<K;j++)
			temp += f[1][i*K+j];
		temp = temp / K;
		temp = (abs(temp)<=0)? 0 : 20*log10(abs(temp)) * 3/2;
		temp = (temp<40) ? 0 : temp-40;
		LineTo(memdc,_X+i+1,CFGWND_Y-Y_-temp);
	}
	DeleteObject(hpen);

	// �E�B���h�E�ɃR�s�[
	hdc = GetDC(hwnd);
	BitBlt(hdc,0,0,CFGWND_X,CFGWND_Y,memdc,0,0,SRCCOPY);
	ReleaseDC(hwnd,hdc);
}

/*--------------------------------------------------------------------
* 	disp_left()
*-------------------------------------------------------------------*/
static void disp_left(HWND hwnd,int rate,int channel)
{
	int  i,j,ch;
	HDC  hdc;
	char str[10];
	int  K = fft_n / GRAFX;
	HPEN hpen;

	if(channel<2){
		disp_mono(hwnd,rate,channel);
		return;
	}

	// �w�i�œh��Ԃ�
	BitBlt(memdc,0,0,CFGWND_X,CFGWND_Y,bgdc,0,0,SRCCOPY);

	// ���g���\��
	TextOut(memdc,_X+1,CFGWND_Y-Y_+3,"0",1);
	itoa(rate/4,str,10);
	TextOut(memdc,GRAFX*2/4+_X+1,CFGWND_Y-Y_+3,str,lstrlen(str));
	itoa(rate/2,str,10);
	TextOut(memdc,CFGWND_X-X_-4,CFGWND_Y-Y_+3,str,lstrlen(str));

	// ��
	hpen = CreatePen(PS_SOLID,1,rgb_left);
	SelectObject(memdc,hpen);
	MoveToEx(memdc,_X,CFGWND_Y-Y_,NULL);
	for(i=0;i<GRAFX;i++){
		double temp = 0;
		for(j=0;j<K;j++)
			temp += f[0][i*K+j];
		temp = temp / K;
		temp = (abs(temp)<=0)? 0 : 20*log10(abs(temp)) * 3/2;
		temp = (temp<40) ? 0 : temp-40;
		LineTo(memdc,_X+i+1,CFGWND_Y-Y_-temp);
	}
	DeleteObject(hpen);

	// �E�B���h�E�ɃR�s�[
	hdc = GetDC(hwnd);
	BitBlt(hdc,0,0,CFGWND_X,CFGWND_Y,memdc,0,0,SRCCOPY);
	ReleaseDC(hwnd,hdc);
}

/*--------------------------------------------------------------------
* 	disp_right()
*-------------------------------------------------------------------*/
static void disp_right(HWND hwnd,int rate,int channel)
{
	int  i,j,ch;
	HDC  hdc;
	char str[10];
	int  K = fft_n / GRAFX;
	HPEN hpen;

	if(channel<2){
		disp_mono(hwnd,rate,channel);
		return;
	}

	// �w�i�œh��Ԃ�
	BitBlt(memdc,0,0,CFGWND_X,CFGWND_Y,bgdc,0,0,SRCCOPY);

	// ���g���\��
	TextOut(memdc,_X+1,CFGWND_Y-Y_+3,"0",1);
	itoa(rate/4,str,10);
	TextOut(memdc,GRAFX*2/4+_X+1,CFGWND_Y-Y_+3,str,lstrlen(str));
	itoa(rate/2,str,10);
	TextOut(memdc,CFGWND_X-X_-4,CFGWND_Y-Y_+3,str,lstrlen(str));

	// �E
	hpen = CreatePen(PS_SOLID,1,rgb_right);
	SelectObject(memdc,hpen);
	MoveToEx(memdc,_X,CFGWND_Y-Y_,NULL);
	for(i=0;i<GRAFX;i++){
		double temp = 0;
		for(j=0;j<K;j++)
			temp += f[1][i*K+j];
		temp = temp / K;
		temp = (abs(temp)<=0)? 0 : 20*log10(abs(temp)) * 3/2;
		temp = (temp<40) ? 0 : temp-40;
		LineTo(memdc,_X+i+1,CFGWND_Y-Y_-temp);
	}
	DeleteObject(hpen);

	// �E�B���h�E�ɃR�s�[
	hdc = GetDC(hwnd);
	BitBlt(hdc,0,0,CFGWND_X,CFGWND_Y,memdc,0,0,SRCCOPY);
	ReleaseDC(hwnd,hdc);
}

/*--------------------------------------------------------------------
* 	show_menu()
*-------------------------------------------------------------------*/
static void show_menu(FILTER *fp,LPARAM lparam)
{
	HMENU hmenu;
	HMENU context;
	POINT pt;
	UINT  id;

	// ���j���[�n���h���擾
	hmenu = LoadMenu(fp->dll_hinst,"CONTEXT");
	context = GetSubMenu(hmenu,0);


	CheckMenuRadioItem(context,0,4,mode,MF_BYPOSITION);
	switch(fft_n){
		case  256: id=IDC_FFT256;  break;
		case  512: id=IDC_FFT512;  break;
		case 1024: id=IDC_FFT1024; break;
		case 2048: id=IDC_FFT2048; break;
		case 4096: id=IDC_FFT4096; break;
	}
	CheckMenuRadioItem(hmenu,IDC_FFT256,IDC_FFT4096,id,MF_BYCOMMAND);


	// ���W�擾
	pt.x = LOWORD(lparam);
	pt.y = HIWORD(lparam);
	ClientToScreen(fp->hwnd,&pt);

	// �\��
	TrackPopupMenuEx(context,0,pt.x,pt.y,fp->hwnd,NULL);

	// �j��
	DestroyMenu(hmenu);
}

/*--------------------------------------------------------------------
* 	set_color()
*-------------------------------------------------------------------*/
#define DefRGB RGB(255,255,255)
static void set_color(FILTER *fp,COLORREF *rgb)
{
	static COLORREF custom[16] = {  DefRGB,DefRGB,DefRGB,DefRGB,DefRGB,DefRGB,DefRGB,DefRGB,
									DefRGB,DefRGB,DefRGB,DefRGB,DefRGB,DefRGB,DefRGB,DefRGB };
	CHOOSECOLOR cc;

	EnableWindow(GetWindow(fp->hwnd,GW_OWNER),FALSE);

	cc.lStructSize    = sizeof(CHOOSECOLOR);
	cc.hwndOwner      = fp->hwnd;
	cc.hInstance      = NULL;
	cc.rgbResult      = *rgb;
	cc.lpCustColors   = custom;
	cc.Flags          = CC_RGBINIT;
	cc.lCustData      = NULL;
	cc.lpfnHook       = NULL;
	cc.lpTemplateName = NULL;

	if(ChooseColor(&cc))
		*rgb = cc.rgbResult;

	EnableWindow(GetWindow(fp->hwnd,GW_OWNER),TRUE);
}


