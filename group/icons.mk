# 
# icons.mk
#
# mobbler, a last.fm mobile scrobbler for Symbian smartphones.
# Copyright (C) 2008  Michael Coffey
# 
# http://code.google.com/p/mobbler
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
# 

ifeq (WINS,$(findstring WINS, $(PLATFORM)))
ZDIR=$(EPOCROOT)epoc32\release\$(PLATFORM)\$(CFG)\Z
else
ZDIR=$(EPOCROOT)epoc32\data\z
endif

TARGETDIR=$(ZDIR)\resource\apps
ICONTARGETFILENAME=$(TARGETDIR)\mobbler.mif

ICONDIR=..\gfx

HEADERDIR=$(EPOCROOT)epoc32\include
HEADERFILENAME=$(HEADERDIR)\mobbler.mbg

do_nothing :
	@rem do_nothing

MAKMAKE : do_nothing

BLD : do_nothing

CLEAN : do_nothing

LIB : do_nothing

CLEANLIB : do_nothing

RESOURCE :

	mifconv $(ICONTARGETFILENAME) /X /h$(HEADERFILENAME) \
		/c32,8 $(ICONDIR)\mobbler.svg \
		$(ICONDIR)\more.svg \
		$(ICONDIR)\love.svg \
		$(ICONDIR)\ban.svg \
		$(ICONDIR)\stop.svg \
		$(ICONDIR)\play.svg \
		$(ICONDIR)\next.svg \
		$(ICONDIR)\speaker_low.svg \
		$(ICONDIR)\speaker_high.svg
		
FREEZE : do_nothing

SAVESPACE : do_nothing

RELEASABLES :
	@echo $(ICONTARGETFILENAME)

FINAL : do_nothing

