from transactions import transferir, verifica_saldo, monitorar_mempool, verifica_saldo_destino
from hashs import converter_wif, aguarda_quebra, quebrar_pollard, check_pollard, ajuste_range
from cleanup import limpeza, interrompido
import time
import signal
import atexit

sim = ['s', 'sim', 'y', 'yes']


def main(address, destino, gpu:bool):
    signal.signal(signal.SIGINT, interrompido)
    
    ########## 1 --- Capturar a Chave Pública ---
        
    if input('Deseja inserir a chave pública manualmente? (s/n): ').lower() in sim:
        chave_publica = input("Insira a chave pública: ")
    else:
        chave_publica = monitorar_mempool(address)
        
    ########## 2 --- Quebrar Chave Pública na Privada ---
    inicio = time.time()
    if chave_publica:
        quebrar_pollard(chave_publica, gpu)
    else:
        print('Chave Pública não encontrada, terminando script')
        return

    ########## 3 - Verifica se foi quebrada
    private_key = aguarda_quebra(20)
    if private_key:
        print(f"------\nPrivate Key HEX: {private_key} \n------")    
        private_key = converter_wif(address, private_key) # Converte pro formato WIF
        fim = time.time()
        print (f'''\n------------------------------------------------------------------------------------------------------------------------
                \n---------------------------------Private Key WIF Compressed: {private_key}
                \n---------------------------------Minha Carteira = {destino}
                \n------------------------------------------------------------------------------------------------------------------------\n''')
    else:
        print('Não encontrada chave privada, terminando script')
        return
    
    ########## 4 - Inicia a Transferencia
    tx = transferir(private_key, destino)
    if tx:
        print('Verificando Saldo da sua carteira')
        verifica_saldo_destino(destino)

    print(f'A Chave Pública foi quebrada em: {(fim-inicio):.2f} segundos' )
    atexit.register(limpeza)


if __name__ == '__main__':
    limpeza()
    # check_pollard()
    
    print('---\nIniciando Script, Atenção\nEsse script busca a chave pública da carteira "alvo" e usa GPU pra quebrar a chave pública na privada, converte em WIF e tenta realizar uma transferencia para sua conta, o código ainda está em desenvolvimento, esteja pronto para transferir manualmente caso falhe.\n---\n')
    
    executar_teste = input('Deseja executar o teste na carteira 65? (s/n): ').lower() 
    if executar_teste in sim:
        address = '18ZMbwUFLMHoZBbfpCjUJQTCMCbktshgpe' ##Carteira do Puzzle 65
    else:
        address = input('Insira o endereço da carteira que deseja procurar: ')
    saldo = verifica_saldo(address)

    if address == '18ZMbwUFLMHoZBbfpCjUJQTCMCbktshgpe':
        ajuste_range(False)
    else:
        ajuste_range(True)
            
    if address == '18ZMbwUFLMHoZBbfpCjUJQTCMCbktshgpe':
        destino = ''
    else:
        destino = input('Insira o endereço da sua carteira (ao deixar o campo vazio não tentará transferir): ')
    destino = 'Não Informado' if destino == '' else destino

    if input('Usando Kangaroo, Deseja usar GPU? (s/n): ').lower() in sim:
        usar_gpu = True
    else:
        usar_gpu = False

    main(address, destino, usar_gpu)

