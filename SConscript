import os
from building import *

objs = []

if GetDepend('PKG_USING_MICRO_ROS_RTTHREAD_PACKAGE'):
    cwd = GetCurrentDir()
    for d in os.listdir(cwd):
        path = os.path.join(cwd, d)
        if os.path.isfile(os.path.join(path, 'SConscript')):
            objs = objs + SConscript(os.path.join(d, 'SConscript'))

Return('objs')