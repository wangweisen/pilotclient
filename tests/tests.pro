load(common_pre)

REQUIRES += contains(BLACK_CONFIG,Unittests)

TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += testblackmisc
SUBDIRS += testblackcore
SUBDIRS += testblackgui

testblackmisc.file = blackmisc/testblackmisc.pro
testblackcore.file = blackcore/testblackcore.pro
testblackgui.file = blackgui/testblackgui.pro

load(common_post)
