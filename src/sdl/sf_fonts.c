#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sf_frotz.h"

// font handling

/*
struct sfontstruct {
  int refcount;
  void (*destroy)(SFONT *);
  int (*height)(SFONT *);
  int (*ascent)(SFONT *);
  int (*descent)(SFONT *);
  int (*minchar)(SFONT *);
  int (*maxchar)(SFONT *);
  int (*hasglyph)(SFONT *,int,int);
  SF_glyph *(*getglyph)(SFONT *,int,int);
  };
*/

typedef struct {
  int refcount;
  word minchar, maxchar, defchar;
  byte ascent, descent;
  int glyphs[0];        // offsets to glyphs from start of rec
  } SF_bdffont;

char * m_fontfiles[8];

static char s[1026];

static char * starts( char *s, char *id){
  int len = strlen(id);
  while (*s == ' ') s++;
  if (memcmp(s,id,len)==0) return (s+len);
  return NULL;
  }

static int hexd( char c){
  if (c >= '0' && c <= '9') return c-'0';
  if (c >= 'A' && c <= 'F') return c-'A'+10;
  if (c >= 'a' && c <= 'f') return c-'a'+10;
  return 0;
  }

static void gethex( char *p, byte *dst, int n){
  while (n--){
	*dst++ = 16*hexd(p[0])+hexd(p[1]);
	p += 2;
	}
  }

#define ERRET(n) { *err = n; return font;}

static SF_bdffont *sBDXload( FILE *f, int *err, int *size, int MAXCHAR){
  int totb, i, k, wh[4];
  byte *po, *pbeg;
  char *p, *q;
  char *fontname = "", *copyright = "unknown";
  int fngot=0,cpgot=0;
  int hasenc, hasbbx;
  SF_bdffont *font = NULL;
  int foffs, minch, maxch, rejected=0;
  int defchar = -1, descent = -1, ascent = -1, nprop, nchars;
	// NOTE: file MUST be opened as binary on MSDOS, otherwise
	// ftell() won't return correct values if the file does not
	// have CR's (as is usually the case, since BDF files frequently 
	// come from Unix systems)

	// header
  for (;;){
	fgets(s,1024,f);
	if (feof(f)) ERRET(-1)	// errorexit(99,"unexpected EOF\n");
	if ((p = starts(s,"FONT "))){
		while (*p == ' ') p++;
		q = p;
		while (*p >= ' ') p++;
		*p = 0;
		fontname = strdup(q);
		fngot = 1;
		}
	if ((p = starts(s,"STARTPROPERTIES "))){
		nprop = atoi(p);
		break;
		}
	}
  for (i=0;i<nprop;i++){
	fgets(s,1024,f);
	if (feof(f)) ERRET(-51)	// errorexit(99,"unexpected EOF\n");
	if ((p = starts(s,"COPYRIGHT "))){
		while (*p == ' ') p++;
		q = p;
		while (*p >= ' ') p++;
		*p = 0;
		copyright = strdup(q);
		cpgot = 1;
		}
	if ((p = starts(s,"DEFAULT_CHAR ")))
		defchar = atoi(p);
	if ((p = starts(s,"FONT_ASCENT ")))
		ascent =  atoi(p);
	if ((p = starts(s,"FONT_DESCENT ")))
		descent = atoi(p);
	}
  for (;;){
	fgets(s,1024,f);
	if (feof(f)) ERRET(-61)	// errorexit(99,"unexpected EOF\n");
	if ((p = starts(s,"CHARS "))){
		nchars = atoi(p);
		break;
		}
	}
  foffs = ftell(f);

	// first pass
  totb = 0;
  minch = 65536; maxch = -1;
  hasenc = 0;
  for (i=0;i<nchars;){
	fgets(s,1024,f);
	if (feof(f)) ERRET(-71)	// errorexit(99,"unexpected EOF\n");
	if ((p = starts(s,"ENCODING "))){
		k = atoi(p);
		k &= 0xffff;
		if (k <= MAXCHAR) {
			if (k < minch) minch = k;
			if (k > maxch) maxch = k;
			}
		else rejected++;
		hasenc = 1;
		}
	if ((p = starts(s,"BBX "))){
		if (!hasenc) ERRET(-10)
		sscanf(p,"%d %d",wh,wh+1);
//printf("c%d %d\n",k,((wh[0]+7)/8)*wh[1] + sizeof(SF_glyph));
		if (k <= MAXCHAR){
		  totb += ((wh[0]+7)/8)*wh[1] + sizeof(SF_glyph);
		  }
		i++;
		hasenc = 0;
		}
	}
//printf("nchars=%d minch=%d maxch=%d\n",nchars,minch,maxch);
//printf("sizeof(SF_glyph)=%d\n",sizeof(SF_glyph));
//  printf("<%s>\n",fontname);
//  printf("<%s>\n",copyright);
//  printf("dc%d a%d d%d %d..%d exp%d read%d totb%d\n",
//	defchar,ascent,descent,minch,maxch,nchars,i,totb);
  if (ascent < 0 || descent < 0 || nchars != i)
	ERRET(-2)
//    errorexit(99,"??? dc%d a%d d%d %d..%d exp%d read%d\n",
//	defchar,ascent,descent,minch,maxch,nchars,k);
  if (defchar < minch || defchar > maxch){
//	printf("WARNING: defchar=%d, set to",defchar);
	if (defchar < minch) defchar = minch;
	if (defchar > maxch) defchar = maxch;
//	printf(" %d\n",defchar);
	}
//  nchars = i;
  totb += (maxch-minch+1)*sizeof(int)+sizeof(SF_bdffont)+strlen(fontname)+
	strlen(copyright)+2;
//  printf("totb %d\n",totb);
  font = calloc(1,totb);
  if (!font) ERRET(-3)	// errorexit(99,"malloc()\n");
//printf("allocated: %p-%p\n",font,((byte *)font)+totb);
//  memcpy(&(font->magic[0]),"gBDX",4);
  font->minchar = minch;
  font->maxchar = maxch;
  font->defchar = defchar;
  font->ascent = ascent;
  font->descent = descent;
  pbeg = (byte *)font;
  po = (byte *)(&(font->glyphs[maxch-minch+1]));
  k = strlen(fontname)+1;
  memcpy(po,fontname,k); po += k; if (fngot) free(fontname);
  k = strlen(copyright)+1;
  memcpy(po,copyright,k); po += k; if (cpgot) free(copyright);
  for (i=minch;i<=maxch;i++) font->glyphs[i-minch] = 0;

	// second pass
  fseek(f,foffs,0);
  for (k=0;k<nchars;k++){
	SF_glyph *bg;
	int j;
	int dwid,w,h;

	bg = (SF_glyph *)po;
	hasenc = hasbbx = 0;
	for (;;){
		fgets(s,1024,f);
		if (feof(f)) ERRET(-81)	// errorexit(99,"unexpected EOF c=%d\n",i);
		if ((p = starts(s,"ENCODING "))){
			i = atoi(p);
			i &= 0xffff;
			if (i <= MAXCHAR)
			  font->glyphs[i-minch] = po-pbeg;
			hasenc = 1;
			}
		if ((p = starts(s,"DWIDTH "))){
			if (!hasenc) ERRET(-11)
			dwid = atoi(p);
			}
		if ((p = starts(s,"BBX "))){
			if (!hasenc) ERRET(-12)
			sscanf(p,"%d %d %d %d",wh,wh+1,wh+2,wh+3);
			hasbbx = 1;
			}
		if (starts(s,"BITMAP")) break;
		}
	if (!hasenc || !hasbbx) ERRET(-13)
	if (i <= MAXCHAR) {
	  bg->dx = dwid;
	  bg->w = wh[0]; w = (wh[0]+7)/8;
	  bg->h = h = wh[1];
	  bg->xof = wh[2];
	  bg->yof = wh[3];
	  po = (byte *)(&(bg->bitmap[0]));
//printf("k=%d ch=%d delta=%d\n",k,i,po-(byte *)bg);
	  for (j=0;j<h;j++){
		fgets(s,1024,f);
		if (feof(f)) ERRET(-91)	// errorexit(99,"unexpected EOF c=%d (B)\n",i);
		gethex(s,po,w);
		po += w;
		}
	  }
//printf("used %d nextpo %p\n",po-pbeg-font->glyphs[i-minch],po);
	}
  *err = 0;
  *size = totb;
  return font;
  }

// destructor for fonts with all dynamic data
static void bdestroy( SFONT *s)
  {
  if (s)
    {
    if (s->data) free(s->data);
    free(s);
    }
  }

static int bheight( SFONT *s)
  {
  if (s) if (s->data)
	{
	SF_bdffont *f = s->data;
	return ((int)(f->ascent)+(int)(f->descent));
	}
  return 0;
  }

static int bascent( SFONT *s)
  {
  if (s) if (s->data) return (int)((SF_bdffont *)(s->data))->ascent;
  return 0;
  }

static int bdescent( SFONT *s)
  {
  if (s) if (s->data) return (int)((SF_bdffont *)(s->data))->descent;
  return 0;
  }

static int bminchar( SFONT *s)
  {
  if (s) if (s->data) return (int)((SF_bdffont *)(s->data))->minchar;
  return 0;
  }

static int bmaxchar( SFONT *s)
  {
  if (s) if (s->data) return (int)((SF_bdffont *)(s->data))->maxchar;
  return 0;
  }

static SF_glyph *getglyph( SFONT *fo, word c, int allowdef)
  {
  int m; SF_bdffont *b;
  if (!fo) return NULL;
  b = fo->data;
  if (!b) return NULL;
  if (c < b->minchar || c > b->maxchar)
	{
	if (allowdef) c = b->defchar;
	else return NULL;
	}
  m = b->glyphs[c-b->minchar];
  if (!m)
	{
	if (allowdef) m = b->glyphs[0];
	else return NULL;
	}
  return (SF_glyph *)(((byte *)b)+m);
  }

static int hasglyph( SFONT *fo, word c, int allowdef)
  {
  return (getglyph(fo,c,allowdef) != NULL);
  }

static SFONT * makefont( SF_bdffont *b)
  {
  SFONT * res;
  if (!b) return NULL;
  res = calloc(1,sizeof(SFONT));
  if (!res)
	{
	free(b);
	return NULL;
	}
  res->destroy = bdestroy;
  res->height = bheight;
  res->data = b;
  res->ascent = bascent;
  res->descent = bdescent;
  res->minchar = bminchar;
  res->maxchar = bmaxchar;
  res->hasglyph = hasglyph;
  res->getglyph = getglyph;
  return res;
  }

static SFONT * loadfont( char *fname, int *err, int *size){
  SF_bdffont * font; SFONT *res;
  FILE *f;
  *err = 0;
  if (!fname) { *err = -8; return NULL;}
	// NOTE: file MUST be opened as binary on MSDOS, otherwise
	// ftell() won't return correct values if the file does not
	// have CR's (as is usually the case, since BDF files frequently 
	// come from Unix systems)
  f = fopen(fname,"rb"); if (!f) {*err = -7; return NULL;}
  font = sBDXload(f,err,size,65535);
  fclose(f);
  if (*err)
	{
	if (font) free(font);
	return NULL;
	}
  res = makefont(font);
  if (!res)
	{
	free(font);
	*err = -9999;
	return NULL;
	}
  return res;
  }

// these are the 8 fonts needed
// PROPORTIONAL FONT
//   ROMAN
//   ROMAN BOLD
//   ITALIC
//   ITALIC BOLD
// FIXED FONT
//   ROMAN
//   ROMAN BOLD
//   ITALIC
//   ITALIC BOLD

#define NUMFONTS 9

static SFONT *myfonts[9] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

static int styleidx( int zfont, int style)
  {
  int k = 0;
  if (zfont == GRAPHICS_FONT) return 8;
  if (zfont != TEXT_FONT) zfont = FIXED_WIDTH_FONT;
  if (style & FIXED_WIDTH_STYLE) zfont = FIXED_WIDTH_FONT;
  if (h_flags & FIXED_FONT_FLAG) zfont = FIXED_WIDTH_FONT;
  if (zfont != TEXT_FONT) k += 4;
  if (style & EMPHASIS_STYLE) k += 2;
  if (style & BOLDFACE_STYLE) k++;
  return k;
  }

static SF_textsetting current;
#define MAXSTACK 16
static SF_textsetting tsstack[MAXSTACK];
static int tsstackptr = 0;

// does not increase the refcount of f - user must do that
static void setfontk( int k, SFONT *f)
  {
  if (myfonts[k])
	{
	myfonts[k]->refcount--;
	if (myfonts[k]->refcount == 0) myfonts[k]->destroy(myfonts[k]);
        }
  myfonts[k] = f;
  }

static void cleanfonts()
  {
  int i;
  for (i=0;i<8;i++) setfontk(i,NULL);
/*  for (i=0;i<8;i++)
	if (m_fontfiles[i]){ free(m_fontfiles[i]); m_fontfiles[i] = NULL;}*/
  }

SF_textsetting * sf_curtextsetting()
  {
  return &current;
  }

void sf_pushtextsettings()
  {
  if (tsstackptr < MAXSTACK)
	{
	tsstack[tsstackptr] = current;
	tsstackptr++;
	}
  }

void sf_poptextsettings()
  {
  if (tsstackptr)
	{
	tsstackptr--;
	current = tsstack[tsstackptr];
	}
  }

/*
 * os_check_unicode
 *
 * Return with bit 0 set if the Unicode character can be
 * displayed, and bit 1 if it can be input.
 * 
 *
 */
int os_check_unicode(int font, zword c)
  {
  return ((current.font->hasglyph(current.font,c,0) != 0) ? 3 : 2);
  }

static int charwidth( zword c, int *oh)
  {
  SF_glyph *g; int ww = 0;
  if (c == ZC_INDENT) return (3*charwidth(' ',oh));
  if (c == ZC_GAP) return (2*charwidth(' ',oh));
  *oh = 0;
  g = current.font->getglyph(current.font,c,1);
  if (g)
	{
	int ext = g->w+g->xof;
	if (ext > g->dx) *oh = ext-g->dx; 
	ww = (g->dx);
	}
  return ww;
  }

/*
 * os_char_width
 *
 * Return the length of the character in screen units.
 *
 */
int os_char_width(zchar c)
  {
  int w, oh;
  w = charwidth( c, &oh);
  return (w+oh);
  }


static void setfont( int zfont){
  int k = styleidx(zfont,current.style);
//printf("%d.",k);
  current.font = myfonts[k];
//  if (k < 4) zfont = TEXT_FONT;
//  else zfont = FIXED_WIDTH_FONT;
  current.zfontnum = zfont;
  current.proportional = (k < 4);
  }

static void setstyle( int style){
  current.style = style;
  setfont(current.zfontnum);
  }

/*
 * os_font_data
 *
 * Return true if the given font is available. The font can be
 *
 *    TEXT_FONT
 *    PICTURE_FONT
 *    GRAPHICS_FONT
 *    FIXED_WIDTH_FONT
 *
 * The font size should be stored in "height" and "width". If
 * the given font is unavailable then these values must _not_
 * be changed.
 *
 */
int os_font_data( int font, int *height, int *width)
  {
  switch (font)
	{
	case TEXT_FONT:
	case FIXED_WIDTH_FONT:
	case GRAPHICS_FONT:
		{
		sf_pushtextsettings();
		setfont(font);
		setstyle(0);
		*height = current.font->height(current.font);
		*width = os_char_width((zword)('0'));
		sf_poptextsettings();
		return 1;
		}
	default: break;
	}
  return 0;
  }

/*
 * os_set_font
 *
 * Set the font for text output. The interpreter takes care not to
 * choose fonts which aren't supported by the interface.
 *
 */
void os_set_font( int new_font)
  {
//printf("os_set_font(%d)\n",new_font);
  sf_flushtext();
  setfont(new_font);
  }

/*
 * os_set_text_style
 *
 * Set the current text style. Following flags can be set:
 *
 *     REVERSE_STYLE
 *     BOLDFACE_STYLE
 *     EMPHASIS_STYLE (aka underline aka italics)
 *     FIXED_WIDTH_STYLE
 *
 */
void os_set_text_style( int new_style)
  {
  sf_flushtext();
  setstyle(new_style);
  }

/*
 * os_string_width
 *
 * Calculate the length of a word in screen units. Apart from letters,
 * the word may contain special codes:
 *
 *    ZC_NEW_STYLE - next character is a new text style
 *    ZC_NEW_FONT  - next character is a new font
 *
 */
int os_string_width(const zchar *s)
  {
  int width = 0, wacc = 0, oh = 0; zword c;

  setfont(current.zfontnum);
	// Look for style or font changes, or indents
  sf_pushtextsettings();
  while ((c = *s++))
	{
	if (c == ZC_NEW_STYLE)
		{
		wacc += oh;
		os_set_text_style(*s++);
		}
	else if (c == ZC_NEW_FONT)
		{
		wacc += oh;
		os_set_font(*s++);
		}
	else
		width += charwidth(c,&oh);
	}
  sf_poptextsettings();

  return (width+oh+wacc);
  }

/*
 * os_display_string
 *
 * Pass a string of characters to os_display_char.
 *
 */
void os_display_string(const zchar *s)
  {
  zword c;
  while ((c = *s++) != 0)
	{
	if (c == ZC_NEW_FONT)
		os_set_font(*s++);
	else if (c == ZC_NEW_STYLE)
		os_set_text_style(*s++);
	else
		os_display_char(c);
	}
  }

/*
 * os_display_char
 *
 * Display a character of the current font using the current colours and
 * text style. The cursor moves to the next position. Printable codes are
 * all ASCII values from 32 to 126, ISO Latin-1 characters from 160 to
 * 255, ZC_GAP (gap between two sentences) and ZC_INDENT (paragraph
 * indentation), and Unicode characters above 255. The screen should not
 * be scrolled after printing to the bottom right corner.
 *
 */
void os_display_char(zchar c)
  {
  if (c == ZC_INDENT)
	{
	os_display_char(' ');
	os_display_char(' ');
	os_display_char(' ');
	}
  else if (c == ZC_GAP)
	{
	os_display_char(' ');
	os_display_char(' ');
	}
  else if ((c >= 32 && c <= 126) || (c >= 160))
	{
	SF_glyph *g;
	setfont(current.zfontnum);
	g = current.font->getglyph(current.font,c,1);
//printf("{%c}%d.%p/%p",c,current.zfontnum,current.font,g);
	if (g)
	  {
//printf("[%c]\n",c); fflush(stdout);
	  sf_writeglyph(g);
	  m_exitPause = true;
	  }
	}
}

/*
 * os_buffer_screen
 *
 * Set the screen buffering mode, and return the previous mode.
 * Possible values for mode are:
 *
 *     0 - update the display to reflect changes when possible
 *     1 - do not update the display
 *    -1 - redraw the screen, do not change the mode
 *
 */
int os_buffer_screen (int mode)
  {
  if (mode == -1)
	sf_flushdisplay();
  return 0;
  }

/*
 * os_wrap_window
 *
 * Return non-zero if the window should have text wrapped.
 *
 */
int os_wrap_window (int win)
  {
  return 1;
  }

/*
 * os_window_height
 *
 * Called when the height of a window is changed.
 *
 */
void os_window_height (int win, int height)
  {
  }

/*
 * os_set_cursor
 *
 * Place the text cursor at the given coordinates. Top left is (1,1).
 *
 */
void os_set_cursor(int row, int col)
  {
  sf_flushtext();
//	theWnd->ResetOverhang();
  current.cx = col-1;
  current.cy = row-1;
  }

extern SF_bdffont * SF_defaultfont;
extern int SF_defaultfontsize;

static void destroySFonly( SFONT *f)
  {
  if (f) free(f);
  }

extern SFONT *SF_font3, *SF_font3double;

SFONT * (*ttfontloader)( char *fspec, int *err) = NULL;
void (*ttfontsdone)() = NULL;

static SFONT *tryloadfont( char *fspec)
  {
  int err,size;
  char *p;
  SFONT *b = NULL;
  for (;;){
    p = strchr(fspec,'|');
    if (p) *p = 0;
    if (ttfontloader)
	b = ttfontloader(fspec,&err);
    if (!b)
	b = loadfont(fspec,&err,&size);
    if (b) break;
    if (p) { *p = '|'; fspec = p+1;}
    else break;
    }
  return b;
  }

SFONT *sf_VGA_SFONT;

// ensure a font loaded
void sf_initfonts()
  {
  int i, j, size=0;
  int w,h,nby,m,nocc;
  byte *cfont, *bmp; SF_glyph *g;
  SF_bdffont *b, *norm, *emph, *bold, *bemp;
  SFONT *Norm, *Emph=NULL, *Bold=NULL, *Bemp=NULL;

  norm = SF_defaultfont;
//dumpfont(norm);
  sf_VGA_SFONT = Norm = makefont(norm);
  if (!Norm) os_fatal("malloc() failure in initfonts()");
  Norm->destroy = destroySFonly;

	// get size of default font
  size = SF_defaultfontsize;

	// copy norm to emphasized
  emph = malloc(size);
  if (!emph) os_fatal("malloc() failure in initfonts()");
  Emph = makefont(emph);
  if (!Emph) os_fatal("malloc() failure in initfonts()");
  memcpy (emph, norm, size);
	// emphasize (underline)...
  cfont = (byte *)emph;
  for (i = norm->minchar;i <= norm->maxchar;i++){
	m = norm->glyphs[i-norm->minchar];
	if (!m) continue;
	g = (SF_glyph *)(cfont + m);
	w = g->dx;
	h = g->h; nby = (g->w+7)/8;
	bmp = (byte *)(&(g->bitmap[0]));
	bmp[h-2] = 0xff;
	}
	// make a copy for bold
  bold = malloc(size);
  if (!bold) os_fatal("malloc() failure in initfonts()");
  Bold = makefont(bold);
  if (!Bold) os_fatal("malloc() failure in initfonts()");
  memcpy (bold, norm, size);
	// boldify...
  cfont = (byte *)bold;
  for (i = norm->minchar;i <= norm->maxchar;i++){
	int c;
	m = norm->glyphs[i-norm->minchar];
	if (!m) continue;
	g = (SF_glyph *)(cfont + m);
	w = g->dx;
	h = g->h; nby = (g->w+7)/8;
	bmp = (byte *)(&(g->bitmap[0]));
	for (j=0;j<h;j++){
	  c = bmp[j];
	  bmp[j] = (c) | (c >> 1);
	  }
	}
	// copy bold to bold, emphasized
  bemp = malloc(size);
  if (!bemp) os_fatal("malloc() failure in initfonts()");
  Bemp = makefont(bemp);
  if (!Bemp) os_fatal("malloc() failure in initfonts()");
  memcpy (bemp, bold, size);
	// emphasize (underline)...
  cfont = (byte *)bemp;
  for (i = norm->minchar;i <= norm->maxchar;i++){
	m = norm->glyphs[i-norm->minchar];
	if (!m) continue;
	g = (SF_glyph *)(cfont + m);
	w = g->dx;
	h = g->h; nby = (g->w+7)/8;
	bmp = (byte *)(&(g->bitmap[0]));
	bmp[h-2] = 0xff;
	}

  myfonts[0] = myfonts[4] = Norm;
  norm->refcount = 2;
  myfonts[1] = myfonts[5] = Bold;
  bold->refcount = 2;
  myfonts[2] = myfonts[6] = Emph;
  emph->refcount = 2;
  myfonts[3] = myfonts[7] = Bemp;
  bemp->refcount = 2;

//  for (i=0;i<8;i++) myfonts[i] = SF_defaultfont;
//  SF_defaultfont->refcount = 9;

  CLEANREG(cleanfonts);

  if (!m_vga_fonts)
    {
    for (i=0;i<8;i++)
      if (m_fontfiles[i])
	{
	SFONT *b = tryloadfont(m_fontfiles[i]);
	if (!b) fprintf(stderr,"WARNING: could not load font%d [%s]\n",i,m_fontfiles[i]);
	else
		{
		setfontk(i,b);
		b->refcount = 1;
		}
	}
    }

  if (ttfontsdone) ttfontsdone();
	// now set the graphics font
  if (myfonts[4]->height(myfonts[4]) < 16)
	myfonts[8] = SF_font3;
  else
	myfonts[8] = SF_font3double;

//for (i=0;i<8;i++){ SFONT *s = myfonts[i]; printf("%d %p %d %d %d %d %d\n",
//i,s,s->minchar(s),s->maxchar(s),s->ascent(s),s->descent(s),s->height(s));}

  }

