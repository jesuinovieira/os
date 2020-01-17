SOBRE
====================================================================

Driver para um display LCD utilizando comunicação I2C em uma Raspberry Pi.

:Autor: Jesuino Vieira Filho
:Autor: Lucas de Camargo Souza

Universidade Federal de Santa Catarina (Campus Joinville) - 2018/2
EMB5632 - Sistemas Operacionais

Para informações mais detalhadas, consulte **Relatório.pdf**.

EXECUTANDO
====================================================================

Inicialize o terminal na pasta que contém os seguintes arquivos e 
digite para:

--------------------------------------------------------------------
Arquivos:
	Makefile
	lcdisplay.c
	lcdisplay.h
	load.sh
	unload.sh
	lcd.c
--------------------------------------------------------------------
Compilar o módulo:
	$ sudo make
--------------------------------------------------------------------
Carregar o módulo no kernel:
	$ sudo make load
--------------------------------------------------------------------
Executar o programa de teste:
	$ make run APPLICATION=lcd
--------------------------------------------------------------------
Remover os arquivos gerados pelo módulo:
	$ sudo make clean
--------------------------------------------------------------------
Descarregar o módulo do kernel:
	$ sudo make unload
