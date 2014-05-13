STM32F3 Gizmo
===================================

Disclaimer
----------

This software is provided "as is," and you use the software at your own risk. The author make no warranties as to performance, merchantability, fitness for a particular purpose, or any other warranties whether expressed or implied. No oral or written communication from the author shall create a warranty. Under no circumstances shall the author be liable for direct, indirect, special, incidental, or consequential damages resulting from the use, misuse, or inability to use this software, even if the author has been advised of the possibility of such damages. 

Compiling the code
------------------

The F3 Gizmo has been built using Yagarto in an Eclipse Juno IDE
but as it is a makefile project it should compile without Eclipse.

It is compiled against ChibiOS/RT version 2.6.2
You can find its sources at sourceforge:
http://sourceforge.net/projects/chibios/

The makefile line:
CHIBIOS = ../../ChibiOS_2.6.2
Should point to the current ChibiOS source location
before calling make


Installing from binary format
-----------------------------

The Binary directory contains the .bin install file
Read the PDF manual in the Doc directory for instructions about how to install the firmware on the STM32F3 Discovery board


