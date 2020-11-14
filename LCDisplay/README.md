# LCDisplay

Driver para um display LCD utilizando comunicação I2C em uma Raspberry Pi.

    Autores: Jesuino Vieira Filho e Lucas de Camargo Souza
    Universidade Federal de Santa Catarina (Campus Joinville) - 2018/2
    EMB5632 - Sistemas Operacionais

Para informações mais detalhadas, consulte `docs/relatorio.pdf`.

# Executando

Inicialize o terminal no diretório que contém o projeto e digite para:

    Compilar o módulo:
        $ sudo make
        
    Carregar o módulo no kernel:
        $ sudo make load
    
    Executar o programa de teste:
        $ make run APPLICATION=lcd
    
    Remover os arquivos gerados pelo módulo:
        $ sudo make clean
    
    Descarregar o módulo do kernel:
        $ sudo make unload
