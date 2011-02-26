# -*-Python-*-

import os

env = DefaultEnvironment(CCFLAGS='-O2 -Wall -std=c99 -pedantic',
                         CPPPATH='.:'+os.environ['C_INCLUDE_PATH'],
                         LIBPATH='.:'+os.environ['LIBRARY_PATH'])

#Library('sburb', Split('memodict.c weft.c'))
Program('sburb', Split('memodict.c weft.c'), LIBS='Judy')
