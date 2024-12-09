import random
import time
import multiprocessing
import secp256k1
import hashlib
import base58

# Endereço de destino fornecido pelo desafio
target_address = "1Hoyt6UBzwL5vvUSTLMQC2mwvvE5PpeSC"

# Chave privada parcial fornecida
partial_key = "4x3x3x4xcxfx6x9xfx3xaxcx5x0x4xbxbx7x2x6x8x7x8xax4x0x8x3x3x3x7x3x"    

# Contador global de verificações e tempo de início
total_hashes_checked = 0
start_time = time.time()

def generate_random_key(partial_key):
    """Gera uma chave privada substituindo os 'x' por números hexadecimais aleatórios."""
    hex_chars = "0123456789abcdef"
    key_list = list(partial_key)
    
    for i, char in enumerate(key_list):
        if char == 'x':
            # Substitui o 'x' por um valor aleatório hexadecimal
            key_list[i] = random.choice(hex_chars)
    
    return "".join(key_list)

def check_key(private_key_hex):
    """Verifica se a chave privada corresponde ao endereço de destino."""
    try: 
        priv_key = bytes.fromhex(private_key_hex)
        private_key = secp256k1.PrivateKey(priv_key)
        public_key = private_key.pubkey
        pub_key_hash = hashlib.new('ripemd160', hashlib.sha256(public_key.serialize()).digest()).digest()
        network_prefix = b'\x00' + pub_key_hash
        checksum = hashlib.sha256(hashlib.sha256(network_prefix).digest()).digest()[:4]
        address_bytes = network_prefix + checksum
        address = base58.b58encode(address_bytes).decode('utf-8')
        return address == target_address
    except Exception as e:
        print(f"Erro na verificação: {e}")
        return False


def brute_force_worker(partial_key, worker_id):
    """Executa o brute-force em uma thread para tentar encontrar a chave privada."""
    hashes_checked = 0
    start_time_worker = time.time()

    while True:
        private_key = generate_random_key(partial_key)
        if check_key(private_key):
            print(f"\nChave privada encontrada: {private_key}")
            break  # Sai do loop quando a chave for encontrada
        
        hashes_checked += 1
        
        # A cada 50000 hashes, mede a velocidade
        if hashes_checked % 50000 == 0:
            elapsed_time = time.time() - start_time_worker
            speed = 50000 / elapsed_time if elapsed_time > 0 else 0
            print(f"Worker {worker_id + 1} - Hashes por segundo: {speed:.2f} hashes/s, lastKey = {private_key}")
            start_time_worker = time.time()  # Reseta o tempo


def start_bruteforce_processes(partial_key, num_processes=8):
    """Inicia as threads de brute-force para trabalhar em paralelo."""
    processes = []
    
    for i in range(num_processes):
        process = multiprocessing.Process(target=brute_force_worker, args=(partial_key, i))
        processes.append(process)
        process.start()
    
    for process in processes:
        process.join()

if __name__ == "__main__":
    # Inicia o brute-force
    start_bruteforce_processes(partial_key)
