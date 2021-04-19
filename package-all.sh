#!/usr/bin/env bash

#    Copyright (C) 2019-2021 Joshua Boudreau <jboudreau@45drives.com>
# 
#    This file is part of cephgeorep.
# 
#    cephgeorep is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 2 of the License, or
#    (at your option) any later version.
# 
#    cephgeorep is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
# 
#    You should have received a copy of the GNU General Public License
#    along with cephgeorep.  If not, see <https://www.gnu.org/licenses/>.

./package-ubuntu20.sh && ./package-ubuntu18.sh && ./package-el7.sh && ./package-el8.sh

echo Packages are in dist/
