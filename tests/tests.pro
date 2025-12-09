TEMPLATE = subdirs
SUBDIRS += \
    corelogic \
    corelogic_tests

corelogic_tests.depends = corelogic
