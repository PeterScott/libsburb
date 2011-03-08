# -*-Python-*-

import os

env = DefaultEnvironment(CCFLAGS='-g -Wall -std=gnu99 -DDEBUG',
                         CPPPATH='.:'+os.environ['C_INCLUDE_PATH'],
                         LIBPATH='.:'+os.environ['LIBRARY_PATH'])

cfiles = '''
memodict.c weft.c patch.c extensible_vectors.c vector_weave.c waitset.c util.c
'''

#Library('sburb', Split(cfiles))
Program('sburb', Split(cfiles), LIBS=['Judy', 'm'])

# Build TAGS file with etags
Command('TAGS', Split(cfiles), "etags $SOURCES") 
