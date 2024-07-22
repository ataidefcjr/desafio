import subprocess
import webbrowser
import hashlib
import base58
import os
import re
import sys
import time

range_inicial = ''
range_final = ''
def ajuste_range(ajuste:bool):
    global range_inicial
    global range_final
    if ajuste:
        print('Informe o range da chave privada da carteira informada.')
        range_inicial = input('Digite o Range Inicial (Carteira 66 = 20000000000000000): ')
        range_final = input('Digite o Range Fianl (Carteira 66 = 3ffffffffffffffff): ')
    else:
        range_inicial = '10000000000000000' 
        range_final = '1ffffffffffffffff'

def check_pollard():
    if not os.path.exists('Kangaroo.exe'):
        print('//\n//                "Kangaroo.exe" não encontrado.')
        print(f'''
//                Baixe o Kangaroo executavel em https://github.com/JeanLucPons/Kangaroo/releases/tag/2.2 
//                e salve no diretório do bot: {os.getcwd()}
//                Ou compile por sua conta.
//''')
        print('Finalizando Script')
        quit()

def quebrar_pollard(chave_publica, gpu):
    caminho_relativo = 'Kangaroo.exe'
    caminho_completo = os.path.abspath(caminho_relativo)
    caminho_completo_aspas = f'"{caminho_completo}"'
    script = [f"{range_inicial}\n", f"{range_final}\n", chave_publica]
    if gpu:
        gpu = ' -gpu'
    else:
        gpu = ''
    start = f'{caminho_completo_aspas}{gpu} -o KFound.txt in.txt'
    print(start)
    print(script)
    with open('in.txt', 'w') as i:
        i.writelines(script)
    time.sleep(1)
    subprocess.run(start, shell=True)

def converter_wif(address, private_key_hex: str) -> str:
    private_key_hex.lower()
    # Remove o prefixo 0x se ele estiver presente
    if private_key_hex.startswith('0x'):
        private_key_hex = private_key_hex[2:]

    private_key_hex = private_key_hex.zfill(64)

    # Adiciona o prefixo 0x80 para a mainnet
    prefix = b'\x80'
    private_key_bytes = bytes.fromhex(private_key_hex)
    # Adiciona o sufixo 0x01 para indicar que é uma chave comprimida
    compressed_suffix = b'\x01'
    extended_key = prefix + private_key_bytes + compressed_suffix
    
    # Realiza o SHA-256 duplo do extended_key
    first_sha256 = hashlib.sha256(extended_key).digest()
    second_sha256 = hashlib.sha256(first_sha256).digest()
    
    # Adiciona os 4 primeiros bytes do segundo SHA-256 ao final do extended_key
    checksum = second_sha256[:4]
    final_key = extended_key + checksum
    
    # Converte para base58
    wif_compressed = base58.b58encode(final_key)
    with open('WIF.txt', 'a') as wif:
        wif.write(f'Endereço: {address}\n WIF: ' + wif_compressed.decode('utf-8') + '\n\n')
    
    return wif_compressed.decode('utf-8')

def aguarda_quebra(segundos: int): #Apos chamar o quebrar chave, fica procurando a key no arquivo KFound.txt na raiz
    found = 'Found.txt'
    kfound = 'KFound.txt'
    time.sleep(1)
    for x in range(segundos):
        sys.stdout.write(f"\rEsperando Quebra da Chave... {x + 1}... / {segundos}\n")
        sys.stdout.flush()
        if os.path.exists(kfound):
            with open(kfound, "r") as file:
                content = file.read()
                match = re.search(r'Priv: (\w+)', content)
                if match:
                    return match.group(1)
        time.sleep(1)

    print('\nVerifique se houve erro, Arquivo não encontrado.')
    if input("Tentar novamente? (s/n): ").lower() in ['s', 'sim', 'y', 'yes']:
        x = int(input("Digite quantos segundos quer aguardar: "))
        aguarda_quebra(x)
    else:
        chave_privada = input("Insira a chave privada: ")
        return chave_privada
