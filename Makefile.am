CPPFLAGS = -DUSE_QUETZAL -DSTRICTZ
bin_PROGRAMS = xfrotz
xfrotz_SOURCES = buffer.c fastmem.c files.c frotz.h hotkey.c input.c main.c \
	math.c object.c process.c random.c redirect.c screen.c sound.c \
	stream.c table.c text.c variable.c quetzal.c x_frotz.h x_init.c \
	x_input.c x_pic.c x_sample.c x_screen.c x_text.c strictz.c
man_MANS = xfrotz.6

ZFONTSBDF = Zork_r400-10.bdf Zork_r400-11.bdf Zork_r400-13.bdf \
	Zork_r400-16.bdf Zork_r400-20.bdf
ZFONTSPCF = $(ZFONTSBDF:.bdf=.pcf)
EXTRA_DIST = $(man_MANS) $(ZFONTSBDF)

CLEANFILES = $(ZFONTSPCF)

install-data-local: $(INSTALL_FONTS)

install-fonts: $(ZFONTSPCF)
	$(mkinstalldirs) $(DESTDIR)$(XFONTS_DIR)
	$(INSTALL) -m 644 $(ZFONTSPCF) $(DESTDIR)$(XFONTS_DIR)
	-$(MKFONTDIR) $(DESTDIR)$(XFONTS_DIR)
	@echo Run \`xset fp rehash\' to notify your X server
	@echo that new fonts are available.

.bdf.pcf:
	$(BDFTOPCF) -t $< -o $@

SUFFIXES = .bdf .pcf
