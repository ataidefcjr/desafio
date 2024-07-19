######### Ainda não funcional, verificar lógica, instalar bitcoin-cli e fazer testes ########


import subprocess
import json

def execute_bitcoin_cli_command(command):
    """Execute a bitcoin-cli command and return the output."""
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if result.returncode != 0:
        raise Exception(f"Command failed: {result.stderr}")
    return result.stdout.strip()

def get_first_utxo():
    """Get the first available UTXO from the wallet."""
    utxos = json.loads(execute_bitcoin_cli_command(['bitcoin-cli', 'listunspent']))
    if not utxos:
        raise Exception("No UTXOs available")
    return utxos[0]

def create_raw_transaction(txid, vout, address, amount, fee):
    input_utxos = [{'txid': txid, 'vout': vout}]
    
    outputs = {address: amount - fee}
    
    raw_tx = execute_bitcoin_cli_command(['bitcoin-cli', 'createrawtransaction', json.dumps(input_utxos), json.dumps(outputs)])
    return raw_tx

def sign_transaction(raw_tx, private_key):
    signed_tx = json.loads(execute_bitcoin_cli_command(['bitcoin-cli', 'signrawtransactionwithkey', raw_tx, json.dumps([private_key])]))
    if not signed_tx.get("complete", False):
        raise Exception("Transaction signing failed")
    return signed_tx["hex"]

def send_transaction(signed_tx):
    txid = execute_bitcoin_cli_command(['bitcoin-cli', 'sendrawtransaction', signed_tx])
    return txid

def perform_transaction(address, private_key):
    try:
        # Get first available UTXO
        utxo = get_first_utxo()
        txid = utxo['txid']
        vout = utxo['vout']
        total_amount = utxo['amount']
        print(f"Using UTXO: txid={txid}, vout={vout}, amount={total_amount} BTC")
        
        # Calculate amount and fee
        fee = 10 * 500 / 1e8  # 0.00005 BTC (10 sat/byte * 500 bytes)
        amount = total_amount - 0.00000001  # Subtract a small amount (1 satoshi)
        
        if amount <= fee:
            raise Exception("Insufficient funds to cover the fee")
        
        # Create raw transaction
        raw_tx = create_raw_transaction(txid, vout, address, amount, fee)
        print(f"Raw transaction created: {raw_tx}")
        
        # Sign transaction
        signed_tx = sign_transaction(raw_tx, private_key)
        print(f"Transaction signed: {signed_tx}")
        
        # Send transaction
        txid = send_transaction(signed_tx)
        print(f"Transaction sent: {txid}")
        
        return txid
    except Exception as e:
        print(f"Error: {e}")
        return None

# Exemplo de uso
address = "endereco_de_destino"  # Endereço para onde os bitcoins serão enviados
private_key = "sua_chave_privada"  # Chave privada do endereço de origem

perform_transaction(address, private_key)
