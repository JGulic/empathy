# Makefile for program source directory in GNU NLS utilities package.
# Copyright (C) 1995, 1996, 1997 by Ulrich Drepper <drepper@gnu.ai.mit.edu>
# Copyright (C) 2004-2008 Rodney Dawes <dobey.pwns@gmail.com>
#
# This file may be copied and used freely without restrictions.  It may
# be used in projects which are not available under a GNU Public License,
# but which still want to provide support for the GNU gettext functionality.
#
# - Modified by Owen Taylor <otaylor@redhat.com> to use GETTEXT_PACKAGE
#   instead of PACKAGE and to look for po2tbl in ./ not in intl/
#
# - Modified by jacob berkman <jacob@ximian.com> to install
#   Makefile.in.in and po2tbl.sed.in for use with glib-gettextize
#
# - Modified by Rodney Dawes <dobey.pwns@gmail.com> for use with intltool
#
# We have the following line for use by intltoolize:
# INTLTOOL_MAKEFILE

GETTEXT_PACKAGE = empathy
PACKAGE = empathy
VERSION = 3.3.4.20120127

SHELL = /bin/sh

srcdir = .
top_srcdir = ..
top_builddir = ..


prefix = /usr/local
exec_prefix = ${prefix}
datadir = ${datarootdir}
datarootdir = ${prefix}/share
libdir = ${exec_prefix}/lib
DATADIRNAME = share
itlocaledir = $(prefix)/$(DATADIRNAME)/locale
subdir = po
install_sh = ${SHELL} /home/jovanka/my-projects/empathy-git/empathy/install-sh
# Automake >= 1.8 provides /bin/mkdir -p.
# Until it can be supposed, use the safe fallback:
mkdir_p = $(install_sh) -d

INSTALL = /bin/install -c
INSTALL_DATA = ${INSTALL} -m 644

GMSGFMT = /usr/bin/msgfmt
MSGFMT = /usr/bin/msgfmt
XGETTEXT = /usr/bin/xgettext
INTLTOOL_UPDATE = /usr/local/bin/intltool-update
INTLTOOL_EXTRACT = /usr/local/bin/intltool-extract
MSGMERGE = INTLTOOL_EXTRACT=$(INTLTOOL_EXTRACT) srcdir=$(srcdir) $(INTLTOOL_UPDATE) --gettext-package $(GETTEXT_PACKAGE) --dist
GENPOT   = INTLTOOL_EXTRACT=$(INTLTOOL_EXTRACT) srcdir=$(srcdir) $(INTLTOOL_UPDATE) --gettext-package $(GETTEXT_PACKAGE) --pot

ALL_LINGUAS = 

PO_LINGUAS=$(shell if test -r $(srcdir)/LINGUAS; then grep -v "^\#" $(srcdir)/LINGUAS; else echo "$(ALL_LINGUAS)"; fi)

USER_LINGUAS=$(shell if test -n "$(LINGUAS)"; then LLINGUAS="$(LINGUAS)"; ALINGUAS="$(ALL_LINGUAS)"; for lang in $$LLINGUAS; do if test -n "`grep '^$$lang$$' $(srcdir)/LINGUAS 2>/dev/null`" -o -n "`echo $$ALINGUAS|tr ' ' '\n'|grep '^$$lang$$'`"; then printf "$$lang "; fi; done; fi)

USE_LINGUAS=$(shell if test -n "$(USER_LINGUAS)" -o -n "$(LINGUAS)"; then LLINGUAS="$(USER_LINGUAS)"; else if test -n "$(PO_LINGUAS)"; then LLINGUAS="$(PO_LINGUAS)"; else LLINGUAS="$(ALL_LINGUAS)"; fi; fi; for lang in $$LLINGUAS; do printf "$$lang "; done)

POFILES=$(shell LINGUAS="$(PO_LINGUAS)"; for lang in $$LINGUAS; do printf "$$lang.po "; done)

DISTFILES = Makefile.in.in POTFILES.in $(POFILES)
EXTRA_DISTFILES = ChangeLog POTFILES.skip Makevars LINGUAS

POTFILES = \
	../data/empathy.desktop.in.in \
	../data/org.gnome.Empathy.gschema.xml.in \
	../data/empathy-accounts.desktop.in.in \
	../libempathy/empathy-ft-handler.c \
	../libempathy/empathy-message.c \
	../libempathy/empathy-tp-contact-list.c \
	../libempathy/empathy-utils.c \
	../libempathy/empathy-time.c \
	../libempathy-gtk/empathy-account-chooser.c \
	../libempathy-gtk/empathy-account-widget.c \
	../libempathy-gtk/empathy-account-widget-aim.ui \
	../libempathy-gtk/empathy-account-widget-generic.ui \
	../libempathy-gtk/empathy-account-widget-groupwise.ui \
	../libempathy-gtk/empathy-account-widget-icq.ui \
	../libempathy-gtk/empathy-account-widget-irc.c \
	../libempathy-gtk/empathy-account-widget-sip.c \
	../libempathy-gtk/empathy-account-widget-irc.ui \
	../libempathy-gtk/empathy-account-widget-jabber.ui \
	../libempathy-gtk/empathy-account-widget-msn.ui \
	../libempathy-gtk/empathy-account-widget-local-xmpp.ui \
	../libempathy-gtk/empathy-account-widget-sip.ui \
	../libempathy-gtk/empathy-account-widget-yahoo.ui \
	../libempathy-gtk/empathy-avatar-chooser.c \
	../libempathy-gtk/empathy-avatar-image.c \
	../libempathy-gtk/empathy-call-utils.c \
	../libempathy-gtk/empathy-chat.c \
	../libempathy-gtk/empathy-chat.ui \
	../libempathy-gtk/empathy-contact-blocking-dialog.c \
	../libempathy-gtk/empathy-contact-blocking-dialog.ui \
	../libempathy-gtk/empathy-chat-text-view.c \
	../libempathy-gtk/empathy-contact-dialogs.c \
	../libempathy-gtk/empathy-contact-dialogs.ui \
	../libempathy-gtk/empathy-contact-search-dialog.c \
	../libempathy-gtk/empathy-contact-widget.c \
	../libempathy-gtk/empathy-contactinfo-utils.c \
	../libempathy-gtk/empathy-contact-widget.ui \
	../libempathy-gtk/empathy-groups-widget.c \
	../libempathy-gtk/empathy-individual-dialogs.c \
	../libempathy-gtk/empathy-individual-edit-dialog.c \
	../libempathy-gtk/empathy-individual-information-dialog.c \
	../libempathy-gtk/empathy-individual-linker.c \
	../libempathy-gtk/empathy-individual-menu.c \
	../libempathy-gtk/empathy-individual-view.c \
	../libempathy-gtk/empathy-individual-widget.c \
	../libempathy-gtk/empathy-individual-widget.ui \
	../libempathy-gtk/empathy-irc-network-chooser-dialog.c \
	../libempathy-gtk/empathy-irc-network-dialog.c \
	../libempathy-gtk/empathy-linking-dialog.c \
	../libempathy-gtk/empathy-log-window.c \
	../libempathy-gtk/empathy-log-window.ui \
	../libempathy-gtk/empathy-new-message-dialog.c \
	../libempathy-gtk/empathy-new-call-dialog.c \
	../libempathy-gtk/empathy-bad-password-dialog.c \
	../libempathy-gtk/empathy-base-password-dialog.c \
	../libempathy-gtk/empathy-password-dialog.c \
	../libempathy-gtk/empathy-presence-chooser.c \
	../libempathy-gtk/empathy-protocol-chooser.c \
	../libempathy-gtk/empathy-search-bar.ui \
	../libempathy-gtk/empathy-sound-manager.c \
	../libempathy-gtk/empathy-status-preset-dialog.c \
	../libempathy-gtk/empathy-status-preset-dialog.ui \
	../libempathy-gtk/empathy-theme-adium.c \
	../libempathy-gtk/empathy-theme-boxes.c \
	../libempathy-gtk/empathy-theme-irc.c \
	../libempathy-gtk/empathy-theme-manager.c \
	../libempathy-gtk/empathy-tls-dialog.c \
	../libempathy-gtk/empathy-ui-utils.c \
	../libempathy-gtk/totem-subtitle-encoding.c \
	../libempathy-gtk/empathy-webkit-utils.c \
	../nautilus-sendto-plugin/empathy-nautilus-sendto.c \
	../src/empathy.c \
	../src/empathy-about-dialog.c \
	../src/empathy-account-assistant.c \
	../src/empathy-accounts-dialog.c \
	../src/empathy-accounts-dialog.ui \
	../src/empathy-auth-client.c \
	../src/empathy-auto-salut-account-helper.c \
	../src/empathy-av.c \
	../src/empathy-streamed-media-window.c \
	../src/empathy-call-window.ui \
	../src/empathy-chat-window.c \
	../src/empathy-chat-window.ui \
	../src/empathy-chatrooms-window.c \
	../src/empathy-chatrooms-window.ui \
	../src/empathy-event-manager.c \
	../src/empathy-ft-manager.c \
	../src/empathy-ft-manager.ui \
	../src/empathy-import-dialog.c \
	../src/empathy-import-widget.c \
	../src/empathy-import-dialog.ui \
	../src/empathy-import-widget.c \
	../src/empathy-roster-window.c \
	../src/empathy-roster-window.ui \
	../src/empathy-roster-window-menubar.ui \
	../src/empathy-new-chatroom-dialog.c \
	../src/empathy-new-chatroom-dialog.ui \
	../src/empathy-preferences.c \
	../src/empathy-preferences.ui \
	../src/empathy-status-icon.c \
	../src/empathy-status-icon.ui \
	../src/empathy-streamed-media-window.ui \
	../src/empathy-call-window-fullscreen.ui \
	../src/empathy-map-view.c \
	../src/empathy-map-view.ui \
	../src/empathy-debug-window.c \
	../src/empathy-invite-participant-dialog.c \
	../src/empathy-accounts.c \
	../src/empathy-av.c \
	../src/empathy-debugger.c \
	../src/empathy-chat.c \
	../src/empathy-notifications-approver.c \
	../src/empathy-call-observer.c \
	../src/empathy-call-window.c \
	../src/empathy-call.c \
	../libempathy-gtk/empathy-search-bar.c \
	../libempathy-gtk/empathy-new-account-dialog.c

CATALOGS=$(shell LINGUAS="$(USE_LINGUAS)"; for lang in $$LINGUAS; do printf "$$lang.gmo "; done)

.SUFFIXES:
.SUFFIXES: .po .pox .gmo .mo .msg .cat

.po.pox:
	$(MAKE) $(GETTEXT_PACKAGE).pot
	$(MSGMERGE) $< $(GETTEXT_PACKAGE).pot -o $*.pox

.po.mo:
	$(MSGFMT) -o $@ $<

.po.gmo:
	file=`echo $* | sed 's,.*/,,'`.gmo \
	  && rm -f $$file && $(GMSGFMT) -o $$file $<

.po.cat:
	sed -f ../intl/po2msg.sed < $< > $*.msg \
	  && rm -f $@ && gencat $@ $*.msg


all: all-yes

all-yes: $(CATALOGS)
all-no:

$(GETTEXT_PACKAGE).pot: $(POTFILES)
	$(GENPOT)

install: install-data
install-data: install-data-yes
install-data-no: all
install-data-yes: all
	linguas="$(USE_LINGUAS)"; \
	for lang in $$linguas; do \
	  dir=$(DESTDIR)$(itlocaledir)/$$lang/LC_MESSAGES; \
	  $(mkdir_p) $$dir; \
	  if test -r $$lang.gmo; then \
	    $(INSTALL_DATA) $$lang.gmo $$dir/$(GETTEXT_PACKAGE).mo; \
	    echo "installing $$lang.gmo as $$dir/$(GETTEXT_PACKAGE).mo"; \
	  else \
	    $(INSTALL_DATA) $(srcdir)/$$lang.gmo $$dir/$(GETTEXT_PACKAGE).mo; \
	    echo "installing $(srcdir)/$$lang.gmo as" \
		 "$$dir/$(GETTEXT_PACKAGE).mo"; \
	  fi; \
	  if test -r $$lang.gmo.m; then \
	    $(INSTALL_DATA) $$lang.gmo.m $$dir/$(GETTEXT_PACKAGE).mo.m; \
	    echo "installing $$lang.gmo.m as $$dir/$(GETTEXT_PACKAGE).mo.m"; \
	  else \
	    if test -r $(srcdir)/$$lang.gmo.m ; then \
	      $(INSTALL_DATA) $(srcdir)/$$lang.gmo.m \
		$$dir/$(GETTEXT_PACKAGE).mo.m; \
	      echo "installing $(srcdir)/$$lang.gmo.m as" \
		   "$$dir/$(GETTEXT_PACKAGE).mo.m"; \
	    else \
	      true; \
	    fi; \
	  fi; \
	done

# Empty stubs to satisfy archaic automake needs
dvi info tags TAGS ID:

# Define this as empty until I found a useful application.
install-exec installcheck:

uninstall:
	linguas="$(USE_LINGUAS)"; \
	for lang in $$linguas; do \
	  rm -f $(DESTDIR)$(itlocaledir)/$$lang/LC_MESSAGES/$(GETTEXT_PACKAGE).mo; \
	  rm -f $(DESTDIR)$(itlocaledir)/$$lang/LC_MESSAGES/$(GETTEXT_PACKAGE).mo.m; \
	done

check: all $(GETTEXT_PACKAGE).pot
	rm -f missing notexist
	srcdir=$(srcdir) $(INTLTOOL_UPDATE) -m
	if [ -r missing -o -r notexist ]; then \
	  exit 1; \
	fi

mostlyclean:
	rm -f *.pox $(GETTEXT_PACKAGE).pot *.old.po cat-id-tbl.tmp
	rm -f .intltool-merge-cache

clean: mostlyclean

distclean: clean
	rm -f Makefile Makefile.in POTFILES stamp-it
	rm -f *.mo *.msg *.cat *.cat.m *.gmo

maintainer-clean: distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."
	rm -f Makefile.in.in

distdir = ../$(PACKAGE)-$(VERSION)/$(subdir)
dist distdir: $(DISTFILES)
	dists="$(DISTFILES)"; \
	extra_dists="$(EXTRA_DISTFILES)"; \
	for file in $$extra_dists; do \
	  test -f $(srcdir)/$$file && dists="$$dists $(srcdir)/$$file"; \
	done; \
	for file in $$dists; do \
	  test -f $$file || file="$(srcdir)/$$file"; \
	  ln $$file $(distdir) 2> /dev/null \
	    || cp -p $$file $(distdir); \
	done

update-po: Makefile
	$(MAKE) $(GETTEXT_PACKAGE).pot
	tmpdir=`pwd`; \
	linguas="$(USE_LINGUAS)"; \
	for lang in $$linguas; do \
	  echo "$$lang:"; \
	  result="`$(MSGMERGE) -o $$tmpdir/$$lang.new.po $$lang`"; \
	  if $$result; then \
	    if cmp $(srcdir)/$$lang.po $$tmpdir/$$lang.new.po >/dev/null 2>&1; then \
	      rm -f $$tmpdir/$$lang.new.po; \
            else \
	      if mv -f $$tmpdir/$$lang.new.po $$lang.po; then \
	        :; \
	      else \
	        echo "msgmerge for $$lang.po failed: cannot move $$tmpdir/$$lang.new.po to $$lang.po" 1>&2; \
	        rm -f $$tmpdir/$$lang.new.po; \
	        exit 1; \
	      fi; \
	    fi; \
	  else \
	    echo "msgmerge for $$lang.gmo failed!"; \
	    rm -f $$tmpdir/$$lang.new.po; \
	  fi; \
	done

Makefile POTFILES: stamp-it
	@if test ! -f $@; then \
	  rm -f stamp-it; \
	  $(MAKE) stamp-it; \
	fi

stamp-it: Makefile.in.in $(top_builddir)/config.status POTFILES.in
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/Makefile.in CONFIG_HEADERS= CONFIG_LINKS= \
	       $(SHELL) ./config.status

# Tell versions [3.59,3.63) of GNU make not to export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
