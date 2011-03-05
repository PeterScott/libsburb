# -*-Python-*-

import os

env = DefaultEnvironment(CCFLAGS='-g -Wall -std=gnu99 -pedantic -DDEBUG',
                         CPPPATH='.:'+os.environ['C_INCLUDE_PATH'],
                         LIBPATH='.:'+os.environ['LIBRARY_PATH'])

#Library('sburb', Split('memodict.c weft.c'))
Program('sburb', Split('''memodict.c weft.c patch.c waiting_set.c vector_weave.c waitset.c
util.c'''), 
        LIBS=['Judy', 'm'])
