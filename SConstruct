# -*-Python-*-

import os

env = DefaultEnvironment(CCFLAGS='-O2 -Wall -std=gnu99 -pedantic -DDEBUG',
                         CPPPATH='.:'+os.environ['C_INCLUDE_PATH'],
                         LIBPATH='.:'+os.environ['LIBRARY_PATH'])

#Library('sburb', Split('memodict.c weft.c'))
Program('sburb', Split('memodict.c weft.c patch.c'), LIBS='Judy')
# FIXME: add vector_weave.c back in later.
