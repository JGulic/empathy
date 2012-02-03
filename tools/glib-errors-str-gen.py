#!/usr/bin/python

import sys
import xml.dom.minidom

from libglibcodegen import NS_TP, get_docstring, xml_escape

class Generator(object):
    def __init__(self, dom, basename):
        self.dom = dom
        self.errors = self.dom.getElementsByTagNameNS(NS_TP, 'errors')[0]
        self.basename = basename

        self.__header = []
        self.__body = []
        self.__docs = []

    def h(self, s):
        if isinstance(s, unicode):
            s = s.encode('utf-8')
        self.__header.append(s)

    def b(self, s):
        if isinstance(s, unicode):
            s = s.encode('utf-8')
        self.__body.append(s)

    def d(self, s):
        if isinstance(s, unicode):
            s = s.encode('utf-8')
        self.__docs.append(s)

    def __call__(self):
        errors = self.errors.getElementsByTagNameNS(NS_TP, 'error')

        self.b('#include <telepathy-glib/errors.h>')
        self.b('')
        self.b('const gchar *')
        self.b('tp_error_get_dbus_name (TpError error)')
        self.b('{')
        self.b('  switch (error)')
        self.b('    {')

        for error in errors:
            ns = error.parentNode.getAttribute('namespace')
            nick = error.getAttribute('name').replace(' ', '')
            uc_nick = error.getAttribute('name').replace(' ', '_').replace('.', '_').upper()
            name = 'TP_ERROR_STR_' + uc_nick
            error_name = '%s.%s' % (ns, nick)

            self.d('/**')
            self.d(' * %s:' % name)
            self.d(' *')
            self.d(' * The D-Bus error name %s' % error_name)
            self.d(' *')
            self.d(' * %s' % xml_escape(get_docstring(error)))
            self.d(' */')
            self.d('')

            self.h('#define %s "%s"' % (name, error_name))

            self.b('      case TP_ERROR_%s:' % uc_nick)
            self.b('        return %s;' % name)

        self.b('      default:')
        self.b('        g_return_val_if_reached (NULL);')
        self.b('    }')
        self.b('}')

        # make both files end with a newline
        self.h('')
        self.b('')

        open(self.basename + '.h', 'w').write('\n'.join(self.__header))
        open(self.basename + '.c', 'w').write('\n'.join(self.__body))
        open(self.basename + '-gtk-doc.h', 'w').write('\n'.join(self.__docs))

if __name__ == '__main__':
    argv = sys.argv[1:]
    basename = argv[0]

    Generator(xml.dom.minidom.parse(argv[1]), basename)()
