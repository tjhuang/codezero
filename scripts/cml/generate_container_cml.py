#! /usr/bin/env python2.6
# -*- mode: python; coding: utf-8; -*-
#
#  Codezero -- a microkernel for embedded systems.
#
#  Copyright © 2009  B Labs Ltd
#
import os, sys, shelve, glob
from os.path import join
from string import Template

PROJRELROOT = '../../'

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), PROJRELROOT)))
sys.path.append(os.path.abspath("../"))

from config.projpaths import *
from config.configuration import *

containers_menu = \
'''
menu containers_menu
	CAPABILITIES
'''

containers_constraint = \
'''
unless CONTAINERS > %d suppress cont%d_menu
'''

def add_container_constraint(cid):
    cml_string = ""
    if cid == 0:
        return ""
    cml_string = containers_constraint % (cid, cid)
    return cml_string

device_suppress_rule = \
'''
when CONT${CONTID}_CAP_DEVICE_${DEVNAME}_USE == y suppress
'''

device_suppress_sym = \
'''\tcont${CONTID}_cap_device_${DEVNAME_LOWER}
'''

devices = ['UART1', 'UART2', 'UART3', 'TIMER1']

#
# When a symbol is used by a single container, sometimes it is
# necessary to hide it in other containers. This cannot be
# achieved statically but rather needs to be autogenerated
# depending on the number of containers used.
#
def generate_container_suppress_rules(ncont):
    finalstr = ''
    # For each device on the platform
    for devname in devices:
        # Generate rule for each container
        for cont in range(ncont):
            # Create string templates
            rule_templ = Template(device_suppress_rule)
            sym_templ = Template(device_suppress_sym)

            rulestr = rule_templ.substitute(CONTID = cont, DEVNAME = devname)
            symstr = ''
            # Fill for each container
            for other_cont in range(ncont):
                if other_cont == cont:
                    continue
                symstr += sym_templ.substitute(CONTID = other_cont, DEVNAME_LOWER = devname.lower())
            finalstr += rulestr + symstr + "\n"
    return finalstr

def generate_container_cml(arch, ncont):
    print "Autogenerating new rule file"
    fbody = ""
    with open(join(CML2_CONFIG_SRCDIR, arch + '.ruleset')) as in_ruleset:
        fbody += in_ruleset.read()

    # Add container visibility constraint
    for cont in range(ncont):
        fbody += add_container_constraint(cont)

    # Generate the containers menu with as many entries as containers
    fbody += containers_menu
    for cont in range(ncont):
        fbody += '\tcont%d_menu\n' % cont

    # Generate inter-container suppression rules for as many rules as containers
    fbody += generate_container_suppress_rules(ncont)

    # Write each container's rules
    for cont in range(ncont):
        with open(CML2_CONT_DEFFILE, "rU") as contdefs:
            defbody = contdefs.read()
            defbody = defbody.replace("%\n", "%%\n")
            fbody += defbody % { 'cn' : cont }

    # Write the result to output rules file.
    with open(CML2_AUTOGEN_RULES, "w+") as out_cml:
        out_cml.write(fbody)
    return CML2_AUTOGEN_RULES

if __name__ == "__main__":
    generate_container_cml('arm', 4)

