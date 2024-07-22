from bit import PrivateKey, network
from time import sleep
import sys
import requests

def transferir(wif, destino):

    my_key = PrivateKey(wif)
    print('---------------------------------------------------\nEndereço da Carteira Capturada: ', my_key.address, '\n---------------------------------------------------')
    saldo = my_key.balance_as('satoshi')
    print('---------------------------------------------------\nSaldo da Carteira Capturada: ', saldo, '\n---------------------------------------------------')
    if destino == 'Não Informado':
        print('\nTransferencia não informada, endereço não informado.\n')
        return None
    taxa = network.get_fee('fast') 
    print(f'Taxa de Transação Sugerida (satoshis por byte): {taxa}')
    taxa *= 2
    taxa = int(taxa) * 250
    print(f'---------- >> --------- >> Taxa a ser utilizada: ', taxa)
    valor = int(saldo)-taxa
    valor = int(valor)
    if valor < 0:
        print(f'\n-----------------\nSaldo nao possibilita transação: {saldo} satoshis\n ------------------')
        return None
    print(f'Valor a ser transferido: {valor} satoshis')
    print ('Iniciando a transferencia...')
    try:
        tx_hash = my_key.send([(destino, valor, "satoshi")], fee=taxa)
        if tx_hash:
            print('\n\nTransação Enviada com Sucesso !!!! ---- \n--- Hash da transação: ',tx_hash, '\n---')
            return tx_hash
        else:
            print('\n---------------Falha ao transferir, tente manualmente.---------------')
            return None
    except:
        print('\n---------------Falha ao transferir, tente manualmente.---------------')
        return None
    
def verifica_saldo(endereco):
    if endereco == '18ZMbwUFLMHoZBbfpCjUJQTCMCbktshgpe':
        return None
    print('Verificando saldo da carteira, aguarde...')
    saldo = network.NetworkAPI.get_balance(endereco) / 1e8

    if saldo > 0:
        print(f"\n--------------- Carteira: {endereco}: \nSaldo: {saldo:.8f} BTC---------------")
        return

    else:
        if input("Carteira sem saldo, deseja continuar? (s/n): ") in ['sim','s','y','yes']:
            return
        else:
            print('Encerrando Bot')
            quit()

def verifica_saldo_destino(endereco):
    for x in range(50):
        sys.stdout.write(f"\rVerificando Saldo da Carteira Destino... Tentativa {x +1}... / 50 ")
        sys.stdout.flush()
        transacoes = network.NetworkAPI.get_transactions(address=endereco)
        saldo = network.NetworkAPI.get_balance(endereco) / 1e8
        
        if saldo > 0:
            print(f"\n---------------Valor recebido com sucesso: {saldo:.8f} BTC---------------")
            return
        
        if transacoes:
            print(f'\nExiste Transferencia, saldo atual: {saldo:.8f} BTC\nID: {transacoes}')
        else:
            print(f'\nNão existe transações em andamento, saldo atual: {saldo:.8f} BTC')

        sleep(5)

def monitorar_mempool(address):
    while True:
        try:
            url = f'https://blockchain.info/q/pubkeyaddr/{address}'
            response = requests.get(url)
            print(f'Procurando Chave Pública...\nResposta API: {response}')
            if response.status_code == 200:
                chave = response.text
                print ('\n\n------------------------------------------------------------------------------------'
                    f'\n\nChave Pública: {chave}'
                    '\n\n------------------------------------------------------------------------------------\n')
                return chave 
                   
            print('Não encontrada, tentando novamente em 5 segundos...')
            sleep(5)

        except Exception as e:
            print (f"\n{e}\nOcorreu uma Excessão ao chamar a API"
                   "\nSe ocorrer varias vezes verifique se o endereço fornecido está correto"
                   "\nTentando novamente em 5 segundos")
            sleep(5)


