LINK = @echo "Linking $@"; $(LIBTOOL) --mode=link $(CCLD) $(AM_CFLAGS) $(CFLAGS) \
        $(AM_LDFLAGS) $(LDFLAGS) -o $@

.c.o:
			@echo "Building $@"
@am__fastdepCC_TRUE@	@if $(COMPILE) -MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" \
@am__fastdepCC_TRUE@	  -c -o $@ `test -f '$<' || echo '$(srcdir)/'`$<; \
@am__fastdepCC_TRUE@	then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; \
@am__fastdepCC_TRUE@	else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; \
@am__fastdepCC_TRUE@	fi
@AMDEP_TRUE@@am__fastdepCC_FALSE@	@source='$<' object='$@' libtool=no @AMDEPBACKSLASH@
@AMDEP_TRUE@@am__fastdepCC_FALSE@	depfile='$(DEPDIR)/$*.Po' tmpdepfile='$(DEPDIR)/$*.TPo' @AMDEPBACKSLASH@
@AMDEP_TRUE@@am__fastdepCC_FALSE@	$(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
@am__fastdepCC_FALSE@	$(COMPILE) -c `test -f '$<' || echo '$(srcdir)/'`$<

.c.obj:
			@echo "Building $@"
@am__fastdepCC_TRUE@	@if $(LTCOMPILE) -MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" \
@am__fastdepCC_TRUE@	  -c -o $@ `if test -f '$<'; then $(CYGPATH_W) '$<'; else $(CYGPATH_W) '$(srcdir)/$<'; fi`; \
@am__fastdepCC_TRUE@	then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; \
@am__fastdepCC_TRUE@	else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; \
@am__fastdepCC_TRUE@	fi
@AMDEP_TRUE@@am__fastdepCC_FALSE@	@source='$<' object='$@' libtool=no @AMDEPBACKSLASH@
@AMDEP_TRUE@@am__fastdepCC_FALSE@	depfile='$(DEPDIR)/$*.Po' tmpdepfile='$(DEPDIR)/$*.TPo' @AMDEPBACKSLASH@
@AMDEP_TRUE@@am__fastdepCC_FALSE@	$(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
@am__fastdepCC_FALSE@	$(LTCOMPILE) -c `if test -f '$<'; then $(CYGPATH_W) '$<'; else $(CYGPATH_W) '$(srcdir)/$<'; fi`

.c.lo:
			@echo "Building $@"
@am__fastdepCC_TRUE@	@if $(LTCOMPILE) -MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" \
@am__fastdepCC_TRUE@	  -c -o $@ `test -f '$<' || echo '$(srcdir)/'`$<; \
@am__fastdepCC_TRUE@	then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Plo"; \
@am__fastdepCC_TRUE@	else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; \
@am__fastdepCC_TRUE@	fi
@AMDEP_TRUE@@am__fastdepCC_FALSE@	@source='$<' object='$@' libtool=yes @AMDEPBACKSLASH@
@AMDEP_TRUE@@am__fastdepCC_FALSE@	depfile='$(DEPDIR)/$*.Plo' tmpdepfile='$(DEPDIR)/$*.TPlo' @AMDEPBACKSLASH@
@AMDEP_TRUE@@am__fastdepCC_FALSE@	$(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
@am__fastdepCC_FALSE@	$(LTCOMPILE) -c -o $@ `test -f '$<' || echo '$(srcdir)/'`$<


install-pkglibLTLIBRARIES: $(pkglib_LTLIBRARIES)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(pkglibdir)
	@list='ls .libs/*.so'; for p in $$list; do \
	  if test -f $$p; then \
	    f="`echo $$p | sed -e 's|^.*/||'`"; \
	    echo " $(LIBTOOL) --mode=install $(pkglibLTLIBRARIES_INSTALL) $(INSTALL_STRIP_FLAG) $$p $(DESTDIR)$(pkglibdir)/$$f"; \
	    $(LIBTOOL) --mode=install $(pkglibLTLIBRARIES_INSTALL) $(INSTALL_STRIP_FLAG) $$p $(DESTDIR)$(pkglibdir)/$$f; \
	  else :; fi; \
	done


pkglibdir = $(prefix)/modules
datadir = $(prefix)/data