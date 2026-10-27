"""Check that the bytecode report splits tests into sources and settings as hyptest does."""
from pathlib import Path
import tempfile
import unittest

from ci.bytecode_report import read_test


class ReadTestTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def write(self, name, content):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(content.encode('utf8'))
        return path

    def test_single_source_with_settings(self):
        path = self.write('test.hyp', (
            'contract C {}\n'
            '// ====\n'
            '// compileViaYul: false\n'
            '// revertStrings: debug\n'
            '// ----\n'
            '// f() -> 1\n'
        ))
        self.assertEqual(read_test(path), (
            {'': 'contract C {}\n'},
            {'compileViaYul': 'false', 'revertStrings': 'debug'},
        ))

    def test_last_line_without_newline(self):
        path = self.write('test.hyp', 'contract C {}')
        self.assertEqual(read_test(path), ({'': 'contract C {}\n'}, {}))

    def test_named_and_external_sources(self):
        self.write('_external/sub/a.hyp', 'contract A {}\n')
        self.write('_external/lib.hyp=hyp', 'library L {}\n')
        path = self.write('test.hyp', (
            '==== ExternalSource: _external/sub/a.hyp ====\n'
            '==== ExternalSource: lib=_external/lib.hyp=hyp ====\n'
            '==== Source: s1.hyp ====\n'
            'import "lib";\n'
            '==== Source: s2.hyp ====\n'
            'import "s1.hyp";\n'
            '// ----\n'
            '// f() -> 1\n'
        ))
        self.assertEqual(read_test(path), (
            {
                '_external/sub/a.hyp': 'contract A {}\n',
                'lib': 'library L {}\n',
                's1.hyp': 'import "lib";\n',
                's2.hyp': 'import "s1.hyp";\n',
            },
            {},
        ))


if __name__ == '__main__':
    unittest.main()
