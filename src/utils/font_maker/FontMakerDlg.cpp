// FontMakerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "FontMaker.h"
#include "FontMakerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFontMakerDlg dialog

CFontMakerDlg::CFontMakerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFontMakerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFontMakerDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFontMakerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFontMakerDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CFontMakerDlg, CDialog)
	//{{AFX_MSG_MAP(CFontMakerDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_FONTSELECT, OnButtonFontselect)
	ON_BN_CLICKED(IDC_BUTTON_FONTMAKE, OnButtonFontmake)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFontMakerDlg message handlers

void func1()
{

}

BOOL CFontMakerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_iFontSize = 0;

	func1();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CFontMakerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CFontMakerDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	// print English and Hangul samples when drawing window.
	else
	{
        CPaintDC dc( this );

        CString clsData;
        SIZE stHangulSize;
		SIZE stEnglishSize;
		CPen clPen( PS_DOT, 1, RGB( 255, 0, 0 ) );
		CPen* pclOldPen;

		// set background color to transparent in order not to erase window background when displaying text.
		dc.SetBkMode( TRANSPARENT );

		// set the created font.
        dc.SelectObject( &m_clFont );

		// set pen.
		pclOldPen = dc.SelectObject( &clPen );

		// save font size.
		GetTextExtentPoint32( dc.m_hDC, "A", 1, &stEnglishSize );
        GetTextExtentPoint32( dc.m_hDC, "가", 2, &stHangulSize );

		// print top and bottom base lines.
		dc.MoveTo( 0, 5 );
		dc.LineTo( 1000, 5 );
		dc.MoveTo( 0, 5 + stEnglishSize.cy - 1 );
		dc.LineTo( 1000, 5 + stEnglishSize.cy - 1 );
		// print English sample.
        dc.TextOut( 0, 5, "abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ" );
		
		// print top and bottom base lines.
		dc.MoveTo( 0, 30 );
		dc.LineTo( 1000, 30 );
		dc.MoveTo( 0, 30 + stHangulSize.cy - 1 );
		dc.LineTo( 1000, 30 + stHangulSize.cy - 1 );
		// print Hangul sample.
        dc.TextOut( 0, 30, "ㄱㄴㄷㄹㅁㅂㅅㅇㅈㅊㅋㅌㅍㅎ 가나다라마바사아자차카타파하" );

		// print font size.
        clsData.Format( "English Font Pixel %dx%d, Hangul Font Pixel %dx%d", 
			stEnglishSize.cx, stEnglishSize.cy, stHangulSize.cx, stHangulSize.cy );
        dc.TextOut( 0, 60, clsData );

		// restore pen.
		dc.SelectObject( pclOldPen );
    }
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CFontMakerDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CFontMakerDlg::OnButtonFontselect() 
{
    CFontDialog dlg;
    LOGFONT stLogFont;
    CPaintDC dc( this );

    if( dlg.DoModal() != IDOK )
    {
        return ;
    }

	// return the selected font.
    dlg.GetCurrentFont( &stLogFont );

	// If font size == 16, GetSize returns 160.
	stLogFont.lfHeight = dlg.GetSize() / 10;
	if( stLogFont.lfHeight > 16 )
	{
		AfxMessageBox( "can not create font over 16 size." );
		return ;
	}

	m_iFontSize = stLogFont.lfHeight;

    m_clFont.DeleteObject();
    if( m_clFont.CreateFontIndirect( &stLogFont ) == FALSE )
    {
        TRACE( "font creation failure\n" );
    }

    // redrawing window.
    Invalidate( TRUE );
}

/* create both Hangul font file and English font file. */
void CFontMakerDlg::OnButtonFontmake() 
{
    CPaintDC dc( this );
    CDC clTempDc;
    CBitmap clBitmap;

    // create DC, and set bitmap.
    clTempDc.CreateCompatibleDC( &dc );
    clBitmap.CreateCompatibleBitmap( &clTempDc, 100, 100 );
    clTempDc.SelectObject( &clBitmap );
    clTempDc.SelectObject( &m_clFont );
    
    // create Hangul font file.
    SaveHangulFont( &clTempDc );
    
    // create English font file.
    SaveEnglishFont( &clTempDc );

    AfxMessageBox( "font creation success" );
}

/* create Hangul font file. */
void CFontMakerDlg::SaveHangulFont( CDC* pclTempDC )
{
    int i;
    int j;
    int iCount;
    char vcBuffer[ 2 ];

    // create file.
    if( CreateHangulFontFile() == FALSE )
    {
        return ;
    }

    /* save font data to file. */
    // create consonants/vowels (0xA4A1 ~ 0xA4D3).
    vcBuffer[ 0 ] = ( char )0xA4;
    for( i = 0xA1 ; i <= 0xD3 ; i++ )
    {
        vcBuffer[ 1 ] = i;
        pclTempDC->TextOut( 0, 0, vcBuffer, 2 );

        // create bitmask about all pixels.
        SaveBitMask( pclTempDC, TRUE );
    }

    iCount = 0;
    // create complete hanguls (0xB0A1 ~ 0xC8FE). 
    for( j = 0xB0 ; j <= 0xC8 ; j++ )
    {
        vcBuffer[ 0 ] = j;
        for( i = 0xA1 ; i <= 0xFE ; i++  )
        {
            vcBuffer[ 1 ] = i;
            pclTempDC->TextOut( 0, 0, vcBuffer, 2 );

            // create bitmask about all pixels.
            SaveBitMask( pclTempDC, TRUE );
            iCount++;
        }
    }

    TRACE( "total count: %d\n", iCount );

    // remove '\r\n' by moving back 3 positions.
    m_clFile.Seek( -3, SEEK_CUR );
    WriteFontData( "};\r\n" );
    CloseFontFile();
}

/* create English font file. */
void CFontMakerDlg::SaveEnglishFont( CDC* pclTempDC )
{
    int i;
    int iCount;
    char cBuffer;

    // create file.
    if( CreateEnglishFontFile() == FALSE )
    {
        return ;
    }
	
	/* save font data to file. */
	
    iCount = 0;
	// create characters (0 ~ 255).
    for( i = 0 ; i <= 0xFF ; i++ )
    {
        cBuffer = i;
		pclTempDC->FillSolidRect( 0, 0, 20, 20, RGB( 255, 255, 255 ) );
        pclTempDC->TextOut( 0, 0, &cBuffer, 1 );

        // create bitmask about all pixels.
        SaveBitMask( pclTempDC, FALSE );
        iCount++;
    }

    TRACE( "Total Count = %d\n", iCount );

    // remove '\r\n' by moving back 3 positions.
    m_clFile.Seek( -3, SEEK_CUR );
    WriteFontData( "};\r\n" );
    CloseFontFile();
}

/* save bitmask */
void CFontMakerDlg::SaveBitMask( CDC* clTempDc, BOOL bHangul )
{
    int i;
    int j;
    COLORREF stColor;
    unsigned short usBitMask;
	int iFontWidth;

	// If it's Hangul, search as many as font size.
	if( bHangul == TRUE )
	{
		iFontWidth = m_iFontSize;
	}
	// If it's English, search as many as half font size.
	else
	{
		iFontWidth = m_iFontSize / 2;
	}

	// create font bitmap, and save it to file.
    for( j = 0 ; j < m_iFontSize ; j++ )
    {
        usBitMask = 0;
        for( i = 0 ; i < iFontWidth ; i++ )
        {
            stColor = clTempDc->GetPixel( i, j );
            if( stColor != 0xFFFFFF )
            {
                usBitMask |= ( 0x01 << ( iFontWidth - 1 - i ) );
            }
        }
        
        // write bitmask.
        WriteFontData( usBitMask, bHangul );
        WriteFontData( "," );
    }
    WriteFontData( "\r\n" );
}

/* create Hangul font file with Hangul font data (array) */
BOOL CFontMakerDlg::CreateHangulFontFile()
{
	char* pcHeader = "// Hangul font data: %d * %d pixels per a character, total 2401 characters (51 consonants/vowels + 2350 complete hanguls)\r\n"
                     "unsigned short g_fontHangul[] = {\r\n";
    char vcBuffer[ 1024 ];

	sprintf( vcBuffer, pcHeader, m_iFontSize, m_iFontSize );
	// create file.
    if( m_clFile.Open( "font_hangul.c", CFile::modeCreate | 
        CFile::modeReadWrite ) == FALSE )
    {
        AfxMessageBox( "Hangul file creation failure" );
        return FALSE;
    }
    m_clFile.Write( vcBuffer, strlen( vcBuffer ) );
    return TRUE;
}

/* create English font file with English font data (array) */
BOOL CFontMakerDlg::CreateEnglishFontFile()
{
	char* pcHeader = "// English font data: %d * %d pixels per a character, total 256 characters (same order as ASCII code)\r\n"
                     "unsigned char g_fontEnglish[] = {\r\n";
    char vcBuffer[ 1024 ];

	sprintf( vcBuffer, pcHeader, m_iFontSize / 2, m_iFontSize );
	// create file.
    if( m_clFile.Open( "font_english.c", CFile::modeCreate | 
        CFile::modeReadWrite ) == FALSE )
    {
        AfxMessageBox( "English file creation failure" );
        return FALSE;
    }
    m_clFile.Write( vcBuffer, strlen( vcBuffer ) );
    return TRUE;
}

/* write font data to file */
void CFontMakerDlg::WriteFontData( unsigned short usData, BOOL bHangul )
{
    char vcBuffer[ 20 ];

	// If it's Hangul, make 2 bytes-sized data.
	if( bHangul == TRUE )
	{
		sprintf( vcBuffer, "0x%04X", usData );
	}
	// If it's English, make 1 byte-sized data.
	else
	{
		sprintf( vcBuffer, "0x%02X", usData & 0xFF );
	}

    m_clFile.Write( vcBuffer, strlen( vcBuffer ) );
}

/* write font data to file */
void CFontMakerDlg::WriteFontData( char* pcData )
{
    m_clFile.Write( pcData, strlen( pcData ) );
}

/* close font file */
void CFontMakerDlg::CloseFontFile()
{
    m_clFile.Close();
}
